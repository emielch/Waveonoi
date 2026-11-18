// Small, fast math helpers. Only include from engine .cpp files (pulls in <math.h>).
#pragma once
#include <math.h>
#include <stdint.h>

namespace wv {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.28318531f;

inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float maxf(float a, float b) { return a > b ? a : b; }
inline float minf(float a, float b) { return a < b ? a : b; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float smooth01(float x) {
  x = clampf(x, 0.f, 1.f);
  return x * x * (3.f - 2.f * x);
}
inline float smoothstepf(float e0, float e1, float x) { return smooth01((x - e0) / (e1 - e0)); }
inline float fractf(float x) { return x - floorf(x); }

// 2^x with ~2e-4 relative error. Much cheaper than expf on the Cortex-M7.
inline float fexp2(float x) {
  if (x < -60.f) return 0.f;
  if (x > 60.f) x = 60.f;
  float fi = floorf(x);
  float f = x - fi;
  float p = 1.0f + f * (0.6951786f + f * (0.2261865f + f * 0.0781196f));
  union {
    float f;
    int32_t i;
  } u;
  u.i = ((int32_t)fi + 127) << 23;
  return p * u.f;
}
inline float fexp(float x) { return fexp2(x * 1.44269504f); }

// sin/cos with ~1e-3 absolute error.
inline float fsin(float x) {
  float t = x * (1.f / kTau);
  t -= floorf(t + 0.5f);  // [-0.5, 0.5) turns
  float y = 8.f * t - 16.f * t * fabsf(t);
  return 0.225f * (y * fabsf(y) - y) + y;
}
inline float fcos(float x) { return fsin(x + 1.57079633f); }

inline uint32_t hash32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}
inline float hash01(uint32_t x) { return (hash32(x) >> 8) * (1.f / 16777216.f); }

// Smooth 1D value noise in [0,1], one independent curve per seed.
inline float vnoise1(uint32_t seed, float t) {
  float fi = floorf(t);
  int32_t i = (int32_t)fi;
  float f = t - fi;
  f = f * f * (3.f - 2.f * f);
  uint32_t s = seed * 0x9E3779B9U;
  float a = hash01(s + (uint32_t)i);
  float b = hash01(s + (uint32_t)(i + 1));
  return a + (b - a) * f;
}

// Smooth 3D value noise in [0,1].
float vnoise3(float x, float y, float z);

struct Rng {
  uint32_t s = 0x2545F491U;
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  float uni() { return (next() >> 8) * (1.f / 16777216.f); }
  float range(float a, float b) { return a + (b - a) * uni(); }
  int below(int n) { return (int)(uni() * n) % (n > 0 ? n : 1); }
  bool chance(float p) { return uni() < p; }
};

}  // namespace wv
