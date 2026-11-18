// Helpers for effects that travel along the strips.
#pragma once
#include "../Engine.h"
#include "../FastMath.h"

namespace wv {

// Rough direction of the branch that starts with the link cur -> nb: looks a
// few LEDs ahead so junction choices see where a branch actually goes.
inline float branchRise(const WallMap& m, int cur, int nb) {
  int prev = cur, at = nb;
  for (int s = 0; s < 5 && m.deg[at] == 2; s++) {
    int next = m.nbr[at][0] == prev ? m.nbr[at][1] : m.nbr[at][0];
    prev = at;
    at = next;
  }
  float du = m.u[at] - m.u[cur], dv = m.v[at] - m.v[cur];
  float d = sqrtf(du * du + dv * dv);
  return d > 0.01f ? dv / d : 0.f;  // -1 down .. +1 up
}

// Random next LED from cur, not back to prev. upBias > 0 favours rising
// branches, < 0 falling ones. skip(i) can veto candidates. Returns kNone at
// a dead end.
template <class Skip>
inline int randomStep(const WallMap& m, Rng& rng, int prev, int cur, float upBias, Skip&& skip) {
  float w[kMaxNbr];
  int c[kMaxNbr];
  int k = 0;
  float sum = 0.f;
  for (int q = 0; q < m.deg[cur]; q++) {
    int nb = m.nbr[cur][q];
    if (nb == prev || skip(nb)) continue;
    float wt = upBias != 0.f && m.deg[cur] > 2 ? fexp(upBias * branchRise(m, cur, nb)) : 1.f;
    c[k] = nb;
    w[k++] = wt;
    sum += wt;
  }
  if (!k) return kNone;
  float r = rng.uni() * sum;
  for (int q = 0; q < k; q++) {
    r -= w[q];
    if (r <= 0.f) return c[q];
  }
  return c[k - 1];
}

inline int randomStep(const WallMap& m, Rng& rng, int prev, int cur, float upBias) {
  return randomStep(m, rng, prev, cur, upBias, [](int) { return false; });
}

inline bool voiceValid(const Engine& e, int vi, uint32_t vid) {
  return vi >= 0 && vi < kMaxVoices && e.voices[vi].id == vid;
}

}  // namespace wv
