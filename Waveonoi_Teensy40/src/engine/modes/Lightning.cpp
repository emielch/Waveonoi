// Lightning: a note fires a bolt from its key up through the network: a fast
// dim leader finds a path towards the top, then the whole channel flashes
// white-hot, flickers with return strokes and glows out in the note's colour.
// Side branches fork off at junctions. Staccato playing becomes a storm.
#include "Walk.h"

namespace wv {
namespace {

constexpr int kBolts = 10;
constexpr int kPath = 400;

struct Bolt {
  bool on;
  float t, leader, amp, mainLen;
  RGBf col, hot;
  int n, ns;
  float strikes[4];
  uint16_t led[kPath];
  uint16_t d[kPath];  // distance from the key, 1/2 units
  uint8_t branch[kPath];
};

Bolt bolts[kBolts] WV_BIG;
float topDist[kMaxLeds] WV_BIG;  // distance along the strips to the top edge
uint8_t visited[kMaxLeds] WV_BIG;

class Lightning : public Mode {
  Rng rng;

  void push(Bolt& b, int led, float dist, bool br) {
    if (b.n >= kPath) return;
    b.led[b.n] = (uint16_t)led;
    b.d[b.n] = (uint16_t)minf(dist * 2.f, 65535.f);
    b.branch[b.n] = br;
    b.n++;
    visited[led] = 1;
  }

  void generate(const WallMap& m, Bolt& b, const Voice& v) {
    memset(visited, 0, m.n);
    b.n = 0;
    float maxLen = 120.f + 420.f * v.vel;
    int cur = m.nearest(v.u, v.v);
    push(b, cur, 0.f, false);
    struct Fork {
      uint16_t led, from;
      float d;
    } forks[8];
    int nf = 0;
    float dist = 0.f;
    while (b.n < kPath && dist < maxLen && topDist[cur] > 3.f) {
      int c[kMaxNbr], k = 0;
      for (int q = 0; q < m.deg[cur]; q++)
        if (!visited[m.nbr[cur][q]]) c[k++] = m.nbr[cur][q];
      if (!k) break;
      int pick = 0;
      if (k > 1) {
        if (rng.chance(0.12f)) {
          pick = rng.below(k);
        } else {
          float best = 1e9f;
          for (int q = 0; q < k; q++) {
            float s = topDist[c[q]] + rng.range(0.f, 18.f);
            if (s < best) {
              best = s;
              pick = q;
            }
          }
        }
        for (int q = 0; q < k; q++)
          if (q != pick && nf < 8 && rng.chance(0.25f + 0.4f * v.vel)) forks[nf++] = {(uint16_t)c[q], (uint16_t)cur, dist};
      }
      dist += m.linkLen(cur, c[pick]);
      cur = c[pick];
      push(b, cur, dist, false);
    }
    b.mainLen = dist;
    for (int fk = 0; fk < nf; fk++) {
      int at = forks[fk].led, prev = forks[fk].from;
      if (visited[at]) continue;
      float bd = forks[fk].d + m.linkLen(prev, at);
      float blen = rng.range(12.f, 30.f + 70.f * v.vel), walked = 0.f;
      push(b, at, bd, true);
      while (walked < blen && b.n < kPath) {
        int next = randomStep(m, rng, prev, at, 1.2f, [&](int i) { return visited[i] != 0; });
        if (next == kNone) break;
        float l = m.linkLen(at, next);
        walked += l;
        bd += l;
        prev = at;
        at = next;
        push(b, at, bd, true);
      }
    }
  }

 public:
  const char* name() const override { return "Lightning"; }

  void reset(Engine& e) override {
    const WallMap& m = e.map;
    for (auto& b : bolts) b.on = false;
    for (int i = 0; i < m.n; i++) topDist[i] = m.v[i] > m.vMax - 8.f ? 0.f : 1e9f;
    m.relax(topDist);
  }

  void noteOn(Engine& e, int vi) override {
    const Voice& v = e.voices[vi];
    Bolt* b = &bolts[0];
    for (auto& q : bolts) {
      if (!q.on) {
        b = &q;
        break;
      }
      if (q.t > b->t) b = &q;
    }
    generate(e.map, *b, v);
    b->on = true;
    b->t = 0.f;
    b->leader = b->mainLen / 1600.f + 0.02f;
    b->amp = 0.6f + 0.8f * v.vel;
    b->col = v.color;
    b->hot = mix(v.color, RGBf{0.8f, 0.88f, 1.f}, 0.75f);
    b->ns = 1;
    b->strikes[0] = b->leader;
    int extra = 1 + (rng.chance(v.vel) ? 1 : 0) + (rng.chance(0.4f) ? 1 : 0);
    for (int k = 0; k < extra && b->ns < 4; k++) {
      b->strikes[b->ns] = b->strikes[b->ns - 1] + rng.range(0.05f, 0.16f);
      b->ns++;
    }
  }

  void render(Engine& e, Frame& f, float dt, float gain) override {
    const WallMap& m = e.map;
    float sky = 0.f;
    RGBf skyCol{0, 0, 0};
    for (auto& b : bolts) {
      if (!b.on) continue;
      b.t += dt;
      if (b.t > b.strikes[b.ns - 1] + 2.5f) {
        b.on = false;
        continue;
      }
      float a = b.amp * gain;
      if (b.t < b.leader) {
        float front = b.t / b.leader * b.mainLen * 2.f;  // in 1/2 units
        for (int k = 0; k < b.n; k++) {
          if (b.d[k] > front) continue;
          float tip = fexp(-(front - b.d[k]) * (1.f / 20.f));
          f.add(b.led[k], b.hot, a * (0.1f + 0.7f * tip) * (b.branch[k] ? 0.5f : 1.f));
        }
        continue;
      }
      float flash = 0.f;
      for (int k = 0; k < b.ns; k++)
        if (b.t >= b.strikes[k]) flash += fexp(-(b.t - b.strikes[k]) / 0.045f) * (k ? 0.75f : 1.f);
      float glowK = 0.45f * fexp(-(b.t - b.strikes[0]) / 0.6f);
      for (int k = 0; k < b.n; k++) {
        float w = b.branch[k] ? 0.5f : 1.f;
        f.add(b.led[k], b.hot, a * w * flash);
        f.add(b.led[k], b.col, a * w * glowK);
      }
      sky += flash * a * 0.018f;
      skyCol += b.col * (flash * a);
    }
    if (sky > 0.001f) {
      float s = skyCol.r + skyCol.g + skyCol.b;
      RGBf c = whiten(skyCol * (1.f / maxf(s, 1e-4f)), 0.5f);
      for (int i = 0; i < m.n; i++) f.add(i, c, sky);
    }
  }
};

Lightning instance;

}  // namespace

Mode* modeLightning() { return &instance; }

}  // namespace wv
