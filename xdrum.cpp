// PXDrum Portable Drum Machine
/*
 *      xdrum.cpp
 *      
 *      Copyright 2009-2013 James Higgs <james@okami>
 *      
 */
#include <stdlib.h>
#include "SDL.h"
#include "SDL_mixer.h"
#include "SDL_thread.h"
#include "platform.h"
#include "pattern.h"
#include "drumkit.h"
#include "song.h"
#include "zones.h"
#include "texmap.h"
#include "fontengine.h"
#include "renderer.h"
#include "gui.h"
#include "transport.h"
#include "joymap.h"
#include "writewav.h"

#define XDRUM_VER	"2.0"

// xdrum "screen modes"
enum XDRUM_MODE { XM_MAIN = 0, XM_FILE = 1, XM_KIT = 2 };

// SDL buffers/surfaces
SDL_Renderer *sdlRenderer = NULL;
SDL_Window *sdlWindow = NULL;
Renderer *renderer = NULL;

// The current drumkit and song objects
DrumKit drumKit;
Song song;

// Pointer to the pattern that is currently playing
DrumPattern* currentPattern = NULL;

int livePatternIndex = 0;

// For copy/paste ops
DrumPattern patternClipboard;
DrumEvent trackClipboard[STEPS_PER_PATTERN];

int currentTrack = 0;						// pattern cursor y
int currentStep = 0;						// pattern cursor x

// Mouse cursor (note: float position needed for smooth movement)
float cursorX = (VIEW_WIDTH / 2);
float cursorY = (VIEW_HEIGHT / 2);
// Do we want to lock mouse to pattern grid cursor (PSP)?
#define LOCK_MOUSE_TO_GRID_CURSOR

// PLayback control
Transport transport;

// Joystick / controller button mapping
JoyMap joyMap;

// WAV output
short outputSample;
WavWriter wavWriter;

// quit/exit flag
bool quit = false;

/// Utility func - setup and SDL_Rect
void SetSDLRect(SDL_Rect& rect, int x, int y,int w, int h)
{
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
}

/// Output SDL joystick info
void printJoystickInfo(SDL_Joystick *joystick)
{
	int index;

	index = SDL_JoystickInstanceID(joystick);

	printf( "JOYSTICK INFO\n\n"
		"Index: %d\n"
		"Name: %s\n"
		"Num axes: %d\n"
		"Num balls: %d\n"
		"Num hats: %d\n"
		"Num buttons: %d\n",
		index,
		SDL_JoystickName(joystick),
		SDL_JoystickNumAxes(joystick),
		SDL_JoystickNumBalls(joystick),
		SDL_JoystickNumHats(joystick),
		SDL_JoystickNumButtons(joystick)
	);
}



/// Progress callback function
void progress_callback(int progress, const char *message)
{
	ShowProgress(renderer, message, progress);
}


/// Set main output volume
/// @param volPercent	Volume as percent of max (0 to 100)
void SetMainVolume(int volPercent)
{
	// clamp value to valid range
	if (volPercent < 0)
		volPercent = 0;
	else if (volPercent > 100)
		volPercent = 100;
		
	// set song vol as 0 to 255 range
	song.vol = (volPercent * 255) / 100;
}

/// Get main output volume
/// @return 	Volume as percent of max (0 to 100)
int GetMainVolume()
{
	return ((song.vol * 100) / 255);
}

/// Syncronise pattern pointer with pattern index
void SyncPatternPointer()
{
	if (NO_PATTERN_INDEX == song.currentPatternIndex)
		currentPattern = NULL;
	else
		currentPattern = &song.patterns[song.currentPatternIndex];
}


/// Get the index of the solo'ed track
/// @return		Index number of track that is solo, else -1
int GetSoloTrack()
{
	for (int i = 0; i < NUM_TRACKS; i++)
		{
		if (TrackMixInfo::TS_SOLO == song.trackMixInfo[i].state)
			return i;
		}
		
	return -1;
}

/// mute/unmute a track if possible (ie: if no solo active)
void SetTrackMute(int track)
{
	printf("SetTrackMute(%d)\n", track);
	if (-1 == GetSoloTrack())
		{
		// Set / reset MUTE state
		if (TrackMixInfo::TS_MUTE == song.trackMixInfo[track].state)
			song.trackMixInfo[track].state = TrackMixInfo::TS_ON;
		else
			song.trackMixInfo[track].state = TrackMixInfo::TS_MUTE;
		}
}

/// solo / unsolo a track
/// @param track		Track index to solo (or -1 to remove solo)
/// Set solo state to the specified track
/// (-1 to switch off solo)
void SetTrackSolo(int track)
{
	// If track is currenty solo'ed, then we want to switch off solo
	if (track >= 0 && track < NUM_TRACKS)
		{
		if (TrackMixInfo::TS_SOLO == song.trackMixInfo[track].state)
			track = -1;				// so that we do not solo this track below
		}
		
	// Remove any current solo (restore previous track state)
	for (int i = 0; i < NUM_TRACKS; i++)
		song.trackMixInfo[i].state = song.trackMixInfo[i].prevState;



	// Apply new solo if neccessary
	if (track >= 0 && track < NUM_TRACKS)
		{
		// Backup track mix state, and mute / solo tracks
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			song.trackMixInfo[i].prevState = song.trackMixInfo[i].state;
			song.trackMixInfo[i].state = TrackMixInfo::TS_MUTE;
			}
		song.trackMixInfo[track].state = TrackMixInfo::TS_SOLO;
		}
		
}

// draw vol/bpm/pitch sliders
static void DrawSliders(unsigned char vol, unsigned char bpm)
{
	if (!renderer)
		return;

	char s[32];
	// draw vol slider control
	SDL_Rect dest = zones[ZONE_VOL];
	dest.w = PATBOX.w;
	//SDL_Rect src = texmap[TM_MINUS_BTN];
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_MINUS_BTN, dest);

	dest = zones[ZONE_VOL];
	dest.x = PATBOX.w;
	dest.w = dest.w - PATBOX.w - PATBOX.w;
	//src = texmap[TM_SLIDER_BG];
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_SLIDER_BG, dest);

	// draw part of vol slider graphic corresponding to the vol level (0 to 255)
	//src = texmap[TM_SLIDER_FG];
	int w = ((vol * dest.w) / 255);
	dest.w = w;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_SLIDER_FG, dest);

	sprintf(s, "%d", (vol * 100) / 255);
	dest = zones[ZONE_VOL];
	dest.x = PATBOX.w;
	dest.y += 2;
	//m_smallFont->DrawText(m_sdlRenderer, s, dest, true);
	renderer->DrawText(false, s, dest, true);

	//src = texmap[TM_PLUS_BTN];
	dest = zones[ZONE_VOL];
	dest.x = dest.w - PATBOX.w;
	dest.w = PATBOX.w;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_PLUS_BTN, dest);

	//draw BPM slider box
	//src = texmap[TM_MINUS_BTN];
	dest = zones[ZONE_BPM];
	dest.w = PATBOX.w;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_MINUS_BTN, dest);

	//src = texmap[TM_SLIDER_BG];
	dest = zones[ZONE_BPM];
	dest.x = PATBOX.w;
	dest.w = dest.w - PATBOX.w - PATBOX.w;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_SLIDER_BG, dest);

	sprintf(s, "%d", bpm);
	dest.x;
	dest.y += 8;
	//m_bigFont->DrawText(m_sdlRenderer, s, dest, true);
	renderer->DrawText(true, s, dest, true);

	//src = texmap[TM_TAP_BTN];
	dest = zones[ZONE_BPM];
	dest.x += dest.w - PATBOX.w - PATBOX.w;
	dest.w = PATBOX.w;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_TAP_BTN, dest);

	//src = texmap[TM_PLUS_BTN];
	dest = zones[ZONE_BPM];
	dest.x = dest.w - PATBOX.w;
	dest.w = PATBOX.w;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_PLUS_BTN, dest);

/*	
	// draw pitch slider
	dest = zones[ZONE_PITCH];
	src = texmap[TM_PITCH_BOX];
	//SDL_BlitSurface(guiTex, &src, surface, &dest);
	SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
*/
/*
	SDL_Rect r1;
	r1.x = 74;
	r1.w = 12;
	if (song.pitch > 0)
		{
		r1.h = song.pitch * 2;
		r1.y = 40 - r1.h;
		}
	else
		{
		r1.h = abs(song.pitch * 2);
		r1.y = 40;
		}
	SDL_FillRect(surface, &r1, SDL_MapRGB(screen->format, 255, 128, 0));
	if (song.pitch > 0)
		sprintf(s, "+%d", song.pitch);
	else
		sprintf(s, "%d", song.pitch);
	dest.x += (16 - (int)strlen(s) * 8 / 2);
	dest.y += 2;
	m_smallFont->DrawText(surface, s, dest, true);
*/	
}

