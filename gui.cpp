/*
 *      gui.cpp
 *      
 *      Copyright 2009-2013 james <james@okami>
 *      
 */

#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h> 			// for listing directory
#endif
#include <stdio.h> 
#include "SDL.h"
#include "platform.h"
#include "fontengine.h"
#include "joymap.h"
#include "texmap.h"
#include "renderer.h"
#include "gui.h"

extern JoyMap joyMap;


///////////////////////////////////////////////////////////////////////////////
// MenuItem class
///////////////////////////////////////////////////////////////////////////////

/// Setup a menu item
void MenuItem::SetItem(int id, const char* itemText, const char* description)
{
	m_id = id;
	strncpy(m_itemText, itemText, MAX_ITEMTEXT_LEN);
	m_itemText[MAX_ITEMTEXT_LEN-1] = 0;
	strncpy(m_description, description, MAX_DESCRIPTION_LEN);
	m_description[MAX_DESCRIPTION_LEN-1] = 0;
}

/// Setup a menu item (with option list)
void MenuItem::SetItem(int id, const char* itemText, const char* optionsText, int selectedOption, const char* description)
{
	m_id = id;
	strncpy(m_itemText, itemText, MAX_ITEMTEXT_LEN);
	m_itemText[MAX_ITEMTEXT_LEN-1] = 0;
	
	strncpy(m_optionsText, optionsText, MAX_OPTIONSTEXT_LEN);
	m_optionsText[MAX_OPTIONSTEXT_LEN-1] = 0;
	m_selectedOption = selectedOption;
	
	strncpy(m_description, description, MAX_DESCRIPTION_LEN);
	m_description[MAX_DESCRIPTION_LEN-1] = 0;
}

/// Get the nth option in an option list menu item
/// @return					Length of option text at index index, or 0 if none
int MenuItem::GetOptionText(int index, char* buffer)
{
	const char* pc = m_optionsText;
	int len = (int)strlen(m_optionsText);
	if (0 == len)
		{
		//printf("GetOptionText(%d) - zero length!\n", index);
		return 0;			// no options
		}
			
	// find start pos
	for (int i = 0; i < index; i++)
		{
		// find next seperator ('|' character)
		while (*pc != '|' && ((pc - m_optionsText) < len))
			{
			pc++;
			}
		if (pc >= m_optionsText + len)
			break;		// not found 
			
		pc++;
		}

	// did we run out of options?	
	if (pc >= m_optionsText + len)
		{
		printf ("Menu index %d too big!\n", index);
		return 0;			// not found
		}
		
	// copy to buffer until '|' or 0'
	char* output = buffer;
	while (*pc != '|' && ((pc - m_optionsText) < len))
		{
		*output++ = *pc++;
		}

	// terminate output buffer	 	
	*output = 0;

	//printf("GetOptionText(%d) - result = '%s'\n", index, buffer);
	
	return (int)strlen(buffer);
}

// Get currently selected option in this menu item
int MenuItem::GetSelectedOption()
{
	return m_selectedOption;	
}

// Set currently selected option in this menu item
void MenuItem::SetSelectedOption(int index)
{
	// validate
	char optionText[MAX_OPTIONSTEXT_LEN];
	if (GetOptionText(index, optionText) > 0)
		m_selectedOption = index;
	else
		m_selectedOption = 0;
}


///////////////////////////////////////////////////////////////////////////////
// Menu class
///////////////////////////////////////////////////////////////////////////////

/// Add a menu item to the menu
int Menu::AddItem(int id, const char* itemText, const char* description)
{
	if (m_numItems < MAX_MENU_ITEMS)
		{
		m_menuItems[m_numItems].SetItem(id, itemText, description);
		m_numItems++;	
		}
		
	return -1;			// could not add!	
}

/// Add a menu item to the menu
int Menu::AddItem(int id, const char* itemText, const char* optionsText, int selectedOption, const char* description)
{
	if (m_numItems < MAX_MENU_ITEMS)
		{
		m_menuItems[m_numItems].SetItem(id, itemText, optionsText, selectedOption, description);
		m_numItems++;	
		}
		
	return -1;			// could not add!	
}


extern void SetSDLRect(SDL_Rect& rect, int x, int y,int w, int h);

#define MENU_TITLE_X		32
#define MENU_ITEM_X			64
#define MENU_ITEM_START_Y	80

