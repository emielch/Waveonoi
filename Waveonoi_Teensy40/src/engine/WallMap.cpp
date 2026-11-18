#include "WallMap.h"

#include <string.h>

#include "../LED.h"
#include "FastMath.h"

namespace wv {

static constexpr float kInf = 1e9f;
static constexpr float kChainLink = 5.5f;  // max gap between consecutive LEDs on one strip
static constexpr float kJoinLink = 4.5f;   // max gap between a strip end and another strip

// ---------------------------------------------------------------- build

void WallMap::build(const LED* leds, int count, int ledsPerStrip, bool flipU, bool flipV,
                    void* scratch, int scratchBytes) {
  n = count < kMaxLeds ? count : kMaxLeds;
  int xmin = 1 << 30, xmax = -(1 << 30), ymin = 1 << 30, ymax = -(1 << 30);
  for (int i = 0; i < n; i++) {
    if (leds[i].xPos < xmin) xmin = leds[i].xPos;
    if (leds[i].xPos > xmax) xmax = leds[i].xPos;
    if (leds[i].yPos < ymin) ymin = leds[i].yPos;
    if (leds[i].yPos > ymax) ymax = leds[i].yPos;
  }
  uMax = (float)(ymax - ymin);
  vMax = (float)(xmax - xmin);
  for (int i = 0; i < n; i++) {
    float y = (float)(leds[i].yPos - ymin);
    float x = (float)(leds[i].xPos - xmin);
    u[i] = flipU ? uMax - y : y;
    v[i] = flipV ? vMax - x : x;
    strip[i] = (uint16_t)(leds[i].channel * ledsPerStrip + leds[i].ledID);
  }
  buildGrid();
  buildGraph(leds, ledsPerStrip, (uint8_t*)scratch, scratchBytes);
  buildJunctionGlow();
  buildEdges();
  buildCells((uint8_t*)scratch, scratchBytes);
}

void WallMap::buildGrid() {
  gsize = 8.f;
  while (uMax / gsize >= 63.f || vMax / gsize >= 47.f) gsize += 1.f;
  ginv = 1.f / gsize;
  gw = (int)(uMax * ginv) + 1;
  gh = (int)(vMax * ginv) + 1;
  int cells = gw * gh;
  memset(gridStart, 0, sizeof(gridStart));
  for (int i = 0; i < n; i++) gridStart[gcoord(v[i], gh) * gw + gcoord(u[i], gw) + 1]++;
  for (int c = 0; c < cells; c++) gridStart[c + 1] += gridStart[c];
  uint16_t fill[kMaxGrid];
  memcpy(fill, gridStart, sizeof(uint16_t) * cells);
  for (int i = 0; i < n; i++) gridLeds[fill[gcoord(v[i], gh) * gw + gcoord(u[i], gw)]++] = (uint16_t)i;
}

bool WallMap::linked(int a, int b) const {
  for (int k = 0; k < deg[a]; k++)
    if (nbr[a][k] == b) return true;
  return false;
}

bool WallMap::link(int a, int b) {
  if (a == b || linked(a, b) || deg[a] >= kMaxNbr || deg[b] >= kMaxNbr) return false;
  float du = u[a] - u[b], dv = v[a] - v[b];
  float d = sqrtf(du * du + dv * dv);
  if (d < 0.5f) d = 0.5f;
  nbr[a][deg[a]] = (uint16_t)b;
  nlen[a][deg[a]++] = d;
  nbr[b][deg[b]] = (uint16_t)a;
  nlen[b][deg[b]++] = d;
  return true;
}

float WallMap::linkLen(int a, int b) const {
  for (int k = 0; k < deg[a]; k++)
    if (nbr[a][k] == b) return nlen[a][k];
  float du = u[a] - u[b], dv = v[a] - v[b];
  return sqrtf(du * du + dv * dv);
}

void WallMap::buildGraph(const LED* leds, int ledsPerStrip, uint8_t* scratch, int scratchBytes) {
  memset(deg, 0, sizeof(deg));

  // 1. Consecutive LEDs on the same strip, unless the strip jumps (it leaves
  //    the wall behind the fabric and comes back elsewhere).
  int idxSize = 8 * 256;
  uint16_t* idx = (uint16_t*)scratch;
  if (scratchBytes >= (int)(idxSize * sizeof(uint16_t))) {
    for (int k = 0; k < idxSize; k++) idx[k] = kNone;
    for (int i = 0; i < n; i++)
      if (leds[i].channel < 8) idx[leds[i].channel * 256 + leds[i].ledID] = (uint16_t)i;
    for (int i = 0; i < n; i++) {
      if (leds[i].channel >= 8 || leds[i].ledID >= 255) continue;
      int j = idx[leds[i].channel * 256 + leds[i].ledID + 1];
      if (j == kNone) continue;
      float du = u[i] - u[j], dv = v[i] - v[j];
      if (du * du + dv * dv <= kChainLink * kChainLink) link(i, j);
    }
  }
  (void)ledsPerStrip;

  // 2. Strip ends join the nearest LED of another strip (junctions).
  for (int i = 0; i < n; i++) {
    if (deg[i] > 1) continue;
    // LEDs within a few hops along the own strip are not candidates.
    uint16_t near[16];
    int nn = 0;
    near[nn++] = (uint16_t)i;
    for (int s = 0; s < nn && nn < 16; s++)
      for (int k = 0; k < deg[near[s]] && nn < 16; k++) {
        uint16_t w = nbr[near[s]][k];
        bool seen = false;
        for (int q = 0; q < nn; q++) seen |= near[q] == w;
        if (!seen) near[nn++] = w;
      }
    int best = -1;
    float bestD2 = kJoinLink * kJoinLink;
    forEachNear(u[i], v[i], kJoinLink, [&](int j, float d2) {
      for (int q = 0; q < nn; q++)
        if (near[q] == j) return;
      if (d2 < bestD2) {
        bestD2 = d2;
        best = j;
      }
    });
    if (best >= 0) link(i, best);
  }

  // 3. Bridge any remaining islands to the main network.
  uint16_t* comp = (uint16_t*)scratch;
  uint16_t* stack = comp + kMaxLeds;
  for (int iter = 0; iter < 64; iter++) {
    for (int i = 0; i < n; i++) comp[i] = kNone;
    int nc = 0, sizes[64] = {0};
    for (int s = 0; s < n; s++) {
      if (comp[s] != kNone) continue;
      int c = nc < 63 ? nc : 63;
      nc++;
      int sp = 0;
      stack[sp++] = (uint16_t)s;
      comp[s] = (uint16_t)c;
      while (sp) {
        int a = stack[--sp];
        sizes[c]++;
        for (int k = 0; k < deg[a]; k++)
          if (comp[nbr[a][k]] == kNone) {
            comp[nbr[a][k]] = (uint16_t)c;
            stack[sp++] = nbr[a][k];
          }
      }
    }
    if (nc <= 1) break;
    int big = 0, small = -1;
    for (int c = 0; c < nc && c < 64; c++) {
      if (sizes[c] > sizes[big]) big = c;
    }
    for (int c = 0; c < nc && c < 64; c++)
      if (c != big && (small < 0 || sizes[c] < sizes[small])) small = c;
    int bi = -1, bj = -1;
    float bd = kInf;
    for (int i = 0; i < n; i++) {
      if (comp[i] != small) continue;
      for (int j = 0; j < n; j++) {
        if (comp[j] == small || deg[j] >= kMaxNbr) continue;
        float du = u[i] - u[j], dv = v[i] - v[j];
        float d2 = du * du + dv * dv;
        if (d2 < bd && deg[i] < kMaxNbr) {
          bd = d2;
          bi = i;
          bj = j;
        }
      }
    }
    if (bi < 0 || !link(bi, bj)) break;
  }
}

void WallMap::buildJunctionGlow() {
  for (int i = 0; i < n; i++) jglow[i] = deg[i] >= 3 ? 0.f : kInf;
  relax(jglow);
  for (int i = 0; i < n; i++) {
    float t = 1.f - jglow[i] / 7.f;
    jglow[i] = t > 0.f ? t * t : 0.f;
  }
}

void WallMap::buildEdges() {
  edgeCount = 0;
  int used = 0;
  for (int i = 0; i < n; i++) {
    edgeOf[i] = -1;
    edgePos[i] = 0.f;
  }
  for (int a = 0; a < n; a++) {
    if (deg[a] == 2) continue;
    for (int k = 0; k < deg[a] && edgeCount < kMaxEdges; k++) {
      int c = nbr[a][k];
      if (deg[c] != 2 || edgeOf[c] >= 0) continue;
      Edge& e = edges[edgeCount];
      e.a = (uint16_t)a;
      e.first = (uint16_t)used;
      int cnt = 0;
      float len = nlen[a][k];
      int prev = a, cur = c;
      while (deg[cur] == 2 && edgeOf[cur] < 0) {
        edgeOf[cur] = (int16_t)edgeCount;
        edgePos[cur] = len;
        edgeLeds[used++] = (uint16_t)cur;
        cnt++;
        int next = nbr[cur][0] == prev ? nbr[cur][1] : nbr[cur][0];
        len += linkLen(cur, next);
        prev = cur;
        cur = next;
      }
      e.b = (uint16_t)cur;
      e.count = (uint16_t)cnt;
      e.len = len;
      for (int q = 0; q < cnt; q++) edgePos[edgeLeds[e.first + q]] /= len;
      int mid = edgeLeds[e.first + cnt / 2];
      e.mu = u[mid];
      e.mv = v[mid];
      edgeCount++;
    }
  }
}

// Cells are the faces of the planar network: rasterise the lines, then
// flood-fill the empty space in between.
void WallMap::buildCells(uint8_t* scratch, int scratchBytes) {
  cellCount = 0;
  memset(cellOf, 0xFF, sizeof(cellOf));
  memset(cellHop, 0xFF, sizeof(cellHop));
  rasterRes = 2.f;
  for (;;) {
    rasterW = (int)(uMax / rasterRes) + 1;
    rasterH = (int)(vMax / rasterRes) + 1;
    int px = rasterW * rasterH;
    if (px < 60000 && px * 3 + 64 <= scratchBytes) break;
    rasterRes += 0.5f;
    if (rasterRes > 8.f) return;
  }
  const int W = rasterW, H = rasterH, P = W * H;
  uint8_t* lab = scratch;
  uint16_t* queue = (uint16_t*)(scratch + ((P + 3) & ~3));
  const int qcap = (scratchBytes - ((P + 3) & ~3)) / 2;
  rasterLabels = lab;
  const uint8_t kFree = 255;
  memset(lab, kFree, P);

  auto mark = [&](float pu, float pv) {
    int x = (int)(pu / rasterRes), y = (int)(pv / rasterRes);
    if (x >= 0 && x < W && y >= 0 && y < H) lab[y * W + x] = 0;
  };
  for (int a = 0; a < n; a++) {
    mark(u[a], v[a]);
    for (int k = 0; k < deg[a]; k++) {
      int b = nbr[a][k];
      if (b < a) continue;
      int steps = (int)(nlen[a][k] / (rasterRes * 0.25f)) + 1;
      for (int s = 1; s <= steps; s++) {
        float t = (float)s / steps;
        mark(lerpf(u[a], u[b], t), lerpf(v[a], v[b], t));
      }
    }
    // Continue dead ends a little so lines that stop just short of the
    // wall's edge still close their cell.
    if (deg[a] == 1) {
      int b = nbr[a][0];
      float du = u[a] - u[b], dv = v[a] - v[b];
      float l = sqrtf(du * du + dv * dv);
      if (l > 0.1f)
        for (float s = 0.5f; s < 10.f; s += 0.5f) mark(u[a] + du / l * s, v[a] + dv / l * s);
    }
  }

  // Flood fill (4-connected, so 8-connected lines are watertight).
  int nlab = 0;
  uint16_t area[250];
  float su[250], sv[250];
  bool border[250];
  for (int s = 0; s < P; s++) {
    if (lab[s] != kFree) continue;
    if (nlab >= 249) {
      lab[s] = 0;
      continue;
    }
    int L = ++nlab;
    area[L] = 0;
    su[L] = sv[L] = 0.f;
    border[L] = false;
    int qh = 0, qt = 0, qn = 0;
    queue[qt] = (uint16_t)s;
    qt = (qt + 1) % qcap;
    qn++;
    lab[s] = (uint8_t)L;
    while (qn) {
      int p = queue[qh];
      qh = (qh + 1) % qcap;
      qn--;
      int x = p % W, y = p / W;
      area[L] = area[L] < 65535 ? area[L] + 1 : 65535;
      su[L] += x;
      sv[L] += y;
      if (x == 0 || y == 0 || x == W - 1 || y == H - 1) border[L] = true;
      const int nb[4] = {x > 0 ? p - 1 : -1, x < W - 1 ? p + 1 : -1, y > 0 ? p - W : -1,
                         y < H - 1 ? p + W : -1};
      for (int k = 0; k < 4; k++) {
        int q = nb[k];
        if (q < 0 || lab[q] != kFree || qn >= qcap) continue;
        lab[q] = (uint8_t)L;
        queue[qt] = (uint16_t)q;
        qt = (qt + 1) % qcap;
        qn++;
      }
    }
  }

  // Keep regions big enough to be real cells; remap to 1..cellCount.
  uint8_t remap[256];
  remap[0] = 0;
  remap[kFree] = 0;
  for (int L = 1; L <= nlab; L++) {
    if (area[L] >= 25 && cellCount < kMaxCells && cellCount < 250) {
      Cell& c = cells[cellCount];
      c.area = area[L];
      c.cu = (su[L] / area[L] + 0.5f) * rasterRes;
      c.cv = (sv[L] / area[L] + 0.5f) * rasterRes;
      c.border = border[L];
      remap[L] = (uint8_t)(++cellCount);
    } else {
      remap[L] = 0;
    }
  }
  for (int p = 0; p < P; p++) lab[p] = remap[lab[p]];

  // Which cells does each LED border? Look at the raster around it.
  for (int i = 0; i < n; i++) {
    int cx = (int)(u[i] / rasterRes), cy = (int)(v[i] / rasterRes);
    uint8_t ids[25];
    uint8_t hits[25];
    int m = 0;
    for (int dy = -2; dy <= 2; dy++)
      for (int dx = -2; dx <= 2; dx++) {
        int x = cx + dx, y = cy + dy;
        if (x < 0 || y < 0 || x >= W || y >= H) continue;
        uint8_t L = lab[y * W + x];
        if (!L) continue;
        int q = 0;
        while (q < m && ids[q] != L) q++;
        if (q == m) {
          ids[m] = L;
          hits[m++] = 0;
        }
        hits[q]++;
      }
    for (int slot = 0; slot < 3; slot++) {
      int best = -1;
      for (int q = 0; q < m; q++)
        if (hits[q] >= 3 && (best < 0 || hits[q] > hits[best])) best = q;
      if (best < 0) break;
      cellOf[i][slot] = (uint8_t)(ids[best] - 1);
      hits[best] = 0;
    }
  }

  // Cell adjacency (sharing at least two LEDs) and hop distances. The flood
  // fill queue is no longer needed, so its memory holds the adjacency table.
  if (qcap * 2 < kMaxCells * kMaxCells) return;
  uint8_t(*shared)[kMaxCells] = (uint8_t(*)[kMaxCells])queue;
  memset(shared, 0, kMaxCells * kMaxCells);
  for (int i = 0; i < n; i++)
    for (int a = 0; a < 3; a++)
      for (int b = 0; b < 3; b++) {
        uint8_t ca = cellOf[i][a], cb = cellOf[i][b];
        if (a != b && ca != 0xFF && cb != 0xFF && shared[ca][cb] < 255) shared[ca][cb]++;
      }
  for (int s = 0; s < cellCount; s++) {
    uint8_t* hop = cellHop[s];
    uint8_t q[kMaxCells];
    int qh = 0, qt = 0;
    hop[s] = 0;
    q[qt++] = (uint8_t)s;
    while (qh < qt) {
      int c = q[qh++];
      for (int d = 0; d < cellCount; d++)
        if (shared[c][d] >= 2 && hop[d] == 0xFF) {
          hop[d] = (uint8_t)(hop[c] + 1);
          q[qt++] = (uint8_t)d;
        }
    }
  }
}

// ---------------------------------------------------------------- queries

int WallMap::nearest(float pu, float pv) const {
  int best = 0;
  float bd = kInf;
  for (int i = 0; i < n; i++) {
    float du = u[i] - pu, dv = v[i] - pv;
    float d2 = du * du + dv * dv;
    if (d2 < bd) {
      bd = d2;
      best = i;
    }
  }
  return best;
}

void WallMap::relax(float* dist) const {
  static uint16_t heap[kMaxLeds];
  static int16_t pos[kMaxLeds];  // -1 = not queued, -2 = done
  int hn = 0;
  auto up = [&](int h) {
    uint16_t x = heap[h];
    while (h > 0) {
      int p = (h - 1) >> 1;
      if (dist[heap[p]] <= dist[x]) break;
      heap[h] = heap[p];
      pos[heap[h]] = (int16_t)h;
      h = p;
    }
    heap[h] = x;
    pos[x] = (int16_t)h;
  };
  auto down = [&](int h) {
    uint16_t x = heap[h];
    for (;;) {
      int c = 2 * h + 1;
      if (c >= hn) break;
      if (c + 1 < hn && dist[heap[c + 1]] < dist[heap[c]]) c++;
      if (dist[x] <= dist[heap[c]]) break;
      heap[h] = heap[c];
      pos[heap[h]] = (int16_t)h;
      h = c;
    }
    heap[h] = x;
    pos[x] = (int16_t)h;
  };
  for (int i = 0; i < n; i++) {
    pos[i] = -1;
    if (dist[i] < 1e8f) {
      heap[hn] = (uint16_t)i;
      up(hn++);
    }
  }
  while (hn) {
    int a = heap[0];
    pos[a] = -2;
    if (--hn) {
      heap[0] = heap[hn];
      pos[heap[0]] = 0;
      down(0);
    }
    for (int k = 0; k < deg[a]; k++) {
      int b = nbr[a][k];
      if (pos[b] == -2) continue;
      float nd = dist[a] + nlen[a][k];
      if (nd < dist[b]) {
        dist[b] = nd;
        if (pos[b] < 0) {
          heap[hn] = (uint16_t)b;
          up(hn++);
        } else {
          up(pos[b]);
        }
      }
    }
  }
}

void WallMap::geodesicFrom(float pu, float pv, float* dist) const {
  float dmin = kInf;
  for (int i = 0; i < n; i++) {
    float du = u[i] - pu, dv = v[i] - pv;
    dist[i] = sqrtf(du * du + dv * dv);
    if (dist[i] < dmin) dmin = dist[i];
  }
  for (int i = 0; i < n; i++) dist[i] = dist[i] <= dmin + 20.f ? dist[i] - dmin : kInf;
  relax(dist);
}

}  // namespace wv
