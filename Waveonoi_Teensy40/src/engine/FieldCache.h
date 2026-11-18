#pragma once
#include "Config.h"

namespace wv {

class WallMap;

// Geodesic distance fields (distance along the strips from a key), cached per
// key because computing one takes a Dijkstra pass. Slots in use by an effect
// are reference counted and never evicted.
class FieldCache {
 public:
  static constexpr float kScale = 8.f;  // stored as uint16 in 1/8 units
  static int bytes() { return kFieldSlots * kMaxLeds * (int)sizeof(uint16_t); }

  void begin(uint16_t* memory);  // kFieldSlots * kMaxLeds entries
  void invalidate();             // after the map or key calibration changes
  int acquire(int key, const WallMap& map, float u0, float v0);  // -1 if all slots busy
  void release(int slot);
  const uint16_t* field(int slot) const { return mem + slot * kMaxLeds; }

 private:
  uint16_t* mem = nullptr;
  int16_t key[kFieldSlots];
  uint8_t refs[kFieldSlots];
  uint32_t stamp[kFieldSlots];
  uint32_t clock = 0;
};

}  // namespace wv
