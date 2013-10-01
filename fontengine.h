/*
 *      fontengine.h
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

/*
#define FONT_CHARS_PER_LINE	32
#define FONT_CHAR_WIDTH		8
#define FONT_CHAR_HEIGHT	16
#define OUT_CHAR_WIDTH		8
#define OUT_CHAR_HEIGHT		16
*/

class FontEngine
{
public:
	// constructor
	FontEngine(SDL_Renderer *renderer, const char* fontfile, int font_char_width, int font_char_height);
	
	// operations
	void DrawGlyph(SDL_Renderer* renderer, char c, int destx, int desty);
	void DrawText(SDL_Renderer* renderer, const char *s, SDL_Rect& rect, bool clip, bool trans = false);
	int GetFontHeight()
		{
		return fontCharHeight;
		}
private:	
	SDL_Texture *fontTex;
	int fontCharsPerline;
	int fontCharWidth;
	int fontCharHeight;
};
