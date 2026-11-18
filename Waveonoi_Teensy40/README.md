# Waveonoi: piano-reactive LED wall

Teensy 4.0 firmware for the Voronoi LED wall (1761 WS2811 LEDs on 8 OctoWS2811
outputs). Every key you press on the piano starts an animation from the spot on
the wall behind that key, using the piano's MIDI output.

## Animations

Pick one on the piano: hold **A0 + B0** (the two lowest white keys) and tap a
white key from middle C upward. The wall flashes to confirm.

| Key | Animation | What it does |
|---|---|---|
| C4 | Ripples | Rings expand from the key like raindrops; chords overlap and mix |
| D4 | Veins | A pulse runs through the lines, splitting at every junction; junctions flare |
| E4 | Rising notes | A column grows up from the key while held, then floats to the top |
| F4 | Sparks | Particles burst up and fall back; hard notes fire a shell that explodes |
| G4 | Lightning | A bolt forks up the network, flashes white, flickers, glows out |
| A4 | Fireflies | Comets wander the network while the note sounds; the pedal keeps them alive |
| B4 | Coral | Notes grow a lasting structure in their colour; it withers when you stop |
| C5 | Honeycomb | The cell above the key lights up, then the light hops outward cell by cell |
| D5 | Interference | Each note is a wave source; chords draw interference patterns, dissonance shimmers |
| E5 | Resonance | Line segments are strings tuned by length; matching ones ring in sympathy |

A0 + B0 + **C8** starts or stops a built-in demo pianist.

All animations share the same rules:
- Colour follows the circle of fifths (C red, G orange, D yellow, A lime, E green,
  B cyan, F# sky, C# blue, G# violet, D# purple, A# pink, F rose), so consonant
  chords get neighbouring colours.
- Velocity sets brightness, speed and reach.
- Notes decay like piano strings: bass rings long, treble short.
- The sustain pedal works as on a piano.

After 20 s without playing, a dim ember glow drifts through the lines. It
disappears on the first key.

## Hardware: MIDI in

The RD-150 has a 5-pin DIN **MIDI OUT**. Connect it to Teensy pin 16 (Serial4 RX)
through an optocoupler. This is the standard circuit from
https://www.pjrc.com/teensy/td_libs_MIDI.html:

```
 MIDI socket (female)            6N138                  Teensy 4.0
                               +-------+
 DIN pin 4 --[220R]--+---------|2     8|--------------- 3.3V
                     |         |       |
                   1N4148      |      6|---+----------- pin 16 (RX4)
                (band at top)  |       |   |
 DIN pin 5 ----------+---------|3     5|-+ +--[470R]--- 3.3V
                               +-------+ |
                                         +------------- GND

 DIN pin 2 (shield) and 6N138 pins 1, 4, 7: not connected
```

You can test everything before the circuit is built. With `-D USB_MIDI_SERIAL`
the Teensy is also a USB MIDI device:

```
python3 sim/piano.py                        # virtual piano: mouse or laptop keyboard
aplaymidi -l                                # find the Teensy's port, e.g. 24:0
python3 sim/make_test_midi.py               # writes sim/midi/keys.mid and playing.mid
aplaymidi -p 24:0 sim/midi/keys.mid         # calibration sweep (use with `cal`)
aplaymidi -p 24:0 sim/midi/playing.mid      # notes, chords, runs, pedal
```

## First-time setup

Upload with `pio run -t upload`, then open the console with `pio device monitor`
and type `help`. Outside VS Code, `pio` lives at `~/.platformio/penv/bin/pio`;
`ln -s ~/.platformio/penv/bin/pio ~/.local/bin/pio` puts it on the PATH.

1. **Orientation:** type `test`. You should see red along the bottom, green on
   the left, blue at the top, and a white bar sweeping left to right. If not,
   use `flipu` or `flipv`. Type `test` again to leave.
2. **Key positions:** type `cal`. The red line marks where the wall thinks A0 is,
   green marks middle C, blue marks C8, and white marks any key you hold down.
   Adjust with `a0 <u>` and `c8 <u>` (wall units: 0 = left edge, about 392 =
   right edge) until the lines sit behind the real keys. `keyv <v>` sets the
   height the light starts from.
3. Settings are stored in EEPROM. `defaults` restores the defaults.

Other commands:
- `m [n]`: list or select an animation.
- `demo`: start or stop the demo pianist.
- `bri`: brightness.
- `idle`: seconds of silence before the ember glow.
- `pc`: let MIDI program changes select the animation.
- `mon`: MIDI monitor.
- `stat`: frame rate and render time.

The ESP32 link gets one new message: `4,<n>` selects animation n.

## Simulator

The animation engine (`src/engine`) is plain C++, so it also runs on a PC.
Animations can be developed and previewed without the wall:

```
make -C sim
sim/sim map out.png                          # derived network, junctions, cells
sim/sim video veins demo out/veins.mp4       # MP4 preview
sim/sim video all demo out/showreel.mp4      # every animation, 12 s each
sim/sim sheet coral pedal out/coral.png 1,3,5  # stills at given seconds
```

Scripts: `single`, `chord`, `run`, `pedal`, `demo`, or a path to any `.mid` file.

## Code layout

- `src/Waveonoi.ino`: setup and main loop, LED output, settings.
- `src/Midi.ino`: DIN and USB MIDI input.
- `src/Console.ino`: USB serial commands.
- `src/Serial.ino`, `src/Time.ino`: ESP32 link and clock (unchanged).
- `src/ledLayout.h`: LED positions. Layout x is wall height (0 = bottom),
  layout y is wall width.
- `src/engine/`:
  - `WallMap` derives the strip network, junctions, segments and cells from the
    layout at boot.
  - `Engine` handles voices, envelopes, modes, crossfades, the idle glow, test
    patterns and output.
  - `modes/` has one file per animation.
- `sim/`: desktop simulator.
