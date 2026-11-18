//---- Network Search ----

#ifndef NetworkSearch_h
#define NetworkSearch_h

#include "Arduino.h"
#include "Color.h"
#include "Segment.h"
#include "Beam.h"
#include "BeamControl.h"
#include "Neural.h"

class NetworkSearch{
	public:
		NetworkSearch();
		void BFS(Segment **coreSegments, uint16_t coreSegAm);
		void animPath(Segment *startSeg, Color effectColor);
	private:
		
};



#endif