// draw track info
static void DrawTrackInfo(Song &song, DrumKit &drumkit)
{
	if (!renderer)
		return;

	char trackName[DRUM_NAME_LEN];

	const int x0 = zones[ZONE_TRACKINFO].x;
	const int y0 = zones[ZONE_TRACKINFO].y;
	const int w0 = zones[ZONE_TRACKINFO].w;
	const int h0 = PATBOX.h;	
	//SDL_Rect src;
	SDL_Rect dest;
	for (int i = 0; i < NUM_TRACKS; i++)
		{
		//src = texmap[TM_SLIDER_BG];
		SetSDLRect(dest, x0, y0 + i*h0, w0, h0);
		//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
		renderer->DrawButton(TM_SLIDER_BG, dest);
		//src = texmap[TM_SLIDER_FG];
		int w = (song.trackMixInfo[i].vol * (w0 - PATBOX.w)) / 255;
		SetSDLRect(dest, x0, y0 + i*h0, w, h0);
		//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
		renderer->DrawButton(TM_SLIDER_FG, dest);
		
		// Draw track name (instr name)
		dest.x += 2;
		dest.y += 2;
		dest.w = w0 - PATBOX.w;
		if (0 == drumkit.drums[i].name[0])
			//sprintf(trackName, "Track %d", i+1);
			strcpy(trackName, "---");
		else
			strcpy(trackName, drumkit.drums[i].name);
		//m_bigFont->DrawText(m_sdlRenderer, trackName, dest, true);
		renderer->DrawText(true, trackName, dest, true);

/*
		// draw part of track mix vol graphic corresponding to the track mix level 
		src = texmap[TM_TRACKVOLSLIDER];
		int h = (song.trackMixInfo[i].vol * 20) / 255;
		src.y += (src.h - h);
		src.h = h + 1;
		dest.x = 72;
		dest.y = y0 + i*h0 + 22 - h;
		dest.w = src.w;
		dest.h = src.h;
		SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
*/		
		// Draw mute/solo state
		SetSDLRect(dest, w0 - PATBOX.w,  y0 + i*h0, PATBOX.w, PATBOX.h);
		if (TrackMixInfo::TS_MUTE == song.trackMixInfo[i].state)
			{
			//src = texmap[TM_LIGHT_OFF];
			//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
			renderer->DrawButton(TM_LIGHT_OFF, dest);
			}
		else if (TrackMixInfo::TS_SOLO == song.trackMixInfo[i].state)
			{
			//src = texmap[TM_LIGHT_SOLO];
			//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
			renderer->DrawButton(TM_LIGHT_SOLO, dest);
			}
		else
			{
			//src = texmap[TM_LIGHT_ON];
			//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
			renderer->DrawButton(TM_LIGHT_ON, dest);
			}

		}
}

// draw song sequence list
static void DrawSequenceList(Song &song)
{
	if (!renderer)
		return;


	char s[40];
	char* patname;
	char no_pat_name[32] = "---";
	SDL_Rect src = texmap[TM_SONGITEM];
	SDL_Rect dest = zones[ZONE_SONGLIST];
	dest.w = PATBOX.w;

	//dest.x += 2;
	//dest.y += 1;
	//m_smallFont->DrawText(m_sdlRenderer, "Sequence", dest, false);
	//dest.y += 10;
	
	// songlist must "follow' song playback
	if (song.songPos > song.songListScrollPos + 6)
		song.songListScrollPos = song.songPos - 6;
	else if (song.songPos < song.songListScrollPos)
		song.songListScrollPos = song.songPos;

	// draw visible part of songlist
	SDL_Rect textRect;
	for (int i = song.songListScrollPos; i < PATTERNS_PER_SONG; i++)
		{
		//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
		renderer->DrawButton(TM_SONGITEM, dest);

		// Draw song pattern index
		int patIndex = song.songList[i];
		if (NO_PATTERN_INDEX == patIndex)
			patname = no_pat_name;
		else
			patname = song.patterns[patIndex].name;
		if (i == song.songPos)
			sprintf(s, "*%2d", i+1);
		else
			sprintf(s, " %2d", i+1);

		textRect = dest;
		textRect.x += 4;
		textRect.y += 4;
		//m_smallFont->DrawText(m_sdlRenderer, s, textRect, false);
		renderer->DrawText(false, s, textRect, false);
		textRect.y += renderer->m_smallFont->GetFontHeight();
		//m_smallFont->DrawText(m_sdlRenderer, patname, textRect, false);
		renderer->DrawText(false, patname, textRect, false);
		//dest.y += 8;
		dest.x += PATBOX.w;
		// kick out if writing over right edge
		if (dest.x >= 960)
			i = PATTERNS_PER_SONG;
		}

}

// draw pattern list and "add pattern to song" button
static void DrawPatternList(Song &song)
{
	if (!renderer)
		return;


	char s[40];
	SDL_Rect src = texmap[TM_PATTERNITEM];
	SDL_Rect dest0 = zones[ZONE_PATLIST];

	for (int i = 0; i < 2; i++)
		{
		SDL_Rect dest = dest0;
		dest.w = PATBOX.w;
		dest.h = PATBOX.h;
		dest.y += i * PATBOX.h;
		for (int j = 0; j < 10; j++)
			{
			//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
			renderer->DrawButton(TM_PATTERNITEM, dest);
			dest.x += PATBOX.w;
			}
		}

	for (int i = 0; i < 2; i++)
		{
		SDL_Rect dest = dest0;
		dest.w = PATBOX.w;
		dest.h = PATBOX.h;
		dest.x += 4;
		dest.y += 4 + i * PATBOX.h;
		for (int j = 0; j < 10; j++)
			{
			// draw pattern number/name
			int patNum = i*10 + j;
			if (patNum == song.currentPatternIndex)
				sprintf(s, "#%2d", patNum + 1);
			else
				sprintf(s, " %2d", patNum + 1);
			//m_smallFont->DrawText(m_sdlRenderer, s, dest, false);
			renderer->DrawText(false, s, dest, false);
			dest.y += renderer->m_smallFont->GetFontHeight();
			//m_smallFont->DrawText(m_sdlRenderer, song.patterns[patNum].name, dest, false);
			renderer->DrawText(false, song.patterns[patNum].name, dest, false);
			dest.y -= renderer->m_smallFont->GetFontHeight();
			dest.x += PATBOX.w;
			}
		}


/*
	dest.x += 2;
	dest.y += 1;
	m_smallFont->DrawText(m_sdlRenderer, "Patterns", dest, false);
	dest.y += 10;

	// patlist must "follow' current pattern number
	if (NO_PATTERN_INDEX != song.currentPatternIndex)
		{
		if (song.currentPatternIndex > song.patListScrollPos + 6)
			song.patListScrollPos = song.currentPatternIndex - 6;
		else if (song.currentPatternIndex < song.patListScrollPos)
			song.patListScrollPos = song.currentPatternIndex;
		}

	for (int i = song.patListScrollPos; i < MAX_PATTERN; i++)
		{
		// draw pattern number/name
		if (i == song.currentPatternIndex)
			sprintf(s, "%2d#%s", i+1, song.patterns[i].name);
		else
			sprintf(s, "%2d %s", i+1, song.patterns[i].name);
		m_smallFont->DrawText(m_sdlRenderer, s, dest, false);
		dest.y += 8;
		// kick out if writing over bottom
		if (dest.y >= zones[ZONE_PATLIST].h - 10 - 8)
			i = MAX_PATTERN;
		}
	
	// draw "add pattern to song" button	
	src = texmap[TM_ADDTOSONGBTN];
	dest = zones[ZONE_ADDTOSONGBTN];
	//SDL_BlitSurface(guiTex, &src, surface, &dest);
	SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
*/
}


// draw drum pattern
static void DrawPatternGrid(DrumPattern* pattern)
{
	if (!renderer)
		return;


	if (!pattern)
		return;
		
	SDL_Rect src = texmap[TM_NONOTE];
	SDL_Rect src2 = texmap[TM_NONOTEBEAT];
	SDL_Rect dest;
	const int x0 = zones[ZONE_PATGRID].x;
	const int y0 = zones[ZONE_PATGRID].y;
	const int w = PATBOX.w;
	const int h = PATBOX.h;
	if (NULL == pattern)
		{
		// Draw empty grid
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{ 
				SetSDLRect(dest, x0 + j * w, y0 + i * h, w, h);
				renderer->DrawButton(TM_NONOTE, dest);
				}
			}
		}
	else
		{
		for (int i = 0; i < NUM_TRACKS; i++)
			{
			for (int j = 0; j < STEPS_PER_PATTERN; j++)
				{
				TEXMAP tileId = TM_NONOTE;
				unsigned char vol = pattern->events[i][j].vol;
				if (0 == vol)
					{
					if (0 == (j & 0x3))
						tileId = TM_NONOTEBEAT;
					else
						tileId = TM_NONOTE;
					}
				else if (1 == vol)		// mute drum (end note)
					tileId = TM_NOTECUT;
				else if (vol > 100)		// accent note
					tileId = TM_ACCENT;
				else					// trigger normal drum note
					tileId = TM_NOTEON;
				SetSDLRect(dest, x0 + j * w, y0 + i * h, w, h);
				renderer->DrawButton(tileId, dest);
				}
			}
		}

	// Draw pattern name
	dest = zones[ZONE_PATNAME];
	//SDL_SetRenderDrawColor(sdlRenderer, 0, 40, 0, 255);
	//SDL_RenderFillRect(sdlRenderer, &dest);
	SDL_Colour colour;
	colour.r = 0;
	colour.g = 40;
	colour.b = 0;
	renderer->DrawFilledRect(dest, colour);
	if (NULL != pattern)
		{
		renderer->DrawText(false, "Pattern:", dest, false);
		dest.y += renderer->m_smallFont->GetFontHeight();
		renderer->DrawText(true, pattern->name, dest, false);
		}

}

