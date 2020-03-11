// LED.h

#ifndef _LED_h
#define _LED_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

struct LED {
	byte channel, ledID;
	int xPos, yPos;
	LED* nextLED;
};



#endif

