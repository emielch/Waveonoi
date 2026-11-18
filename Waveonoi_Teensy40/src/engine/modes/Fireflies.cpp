// Fireflies: each note releases a glowing comet that wanders the network,
// picking a random branch at every junction and breathing softly. It lives
// as long as the note sounds, so with the sustain pedal down the wall fills
// up; lifting the pedal lets them all fade away.
#include "Walk.h"

namespace wv {
namespace {

RGBf glow[kMaxLeds] WV_BIG;

class Fireflies : public Mode {
  struct Fly {
    bool on, released;
    uint8_t vi;
    uint32_t vid;
    uint16_t prev, cur, next;
    float p, len, speed, amp, phase, fade, age;
    RGBf col;
  };
  static constexpr int kFlies = 64;
  Fly flies[kFlies];
  Rng rng;

 public:
  const char* name() const override { return "Fireflies"; }

  void reset(Engine& e) override {
    for (auto& f : flies) f.on = false;
    for (int i = 0; i < e.map.n; i++) glow[i] = RGBf{0, 0, 0};
  }

  void noteOn(Engine& e, int vi) override {
    const WallMap& m = e.map;
    const Voice& v = e.voices[vi];
    Fly* f = &flies[0];
    for (auto& q : flies) {
      if (!q.on) {
        f = &q;
        break;
      }
      if (q.age > f->age) f = &q;
    }
    f->on = true;
    f->released = false;
    f->vi = (uint8_t)vi;
    f->vid = v.id;
    f->cur = (uint16_t)m.nearest(v.u, v.v);
    f->prev = kNone;
    int next = randomStep(m, rng, kNone, f->cur, 3.f);
    f->next = (uint16_t)(next == kNone ? f->cur : next);
    f->len = maxf(m.linkLen(f->cur, f->next), 0.5f);
    f->p = 0.f;
    f->speed = 40.f + 120.f * v.vel;
    f->amp = 0.5f + 0.6f * v.vel;
    f->phase = rng.range(0.f, kTau);
    f->fade = 1.f;
    f->age = 0.f;
    f->col = v.color;
  }

  void release(Engine& e, int vi) override {
    for (auto& f : flies)
      if (f.on && f.vid == e.voices[vi].id) f.released = true;
  }

  void render(Engine& e, Frame& fr, float dt, float gain) override {
    const WallMap& m = e.map;
    float decay = fexp(-dt / 0.45f);
    for (int i = 0; i < m.n; i++) glow[i] = glow[i] * decay;
    for (auto& f : flies) {
      if (!f.on) continue;
      f.age += dt;
      if (!voiceValid(e, f.vi, f.vid) || f.age > 45.f) f.released = true;
      if (f.released) {
        f.fade -= dt / 1.2f;
        if (f.fade <= 0.f) {
          f.on = false;
          continue;
        }
      }
      float I = f.amp * f.fade * (0.55f + 0.45f * fsin(f.age * kTau * 0.55f + f.phase));
      RGBf c = f.col * I;
      float step = f.speed * dt;
      while (step > 0.f) {
        float rest = (1.f - f.p) * f.len;
        if (step < rest) {
          f.p += step / f.len;
          break;
        }
        step -= rest;
        maxInto(glow[f.next], c);
        int nx = randomStep(m, rng, f.cur, f.next, f.age < 1.5f ? 2.f : 0.f);
        f.prev = f.cur;
        f.cur = f.next;
        f.next = (uint16_t)(nx == kNone ? f.prev : nx);  // dead end: turn around
        f.len = maxf(m.linkLen(f.cur, f.next), 0.5f);
        f.p = 0.f;
      }
      maxInto(glow[f.cur], c * (1.f - f.p));
      maxInto(glow[f.next], c * f.p);
      // bright head on top of the trail
      RGBf hot = whiten(f.col, 0.5f);
      fr.add(f.cur, hot, I * 0.5f * (1.f - f.p) * gain);
      fr.add(f.next, hot, I * 0.5f * f.p * gain);
    }
    for (int i = 0; i < m.n; i++) fr.add(i, glow[i], gain);
  }
};

Fireflies instance;

}  // namespace

Mode* modeFireflies() { return &instance; }

}  // namespace wv
