#include "Smf.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace {

struct Raw {
  long tick;
  int order;  // keeps file order for equal ticks
  int status, a, b;
  long tempo;  // > 0 for tempo meta events
};

struct Reader {
  const std::vector<unsigned char>& d;
  size_t p;
  size_t end;
  bool ok() const { return p < end; }
  int u8() { return p < end ? d[p++] : 0; }
  long be(int n) {
    long v = 0;
    for (int i = 0; i < n; i++) v = (v << 8) | u8();
    return v;
  }
  long vlq() {
    long v = 0;
    for (int i = 0; i < 4; i++) {
      int c = u8();
      v = (v << 7) | (c & 0x7F);
      if (!(c & 0x80)) break;
    }
    return v;
  }
};

}  // namespace

bool readSmf(const std::string& path, std::vector<MidiEvent>& out, std::string& err) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    err = "cannot open " + path;
    return false;
  }
  std::vector<unsigned char> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  Reader r{d, 0, d.size()};
  if (d.size() < 14 || r.be(4) != 0x4D546864) {  // "MThd"
    err = "not a MIDI file";
    return false;
  }
  long hlen = r.be(4);
  r.be(2);  // format
  int ntracks = (int)r.be(2);
  int division = (int)r.be(2);
  r.p = 8 + hlen;
  if (division & 0x8000) {
    err = "SMPTE time division not supported";
    return false;
  }

  std::vector<Raw> raw;
  int order = 0;
  for (int tr = 0; tr < ntracks && r.p + 8 <= d.size(); tr++) {
    long id = r.be(4), len = r.be(4);
    size_t start = r.p, stop = std::min(d.size(), start + (size_t)len);
    if (id != 0x4D54726B) {  // "MTrk"
      r.p = stop;
      continue;
    }
    Reader t{d, start, stop};
    long tick = 0;
    int running = 0;
    while (t.ok()) {
      tick += t.vlq();
      int st = t.u8();
      if (st == 0xFF) {
        int type = t.u8();
        long l = t.vlq();
        if (type == 0x51 && l == 3) {
          raw.push_back({tick, order++, 0, 0, 0, t.be(3)});
        } else {
          t.p += l;
        }
        if (type == 0x2F) break;
        continue;
      }
      if (st == 0xF0 || st == 0xF7) {
        t.p += t.vlq();
        continue;
      }
      int a;
      if (st & 0x80) {
        running = st;
        a = t.u8();
      } else {
        a = st;
        st = running;
      }
      int kind = st & 0xF0;
      int b = (kind == 0xC0 || kind == 0xD0) ? 0 : t.u8();
      if (kind == 0x80 || kind == 0x90 || kind == 0xB0 || kind == 0xC0)
        raw.push_back({tick, order++, kind, a, b, 0});
    }
    r.p = stop;
  }
  std::sort(raw.begin(), raw.end(), [](const Raw& x, const Raw& y) {
    return x.tick != y.tick ? x.tick < y.tick : x.order < y.order;
  });

  double secPerTick = 0.5 / division;  // 120 bpm until told otherwise
  double t = 0;
  long last = 0;
  for (const Raw& e : raw) {
    t += (e.tick - last) * secPerTick;
    last = e.tick;
    if (e.tempo > 0) {
      secPerTick = e.tempo / 1e6 / division;
      continue;
    }
    int type = e.status;
    if (type == 0x90 && e.b == 0) type = 0x80;
    out.push_back({t, type, e.a, e.b});
  }
  return true;
}
