// xdrum screen zones (for gui input)
/*
 *      zones.h
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

enum ZONES { ZONE_VOL = 1,
			ZONE_BPM = 2,
			ZONE_SONGLIST = 3,
			ZONE_PATLIST = 4,
			ZONE_ADDTOSONGBTN = 5,
			ZONE_TRACKINFO = 6,
			ZONE_PATGRID = 7,
			ZONE_SONGNAME = 8,
			ZONE_KITNAME = 9,
			ZONE_OPTIONS = 10,
			ZONE_TRANSPORT = 11,
			ZONE_PATNAME = 12,
			ZONE_MAX = 13
		};

extern SDL_Rect zones[ZONE_MAX];

// pattern grid box size
extern SDL_Rect PATBOX;
