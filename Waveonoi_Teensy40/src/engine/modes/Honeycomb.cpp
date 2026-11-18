// Honeycomb: the cell above the key lights its outline, then the light hops
// outward cell by cell like falling dominoes. Harder notes reach further.
#include "Walk.h"

namespace wv {
namespace {

class Honeycomb : public Mode {
  struct Wave {
    bool on;
    uint8_t start, vi;
    uint32_t vid;
    float t, amp, hop, fall;
    RGBf col;
  };
  static constexpr int kWaves = 32;
  Wave waves[kWaves];
  RGBf acc[kMaxCells];

  static int startCell(const WallMap& m, float u, float v) {
    int i = m.nearest(u, v + 4.f);
    int best = -1;
    float bd = 1e9f;
    for (int k = 0; k < 3; k++) {
      int c = m.cellOf[i][k];
      if (c == 0xFF) continue;
      float du = m.cells[c].cu - u, dv = m.cells[c].cv - v;
      float d = du * du + dv * dv;
      if (d < bd) {
        bd = d;
        best = c;
      }
    }
    return best;
  }

 public:
  const char* name() const override { return "Honeycomb"; }

  void reset(Engine&) override {
    for (auto& w : waves) w.on = false;
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    int c = startCell(e.map, v.u, v.v);
    if (c < 0) return;
    Wave* w = &waves[0];
    for (auto& q : waves) {
      if (!q.on) {
        w = &q;
        break;
      }
      if (q.t > w->t) w = &q;
    }
    w->on = true;
    w->start = (uint8_t)c;
    w->vi = (uint8_t)vi;
    w->vid = v.id;
    w->t = 0.f;
    w->amp = 0.5f + 0.8f * v.vel;
    w->hop = 0.2f - 0.08f * v.vel;
    w->fall = 0.42f + 0.33f * v.vel;
    w->col = v.color;
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    for (int c = 0; c < m.cellCount; c++) acc[c] = RGBf{0, 0, 0};
    for (auto& w : waves) {
      if (!w.on) continue;
      w.t += dt;
      float maxHops = logf(0.01f / w.amp) / logf(w.fall);
      if (w.t > maxHops * w.hop + 2.f) {
        w.on = false;
        continue;
      }
      float sustain = 0.f;
      if (voiceValid(e, w.vi, w.vid) && e.voices[w.vi].sounding()) sustain = e.voices[w.vi].level * 0.35f;
      float pw[32];
      pw[0] = w.amp;
      for (int h = 1; h < 32; h++) pw[h] = pw[h - 1] * w.fall;
      const uint8_t* hops = m.cellHop[w.start];
      for (int c = 0; c < m.cellCount; c++) {
        int h = hops[c];
        if (h >= 32) continue;
        float x = w.t - h * w.hop;
        if (x < 0.f) continue;
        // Sharp fall-off: on the LEDs' perceptual curve even 10% linear
        // still reads as "lit", so a soft tail would flood the wall.
        float y = (x - 0.04f) * (h ? 6.5f : 2.f);
        float k = pw[h] * (x < 0.04f ? x / 0.04f : fexp(-y * y));
        if (h == 0) k += sustain;
        if (k < 0.002f) continue;
        acc[c] += whiten(w.col, 0.4f * fexp(-(float)h)) * k;
      }
    }
    for (int i = 0; i < m.n; i++) {
      RGBf best{0, 0, 0};
      for (int k = 0; k < 3; k++) {
        int c = m.cellOf[i][k];
        if (c != 0xFF) maxInto(best, acc[c]);
      }
      if (best.r + best.g + best.b > 0.f) f.add(i, best, gain);
    }
  }
};

Honeycomb instance;

}  // namespace

Mode* modeHoneycomb() { return &instance; }

}  // namespace wv
