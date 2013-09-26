/*
 *      fontengine.cpp
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

#include "SDL.h"
#include "platform.h"
#include "fontengine.h"

/// Construct the font
/// note: parameters MUST match the actual font bitmap used
FontEngine::FontEngine(SDL_Renderer *renderer, const char* fontfile, int font_char_width, int font_char_height)
{
	SDL_Surface *fontSurf = SDL_LoadBMP(fontfile);
	
	if (fontSurf)
		{
		fontCharWidth = font_char_width;
		fontCharHeight = font_char_height;
		fontCharsPerline = fontSurf->w / fontCharWidth;
		printf("FontEngine created using font image %s.\n", fontfile);

		fontTex = SDL_CreateTextureFromSurface(renderer, fontSurf);
		SDL_assert(fontTex);
		SDL_FreeSurface(fontSurf);
		}
	else
		{
		fontCharWidth = 0;
		fontCharHeight = 0;
		fontCharsPerline = 0;
		printf("FontEngine: Failed to load font image!\n");
		}
}



void FontEngine::DrawGlyph(SDL_Renderer* renderer, char c, int destx, int desty)
{
	if (fontTex)
		{
		// get src rect of character
		SDL_Rect src;
		src.x = (c % fontCharsPerline) * fontCharWidth;
		src.y = (c / fontCharsPerline) * fontCharHeight;
		src.w = fontCharWidth;
		src.h = fontCharHeight;
		// blit character to dest rect
		SDL_Rect dest;
		dest.x = destx;
		dest.y = desty;
		dest.w = fontCharWidth;
		dest.h = fontCharHeight;
		//SDL_BlitSurface(fontImg, &src, surface, &dest);
		SDL_RenderCopy(renderer, fontTex, &src, &dest);
		}
}

/// Draw text to the specified surface
/// @param surface			SDL surface to draw text to
/// @param s				Character string to draw
/// @param rect				Destination rectangle for the text
/// @param clip				If true, clip the text output to the destination rect
void FontEngine::DrawText(SDL_Renderer* renderer, const char *s, SDL_Rect& rect, bool clip)
{
	if (fontTex)
		{
		if (clip)
			//SDL_SetClipRect(surface, &rect);
			SDL_RenderSetClipRect(renderer, &rect);
			
		int destx = rect.x;
		const char* p = s;
		while (*p)
			{
			DrawGlyph(renderer, *p, destx, rect.y);
			destx += fontCharWidth;
			p++;	
			}

		if (clip)
			//SDL_SetClipRect(surface, NULL);
			SDL_RenderSetClipRect(renderer, NULL);
		}
}
