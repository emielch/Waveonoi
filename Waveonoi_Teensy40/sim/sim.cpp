// Desktop simulator for the Waveonoi piano animations.
//
// Runs the firmware's engine (../src/engine) on the real LED layout and draws
// the wall as it looks through the curtain. Usage:
//
//   sim map   <out.png>                                 network / cells debug image
//   sim video <mode> <script> <out.mp4> [seconds]       MP4 preview
//   sim sheet <mode> <script> <out.png> <t1,t2,...>      contact sheet of moments
//   sim bench <mode> <script>                            engine timing only
//
// <mode>   number (1 = first) or name prefix, e.g. "ripples", "veins"
// <script> single | chord | run | pedal | demo | path/to/file.mid
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../src/engine/Engine.h"
#include "../src/ledLayout.h"
#include "Smf.h"

using namespace wv;

static Engine engine;
static constexpr int kLedsPerStrip = 227;
static constexpr float S = 2.f;  // image pixels per wall unit
static constexpr int M = 24;     // margin
static constexpr int KB = 40;    // keyboard strip

// ------------------------------------------------------------------ images

struct Image {
  int w = 0, h = 0;
  std::vector<float> px;  // linear RGB
  Image() {}
  Image(int w_, int h_) : w(w_), h(h_), px((size_t)w_ * h_ * 3, 0.f) {}
  void add(int x, int y, float r, float g, float b) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    float* p = &px[((size_t)y * w + x) * 3];
    p[0] += r;
    p[1] += g;
    p[2] += b;
  }
  void set(int x, int y, float r, float g, float b) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    float* p = &px[((size_t)y * w + x) * 3];
    p[0] = r;
    p[1] = g;
    p[2] = b;
  }
  void rect(int x0, int y0, int x1, int y1, float r, float g, float b) {
    for (int y = y0; y < y1; y++)
      for (int x = x0; x < x1; x++) set(x, y, r, g, b);
  }
  void line(float x0, float y0, float x1, float y1, float r, float g, float b) {
    int n = (int)(std::max(fabsf(x1 - x0), fabsf(y1 - y0))) + 1;
    for (int k = 0; k <= n; k++) {
      float t = (float)k / n;
      set((int)(x0 + (x1 - x0) * t), (int)(y0 + (y1 - y0) * t), r, g, b);
    }
  }
  void dot(float cx, float cy, int rad, float r, float g, float b) {
    for (int y = -rad; y <= rad; y++)
      for (int x = -rad; x <= rad; x++)
        if (x * x + y * y <= rad * rad) set((int)cx + x, (int)cy + y, r, g, b);
  }
  std::vector<unsigned char> srgb() const {
    std::vector<unsigned char> out((size_t)w * h * 3);
    for (size_t k = 0; k < out.size(); k++) {
      float y = std::min(1.f, std::max(0.f, px[k]));
      float s = y <= 0.0031308f ? 12.92f * y : 1.055f * powf(y, 1.f / 2.4f) - 0.055f;
      out[k] = (unsigned char)(s * 255.f + 0.5f);
    }
    return out;
  }
};

static bool writePng(const Image& img, const std::string& path, float scale = 1.f) {
  std::string ppm = path + ".ppm";
  FILE* f = fopen(ppm.c_str(), "wb");
  if (!f) return false;
  fprintf(f, "P6\n%d %d\n255\n", img.w, img.h);
  auto b = img.srgb();
  fwrite(b.data(), 1, b.size(), f);
  fclose(f);
  char cmd[1024];
  snprintf(cmd, sizeof cmd, "ffmpeg -y -loglevel error -i '%s' -vf scale=iw*%.3f:-1:flags=area '%s'",
           ppm.c_str(), scale, path.c_str());
  int rc = system(cmd);
  remove(ppm.c_str());
  return rc == 0;
}

static float wallX(float u) { return M + u * S; }
static float wallY(float v) { return M + (engine.map.vMax - v) * S; }
static int imgW() { return (int)(engine.map.uMax * S) + 2 * M; }
static int imgH() { return (int)(engine.map.vMax * S) + 2 * M + KB; }

