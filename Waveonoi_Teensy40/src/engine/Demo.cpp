#include "Demo.h"

#include "Engine.h"

namespace wv {

static constexpr float kStep = 0.15f;  // one 16th note
static constexpr int kStepsPerBar = 16;

// i - VI - III - VII in A minor, then iv - V
static const int8_t kChords[][4] = {
    {57, 60, 64, 45},  // Am  (A3 C4 E4, bass A2)
    {53, 57, 60, 41},  // F
    {48, 52, 55, 36},  // C
    {55, 59, 62, 43},  // G
    {50, 53, 57, 38},  // Dm
    {52, 56, 59, 40},  // E
};
static const int8_t kProgression[] = {0, 1, 2, 3, 0, 1, 4, 5};
static const int8_t kScale[] = {0, 2, 3, 5, 7, 8, 11};  // A harmonic minor, from A

void Demo::start(uint32_t seed) {
  on = true;
  t = 0.f;
  next = 0.f;
  step = 0;
  nOff = 0;
  rng.s = seed * 2654435761U + 1;
}

void Demo::stop(Engine& e) {
  if (!on) return;
  on = false;
  for (int k = 0; k < nOff; k++) e.noteOff(offs[k].note);
  nOff = 0;
  e.sustain(false);
}

void Demo::play(Engine& e, int note, int vel, float dur) {
  if (note < 21 || note > 108) return;
  for (int k = 0; k < nOff; k++)
    if (offs[k].note == note) {  // re-strike: release the old one first
      e.noteOff(note);
      offs[k] = offs[--nOff];
      break;
    }
  if (nOff >= 64) return;
  e.noteOn(note, vel < 1 ? 1 : (vel > 127 ? 127 : vel));
  offs[nOff++] = Off{t + dur, (uint8_t)note};
}

void Demo::update(Engine& e, float dt) {
  if (!on) return;
  t += dt;
  for (int k = 0; k < nOff;) {
    if (offs[k].t <= t) {
      e.noteOff(offs[k].note);
      offs[k] = offs[--nOff];
    } else {
      k++;
    }
  }
  while (t >= next) {
    int bar = step / kStepsPerBar;
    int s = step % kStepsPerBar;
    const int8_t* ch = kChords[kProgression[bar % 8]];
    int section = (bar / 8) % 3;  // 0 calm, 1 flowing, 2 bright
    if (s == 0) {
      e.sustain(false);
      e.sustain(true);
      play(e, ch[3], 70 + rng.below(30), kStep * 15);
      for (int k = 0; k < 3; k++) play(e, ch[k], 45 + rng.below(25) + section * 10, kStep * 14);
    }
    bool run = section == 2 && bar % 4 == 3 && s >= 8;
    if (run) {
      // fast run upwards through the scale, two notes per step
      for (int h = 0; h < 2; h++) {
        int idx = (s - 8) * 2 + h;
        int note = 69 + 12 * (idx / 7) + kScale[idx % 7];
        play(e, note, 60 + idx * 3, 0.1f);
      }
    } else {
      float density = section == 0 ? 0.3f : (section == 1 ? 0.55f : 0.75f);
      if ((s % 2 == 0 && rng.chance(density + 0.25f)) || rng.chance(density * 0.3f)) {
        int k = rng.below(3);
        int oct = 12 * (1 + rng.below(section == 2 ? 3 : 2));
        int vel = 40 + rng.below(50) + section * 15;
        play(e, ch[k] + oct, vel, kStep * (1 + rng.below(3)));
      }
      if (section == 2 && s == 8 && rng.chance(0.5f)) play(e, ch[3] + 12, 110, kStep * 6);
    }
    step++;
    next += kStep;
  }
}

}  // namespace wv
