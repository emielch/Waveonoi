
#include "ledLayout.h"
#include "Orb.h"
#include <Colore.h>
#include "LED.h"
#include "LUT.h"

#include <OctoWS2811.h>

const int ledsPerStrip = 227;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 strip(ledsPerStrip, displayMemory, drawingMemory, config);

RGB drawBuff[ledsPerStrip * 8];

const int orbAm = 5;
Orb orbs[orbAm];


void set_ledLib(int pixel, byte r, byte g, byte b) {
	//strip.setPixel(pixel, LUT[r], LUT[g], LUT[b]);
	//strip.setPixel(pixel, r, g, b);
	drawBuff[pixel].r = r;
	drawBuff[pixel].g = g;
	drawBuff[pixel].b = b;
}


void show_ledLib() {
	for (int i = 0; i < ledsPerStrip*8; i++) {
		strip.setPixel(i, LUT[drawBuff[i].r], LUT[drawBuff[i].g], LUT[drawBuff[i].b]);
	}
	strip.show();
}

uint32_t bufsize = ledsPerStrip * 24;
void reset_ledLib() {
	//memset(drawingMemory, 0, bufsize);
	memset(drawBuff, 0, bufsize);
}


Color get_ledLib(int pixel) {
	//uint32_t conn = strip.getPixel(pixel);  // retrieve the color that has already been saved
	//byte b = conn & 255;       // unpack the color
	//byte g = conn >> 8 & 255;
	//byte r = conn >> 16 & 255;
	Color pixelCol(drawBuff[pixel].r, drawBuff[pixel].g, drawBuff[pixel].b, RGB_MODE);
	return pixelCol;
}


int xMax = 0;
int yMax = 0;
int ledAm = 0;


void setup() {
	delay(1000);
	strip.begin();
	ledAm = sizeof(leds) / sizeof(LED);
	Serial.println(ledAm);

	for (int i = 0; i < ledAm; i++) {
		if (leds[i].xPos > xMax) xMax = leds[i].xPos;
		if (leds[i].yPos > yMax) yMax = leds[i].yPos;
	}

	Serial.println(xMax);
	Serial.println(yMax);

	for (int i = 0; i < orbAm; i++) {
		orbs[i].init(random(150,400), 55, i*137.508, xMax, yMax);
	}
}

elapsedMillis sincePrint = 0;

float dt;

void loop() {
	reset_ledLib();

	for (int i = 0; i < orbAm; i++) {
		orbs[i].render();
	}

	show_ledLib();

	for (int i = 0; i < orbAm; i++) {
		orbs[i].move(dt);
	}

	calcDt();
	if (sincePrint > 1000) {
		Serial.print("FPS: ");
		Serial.println(getFPS());
		sincePrint -= 1000;
	}
}


void render() {
	
}



uint32_t lastCalc; // variable to keep track of the loops per second


void calcDt() {
	uint32_t currTime = micros();
	dt = (currTime - lastCalc) / 1000000.;  // assume one frame per second
	lastCalc = currTime;
}

float getFPS() {
	return 1 / dt;
}