// Interference: every sounding note is a wave source at its key, its
// wavelength set by its pitch (bass long, treble short). Where crests from
// different notes meet they add up; chords draw interference patterns across
// the wall. Dissonant intervals make the pattern shimmer and boil, consonant
// ones stay calm.
#include "Walk.h"

namespace wv {
namespace {

// Roughness of an interval (semitones mod 12): seconds, sevenths and the
// tritone are rough; fifths, fourths, thirds and sixths smooth.
const float kDiss[12] = {0.f, 1.f, 0.55f, 0.15f, 0.1f, 0.05f, 0.85f, 0.f, 0.15f, 0.1f, 0.45f, 0.9f};

class Interference : public Mode {
  struct Src {
    float u, v, k, w, front, a;
    int note;
    RGBf col;
  };
  static constexpr int kSrc = 10;

 public:
  const char* name() const override { return "Interference"; }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    Src s[kSrc];
    int ns = 0;
    for (int vi = 0; vi < kMaxVoices; vi++) {
      const Voice& v = e.voices[vi];
      if (!v.alive() || v.level < 0.01f) continue;
      Src x;
      float lambda = 130.f * fexp2(-(v.note - 21) / 24.f);
      x.u = v.u;
      x.v = v.v - 8.f;
      x.k = kTau / lambda;
      x.w = x.k * 28.f;  // crests drift outward at 28 units/s
      x.front = 160.f * v.age;
      x.a = v.level;
      x.note = v.note;
      x.col = v.color;
      if (ns < kSrc) {
        s[ns++] = x;
      } else {  // keep the loudest
        int weakest = 0;
        for (int q = 1; q < kSrc; q++)
          if (s[q].a < s[weakest].a) weakest = q;
        if (x.a > s[weakest].a) s[weakest] = x;
      }
    }
    if (!ns) return;

    float num = 0.f, den = 0.f;
    for (int a = 0; a < ns; a++)
      for (int b = a + 1; b < ns; b++) {
        float w = s[a].a * s[b].a;
        int iv = s[a].note - s[b].note;
        if (iv < 0) iv = -iv;
        num += w * kDiss[iv % 12];
        den += w;
      }
    float jit = den > 0.f ? 2.4f * num / den : 0.f;
    float t = e.time;

    for (int i = 0; i < m.n; i++) {
      float F = 0.f, W = 0.f, cw = 0.f;
      RGBf c{0, 0, 0};
      float nz = jit > 0.01f ? (vnoise1((uint32_t)i, t * 7.f) - 0.5f) * jit : 0.f;
      for (int q = 0; q < ns; q++) {
        const Src& x = s[q];
        float du = m.u[i] - x.u, dv = m.v[i] - x.v;
        float r = sqrtf(du * du + dv * dv);
        float edge = (x.front - r) * (1.f / 20.f) + 1.f;
        if (edge <= 0.f) continue;
        if (edge > 1.f) edge = 1.f;
        float att = x.a * edge / (1.f + r * (1.f / 150.f));
        float cs = fcos(x.k * r - x.w * t + nz * (1 + (q & 1)));
        F += att * cs;
        W += att;
        float wc = att * (0.5f + 0.5f * cs);
        c += x.col * wc;
        cw += wc;
      }
      if (W < 0.002f || cw < 1e-5f) continue;
      float b = (1.f + F / W) * 0.5f;
      b *= b;  // ^4: keeps dark nodal lines even when many notes overlap
      float I = b * b * minf(W * 1.5f, 1.4f) * gain;
      f.add(i, c * (1.f / cw), I);
    }
  }
};

Interference instance;

}  // namespace

Mode* modeInterference() { return &instance; }

}  // namespace wv
