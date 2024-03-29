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
//     Cheat menu.
//


#include "doomkeys.h"
#include "doomstat.h"
#include "m_cheat.h"
#include "i_cheat.h"
#include "i_video.h"
#include "s_sound.h"

#include "my_stdio.h"

#include "playdate.h"

boolean entering_cheat = false;

extern boolean is_wiping;
extern boolean cheat_redraw;

#define CHAR_WIDTH 36
#define NUM_CHARS 37
#define FULL_WIDTH (CHAR_WIDTH * NUM_CHARS)

static int cheat_scroll = 0;
static int cheat_selected = 0;
static int cheat_length = 0;
static float cheat_crank = 0.0f;

static signed char keyrepeat_timers[PDKEY_COUNT];

#define TIMER_REPEAT 10

static const char characters[NUM_CHARS][4] = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "▸",
};

void I_InitCheat(void) {
    // If screen wipe, don't open cheat menu, because game not running.
    // If already entering cheat, don't run this.
    // If not in-game, don't run this.
    if (is_wiping || entering_cheat || (gamestate != GS_LEVEL))
        return;

    cheat_scroll = cheat_selected = cheat_length = 0;

    for (int i = 0; i < PDKEY_COUNT; i++) {
        keyrepeat_timers[i] = -1;
    }

    cheat_crank = playdate->system->getCrankAngle();
    entering_cheat = true;
}

void I_CheatResponder(event_t *event) {
    if (event->type == ev_keydown) {
        keyrepeat_timers[event->data1] = 0;
    } else if (event->type == ev_keyup) {
        keyrepeat_timers[event->data1] = -1;
    }
}

static boolean check_key_repeat(int key) {
    return keyrepeat_timers[key] == 0 || keyrepeat_timers[key] == TIMER_REPEAT;
}

void I_DrawCheat(void) {
    playdate->graphics->fillRect(0, 0, LCD_COLUMNS, CHAR_WIDTH * 2, kColorBlack);
    playdate->graphics->setDrawMode(kDrawModeFillWhite);

    int x = (LCD_COLUMNS / 2) - (CHAR_WIDTH / 2) - cheat_scroll - FULL_WIDTH;
    for (int i = 0; i < 2 * NUM_CHARS + 6; i++) {
        if (x >= -CHAR_WIDTH && x < LCD_COLUMNS) {
            const char *character = &characters[(i + NUM_CHARS) % NUM_CHARS][0];
            I_DrawText(character, x, 0, CHAR_WIDTH);
        }
        x += CHAR_WIDTH;
    }

    int old_selected = cheat_selected;

    if (check_key_repeat(PDKEY_LEFT)) {
        cheat_selected = (cheat_selected + NUM_CHARS - 1) % NUM_CHARS;
    }

    if (check_key_repeat(PDKEY_RIGHT)) {
        cheat_selected = (cheat_selected + 1) % NUM_CHARS;
    }

    if (!playdate->system->isCrankDocked()) {
        float new_crank = playdate->system->getCrankAngle();
        while (new_crank - cheat_crank > 15.0f) {
            cheat_selected = (cheat_selected + 1) % NUM_CHARS;
            cheat_crank = fmodf(cheat_crank + 15.0f, 360.0f);
        }
        while (new_crank - cheat_crank < -15.0f) {
            cheat_selected = (cheat_selected + NUM_CHARS - 1) % NUM_CHARS;
            cheat_crank = fmodf(cheat_crank + 345.0f, 360.0f);
        }
    }

    if (old_selected != cheat_selected) {
        S_StartSound(NULL, sfx_swtchn);
    }

    int scroll_target = cheat_selected * CHAR_WIDTH;
    int diff0 = (scroll_target - cheat_scroll) % FULL_WIDTH;
    int diff = ((2 * diff0) % FULL_WIDTH) - diff0;
    if (abs(diff) < 3) {
        cheat_scroll = scroll_target;
    } else {
        cheat_scroll = (cheat_scroll + diff / 2 + FULL_WIDTH) % FULL_WIDTH;
    }

    playdate->graphics->fillRect(((LCD_COLUMNS - CHAR_WIDTH) / 2), 0, CHAR_WIDTH, CHAR_WIDTH, kColorXOR);

    char buf[MAX_CHEAT_LEN + 2];
    snprintf(buf, sizeof(buf), "%.*s|", cheat_length, last_cheat);
    I_DrawLine(buf, 36);

    if (check_key_repeat(PDKEY_FIRE)) {
        S_StartSound(NULL, sfx_pistol);

        if (cheat_selected == 36) {
            if (cheat_length < MAX_CHEAT_LEN) {
                last_cheat[cheat_length] = 0;
            }
            entering_cheat = false;
            cheat_redraw = true;
            event_t event;
            event.type = ev_cheat;
            D_PostEvent(&event);
        } else if (cheat_length < MAX_CHEAT_LEN) {
            last_cheat[cheat_length++] = characters[cheat_selected][0];
        }
    } else if (check_key_repeat(PDKEY_BACK)) {
        S_StartSound(NULL, sfx_swtchx);
        if (cheat_length > 0)
            --cheat_length;
    }

    for (int i = 0; i < PDKEY_COUNT; i++) {
        if (keyrepeat_timers[i] >= 0 && keyrepeat_timers[i] < TIMER_REPEAT) {
            ++keyrepeat_timers[i];
        }
    }
}