static float byteToLinear(uint8_t b) {
  float L = b / 2.55f;
  return L > 8.f ? powf((L + 16.f) / 116.f, 3.f) : L / 903.3f;
}

// The wall as seen through the curtain: a sharp LED core plus a soft glow.
static void drawWall(Image& img) {
  const WallMap& m = engine.map;
  Image halo(img.w / 2 + 1, img.h / 2 + 1);
  std::fill(img.px.begin(), img.px.end(), 0.0025f);
  for (int i = 0; i < m.n; i++) {
    uint8_t r8, g8, b8;
    engine.pixel(i, r8, g8, b8);
    float r = byteToLinear(r8), g = byteToLinear(g8), b = byteToLinear(b8);
    float cx = wallX(m.u[i]), cy = wallY(m.v[i]);
    if (r + g + b < 1e-5f) {
      img.add((int)cx, (int)cy, 0.006f, 0.006f, 0.006f);
      continue;
    }
    for (int y = -5; y <= 5; y++)
      for (int x = -5; x <= 5; x++) {
        float k = expf(-(x * x + y * y) / (2.f * 2.4f * 2.4f)) * 0.9f;
        img.add((int)cx + x, (int)cy + y, r * k, g * k, b * k);
      }
    float hx = cx * 0.5f, hy = cy * 0.5f;
    for (int y = -16; y <= 16; y++)
      for (int x = -16; x <= 16; x++) {
        float k = expf(-(x * x + y * y) / (2.f * 6.5f * 6.5f)) * 0.075f;
        halo.add((int)hx + x, (int)hy + y, r * k, g * k, b * k);
      }
  }
  for (int y = 0; y < img.h; y++)
    for (int x = 0; x < img.w; x++) {
      float fx = x * 0.5f, fy = y * 0.5f;
      int x0 = (int)fx, y0 = (int)fy;
      float tx = fx - x0, ty = fy - y0;
      for (int c = 0; c < 3; c++) {
        auto H = [&](int xx, int yy) {
          xx = std::min(xx, halo.w - 1);
          yy = std::min(yy, halo.h - 1);
          return halo.px[((size_t)yy * halo.w + xx) * 3 + c];
        };
        float v = (H(x0, y0) * (1 - tx) + H(x0 + 1, y0) * tx) * (1 - ty) +
                  (H(x0, y0 + 1) * (1 - tx) + H(x0 + 1, y0 + 1) * tx) * ty;
        img.px[((size_t)y * img.w + x) * 3 + c] += v;
      }
    }
}

