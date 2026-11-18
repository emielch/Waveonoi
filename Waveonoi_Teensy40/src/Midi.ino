// MIDI input: the piano's DIN MIDI OUT through a 6N138 optocoupler into
// Serial4 (RX = pin 16), and USB MIDI from a computer (handy for testing:
// `aplaymidi -p <port> song.mid`). Both feed the same engine.

MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, midiDin);

void midiBegin() {
	midiDin.begin(MIDI_CHANNEL_OMNI);
	midiDin.turnThruOff();
}

void handleMidi(const char* src, uint8_t type, uint8_t d1, uint8_t d2) {
	if (type == 0xFE) {  // active sensing: the piano is connected
		activeSensing = true;
		sinceSensing = 0;
		return;
	}
	if (midiMonitor) Serial.printf("%s  %02X  %3d %3d\n", src, type, d1, d2);
	switch (type) {
		case 0x90:
			engine.noteOn(d1, d2);  // velocity 0 counts as note off
			break;
		case 0x80:
			engine.noteOff(d1);
			break;
		case 0xB0:
			if (d1 == 64) engine.sustain(d2 >= 64);
			if (d1 == 120 || d1 == 123) engine.allNotesOff();
			break;
		case 0xC0:
			engine.programChange(d1);
			break;
	}
}

void readMidi() {
	for (int n = 0; n < 64 && midiDin.read(); n++)
		handleMidi("din", (uint8_t)midiDin.getType(), midiDin.getData1(), midiDin.getData2());
	for (int n = 0; n < 64 && usbMIDI.read(); n++)
		handleMidi("usb", usbMIDI.getType(), usbMIDI.getData1(), usbMIDI.getData2());

	// The piano sends active sensing every ~300 ms. If it stops, the cable was
	// pulled or the piano switched off: release everything so nothing hangs.
	if (activeSensing && sinceSensing > 600) {
		activeSensing = false;
		engine.allNotesOff();
		if (midiMonitor) Serial.println("din  connection lost: all notes off");
	}
}
