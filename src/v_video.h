//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//


#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"

// Needed because we are refering to patches.
#include "v_patch.h"

//
// VIDEO
//

#define CENTERY			(SCREENHEIGHT/2)

void V_ClearScreen(void);

// Draw a block from the specified source screen to the screen.

void V_CopyRect(int srcx, int srcy, pixel_t *source,
                int width, int height,
                int destx, int desty);

void V_DrawPatch(int x, int y, patch_t *patch);
void V_DrawPatchFlipped(int x, int y, patch_t *patch);
void V_DrawPatchDirect(int x, int y, patch_t *patch);

// Specialized version of DrawPatch that picks colors better for drawing font.
void V_DrawPatchFont(int x, int y, patch_t *patch);

// Draw a linear block of pixels into the view buffer.

void V_CopyScreen(const pixel_t *buffer);

// Draw black bars on sides of screen for full-screen patches.
void V_BlackBars(void);

// Draw a patch column stretched to the full screen height. Use this for
// full-screen stuff: intermission background, title screen, etc.
void V_StretchColumn(int x, int y, column_t *column);

// Draw a stretched fullscreen patch.
void V_DrawStretched(int x, int y, patch_t *patch);

// Temporarily switch to using a different buffer to draw graphics, etc.

void V_UseBuffer(pixel_t *buffer);

// Return to using the normal screen buffer to draw graphics.

void V_RestoreBuffer(void);

#endif

