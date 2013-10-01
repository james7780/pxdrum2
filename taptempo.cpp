// PXDrum Portable Drum Machine
/*
 *      taptempo.cpp
 *      Manages tempo changes via tap tempo
 *
 *      Copyright 2009-2013 James Higgs <james@okami>
 *      
 */
#include "SDL.h"
#include "taptempo.h"

TapTempo::TapTempo()
{
	for (int i = 0; i < TAP_QUEUE_LEN; i++)
		{
		tapTickQueue[i] = 0;
		}
}

TapTempo::~TapTempo()
{
}

/// Add tap to tap tempo queue
///@return		Calculated tempo (if we have enough recent taps in the queue)
int TapTempo::AddTap()
{
	// Reset queue if previous tap too old (> 3 secs), else shift queue
	unsigned int tapTick = SDL_GetTicks();
	if (tapTick - tapTickQueue[TAP_QUEUE_LEN - 1] > TAP_TIMEOUT)
		{
		for (int i = 0; i < TAP_QUEUE_LEN - 1; i++)
			{
			tapTickQueue[i] = 0;
			}
		}
	else
		{
		for (int i = 0; i < TAP_QUEUE_LEN - 1; i++)
			{
			tapTickQueue[i] = tapTickQueue[i+1];
			}
		}

	// And queue up our latest tap
	tapTickQueue[TAP_QUEUE_LEN - 1] = tapTick;

	// Now calculate tempo if possible
	int bpm = 0;
	int accumPeriods = 0;
	for (int i = 0; i < TAP_QUEUE_LEN - 1; i++)
		{
		if (0 == tapTickQueue[i])
			{
			// not enough taps queued
			accumPeriods = 0;
			break;
			}
		accumPeriods += (tapTickQueue[i+1] - tapTickQueue[i]);
		}

	if (accumPeriods > 0)
		{
		float avgInterval = (float)accumPeriods / (TAP_QUEUE_LEN - 1);
		float bps = 1000.0f / avgInterval;
		bpm = (int)(bps * 60.0f);
		}

	return bpm;
}
