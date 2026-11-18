#pragma once
#include "FastMath.h"

namespace wv {

class Engine;

// A small generative pianist: chords, bass, melody, runs and pedalling.
// Used by the `demo` console command and by the simulator.
class Demo {
 public:
  bool active() const { return on; }
  void start(uint32_t seed = 1);
  void stop(Engine& e);
  void update(Engine& e, float dt);

 private:
  struct Off {
    float t;
    uint8_t note;
  };
  bool on = false;
  float t = 0.f, next = 0.f;
  int step = 0;
  Off offs[64];
  int nOff = 0;
  Rng rng;
  void play(Engine& e, int note, int vel, float dur);
};

}  // namespace wv
