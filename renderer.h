#pragma once

extern void SetSDLColour(SDL_Colour &colour, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

class Renderer
{
public:
	Renderer(SDL_Renderer* renderer);
	~Renderer(void);

	void ShowSplashScreen();
	void Clear();
	void DrawText(bool big, const char *s, SDL_Rect& rect, bool clip);
	//void DrawSliders(unsigned char vol, unsigned char bpm);
	//void DrawTrackInfo(Song &song, DrumKit &drumkit);
	//void DrawSequenceList(Song &song);
	//void DrawPatternList(Song &song);
	//void DrawPatternGrid(DrumPattern* pattern);
	//void DrawGeneralInfo(const char *songName, const char *drumkitName, Transport &transport);
	void DrawCursor(int x, int y, bool showWavWriterCursor);
	void DrawButton(TEXMAP textureId, int x, int y);
	void DrawButton(TEXMAP textureId, const SDL_Rect &destRect);
	void Blit();
	void DrawRect(const SDL_Rect &rect, const SDL_Colour colour);
	void DrawFilledRect(const SDL_Rect &rect, const SDL_Colour colour);

	// TODO
	// DrawMenuBG
	// DRaw menu border
	// Draw menu separator
	// Draw menu highlight
	// Draw menu text

public:
	SDL_Colour m_bgColour;
	SDL_Colour m_borderColour;
	SDL_Colour m_separatorColour;
	SDL_Colour m_highlightColour;

	// For rendering text
	FontEngine* m_bigFont;
	FontEngine* m_smallFont;

	SDL_Renderer *m_sdlRenderer;

private:
	SDL_Texture *m_guiTex;
	SDL_Texture *m_cursorTex;

};

