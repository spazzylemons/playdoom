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
//	Mission begin melt/wipe screen special effect.
//

#include <string.h>

#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"

#include "doomtype.h"

#include "f_wipe.h"

//
//                       SCREEN WIPE PACKAGE
//

// when zero, stop the wipe
static boolean	go = 0;

static pixel_t*	wipe_scr_start;
static pixel_t*	wipe_scr_end;
static pixel_t*	wipe_scr;

static int*	y;

static int wipe_initMelt(void)
{
    int i, r;
    
    // copy start screen to main screen
    memcpy(wipe_scr, wipe_scr_start, SCREENSTRIDE*SCREENHEIGHT*sizeof(*wipe_scr));
    
    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    y = (int *) Z_Malloc(SCREENWIDTH*sizeof(int), PU_STATIC, 0);
    y[0] = -(M_Random()%16);
    for (i=1;i<SCREENWIDTH;i++)
    {
	r = (M_Random()%3) - 1;
	y[i] = y[i-1] + r;
	if (y[i] > 0) y[i] = 0;
	else if (y[i] == -16) y[i] = -15;
    }

    return 0;
}

static int wipe_doMelt(int ticks)
{
    int		i;
    int		j;
    int		dy;
    int		idx;
    
    pixel_t*	s;
    pixel_t*	d;
    boolean	done = true;

    // increment each y value.
    while (ticks--) {
        for (i = 0; i < SCREENWIDTH; i++) {
            if (y[i] < 0) {
                y[i]++;
                done = false;
            } else if (y[i] < SCREENHEIGHT) {
                y[i] += (y[i] < 16) ? y[i] + 1 : 8;
                if (y[i] >= SCREENHEIGHT) {
                    y[i] = SCREENHEIGHT;
                }
                done = false;
            }
        }
    }

    // ugly blitting to do the wipe
    for (i = 0; i < SCREENWIDTH; i++) {
        uint8_t xmask = 0x80 >> (i & 7);

        s = &wipe_scr_end[i >> 3];
        d = &wipe_scr[i >> 3];

        for (j = 0; j < y[i]; j++) {
            if (xmask == 0x80)
                *d = 0;
            *d |= *s & xmask;
            d += SCREENSTRIDE;
            s += SCREENSTRIDE;
        }

        s = &wipe_scr_start[i >> 3];
        for (; j < SCREENHEIGHT; j++) {
            if (xmask == 0x80)
                *d = *s & xmask;
            *d |= *s & xmask;
            d += SCREENSTRIDE;
            s += SCREENSTRIDE;
        }
    }

    return done;

}

static int wipe_exitMelt(void)
{
    Z_Free(y);
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);
    return 0;
}

int
wipe_StartScreen(void)
{
    wipe_scr_start = Z_Malloc(SCREENSTRIDE * SCREENHEIGHT * sizeof(*wipe_scr_start), PU_STATIC, NULL);
    I_ReadScreen(wipe_scr_start);
    return 0;
}

int
wipe_EndScreen(void)
{
    wipe_scr_end = Z_Malloc(SCREENSTRIDE * SCREENHEIGHT * sizeof(*wipe_scr_end), PU_STATIC, NULL);
    I_ReadScreen(wipe_scr_end);
    V_CopyScreen(wipe_scr_start);
    return 0;
}

int
wipe_ScreenWipe(int ticks)
{
    int rc;

    // initial stuff
    if (!go)
    {
	go = 1;
	wipe_scr = I_VideoBuffer;
	wipe_initMelt();
    }

    // do a piece of wipe-in
    rc = wipe_doMelt(ticks);

    // final stuff
    if (rc)
    {
	go = 0;
	wipe_exitMelt();
    }

    return !go;
}

