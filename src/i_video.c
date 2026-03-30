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
#include "r_main.h"
#include "r_state.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include <stdlib.h>

#include "playdate.h"

// The screen buffer; this is modified to draw things to the screen
pixel_t *I_VideoBuffer = NULL;

// Pre-composed gray colormaps: gray_colormaps[colormap_offset + palette_index] = gray shade
uint8_t *gray_colormaps = NULL;

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

// 32×32 Bayer matrix (0-255). Tiles every 32px — much less visible than 4×4.
static const uint8_t bayer32[32][8] = {
    {  0,127, 31,159,  7,135, 39,167}, {191, 63,223, 95,199, 71,231,103},
    { 47,175, 15,143, 55,183, 23,151}, {239,111,207, 79,247,119,215, 87},
    { 11,139, 43,171,  3,131, 35,163}, {203, 75,235,107,195, 67,227, 99},
    { 59,187, 27,155, 51,179, 19,147}, {251,123,219, 91,243,115,211, 83},
    {  2,130, 34,162, 10,138, 42,170}, {194, 66,226, 98,202, 74,234,106},
    { 50,178, 18,146, 58,186, 26,154}, {242,114,210, 82,250,122,218, 90},
    { 14,142, 46,174,  6,134, 38,166}, {206, 78,238,110,198, 70,230,102},
    { 62,190, 30,158, 54,182, 22,150}, {254,126,222, 94,246,118,214, 86},
    {  0,128, 32,160,  8,136, 40,168}, {192, 64,224, 96,200, 72,232,104},
    { 48,176, 16,144, 56,184, 24,152}, {240,112,208, 80,248,120,216, 88},
    { 12,140, 44,172,  4,132, 36,164}, {204, 76,236,108,196, 68,228,100},
    { 60,188, 28,156, 52,180, 20,148}, {252,124,220, 92,244,116,212, 84},
    {  3,131, 35,163, 11,139, 43,171}, {195, 67,227, 99,203, 75,235,107},
    { 51,179, 19,147, 59,187, 27,155}, {243,115,211, 83,251,123,219, 91},
    { 15,143, 47,175,  7,135, 39,167}, {207, 79,239,111,199, 71,231,103},
    { 63,191, 31,159, 55,183, 23,151}, {255,127,223, 95,247,119,215, 87},
};

// 256 levels × 32 rows = 8KB. Full range matches the 0-255 Bayer thresholds.
// Every possible gray value gets its own unique pattern — no quantization banding.
#define NUM_SHADE_LEVELS 128
uint8_t shades[NUM_SHADE_LEVELS][32];

static void I_InitDitherTables(void) {
    for (int gray = 0; gray < 128; gray++) {
        int brightness = gray * 255 / 127;
        for (int row = 0; row < 32; row++) {
            uint8_t pattern = 0;
            for (int col = 0; col < 8; col++) {
                if (brightness > bayer32[row][col])
                    pattern |= (0x80 >> col);
            }
            shades[gray][row] = pattern;
        }
    }
}


//
// I_SetPalette
//
void I_SetPalette (byte *doompalette)
{
    int i;
    byte *pal = doompalette;

    for (i=0; i<256; ++i)
    {
        uint8_t red = *pal++;
        uint8_t green = *pal++;
        uint8_t blue = *pal++;
        // Perceptual luminance (0.0 - 1.0)
        float lum = (red * 299 + green * 587 + blue * 114) / 255000.0f;

        // Fully linear with slight gain. No curve — let DOOM's colormaps
        // handle distance darkening. The 32×32 Bayer handles the rest.
        int l = (int)(lum * 255.0f);
        l = (l * 9) >> 3;          // 1.125x gain
        if (l > 255) l = 255;

        graymap[i] = (uint8_t)(l >> 1);  // 0-255 → 0-127
        if (graymap[i] > 127) graymap[i] = 127;
    }

    I_InitDitherTables();
    I_RebuildGrayColormaps();
}

void I_RebuildGrayColormaps(void)
{
    if (colormaps == NULL)
        return;

    // COLORMAP lump: 34 maps of 256 bytes each
    int num_maps = 34;
    int total = num_maps * 256;

    if (gray_colormaps == NULL)
        gray_colormaps = Z_Malloc(total, PU_STATIC, NULL);

    for (int i = 0; i < total; i++) {
        gray_colormaps[i] = graymap[colormaps[i]];
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
