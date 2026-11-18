// Piano-reactive animation engine for the Waveonoi wall.
//
// Feed it MIDI-style events (noteOn/noteOff/sustain), call render(dt) once
// per frame, then read every LED with pixel(). No Arduino dependencies: the
// simulator in ../../sim runs the exact same code on a PC.
#pragma once
#include "Colors.h"
#include "Config.h"
#include "Demo.h"
#include "FieldCache.h"
#include "Frame.h"
#include "Mode.h"
#include "Voices.h"
#include "WallMap.h"

struct LED;

namespace wv {

struct Settings {
  uint32_t magic;     // marks valid settings in EEPROM
  float keyA0U;       // wall position (u) of the lowest key, A0
  float keyC8U;       // wall position (u) of the highest key, C8
  float keyV;         // height the keys emit from (0 = bottom LEDs)
  uint8_t flipU;      // mirror left/right
  uint8_t flipV;      // mirror top/bottom
  uint8_t mode;       // current animation
  uint8_t pcSwitch;   // MIDI program change selects the animation
  float brightness;   // 0..1 master
  float idleSeconds;  // silence before the ambient glow starts

  static constexpr uint32_t kMagic = 0x57564E31;  // "WVN1"
  void setDefaults();
};

class Engine {
 public:
  enum Test : uint8_t { kTestOff = 0, kTestOrientation, kTestCalibration };

  void begin(const LED* leds, int count, int ledsPerStrip, const Settings& s);
  void rebuildMap();  // after changing flipU / flipV
  void keysChanged();  // after changing keyA0U / keyC8U / keyV

  // Input
  void noteOn(int note, int velocity);
  void noteOff(int note);
  void sustain(bool down);
  void programChange(int program);
  void allNotesOff();

  // Modes
  void setMode(int m, bool flash = true, bool instant = false);
  int mode() const { return cur; }
  int modeCount() const { return nModes; }
  const char* modeName(int m) const;

  // Frame
  void render(float dt);
  void pixel(int i, uint8_t& r, uint8_t& g, uint8_t& b) const;
  float keyU(int note) const;
  bool anySounding() const;

  Settings settings;
  WallMap map;
  Voice voices[kMaxVoices];
  bool pedalDown = false;
  FieldCache fields;
  Frame frame;
  Rng rng;
  Demo demo;
  uint8_t test = kTestOff;
  float time = 0.f;
  float sinceNote = 1e6f;      // seconds since any key/pedal activity
  float master = 1.f;          // extra brightness factor (firmware on/off fade)
  bool settingsDirty = false;  // settings changed from the piano; firmware saves them

 private:
  static constexpr int kMaxModes = 16;
  Mode* modes[kMaxModes];
  int nModes = 0;
  int cur = 0, prev = -1;
  float xfade = 1.f;
  float idleLevel = 0.f;
  float flashT = 10.f;
  uint32_t nextId = 1;
  bool keyDown[128];
  bool swallowed[128];
  const LED* layout = nullptr;
  int layoutCount = 0, layoutPerStrip = 0;
  uint8_t lut[4096];

  float decayTime(int note) const;
  void updateVoices(float dt);
  void releaseVoice(int vi);
  bool gesture(int note);
  void renderIdle(float dt);
  void renderTest();
  template <class F>
  void forActive(F&& f) {
    if (prev >= 0) f(modes[prev]);
    f(modes[cur]);
  }
};

}  // namespace wv
