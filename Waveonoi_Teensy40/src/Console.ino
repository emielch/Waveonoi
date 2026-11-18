// Commands over the USB serial port (PlatformIO: `pio device monitor`).
// Type `help` for the list. Changed settings are stored in EEPROM.

#define CONSOLE_SIZE 64
char consoleBuf[CONSOLE_SIZE + 1];
int consolePos = 0;

void checkConsole() {
	while (Serial.available()) {
		char c = (char)Serial.read();
		if (c == '\n' || c == '\r') {
			if (consolePos > 0) {
				consoleBuf[consolePos] = 0;
				runCommand(consoleBuf);
				consolePos = 0;
			}
		} else if (consolePos < CONSOLE_SIZE) {
			consoleBuf[consolePos++] = c;
		}
	}
}

void printSettings() {
	wv::Settings& s = engine.settings;
	Serial.printf("mode %d (%s)  a0 %.1f  c8 %.1f  keyv %.1f  flipu %d  flipv %d  bri %d  idle %.0f  pc %s\n",
	              engine.mode() + 1, engine.modeName(engine.mode()), s.keyA0U, s.keyC8U, s.keyV, s.flipU, s.flipV,
	              (int)(s.brightness * 100 + 0.5f), s.idleSeconds, s.pcSwitch ? "on" : "off");
}

void printHelp() {
	Serial.println(
	    "help            this list\n"
	    "m [n]           list animations / select animation n\n"
	    "demo            start/stop the built-in demo pianist\n"
	    "test            orientation pattern: red = bottom, green = left, blue = top,\n"
	    "                white bar sweeps left -> right\n"
	    "cal             key calibration: red = A0, green = middle C, blue = C8,\n"
	    "                white = keys you hold down\n"
	    "a0 <u>          wall position of the lowest key (0 = left edge)\n"
	    "c8 <u>          wall position of the highest key\n"
	    "keyv <v>        height the keys emit from (0 = bottom LEDs)\n"
	    "flipu / flipv   mirror the wall left-right / top-bottom\n"
	    "bri <0-100>     brightness\n"
	    "idle <seconds>  silence before the ambient glow\n"
	    "pc              toggle: MIDI program change selects the animation\n"
	    "mon             toggle MIDI monitor\n"
	    "stat            frame rate, render time, voices\n"
	    "save            store settings now\n"
	    "defaults        restore default settings\n"
	    "On the piano: hold A0 + B0 and tap C4, D4, E4, ... to pick an animation; C8 toggles the demo.");
}

void runCommand(char* line) {
	char* cmd = strtok(line, " ");
	char* arg = strtok(0, " ");
	if (!cmd) return;
	wv::Settings& s = engine.settings;
	bool changed = false;

	if (!strcmp(cmd, "help") || !strcmp(cmd, "?")) {
		printHelp();
	} else if (!strcmp(cmd, "m")) {
		if (arg) {
			engine.setMode(atoi(arg) - 1);
			changed = true;
		} else {
			for (int i = 0; i < engine.modeCount(); i++)
				Serial.printf("%c %2d  %s\n", i == engine.mode() ? '>' : ' ', i + 1, engine.modeName(i));
		}
	} else if (!strcmp(cmd, "demo")) {
		if (engine.demo.active()) {
			engine.demo.stop(engine);
		} else {
			engine.demo.start(micros());
		}
		Serial.println(engine.demo.active() ? "demo on" : "demo off");
	} else if (!strcmp(cmd, "test")) {
		engine.test = engine.test == wv::Engine::kTestOrientation ? wv::Engine::kTestOff : wv::Engine::kTestOrientation;
	} else if (!strcmp(cmd, "cal")) {
		engine.test = engine.test == wv::Engine::kTestCalibration ? wv::Engine::kTestOff : wv::Engine::kTestCalibration;
	} else if (!strcmp(cmd, "a0") && arg) {
		s.keyA0U = atof(arg);
		engine.keysChanged();
		changed = true;
	} else if (!strcmp(cmd, "c8") && arg) {
		s.keyC8U = atof(arg);
		engine.keysChanged();
		changed = true;
	} else if (!strcmp(cmd, "keyv") && arg) {
		s.keyV = atof(arg);
		engine.keysChanged();
		changed = true;
	} else if (!strcmp(cmd, "flipu") || !strcmp(cmd, "flipv")) {
		if (cmd[4] == 'u')
			s.flipU = !s.flipU;
		else
			s.flipV = !s.flipV;
		engine.rebuildMap();
		changed = true;
	} else if (!strcmp(cmd, "bri") && arg) {
		s.brightness = constrain(atof(arg), 0, 100) / 100.f;
		changed = true;
	} else if (!strcmp(cmd, "idle") && arg) {
		s.idleSeconds = max(1.f, (float)atof(arg));
		changed = true;
	} else if (!strcmp(cmd, "pc")) {
		s.pcSwitch = !s.pcSwitch;
		changed = true;
	} else if (!strcmp(cmd, "mon")) {
		midiMonitor = !midiMonitor;
		Serial.println(midiMonitor ? "MIDI monitor on" : "MIDI monitor off");
	} else if (!strcmp(cmd, "stat")) {
		int voices = 0;
		for (int i = 0; i < wv::kMaxVoices; i++) voices += engine.voices[i].alive();
		Serial.printf("fps %.0f  render %lu us  voices %d  pedal %s  piano %s\n", getFPS(), renderMicros, voices,
		              engine.pedalDown ? "down" : "up", activeSensing ? "connected" : "-");
	} else if (!strcmp(cmd, "save")) {
		changed = true;
	} else if (!strcmp(cmd, "defaults")) {
		s.setDefaults();
		engine.rebuildMap();
		engine.setMode(s.mode);
		changed = true;
	} else {
		Serial.printf("unknown command '%s', try 'help'\n", cmd);
		return;
	}
	if (changed) saveSettings();
	printSettings();
}
