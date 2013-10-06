// texture map coords for xdrum gfx
/*
 *      texmap.h
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

enum TEXMAP {
			TM_NONOTEBEAT = 0,
			TM_NONOTE = 1,
			TM_NOTEON = 2,
			TM_ACCENT = 3,
			TM_NOTECUT = 4,
			TM_MINUS_BTN = 5,
			TM_PLUS_BTN = 6,
			TM_SLIDER_BG = 7,
			TM_SLIDER_FG = 8,
			TM_TAP_BTN = 9,
			TM_LIGHT_ON = 10,
			TM_LIGHT_OFF = 11,
			TM_LIGHT_SOLO = 12,
			TM_ADDTOSONGBTN = 13,
			TM_VOLSLIDER = 14,
			TM_BPMSLIDER = 15,
			TM_TRACKVOLSLIDER = 16,
			TM_MODE_BTN = 17,
			TM_PLAY_BTN = 18,
			TM_PAUSE_BTN = 19,
			TM_REWIND_BTN = 20,
			TM_HELP_BTN = 21,
			TM_SONGITEM = 22,
			TM_SONGITEM_HILITE = 23,
			TM_PATTERNITEM = 24,
			TM_PATTERNITEM_HILITE = 25,
			TM_OK_BTN = 26,
			TM_CANCEL_BTN = 27,
			TM_COUNT = 28
};

extern SDL_Rect texmap[TM_COUNT];
