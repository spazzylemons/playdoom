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
//     Cheat menu.
//


#ifndef __I_CHEAT__
#define __I_CHEAT__

#include "d_event.h"

void I_InitCheat(void);
void I_DrawCheat(void);
void I_CheatResponder(event_t *event);

extern boolean entering_cheat;

#endif
