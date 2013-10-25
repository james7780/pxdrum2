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

		Uint8 *pPixel = (Uint8 *)guiSurf->pixels + (256*guiSurf->pitch);	
		Uint32 pixel = *(Uint32 *)pPixel;
		Uint8 r, g, b;
		SDL_GetRGB(pixel, guiSurf->format, &r, &g, &b);
		SetSDLColour(m_bgColour, r, g, b, 255);
		printf("Background colour = %08X\n", m_bgColour);

		//m_borderColour = *((SDL_Colour*)guiSurf->pixels + (272*guiSurf->w));
		pPixel = (Uint8 *)guiSurf->pixels + (272*guiSurf->pitch);	
		pixel = *(Uint32 *)pPixel;
		SDL_GetRGB(pixel, guiSurf->format, &r, &g, &b);
		SetSDLColour(m_borderColour, r, g, b, 255);
		printf("Border colour = %08X\n", m_borderColour);

		pPixel = (Uint8 *)guiSurf->pixels + (288*guiSurf->pitch);	
		pixel = *(Uint32 *)pPixel;
		SDL_GetRGB(pixel, guiSurf->format, &r, &g, &b);
		SetSDLColour(m_separatorColour, r, g, b, 255);
		printf("Separator colour = %08X\n", m_separatorColour);

		pPixel = (Uint8 *)guiSurf->pixels + (304*guiSurf->pitch);	
		pixel = *(Uint32 *)pPixel;
		SDL_GetRGB(pixel, guiSurf->format, &r, &g, &b);
		SetSDLColour(m_highlightColour, r, g, b, 255);
		printf("HIghlight colour = %08X\n", m_highlightColour);

		SDL_UnlockSurface(guiSurf);

		//// TEST
		//SetSDLColour(m_bgColour, 0, 0, 0, 255);
		//SetSDLColour(m_borderColour, 255, 255, 255, 255);
		//SetSDLColour(m_separatorColour, 0, 255, 255, 255);
		//SetSDLColour(m_highlightColour, 255, 255, 0, 255);

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
	SDL_SetRenderDrawColor(m_sdlRenderer, m_bgColour.r, m_bgColour.g, m_bgColour.b, 255);
	SDL_RenderClear(m_sdlRenderer);
}

/// Draw text
/// @param big			Use big font (else use smaller font)
void Renderer::DrawText(const char *s, SDL_Rect& rect, int options)
{
	FontEngine *font = (options & TEXT_BIGFONT) ? m_bigFont : m_smallFont;
	if (font)
		{
		bool clip = (options & TEXT_CLIP) ? true : false;
		bool trans = (options & TEXT_TRANS) ? true : false;
		font->DrawText(m_sdlRenderer, s, rect, clip, trans);
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

/// Draw a piece of a texture from the GUI texmap
/// @param srcRect		
void Renderer::DrawGUITexture(const SDL_Rect &srcRect, const SDL_Rect &destRect)
{
	if (m_guiTex)
		{
		SDL_RenderCopy(m_sdlRenderer, m_guiTex, &srcRect, &destRect);
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
	SDL_SetRenderDrawColor(m_sdlRenderer, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderDrawRect(m_sdlRenderer, &rect);
}

/// Draw a filled rectangle
void Renderer::DrawFilledRect(const SDL_Rect &rect, const SDL_Colour colour)
{
	SDL_SetRenderDrawColor(m_sdlRenderer, colour.r, colour.g, colour.b, colour.a);
	SDL_RenderFillRect(m_sdlRenderer, &rect);
}

/// Fade screen towards the given colour, using the alpha value to blend
void Renderer::Fade(const SDL_Colour &fadeColour)
{
	SDL_Rect rect;
	SetSDLRect(rect, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
	SDL_SetRenderDrawBlendMode(m_sdlRenderer, SDL_BLENDMODE_BLEND);
	this->DrawFilledRect(rect, fadeColour);
	SDL_SetRenderDrawBlendMode(m_sdlRenderer, SDL_BLENDMODE_NONE);
	this->Blit();
}