/// Draw general info
/// - song name
/// - drumkit name
/// - playback mode (song / pattern)
static void DrawGeneralInfo(const char *songName, const char *drumkitName, Transport &transport)
{
	if (!renderer)
		return;

	char s[200];

	// draw song name
	SDL_Rect dest = zones[ZONE_SONGNAME];
	//m_smallFont->DrawText(m_sdlRenderer, "Song:", dest, false);
	renderer->DrawText(false, "Song:", dest, false);
	dest.y += renderer->m_smallFont->GetFontHeight();
	//m_bigFont->DrawText(m_sdlRenderer, songName, dest, false);
	renderer->DrawText(true, songName, dest, false);
	// draw drumkit name
	dest = zones[ZONE_KITNAME];
	//m_smallFont->DrawText(m_sdlRenderer, "Kit:", dest, false);
	renderer->DrawText(false, "Kit:", dest, false);
	dest.y += renderer->m_smallFont->GetFontHeight();
	//m_bigFont->DrawText(m_sdlRenderer, drumkitName, dest, false);
	renderer->DrawText(true, drumkitName, dest, false);

	// draw shuffle / volrand options values
	dest = zones[ZONE_OPTIONS];
	dest.x += 4;
	sprintf(s, "Shuffle %d", transport.shuffle);
	//m_smallFont->DrawText(m_sdlRenderer, s, dest, false);
	renderer->DrawText(false, s, dest, false);
	dest.y += 16;
	sprintf(s, "Jitter  %d", transport.jitter);
	//m_smallFont->DrawText(m_sdlRenderer, s, dest, false);
	renderer->DrawText(false, s, dest, false);
	dest.y += 16;
	sprintf(s, "VolRand %d ", transport.volrand);
	//m_smallFont->DrawText(m_sdlRenderer, s, dest, false);
	renderer->DrawText(false, s, dest, false);

	// transport display (mode, play/rewind, etc)
	dest = zones[ZONE_TRANSPORT];
	//SDL_Rect src = texmap[TM_MODE_BTN];
	//src.w *= 3;
	//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
	renderer->DrawButton(TM_MODE_BTN, dest);

	dest.x += 4;
	dest.y += 20;
	if (Transport::PM_PATTERN == transport.mode)
		//m_smallFont->DrawText(m_sdlRenderer, "Pattern", dest, false);
		renderer->DrawText(false, "Pattern", dest, false);
	else if (Transport::PM_SONG == transport.mode)
		//m_smallFont->DrawText(m_sdlRenderer, " Song", dest, false);
		renderer->DrawText(false, " Song", dest, false);
	else
		//m_smallFont->DrawText(m_sdlRenderer, " Live", dest, false);
		renderer->DrawText(false, " Live", dest, false);

	if (transport.playing)
		{
		dest = zones[ZONE_TRANSPORT];
		dest.x += 64;
		dest.w = 64;
		//src = texmap[TM_PAUSE_BTN];
		//SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
		renderer->DrawButton(TM_PAUSE_BTN, dest);
		}

}

/// Redraw everything
static void DrawAll()
{
	if (!renderer)
		return;

	// clear background
	renderer->Clear();
	// draw various elements
	DrawSliders(song.vol, song.BPM);
	DrawGeneralInfo(song.name, drumKit.name, transport);
	DrawSequenceList(song);
	DrawPatternList(song);
	DrawTrackInfo(song, drumKit);
	// Draw the pattern at index currentPatternIndex (for live mode)
	if (NO_PATTERN_INDEX == song.currentPatternIndex)
		DrawPatternGrid(NULL);
	else
		DrawPatternGrid(&song.patterns[song.currentPatternIndex]);
}

/// Prompt user to load a drumkit
/// @return			true if drumkit selected and loaded OK
static bool PromptLoadDrumkit()
{
	bool loaded = false;
	char kitname[DRUMKIT_NAME_LEN];
	strcpy(kitname, drumKit.name);
	if (DoFileSelect(renderer, "Select DrumKit to load", "kits", kitname))
		{
		// NB: WE must stop playback while loading/unloading samples (chunks)
		bool wasPlaying = transport.playing;
		transport.playing = false;
		Mix_HaltChannel(-1);			// stop all channels playing
		loaded = drumKit.Load(kitname, progress_callback);
		if (!loaded)
			DoMessage(renderer, "Error", "Error loading drumkit!", false); 
		transport.playing = wasPlaying;
		}

	return loaded;
}

/// Display the options menu and process the result 
static int DoOptionsMenu()
{
	int selectedShuffleOption = transport.shuffle;
	int selectedJitterOption = transport.jitter / 5;
	int selectedVolrandOption = transport.volrand / 10;
	int selectedFlashOption = transport.flashOnBeat ? 1 : 0; 

	Menu menu;
	menu.AddItem(1, "Shuffle", "0|1|2|3", selectedShuffleOption, "Set shuffle amount (2 = standard shuffle)");	
	menu.AddItem(2, "Jitter (ms)", "0|5|10|15", selectedJitterOption, "!!! NOT IMPLEMENTED !!!"); // Randomise playback timing");	
	menu.AddItem(3, "VolRand (%)", "0|10|20|30|40|50", selectedVolrandOption, "Randomise hit volume");	
	menu.AddItem(4, "Flash BG on Beat", "No|Yes", selectedFlashOption, "Flash pattern grid background on the beat");	
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, "Options Menu", 0);
	
	// Update shuffle and jitter from menu item data
	if (selectedId > -1)
		{
		transport.shuffle = menu.GetItemSelectedOption(1);
		transport.jitter = menu.GetItemSelectedOption(2) * 5;
		transport.volrand = menu.GetItemSelectedOption(3) * 10;
		transport.flashOnBeat = (0 == menu.GetItemSelectedOption(4)) ? false : true;
		}
	
	DrawAll();
	
	return selectedId;
}

/// Display the Volume / BPM menu and process the result 
static int DoVolBPMMenu(int initialSelection)
{
	int selectedVolOption = GetMainVolume() / 10;		// main vol 0 to 100
	int selectedBPMOption = (song.BPM / 10) - 6;		// starts at 60

	Menu menu;
	menu.AddItem(1, "Main Vol (%)", "Mute|10|20|30|40|50|60|70|80|90|100", selectedVolOption, "Set main volume");	
	menu.AddItem(2, "BPM", "60|70|80|90|100|110|120|130|140|150|160|170|180", selectedBPMOption, "Set playback speed in BPM");	
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, "Volume/BPM Menu", initialSelection);
	
	// Update shuffle and jitter from menu item data
	if (selectedId > -1)
		{
		SetMainVolume(menu.GetItemSelectedOption(1) * 10);
		song.BPM = (menu.GetItemSelectedOption(2) * 10) + 60;
		}
	
	DrawAll();
	
	return selectedId;
}


/// Display the song context menu and process the result 
static int DoFileMenu(int initialSelection)
{
	Menu menu;
	menu.AddItem(1, "Load song", "Load a song from file");	
	menu.AddItem(2, "Save song", "Save this song to a file");	
	menu.AddItem(3, "Load DrumKit", "Load a different drumkit");
	menu.AddItem(4, "Record to WAV", "Record next play to WAV file");

	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, "File Menu", initialSelection);
	// process result
	switch (selectedId)
		{
		case 1 :		// LOAD
			{
			char songname[SONG_NAME_LEN];
			strcpy(songname, song.name);
			if (DoFileSelect(renderer, "Select song to load", "songs", songname))
				{
				char filename[200];
				strcpy(filename, "songs/");
				strcat(filename, songname);
				//strcat(filename, ".xds");
				if (!song.Load(filename, progress_callback))
					DoMessage(renderer, "Error", "Error loading song!", false); 
				}
			}
			break;
		case 2 :		// SAVE
			{
			// Get name
			char songname[SONG_NAME_LEN];
			strcpy(songname, song.name);
			if (DoTextInput(renderer, "Enter song name", songname, SONG_NAME_LEN))
				{
				// TODO : implement song.SetName() for safety
				strcpy(song.name, songname);
				
				char filename[200];
				strcpy(filename, "songs/");
				strcat(filename, songname);
				strcat(filename, ".xds");
				song.Save(filename, progress_callback);
				}
			}
			break;
		case 3 :		// LOAD DRUMKIT
			{
			PromptLoadDrumkit();
			}
			break;
		case 4 :		// RECORD TO WAV
			{
			// Get wav file name
			char wavname[SONG_NAME_LEN];
			strcpy(wavname, song.name);
			if (DoTextInput(renderer, "Enter output file name", wavname, SONG_NAME_LEN))
				{
				char filename[200];
				strcpy(filename, "wav/");
				strcat(filename, wavname);
				strcat(filename, ".wav");
				wavWriter.Open(filename);
				}
			}
			break;
		}

	DrawAll();
	
	return selectedId;
}
	
