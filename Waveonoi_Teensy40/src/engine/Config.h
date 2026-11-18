// Engine-wide limits. The engine is plain C++ (no Arduino headers) so the
// same code runs on the Teensy and in the desktop simulator (../../sim).
#pragma once
#include <stdint.h>

namespace wv {

constexpr int kMaxLeds = 1800;
constexpr int kMaxNbr = 4;        // graph neighbours per LED
constexpr int kMaxVoices = 32;
constexpr int kMaxEdges = 256;    // strip segments between junctions
constexpr int kMaxCells = 120;    // Voronoi cells (faces of the strip network)
constexpr int kFieldSlots = 24;   // cached geodesic distance fields
constexpr uint16_t kNone = 0xFFFF;

}  // namespace wv

// Large buffers go to the Teensy's second RAM bank (OCRAM). That memory is
// not zeroed at boot, so every user clears its buffer explicitly.
#if defined(__IMXRT1062__)
#define WV_BIG __attribute__((section(".dmabuffers"), used))
#else
#define WV_BIG
#endif
