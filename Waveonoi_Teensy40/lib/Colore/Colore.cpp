/*
Colore.cpp - Library for saving colors
Created by Emiel Harmsen 14-1-2015.
*/

#include "Arduino.h"
#include "Colore.h"



Colore::Colore(uint16_t leds, Segment *segments, uint16_t segLen, Beam *beamArray, uint16_t beamAm, void (*_setPixel)(int pixel, byte, byte, byte), Color (*_getPixel)(int), void (*_showPixels)(), void (*_resetPixels)()){
	dt = 0.05;
	spdFac = 1;
	totLedAm = leds;
	segArray = segments;
	segArray_len = segLen;
	
	setPixel = _setPixel;
	getPixel = _getPixel;
	showPixels = _showPixels;
	resetPixels = _resetPixels;
	
	neuralMode = false;
	
	beamControl.begin(beamArray,beamAm);
	for(int i=0; i<segArray_len; i++){
		segArray[i].setBeamControl( &beamControl );
	}
}

void Colore::beginNN(int bDec, float nCharge, int _DCSpd, int _DCPower, float _DCSpread, boolean _DCSpdMode, float fadeInSpd, float fadeOutSpd, boolean directSynapse){
	neural.begin(bDec, nCharge, _DCSpd, _DCPower, _DCSpread, _DCSpdMode, fadeInSpd, fadeOutSpd, directSynapse, &beamControl);
	neuralMode = true;
}

void Colore::update(){
	update(true,true);
}

void Colore::update(boolean clearPixels){
	update(clearPixels,true);
}

void Colore::update(boolean clearPixels, boolean sendPixels){
	calcDt();
	
	if(clearPixels) resetPixels();

	for(int i=0; i<segArray_len; i++){
		segArray[i].move(dt*spdFac);
		segArray[i].draw(setPixel,getPixel);
	}
	
	if(neuralMode) neural.update();

	if(sendPixels) showPixels();
}

void Colore::calcDt(){
	dt = (micros() - lastCalc) / 1000000.;  // assume one frame per second
	lastCalc = micros();
}

float Colore::getDt(){
	return dt;
}

float Colore::getFPS(){
	return 1/dt;
}

void Colore::setSpdFac(float _spdFac){
	spdFac = _spdFac;
}

uint16_t Colore::getActiveBeamsAm(){
	return beamControl.activeBeams;
}


boolean Colore::addNNBeam(Segment *seg, float spd, byte spdMode, float len, Color col, int power){
	if (neuralMode)	return neural.startNeuronBeam(seg, NULL, spd, spdMode, len, col, power, true);
	return false;
}

boolean Colore::lightUp(Segment *seg, float spd, Color col){
	Beam* newBeam = beamControl.freeBeam();
	if(newBeam == NULL) return false;
	newBeam->begin(seg, false, spd, SEGMENT_SPD, 0, col, FLASH);
	return true;
}