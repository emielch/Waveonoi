// Orb.h

#ifndef _ORB_h
#define _ORB_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <Colore.h>
#include "LED.h"

extern LED leds[];

extern int ledAm;
extern const int ledsPerStrip;
extern void set_ledLib(int pixel, byte r, byte g, byte b);
extern Color get_ledLib(int pixel);


class Orb {

public:
	Orb();
	void init(float _size, float _spd, float _hue, int _xBound, int _yBound);
	void move(float dt);
	void render();


private:
	float xPos, yPos, size, xSpd, ySpd;
	Color c;
	float hue;

	int xBound, yBound;


};

#endif

