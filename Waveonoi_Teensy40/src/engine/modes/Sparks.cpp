// Sparks: every note throws a burst of glowing particles up from its key that
// arc and fall back under gravity, leaving short trails where they cross the
// lines. Hard notes also launch a shell that bursts at the top of its flight.
#include "Walk.h"

namespace wv {
namespace {

RGBf glow[kMaxLeds] WV_BIG;

class Sparks : public Mode {
  struct P {
    float u, v, vu, vv, age, life, amp, seed;
    RGBf col;
    bool shell;
  };
  static constexpr int kMax = 420;
  P parts[kMax];
  int np = 0;
  Rng rng;

  void add(const P& p) {
    if (np < kMax) parts[np++] = p;
  }

  void burst(const P& s) {
    for (int k = 0; k < 22; k++) {
      float ang = rng.range(0.f, kTau), sp = rng.range(50.f, 150.f);
      add(P{s.u, s.v, fcos(ang) * sp, fsin(ang) * sp + 30.f, 0.f, rng.range(0.8f, 1.6f), 0.9f,
            rng.range(0.f, 100.f), whiten(s.col, rng.range(0.1f, 0.5f)), false});
    }
  }

 public:
  const char* name() const override { return "Sparks"; }

  void reset(Engine& e) override {
    np = 0;
    for (int i = 0; i < e.map.n; i++) glow[i] = RGBf{0, 0, 0};
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    int n = 7 + (int)(24.f * v.vel);
    float hue = noteHue(v.note);
    for (int k = 0; k < n; k++) {
      float ang = kPi * 0.5f + rng.range(-0.55f, 0.55f);
      float sp = (120.f + 280.f * v.vel) * rng.range(0.5f, 1.f);
      RGBf c = hueColor(hue + rng.range(-0.03f, 0.03f));
      if (v.note > 80) c = v.color;
      add(P{v.u + rng.range(-3.f, 3.f), v.v - 2.f, fcos(ang) * sp, fsin(ang) * sp, 0.f, rng.range(0.9f, 1.9f),
            0.5f + 0.6f * v.vel, rng.range(0.f, 100.f), c, false});
    }
    if (v.vel > 0.7f)
      add(P{v.u, v.v, rng.range(-20.f, 20.f), 280.f + 180.f * v.vel, 0.f, 3.f, 1.f, 0.f, whiten(v.color, 0.3f), true});
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    float decay = fexp(-dt / 0.14f);
    for (int i = 0; i < m.n; i++) glow[i] = glow[i] * decay;

    for (int k = 0; k < np;) {
      P& p = parts[k];
      float pu = p.u, pv = p.v;
      p.vv -= 240.f * dt;
      p.vu *= 1.f - 0.6f * dt;
      p.u += p.vu * dt;
      p.v += p.vv * dt;
      p.age += dt;
      if (p.shell && p.vv < 40.f) {
        P s = p;
        parts[k] = parts[--np];
        burst(s);
        continue;
      }
      if (p.age > p.life || p.v < -10.f || p.u < -30.f || p.u > m.uMax + 30.f) {
        parts[k] = parts[--np];
        continue;
      }
      float life = 1.f - p.age / p.life;
      float I = p.amp * life * sqrtf(life) * (p.shell ? 1.f : 0.75f + 0.25f * fsin(p.age * 40.f + p.seed));
      float du = p.u - pu, dv = p.v - pv;
      int steps = (int)(sqrtf(du * du + dv * dv) * 0.5f) + 1;
      for (int s = 1; s <= steps; s++) {
        float t = (float)s / steps;
        m.forEachNear(pu + du * t, pv + dv * t, 4.5f, [&](int i, float d2) {
          maxInto(glow[i], p.col * (I * fexp(-d2 * (1.f / 9.7f))));
        });
      }
      k++;
    }
    for (int i = 0; i < m.n; i++) f.add(i, glow[i], gain);
  }
};

Sparks instance;

}  // namespace

Mode* modeSparks() { return &instance; }

}  // namespace wv
