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
#include "GridCell.h"

extern LED leds[];

extern int ledAm;
extern const int ledsPerStrip;
extern void set_ledLib(int pixel, byte r, byte g, byte b);
extern Color get_ledLib(int pixel);
extern GridCell grid[];
extern const int gridcellAm;


class Orb {

public:
	Orb();
	void init(float _size, float _spd, float _hue, int _xBound, int _yBound);
	void move(float dt);
	void render();
	void setBri(float _bri) { bri = _bri; }
	float xPos, yPos, size, xSpd, ySpd;

private:
	
	Color c;
	float hue, bri;

	int xBound, yBound;


};

#endif

