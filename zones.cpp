// xdrum screen zones (for gui input)
/*
 *      zones.h
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

 #include "SDL.h"
 #include "zones.h"

#ifdef PSP
SDL_Rect zones[ZONE_MAX] = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 32, 80 },			// vol
	{ 32, 0, 32, 80 },			// bpm
	{ 64, 0, 32, 80 },			// pitch
	{ 384, 0, 96, 80 },			// song list
	{ 272, 0, 96, 80 },			// pattern list
	{ 370, 32, 12, 16 },		// add-to-song button
	{ 0, 80, 96, 192 },			// track info
	{ 96, 80, 384, 192 },		// pattern grid
	{ 98, 0, 172, 16 },			// song name
	{ 98, 16, 172, 16 },		// drumkit name
	{ 98, 32, 172, 16 },		// options display (shuffle / volrand)
	{ 98, 48, 172, 16 },		// mode 
	{ 98, 64, 172, 16 },		// pattern name
};
	
// pattern grid box size
SDL_Rect PATBOX = { 0, 0, 24, 24 };
#else
SDL_Rect zones[ZONE_MAX] = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 188, 48 },			// vol
	{ 0, 52, 188, 48 },			// bpm
	{ 480, 0, 480, 48 },		// song list
	{ 480, 56, 480, 96 },		// pattern list
	{ 740, 64, 24, 32 },		// add-to-song button
	{ 0, 160, 188, 384 },		// track info
	{ 192, 160, 768, 384 },		// pattern grid
	{ 192, 52, 280, 48 },		// song name
	{ 0, 112, 188, 48 },		// drumkit name
	{ 388, 0, 92, 48 },			// options display (shuffle / volrand)
	{ 192, 0, 192, 48 },		// transport 
	{ 192, 112, 280, 48 },		// pattern name
};

// pattern grid box size
SDL_Rect PATBOX = { 0, 0, 48, 48 };
#endif

