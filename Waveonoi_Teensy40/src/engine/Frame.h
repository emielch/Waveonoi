#pragma once
#include <string.h>

#include "Colors.h"
#include "Config.h"

namespace wv {

// One frame of linear light per LED. Modes add into it; values above 1 are
// allowed and get tone-mapped on output.
struct Frame {
  RGBf px[kMaxLeds];
  int n = 0;
  void clear() { memset(px, 0, sizeof(RGBf) * n); }
  void add(int i, const RGBf& c, float k) {
    px[i].r += c.r * k;
    px[i].g += c.g * k;
    px[i].b += c.b * k;
  }
};

}  // namespace wv
