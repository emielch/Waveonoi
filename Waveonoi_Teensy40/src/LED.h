// LED.h - one entry of the physical LED layout (see ledLayout.h)

#ifndef _LED_h
#define _LED_h

#include <stdint.h>

struct LED {
	uint8_t channel;  // OctoWS2811 output (0..7)
	uint8_t ledID;    // index along that strip
	int16_t xPos;     // layout x: wall height, 0 = bottom edge
	int16_t yPos;     // layout y: wall width, 0 = left edge
};

#endif
