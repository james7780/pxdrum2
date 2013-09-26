// PXDrum Portable Drum Machine
// Copyright James Higgs 2008/2009
//
/*
 *      renderer.cpp - Handles drawing of graphics for pxdrum
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

//#include <stdlib.h>
#include "SDL.h"
#include "platform.h"
#include "zones.h"
#include "texmap.h"
#include "pattern.h"
#include "song.h"
#include "drumkit.h"
#include "transport.h"
#include "fontengine.h"
#include "Renderer.h"

// extern - Set up an SDL colour
void SetSDLColour(SDL_Colour &colour, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	colour.r = r;
	colour.g = g;
	colour.b = b;
	colour.a = a;
}

// Helper function
/// Load a bitmap image and convert it to display format
SDL_Surface* LoadImageConvertToDisplay(const char* filename, bool setColourKey)
{
	// Load the bitmap
	SDL_Surface* img = SDL_LoadBMP(filename);
	if (!img)
		{
		printf("Error loading image %s!\n", filename);
		return NULL;
		}

	// set colour key if neccessary
	if (setColourKey)
		{
		Uint32 colourKey = SDL_MapRGB(img->format, 0, 0, 0);		
		if (-1 == SDL_SetColorKey(img, SDL_TRUE, colourKey))
			{
			printf("Error setting cursor colour key!\n");
			SDL_FreeSurface(img);
			return NULL;
			}
		}

	// Convert bitmap to display format
	// NB: For PSP, colour key will not work if bitmap is in display format
	// (ie: We need to "force" software colourkey for the cursor bitmap)
	SDL_Surface* displayImg = img;
	if (!setColourKey)
		{
			// TODO SDL2 - fix
//		displayImg = SDL_DisplayFormat(img);
//		SDL_FreeSurface(img);
		}

	return displayImg;	
}

Renderer::Renderer(SDL_Renderer* renderer)
{
	// TODO : ENSURE(renderer) (or throw exception)
	SDL_assert(renderer);
	m_sdlRenderer = renderer;

	// Load gui texture and resolve ui colours
	SDL_Surface *guiSurf = LoadImageConvertToDisplay("gfx/textures512.bmp", false);
	if (guiSurf)
		{
		// background, border, separator, highlight colours for gui come from textures.bmp
		SDL_LockSurface(guiSurf);
		m_bgColour = *((SDL_Colour*)guiSurf->pixels + 128 + (188*guiSurf->w));
		printf("Background colour = %08X\n", m_bgColour);
		m_borderColour = *((SDL_Colour*)guiSurf->pixels + 128 + (196*guiSurf->w));
		printf("Border colour = %08X\n", m_borderColour);
		m_separatorColour = *((SDL_Colour*)guiSurf->pixels + 128 + (204*guiSurf->w));
		printf("Separator colour = %08X\n", m_separatorColour);
		m_highlightColour = *((SDL_Colour*)guiSurf->pixels + 128 + (212*guiSurf->w));
		printf("HIghlight colour = %08X\n", m_highlightColour);
		SDL_UnlockSurface(guiSurf);

		// TEST
		SetSDLColour(m_bgColour, 0, 0, 0, 255);
		SetSDLColour(m_borderColour, 255, 255, 255, 255);
		SetSDLColour(m_separatorColour, 0, 255, 255, 255);
		SetSDLColour(m_highlightColour, 255, 255, 0, 255);

		// Copy this surface to a texture
		m_guiTex = SDL_CreateTextureFromSurface(m_sdlRenderer, guiSurf);
		SDL_assert(m_guiTex);
		SDL_FreeSurface(guiSurf);
		}
	else
		{
 		printf("Error loading texture gfx!\n");
		}

	// Cursors come from cursor.bmp
	SDL_Surface * cursorSurf = LoadImageConvertToDisplay("gfx/cursor.bmp", true);
	if (cursorSurf)
		{
		// Copy this surface to a texture
		m_cursorTex = SDL_CreateTextureFromSurface(m_sdlRenderer, cursorSurf);
		SDL_assert(m_cursorTex);
		SDL_FreeSurface(cursorSurf);
		}
	else
		{
 		printf("Error loading cursor gfx!\n");
		}

	// init fonts
	m_bigFont = new FontEngine(m_sdlRenderer, "gfx/font_16x32.bmp", 16, 32);
	if (!m_bigFont)
		printf("Error - unable to init big font!\n");

	m_smallFont = new FontEngine(m_sdlRenderer, "gfx/font_8x16.bmp", 8, 16);
	if (!m_smallFont)
		printf("Error - unable to init small font!\n");

}

Renderer::~Renderer(void)
{
}

/// Show the splash screen
void Renderer::ShowSplashScreen()
{
	SDL_Surface *splashSurf = LoadImageConvertToDisplay("gfx/bg.bmp", false);
	if (splashSurf)
		{
		SDL_Texture *texture = SDL_CreateTextureFromSurface(m_sdlRenderer, splashSurf);
		SDL_RenderClear(m_sdlRenderer);
		SDL_RenderCopy(m_sdlRenderer, texture, NULL, NULL);
		SDL_RenderPresent(m_sdlRenderer);

		SDL_FreeSurface(splashSurf);
		SDL_DestroyTexture(texture);
		}
}

void Renderer::Clear()
{
	// clear background
	//SDL_FillRect(backImg, NULL, g_bgColour); //SDL_MapRGB(screen->format, 0, 0, 0));
	SDL_SetRenderDrawColor(m_sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(m_sdlRenderer);
}

/// Draw text
/// @param big			Use big font (else use smaller font)
void Renderer::DrawText(bool big, const char *s, SDL_Rect& rect, bool clip)
{
	FontEngine *font = big ? m_bigFont : m_smallFont;
	if (font)
		{
		font->DrawText(m_sdlRenderer, s, rect, clip);
		}

}
	
// draw cursor
void Renderer::DrawCursor(int x, int y, bool showWavWriterCursor)
{
	if (!m_cursorTex)
		return;

	SDL_Rect src;
	SDL_Rect dest;

	if (showWavWriterCursor)
		{
		SetSDLRect(src, 64, 0, 32, 24);
		SetSDLRect(dest, x, y, 32, 24);
		}
	else
		{
		SetSDLRect(src, 0, 0, 16, 16);
		SetSDLRect(dest, x, y, 16, 16);
		}

	//SDL_BlitSurface(cursorImg, &src, screen, &dest);
	SDL_RenderCopy(m_sdlRenderer, m_cursorTex, &src, &dest);
}

/// Draw a button from the GUI texmap
void Renderer::DrawButton(TEXMAP textureId, int x, int y)
{
	if (m_guiTex && textureId < TM_COUNT)
		{
		SDL_Rect src = texmap[textureId];
		SDL_Rect dest;
		SetSDLRect(dest, x, y, src.w, src.h);
		SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &dest);
		}
}

/// Draw a button from the GUI texmap
void Renderer::DrawButton(TEXMAP textureId, const SDL_Rect &destRect)
{
	if (m_guiTex && textureId < TM_COUNT)
		{
		SDL_Rect src = texmap[textureId];
		SDL_RenderCopy(m_sdlRenderer, m_guiTex, &src, &destRect);
		}
}

/// "Present" the SDL screen
void Renderer::Blit()
{
	SDL_RenderPresent(m_sdlRenderer);
}

/// Draw a rectangle outline
void Renderer::DrawRect(const SDL_Rect &rect, const SDL_Colour colour)
{
	SDL_SetRenderDrawColor(m_sdlRenderer, colour.r, colour.g, colour.b, 255);
	SDL_RenderDrawRect(m_sdlRenderer, &rect);
}

/// Draw a filled rectangle
void Renderer::DrawFilledRect(const SDL_Rect &rect, const SDL_Colour colour)
{
	SDL_SetRenderDrawColor(m_sdlRenderer, colour.r, colour.g, colour.b, 255);
	SDL_RenderFillRect(m_sdlRenderer, &rect);
}
