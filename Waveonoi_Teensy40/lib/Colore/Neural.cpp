#include "Arduino.h"
#include "Neural.h"

Neural::Neural(){
	beamDecay = 50;
	neuronChargeFac = 0.05;
	DCSpd = 100;
	DCPower = 100;
	DCSpread = 10;
	DCSpdMode = PIXEL_SPD;
	fadeInSpd = 8;
	fadeOutSpd = 0.3;
	allowDirectSynapse = false;
}


void Neural::begin(int bDec, float nCharge, int _DCSpd, int _DCPower, float _DCSpread, boolean _DCSpdMode, float inSpd, float outSpd, boolean directSynapse, BeamControl *_beamControl){
	beamDecay = bDec;
	neuronChargeFac = nCharge;
	DCSpd = _DCSpd;
	DCPower = _DCPower;
	DCSpread = _DCSpread;
	DCSpdMode = _DCSpdMode;
	fadeInSpd = inSpd;
	fadeOutSpd = outSpd;
	allowDirectSynapse = directSynapse;
	beamControl = _beamControl;
}

void Neural::setSendFireFunction(void (*_sendNeuronFire)(Segment *)){
	sendNeuronFire = _sendNeuronFire;
}

void Neural::setSendDischargeFunction(void (*_sendNeuronDischarge)(Segment *)){
	sendNeuronDischarge = _sendNeuronDischarge;
}



void Neural::update(){
	for(int i=0; i<beamControl->beamArray_len; i++){
		if( beamControl->beamArray[i].isActive() ){
			if ( beamControl->beamArray[i].isNeuralMode() ){
				if( beamControl->beamArray[i].justArrived() ){
					arriveBeam( &beamControl->beamArray[i] );
				}
			}
		}
	}
}


boolean Neural::startNeuronBeam(Segment *neuron, Segment *skipSeg, float _spd, byte _spdMode, float _len, Color _col, int _power, boolean animNeur){
	
	if(sendNeuronFire != NULL){
		sendNeuronFire(neuron);
	}
	
	// stop if the segment not a NEURON
	if( neuron->nnType!=NEURON){
		Serial.println(F("Segment is not a NEURON"));
		return false;
	} 
	
	// get the color and change the brightness according to power
	_col.setHSB(_col.hue(), _col.saturation(), constrain(_power,0,100));
	
	// animate the neuron
	if(animNeur){
		Color neurCol = neuron->getCurrentColor();
		neurCol.addHDR( _col, 1 );
		Color neurBGColor(0,0,neuron->power,HSB_MODE);
		neuron->setFadeInOut(neurCol,neurBGColor,fadeInSpd,fadeOutSpd);
	}
	
	
	
	//--- choose the next segment
	
	byte nextSegId;
	byte chooseAm = neuron->connAm;  // amount of connections that can be chosen from
	
	// stop if neuron has no connections
	if( chooseAm == 0 ){
		Serial.println(F("Neuron has no connections, arrived at dead end"));
		return false;
	}
	
	// if the neuron has only one connection choose that one
	else if( chooseAm == 1 ) nextSegId = 0;
	
	// if multiple connections; choose one randomly
	else{
		
		// determine the place of the segment that has to be skipped in the connections array
		int skipId = -1;
		for (int i=0; i<chooseAm; i++){
			if( neuron->connSeg[i] == skipSeg ){
				skipId = i;
				chooseAm--;
				break;
			}
		}
		
		// choose one of the connected segments
		nextSegId = random(0,chooseAm);
		if(skipId != -1 && nextSegId >= skipId) nextSegId++;  // correction for the skipped segment
	}
	
	Segment *nextSynapse = neuron->connSeg[nextSegId];
	
	
	
	// if the next segment is a NEURON; start another startNeuronBeam() skipping the current neuron and return
	if(nextSynapse->nnType == NEURON){
		return startNeuronBeam( nextSynapse, neuron, _spd, _spdMode, _len, _col, _power, true);
	}
	// continue if the next segment is a SYNAPSE

	// determine the correct direction for the beam
	boolean dir = UP;
	if( nextSynapse->connSeg[1] == neuron ) dir = DOWN; // if the neuron at the end of the synapse is the neuron the beam came from; the direction is DOWN. Else its UP
		
	return startSynapseBeam(nextSynapse, dir, _spd, _spdMode, _len, _col, _power);
}


boolean Neural::startSynapseBeam(Segment *synapse, boolean dir, float _spd, byte _spdMode, float _len, Color _col, int _power ){
	if( synapse->nnType != SYNAPSE ){
		Serial.println(F("Segment is not a SYNAPSE"));
		return false;
	}
	
	// send the pulse into the neural network
	return synapse->setNNBeam(dir, _spd, _spdMode, _len, _col, _power);
}


void Neural::arriveBeam(Beam *beam){
	beam->arrive();
	
	// stop if arrived at dead end
	if( beam->onSegment->connAm == 0 || beam->onSegment->connAm == 1 ){
		Serial.println(F("Arrived at a dead end"));
		return;
	}
	
	// determine the segment it arrived at
	byte connId = ((beam->dir == UP) ? 1 : 0);
	Segment *nextSegment = beam->onSegment->connSeg[connId];
	
	//check the type of the next segment
	if( nextSegment->nnType == SYNAPSE && !allowDirectSynapse){
		Serial.println(F("The beam has arrived at not a NEURON"));
		return;
	}
	
	// increase the power of the segment it arrived at, accoring to the power of the beam
	// and discharge the neuron if neccesary
	int newPower = beam->power;
	boolean animNeuron = true;
	if(nextSegment->nnType == NEURON){
		nextSegment->power += beam->power * neuronChargeFac;
		if(nextSegment->power >= 100){
			neuronDischarge(nextSegment, beam->color.hue());
			animNeuron = false; // dont animate the neuron when the beam passes, so it stays black for dramatic discharge effect
		}
	
		// calc the new power of the beam, stop if no power left
		newPower -= beamDecay;
		if (newPower <= 0){
			return;
		}
	}
	
	// get the speed type and the according speed
	float spd = beam->segSpd;
	byte spdMode = beam->spdMode;
	if( spdMode == PIXEL_SPD ){
		spd = beam->pixelSpd;
	}

	// create the next beam
	if (nextSegment->nnType == SYNAPSE){
		boolean dir = DOWN;
		if( nextSegment->connSeg[0] == beam->onSegment) dir = UP;
		startSynapseBeam(nextSegment, dir, spd, spdMode, beam->spread, beam->color, newPower);
	}else{
		startNeuronBeam(nextSegment, beam->onSegment, spd, spdMode, beam->spread, beam->color, newPower, animNeuron);
	}
	
}


void Neural::neuronDischarge(Segment *neuron, int hue){
	if(sendNeuronDischarge != NULL){
		sendNeuronDischarge(neuron);
	}
	
	// animate the neuron
	Color white(255,255,255,RGB_MODE);
	Color black(0,0,0,RGB_MODE);
	neuron->setFadeInOut(white,black,8,3);
	neuron->power = 0;
	
	// set the color for the beams
	Color col(hue,100,100,HSB_MODE);
	
	// fire beams in all directions
	for (int i=0; i<neuron->connAm; i++){
		Segment *nextSynapse = neuron->connSeg[i];
		
		// determine the correct direction for the beam
		boolean dir = DOWN;
		if( nextSynapse->connSeg[0] == neuron ) dir = UP;
		
		// fire the beam
		startSynapseBeam(nextSynapse, dir, DCSpd, DCSpdMode, DCSpread, col, DCPower);
	}
}