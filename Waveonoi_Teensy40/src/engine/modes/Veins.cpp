// Veins: each note sends a pulse of light from its key through the lines
// themselves, splitting at every junction like roots or lightning. Junctions
// flare as the pulse front passes them. Harder = faster, brighter, further.
#include "Walk.h"

namespace wv {
namespace {

class Veins : public Mode {
  struct Pulse {
    bool on;
    int8_t slot;
    uint8_t vi;
    uint32_t vid;
    float t, speed, reach, amp;
    RGBf col;
  };
  Pulse pulses[kFieldSlots];

  void kill(Engine& e, Pulse& p) {
    e.fields.release(p.slot);
    p.on = false;
  }

 public:
  const char* name() const override { return "Veins"; }

  void reset(Engine& e) override {
    for (auto& p : pulses)
      if (p.on) kill(e, p);
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    Pulse* p = nullptr;
    for (auto& q : pulses)
      if (!q.on) {
        p = &q;
        break;
      }
    if (!p) {  // all busy: drop the oldest pulse
      p = &pulses[0];
      for (auto& q : pulses)
        if (q.t > p->t) p = &q;
      kill(e, *p);
    }
    int slot = e.fields.acquire(v.note, e.map, v.u, v.v);
    if (slot < 0) return;
    p->on = true;
    p->slot = (int8_t)slot;
    p->vi = (uint8_t)vi;
    p->vid = v.id;
    p->t = 0.f;
    p->speed = 150.f + 230.f * v.vel;
    p->reach = 70.f + 380.f * v.vel * v.vel;
    p->amp = 0.5f + 0.9f * v.vel;
    p->col = v.color;
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    const float inv = 1.f / FieldCache::kScale;
    for (auto& p : pulses) {
      if (!p.on) continue;
      p.t += dt;
      float front = p.speed * p.t;
      float a = p.amp * gain * fexp(-p.t / 4.f);
      float maxG = p.reach * logf(maxf(a, 0.0041f) / 0.004f);
      if (a < 0.004f || front > maxG + 200.f) {
        kill(e, p);
        continue;
      }
      float root = 0.f;
      if (voiceValid(e, p.vi, p.vid) && e.voices[p.vi].sounding()) root = e.voices[p.vi].level * 0.45f * gain;
      const uint16_t* F = e.fields.field(p.slot);
      RGBf hot = whiten(p.col, 0.7f);
      float invReach = 1.f / p.reach;
      for (int i = 0; i < m.n; i++) {
        float g = F[i] * inv;
        if (root > 0.f && g < 25.f) {
          float t = 1.f - g / 25.f;
          f.add(i, p.col, root * t * t);
        }
        float x = g - front;
        if (x > 8.f || x < -250.f) continue;
        float I = a * fexp(-g * invReach);
        float head = fexp(-x * x * (1.f / 16.f));
        float tail = x < 0.f ? 0.3f * fexp(x * (1.f / 45.f)) : 0.f;
        f.add(i, p.col, I * (head + tail));
        f.add(i, hot, I * head * (0.25f + 1.6f * m.jglow[i]));
      }
    }
  }
};

Veins instance;

}  // namespace

Mode* modeVeins() { return &instance; }

}  // namespace wv