/// Display the menu and get chosen item from user
/// @return		id of item selected, or -1 if cancelled
int Menu::DoMenu(Renderer *renderer, SDL_Rect* extentsIn, const char* title, int initialSelection)
{
	if (!renderer || !title || 0 == m_numItems)
		{
		printf("Error: DoMenu - bad parameter!\n");
		return -1;
		}

	// Fade background
	SDL_Colour fadeColour;
	SetSDLColour(fadeColour, 0, 0, 0, 192);
	renderer->Fade(fadeColour);

	int itemHeight = (3 * renderer->m_bigFont->GetFontHeight()) / 2;

	// Calculate extents if not supplied
	SDL_Rect extents;
	if (extentsIn)
		{
		extents = *extentsIn;
		}
	else
		{
		int border = VIEW_WIDTH / 20;
		SetSDLRect(extents, border, border, VIEW_WIDTH - border * 2, 0);
		}

	// Also calculate height automatically if set to 0
	if (0 == extents.h)
		extents.h = itemHeight * (m_numItems + 4);

	int contentWidth = extents.w - (MENU_TITLE_X*2);

	SDL_Rect okButtonRect, cancelButtonRect;
	SetSDLRect(okButtonRect, extents.x + extents.w - MENU_TITLE_X - 48 * 2 - 8, extents.y + 6, 48, 48);
	SetSDLRect(cancelButtonRect, extents.x + extents.w - MENU_TITLE_X - 48, extents.y + 6, 48, 48);

	// handle initial selection
	int selected = 0;
	if (initialSelection >= 0 && initialSelection < m_numItems)
		selected = initialSelection;


	SDL_Event event;
	unsigned short keyCode;
	int mx = 0;
	int my = 0;
	bool done = false;
	while(!done)
		{
		bool upPressed = false;
		bool downPressed = false;
		bool leftPressed = false;
		bool rightPressed = false;
		bool escapePressed = false;
		bool selectPressed = false;
	
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					// controller buttons are mapped according to joymap.cfg file,
					// which maps controller buttons to SDL/ASCII keys
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (SDLK_ESCAPE == keyCode)
						escapePressed = true;
					else if (LCLICK == keyCode)
						selectPressed = true;
					else if (SDLK_UP == keyCode)
						upPressed = true;
					else if (SDLK_DOWN == keyCode)
						downPressed = true;
					else if (SDLK_LEFT == keyCode)
						leftPressed = true;
					else if (SDLK_RIGHT == keyCode)
						rightPressed = true;
/*					
					if(event.jbutton.button == 0) // TRIANGLE
						escapePressed = true;
					else if(event.jbutton.button == 2) // CROSS
						selectPressed = true;
					else if(event.jbutton.button == 6) // DOWN
						downPressed = true;
					else if(event.jbutton.button == 8) // UP
						upPressed = true;
					else if(event.jbutton.button == 7) // LEFT
						leftPressed = true;
					else if(event.jbutton.button == 9) // RIGHT
						rightPressed = true;
*/						
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					//SDL_Delay(1000);
					if (SDLK_UP == event.key.keysym.sym)
						upPressed = true;
					else if (SDLK_DOWN == event.key.keysym.sym)
						downPressed = true;
					else if (SDLK_LEFT == event.key.keysym.sym)
						leftPressed = true;
					else if (SDLK_RIGHT == event.key.keysym.sym)
						rightPressed = true;
					else if (SDLK_RETURN == event.key.keysym.sym)
						selectPressed = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						selectPressed = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						escapePressed = true;
					break;
				// For systems with mouses/trackballs
				case SDL_MOUSEMOTION:
					mx = event.motion.x;
					my = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (1 == event.button.button)		// Left mouse button
						{
						mx = event.motion.x;
						my = event.motion.y;
						// Clicked OK or Cancel?
						if (my < (okButtonRect.y + okButtonRect.h))
							{
							if (mx > okButtonRect.x && mx < okButtonRect.x + okButtonRect.w)
								selectPressed = true;
							else if (mx > cancelButtonRect.x && mx < cancelButtonRect.x + cancelButtonRect.w)
								escapePressed = true;
							}
						else
							{
							// Select item, or change item option?
							int clickedItem = (event.motion.y - extents.y - MENU_ITEM_START_Y) / itemHeight;
							if (clickedItem > -1 && clickedItem < m_numItems)
								{
								selected = clickedItem;
								char optionText[MAX_OPTIONSTEXT_LEN];
								if (0 == m_menuItems[clickedItem].GetOptionText(0, optionText))
									{
									// select item and exit
									done = true;
									}
								else
									{
									// update item's option
									if (event.motion.x > VIEW_WIDTH/2)
										{
										if (event.motion.x < (VIEW_WIDTH/2 + 24))
											leftPressed = true;
										else
											rightPressed = true;
										}
									}
								}
							}
						}
					break;
				default :
					break;
				} // end switch
			} // wend pollevent

		// react to keypresses
		if (escapePressed)
			{
			selected = -1;
			done = true;
			}
		else if (selectPressed)
			{
			done = true;
			}
		else if (upPressed)
			{
			if (selected > 0)
				selected--;	
			}
		else if (downPressed)
			{
			if (selected < (m_numItems - 1))
				selected++;	
			}
		else if (leftPressed)
			{
			int selectedOption = m_menuItems[selected].GetSelectedOption();
			if (selectedOption > 0)
				m_menuItems[selected].SetSelectedOption(selectedOption - 1);
			else
				m_menuItems[selected].SetSelectedOption(0);
				
			//printf("After left, selopt = %d\n", m_menuItems[selected].GetSelectedOption());
			}
		else if (rightPressed)
			{
			int selectedOption = m_menuItems[selected].GetSelectedOption();
			char buffer[MAX_OPTIONSTEXT_LEN];
			if (m_menuItems[selected].GetOptionText(selectedOption + 1, buffer) > 0)
				selectedOption++;
			m_menuItems[selected].SetSelectedOption(selectedOption);
			
			//printf("After right, selopt = %d\n", m_menuItems[selected].GetSelectedOption());
			}
			
		// Redraw screen
		// Clear menu background & draw border
		SDL_Rect rect = extents;
		renderer->DrawFilledRect(rect, renderer->m_bgColour);
		rect.h = 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.y += extents.h - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		SetSDLRect(rect, extents.x, extents.y, 4, extents.h);
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.x = extents.x + extents.w - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		
		// Draw title
		SetSDLRect(rect, extents.x + MENU_TITLE_X, extents.y + 8, contentWidth, itemHeight);
		renderer->DrawText(title, rect, Renderer::TEXT_BIGFONT);

		renderer->DrawButton(TM_OK_BTN, okButtonRect);
		renderer->DrawButton(TM_CANCEL_BTN, cancelButtonRect);

		SetSDLRect(rect, extents.x + MENU_TITLE_X, extents.y + 8 + itemHeight, contentWidth, 2);
		renderer->DrawFilledRect(rect, renderer->m_separatorColour);

		// Draw items
		for (int i = 0; i < m_numItems; i++)
			{
			// draw item text
			SetSDLRect(rect, extents.x + MENU_ITEM_X, extents.y + MENU_ITEM_START_Y + (i * itemHeight), 0, 0);
			renderer->DrawText(m_menuItems[i].GetItemText(), rect, Renderer::TEXT_BIGFONT);

			// if this is an option item, then draw currently selected option
			// with arrow markers before and after the text
			char optionText[MAX_OPTIONSTEXT_LEN];
			optionText[0] = 126;
			optionText[1] = 32;
			int selectedOption = m_menuItems[i].GetSelectedOption();
			if (m_menuItems[i].GetOptionText(selectedOption, optionText+2) > 0)
				{
				int len = (int)strlen(optionText);
				optionText[len] = 32;
				optionText[len+1] = 127;
				optionText[len+2] = 0;		// terminate
				SetSDLRect(rect, VIEW_WIDTH/2, extents.y + MENU_ITEM_START_Y + (i * itemHeight), 0, 0);
				renderer->DrawText(optionText, rect, Renderer::TEXT_BIGFONT);
				}
			}
	
		// Draw description of selected item
		if (selected >= 0 && selected < m_numItems)
			{
			SetSDLRect(rect, extents.x + MENU_TITLE_X, extents.y + extents.h - itemHeight, 0, 0);
			renderer->DrawText(m_menuItems[selected].GetDescription(), rect, Renderer::TEXT_BIGFONT);
			}

		// Draw selection highlight
		int y1 = extents.y + MENU_ITEM_START_Y + (selected * itemHeight) - 4;
		SetSDLRect(rect, extents.x + MENU_TITLE_X, y1, contentWidth, itemHeight - 1);
		renderer->DrawRect(rect, renderer->m_highlightColour);

		// Draw mouse cursor
		SDL_ShowCursor(SDL_ENABLE);
			
		renderer->Blit();

		SDL_Delay(20);					// gives time to other threads
		} // wend done
	
	// return id of selected item, or 
	int selectedId = -1;
	if (selected >= 0 && selected < m_numItems)
		selectedId = m_menuItems[selected].GetId();

	//printf("DoMenu: selected id = %d\n", selectedId);
		
	return selectedId;
}

