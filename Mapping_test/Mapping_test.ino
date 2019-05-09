
#include "GridCell.h"
#include "ledLayout.h"
#include "Orb.h"
#include <Colore.h>
#include "LED.h"

#include <TimeLib.h>
#include <TimeAlarms.h>

#include <OctoWS2811_Dither.h>

const int ledsPerStrip = 227;
DMAMEM int displayMemory[ledsPerStrip * 6];
COL_RGB copyMemory[ledsPerStrip * 8];
COL_RGB drawingMemory[ledsPerStrip * 8];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811_Dither strip(ledsPerStrip, displayMemory, copyMemory, drawingMemory, config);

const int orbAm = 5;
Orb orbs[orbAm];

#define GRID_X 3
#define GRID_Y 5
const int gridcellAm = GRID_X * GRID_Y;
GridCell grid[gridcellAm];

AlarmId onAlarmID, offAlarmID;
long onTime = 28800;
long offTime = 72000;
bool masterSwitch = true;


void set_ledLib(int pixel, byte r, byte g, byte b) {
	strip.setPixel(pixel, r, g, b);
	//strip.setPixel(pixel, r, g, b);
}


void show_ledLib() {
	//for (int i = 0; i < ledsPerStrip*8; i++) {
	//	strip.setPixel(i, LUT[drawBuff[i].r], LUT[drawBuff[i].g], LUT[drawBuff[i].b]);
	//}
	strip.show();
}

uint32_t bufsize = ledsPerStrip * 24;
void reset_ledLib() {
	memset(drawingMemory, 0, bufsize);
}


Color get_ledLib(int pixel) {
	//uint32_t conn = strip.getPixel(pixel);  // retrieve the color that has already been saved
	//byte b = conn & 255;       // unpack the color
	//byte g = conn >> 8 & 255;
	//byte r = conn >> 16 & 255;
	Color pixelCol(drawingMemory[pixel].r, drawingMemory[pixel].g, drawingMemory[pixel].b, RGB_MODE);
	return pixelCol;
}


int xMax = 0;
int yMax = 0;
int ledAm = 0;
float bri = 100;
bool turnedOn = true;
float fadeSpd = 20;


void setup() {
	//while (!Serial) {}
	Serial1.begin(115200);
	setSyncProvider(getTeensy3Time);
	delay(1000);
	strip.begin();
	ledAm = sizeof(leds) / sizeof(LED);
	Serial.println(ledAm);

	initGrid();

	for (int i = 0; i < orbAm; i++) {
		orbs[i].init(random(150, 400), 55, i*137.508, xMax, yMax);
	}

	if (timeStatus() != timeSet) {
		Serial.println("Unable to sync with the RTC");
	}
	else {
		Serial.println("RTC has set the system time");
	}

	setAlarms();
}

time_t getTeensy3Time()
{
	return Teensy3Clock.get();
}

elapsedMillis sincePrint = 0;

float dt;

void loop() {
	Alarm.delay(0);
	checkSerial();
	reset_ledLib();

	if (turnedOn && bri < 100) {
		bri += fadeSpd * dt;
		if (bri > 100) bri = 100;
		updateOrbBri();
	}
	else if (!turnedOn && bri > 0) {
		bri -= fadeSpd * dt;
		if (bri < 0) bri = 0;
		updateOrbBri();
	}


	for (int i = 0; i < orbAm; i++) {
		orbs[i].render();
	}

	show_ledLib();

	for (int i = 0; i < orbAm; i++) {
		orbs[i].move(dt);
	}

	calcDt();
	if (sincePrint > 1000) {
		Serial.println();
		Serial.print("FPS: \t");
		Serial.println(getFPS());
		Serial.print("Curr: \t");
		printTime(now());
		Serial.print("ON: \t");
		printTime(Alarm.read(onAlarmID));
		Serial.print("OFF: \t");
		printTime(Alarm.read(offAlarmID));
		Serial.println();

		sincePrint -= 1000;
	}
}


void turnOn() {
	if (masterSwitch) {
		turnedOn = true;
		Serial.println("turn On");
	}
}

void turnOff() {
	turnedOn = false;
	Serial.println("turn Off");
}

void printTime(long t) {
	Serial.print(getHours(t));
	printDigits(getMinutes(t));
	printDigits(getSeconds(t));
	Serial.println();
}

void printDigits(int digits) {
	// utility function for digital clock display: prints preceding colon and leading 0
	Serial.print(":");
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}


void updateOrbBri() {
	for (int i = 0; i < orbAm; i++) {
		orbs[i].setBri(bri);
	}
}

void initGrid() {
	for (int i = 0; i < ledAm; i++) {
		if (leds[i].xPos > xMax) xMax = leds[i].xPos;
		if (leds[i].yPos > yMax) yMax = leds[i].yPos;
	}

	Serial.println(xMax);
	Serial.println(yMax);


	float xsize = (float)(xMax + 1) / GRID_X;
	float ysize = (float)(yMax + 1) / GRID_Y;

	for (int x = 0; x < GRID_X; x++) {
		for (int y = 0; y < GRID_Y; y++) {
			grid[x + y * GRID_X].setCoord(x*xsize, (x + 1)*xsize - 1, y*ysize, (y + 1)*ysize - 1);
			grid[x + y * GRID_X].scanLeds(leds, ledAm);
		}
	}
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