// PXDrum Portable Drum Machine
/*
 *      taptempo.cpp
 *      Manages tempo changes via tap tempo
 *
 *      Copyright 2009-2013 James Higgs <james@okami>
 *      
 */
#pragma once

#define TAP_QUEUE_LEN	3				// Number of taps req. before tempo calculated
#define TAP_TIMEOUT		3000			// Tap tempo times out after 3 seconds of no taps

class TapTempo
{
public:
	TapTempo();
	~TapTempo();

	// Notify that tap has occurred, get calculated tempo back
	int AddTap();

private:
	unsigned int tapTickQueue[TAP_QUEUE_LEN];
};