/// Get which option was selected in an option item in this menu
/// @param id		Menu item id
/// @return			Selected option index for the specified menu item (if it's an option item) 
int Menu::GetItemSelectedOption(int id)
{
	// find item with specified id
	for (int i = 0; i < m_numItems; i++)
		{
		if (id == m_menuItems[i].GetId())
			{
			return m_menuItems[i].GetSelectedOption();
			}
		}

	//printf ("GetItemSelectedOption(%d) - id not found!\n", id);
	return 0;
}	




// Implementation of global GUI functions

/// Show message to user
/// @param title		Caption (title) text
/// @param msg			Text to show user (can be \n separated list of items)
/// @param confirm		If true, then act like a confirm dialog - confirm user action (eg delete, overwrite) 
bool DoMessage(Renderer *renderer, const char *title, const char *msg, bool confirm)
{
	if (!renderer || !msg)
		{
		printf("Error: DoMessage - bad parameter!\n");
		return false;
		}

	// Fade background
	SDL_Colour fadeColour;
	SetSDLColour(fadeColour, 0, 0, 0, 128);
	renderer->Fade(fadeColour);

	int border = VIEW_WIDTH / 20;
	SDL_Rect extents;
	SetSDLRect(extents, border, border, VIEW_WIDTH - border * 2, VIEW_HEIGHT - border * 2);

	// split input string into lines
	char *textLine[20];
	int numLines = 0;
	char s[2000];
	strcpy(s, msg);
	//printf("Text = %s\n", s);
	char *nextLine = strtok(s, "\n");
	while (nextLine)
		{
		//printf("Line %d: %s\n", numLines, nextLine);
		textLine[numLines++] = nextLine;
		nextLine = strtok(NULL, "\n");
		}
	// mark end of text lines
	textLine[numLines] = 0;

	SDL_Rect okButtonRect, cancelButtonRect;
	SetSDLRect(okButtonRect, extents.x + extents.w - MENU_TITLE_X - 48 * 2 - 8, extents.y + 6, 48, 48);
	SetSDLRect(cancelButtonRect, extents.x + extents.w - MENU_TITLE_X - 48, extents.y + 6, 48, 48);

	SDL_ShowCursor(SDL_ENABLE);

	bool confirmYes = false;

	int itemHeight = (3 * renderer->m_bigFont->GetFontHeight()) / 2;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	while(!done)
		{
	
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (LCLICK == keyCode)
						confirmYes = true;
					else if (SDLK_ESCAPE == keyCode)
						done = true;
/*					
					//if(event.jbutton.button == 0) // TRIANGLE
					//	escapePressed = true;
					if(event.jbutton.button == 2) // CROSS
						done = true;
*/ 
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					if (SDLK_RETURN == event.key.keysym.sym)
						confirmYes = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						confirmYes = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						done = true;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (1 == event.button.button)		// Left mouse button
						{
						Sint32 mx = event.motion.x;
						Sint32 my = event.motion.y;
						// Clicked OK or Cancel?
						if (my < (okButtonRect.y + okButtonRect.h))
							{
							if (mx > okButtonRect.x && mx < okButtonRect.x + okButtonRect.w)
								confirmYes = true;
							else if (mx > cancelButtonRect.x && mx < cancelButtonRect.x + cancelButtonRect.w)
								done = true;
							}
						}
					break;
				} // end switch
			} // wend pollevent

		if (confirmYes)
			done = true;

		// Redraw screen
		// Clear message background & draw border
		rect = extents;
		renderer->DrawFilledRect(rect, renderer->m_bgColour);
		rect.h = 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.y = extents.y + extents.h - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		SetSDLRect(rect, extents.x, extents.y, 4, extents.h);
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.x = extents.x + extents.w - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);

		int contentWidth = extents.w - (MENU_TITLE_X*2);
				
		// Draw title
		SetSDLRect(rect, extents.x + MENU_TITLE_X, extents.y + 8, contentWidth, itemHeight);
		renderer->DrawText(title, rect, Renderer::TEXT_BIGFONT);

		SetSDLRect(rect, extents.x + MENU_TITLE_X, extents.y + 8 + itemHeight, contentWidth, 2);
		renderer->DrawFilledRect(rect, renderer->m_separatorColour);

		// Draw OK/Cancel buttons
		renderer->DrawButton(TM_OK_BTN, okButtonRect);
		renderer->DrawButton(TM_CANCEL_BTN, cancelButtonRect);
		
		// Draw message items
		int y = extents.y + MENU_ITEM_START_Y;
		for (int i = 0; i < numLines; i++)
			{
			SetSDLRect(rect, extents.x + MENU_ITEM_X, y, contentWidth, itemHeight);
			renderer->DrawText(textLine[i], rect, Renderer::TEXT_BIGFONT);
			y += itemHeight;
			}

		// Draw instructions
		SetSDLRect(rect, extents.x + MENU_TITLE_X, extents.y + extents.h - itemHeight, 0, 0);
		if (confirm)
			{
#ifdef PSP				
			font->DrawText(surface, "X = Yes, ^ = No", rect, false);
#else
			renderer->DrawText("ENTER = Yes, ESC = No", rect, Renderer::TEXT_BIGFONT);
#endif
			}
		else
			{
#ifdef PSP				
			font->DrawText(surface, "Press X to continue", rect, false);
#else
			renderer->DrawText("Press ENTER to continue", rect, Renderer::TEXT_BIGFONT);
#endif
			}

		//SDL_RenderPresent(renderer);
		renderer->Blit();
		SDL_Delay(20);					// gives time to other threads
		} // wend done

	SDL_ShowCursor(SDL_DISABLE);
	
	return confirmYes;	
}

