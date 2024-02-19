//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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
//	Gamma correction LUT stuff.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//

#include <string.h>

#include "i_system.h"

#include "doomtype.h"

#include "i_input.h"
#include "i_swap.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "playdate.h"

// The screen buffer that the v_video.c code draws to.

static pixel_t *dest_screen = NULL;


//
// V_CopyRect
//
void V_CopyRect(int srcx, int srcy, pixel_t *source,
                int width, int height,
                int destx, int desty)
{
    pixel_t *src;
    pixel_t *dest;

#ifdef RANGECHECK
    if (srcx < 0
     || srcx + width > SCREENWIDTH
     || srcy < 0
     || srcy + height > SCREENHEIGHT
     || destx < 0
     || destx + width > SCREENWIDTH
     || desty < 0
     || desty + height > SCREENHEIGHT)
    {
        I_Error ("Bad V_CopyRect");
    }
#endif

    src = source + SCREENSTRIDE * srcy + (srcx >> 3);
    dest = dest_screen + SCREENSTRIDE * desty + (destx >> 3);

    for ( ; height>0 ; height--) {
        uint8_t srcxmask = 0x80 >> (srcx & 7);
        uint8_t destxmask = 0x80 >> (destx & 7);
        pixel_t *srcline = src;
        pixel_t *destline = dest;
        // TODO optimize this for 1-bit screen. Yuck.
        for (int w = 0; w < width; w++) {
            if (*srcline & srcxmask) {
                *destline |= destxmask;
            } else {
                *destline &= ~destxmask;
            }

            srcxmask >>= 1;
            if (srcxmask == 0) {
                srcline++;
                srcxmask = 0x80;
            }

            destxmask >>= 1;
            if (destxmask == 0) {
                destline++;
                destxmask = 0x80;
            }
        }

        src += SCREENSTRIDE;
        dest += SCREENSTRIDE;
    }
}

//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//

void V_DrawPatch(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    pixel_t *desttop;
    pixel_t *dest;
    byte *source;
    int w;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > SCREENWIDTH
     || y < 0
     || y + SHORT(patch->height) > SCREENHEIGHT)
    {
        I_Error("Bad V_DrawPatch");
    }
#endif

    col = 0;
    desttop = &dest_screen[(y * SCREENSTRIDE) + (x >> 3)];
    uint8_t xmask = 0x80 >> (x & 7);

    w = SHORT(patch->width);

    for ( ; col<w ; col++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            int yoff = y + column->topdelta;
            source = (byte *)column + 3;

            dest = desttop + column->topdelta*SCREENSTRIDE;
            count = column->length;

            while (count--)
            {
                if (shades[graymap[*source++]][(yoff++) & 3] & xmask) {
                    *dest |= xmask;
                } else {
                    *dest &= ~xmask;
                }
                dest += SCREENSTRIDE;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        xmask >>= 1;
        if (xmask == 0) {
            xmask = 0x80;
            ++desttop;
        }
    }
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//

void V_DrawPatchFlipped(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    pixel_t *desttop;
    pixel_t *dest;
    byte *source;
    int w;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > SCREENWIDTH
     || y < 0
     || y + SHORT(patch->height) > SCREENHEIGHT)
    {
        I_Error("Bad V_DrawPatchFlipped");
    }
#endif

    col = 0;
    desttop = &dest_screen[(y * SCREENSTRIDE) + (x >> 3)];
    uint8_t xmask = 0x80 >> (x & 7);

    w = SHORT(patch->width);

    for ( ; col<w ; col++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[w-1-col]));

        // step through the posts in a column
        while (column->topdelta != 0xff )
        {
            int yoff = y + column->topdelta;
            source = (byte *)column + 3;
            dest = desttop + column->topdelta*SCREENSTRIDE;
            count = column->length;

            while (count--)
            {
                if (shades[graymap[*source++]][(yoff++) & 3] & xmask) {
                    *dest |= xmask;
                } else {
                    *dest &= ~xmask;
                }
                dest += SCREENSTRIDE;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        xmask >>= 1;
        if (xmask == 0) {
            xmask = 0x80;
            ++desttop;
        }
    }
}



