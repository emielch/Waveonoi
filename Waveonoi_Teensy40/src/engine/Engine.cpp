#include "Engine.h"

#include <string.h>

#include "../LED.h"
#include "FastMath.h"

namespace wv {

// Distance-field cache; doubles as scratch memory while the map is built.
static uint16_t fieldMemory[kFieldSlots * kMaxLeds] WV_BIG;

static constexpr int kLowKey = 21;    // A0
static constexpr int kHighKey = 108;  // C8
static constexpr float kCrossfade = 0.8f;

void Settings::setDefaults() {
  magic = kMagic;
  // Calibrated on the wall with the RD-150 in front of it.
  keyA0U = 110.f;
  keyC8U = 280.f;
  keyV = 20.f;
  flipU = 0;
  flipV = 0;
  mode = 0;
  pcSwitch = 0;
  brightness = 1.f;
  idleSeconds = 20.f;
}

void Engine::begin(const LED* leds, int count, int ledsPerStrip, const Settings& s) {
  settings = s;
  layout = leds;
  layoutCount = count;
  layoutPerStrip = ledsPerStrip;

  // Output curve: linear light -> CIE L* bytes (the LED library undoes L*).
  for (int k = 0; k < 4096; k++) {
    float y = k / 4095.f;
    float L = y > 0.008856f ? 116.f * cbrtf(y) - 16.f : 903.3f * y;
    int b = (int)(L * 2.55f + 0.5f);
    lut[k] = (uint8_t)(b > 255 ? 255 : b);
  }

  nModes = 0;
  Mode* (*const factories[])() = {modeRipples,   modeVeins,     modeRising,      modeSparks,
                                  modeLightning, modeFireflies, modeCoral,       modeHoneycomb,
                                  modeInterference, modeResonance};
  for (auto f : factories)
    if (nModes < kMaxModes) modes[nModes++] = f();

  cur = settings.mode < nModes ? settings.mode : 0;
  prev = -1;
  xfade = 1.f;
  rebuildMap();
}

void Engine::rebuildMap() {
  for (int i = 0; i < kMaxVoices; i++) voices[i] = Voice();
  memset(keyDown, 0, sizeof(keyDown));
  memset(swallowed, 0, sizeof(swallowed));
  pedalDown = false;
  map.build(layout, layoutCount, layoutPerStrip, settings.flipU, settings.flipV, fieldMemory,
            FieldCache::bytes());
  fields.begin(fieldMemory);  // after build: the scratch contents are now garbage
  frame.n = map.n;
  frame.clear();
  for (int m = 0; m < nModes; m++) modes[m]->reset(*this);
}

void Engine::keysChanged() {
  fields.invalidate();
  for (int i = 0; i < kMaxVoices; i++) {
    voices[i].u = keyU(voices[i].note);
    voices[i].v = settings.keyV;
  }
}

float Engine::keyU(int note) const {
  float t = (note - kLowKey) / (float)(kHighKey - kLowKey);
  return clampf(settings.keyA0U + t * (settings.keyC8U - settings.keyA0U), 0.f, map.uMax);
}

const char* Engine::modeName(int m) const {
  return m >= 0 && m < nModes ? modes[m]->name() : "?";
}

void Engine::setMode(int m, bool flash, bool instant) {
  if (m < 0 || m >= nModes) return;
  if (flash) flashT = 0.f;
  if (m == cur) return;
  if (prev >= 0) modes[prev]->reset(*this);
  prev = cur;
  cur = m;
  xfade = 0.f;
  modes[cur]->reset(*this);
  settings.mode = (uint8_t)m;
  if (instant) {
    modes[prev]->reset(*this);
    prev = -1;
    xfade = 1.f;
  }
}

// ---------------------------------------------------------------- input

static int whiteIndex(int note) {
  static const int8_t k[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
  int w = k[note % 12];
  return w < 0 ? -1 : (note / 12) * 7 + w;
}

// Hold A0 + B0 and tap a white key from middle C up to pick an animation
// (C4 = 1st, D4 = 2nd, ...). C8 toggles the built-in demo.
bool Engine::gesture(int note) {
  if (!keyDown[kLowKey] || !keyDown[kLowKey + 2] || note == kLowKey || note == kLowKey + 2)
    return false;
  if (note == kHighKey) {
    if (demo.active())
      demo.stop(*this);
    else
      demo.start(nextId);
    flashT = 0.f;
    return true;
  }
  int w = whiteIndex(note);
  int idx = w - whiteIndex(60);
  if (w < 0 || idx < 0 || idx >= nModes) return false;
  setMode(idx);
  settingsDirty = true;
  return true;
}

void Engine::noteOn(int note, int velocity) {
  if (note < 0 || note > 127) return;
  if (velocity <= 0) {
    noteOff(note);
    return;
  }
  sinceNote = 0.f;
  if (gesture(note)) {
    swallowed[note] = true;
    return;
  }
  keyDown[note] = true;

  // Re-striking a note that still rings: the old voice fades out.
  for (int i = 0; i < kMaxVoices; i++) {
    Voice& v = voices[i];
    if (v.alive() && v.note == note && v.relAge < 0.f) {
      if (v.held) forActive([&](Mode* m) { m->keyUp(*this, i); });
      v.held = v.sustained = false;
      releaseVoice(i);
    }
  }

  int slot = -1;
  for (int i = 0; i < kMaxVoices && slot < 0; i++)
    if (!voices[i].alive()) slot = i;
  for (int pass = 0; pass < 2 && slot < 0; pass++) {
    float best = 1e9f;
    for (int i = 0; i < kMaxVoices; i++)
      if ((pass == 1 || !voices[i].held) && voices[i].level < best) {
        best = voices[i].level;
        slot = i;
      }
  }
  Voice& v = voices[slot];
  v = Voice();
  v.id = nextId++;
  if (nextId == 0) nextId = 1;
  v.note = (uint8_t)note;
  v.vel = velocity / 127.f;
  v.held = true;
  v.u = keyU(note);
  v.v = settings.keyV;
  v.color = noteColor(note);
  v.level = 0.25f + 0.75f * v.vel * sqrtf(v.vel);
  forActive([&](Mode* m) { m->noteOn(*this, slot); });
}

void Engine::noteOff(int note) {
  if (note < 0 || note > 127) return;
  if (swallowed[note]) {
    swallowed[note] = false;
    return;
  }
  keyDown[note] = false;
  sinceNote = 0.f;
  for (int i = 0; i < kMaxVoices; i++) {
    Voice& v = voices[i];
    if (!v.alive() || v.note != note || !v.held) continue;
    v.held = false;
    forActive([&](Mode* m) { m->keyUp(*this, i); });
    if (pedalDown)
      v.sustained = true;
    else
      releaseVoice(i);
  }
}

void Engine::releaseVoice(int i) {
  voices[i].relAge = 0.f;
  forActive([&](Mode* m) { m->release(*this, i); });
}

void Engine::sustain(bool down) {
  if (down == pedalDown) return;
  pedalDown = down;
  sinceNote = 0.f;
  forActive([&](Mode* m) { m->pedal(*this, down); });
  if (down) return;
  for (int i = 0; i < kMaxVoices; i++)
    if (voices[i].alive() && voices[i].sustained) {
      voices[i].sustained = false;
      releaseVoice(i);
    }
}

void Engine::programChange(int program) {
  if (!settings.pcSwitch || nModes == 0) return;
  setMode(program % nModes);
  settingsDirty = true;
}

void Engine::allNotesOff() {
  for (int n = 0; n < 128; n++)
    if (keyDown[n]) noteOff(n);
  sustain(false);
}

bool Engine::anySounding() const {
  for (int i = 0; i < kMaxVoices; i++)
    if (voices[i].alive() && voices[i].sounding()) return true;
  return false;
}

// Bass strings ring for many seconds, the top octave for well under one.
float Engine::decayTime(int note) const { return 9.f * fexp2(-(note - kLowKey) / 22.f); }

void Engine::updateVoices(float dt) {
  for (int i = 0; i < kMaxVoices; i++) {
    Voice& v = voices[i];
    if (!v.alive()) continue;
    v.age += dt;
    if (v.relAge >= 0.f) v.relAge += dt;
    float level = (0.25f + 0.75f * v.vel * sqrtf(v.vel)) * fexp(-v.age / decayTime(v.note));
    if (v.relAge >= 0.f) level *= fexp(-v.relAge / 0.15f);
    v.level = level;
    if (v.relAge >= 0.f && (level < 0.002f || v.relAge > 3.f)) v.id = 0;
  }
}

// ---------------------------------------------------------------- frame

void Engine::render(float dt) {
  dt = clampf(dt, 0.f, 0.05f);
  time += dt;
  sinceNote += dt;
  flashT += dt;
  demo.update(*this, dt);
  updateVoices(dt);
  frame.clear();
  if (test != kTestOff) {
    renderTest();
    return;
  }

  if (prev >= 0) {
    xfade += dt / kCrossfade;
    if (xfade >= 1.f) {
      modes[prev]->reset(*this);
      prev = -1;
    }
  }
  if (prev >= 0) {
    float x = smooth01(xfade);
    modes[prev]->render(*this, frame, dt, 1.f - x);
    modes[cur]->render(*this, frame, dt, x);
  } else {
    modes[cur]->render(*this, frame, dt, 1.f);
  }

  renderIdle(dt);
  if (flashT < 0.8f) {
    float k = 0.3f * fexp(-flashT / 0.12f);
    for (int i = 0; i < map.n; i++) frame.add(i, RGBf{1.f, 1.f, 1.f}, k);
  }
}

// Ambient "embers" after a stretch of silence: slow warm noise drifting
// through the lines, gone the moment a key is touched.
void Engine::renderIdle(float dt) {
  bool idle = sinceNote > settings.idleSeconds && !anySounding() && !demo.active();
  float target = idle ? 1.f : 0.f;
  float tau = target > idleLevel ? 4.f : 0.25f;
  idleLevel += (target - idleLevel) * (1.f - fexp(-dt / tau));
  if (idleLevel < 0.002f) return;
  float drift = 0.05f * fsin(time * kTau / 420.f);
  for (int i = 0; i < map.n; i++) {
    float n1 = vnoise3(map.u[i] * 0.022f, map.v[i] * 0.022f, time * 0.05f);
    float n2 = vnoise3(map.u[i] * 0.06f + 31.f, map.v[i] * 0.06f + 17.f, time * 0.12f);
    float heat = smooth01((n1 - 0.3f) * 2.f) * (0.55f + 0.45f * n2);
    frame.add(i, hueColor(drift + 0.1f * heat), idleLevel * (0.006f + 0.07f * heat * heat));
  }
}

void Engine::renderTest() {
  const RGBf red{1, 0, 0}, green{0, 1, 0}, blue{0, 0, 1}, white{1, 1, 1};
  if (test == kTestOrientation) {
    // red = bottom, green = left, blue = top, white bar sweeps left -> right
    float bar = fractf(time / 3.f) * map.uMax;
    for (int i = 0; i < map.n; i++) {
      float u = map.u[i], v = map.v[i];
      if (v < 18.f) frame.add(i, red, 0.6f);
      if (u < 18.f) frame.add(i, green, 0.6f);
      if (v > map.vMax - 18.f) frame.add(i, blue, 0.6f);
      if (fabsf(u - bar) < 4.f) frame.add(i, white, 0.8f);
    }
    return;
  }
  // Calibration: red = A0, green = middle C, blue = C8, white = keys held now.
  float a0 = keyU(21), c4 = keyU(60), c8 = keyU(108);
  for (int i = 0; i < map.n; i++) {
    float u = map.u[i], v = map.v[i];
    if (fabsf(u - a0) < 2.5f) frame.add(i, red, 0.7f);
    if (fabsf(u - c4) < 2.5f) frame.add(i, green, 0.7f);
    if (fabsf(u - c8) < 2.5f) frame.add(i, blue, 0.7f);
    if (fabsf(v - settings.keyV) < 3.f) frame.add(i, white, 0.12f);
    for (int k = 0; k < kMaxVoices; k++)
      if (voices[k].alive() && voices[k].held && fabsf(u - voices[k].u) < 2.5f)
        frame.add(i, white, 0.7f);
  }
}

void Engine::pixel(int i, uint8_t& r, uint8_t& g, uint8_t& b) const {
  RGBf c = frame.px[i];
  c.r = maxf(c.r, 0.f);
  c.g = maxf(c.g, 0.f);
  c.b = maxf(c.b, 0.f);
  // Over-bright light keeps its hue and spills towards white.
  float m = maxf(c.r, maxf(c.g, c.b));
  if (m > 1.f) {
    c = c * (1.f / m);
    float w = minf(1.f, (m - 1.f) * 0.35f);
    c.r += (1.f - c.r) * w;
    c.g += (1.f - c.g) * w;
    c.b += (1.f - c.b) * w;
  }
  float k = master * settings.brightness * 4095.f;
  auto q = [&](float x) {
    int idx = (int)(x * k + 0.5f);
    return lut[idx > 4095 ? 4095 : idx];
  };
  r = q(c.r);
  g = q(c.g);
  b = q(c.b);
}

}  // namespace wv
