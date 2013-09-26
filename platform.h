// platform defines
/*
 *      platform.h
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

// Output width and height (may be scaled to screen resolution by hardware)
// Doubled up to PS Vita resolution
#ifdef PSP
#define VIEW_WIDTH 	480
#define VIEW_HEIGHT 272
#else
#define VIEW_WIDTH 	960
#define VIEW_HEIGHT 544
#endif

#ifdef PSP
#define JOYRANGE	65536
#define JOYMID		0
#else
#define JOYRANGE	65536			// PS3 controller! (SDL should always have -32767 to +32767 range)
#define JOYMID		0
#endif

#ifdef PSP
	#include <stdio.h>
	#include <string.h>
	#include <pspdebug.h>
	#define printf pspDebugScreenPrintf
	#define main SDL_main
#else
	#include <stdio.h>
#endif

#ifdef _WIN32
	#include <cstdlib>
	#include <string.h>
#endif

/// Utility func - setup and SDL_Rect
extern void SetSDLRect(SDL_Rect& rect, int x, int y,int w, int h);



