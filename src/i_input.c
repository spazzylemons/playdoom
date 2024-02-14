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
//     SDL implementation of system-specific input interface.
//


#include "doomkeys.h"
#include "doomtype.h"
#include "d_event.h"
#include "i_input.h"
#include "m_argv.h"
#include "m_config.h"

#include "playdate.h"

// If true, I_StartTextInput() has been called, and we are populating
// the data3 field of ev_keydown events.
static boolean text_input_enabled = true;

// Bit mask of mouse button state.
static unsigned int mouse_button_state = 0;

static boolean sdl_keysdown[6] = {0,0,0,0,0,0};
static boolean current_state[11] = {0,0,0,0,0,0,0,0,0,0,0};
static boolean previous_state[11] = {0,0,0,0,0,0,0,0,0,0,0};

static uint8_t buttonmap[6] = {
    PDKEY_LEFT - 1,
    PDKEY_RIGHT - 1,
    PDKEY_UP - 1,
    PDKEY_DOWN - 1,
    PDKEY_BACK - 1,
    PDKEY_FIRE - 1,
};

extern boolean entering_cheat;

void I_ReadButtons(void)
{
    event_t event;

    PDButtons buttons;
    playdate->system->getButtonState(&buttons, NULL, NULL);

    for (int i = 0; i < 6; i++) {
        sdl_keysdown[buttonmap[i]] = !!(buttons & (1 << i));
    }

    for (int i = 0; i < 5; i++) {
        current_state[i] = sdl_keysdown[i] && !sdl_keysdown[5];
        current_state[i + 6] = sdl_keysdown[i] && sdl_keysdown[5];
    }
    current_state[5] = sdl_keysdown[5];

    for (int i = 0; i < 11; i++) {
        if (current_state[i] && !previous_state[i]) {
            event.type = ev_keydown;
            event.data2 = event.data1 = i + 1;
            event.data3 = 0;
            D_PostEvent(&event);
        } else if (!current_state[i] && previous_state[i]) {
            event.type = ev_keyup;
            event.data1 = i + 1;
            event.data2 = event.data3 = 0;
            D_PostEvent(&event);
        }
        previous_state[i] = current_state[i];
    }
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
        ev.data1 = mouse_button_state;
        ev.data2 = x;
        ev.data3 = 0;

        // XXX: undefined behaviour since event is scoped to
        // this function
        D_PostEvent(&ev);
    }
}
