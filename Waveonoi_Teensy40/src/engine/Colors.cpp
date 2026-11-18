#include "Colors.h"
#include "FastMath.h"

namespace wv {

// 12 hand-picked hues around the wheel (linear light, brightest channel = 1).
static const RGBf kWheel[12] = {
    {1.00f, 0.02f, 0.01f},  // red
    {1.00f, 0.16f, 0.00f},  // orange
    {1.00f, 0.55f, 0.00f},  // yellow
    {0.45f, 1.00f, 0.02f},  // lime
    {0.02f, 1.00f, 0.18f},  // green
    {0.00f, 0.85f, 0.85f},  // cyan
    {0.02f, 0.35f, 1.00f},  // sky
    {0.08f, 0.05f, 1.00f},  // blue
    {0.40f, 0.02f, 1.00f},  // violet
    {0.85f, 0.00f, 1.00f},  // purple
    {1.00f, 0.02f, 0.45f},  // pink
    {1.00f, 0.00f, 0.15f},  // rose
};

RGBf hueColor(float h) {
  float x = fractf(h) * 12.f;
  int i = (int)x;
  if (i > 11) i = 11;
  float t = x - i;
  return mix(kWheel[i], kWheel[(i + 1) % 12], t);
}

float noteHue(int note) {
  int pc = ((note % 12) + 12) % 12;
  int fifths = (pc * 7) % 12;
  return fifths / 12.f;
}

RGBf noteColor(int note) {
  RGBf c = hueColor(noteHue(note));
  // The top octaves turn paler, like overtones thinning out.
  return whiten(c, 0.5f * smoothstepf(80.f, 108.f, (float)note));
}

float vnoise3(float x, float y, float z) {
  float fx = floorf(x), fy = floorf(y), fz = floorf(z);
  int32_t ix = (int32_t)fx, iy = (int32_t)fy, iz = (int32_t)fz;
  float tx = x - fx, ty = y - fy, tz = z - fz;
  tx = tx * tx * (3.f - 2.f * tx);
  ty = ty * ty * (3.f - 2.f * ty);
  tz = tz * tz * (3.f - 2.f * tz);
  auto h = [](int32_t a, int32_t b, int32_t c) {
    return hash01((uint32_t)a * 73856093U ^ (uint32_t)b * 19349663U ^ (uint32_t)c * 83492791U);
  };
  float c000 = h(ix, iy, iz), c100 = h(ix + 1, iy, iz);
  float c010 = h(ix, iy + 1, iz), c110 = h(ix + 1, iy + 1, iz);
  float c001 = h(ix, iy, iz + 1), c101 = h(ix + 1, iy, iz + 1);
  float c011 = h(ix, iy + 1, iz + 1), c111 = h(ix + 1, iy + 1, iz + 1);
  float a = lerpf(lerpf(c000, c100, tx), lerpf(c010, c110, tx), ty);
  float b = lerpf(lerpf(c001, c101, tx), lerpf(c011, c111, tx), ty);
  return lerpf(a, b, tz);
}

}  // namespace wv