/// Display the track context menu and process the result 
/// @param track 		The track that we want to show menu for
static int DoTrackMenu(int track)
{
	char menuTitle[64];
	sprintf(menuTitle, "Track Menu [%s]", drumKit.drums[track].name);

	int selectedVolOption = (song.trackMixInfo[track].vol * 10) / 255;		// track vol 0 to 255

	Menu menu;
	menu.AddItem(1, "Track Vol (%)", "Mute|10|20|30|40|50|60|70|80|90|100", selectedVolOption, "Set track mix volume");	
	menu.AddItem(2, "Copy track", "Copy the track data for this pattern");	
	menu.AddItem(3, "Paste track", "Paste track data");	
	menu.AddItem(4, "Clear track", "Remove all events on this track");
	if (TrackMixInfo::TS_MUTE != song.trackMixInfo[track].state)
		menu.AddItem(5, "Mute track", "Mute this track");
	else
		menu.AddItem(5, "Unmute track", "Unute this track");
	if (TrackMixInfo::TS_SOLO != song.trackMixInfo[track].state)
		menu.AddItem(6, "Solo track", "Solo this track");
	else
		menu.AddItem(6, "Unsolo track", "Switch solo mode off");
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, menuTitle, 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// Set track vol
			{
			// ONLY CHANGE TRACK VOL IF IT IS SELECTED MENU ITEM
			// (because it only goes in steps of 10%)
			song.trackMixInfo[track].vol = (menu.GetItemSelectedOption(1) * 255) / 10;
			}
			break;
		case 2 :		// COPY
			{
			for (int i = 0; i < STEPS_PER_PATTERN; i++)
				{
				trackClipboard[i].CopyFrom(&currentPattern->events[track][i]);
				}
			}
			break;
		case 3 :		// PASTE
			{
			for (int i = 0; i < STEPS_PER_PATTERN; i++)
				{
				currentPattern->events[track][i].CopyFrom(&trackClipboard[i]);
				}
			}
			DrawPatternGrid(currentPattern);
			break;
		case 4 :		// CLEAR
			{
			for (int i = 0; i < STEPS_PER_PATTERN; i++)
				{
				currentPattern->events[track][i].Init();
				}
			}
			DrawPatternGrid(currentPattern);
			break;
		case 5 :		// MUTE / UNMUTE
			// Set / reset MUTE state
			SetTrackMute(track);
			break;
		case 6 :		// SOLO
			// Set / remove solo state
			SetTrackSolo(track);
			break;
		}

	DrawTrackInfo(song, drumKit);

	return selectedId;
}

/// Display the pattern context menu and process the result 
static int DoPatternMenu()
{
	char menuTitle[64];
	sprintf(menuTitle, "Pattern Menu [%s]", currentPattern->name);

	Menu menu;
	menu.AddItem(1, "Copy pattern", "Copy pattern data to the pattern buffer");	
	menu.AddItem(2, "Paste pattern", "Paste pattern data from the pattern buffer");	
	menu.AddItem(3, "Clear pattern", "Remove all events in this pattern");
	menu.AddItem(4, "Rename pattern", "Change name of the pattern");
	menu.AddItem(5, "Insert into song", "Insert current pattern into song");
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, menuTitle, 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// COPY
			patternClipboard.CopyFrom(currentPattern);
			break;
		case 2 :		// PASTE
			currentPattern->CopyFrom(&patternClipboard);
			break;
		case 3 :		// CLEAR
			currentPattern->Clear();
			break;
		case 4 :		// RENAME
			{
			// Get name
			char patname[200];
			strcpy(patname, currentPattern->name);
			if (DoTextInput(renderer, "Enter new pattern name", patname, PATTERN_NAME_LENGTH))
				{
				// TODO : Implement pattern->SetName() (safer!)
				strcpy(currentPattern->name, patname);
				}
			}
			break;
		case 5 :		// INSERT PATTERN
			song.InsertPattern(song.currentPatternIndex);
			break;
		}

	DrawAll();
	
	return selectedId;
}

/// Display the songlist context menu and process the result 
static int DoSequenceMenu()
{
	int currentPosOption = song.songPos / 10;		// track vol 0 to 255

	Menu menu;
	menu.AddItem(1, "Insert pattern", "Insert current pattern into song");
	menu.AddItem(2, "Remove pattern", "Remove pattern from the song");
	menu.AddItem(3, "Go to position", "1|11|21|31|41|51|61|71|81|91", currentPosOption, "Set current song position");	
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, "Sequence Menu", 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// INSERT
			song.InsertPattern(song.currentPatternIndex);
			break;
		case 2 :		// REMOVE
			song.RemovePattern();
			break;
		case 3 :		// SONG GOTO
			// ONLY CHANGE SONG POS IF IT IS SELECTED MENU ITEM
			// (because it only goes in steps of 10)
			song.songPos = menu.GetItemSelectedOption(3) * 10;
			if (song.songPos > PATTERNS_PER_SONG)
				song.songPos = 0;					// safety net
			break;
		}

	DrawAll();
	
	return selectedId;
}

/// Show help menu
static int DoHelpMenu()
{
#ifdef PSP			
			const char* text = 	"START to stop/start playing\n" \
								"SQUARE to rewind\n" \
								"SELECT to change mode\n" \
								"LSB for previous pattern\n" \
								"RSB for next pattern";
#else
			const char* text = 	"SPACE to stop/start playing\n" \
								"R to rewind\n" \
								"M to change mode\n" \
								"PGUP for previous pattern\n" \
								"PGDOWN for next pattern";
#endif
	DoMessage(renderer, "Help - Keys / Buttons", text, false);
	//SDL_ShowSimpleMessageBox(0, "Help - Keys/Buttons", text, sdlWindow);							
	return 0;
}

/// Display the main menu (menu of menus)
static int DoMainMenu()
{
	Menu menu;
	menu.AddItem(1, "File Menu", "Display the file menu");
	menu.AddItem(2, "Pattern Menu", "Display the pattern menu");
	menu.AddItem(3, "Sequence Menu", "Display the sequence menu");
	menu.AddItem(4, "Track Menu", "Display the track menu");
	menu.AddItem(5, "Options Menu", "Display the options menu");
	menu.AddItem(6, "Help Menu", "Display the help menu");
	menu.AddItem(7, "Quit", "Exit this program");
	SDL_Rect r1;
	SetSDLRect(r1, 16, 16, VIEW_WIDTH - 32, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, "Main Menu", 0);
	// process result
	switch (selectedId)
		{
		case 1 :		// FILE MENU
			DoFileMenu(0);
			break;
		case 2 :		// PATTERN MENU
			DoPatternMenu();
			break;
		case 3 :		// SONGLIST MENU
			DoSequenceMenu();
			break;
		case 4 :		// TRACK MENU
			DoTrackMenu(currentTrack);
			break;
		case 5 :		// OPTIONS MENU
			DoOptionsMenu();
			break;
		case 6 :		// HELP MENU
			DoHelpMenu();
			break;
		case 7 :		// QUIT
			// Prompt to confirm quit, then to save
			if (DoMessage(renderer, "Confirm Quit", "Sure to quit?", true))
				{
				quit = true;
				if (DoMessage(renderer, "Save On Exit", "Do you want to save?", true))
					{
					// Get name
					char songname[SONG_NAME_LEN];
					strcpy(songname, song.name);
					if (DoTextInput(renderer, "Enter song name", songname, SONG_NAME_LEN))
						{
						// TODO : implement song.SetName() for safety
						strcpy(song.name, songname);
						
						char filename[200];
						strcpy(filename, "songs/");
						strcat(filename, songname);
						strcat(filename, ".xds");
						song.Save(filename, progress_callback);
						}
					}
				}
			break;
		default :
			break;
		} // end switch

	DrawAll();
	
	return selectedId;
}

/// Display the note context menu and process the result 
/// @param track			Relevant track (if -1, then use "current track" and "current step")
/// @param step				Relevant step in the track
/// @return					Id of selected item, or -1 if menu escaped
static int DoNoteMenu(int track, int step)
{
	if (!currentPattern)
		return -1;

	// use pattern grid cursor position if none specified 
	if (-1 == track)
		{
		track = currentTrack;
		step = currentStep;
		}

	char menuTitle[64];
	sprintf(menuTitle, "Note Menu [Track %d, Step %d]", track+1, step+1);

	// TODO : Handle cut notes (do not display vol menu item?) 
	int volPercent = (currentPattern->events[track][step].vol * 100) / 127;
	int selectedVolOption = (volPercent + 5) / 10;
	int selectedDecayOption = 0;
	int selectedCountOption = 0;
	int selectedSpacingOption = 0;

	Menu menu;
	menu.AddItem(1, "Note Vol (%)", "0|10|20|30|40|50|60|70|80|90|100", selectedVolOption, "Set note volume (10 - 100)");	
	menu.AddItem(2, "Repeat Decay (%)", "0|10|20|30|40|50|60|70|80|90", selectedDecayOption, "Volume decay between repeat notes");	
	menu.AddItem(3, "Repeat Count", "0|1|2|3|4|5|6|7", selectedCountOption, "Number of repeat notes");
	menu.AddItem(4, "Repeat Space", "1|2|3|4", selectedSpacingOption, "Steps between repeat notes");
	SDL_Rect r1;
	SetSDLRect(r1, 64, 16, VIEW_WIDTH - 72, VIEW_HEIGHT - 32);
	int selectedId = menu.DoMenu(renderer, &r1, menuTitle, 0);
	
	// Update options from menu item data
	switch (selectedId)
		{
		case 1 : // note vol
			selectedVolOption = menu.GetItemSelectedOption(1);
			currentPattern->events[track][step].vol = (selectedVolOption * 127) / 10;
			break;
		case 2 :
		case 3 :
		case 4 :
			{
			selectedDecayOption = menu.GetItemSelectedOption(2);
			float repeatVol = (float)currentPattern->events[track][step].vol;
			float decay = (float)selectedDecayOption * 0.1f;
			int count = menu.GetItemSelectedOption(3);
			int space = menu.GetItemSelectedOption(4) + 1;
			int repeatStep = step;
			for (int i = 0; i < count; i++)
				{
				repeatStep += space;
				if (repeatStep < STEPS_PER_PATTERN)
					{
					repeatVol = repeatVol * (1.0f - decay);
					currentPattern->events[track][repeatStep].vol = (unsigned char)repeatVol;
					}
				}
			}
			break;
		}
	
	DrawAll();
	
	return selectedId;
}