// virtual key grid position to ascii char map
static char virtualKeyMap[40] = {
	'1', '2', '3', '4', '5', '6', '7','8', '9', '0',
	'q', 'w', 'e', 'r', 't', 'y', 'u','i', 'o', 'p',
	'a', 's', 'd', 'f', 'g', 'h', 'j','k', 'l', 8,
	'z', 'x', 'c', 'v', 'b', 'n', 'm',' ', '[', ']'
};

/// Edit text string
/// @param text			Text to edit
/// @param maxlen		Maximum length of the input/output text
/// @return				false if cancelled, else true
bool DoTextInput(Renderer *renderer, const char* prompt, char* text, int maxlen)
{
	if (!renderer || !prompt || !text)
		return false;

	// Load the virtual keyboard bitmaps
	SDL_Surface* keyboardImg = SDL_LoadBMP("gfx/vk_medium.bmp");
	if (!keyboardImg)
		{
		printf("Error loading keyboard image %s!\n", "gfx/vk_medium.bmp");
		return false;
		}

	SDL_Surface* keyboardHiliteImg = SDL_LoadBMP("gfx/vk_medium_hilite.bmp");
	if (!keyboardHiliteImg)
		{
		printf("Error loading keyboard image %s!\n", "gfx/vk_medium_hilite.bmp");
		return false;
		}

	SDL_Texture *keyTexture = SDL_CreateTextureFromSurface(renderer->m_sdlRenderer, keyboardImg);
	SDL_Texture *keyTextureHilite = SDL_CreateTextureFromSurface(renderer->m_sdlRenderer, keyboardHiliteImg);

	// Virtual keyboard extents/position
	int vkWidth = keyboardImg->w;
	int vkHeight = keyboardImg->h;
	int vkKeyWidth = vkWidth / 10;
	int vkKeyHeight = vkHeight / 4;
	SDL_Rect vkDestRect;
	SetSDLRect(vkDestRect, (VIEW_WIDTH - vkWidth) / 2, VIEW_HEIGHT / 2, vkWidth, vkHeight);

	SDL_Rect okButtonRect, cancelButtonRect;
	SetSDLRect(okButtonRect, VIEW_WIDTH - MENU_TITLE_X - 48 * 2 - 8, 6, 48, 48);
	SetSDLRect(cancelButtonRect, VIEW_WIDTH - MENU_TITLE_X - 48, 6, 48, 48);

	SDL_ShowCursor(SDL_ENABLE);

	int charPos = (int)strlen(text);
	int vkPos = 0;				// posn in virtual kb
	int itemHeight = (3 * renderer->m_bigFont->GetFontHeight()) / 2;
	int counter = 0;
	SDL_Rect src;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	bool escapePressed = false;
	while(!done)
		{
		int dx = 0;
		int dy = 0;
		bool addChar = false;
		keyCode = 0;
		bool backspace = false;
		
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					{
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					unsigned short keyPressed = joyMap.GetValueAt(event.jbutton.button);
					if (LCLICK == keyPressed)
						addChar = true;
					else if (SDLK_ESCAPE == keyPressed)
						escapePressed = true;
					if (SDLK_SPACE == keyPressed)
						done = true;
					else if ('R' == keyPressed)			// "backspace" [SQUARE button on PS controllers]
						backspace = true;
					else if (SDLK_LEFT == keyPressed)
						dx = -1;
					else if (SDLK_RIGHT == keyPressed)
						dx = 1;
					else if (SDLK_UP == keyPressed)
						dy = -1;
					else if (SDLK_DOWN == keyPressed)
						dy = 1;
					keyCode = 0;		// prevent adding to text
					}
					break;
				case SDL_KEYDOWN:
					{
					printf("Key pressed: %d\n", event.key.keysym.sym);
					SDL_Keycode keyPressed = event.key.keysym.sym;
					if (SDLK_RETURN == keyPressed)
						{
						done = true;
						}
					else if (SDLK_BACKSPACE == keyPressed)
						backspace = true;
					else if (SDLK_ESCAPE == keyPressed)
						escapePressed = true;

					if ((keyPressed >= SDLK_0 && keyPressed <= SDLK_9) ||
							 (keyPressed >= SDLK_a && keyPressed <= SDLK_z) ||
							 keyPressed == SDLK_UNDERSCORE ||
							 keyPressed == SDLK_SPACE)
						{
						// Add typed character
						keyCode = keyPressed;
						}
					else
						{
						keyCode = 0;		// non-valid key typed
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (1 == event.button.button)		// Left mouse button
						{
						Sint32 mx = event.motion.x;
						Sint32 my = event.motion.y;
						// Clicked OK or Cancel?
						if (my < (okButtonRect.y + okButtonRect.h))
							{
							if (mx > okButtonRect.x && mx < okButtonRect.x + okButtonRect.w)
								done = true;
							else if (mx > cancelButtonRect.x && mx < cancelButtonRect.x + cancelButtonRect.w)
								escapePressed = true;
							}
						else if (mx > vkDestRect.x && mx < vkDestRect.x + vkDestRect.w &&
								my > vkDestRect.y && my < vkDestRect.y + vkDestRect.h)
							{
							// Clicked on virtual key
							int x0 = (mx - vkDestRect.x) / vkKeyWidth;
							int y0 = (my - vkDestRect.y) / vkKeyHeight;
							vkPos = x0 + (y0 * 10);
							addChar = true;

							// Flash highlight clicked virtual button
							SetSDLRect(src, x0*vkKeyWidth, y0*vkKeyHeight, vkKeyWidth, vkKeyHeight);
							SetSDLRect(rect, vkDestRect.x + x0*vkKeyWidth, vkDestRect.y + y0*vkKeyHeight, vkKeyWidth, vkKeyHeight);
							SDL_RenderCopy(renderer->m_sdlRenderer, keyTextureHilite, &src, &rect);
							renderer->Blit();
							SDL_Delay(20);
							}
						}
					break;
				} // end switch
			} // wend pollevent

		// Respond to input
		if (escapePressed)
			break;

		// Validate if done
		if (done)
			{
			if (0 == text[0])
				done = false;			// No empty names please
			}

		if (addChar)
			{
			// append currently highlighted char to the text string
			char mappedChar = virtualKeyMap[vkPos];
			if (8 == mappedChar)
				{
				backspace = true;
				}
			else if (charPos < (maxlen - 1))
				{
				text[charPos++] = mappedChar;
				text[charPos] = 0;
				}
			}
		else if (keyCode > 0)
			{
			// Add typed keycode
			if (charPos < (maxlen - 1))
				{
				text[charPos++] = (char)keyCode;
				text[charPos] = 0;
				}
			}

		if (backspace)
			{
			// remove previous char
			if (charPos > 0)
				{
				charPos--;
				text[charPos] = 0;
				}
			}
			
		// Redraw screen
		// Clear menu background & draw border
		SetSDLRect(rect, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
		renderer->DrawFilledRect(rect, renderer->m_bgColour);
		rect.h = 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.y = VIEW_HEIGHT - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		SetSDLRect(rect, 0, 0, 4, VIEW_HEIGHT);
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.x = VIEW_WIDTH - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		
		// Draw text prompt
		SetSDLRect(rect, 8, 8,  VIEW_WIDTH - 16, itemHeight);
		renderer->DrawText(prompt, rect, Renderer::TEXT_BIGFONT);

		// Draw OK/Cancel buttons
		renderer->DrawButton(TM_OK_BTN, okButtonRect);
		renderer->DrawButton(TM_CANCEL_BTN, cancelButtonRect);

		// Draw input text
		char s[200];
		strcpy(s, text);
		strcat(s, "_");
		SetSDLRect(rect, 8, 40,  VIEW_WIDTH - 16, itemHeight);
		renderer->DrawText(s, rect, Renderer::TEXT_BIGFONT);

		// Draw instructions
		SetSDLRect(rect, 8, VIEW_HEIGHT - itemHeight, 0, 0);
		if (0 == ((counter / 200) % 2))
			{
#ifdef PSP				
			font->DrawText(screen, "Use DPAD to select char, X to input char.", rect, false);
#else
			renderer->DrawText("Type or click letters.", rect, Renderer::TEXT_BIGFONT);
#endif
			}
		else
			{
#ifdef PSP				
			font->DrawText(screen, "Press START to finish, TRIANGLE to cancel.", rect, false);
#else
			renderer->DrawText("Press ENTER to finish, ESC to cancel.", rect, Renderer::TEXT_BIGFONT);
#endif
			}


		// draw virtual keyboard
		SDL_Rect src;
		SetSDLRect(src, 0, 0, keyboardImg->w, keyboardImg->h);
		SDL_RenderCopy(renderer->m_sdlRenderer, keyTexture, &src, &vkDestRect);

		//SetSDLRect(src, x*vkKeyWidth, y*vkKeyHeight, vkKeyWidth, vkKeyHeight);
		//SetSDLRect(rect, vkDestRect.x + x*vkKeyWidth, vkDestRect.y + y*vkKeyHeight, vkKeyWidth, vkKeyHeight);
		//SDL_RenderCopy(renderer->m_sdlRenderer, keyTextureHilite, &src, &rect);
	
		//SDL_RenderPresent(renderer);
		renderer->Blit();
		SDL_Delay(20);
		counter++;
		} // wend done

	SDL_ShowCursor(SDL_DISABLE);

	// Tidy up
	SDL_DestroyTexture(keyTexture);
	SDL_DestroyTexture(keyTextureHilite);

	return (!escapePressed);	
}

/// Browse for file in the specified folder
/// @param prompt		User prompt
/// @param folder		The folder to select a file from
/// @param filename		The input filename, also output filename
/// @return				false if cancelled, else true
/// @note
///    File selector takes up the whole screen
bool DoFileSelect(Renderer *renderer, const char* prompt, const char* folder, char* filename)
{
	if (!renderer || !prompt || !folder || !filename)
		return false;

	// array of strings for filenames
	char listnames[MAX_FILELIST_ITEMS][MAX_FILENAME_LEN];
	for (int i = 0; i < MAX_FILELIST_ITEMS; i++)
		strcpy(listnames[i], "");

	int numFiles = 0;
#ifdef _WIN32
	struct _finddata_t data;
	char findpath[256];
	strcpy(findpath, folder);
	strcat(findpath, "/*");
	intptr_t handle = _findfirst(findpath, &data);
	if (-1 != handle)
		{
		do
			{
			if (0 == strcmp(data.name, "."))
				continue;
			else if (0 == strcmp(data.name, ".."))
				continue;
			else if (numFiles < MAX_FILELIST_ITEMS)
				{
				strcpy(listnames[numFiles], data.name);
				numFiles++;
				}

			} while (0 == _findnext(handle, &data));
		_findclose(handle);
		}
#else
	DIR           *d;
	struct dirent *dir;
	d = opendir(folder);			// opendir(".");
	if (d)
		{
		while ((dir = readdir(d)) != NULL)
			{
			//printf("%s\n", dir->d_name);
			if (0 == strcmp(dir->d_name, "."))
				continue;
			if (0 == strcmp(dir->d_name, ".."))
				continue;
			if (numFiles < MAX_FILELIST_ITEMS)
				{
				strcpy(listnames[numFiles], dir->d_name);
				numFiles++;
				}
			}

		closedir(d);
		}
#endif

	SDL_Rect cancelButtonRect;
	SetSDLRect(cancelButtonRect, VIEW_WIDTH - MENU_TITLE_X - 48, 6, 48, 48);

	SDL_ShowCursor(SDL_ENABLE);
	int currentItem = 0;				// posn in list
	int scrollPos = 0;					// = index of first item visible 
	int itemsPerPage = 12;
	int itemHeight = renderer->m_bigFont->GetFontHeight();
	int counter = 0;
	SDL_Rect rect;
	SDL_Event event;
	unsigned short keyCode;
	bool done = false;
	bool escapePressed = false;
	while(!done)
		{
		int dy = 0;
		while(SDL_PollEvent(&event))
			{
			switch(event.type)
				{
				// For PSP (and others... ?)
				case SDL_JOYBUTTONDOWN:
					//printf("Pressed button %d\n", event.jbutton.button);
					//SDL_Delay(1000);
					keyCode = joyMap.GetValueAt(event.jbutton.button);
					if (LCLICK == keyCode)
						done = true;
					else if (SDLK_ESCAPE == keyCode)
						escapePressed = true;
					else if (SDLK_UP == keyCode)
						dy = -1;
					else if (SDLK_DOWN == keyCode)
						dy = 1;
/*
					switch (event.jbutton.button)
						{
						case 0 : 			// TRIANGLE
							escapePressed = true;
							break;
						case 2 : 			// CROSS
							done = true;
							break;
						case 8 :			// UP
							dy = -1;
							break;
						case 6 :     		// DOWN
							dy = 1;						
							break;
						}
*/
					break;
				case SDL_KEYDOWN:
					//printf("Key pressed: %d\n", event.key.keysym.sym);
					if (SDLK_RETURN == event.key.keysym.sym)
						done = true;
					else if (SDLK_SPACE == event.key.keysym.sym)
						done = true;
					else if (SDLK_ESCAPE == event.key.keysym.sym)
						escapePressed = true;
					else if (SDLK_UP == event.key.keysym.sym)
						dy = -1;
					else if (SDLK_DOWN == event.key.keysym.sym)
						dy = 1;
					break;
				// For systems with mouses/trackballs
				case SDL_MOUSEBUTTONDOWN:
					if (1 == event.button.button)		// Left mouse button
						{
						Sint32 mx = event.motion.x;
						Sint32 my = event.motion.y;
						// Clicked Cancel?
						if (my < (cancelButtonRect.y + cancelButtonRect.h))
							{
							if (mx > cancelButtonRect.x && mx < cancelButtonRect.x + cancelButtonRect.w)
								escapePressed = true;
							}
						else
							{
							// Select item, or change item option?
							int clickedItem = scrollPos + (event.motion.y - MENU_ITEM_START_Y) / itemHeight;
							if (clickedItem > -1 && clickedItem < numFiles)
								{
								currentItem = clickedItem;
								done = true;
								}
							}
						}
					break;
				} // end switch
			} // wend pollevent

		// Respond to input
		if (escapePressed)
			break;

		// update current selected filename
		if (1 == dy && currentItem < (numFiles-1))
			currentItem++;
		else if (-1 == dy && currentItem > 0)
			currentItem--;
			
		if (currentItem < scrollPos)
			scrollPos = currentItem;
		else if (currentItem >= (scrollPos + itemsPerPage))
			scrollPos++;
		
		// Redraw screen
		// Clear menu background & draw border
		SetSDLRect(rect, 0, 0, VIEW_WIDTH, VIEW_HEIGHT);
		renderer->DrawFilledRect(rect, renderer->m_bgColour);
		rect.h = 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.y = VIEW_HEIGHT - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		SetSDLRect(rect, 0, 0, 4, VIEW_HEIGHT);
		renderer->DrawFilledRect(rect, renderer->m_borderColour);
		rect.x = VIEW_WIDTH - 4;
		renderer->DrawFilledRect(rect, renderer->m_borderColour);

		// Draw text prompt and separator
		SetSDLRect(rect, MENU_TITLE_X, 8,  VIEW_WIDTH - 16, itemHeight);
		renderer->DrawText(prompt, rect, Renderer::TEXT_BIGFONT);

		SetSDLRect(rect, MENU_TITLE_X, 8 + itemHeight, VIEW_WIDTH - (MENU_TITLE_X*2), 2);
		renderer->DrawFilledRect(rect, renderer->m_separatorColour);

		// Draw Cancel button
		renderer->DrawButton(TM_CANCEL_BTN, cancelButtonRect);

		// Draw visible portion of file list
		for (int i = 0; i < itemsPerPage; i++)
			{
			//strcpy(s, text);
			//strcat(s, "_");
			SetSDLRect(rect, MENU_ITEM_X, MENU_ITEM_START_Y + i * itemHeight, VIEW_WIDTH - 16, itemHeight);
			renderer->DrawText(listnames[scrollPos + i], rect, Renderer::TEXT_BIGFONT);
			}

		// Draw selection highlight
		int y1 = MENU_ITEM_START_Y + ((currentItem - scrollPos) * itemHeight) - 2;
		SetSDLRect(rect, MENU_TITLE_X, y1, VIEW_WIDTH - MENU_TITLE_X * 2, itemHeight);
		renderer->DrawRect(rect, renderer->m_highlightColour);

		// Draw instructions
		SetSDLRect(rect, MENU_TITLE_X, VIEW_HEIGHT - 4 - itemHeight, 0, 0);
#ifdef PSP				
		font->DrawText("Use DPAD to highlight file, X to select.", rect, Renderer::TEXT_BIGFONT);
#else
		renderer->DrawText("Use arrows to highlight file, ENTER to select.", rect, Renderer::TEXT_BIGFONT);
#endif

		//SDL_RenderPresent(renderer);
		renderer->Blit();
		SDL_Delay(20);
		counter++;
		} // wend done

	SDL_ShowCursor(SDL_DISABLE);

	// get selected filename
	if (!escapePressed)
		strcpy(filename, listnames[currentItem]);
	
	return (!escapePressed);	
}

#define PROGBOX_WIDTH	480
#define PROGBOX_HEIGHT	160

/// Show progress bar 
/// NB: Overwritten by next screen update
bool ShowProgress(Renderer *renderer, const char* text, int progress)
{
	if (!renderer || !text || progress < 0 || progress > 100)
		{
		printf("Error: DoMessage - bad parameter!\n");
		return false;
		}

	SDL_Rect extents;
	SetSDLRect(extents, VIEW_WIDTH/2 - PROGBOX_WIDTH/2, VIEW_HEIGHT/2 - PROGBOX_HEIGHT/2, PROGBOX_WIDTH, PROGBOX_HEIGHT);

	int itemHeight = renderer->m_bigFont->GetFontHeight();
	SDL_Rect rect;
	// Clear progress popup background
	renderer->DrawFilledRect(extents, renderer->m_bgColour);

	// Draw border
	SetSDLRect(rect, extents.x, extents.y, extents.w, 4);
	renderer->DrawFilledRect(rect, renderer->m_borderColour);
	rect.y = extents.y + extents.h - 4;
	renderer->DrawFilledRect(rect, renderer->m_borderColour);
	SetSDLRect(rect, extents.x, extents.y, 4, extents.h);
	renderer->DrawFilledRect(rect, renderer->m_borderColour);
	rect.x = extents.x + extents.w - 4;
	renderer->DrawFilledRect(rect, renderer->m_borderColour);
	// Draw text
	SetSDLRect(rect, extents.x + 16, extents.y + 16, extents.w - 16, itemHeight);
	renderer->DrawText(text, rect, Renderer::TEXT_BIGFONT | Renderer::TEXT_CLIP);

	// Draw progress bar
	SetSDLRect(rect, extents.x + 20, extents.y + PROGBOX_HEIGHT/2,  progress*4, 20);
	renderer->DrawFilledRect(rect, renderer->m_highlightColour);

	//SDL_RenderPresent(renderer);
	renderer->Blit();

	return true;	
}
