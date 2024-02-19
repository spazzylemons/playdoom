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
//	System specific interface stuff.
//


#ifndef __I_VIDEO__
#define __I_VIDEO__

#include "doomtype.h"

// Screen width and height.

#define SCREENWIDTH  400
#define SCREENHEIGHT 240

#define SCREENSTRIDE 52

// Screen height used when aspect_ratio_correct=true.

#define SCREENHEIGHT_4_3 240

extern uint8_t graymap[256];
extern const uint8_t shades[17][4];

typedef boolean (*grabmouse_callback_t)(void);

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics (void);

void I_GraphicsCheckCommandLine(void);

void I_ShutdownGraphics(void);

// Takes full 8 bit values.
void I_SetPalette (byte* palette);

void I_UpdateNoBlit (void);
void I_FinishUpdate (void);

void I_ReadScreen (pixel_t* scr);

// Called before processing any tics in a frame (just after displaying a frame).
// Time consuming syncronous operations are performed here (joystick reading).

void I_StartFrame (void);

// Called before processing each tic in a frame.
// Quick syncronous operations are performed here.

void I_StartTic (void);

// Playdate video routines shared by loading screen and cheat screen.

// Load Playdate font.
void I_LoadFont(const char *path);
// Draw centered text.
void I_DrawText(const char *text, int x, int y, int w);
// Draw centered line.
void I_DrawLine(const char *text, int y);

extern pixel_t *I_VideoBuffer;

#endif
