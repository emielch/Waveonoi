// GridCell.h

#ifndef _GRIDCELL_h
#define _GRIDCELL_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "LED.h"

class GridCell
{
protected:
	LED* lastLed;


public:
	int xs, xe, ys, ye;
	LED* firstLed;

	GridCell();
	void setCoord(int _xs, int _xe, int _ys, int _ye);
	void addLed(LED* newLed);
	void scanLeds(LED* leds, int ledAm);
};


#endif