// Get mouse zone, based on current screen mode
static int GetMouseZone(int x, int y, int mode)
{
	int zone = 0;	 		// no zone / bad zone
	
	if (XM_MAIN == mode)
		{
		for (int i = 1; i < ZONE_MAX; i++)
			{
			if (x >= zones[i].x && x < (zones[i].x + zones[i].w))
				{
				if (y >= zones[i].y && y < (zones[i].y + zones[i].h))
					{
					zone = i;
					i = ZONE_MAX;
					} 
				}
			}
		}
	else if (XM_FILE == mode)
		{
		}
		
	return zone;
}

/// Jump the mouse cursor to the pattern grid cursor
static void SetMouseToGridCursor()
{
	SDL_Rect rect = zones[ZONE_PATGRID];
	cursorX = (float)(rect.x + (currentStep * PATBOX.w) + (PATBOX.x / 2));
	cursorY = (float)(rect.y + (currentTrack * PATBOX.h) + (PATBOX.y / 2));
	SDL_WarpMouseInWindow(sdlWindow, (int)cursorX, (int)cursorY);
}

// Handle key press
void ProcessKeyPress(SDL_Scancode sym, int zone)
{
#ifdef PSP
	// Handle PSP headphone remote control
	// The headphone remote control is treated as a keyboard that sends the
	// following keypresses:
	// F10 Play/Pause
	// F11 Forward
	// F12 Back
	// F13 Volume Up
	// F14 Volume Down
	// F15 Hold
	if (SDLK_F10 == sym)
		sym = SDLK_SPACE;
	else if (SDLK_F11 == sym)
		sym = SDLK_PAGEDOWN;
	else if (SDLK_F12 == sym)
		sym = SDLK_PAGEUP;
/*	else if (SDLK_F13 == sym)
		{
		int vol = (song.vol * 100) / 255;
		if (vol < 98)
		vol += 3;
		SetMainVolume(vol);
		DrawSliders(backImg);
		}
	else if (SDLK_F14 == sym)
		{
		int vol = (song.vol * 100) / 255;
		if (vol > 2)
		vol -= 1;
		SetMainVolume(vol);
		DrawSliders(backImg);
		}*/
#endif
	
	switch (sym)
		{
		case SDL_SCANCODE_LEFT :
			if (currentStep > 0)
				currentStep--;
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			// Lock mouse cursor to pattern grid current pos
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDL_SCANCODE_RIGHT :
			currentStep++;
			if (currentStep >= STEPS_PER_PATTERN)
				currentStep = 0;			// wrap around
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDL_SCANCODE_UP :
			if (currentTrack > 0)
				currentTrack--;
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDL_SCANCODE_DOWN :
			if (currentTrack < (NUM_TRACKS - 1))
				currentTrack++;
#ifdef LOCK_MOUSE_TO_GRID_CURSOR				
			if (ZONE_PATGRID == zone)
				SetMouseToGridCursor();
#endif
			break;
		case SDL_SCANCODE_X :
			// set note on
			if (currentPattern)
				{
				unsigned char vol = currentPattern->events[currentTrack][currentStep].vol;
				if (0 == vol)
					vol = 64;			// note on
				else if (64 == vol)
					vol = 127;			// accent
				else if (127 == vol)
					vol = 1;			// cut
				else
					vol = 0;			// no note
				currentPattern->events[currentTrack][currentStep].vol = vol;
				// play sample
				if (vol > 1)
					{
					Mix_VolumeChunk(drumKit.drums[currentTrack].sampleData, vol);
					Mix_PlayChannel(-1, drumKit.drums[currentTrack].sampleData, 0);
					}
				// redraw grid
				DrawPatternGrid(currentPattern);								
				}
			break;
		case SDL_SCANCODE_C :
		case SDL_SCANCODE_DELETE :
			// cut note
			if (currentPattern)
				{
				if (1 == currentPattern->events[currentTrack][currentStep].vol)
					currentPattern->events[currentTrack][currentStep].vol = 0;
				else
					currentPattern->events[currentTrack][currentStep].vol = 1;
				// redraw grid
				DrawPatternGrid(currentPattern);								
				}
			break;
		case SDL_SCANCODE_SPACE :
			// Start or stop playback or record to wav
			if (wavWriter.IsOpen())
				{
				if (transport.playing)
					wavWriter.Close();
				else
					wavWriter.StartWriting();
				}

			transport.playing = !transport.playing;
			// redraw play/pause button
			DrawGeneralInfo(song.name, drumKit.name, transport);								
			break;
		case SDL_SCANCODE_B :
		case SDL_SCANCODE_R :
		case SDL_SCANCODE_BACKSPACE :
			transport.playing = false;
			transport.patternPos = 0;
			if (Transport::PM_SONG == transport.mode)
				{
				song.songPos = 0;
			    song.currentPatternIndex = song.songList[song.songPos];
			    SyncPatternPointer();
				}
			DrawAll();	
			break;
		case SDL_SCANCODE_M :
			// switch playback mode (song/pattern)
			if (Transport::PM_PATTERN == transport.mode)
				transport.mode = Transport::PM_SONG;
			else if (Transport::PM_SONG == transport.mode)
				transport.mode = Transport::PM_LIVE;
			else transport.mode = Transport::PM_PATTERN;
			DrawAll();
			break;
		case SDL_SCANCODE_LEFTBRACKET :
		case SDL_SCANCODE_PAGEUP:
			// previous pattern / song pos
			if (Transport::PM_PATTERN == transport.mode || Transport::PM_LIVE == transport.mode)
				{
				if (song.currentPatternIndex >= MAX_PATTERN)
					song.currentPatternIndex = 0;
				else if (song.currentPatternIndex > 0)
					song.currentPatternIndex--;
				}
			else if (Transport::PM_SONG == transport.mode)
				{
			    if (song.songPos > 0)
			    	song.songPos--;
			    song.currentPatternIndex = song.songList[song.songPos];
				}
			// sync pattern pointer (unless live mode)
			if (Transport::PM_LIVE != transport.mode)
				SyncPatternPointer();
			DrawAll();	
			break;
		case SDL_SCANCODE_RIGHTBRACKET :
		case SDL_SCANCODE_PAGEDOWN:
			// next pattern / song pos
			if (Transport::PM_PATTERN == transport.mode || Transport::PM_LIVE == transport.mode)
				{
				if (song.currentPatternIndex >= MAX_PATTERN)
					song.currentPatternIndex = 0;
				else if (song.currentPatternIndex < (MAX_PATTERN - 1))
					song.currentPatternIndex++;
				}
			else if (Transport::PM_SONG == transport.mode)
				{
			    if (song.songPos < (PATTERNS_PER_SONG - 1))
			    	song.songPos++;
			    song.currentPatternIndex = song.songList[song.songPos];
				}
			// sync pattern pointer (unless live mode)
			if (Transport::PM_LIVE != transport.mode)
				SyncPatternPointer();
			DrawAll();	
			break;
		case SDL_SCANCODE_ESCAPE :
		case SDL_SCANCODE_MENU :
		case SDL_SCANCODE_RGUI :
			DoMainMenu();
			break;
		case SDL_SCANCODE_HELP :
		case SDL_SCANCODE_F1 :
			DoHelpMenu();
			break;
		default :
			break;
		}
}

