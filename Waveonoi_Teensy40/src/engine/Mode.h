#pragma once

namespace wv {

class Engine;
struct Frame;

// An animation. The engine forwards note events and calls render() once per
// frame; render() advances the animation by dt and adds light scaled by gain
// (gain < 1 while cross-fading between modes).
class Mode {
 public:
  virtual const char* name() const = 0;
  virtual void reset(Engine&) {}                // drop all state
  virtual void noteOn(Engine&, int voice) {}    // key struck
  virtual void keyUp(Engine&, int voice) {}     // key released (may still sustain)
  virtual void release(Engine&, int voice) {}   // sound stops (key up and pedal up)
  virtual void pedal(Engine&, bool down) {}
  virtual void render(Engine&, Frame&, float dt, float gain) = 0;
};

// All modes, in the order of the mode-switch keys (C4, D4, E4, ...).
Mode* modeRipples();
Mode* modeVeins();
Mode* modeRising();
Mode* modeSparks();
Mode* modeLightning();
Mode* modeFireflies();
Mode* modeCoral();
Mode* modeHoneycomb();
Mode* modeInterference();
Mode* modeResonance();

}  // namespace wv
