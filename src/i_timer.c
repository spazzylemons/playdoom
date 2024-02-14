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
//      Timer functions.
//

#include "i_timer.h"
#include "doomtype.h"
#include "playdate.h"

//
// I_GetTime
// returns time in 1/35th second tics
//

static uint32_t basetime = 0;

int  I_GetTime (void)
{
    uint32_t ticks;

    ticks = playdate->system->getCurrentTimeMilliseconds();

    if (basetime == 0)
        basetime = ticks;

    ticks -= basetime;

    return (ticks * TICRATE) / 1000;    
}

//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void)
{
    uint32_t ticks;

    ticks = playdate->system->getCurrentTimeMilliseconds();

    if (basetime == 0)
        basetime = ticks;

    return ticks - basetime;
}


void I_InitTimer(void)
{
}