// Handle left mouse click
void ProcessLeftClick(int x, int y)
{
	int zone = GetMouseZone(x, y, XM_MAIN);
	
	// no zone or invalid zone?
	if (0 == zone)
		return;
		
	// "local x and y" (relative to the zone clicked in)
	SDL_Rect rect = zones[zone];
	int x0 = x - rect.x;
	int y0 = y - rect.y;
	printf("ProcessLeftClick: zone = %d, x0 = %d, y0 = %d\n", zone, x0, y0);
		
	switch(zone)
		{
		case ZONE_VOL :
			{
			// If [+] or [-] button, increment or decrement
			// If on volmeter, set main vol (0 to 100%)
			int vol = (song.vol * 100) / 255;
			if (x0 < PATBOX.w)
				vol -= 1;
			else if (x0 >= (rect.w - PATBOX.w))
				vol += 1;
			else if (x0 >= PATBOX.w && x0 < (rect.w - PATBOX.w))
				vol =  ((x0 - PATBOX.w) * 100) / (rect.w - PATBOX.w - PATBOX.w);
				
			SetMainVolume(vol);
			DrawSliders(song.vol, song.BPM);
			}
			break;
		case ZONE_BPM :
			{
			// If [+] or [-] button, increment or decrement BPM
			// If on bpm-meter, set bpm (50 to 250)
			if (x0 < PATBOX.w)
				song.BPM -= 1;
			else if (x0 >= (rect.w - PATBOX.w))
				song.BPM += 1;
			else if (x0 >= PATBOX.w && x0 < (rect.w - PATBOX.w))
				song.BPM = x0 - PATBOX.w + 50;
				
			DrawSliders(song.vol, song.BPM);
			}		
			break;
		case ZONE_SONGLIST :
			{
			if (x0 < 10)
				{
				if (song.songListScrollPos > 0)
					song.songListScrollPos--;
				}
			else if (x0 > rect.w - 11)
				{
				if (song.songListScrollPos < PATTERNS_PER_SONG - 10)
					song.songListScrollPos++;
				}
			else
				{
			    int songIndex = song.songListScrollPos + (x0 / PATBOX.w);
			    if (songIndex >= 0 && songIndex < PATTERNS_PER_SONG)
			    	{
					song.songPos = songIndex;
					song.currentPatternIndex = song.songList[song.songPos];
					SyncPatternPointer();
					}
				}
			DrawAll();
			}
			break;
		case ZONE_PATLIST :
			{
			int patNum = x0 / PATBOX.w;
			if (y0 >= PATBOX.h)
				{
				patNum += 10;
				}
		    song.currentPatternIndex = patNum;
   			if (Transport::PM_LIVE != transport.mode)
				{
				SyncPatternPointer();
				}
			DrawAll();
			}
			break;
		case ZONE_ADDTOSONGBTN :
			song.InsertPattern(song.currentPatternIndex);
			DrawSequenceList(song);
			break;			
		case ZONE_TRACKINFO :
			{
			// Handle mute / solo / mix vol 
			int track = y0 / PATBOX.h;
			if (x0 > rect.w - PATBOX.w)
				{
				// Set / reset MUTE state
				SetTrackMute(track);
				}
			else if (x0 < (rect.w - PATBOX.w))
				{
				// adjust track vol
				unsigned char trackMixVol = song.trackMixInfo[track].vol;
				if (x0 > (rect.w - PATBOX.w - PATBOX.w))
					{
					if (trackMixVol < 245)
						trackMixVol += 10;
					}
				else if (x0 < PATBOX.w)
					{
					if (trackMixVol > 10)
						trackMixVol -= 10;
					}
				song.trackMixInfo[track].vol = trackMixVol;
				}	
				
			DrawTrackInfo(song, drumKit);
			}
			break;
		case ZONE_PATGRID :
			{
			// Pattern grid click
			int step = x0 / PATBOX.w;
			int track = y0 / PATBOX.h;
			printf("Patgrid: track = %d, step = %d\n", track, step);
			// update pattern cursor pos
			currentStep = step;
			currentTrack = track;
			if (currentPattern)
				{
				/*
				unsigned char vol = currentPattern->events[track][event].vol;
				if (0 == vol)
					vol = 64;
				else if (64 == vol)
					vol = 127;
				else
					vol = 0;
				currentPattern->events[track][event].vol = vol;
				// redraw grid
				DrawPatternGrid(backImg, currentPattern);
				*/
				ProcessKeyPress(SDL_SCANCODE_X, ZONE_PATGRID);							
				}
			}
			break;
		case ZONE_PATNAME :
			{
			DoPatternMenu();
			}
			break;
		case ZONE_SONGNAME :
			{
			DoFileMenu(0);
			}
			break;
		case ZONE_KITNAME :
			{
			DoFileMenu(2);	
			}
			break;
		case ZONE_OPTIONS :
			{
			DoOptionsMenu();	
			}
			break;
		case ZONE_TRANSPORT :
			{
			// Handle mode / play / rewind button 
			int which = x0 / 64;
			if (0 == which)
				{
				// switch playback mode (song/pattern)
				ProcessKeyPress(SDL_SCANCODE_M, ZONE_TRANSPORT);
				}
			else if (1 == which)
				{
				// play / stop
				ProcessKeyPress(SDL_SCANCODE_SPACE, ZONE_TRANSPORT);
				}
			else if (2 == which)
				{
				// rewind
				ProcessKeyPress(SDL_SCANCODE_R, ZONE_TRANSPORT);
				}
			}
			break;
		default :
			printf("No zone!\n");
		}
}

// Handle right mouse click
void ProcessRightClick(int x, int y)
{
	int zone = GetMouseZone(x, y, XM_MAIN);
	printf("ProcessRightClick: zone = %d\n", zone);
	
	// no zone or invalid zone?
	if (0 == zone)
		return;
		
	// "local x and y" (relative to the zone clicked in)
	SDL_Rect rect = zones[zone];
	int x0 = x - rect.x;
	int y0 = y - rect.y;
		
	switch(zone)
		{
		case ZONE_VOL :
			{
			DoVolBPMMenu(0);
			}
			break;
		case ZONE_BPM :
			{
			DoVolBPMMenu(1);
			}		
			break;
		case ZONE_SONGLIST :
			{
			if (y0 < 10)
				{
				}
			else if (y0 > rect.h - 11)
				{
				}
			else
				{
				// do song list menu
				DoSequenceMenu();
				// NB: songlist menu does a DrawAll()
				}
			}
			break;
		case ZONE_PATLIST :
			{
			if (y0 < 10)
				{
				}
			else if (y0 > rect.h - 11)
				{
				}
			else
				{
				// pattern menu
				DoPatternMenu();
				// NB: pattern menu does a DrawAll()
				}
			}
			break;
		case ZONE_TRACKINFO :
			{
			// TODO : Pop up track menu (Use PSP menus?)
			//        - Copy
			//        - Paste
			//        - Clear
			//        - mute
			//        - solo
			// Handle mute / solo / mix vol 
			int track = y0 / PATBOX.h;
			y0 = y0 - (track * PATBOX.h);		// get y0 local to the specific track
			
			DoTrackMenu(track);
			}
			break;
		case ZONE_PATGRID :
			{
			// Show Right-click Note Menu
			int track = y0 / PATBOX.h;
			int step = x0 / PATBOX.w;
			DoNoteMenu(track, step);
			}
			break;
		case ZONE_PATNAME :
			{
			DoPatternMenu();
			}
			break;
		case ZONE_SONGNAME :
			{
			DoFileMenu(0);
			}
			break;
		case ZONE_KITNAME :
			{
			DoFileMenu(2);	
			}
			break;
		case ZONE_OPTIONS :
			{
			DoOptionsMenu();	
			}	
			break;
		default :
			printf("No zone!\n");
		}
	
}


/// Handle controller buttons
/// eg: PSP
void ProcessControllerButtonPress(Uint8 button, int x, int y)
{
	int zone = GetMouseZone(x, y, XM_MAIN);
	
	// controller buttons are mapped according to joymap.cfg file,
	// which maps controller buttons to SDL/ASCII keys
	unsigned short keyCode = joyMap.GetValueAt(button);
	if (LCLICK == keyCode)
		{
		// If clicked in pattern grid, then trigger "add note", else 
		// treat like left mouse click
		if (ZONE_PATGRID == zone)
			ProcessKeyPress(SDL_SCANCODE_X, ZONE_PATGRID);
		else
			ProcessLeftClick(x, y);
		}
	else if (RCLICK == keyCode)
		ProcessRightClick(x, y);
	else
		ProcessKeyPress((SDL_Scancode)keyCode, zone);
}



// thread example
int global_data = 0;
bool beatFlash = false;

// interval calcs
// beats per second (BPS) = BPM / 60
// quarter-beats per second (QBPS) = BPS * 4
// interval (msec) =  1000 / QBPS
// eg: 100 BPM = 6.666 QBPS, therefore interval = 150 ms (per quarter-beat)

