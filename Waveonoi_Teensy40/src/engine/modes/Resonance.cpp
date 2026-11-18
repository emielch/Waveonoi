// Resonance: every line segment between two junctions is a string, tuned by
// its length. A note plucks the segments near its key, and segments anywhere
// on the wall tuned to that note (or its fifth or third) ring in sympathy,
// the excitation spreading outward from the key. Each string shows a
// standing wave: bright bellies separated by dark nodes.
#include "Walk.h"

namespace wv {
namespace {

constexpr float kSpread = 230.f;  // units/s the excitation travels

class Resonance : public Mode {
  struct Slot {
    uint8_t vi, m;
    uint32_t vid;
    float s, amp, t0;
    RGBf col;
  };
  struct Pluck {
    bool on;
    uint8_t vi;
    uint32_t vid;
    int pc;
    float u, v, t;
    RGBf col;
  };
  Slot slots[kMaxEdges][2];
  Pluck plucks[24];
  float tune[kMaxEdges];   // pitch class of each string, 0..12
  float phase[kMaxEdges];

  static float circ(float d) {  // wrap to [-6, 6)
    d = fmodf(d + 6.f, 12.f);
    if (d < 0.f) d += 12.f;
    return d - 6.f;
  }

  void excite(Engine& e, int ei, const Pluck& p, float s, int mode) {
    Slot* best = &slots[ei][0];
    for (int k = 0; k < 2; k++) {
      Slot& q = slots[ei][k];
      if (q.vid == p.vid) {
        best = &q;
        break;
      }
      if (q.amp < best->amp) best = &q;
    }
    if (best->vid != p.vid && best->amp > s) return;
    best->vi = p.vi;
    best->vid = p.vid;
    best->m = (uint8_t)mode;
    best->s = s;
    best->amp = s;
    best->t0 = e.time;
    best->col = p.col;
  }

  void arrive(Engine& e, int ei, const Pluck& p, float dist) {
    const Edge& ed = e.map.edges[ei];
    int lobes = (int)(ed.len / 18.f + 0.5f);
    lobes = lobes < 1 ? 1 : (lobes > 4 ? 4 : lobes);
    float direct = clampf(1.f - dist / 60.f, 0.f, 1.f);
    // sympathetic: unison/octaves, fifth (3rd harmonic), major third (5th)
    float d1 = circ(p.pc - tune[ei]), d3 = circ(p.pc - tune[ei] - 7.02f), d5 = circ(p.pc - tune[ei] - 3.86f);
    float s1 = fexp(-d1 * d1 * 5.f), s3 = 0.6f * fexp(-d3 * d3 * 5.f), s5 = ed.len > 35.f ? 0.35f * fexp(-d5 * d5 * 5.f) : 0.f;
    float sym = s1;
    int mode = lobes;
    if (s3 > sym) {
      sym = s3;
      mode = 3;
    }
    if (s5 > sym) {
      sym = s5;
      mode = 5;
    }
    sym *= 0.6f + 0.4f * fexp(-dist / 250.f);
    float s = maxf(direct, sym);
    if (direct >= sym) mode = lobes;
    if (s > 0.06f) excite(e, ei, p, s, mode);
  }

 public:
  const char* name() const override { return "Resonance"; }

  void reset(Engine& e) override {
    const WallMap& m = e.map;
    for (int ei = 0; ei < m.edgeCount; ei++) {
      float p = 12.f * log2f(100.f / maxf(m.edges[ei].len, 1.f));
      tune[ei] = fmodf(fmodf(p, 12.f) + 12.f, 12.f);
      phase[ei] = hash01((uint32_t)ei * 7919U) * kTau;
      for (auto& s : slots[ei]) s = Slot{0, 1, 0, 0.f, 0.f, 0.f, RGBf{0, 0, 0}};
    }
    for (auto& p : plucks) p.on = false;
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    Pluck* p = &plucks[0];
    for (auto& q : plucks) {
      if (!q.on) {
        p = &q;
        break;
      }
      if (q.t > p->t) p = &q;
    }
    *p = Pluck{true, (uint8_t)vi, v.id, v.note % 12, v.u, v.v, 0.f, v.color};
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    for (auto& p : plucks) {
      if (!p.on) continue;
      float t0 = p.t;
      p.t += dt;
      bool first = t0 == 0.f;
      for (int ei = 0; ei < m.edgeCount; ei++) {
        const Edge& ed = m.edges[ei];
        if (ed.len < 10.f) continue;
        float du = ed.mu - p.u, dv = ed.mv - p.v;
        float d = sqrtf(du * du + dv * dv);
        float ta = d / kSpread;
        if ((ta > t0 || first) && ta <= p.t) arrive(e, ei, p, d);
      }
      if (p.t > 600.f / kSpread) p.on = false;
    }

    float t = e.time;
    for (int ei = 0; ei < m.edgeCount; ei++) {
      const Edge& ed = m.edges[ei];
      for (auto& s : slots[ei]) {
        if (s.amp <= 0.f) continue;
        if (voiceValid(e, s.vi, s.vid)) {
          s.amp = s.s * e.voices[s.vi].level;
        } else {
          s.amp *= fexp(-dt / 0.25f);
          if (s.amp < 0.003f) {
            s.amp = 0.f;
            s.vid = 0;
            continue;
          }
        }
        float A = s.amp * minf(1.f, (t - s.t0) / 0.04f) * gain;
        float osc = 0.72f + 0.28f * fcos(kTau * (1.5f + 1.2f * s.m) * t + phase[ei]);
        RGBf hot = whiten(s.col, 0.5f);
        for (int k = 0; k < ed.count; k++) {
          int led = m.edgeLeds[ed.first + k];
          float sh = fabsf(fsin(kPi * s.m * m.edgePos[led]));
          float I = A * osc * sh * sqrtf(sh);
          f.add(led, s.col, I);
          f.add(led, hot, I * sh * 0.25f);
        }
      }
    }
  }
};

Resonance instance;

}  // namespace

Mode* modeResonance() { return &instance; }

}  // namespace wv
