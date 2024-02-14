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
//       Key definitions
//

#ifndef __DOOMKEYS__
#define __DOOMKEYS__

// Playdate controls bound to keys.
enum {
    PDKEY_LEFT = 1,
    PDKEY_RIGHT,
    PDKEY_UP,
    PDKEY_DOWN,
    PDKEY_FIRE,

    PDKEY_BACK,

    PDKEY_PREVWEAPON,
    PDKEY_NEXTWEAPON,
    PDKEY_AUTOMAP,
    PDKEY_AUTORUN,
    PDKEY_USE,

    PDKEY_MENU,

    PDKEY_COUNT
};

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
#define KEY_ENTER	13
#define KEY_BACKSPACE	0x7f

#endif          // __DOOMKEYS__