// play thread
// new "float" version (as of v1.2 25/4/2009)
int play_thread_func(void *data)
{
	printf("Play thread starting...\n");
	
	// set up timing
	float nextTick = (float)SDL_GetTicks();
	
    int last_value = 0;
    while ( global_data != -1 )
    	{
        if ( global_data != last_value )
        	{
            //printf("Data value changed to %d\n", global_data);
            last_value = global_data;
        	}

		// calc tick interval (in case BPM has changed)
		// 16 ticks per beat = 64 ticks per pattern
		float bps = (float)song.BPM / 60.0f;
		float tps = bps * 16;
		float interval = (1000.0f / tps);

		// check if CPU is struggling
		global_data = (SDL_GetTicks() < (Uint32)nextTick) ? 0 : 1;			// 1 means CPU struggling
		
		// wait for next tick
		// TODO: put delay inside while!
		//SDL_Delay(10);		// !!! need this otherwise other thread will never process!
		int remaining = (Uint32)nextTick - SDL_GetTicks();
		if (remaining > 15)
			SDL_Delay(remaining - 5); 
		else
			global_data = 1;
		// note that we cannot use SDL_Delay() to wait for the next tick because
		// it's granularity is 10ms on some systems!!!
		while (SDL_GetTicks() < (Uint32)nextTick) ;

		nextTick += interval;
			
		// only process events if we are playing
		if (transport.playing)
			{
			// are we on the next quarter-beat?
			int beatPos = transport.patternPos & 0xF;
			bool onEvent = false;
			if (transport.shuffle > 0)
				onEvent = (0 == beatPos || 4 == beatPos || (8 + transport.shuffle) == beatPos || 12 == beatPos);
			else
				onEvent = (0 == (beatPos & 0x3));
			if (onEvent)
				{
				int event = transport.patternPos / 4;
				// do we have a note to play?
				if (currentPattern)
					{
					for (int track = 0; track < NUM_TRACKS; track++)
						{
						if (currentPattern->events[track][event].vol > 1)
							{
							// calculate output vol
							int trackMixVol = (TrackMixInfo::TS_MUTE == song.trackMixInfo[track].state) ? 0 : song.trackMixInfo[track].vol;
							if (trackMixVol > 0)
								{
								int chunkVol = (song.vol * trackMixVol * currentPattern->events[track][event].vol) / (512 * 128);
								// randomise output vol if neccessary
								if (transport.volrand > 0)
									{
									int range = (chunkVol * transport.volrand) / 100;
									chunkVol += (rand() % range) - range / 2;
									if (chunkVol < 0)
										chunkVol = 0;
									else if (chunkVol >= MIX_MAX_VOLUME)
										chunkVol = MIX_MAX_VOLUME - 1;
									}
								// play the sample
								if (drumKit.drums[track].sampleData)
									{
									Mix_VolumeChunk(drumKit.drums[track].sampleData, chunkVol);
									Mix_PlayChannel(-1, drumKit.drums[track].sampleData, 0);
									// FUTURE? - Set pan (get channel from Mix_Playchannel() return value)
									//Mix_SetPanning(channel, 127, 127);	// channel, left, right
									}
								}
							}
						else if (1 == currentPattern->events[track][event].vol)
							{
							// cut note
							// Find which channel (if any) this track's sample chunk was the last played on
							for (int channel = 0; channel < MIX_CHANNELS; channel++)
								{
								Mix_Chunk* chunk = Mix_GetChunk(channel);
								if (chunk && (chunk == drumKit.drums[track].sampleData))
									{
									//printf("cut track %d, channel %d\n", track, channel);
									Mix_HaltChannel(channel);
									channel = MIX_CHANNELS;
									}
								}
							}
						}
					}
				}

			// If on the beat, then set flash flag
			if (transport.flashOnBeat && (0 == beatPos))
				beatFlash = true;

			// update current pattern tick pos
			transport.patternPos++;
			if (64 == transport.patternPos)
				{
				if (Transport::PM_SONG == transport.mode)
					{
					// next song pos
					song.songPos++;
					if (PATTERNS_PER_SONG == song.songPos)
						song.songPos = 0;
					song.currentPatternIndex = song.songList[song.songPos];
					
					// If we hit a "No pattern" in our songlist, then rewind
					if (NO_PATTERN_INDEX == song.currentPatternIndex)
						{
						song.songPos = 0;
						song.currentPatternIndex = song.songList[0];
						}
					
					SyncPatternPointer();
					}
				else if (Transport::PM_LIVE == transport.mode)
					{
					// start playing "queued" pattern
					SyncPatternPointer();
					}
				transport.patternPos = 0;
				}
				
			} // end if playing
        	
    	} // wend
    	
    // play thread should never quit until program quit
    printf("Play thread quitting\n");
    return(0);
}

// TEST 	
// make a passthru processor function that does nothing...
void noEffect(void *udata, Uint8 *stream, int len)
{
    // Get current output "level"
	short* samples = (short *)stream;
	short maxval = 0;
	for (int i = 0; i < 200; i++)
		{
		if (samples[i] > maxval)
			maxval = samples[i];
		}

	//maxval = abs(maxval);
	if (maxval > 10000)
		maxval = 10000;
	//printf("val = %d\t", maxval);
	if (maxval > outputSample)
		outputSample = maxval;
	else if (outputSample > 0)
		{
		//outputSample = (outputSample + val);
		outputSample = (outputSample * 19) / 20;
		}

	// If we are recording, then write to record file
	if (wavWriter.IsOpen() && wavWriter.IsWriting())
		{
		wavWriter.AppendData(stream, len);
		}
}
//END TEST



// required for PSP (but not Linux !?!?)
#ifdef __cplusplus 
extern "C"
#endif
int main(int argc, char *argv[])
{
	int audio_rate;
	int audio_channels;
	// set this to any of 512,1024,2048,4096
	// the higher it is, the more FPS shown and CPU needed
#ifdef RPI
	int audio_buffers = 1024;
#else
	int audio_buffers = 512;
#endif
	Uint16 audio_format;

	SDL_Rect src;
	SDL_Rect dest;

	float cursorDX = 0.0;
	float cursorDY = 0.0;

	printf("PXDRUM V%s - Copyright James Higgs 2009-2013\n", XDRUM_VER);

	// init song
	song.vol = SDL_MIX_MAXVOLUME;
	
	song.currentPatternIndex = 0;
	currentPattern = &song.patterns[song.currentPatternIndex];
	currentPattern->events[0][0].vol = 100;
	currentPattern->events[0][4].vol = 60;

	SDL_Joystick *joystick = NULL;

	// initialize SDL for audio, video and joystick
	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        {
		printf("init error: %s\n", SDL_GetError());
		return 1;
        }

	sdlWindow = SDL_CreateWindow("PXDRUM 2",
                          SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED,
                          VIEW_WIDTH, VIEW_HEIGHT,
                          /*SDL_WINDOW_FULLSCREEN | */ SDL_WINDOW_OPENGL);

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_PRESENTVSYNC);

	// Create the xdrum renderer utility class
	// (Loads textures and fonts too)
	// TODO : move sdlRenderer pointer into Renderer class
	renderer = new Renderer(sdlRenderer);

	// Set window caption for windowed versions
	char caption[64];
	sprintf(caption, "PXDrum v%s", XDRUM_VER);
	SDL_SetWindowTitle(sdlWindow, caption);

	// Show splash screen while loading
	//ShowSplashScreen(sdlRenderer);
	renderer->ShowSplashScreen();

	// initialize sdl mixer, open up the audio device
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, audio_buffers) < 0)
        {
		printf("Mix_OpenAudio: %s\n", SDL_GetError());
		return 1;
        }

	// print out some info on the audio device and stream
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
	int bits = audio_format & 0xFF;
	printf("Opened audio at %d Hz %d bit %s, %d bytes audio buffer\n", audio_rate,
			bits, audio_channels > 1 ? "stereo" : "mono", audio_buffers );


	SDL_Delay(500);
