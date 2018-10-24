// 
// 
// 

#include "Orb.h"

Orb::Orb()
{
	xSpd = 0;
	ySpd = 0;
}

void Orb::init(float _size, float _spd, float _hue, int _xBound, int _yBound)
{
	xBound = _xBound;
	yBound = _yBound;

	xPos = random(0, xBound);
	yPos = random(0, yBound);
	xSpd = random(-_spd, _spd);
	ySpd = random(-_spd, _spd);;
	size = _size;
	hue = _hue;

	
}

void Orb::move(float dt)
{
	hue += 5 * dt;
	if (hue >= 360) hue -= 360;
	c.setHSB(hue, 100,	100);


	xPos += xSpd * dt;
	yPos += ySpd * dt;

	if (xPos > xBound) {
		xSpd -= 50* dt;
	}
	else if (xPos < 0) {
		xSpd += 50* dt;
	}

	if (yPos > yBound) {
		ySpd -= 50* dt;
	}
	else if (yPos < 0) {
		ySpd += 50* dt;
	}
}

void Orb::render()
{
	int x1 = xPos - size / 2;
	int x2 = xPos + size / 2;
	int y1 = yPos - size / 2;
	int y2 = yPos + size / 2;

	for (int i = 0; i < ledAm; i++) {
		if (leds[i].xPos >= x1 && leds[i].xPos <= x2 && leds[i].yPos >= y1 && leds[i].yPos <= y2) {
			float Dx = leds[i].xPos - xPos;
			float Dy = leds[i].yPos - yPos;
			float dist = sqrt(Dx * Dx + Dy * Dy);

			float bri = (size / 2) - dist;
			bri = bri / (size / 2);
			if (bri < 0) bri = 0;

			int ledID = leds[i].channel*ledsPerStrip + leds[i].ledID;
			Color pixCol = get_ledLib(ledID);
			pixCol.add(c,bri);

			set_ledLib(ledID, pixCol.red(), pixCol.green(), pixCol.blue());
		}
	}
}
