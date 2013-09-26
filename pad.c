// Soledad Penades http://soledadpenades.com
#include <stdlib.h>
#include "SDL.h"
#include "SDL_main.h"
#include "SDL_mixer.h"

#ifdef PSP
	#include <pspdebug.h>
	#define printf pspDebugScreenPrintf
#else
	#include <stdio.h>
#endif

#define W 480
#define H 272

#define NUM_TRACKS 8

SDL_Surface *screen;
SDL_Surface *textures;
SDL_Surface *cursorImg;

void printJoystickInfo(SDL_Joystick *joystick)
{
	int index;

	index = SDL_JoystickIndex(joystick);

	printf( "JOYSTICK INFO\n\n"
		            "Index: %d\n"
		            "Name: %s\n"
		            "Num axes: %d\n"
		            "Num balls: %d\n"
		            "Num hats: %d\n"
		            "Num buttons: %d\n",
		            index,
		            SDL_JoystickName(index),
		            SDL_JoystickNumAxes(joystick),
		            SDL_JoystickNumBalls(joystick),
		            SDL_JoystickNumHats(joystick),
		            SDL_JoystickNumButtons(joystick)
	);
}

/*
/// Load the required graphics "textures"
int InitImages()
{
	textures = SDL_LoadBMP("gfx/textures.bmp");
	return 0;
}
*/

// draw vol/bpm/pitch sliders
void DrawSliders(SDL_Surface* surface)
{
	SDL_Rect dest;
	dest.x = 0;
	dest.y = 0;
	dest.w = 96;
	dest.h = 80;
	SDL_Rect src;
	src.x = src.y = 0;
	src.w = 96;
	src.h = 80;
	SDL_BlitSurface(textures, &src, surface, &dest);
}

// draw vol/bpm/pitch sliders
void DrawTrackInfo(SDL_Surface* surface)
{
	SDL_Rect src;
	src.x = 0;
	src.y = 128;
	src.w = 96;
	src.h = 24;
	SDL_Rect dest;
	int i;
	for (i = 0; i < NUM_TRACKS; i++)
		{
		dest.x = 0;
		dest.y = 80 + i*24;
		dest.w = 96;
		dest.h = 24;
		SDL_BlitSurface(textures, &src, surface, &dest);
		}
}

// draw vol/bpm/pitch sliders
void DrawCursor(int x, int y)
{
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = 16;
	src.h = 16;
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = 16;
	dest.h = 16;
	SDL_BlitSurface(cursorImg, &src, screen, &dest);
}

int main(int argc, char *argv[])
{
	int audio_rate,audio_channels,
	// set this to any of 512,1024,2048,4096
	// the higher it is, the more FPS shown and CPU needed
	audio_buffers=512;
	Uint16 audio_format;
	Uint32 t;

	int cursorx = (W / 2) * 8192;
	int cursory = (H / 2) * 8192;

	Mix_Music *music;
	int volume=SDL_MIX_MAXVOLUME;

	SDL_Joystick *joystick = NULL;
	SDL_Event event;
	int done = 0;

	// initialize SDL for audio, video and joystick
	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        {
                printf("init error: %s\n", SDL_GetError());
                return 1;
        }


	if((screen = SDL_SetVideoMode(W, H, 32, 0)) == NULL)
        {
                printf("SetVideoMode: %s\n", SDL_GetError());
                return 1;
        }

	//textures = SDL_LoadBMP("gfx/textures.bmp");
	//cursorImg = SDL_LoadBMP("gfx/cursor.bmp");
	textures = SDL_LoadBMP("textures.bmp");
	cursorImg = SDL_LoadBMP("cursor.bmp");

	DrawSliders(screen);
	DrawTrackInfo(screen);
	SDL_Flip(screen);


	// initialize sdl mixer, open up the audio device
	if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,audio_buffers)<0)
        {
                printf("Mix_OpenAudio: %s\n", SDL_GetError());
                return 1;
        }

	// print out some info on the audio device and stream
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
	int bits=audio_format&0xFF;
	printf("Opened audio at %d Hz %d bit %s, %d bytes audio buffer\n", audio_rate,
			bits, audio_channels>1?"stereo":"mono", audio_buffers );


	SDL_Delay(1000);

	Mix_Chunk *sfx = Mix_LoadWAV("R2BEEP.WAV");
	if (!sfx)
        {
                printf("Error loading sample!\n");
        }

	// And play a corresponding sound
	Mix_PlayChannel(0, sfx, 0);

	if(SDL_NumJoysticks())
        {
                joystick = SDL_JoystickOpen(0);
                printJoystickInfo(joystick);
        }
	else
        {
                printf("No joystick detected\n");
        }

	done = 0;
	while(!done)
		{
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				case SDL_JOYBUTTONDOWN:
					printf("Pressed button %d\n", event.jbutton.button);
					break;
                case SDL_JOYAXISMOTION:
					 printf("Axis motion: j index = %d axis = %d value = %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
					if (0 == event.jaxis.axis)
						cursorx += event.jaxis.value;
					if (1 == event.jaxis.axis)
						cursory += event.jaxis.value;
					break;
                case SDL_QUIT:
					done = 1;
					break;
				} // end switch
			}

		//sleep(2);
		DrawCursor(cursorx / 8192, cursory / 8192);
		SDL_Flip(screen);
        }

	if(joystick)
		{
		SDL_JoystickClose(joystick);
        }

	Mix_FreeChunk(sfx);
	Mix_CloseAudio();
	SDL_Quit();

	return(0);
}

