// Minimal Standard MIDI File reader (format 0/1): notes, sustain pedal, program changes.
#pragma once
#include <string>
#include <vector>

struct MidiEvent {
  double t;      // seconds
  int type;      // 0x90 note on, 0x80 note off, 0xB0 control change, 0xC0 program change
  int a, b;      // note/controller/program, velocity/value
};

bool readSmf(const std::string& path, std::vector<MidiEvent>& out, std::string& err);
