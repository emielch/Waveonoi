#pragma once
#include "Colors.h"
#include "Config.h"

namespace wv {

// One struck note. The envelope follows a piano: instant attack, slow decay
// while the string rings (bass longer than treble), quick fade once released.
struct Voice {
  uint32_t id = 0;        // 0 = free slot; unique per strike otherwise
  uint8_t note = 0;
  float vel = 0.f;        // 0..1
  float age = 0.f;        // seconds since the strike
  float relAge = -1.f;    // seconds since the sound was released, <0 while sounding
  bool held = false;      // key is down
  bool sustained = false; // key is up but the sustain pedal holds the note
  float level = 0.f;      // current loudness, ~0..1
  float u = 0.f, v = 0.f; // origin on the wall (behind the key)
  RGBf color{0, 0, 0};

  bool alive() const { return id != 0; }
  bool sounding() const { return held || sustained; }
};

}  // namespace wv
