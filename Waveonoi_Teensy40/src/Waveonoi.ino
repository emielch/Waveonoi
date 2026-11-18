// Waveonoi: piano-reactive LED wall.
//
// MIDI from the piano (DIN on Serial4 / pin 16, or USB MIDI) drives the
// animation engine in src/engine. Other files in this sketch:
//   Midi.ino     MIDI input
//   Console.ino  USB serial commands (calibration, modes, demo, ...)
//   Serial.ino   link to the ESP32 web server on Serial1
//   Time.ino     clock and on/off timer helpers

#include <EEPROM.h>
#include <MIDI.h>
#include <OctoWS2811_Dither.h>

#include "engine/Engine.h"
#include "ledLayout.h"

const int ledsPerStrip = 227;

DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811_Dither strip(ledsPerStrip, displayMemory, drawingMemory, config);

const int ledAm = sizeof(leds) / sizeof(LED);
wv::Engine engine;

bool masterSwitch = true;
unsigned long onTime = 28800;
unsigned long offTime = 72000;

bool turnedOn = true;
float bri = 100;  // on/off fade, percent
float fadeSpd = 20;

// Shared between the .ino files (they are concatenated after this one).
bool midiMonitor = false;
bool activeSensing = false;  // the piano's DIN MIDI is connected
elapsedMillis sinceSensing;
float dt = 0;
uint32_t renderMicros = 0;

void setup() {
	Serial1.begin(115200);
	strip.begin();
	midiBegin();
	loadSettings();
	wv::Settings s = engine.settings;
	engine.begin(leds, ledAm, ledsPerStrip, s);
	delay(300);
	Serial.printf("Waveonoi: %d LEDs, %d cells, %d segments, mode %d (%s). Type 'help'.\n", engine.map.n,
	              engine.map.cellCount, engine.map.edgeCount, engine.mode() + 1, engine.modeName(engine.mode()));
}

void loop() {
	checkSerial();
	checkConsole();
	readMidi();

	if (turnedOn && bri < 100) {
		bri += fadeSpd * dt;
		if (bri > 100) bri = 100;
	} else if (!turnedOn && bri > 0) {
		bri -= fadeSpd * dt;
		if (bri < 0) bri = 0;
	}
	engine.master = bri / 100.f;

	uint32_t t0 = micros();
	engine.render(dt);
	for (int i = 0; i < engine.map.n; i++) {
		uint8_t r, g, b;
		engine.pixel(i, r, g, b);
		strip.setPixel(engine.map.strip[i], r, g, b);
	}
	renderMicros = micros() - t0;
	strip.show();

	if (engine.settingsDirty) {
		engine.settingsDirty = false;
		saveSettings();
	}
	calcDt();
}

void turnOn() {
	if (masterSwitch) {
		if (turnedOn) return;
		turnedOn = true;
		Serial.println("turn On");
	}
}

void turnOff() {
	if (!turnedOn) return;
	turnedOn = false;
	Serial.println("turn Off");
}

void loadSettings() {
	wv::Settings s;
	EEPROM.get(0, s);
	bool ok = s.magic == wv::Settings::kMagic && isfinite(s.keyA0U) && isfinite(s.keyC8U) && isfinite(s.keyV) &&
	          isfinite(s.brightness) && isfinite(s.idleSeconds) && s.brightness >= 0 && s.brightness <= 1;
	if (!ok) s.setDefaults();
	engine.settings = s;
}

void saveSettings() {
	EEPROM.put(0, engine.settings);
}

uint32_t lastCalc;

void calcDt() {
	uint32_t currTime = micros();
	dt = (currTime - lastCalc) / 1000000.;
	lastCalc = currTime;
}

float getFPS() {
	return dt > 0 ? 1 / dt : 0;
}
