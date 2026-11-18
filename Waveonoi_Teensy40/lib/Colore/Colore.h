/*
Colore.h - Library for making LED animations
Created by Emiel Harmsen 14-1-2015.
*/

#ifndef Colore_h
#define Colore_h

#include "Arduino.h"
#include "Color.h"
#include "Segment.h"
#include "Beam.h"
#include "BeamControl.h"
#include "Neural.h"
#include "NetworkSearch.h"



// --- main class, controlling library etc. ---

class Colore{
	public:
		Colore(uint16_t leds, Segment *segments, uint16_t segLen, Beam *beamArray, uint16_t beamAm, void (*_setPixel)(int pixel, byte, byte, byte), Color (*_getPixel)(int), void (*_showPixels)(), void (*_resetPixels)());
		void beginNN(int bDec, float nCharge, int _DCSpd, int _DCPower, float _DCSpread, boolean _DCSpdMode, float fadeInSpd, float fadeOutSpd, boolean directSynapse);
		void update();
		void update(boolean clearPixels);
		void update(boolean clearPixels, boolean sendPixels);
		
		boolean addNNBeam(Segment *seg, float spd, byte spdMode, float len, Color col, int power);
		boolean lightUp(Segment *seg, float spd, Color col);
		float getDt();
		float getFPS();
		uint16_t getActiveBeamsAm();
		void setSpdFac(float _spdFac);
		Neural neural;
	
	private:
		uint16_t totLedAm;
		Segment *segArray;
		uint16_t segArray_len;
		boolean neuralMode;
		
		unsigned long lastCalc; // variable to keep track of the loops per second
		float dt;
		float spdFac;
		
		void (*setPixel)(int pixel, byte r, byte g, byte b);
		void (*showPixels)();
		void (*resetPixels)();
		Color (*getPixel)(int);
		void calcDt();
		
		BeamControl beamControl;
};


#endif