//
// V_DrawPatchDirect
// Draws directly to the screen on the pc.
//

void V_DrawPatchDirect(int x, int y, patch_t *patch)
{
    V_DrawPatch(x, y, patch);
}

void V_DrawPatchFont(int x, int y, patch_t *patch)
{
    int count;
    int col;
    column_t *column;
    pixel_t *desttop;
    pixel_t *dest;
    byte *source;
    int w;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > SCREENWIDTH
     || y < 0
     || y + SHORT(patch->height) > SCREENHEIGHT)
    {
        I_Error("Bad V_DrawPatch");
    }
#endif

    col = 0;
    desttop = &dest_screen[(y * SCREENSTRIDE) + (x >> 3)];
    uint8_t xmask = 0x80 >> (x & 7);

    w = SHORT(patch->width);

    for ( ; col<w ; col++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            int yoff = y + column->topdelta;
            source = (byte *)column + 3;

            dest = desttop + column->topdelta*SCREENSTRIDE;
            count = column->length;

            while (count--)
            {
                if (*source++ < 190) {
                    *dest |= xmask;
                } else {
                    *dest &= ~xmask;
                }
                dest += SCREENSTRIDE;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }

        xmask >>= 1;
        if (xmask == 0) {
            xmask = 0x80;
            ++desttop;
        }
    }
}

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//

void V_CopyScreen(const pixel_t *buffer)
{
    memcpy(dest_screen, buffer, SCREENSTRIDE * SCREENHEIGHT);
}

void V_BlackBars(void) {
    uint8_t *buffer = I_VideoBuffer;
    for (int y = 0; y < SCREENHEIGHT; y++) {
        memset(buffer, 0, 5);
        memset(buffer + 45, 0, 5);
        buffer += SCREENSTRIDE;
    }
}

void V_ClearScreen(void) {
    memset(I_VideoBuffer, 0, SCREENSTRIDE * SCREENHEIGHT);
}

void V_StretchColumn(int x, int y, column_t *column) {
    int count;
    pixel_t *desttop;
    byte *source;

#ifdef RANGECHECK
    if (x < 0 || x >= SCREENWIDTH)
    {
        I_Error("Bad V_DrawPatch");
    }
#endif

    desttop = &dest_screen[(y * SCREENSTRIDE) + (x >> 3)];
    uint8_t xmask = 0x80 >> (x & 7);

    // step through the posts in a column
    while (column->topdelta != 0xff)
    {
        source = (byte *)column + 3;

        int desty = column->topdelta;

        count = column->length;

        while (count--)
        {
            int yoff = y + ((6*desty)/5);
            pixel_t *dest = desttop + ((6*desty)/5)*SCREENSTRIDE;
            pixel_t *destend = desttop + ((6*(desty+1))/5)*SCREENSTRIDE;
            uint8_t gray = graymap[*source++];
            do {
                if (shades[gray][(yoff++) & 3] & xmask) {
                    *dest |= xmask;
                } else {
                    *dest &= ~xmask;
                }
                dest += SCREENSTRIDE;
            } while (dest < destend);
            ++desty;
        }
        column = (column_t *)((byte *)column + column->length + 4);
    }
}

void V_DrawStretched(int x, int y, patch_t *patch) {
    int col;
    int w;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    y = (y*6)/5;

    col = 0;

    w = SHORT(patch->width);

    for ( ; col<w ; col++, x++)
    {
        V_StretchColumn(x, y, (column_t *)((byte *)patch + LONG(patch->columnofs[col])));
    }
}

// Set the buffer that the code draws to.

void V_UseBuffer(pixel_t *buffer)
{
    dest_screen = buffer;
}

// Restore screen buffer to the i_video screen buffer.

void V_RestoreBuffer(void)
{
    dest_screen = I_VideoBuffer;
}
