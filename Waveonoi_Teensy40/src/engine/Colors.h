// Colours in linear light: 1.0 = full PWM. Conversion to the 8-bit values the
// LED library expects (CIE L*) happens once, in Engine::pixel().
#pragma once
#include <stdint.h>

namespace wv {

struct RGBf {
  float r, g, b;
};

inline RGBf rgbf(float r, float g, float b) { return RGBf{r, g, b}; }
inline RGBf operator*(const RGBf& c, float k) { return RGBf{c.r * k, c.g * k, c.b * k}; }
inline RGBf operator+(const RGBf& a, const RGBf& b) { return RGBf{a.r + b.r, a.g + b.g, a.b + b.b}; }
inline RGBf& operator+=(RGBf& a, const RGBf& b) {
  a.r += b.r;
  a.g += b.g;
  a.b += b.b;
  return a;
}
inline RGBf mix(const RGBf& a, const RGBf& b, float t) {
  return RGBf{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
// Move a colour towards white, keeping its brightest channel.
inline RGBf whiten(const RGBf& c, float k) {
  float m = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
  return RGBf{c.r + (m - c.r) * k, c.g + (m - c.g) * k, c.b + (m - c.b) * k};
}
inline void maxInto(RGBf& a, const RGBf& b) {
  if (b.r > a.r) a.r = b.r;
  if (b.g > a.g) a.g = b.g;
  if (b.b > a.b) a.b = b.b;
}

// Hue wheel tuned for LEDs; h in turns (wraps). 0 = red, 1/6 ~ yellow, ...
RGBf hueColor(float h);

// Hue of a MIDI note: pitch classes laid out along the circle of fifths, so
// consonant notes get neighbouring hues (C red, G orange, D yellow, A lime,
// E green, B cyan, F# sky, C# blue, G# violet, D# purple, A# pink, F rose).
float noteHue(int note);
RGBf noteColor(int note);

}  // namespace wv
