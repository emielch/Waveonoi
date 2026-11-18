// Rising notes: a column of light grows upward from the key for as long as it
// is held, then detaches and floats to the top - a piano roll in reverse.
#include "Walk.h"

namespace wv {
namespace {

class Rising : public Mode {
  struct Bar {
    bool on;
    uint8_t vi;
    uint32_t vid;
    float u0, v0, t, tRel, speed, w, amp;
    RGBf col;
  };
  static constexpr int kBars = 64;
  Bar bars[kBars];

 public:
  const char* name() const override { return "Rising notes"; }

  void reset(Engine&) override {
    for (auto& b : bars) b.on = false;
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    Bar* b = &bars[0];
    for (auto& q : bars) {
      if (!q.on) {
        b = &q;
        break;
      }
      if (q.t > b->t) b = &q;
    }
    b->on = true;
    b->vi = (uint8_t)vi;
    b->vid = v.id;
    b->u0 = v.u;
    b->v0 = v.v - 4.f;
    b->t = 0.f;
    b->tRel = -1.f;
    b->speed = 85.f + 45.f * v.vel;
    b->w = 3.2f + 2.4f * v.vel;
    b->amp = 0.45f + 0.75f * v.vel;
    b->col = v.color;
  }

  void keyUp(Engine& e, int vi) override {
    for (auto& b : bars)
      if (b.on && b.vid == e.voices[vi].id && b.tRel < 0.f) b.tRel = b.t;
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    for (auto& b : bars) {
      if (!b.on) continue;
      b.t += dt;
      // Detach if the key is up (also when its voice got reused or a note hangs).
      if (b.tRel < 0.f && (!voiceValid(e, b.vi, b.vid) || !e.voices[b.vi].held || b.t > 30.f)) b.tRel = b.t;
      float top = b.v0 + b.speed * b.t;
      float bot = b.tRel < 0.f ? b.v0 : b.v0 + b.speed * (b.t - b.tRel);
      if (bot > m.vMax + 30.f) {
        b.on = false;
        continue;
      }
      float a = b.amp * gain * (0.6f + 0.4f * fexp(-b.t / 0.6f));
      float halo = 3.5f * b.w;
      float invW2 = 1.f / (b.w * b.w), invH2 = 1.f / (halo * halo);
      RGBf hot = whiten(b.col, 0.6f);
      m.forEachInBox(b.u0 - 2.f * halo, b.u0 + 2.f * halo, bot - 14.f, top + 10.f, [&](int i) {
        float du = m.u[i] - b.u0, v = m.v[i];
        float across = fexp(-du * du * invW2) + 0.15f * fexp(-du * du * invH2);
        float along;
        if (v > top) {
          float x = (v - top) * 0.25f;
          along = fexp(-x * x);
        } else if (v < bot) {
          float x = (bot - v) * (1.f / 9.f);
          along = fexp(-x * x);
        } else {
          along = 1.f;
        }
        float hx = (v - top) * (1.f / 6.f);
        float head = fexp(-hx * hx);
        f.add(i, b.col, a * across * along);
        f.add(i, hot, a * across * head * 0.6f);
      });
    }
  }
};

Rising instance;

}  // namespace

Mode* modeRising() { return &instance; }

}  // namespace wv
