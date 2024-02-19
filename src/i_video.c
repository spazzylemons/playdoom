//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM graphics stuff for SDL.
//

#include "my_stdio.h"

#include "doomtype.h"
#include "i_input.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include <stdlib.h>

#include "playdate.h"

// The screen buffer; this is modified to draw things to the screen
pixel_t *I_VideoBuffer = NULL;

// Loaded font.
static LCDFont *font = NULL;

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

//
// I_StartTic
//
void I_StartTic (void)
{
    I_ReadButtons();
    I_ReadMouse();
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    static int lasttic;
    int tics;
    int i;

    playdate->graphics->markUpdatedRows(0, LCD_ROWS - 1);
}


//
// I_ReadScreen
//
void I_ReadScreen (pixel_t* scr)
{
    memcpy(scr, I_VideoBuffer, SCREENSTRIDE*SCREENHEIGHT*sizeof(*scr));
}

// which of the 17 shades of gray for each color?
uint8_t graymap[256];

// the shades themselves
#define DitherPattern(a, b, c, d) { a * 17, b * 17, c * 17, d * 17 }

const uint8_t shades[17][4] = {
    DitherPattern(0x0, 0x0, 0x0, 0x0),
    DitherPattern(0x8, 0x0, 0x0, 0x0),
    DitherPattern(0x8, 0x0, 0x2, 0x0),
    DitherPattern(0x8, 0x0, 0xa, 0x0),
    DitherPattern(0xa, 0x0, 0xa, 0x0),
    DitherPattern(0xa, 0x0, 0xa, 0x1),
    DitherPattern(0xa, 0x4, 0xa, 0x1),
    DitherPattern(0xa, 0x4, 0xa, 0x5),
    DitherPattern(0xa, 0x5, 0xa, 0x5),
    DitherPattern(0xd, 0x5, 0xa, 0x5),
    DitherPattern(0xd, 0x5, 0xe, 0x5),
    DitherPattern(0xd, 0x5, 0xf, 0x5),
    DitherPattern(0xf, 0x5, 0xf, 0x5),
    DitherPattern(0xf, 0x5, 0xf, 0xd),
    DitherPattern(0xf, 0x7, 0xf, 0xd),
    DitherPattern(0xf, 0x7, 0xf, 0xf),
    DitherPattern(0xf, 0xf, 0xf, 0xf),
};


//
// I_SetPalette
//
void I_SetPalette (byte *doompalette)
{
    int i;

    for (i=0; i<256; ++i)
    {
        uint8_t red = *doompalette++;
        uint8_t green = *doompalette++;
        uint8_t blue = *doompalette++;
        graymap[i] = (red * 299 + green * 587 + blue * 114) / 15001;
    }
}

void I_InitGraphics(void)
{
    byte *doompal;
    char *env;

    // Set the palette

    doompal = W_CacheLumpName("PLAYPAL", PU_CACHE);
    I_SetPalette(doompal);

    I_VideoBuffer = playdate->graphics->getFrame();
    V_RestoreBuffer();

    // Clear the screen to black.

    memset(I_VideoBuffer, 0, SCREENSTRIDE * SCREENHEIGHT);
}

// Load Playdate font.
void I_LoadFont(const char *path) {
    if (font != NULL) {
        playdate->system->realloc(font, 0);
    }
    font = playdate->graphics->loadFont(path, NULL);
    playdate->graphics->setFont(font);
}

// Draw centered text.
void I_DrawText(const char *text, int x, int y, int w) {
    int length = strlen(text);
    int width = playdate->graphics->getTextWidth(font, text, length, kUTF8Encoding, 0);
    playdate->graphics->drawText(text, length, kUTF8Encoding, x + (w - width) / 2, y);
}

// Draw centered line.
void I_DrawLine(const char *text, int y) {
    I_DrawText(text, 0, y, LCD_COLUMNS);
}
