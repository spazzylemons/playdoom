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
//     Input handling.
//


#include "doomkeys.h"
#include "doomtype.h"
#include "d_event.h"
#include "i_input.h"
#include "m_argv.h"
#include "m_config.h"

#include "playdate.h"

static uint8_t buttonsdown = 0;
static uint16_t current_state = 0;
static uint16_t previous_state = 0;

static uint8_t buttonmasks[6] = {
    1 << (PDKEY_LEFT - 1),
    1 << (PDKEY_RIGHT - 1),
    1 << (PDKEY_UP - 1),
    1 << (PDKEY_DOWN - 1),
    1 << (PDKEY_BACK - 1),
    1 << (PDKEY_FIRE - 1),
};

extern boolean entering_cheat;

void I_ReadButtons(void)
{
    event_t event;

    PDButtons buttons;
    playdate->system->getButtonState(&buttons, NULL, NULL);

    for (int i = 0; i < 6; i++) {
        if (buttons & (1 << i)) {
            buttonsdown |= buttonmasks[i];
        } else {
            buttonsdown &= ~buttonmasks[i];
        }
    }

    for (int i = 0; i < 5; i++) {
        if ((buttonsdown & (1 << i)) && !(buttonsdown & (1 << 5))) {
            current_state |= 1 << i;
        } else {
            current_state &= ~(1 << i);
        }

        if ((buttonsdown & (1 << i)) && (buttonsdown & (1 << 5))) {
            current_state |= 1 << (i + 6);
        } else {
            current_state &= ~(1 << (i + 6));
        }
    }
    current_state &= ~(1 << 5);
    current_state |= buttonsdown & (1 << 5);

    for (int i = 0; i < 11; i++) {
        if ((current_state & (1 << i)) && !(previous_state & (1 << i))) {
            event.type = ev_keydown;
            event.data2 = event.data1 = i + 1;
            event.data3 = 0;
            D_PostEvent(&event);
        } else if (!(current_state & (1 << i)) && (previous_state & (1 << i))) {
            event.type = ev_keyup;
            event.data1 = i + 1;
            event.data2 = event.data3 = 0;
            D_PostEvent(&event);
        }
    }
    previous_state = current_state;
}

//
// Read the change in mouse state to generate mouse motion events
//
// This is to combine all mouse movement for a tic into one mouse
// motion event.
void I_ReadMouse(void)
{
    int x, y;
    event_t ev;

    if (!entering_cheat && !playdate->system->isCrankDocked()) {
        x = 20.0f * playdate->system->getCrankChange();
    } else {
        x = 0;
    }
    y = 0;

    if (x != 0 || y != 0) 
    {
        ev.type = ev_mouse;
        ev.data1 = 0;
        ev.data2 = x;
        ev.data3 = 0;

        // XXX: undefined behaviour since event is scoped to
        // this function
        D_PostEvent(&ev);
    }
}
