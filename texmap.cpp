// texture map coords for xdrum gfx
/*
 *      texmap.h
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

 #include "SDL.h"
 #include "texmap.h"

SDL_Rect texmap[TM_COUNT] = {
	{ 256, 256, 48, 48 },		// no note (on the beat)
	{ 304, 256, 48, 48 },		// no note
	{ 352, 256, 48, 48 },		// note on
	{ 400, 256, 48, 48 },		// accent
	{ 448, 256, 48, 48 },		// note cut
	{ 0, 0, 48, 48 },			// TM_MINUS_BTN
	{ 48, 0, 48, 48 },			// TM_PLUS_BTN
	{ 96, 0, 48, 48 },			// TM_SLIDER_BG
	{ 144, 0, 48, 48 },			// TM_SLIDER_FG
	{ 192, 0, 48, 48 },			// TM_TAP_BTN
	{ 0, 48, 48, 48 },			// TM_LIGHT_ON
	{ 48, 48, 48, 48 },			// TM_LIGHT_OFF
	{ 96, 48, 48, 48 },			// TM_LIGHT_SOLO
	{ 0, 96, 12, 16 },			// add to song button
	{ 128, 0, 16, 56},			// vol slider graphic
	{ 160, 0, 16, 56},			// BPM slider graphic
	{ 72, 104, 11, 21},			// track mix vol slider graphic
	{ 0, 96, 64, 48},			// TM_MODE_BTN
	{ 64, 96, 64, 48},			// TM_PLAY_BTN
	{ 192, 96, 64, 48},			// TM_PAUSE_BTN
	{ 128, 96, 64, 48},			// TM_REWIND_BTN
	{ 0, 0, 48, 48},			// TM_HELP_BTN			(TODO)
	{ 240, 0, 48, 48},			// TM_SONGITEM
	{ 240, 48, 48, 48},			// TM_SONGITEM_HILITE
	{ 288, 0, 48, 48},			// TM_PATTERNITEM
	{ 288, 48, 48, 48},			// TM_PATTERNITEM_HILITE
	{ 0, 144, 48, 48},			// TM_OK_BTN
	{ 48, 144, 48, 48}			// TM_CANCEL_BTN
};

/* old (256x256 texmap)
SDL_Rect texmap[TM_MAX] = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 32, 80 },			// vol box
	{ 32, 0, 32, 80 },			// bpm box
	{ 64, 0, 32, 80 },			// pitch box
	{ 0, 160, 96, 80 },			// songlist
	{ 0, 160, 96, 80 },			// patlist
	{ 0, 96, 12, 16 },		// add to song button
	{ 0, 128, 96, 24 },			// trackinfo box
	{ 128, 128, 24, 24 },		// no note (on the beat)
	{ 152, 128, 24, 24 },		// no note
	{ 176, 128, 24, 24 },		// note on
	{ 200, 128, 24, 24 },		// accent
	{ 224, 128, 24, 24 },		// note cut
	{ 128, 0, 16, 56},			// vol slider graphic
	{ 160, 0, 16, 56},			// BPM slider graphic
	{ 72, 104, 11, 21}			// track mix vol slider graphic
};
*/
