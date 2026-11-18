#include "FieldCache.h"

#include "WallMap.h"

namespace wv {

void FieldCache::begin(uint16_t* memory) {
  mem = memory;
  for (int s = 0; s < kFieldSlots; s++) refs[s] = 0;
  invalidate();
}

void FieldCache::invalidate() {
  for (int s = 0; s < kFieldSlots; s++) {
    key[s] = -1;
    stamp[s] = 0;
  }
}

int FieldCache::acquire(int k, const WallMap& map, float u0, float v0) {
  clock++;
  for (int s = 0; s < kFieldSlots; s++)
    if (key[s] == k) {
      refs[s]++;
      stamp[s] = clock;
      return s;
    }
  int best = -1;
  for (int s = 0; s < kFieldSlots; s++)
    if (refs[s] == 0 && (best < 0 || stamp[s] < stamp[best])) best = s;
  if (best < 0) return -1;

  static float tmp[kMaxLeds];
  map.geodesicFrom(u0, v0, tmp);
  uint16_t* out = mem + best * kMaxLeds;
  for (int i = 0; i < map.n; i++) {
    float d = tmp[i] * kScale;
    out[i] = d < 65535.f ? (uint16_t)d : (uint16_t)65535;
  }
  key[best] = (int16_t)k;
  refs[best] = 1;
  stamp[best] = clock;
  return best;
}

void FieldCache::release(int slot) {
  if (slot >= 0 && slot < kFieldSlots && refs[slot]) refs[slot]--;
}

}  // namespace wv
