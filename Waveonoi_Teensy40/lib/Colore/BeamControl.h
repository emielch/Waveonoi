//--- Beam controller
#ifndef BeamControl_h
#define BeamControl_h

#include "Arduino.h"
#include "Beam.h"

class Beam; // forward declaration

class BeamControl{
	public:
		BeamControl();
		void begin(Beam *_beamArray, uint16_t beamAm);
		Beam* freeBeam();
		
		Beam *beamArray;
		uint16_t beamArray_len;
		uint16_t activeBeams;
		
	private:
		
};

#endif