// Geometry and topology of the LED wall, derived at boot from the layout.
//
// Wall coordinates: u = left -> right, v = bottom -> top (units of the layout,
// LED pitch ~2.2). The strips form a planar network of Voronoi lines; this
// class finds that network (which LEDs touch), its junctions, the segments
// between junctions ("edges") and the faces of the network ("cells").
#pragma once
#include "Config.h"

struct LED;

namespace wv {

struct Edge {
  uint16_t a, b;          // end LEDs (junction or dead end)
  uint16_t first, count;  // interior LEDs are edgeLeds[first .. first+count)
  float len;              // length along the strip
  float mu, mv;           // midpoint
};

struct Cell {
  float cu, cv;   // centroid
  uint16_t area;  // raster pixels
  bool border;    // touches the wall's bounding box
};

class WallMap {
 public:
  // scratch is only used during build (cell raster etc.).
  void build(const LED* leds, int count, int ledsPerStrip, bool flipU, bool flipV, void* scratch,
             int scratchBytes);

  int n = 0;
  float u[kMaxLeds], v[kMaxLeds];
  uint16_t strip[kMaxLeds];  // index for the LED driver: channel * ledsPerStrip + ledID
  float uMax = 0, vMax = 0;  // u in [0,uMax], v in [0,vMax]

  // Network
  uint8_t deg[kMaxLeds];
  uint16_t nbr[kMaxLeds][kMaxNbr];
  float nlen[kMaxLeds][kMaxNbr];
  float jglow[kMaxLeds];  // 1 on junctions, fading to 0 a few LEDs away

  // Segments between junctions
  int edgeCount = 0;
  Edge edges[kMaxEdges];
  int16_t edgeOf[kMaxLeds];  // -1 for junctions / unassigned
  float edgePos[kMaxLeds];   // 0..1 along the edge from a to b
  uint16_t edgeLeds[kMaxLeds];

  // Cells
  int cellCount = 0;
  Cell cells[kMaxCells];
  uint8_t cellOf[kMaxLeds][3];            // cells bordering each LED, 0xFF = none
  uint8_t cellHop[kMaxCells][kMaxCells];  // hop distance between cells, 0xFF = unreachable

  // Cell raster for debug images (simulator only): it lives in the scratch
  // memory, which the engine reuses after build.
  int rasterW = 0, rasterH = 0;
  float rasterRes = 2.f;
  const uint8_t* rasterLabels = nullptr;  // 0 = line/ignored, 1+cell index otherwise

  int nearest(float pu, float pv) const;
  float linkLen(int a, int b) const;
  // Dijkstra over the network. dist holds seeds (finite) and >= 1e8 elsewhere.
  void relax(float* dist) const;
  // Distance along the strips from a point; starts at the LEDs closest to it.
  void geodesicFrom(float pu, float pv, float* dist) const;

  // Calls f(i) for LEDs in grid cells overlapping the box (superset of the box).
  template <class F>
  void forEachInBox(float u0, float u1, float v0, float v1, F&& f) const {
    int x0 = gcoord(u0, gw), x1 = gcoord(u1, gw);
    int y0 = gcoord(v0, gh), y1 = gcoord(v1, gh);
    for (int gy = y0; gy <= y1; gy++)
      for (int gx = x0; gx <= x1; gx++) {
        int c = gy * gw + gx;
        for (int k = gridStart[c]; k < gridStart[c + 1]; k++) f((int)gridLeds[k]);
      }
  }
  // Calls f(i, d2) for LEDs within radius r.
  template <class F>
  void forEachNear(float pu, float pv, float r, F&& f) const {
    float r2 = r * r;
    forEachInBox(pu - r, pu + r, pv - r, pv + r, [&](int i) {
      float du = u[i] - pu, dv = v[i] - pv;
      float d2 = du * du + dv * dv;
      if (d2 <= r2) f(i, d2);
    });
  }

 private:
  static constexpr int kMaxGrid = 64 * 48;
  float gsize = 8.f, ginv = 1.f / 8.f;
  int gw = 1, gh = 1;
  uint16_t gridStart[kMaxGrid + 1];
  uint16_t gridLeds[kMaxLeds];

  int gcoord(float x, int lim) const {
    int c = (int)(x * ginv);
    return c < 0 ? 0 : (c >= lim ? lim - 1 : c);
  }
  bool linked(int a, int b) const;
  bool link(int a, int b);
  void buildGrid();
  void buildGraph(const LED* leds, int ledsPerStrip, uint8_t* scratch, int scratchBytes);
  void buildJunctionGlow();
  void buildEdges();
  void buildCells(uint8_t* scratch, int scratchBytes);
};

}  // namespace wv
