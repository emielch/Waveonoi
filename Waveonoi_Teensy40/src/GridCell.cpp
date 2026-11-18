// 
// 
// 

#include "GridCell.h"

GridCell::GridCell()
{
	firstLed = NULL;
	lastLed = NULL;
}

void GridCell::setCoord(int _xs, int _xe, int _ys, int _ye)
{
	xs = _xs;
	xe = _xe;
	ys = _ys;
	ye = _ye;

	Serial.print(_xs);
	Serial.print(" ");
	Serial.print(_xe);
	Serial.print(" ");
	Serial.print(_ys);
	Serial.print(" ");
	Serial.print(_ye);
	Serial.println(" ");
}

void GridCell::addLed(LED * newLed)
{
	if (firstLed == NULL) {
		firstLed = newLed;
		lastLed = newLed;
	}
	else {
		lastLed->nextLED = newLed;
		lastLed = newLed;
		lastLed->nextLED = NULL;
	}
}

void GridCell::scanLeds(LED * leds, int ledAm)
{
	int c = 0;
	for (int i = 0; i < ledAm; i++) {
		if (leds[i].xPos >= xs && leds[i].xPos <= xe && leds[i].yPos >= ys && leds[i].yPos <= ye) {
			addLed(&leds[i]);
			c++;
		}
	}
	Serial.println(c);
}
