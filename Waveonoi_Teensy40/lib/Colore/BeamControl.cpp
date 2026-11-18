#include "Arduino.h"
#include "BeamControl.h"


BeamControl::BeamControl(){
}

void BeamControl::begin(Beam *_beamArray, uint16_t beamAm){
	beamArray = _beamArray;
	beamArray_len = beamAm;
	activeBeams = 0;
}

Beam* BeamControl::freeBeam(){
	for(int i=0; i<beamArray_len; i++){
		if( !beamArray[i].isActive() ){
			return &beamArray[i];
		}
	}
	return NULL;
}