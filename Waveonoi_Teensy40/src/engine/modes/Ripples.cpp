// Ripples: every note drops a ring that expands across the wall from its key,
// like rain on water. Chords overlap and mix; harder notes travel faster,
// wider and brighter. The key itself glows while the note sounds.
#include "../Engine.h"
#include "../FastMath.h"

namespace wv {
namespace {

class Ripples : public Mode {
  struct Ring {
    bool on;
    float u0, v0, t, speed, width, amp;
    RGBf col;
  };
  static constexpr int kRings = 48;
  Ring rings[kRings];

 public:
  const char* name() const override { return "Ripples"; }

  void reset(Engine&) override {
    for (auto& r : rings) r.on = false;
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    Ring* r = &rings[0];
    for (auto& q : rings) {
      if (!q.on) {
        r = &q;
        break;
      }
      if (q.t > r->t) r = &q;  // full: replace the oldest ring
    }
    r->on = true;
    r->u0 = v.u;
    r->v0 = v.v - 6.f;
    r->t = 0.f;
    r->speed = 110.f + 170.f * v.vel;
    r->width = 3.5f + 6.f * v.vel;
    r->amp = 0.35f + 1.1f * v.vel * v.vel;
    r->col = v.color;
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    const float far = sqrtf(m.uMax * m.uMax + m.vMax * m.vMax) + 40.f;
    for (auto& r : rings) {
      if (!r.on) continue;
      r.t += dt;
      float rad = r.speed * r.t;
      float w = r.width;
      float a = r.amp * fexp(-r.t / 2.4f) / (1.f + rad / 140.f) * gain;
      if (a < 0.003f || rad - 12.f * w > far) {
        r.on = false;
        continue;
      }
      float outer = rad + 3.f * w, inner = rad - 10.f * w;
      float out2 = outer * outer, in2 = inner > 0.f ? inner * inner : 0.f;
      float invW = 1.f / w;
      RGBf hot = whiten(r.col, 0.6f);
      m.forEachInBox(r.u0 - outer, r.u0 + outer, r.v0 - outer, r.v0 + outer, [&](int i) {
        float du = m.u[i] - r.u0, dv = m.v[i] - r.v0;
        float d2 = du * du + dv * dv;
        if (d2 > out2 || d2 < in2) return;
        float x = (sqrtf(d2) - rad) * invW;
        float g = fexp(-x * x);
        if (x < 0.f) {
          f.add(i, r.col, a * (g + 0.18f * fexp(x * 0.35f)));
        } else {
          f.add(i, r.col, a * g);
          f.add(i, hot, a * 0.4f * g);  // whiter leading edge
        }
      });
    }
    // Light pools at the key while the note sounds.
    for (int k = 0; k < kMaxVoices; k++) {
      const Voice& v = e.voices[k];
      if (!v.alive() || !v.sounding()) continue;
      float lv = v.level * 0.35f * gain;
      m.forEachNear(v.u, v.v, 30.f, [&](int i, float d2) {
        float t = 1.f - sqrtf(d2) / 30.f;
        f.add(i, v.color, lv * t * t);
      });
    }
  }
};

Ripples instance;

}  // namespace

Mode* modeRipples() { return &instance; }

}  // namespace wv
