// Coral: notes grow a lasting structure out of their key. Each note first
// sends a pulse of sap up through what has already grown, then extends the
// structure into fresh territory in its own colour (longer while held).
// A whole piece paints the wall; when you stop playing it slowly withers.
#include "Walk.h"

namespace wv {
namespace {

RGBf col[kMaxLeds] WV_BIG;     // colour of the grown structure
float lvl[kMaxLeds] WV_BIG;    // how alive it is, 0..1
RGBf boost[kMaxLeds] WV_BIG;   // transient light: sap pulses and growing tips

constexpr float kClaimed = 0.12f;

class Coral : public Mode {
  struct Grower {
    bool on, sap;
    uint8_t vi;
    uint32_t vid;
    uint16_t cur, next;
    float p, len, speed, budget, sapLeft, amp;
    RGBf col;
  };
  static constexpr int kGrowers = 32;
  Grower gr[kGrowers];
  Rng rng;

  static bool claimed(int i) { return lvl[i] > kClaimed; }

  void paint(int i, const Grower& g) {
    col[i] = claimed(i) ? mix(col[i], g.col, 0.7f) : g.col;
    lvl[i] = 1.f;
    maxInto(boost[i], whiten(g.col, 0.5f) * g.amp);
  }

  // Next LED for a grower standing on cur, coming from prev.
  int choose(const WallMap& m, Grower& g, int prev, int cur) {
    if (g.sap) {
      int fresh = randomStep(m, rng, prev, cur, 1.f, [&](int i) { return claimed(i); });
      if (fresh != kNone || g.sapLeft <= 0.f) {
        g.sap = false;  // found the edge of the structure: start growing
        if (fresh != kNone) return fresh;
      } else {
        int n = randomStep(m, rng, prev, cur, 1.5f);
        return n;
      }
    }
    int fresh = randomStep(m, rng, prev, cur, 1.f, [&](int i) { return claimed(i); });
    if (fresh != kNone) return fresh;
    if (rng.chance(0.35f)) return kNone;  // boxed in: stop
    return randomStep(m, rng, prev, cur, 0.5f);  // grow over older parts, repainting them
  }

 public:
  const char* name() const override { return "Coral"; }

  void reset(Engine& e) override {
    for (auto& g : gr) g.on = false;
    for (int i = 0; i < e.map.n; i++) {
      col[i] = boost[i] = RGBf{0, 0, 0};
      lvl[i] = 0.f;
    }
  }

  void noteOn(Engine& e, int vi) override {
    const WallMap& m = e.map;
    const Voice& v = e.voices[vi];
    Grower* g = &gr[0];
    for (auto& q : gr) {
      if (!q.on) {
        g = &q;
        break;
      }
      if (q.budget < g->budget) g = &q;
    }
    g->on = true;
    g->vi = (uint8_t)vi;
    g->vid = v.id;
    g->col = v.color;
    g->amp = 0.6f + 0.5f * v.vel;
    g->speed = 50.f + 70.f * v.vel;
    g->budget = 20.f + 60.f * v.vel;
    g->sapLeft = 280.f;
    g->cur = (uint16_t)m.nearest(v.u, v.v);
    g->sap = claimed(g->cur);
    if (!g->sap) paint(g->cur, *g);
    int n = choose(m, *g, kNone, g->cur);
    if (n == kNone) {
      g->on = false;
      return;
    }
    g->next = (uint16_t)n;
    g->len = maxf(m.linkLen(g->cur, n), 0.5f);
    g->p = 0.f;
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    bool quiet = e.sinceNote > 6.f && !e.anySounding();
    float wither = fexp(-dt / (quiet ? 5.f : 150.f));
    float fadeBoost = fexp(-dt / 0.3f);
    for (int i = 0; i < m.n; i++) {
      lvl[i] *= wither;
      boost[i] = boost[i] * fadeBoost;
    }

    for (auto& g : gr) {
      if (!g.on) continue;
      if (voiceValid(e, g.vi, g.vid) && e.voices[g.vi].held) g.budget = minf(g.budget + g.speed * dt * 0.7f, 250.f);
      float step = (g.sap ? 330.f : g.speed) * dt;
      while (step > 0.f && g.on) {
        float rest = (1.f - g.p) * g.len;
        if (step < rest) {
          g.p += step / g.len;
          break;
        }
        step -= rest;
        int prev = g.cur;
        g.cur = g.next;
        if (g.sap) {
          maxInto(boost[g.cur], whiten(g.col, 0.4f) * (0.9f * g.amp));
          g.sapLeft -= g.len;
        } else {
          paint(g.cur, g);
          g.budget -= g.len;
          if (g.budget <= 0.f) {
            g.on = false;
            break;
          }
        }
        int n = choose(m, g, prev, g.cur);
        if (n == kNone) {
          g.on = false;
          break;
        }
        g.next = (uint16_t)n;
        g.len = maxf(m.linkLen(g.cur, n), 0.5f);
        g.p = 0.f;
      }
      if (g.on && !g.sap) maxInto(boost[g.next], whiten(g.col, 0.6f) * (g.amp * g.p));
    }

    float t = e.time * 0.4f;
    for (int i = 0; i < m.n; i++) {
      float l = lvl[i];
      if (l > 0.003f) f.add(i, col[i], gain * l * (0.26f + 0.12f * vnoise1((uint32_t)i, t)));
      f.add(i, boost[i], gain);
    }
  }
};

Coral instance;

}  // namespace

Mode* modeCoral() { return &instance; }

}  // namespace wv