// The piano under the wall, at the calibrated key positions.
static void drawKeyboard(Image& img) {
  static const bool black[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
  int y0 = img.h - KB + 10, y1 = img.h - 6;
  float kw = (engine.keyU(108) - engine.keyU(21)) / 51.f * S;
  img.rect((int)(wallX(engine.keyU(21)) - kw), y0 - 3, (int)(wallX(engine.keyU(108)) + kw), y1 + 3,
           0.01f, 0.01f, 0.01f);
  for (int pass = 0; pass < 2; pass++)
    for (int n = 21; n <= 108; n++) {
      if (black[n % 12] != (pass == 1)) continue;
      RGBf c = pass ? RGBf{0.004f, 0.004f, 0.004f} : RGBf{0.5f, 0.5f, 0.5f};
      for (int k = 0; k < kMaxVoices; k++) {
        const Voice& v = engine.voices[k];
        if (v.alive() && v.held && v.note == n) c = whiten(v.color, 0.2f);
      }
      float x = wallX(engine.keyU(n));
      float w = pass ? kw * 0.32f : kw * 0.45f;
      img.rect((int)(x - w), y0, (int)(x + w), pass ? (y0 + y1) / 2 : y1, c.r, c.g, c.b);
    }
  if (engine.pedalDown) img.dot(M + 10, (y0 + y1) / 2, 4, 0.6f, 0.6f, 0.6f);
}

// ------------------------------------------------------------------ scripts

static std::vector<MidiEvent> events;
static double scriptLen = 6;
static bool scriptDemo = false;

static void note(double t, int n, int vel, double dur) {
  events.push_back({t, 0x90, n, vel});
  events.push_back({t + dur, 0x80, n, 0});
}
static void pedal(double t, bool down) { events.push_back({t, 0xB0, 64, down ? 127 : 0}); }

static bool loadScript(const std::string& s) {
  events.clear();
  scriptDemo = false;
  if (s == "single") {  // the extremes and the middle, then soft vs hard
    note(0.2, 21, 100, 0.8);
    note(1.6, 60, 100, 0.8);
    note(3.0, 108, 100, 0.8);
    note(4.4, 60, 30, 0.5);
    note(5.6, 60, 127, 0.5);
    scriptLen = 8;
  } else if (s == "chord") {  // consonant chord with pedal, then a cluster
    pedal(0.0, true);
    for (int n : {48, 52, 55, 60, 64}) note(0.2, n, 90, 2.5);
    pedal(3.0, false);
    for (int n : {60, 61, 62}) note(3.5, n, 85, 1.5);
    scriptLen = 6;
  } else if (s == "run") {  // two octaves of C major up and down
    const int sc[] = {0, 2, 4, 5, 7, 9, 11};
    int k = 0;
    for (int i = 0; i <= 14; i++, k++) note(0.2 + k * 0.1, 60 + 12 * (i / 7) + sc[i % 7], 80, 0.09);
    for (int i = 13; i >= 0; i--, k++) note(0.2 + k * 0.1, 60 + 12 * (i / 7) + sc[i % 7], 80, 0.09);
    scriptLen = 5;
  } else if (s == "pedal") {  // arpeggios piling up under the pedal, then release
    pedal(0.0, true);
    const int arp[] = {45, 52, 57, 60, 64, 69, 72, 76};
    for (int k = 0; k < 48; k++) note(0.2 + k * 0.125, arp[k % 8] + (k / 16) * 2, 60 + (k % 8) * 6, 0.12);
    pedal(6.6, false);
    scriptLen = 9;
  } else if (s == "demo") {
    scriptDemo = true;
    scriptLen = 30;
  } else {
    std::string err;
    if (!readSmf(s, events, err)) {
      fprintf(stderr, "%s\n", err.c_str());
      return false;
    }
    scriptLen = events.empty() ? 1 : events.back().t + 3;
  }
  std::stable_sort(events.begin(), events.end(),
                   [](const MidiEvent& a, const MidiEvent& b) { return a.t < b.t; });
  return true;
}

static void dispatch(const MidiEvent& e) {
  if (e.type == 0x90) engine.noteOn(e.a, e.b);
  if (e.type == 0x80) engine.noteOff(e.a);
  if (e.type == 0xB0 && e.a == 64) engine.sustain(e.b >= 64);
  if (e.type == 0xC0) engine.programChange(e.a);
}

// ------------------------------------------------------------------ running

struct Runner {
  double t = 0;
  size_t next = 0;
  double sumMs = 0, maxMs = 0;
  long steps = 0;
  static constexpr double kStep = 1.0 / 120.0;  // engine runs at 120 fps
  void advanceTo(double target) {
    while (t + kStep <= target + 1e-9) {
      while (next < events.size() && events[next].t <= t) dispatch(events[next++]);
      auto a = std::chrono::steady_clock::now();
      engine.render((float)kStep);
      double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count();
      sumMs += ms;
      maxMs = std::max(maxMs, ms);
      steps++;
      t += kStep;
    }
  }
  void report() const {
    fprintf(stderr, "engine: %ld frames, avg %.3f ms, max %.3f ms (host)\n", steps, sumMs / std::max(1L, steps), maxMs);
  }
};

static int findMode(const std::string& s) {
  if (!s.empty() && isdigit((unsigned char)s[0])) return atoi(s.c_str()) - 1;
  for (int m = 0; m < engine.modeCount(); m++) {
    std::string n = engine.modeName(m);
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    if (n.rfind(s, 0) == 0) return m;
  }
  return -1;
}

static void startRun(int mode) {
  engine.setMode(mode, false, true);
  engine.sinceNote = 0.f;  // no idle glow at the start of a script
  if (scriptDemo) engine.demo.start(7);
}

// ------------------------------------------------------------------ map view

static void hueRgb(float h, float& r, float& g, float& b) {
  RGBf c = hueColor(h);
  r = c.r;
  g = c.g;
  b = c.b;
}

static int cmdMap(const std::string& out) {
  const WallMap& m = engine.map;
  Image img(imgW(), imgH());
  // cells
  for (int y = 0; y < m.rasterH; y++)
    for (int x = 0; x < m.rasterW; x++) {
      int L = m.rasterLabels[y * m.rasterW + x];
      if (!L) continue;
      float r, g, b;
      hueRgb((L * 0.618034f), r, g, b);
      float k = m.cells[L - 1].border ? 0.02f : 0.06f;
      float u = (x + 0.5f) * m.rasterRes, v = (y + 0.5f) * m.rasterRes;
      for (int dy = 0; dy < (int)(m.rasterRes * S); dy++)
        for (int dx = 0; dx < (int)(m.rasterRes * S); dx++)
          img.set((int)(wallX(u - m.rasterRes / 2) + dx), (int)(wallY(v + m.rasterRes / 2) + dy), r * k, g * k, b * k);
    }
  // links: strip links grey, junction joins yellow
  int joins = 0, links = 0;
  for (int a = 0; a < m.n; a++)
    for (int k = 0; k < m.deg[a]; k++) {
      int b = m.nbr[a][k];
      if (b < a) continue;
      links++;
      bool chain = abs((int)m.strip[a] - (int)m.strip[b]) == 1;
      if (!chain) joins++;
      float c = chain ? 0.05f : 1.f;
      img.line(wallX(m.u[a]), wallY(m.v[a]), wallX(m.u[b]), wallY(m.v[b]), c, chain ? c : 0.8f, chain ? c : 0.f);
    }
  // edges coloured by id, junctions white, dead ends red
  for (int i = 0; i < m.n; i++) {
    float r = 1, g = 1, b = 1;
    int rad = 1;
    if (m.deg[i] >= 3) {
      rad = 3;
    } else if (m.deg[i] <= 1) {
      r = 1, g = 0.1f, b = 0.1f;
      rad = 3;
    } else if (m.edgeOf[i] >= 0) {
      hueRgb(m.edgeOf[i] * 0.381966f, r, g, b);
    }
    img.dot(wallX(m.u[i]), wallY(m.v[i]), rad, r, g, b);
  }
  // cell centroids
  for (int c = 0; c < m.cellCount; c++) img.dot(wallX(m.cells[c].cu), wallY(m.cells[c].cv), 2, 0.3f, 0.3f, 0.3f);
  drawKeyboard(img);

  int degs[6] = {0};
  for (int i = 0; i < m.n; i++) degs[std::min<int>(m.deg[i], 5)]++;
  int border = 0;
  for (int c = 0; c < m.cellCount; c++) border += m.cells[c].border;
  float minLen = 1e9f, maxLen = 0, sumLen = 0;
  for (int e = 0; e < m.edgeCount; e++) {
    minLen = std::min(minLen, m.edges[e].len);
    maxLen = std::max(maxLen, m.edges[e].len);
    sumLen += m.edges[e].len;
  }
  printf("leds %d  size %.0f x %.0f  links %d (junction joins %d)\n", m.n, m.uMax, m.vMax, links, joins);
  printf("degree: 0:%d 1:%d 2:%d 3:%d 4+:%d\n", degs[0], degs[1], degs[2], degs[3], degs[4] + degs[5]);
  printf("edges %d  len min %.1f avg %.1f max %.1f\n", m.edgeCount, minLen, sumLen / std::max(1, m.edgeCount), maxLen);
  printf("cells %d (border %d)  raster %dx%d @%.1f\n", m.cellCount, border, m.rasterW, m.rasterH, m.rasterRes);
  return writePng(img, out) ? 0 : 1;
}

// ------------------------------------------------------------------ main

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr,
            "usage:\n  sim map <out.png>\n  sim video <mode> <script> <out.mp4> [seconds]\n"
            "  sim sheet <mode> <script> <out.png> <t1,t2,...>\n  sim bench <mode> <script>\n");
    return 1;
  }
  Settings s;
  s.setDefaults();
  engine.begin(leds, sizeof(leds) / sizeof(LED), kLedsPerStrip, s);
  std::string cmd = argv[1];
  if (cmd == "map") return cmdMap(argv[2]);
  if (argc < 4) return 1;

  bool cycle = std::string(argv[2]) == "all";  // showreel: next mode every 12 s
  int mode = cycle ? 0 : findMode(argv[2]);
  if (mode < 0 || mode >= engine.modeCount()) {
    fprintf(stderr, "unknown mode %s; modes:", argv[2]);
    for (int k = 0; k < engine.modeCount(); k++) fprintf(stderr, " %d=%s", k + 1, engine.modeName(k));
    fprintf(stderr, "\n");
    return 1;
  }
  if (!loadScript(argv[3])) return 1;
  startRun(mode);
  Runner run;

  if (cmd == "bench") {
    run.advanceTo(scriptLen);
    run.report();
    return 0;
  }
  if (cmd == "video" && argc >= 5) {
    double len = argc >= 6 ? atof(argv[5]) : scriptLen;
    const int fps = 60;
    Image img(imgW(), imgH());
    char c[1024];
    snprintf(c, sizeof c,
             "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgb24 -s %dx%d -r %d -i - "
             "-c:v libx264 -pix_fmt yuv420p -crf 18 -preset medium '%s'",
             img.w, img.h, fps, argv[4]);
    FILE* p = popen(c, "w");
    if (!p) return 1;
    if (cycle && argc < 6) len = 12.0 * engine.modeCount();
    for (int f = 0; f < (int)(len * fps); f++) {
      if (cycle && f > 0 && f % (12 * fps) == 0) engine.setMode((engine.mode() + 1) % engine.modeCount(), false);
      run.advanceTo((double)f / fps);
      drawWall(img);
      drawKeyboard(img);
      if (cycle)  // mode number as dots in the top-right corner
        for (int q = 0; q <= engine.mode(); q++) img.rect(img.w - 14 - q * 10, 6, img.w - 8 - q * 10, 12, 0.5f, 0.5f, 0.5f);
      auto b = img.srgb();
      fwrite(b.data(), 1, b.size(), p);
    }
    pclose(p);
    run.report();
    return 0;
  }
  if (cmd == "sheet" && argc >= 6) {
    std::vector<double> times;
    for (char* tok = strtok(argv[5], ","); tok; tok = strtok(nullptr, ",")) times.push_back(atof(tok));
    std::sort(times.begin(), times.end());
    int cols = std::min<int>(3, (int)times.size());
    int rows = ((int)times.size() + cols - 1) / cols;
    Image frame(imgW(), imgH());
    Image sheet(frame.w * cols + 4 * (cols - 1), frame.h * rows + 4 * (rows - 1));
    std::fill(sheet.px.begin(), sheet.px.end(), 0.15f);
    for (size_t k = 0; k < times.size(); k++) {
      run.advanceTo(times[k]);
      drawWall(frame);
      drawKeyboard(frame);
      // time marker: one tick per 0.5 s along the top edge
      for (int q = 0; q < (int)(times[k] * 2 + 0.5); q++) frame.rect(6 + q * 8, 6, 11 + q * 8, 12, 0.5f, 0.5f, 0.5f);
      int ox = (int)(k % cols) * (frame.w + 4), oy = (int)(k / cols) * (frame.h + 4);
      for (int y = 0; y < frame.h; y++)
        for (int x = 0; x < frame.w; x++) {
          const float* p = &frame.px[((size_t)y * frame.w + x) * 3];
          sheet.set(ox + x, oy + y, p[0], p[1], p[2]);
        }
    }
    run.report();
    return writePng(sheet, argv[4], 0.5f) ? 0 : 1;
  }
  fprintf(stderr, "bad arguments\n");
  return 1;
}