/*
	// Note: may be issue with file handles not released
	trackInfo[0].sampleData = Mix_LoadWAV("audio/Voyager/VGR Kick-low end.wav");
	
	if (!trackInfo[0].sampleData)
		{
		printf("Error loading sample!\n");
		}

	trackInfo[1].sampleData = Mix_LoadWAV("audio/Voyager/VGR Snare.wav");
	trackInfo[2].sampleData = Mix_LoadWAV("audio/Voyager/VGR CHH.wav");
*/

	// Load default drumkit
	if (!drumKit.Load("default", progress_callback))
		{
		//DoMessage(sdlRenderer, bigFont, "Error", "Cannot load drumkit 'default'!\nPlease select a drumkit.", false);
		SDL_ShowSimpleMessageBox(0, "Error", "Cannot load drumkit 'default'!\nPlease select a drumkit.", sdlWindow);
		if (!PromptLoadDrumkit())
			{
			//DoMessage(sdlRenderer, bigFont, "Error", "Cannot continue without drumkit!\nProgram will exit.", false);
			SDL_ShowSimpleMessageBox(0, "Error", "Cannot continue without drumkit!\nProgram will exit.", sdlWindow);
			return 1;
			}
		}

	// Initial draw of everything
	DrawAll();
	
	// And play a corresponding sound
	Mix_PlayChannel(0, drumKit.drums[0].sampleData, 0);


	if(SDL_NumJoysticks())
        {
		joystick = SDL_JoystickOpen(0);
		printJoystickInfo(joystick);
#ifdef PSP
		joyMap.Load("joymap.psp.cfg");
#else
		joyMap.Load("joymap.cfg");
#endif
        }
	else
        {
		printf("No joystick detected\n");
        }

	// register noEffect as a postmix processor
	Mix_SetPostMix(noEffect, NULL);

	// Create thread to play beats in the background
	SDL_Thread *playThread = SDL_CreateThread(play_thread_func, "audio_thread", NULL);
	if (!playThread)
		{
		printf("Unable to create play thread: %s\n", SDL_GetError());
		return 1;
		}

	// start playing
	transport.playing = true;
	
	// for detecting pattern change (when playing song mode)
	int displayedPatternIndex = song.currentPatternIndex;
	int displayedSongPos = song.songPos;

	SDL_ShowCursor(SDL_DISABLE);
	SDL_Surface *patternDragCursorImg = SDL_LoadBMP("gfx/patternDragCursor.bmp");
	SDL_Cursor *patternDragCursor = SDL_CreateColorCursor(patternDragCursorImg, 0, 0);
	SDL_Surface *songItemDragCursorImg = SDL_LoadBMP("gfx/songItemDragCursor.bmp");
	SDL_Cursor *songItemDragCursor = SDL_CreateColorCursor(songItemDragCursorImg, 0, 0);

	//Uint32 lastTick = SDL_GetTicks();
	int currentZone = 0;
	bool draggingPattern = false;
	int draggedSongItem = -1;
	SDL_Event event;
	quit = false;
	while(!quit)
		{
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					ProcessControllerButtonPress(event.jbutton.button, (int)cursorX, (int)cursorY);	
					break;
				case SDL_JOYBUTTONUP:
					//if(event.jbutton.button == 7 /* LEFT */ && dir==0) dir=-1;
					//if(event.jbutton.button == 8 /* UP */ && dir==1) dir=-1;
					//if(event.jbutton.button == 9 /* RIGHT */ && dir==2) dir=-1;
					//if(event.jbutton.button == 6 /* DOWN */ && dir==3) dir=-1;
					break;
                case SDL_JOYAXISMOTION:
                	// Simulate a mouse
					//printf("Axis motion: j index = %d axis = %d value = %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
					if (0 == event.jaxis.axis)
						{
						float dx = (float)(event.jaxis.value - JOYMID) / (JOYRANGE / 2);
						//printf("DX = %.3f\n", cursorDX);
						if (dx > -0.15 && dx < 0.15)
							cursorDX = 0.0;
						else
							cursorDX = (cursorDX + dx) * 0.5f;			// avg of last 2
						}
					else if (1 == event.jaxis.axis)
						{
						float dy = (float)(event.jaxis.value - JOYMID) / (JOYRANGE / 2);
						if (dy > -0.15 && dy < 0.15)
							cursorDY = 0.0;
						else
							cursorDY = (cursorDY + dy) * 0.5f;			// avg of last 2
						}
					break;
				// For systems with mouses/trackballs
				case SDL_MOUSEMOTION:
					//printf("Mouse moved by %d,%d to (%d,%d)\n", 
					//	   event.motion.xrel, event.motion.yrel,
					//	   event.motion.x, event.motion.y);
					cursorX = (float)event.motion.x;
					cursorY = (float)event.motion.y;
					cursorDX = 0; cursorDY = 0;
					if (draggingPattern)
						{
						SDL_SetCursor(patternDragCursor);
						SDL_ShowCursor(SDL_ENABLE);
						}
					else if (draggedSongItem > -1)
						{
						SDL_SetCursor(songItemDragCursor);
						SDL_ShowCursor(SDL_ENABLE);
						}
					else
						{
						SDL_ShowCursor(SDL_DISABLE);
						}
					break;
				case SDL_MOUSEBUTTONDOWN:
					//printf("Mouse button %d pressed at (%d,%d)\n",
					//	   event.button.button, event.button.x, event.button.y);
					if (1 == event.button.button)		// Left mouse button
						{
						ProcessLeftClick((int)cursorX, (int)cursorY);
						// Start of pattern item drag or song item drag?
						if (ZONE_PATLIST == GetMouseZone((int)cursorX, (int)cursorY, XM_MAIN))
							draggingPattern = true;
						else if (ZONE_SONGLIST == GetMouseZone((int)cursorX, (int)cursorY, XM_MAIN))
							draggedSongItem = song.songPos;
						}
					else if (3 == event.button.button)	// Right mouse button
						ProcessRightClick((int)cursorX, (int)cursorY);
					break;
				case SDL_MOUSEBUTTONUP:
					{
					int destZone = GetMouseZone((int)cursorX, (int)cursorY, XM_MAIN);
					if (draggingPattern && ZONE_SONGLIST == destZone)
						{
						// insert the selected pattern into the song
						int insertPos = song.songListScrollPos + ((int)cursorX - zones[ZONE_SONGLIST].x) / PATBOX.w;
						if (insertPos < (PATTERNS_PER_SONG - 1))
							{
							int savedSongPos = song.songPos;
							song.songPos = insertPos;
							song.InsertPattern(song.currentPatternIndex);
							song.songPos = savedSongPos;
							if (song.songPos >= insertPos)
								song.songPos++;
							DrawSequenceList(song);
							}
						}
					else if (draggedSongItem > -1 && ZONE_SONGLIST == destZone)
						{
						// move song item
						int destPos = song.songListScrollPos + ((int)cursorX - zones[ZONE_SONGLIST].x) / PATBOX.w;
						if (destPos != draggedSongItem && destPos < (PATTERNS_PER_SONG - 1))
							{
							int savedSongPos = song.songPos;
							int srcPattern = song.songList[draggedSongItem];
							song.songPos = destPos;
							song.InsertPattern(srcPattern);
							song.songPos = draggedSongItem;
							if (draggedSongItem > destPos)
								song.songPos++;
							song.RemovePattern();
							song.songPos = savedSongPos;
							DrawSequenceList(song);
							}
						}
					else if (draggedSongItem > -1 && ZONE_PATLIST == destZone)
						{
						// remove song item
						int savedSongPos = song.songPos;
						song.RemovePattern();
						song.songPos = savedSongPos;
						DrawSequenceList(song);
						}
					draggingPattern = false;
					draggedSongItem = -1;
					SDL_ShowCursor(SDL_DISABLE);
					}
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					ProcessKeyPress(event.key.keysym.scancode, currentZone);
					break;
                case SDL_QUIT:
					quit = true;
					break;
				} // end switch
			}

		// get current zone
		currentZone = GetMouseZone((int)cursorX, (int)cursorY, XM_MAIN);

		// If we have chaned the pattern we're playing, we need to redraw
		if (song.currentPatternIndex != displayedPatternIndex)
			{
			DrawAll();
			displayedPatternIndex = song.currentPatternIndex;
			printf("redraw - pattern changed to %d\n", song.currentPatternIndex);
			}
		else if (displayedSongPos != song.songPos)
			{
			DrawSequenceList(song);
			displayedSongPos = song.songPos;
			}
		else
			{
			DrawAll();
			}


		// blit BG "layer" to screen
		//BlitBG();
		//SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
		//SDL_RenderClear(sdlRenderer);
		
		// OPTIONAL - flash pattern bg on beat
		if (beatFlash)
			{
			dest = zones[ZONE_PATGRID];
			//SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 200, 255, 200));
			SDL_SetRenderDrawColor(sdlRenderer, 200, 255, 200, 255);
			SDL_RenderDrawRect(sdlRenderer, &dest);
			}

		// Lock grid cursor to mouse if mouse is in grid
		if (ZONE_PATGRID == currentZone)
			{
			currentStep	= ((int)cursorX - zones[ZONE_PATGRID].x) / PATBOX.w;
			currentTrack = ((int)cursorY - zones[ZONE_PATGRID].y) / PATBOX.h;
			}
		
		// Draw pattern cursor
		if (ZONE_PATGRID == currentZone)
			{
			SetSDLRect(src, 24, 0, PATBOX.w, PATBOX.h);
			SetSDLRect(dest, zones[ZONE_PATGRID].x + currentStep * PATBOX.w, zones[ZONE_PATGRID].y + currentTrack * PATBOX.h, PATBOX.w, PATBOX.h);
			//SDL_BlitSurface(cursorImg, &src, screen, &dest);
			// TODO : Reimplament in Renderer
			//SDL_RenderCopy(sdlRenderer, cursorTex, &src, &dest);
			}

		// Draw playback bar
		// TODO : blit from textures		
		SetSDLRect(dest, zones[ZONE_PATGRID].x + (transport.patternPos * PATBOX.w / 4), zones[ZONE_PATGRID].y, 4, zones[ZONE_PATGRID].h);
		if (0 == global_data)
			SDL_SetRenderDrawColor(sdlRenderer, 0, 255, 0, 255);
		else // CPU struggling!
			SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 0, 255);

		//SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0, 255, 0));
		SDL_RenderDrawRect(sdlRenderer, &dest);

		// Update "joystick mouse" position
		if (ZONE_PATGRID == currentZone)
			{
			cursorX += cursorDX * 5.0f;
			cursorY += cursorDY * 5.0f;
			}
		else
			{
			cursorX += cursorDX * 3.0f;
			cursorY += cursorDY * 3.0f;
			}
		if (cursorX < 0.0) cursorX = 0.0;
		if (cursorX > VIEW_WIDTH - 1) cursorX = VIEW_WIDTH - 1;
		if (cursorY < 0) cursorY = 0;
		if (cursorY > VIEW_HEIGHT -1) cursorY = VIEW_HEIGHT -1;
		// Draw mouse cursor
		renderer->DrawCursor((int)cursorX, (int)cursorY, wavWriter.IsWriting());

		// Draw output sample level
		//SetSDLRect(dest, 72, 68 - (outputSample / 200), 16, outputSample / 200);
		//SDL_SetRenderDrawColor(sdlRenderer, 0, 255, 0, 255);
		//SDL_RenderDrawRect(sdlRenderer, &dest);

		//SDL_Flip(screen);			// waits for vsync
		SDL_RenderPresent(sdlRenderer);
		
		// control update rate of this loop
		//Uint32 time_remaining = 30 - (SDL_GetTicks() - lastTick);
		//if (time_remaining > 0)
		//	SDL_Delay(time_remaining);
		//lastTick = SDL_GetTicks();
		SDL_Delay(20);					// gives time to other threads
        } // wend

	// clean up
	// stop playing and close playback thread
	transport.playing = false;
	// TODO SDL2 - fix
	//SDL_KillThread(playThread);

	if (wavWriter.IsOpen())
		wavWriter.Close();

	// close joystick
	if (joystick)
		{
		SDL_JoystickClose(joystick);
        }

	// close SDL_mixer, then quit SDL
	Mix_CloseAudio();
	SDL_DestroyRenderer(sdlRenderer);
	SDL_Quit();

	return(0);
}
