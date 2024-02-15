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
//	Main program, simply calls D_DoomMain high level loop.
//

#include "my_stdio.h"

#include "doomtype.h"
#include "doomkeys.h"
#include "doomstat.h"
#include "i_input.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_cheat.h"
#include "s_sound.h"

#include "playdate.h"

PlaydateAPI *playdate;

extern boolean is_wiping;
extern boolean entering_cheat;
extern boolean cheat_redraw;

static LCDFont* font = NULL;

#define CHAR_WIDTH 36
#define NUM_CHARS 37
#define FULL_WIDTH (CHAR_WIDTH * NUM_CHARS)

static int cheat_scroll = 0;
static int cheat_selected = 0;
static int cheat_length = 0;
static float cheat_crank = 0.0f;

static int keyrepeat_timers[PDKEY_COUNT];

#define TIMER_REPEAT 10

static const char characters[NUM_CHARS][4] = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "▸",
};

static void init_cheat(void *userdata) {
    (void) userdata;

    if (!is_wiping && !entering_cheat && (gamestate == GS_LEVEL)) {
        cheat_scroll = cheat_selected = cheat_length = 0;

        for (int i = 0; i < PDKEY_COUNT; i++) {
            keyrepeat_timers[i] = -1;
        }

        cheat_crank = playdate->system->getCrankAngle();
        entering_cheat = true;
    }
}

void handle_cheat_input(event_t *event) {
    if (event->type == ev_keydown) {
        keyrepeat_timers[event->data1] = 0;
    } else if (event->type == ev_keyup) {
        keyrepeat_timers[event->data1] = -1;
    }
}

static boolean check_key_repeat(int key) {
    return keyrepeat_timers[key] == 0 || keyrepeat_timers[key] == TIMER_REPEAT;
}

static void DrawCenteredText(const char *text, int x, int y, int w) {
    int length = strlen(text);
    int width = playdate->graphics->getTextWidth(font, text, length, kUTF8Encoding, 0);
    playdate->graphics->drawText(text, length, kUTF8Encoding, x + (w - width) / 2, y);
}

static void DrawCenteredLine(const char *text, int y) {
    DrawCenteredText(text, 0, y, LCD_COLUMNS);
}

static boolean RunInit(void) {
    typedef struct {
        void (*action)(void);
        const char *description;
    } init_func_t;

    // Split the main init into a LOT of smaller functions so we don't get watchdog'd
    void D_DoomMain1(void);
    void D_DoomMain2(void);
    void D_DoomMain3(void);
    void D_DoomMain4(void);
    void D_DoomMain5(void);
    void D_DoomMain6(void);
    void D_DoomMain7(void);
    void D_DoomMain8(void);
    void D_DoomMain9(void);
    void D_DoomMain10(void);

    static const init_func_t initRoutines[] = {
        { D_DoomMain1, "General initialization..." },
        { D_DoomMain2, "Loading sounds..." },
        { D_DoomMain3, "Loading music..." },
        { D_DoomMain4, "Various configuration..." },
        { D_DoomMain5, "Initializing renderer..." },
        { D_DoomMain6, "Initializing playloop..." },
        { D_DoomMain7, "Setting up sound engine..." },
        { D_DoomMain8, "Setting up UI..." },
        { D_DoomMain9, "Finishing up..." },
        { D_DoomMain10, "" },
    };

    static const int numInitRoutines = arrlen(initRoutines);
    static int initIndex = 0;

    if (initIndex == numInitRoutines) {
        return false;
    } else {
        // Print what we're doing, with a progress bar.
        const char *message = initRoutines[initIndex].description;

        playdate->graphics->clear(kColorWhite);
        DrawCenteredLine("Loading, this may take a while.", 80);
        DrawCenteredLine(message, 100);

        // progress of loading.
        playdate->graphics->fillRect(0, 120, (initIndex * LCD_COLUMNS) / numInitRoutines, 20, kColorBlack);

        // Render now!
        playdate->graphics->display();

        initRoutines[initIndex].action();
        ++initIndex;

        // If we're done, we should replace the currently loaded font with the
        // one used for the cheat module.
        if (initIndex == numInitRoutines) {
            playdate->system->realloc(font, 0);
            font = playdate->graphics->loadFont("/System/Fonts/Roobert-24-Medium", NULL);
            playdate->graphics->setFont(font);
        }

        return true;
    }
}

static void draw_cheat(void) {
    playdate->graphics->pushContext(NULL);
    playdate->graphics->fillRect(0, 0, LCD_COLUMNS, CHAR_WIDTH * 2, kColorBlack);
    playdate->graphics->setDrawMode(kDrawModeFillWhite);

    int x = (LCD_COLUMNS / 2) - (CHAR_WIDTH / 2) - cheat_scroll - FULL_WIDTH;
    for (int i = 0; i < 2 * NUM_CHARS + 6; i++) {
        if (x >= -CHAR_WIDTH && x < LCD_COLUMNS) {
            const char *character = &characters[(i + NUM_CHARS) % NUM_CHARS][0];
            DrawCenteredText(character, x, 0, CHAR_WIDTH);
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
    DrawCenteredLine(buf, 36);

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

    playdate->graphics->popContext();
}

void D_DoomLoopRun (void);

static int the_argc = 1;
static char *the_argv[] = {"doom", NULL};

static int update(void *userdata) {
    (void) userdata;

    if (RunInit()) {
        goto end;
    }

    D_DoomLoopRun();
    if (entering_cheat) {
        draw_cheat();
    }

    playdate->system->drawFPS(0, 0);

end:
    return 1;
}

static void game_menu(void *userdata) {
    (void) userdata;

    // send events where the menu key is down.
    event_t event;
    event.type = ev_keydown;
    event.data2 = event.data1 = PDKEY_MENU;
    event.data3 = 0;
    D_PostEvent(&event);
    event.type = ev_keyup;
    event.data1 = PDKEY_MENU;
    event.data2 = event.data3 = 0;
    D_PostEvent(&event);
}

extern void I_Quit(void);

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
    if (event == kEventInit) {
        playdate = pd;

        myargc = the_argc;
        myargv = the_argv;

        playdate->display->setRefreshRate(TICRATE);

        // Load a smaller font for the initialization screen.
        font = playdate->graphics->loadFont("/System/Fonts/Asheville-Sans-14-Bold", NULL);
        playdate->graphics->setFont(font);

        playdate->system->setUpdateCallback(update, NULL);
        playdate->system->addMenuItem("open menu", game_menu, NULL);
        playdate->system->addMenuItem("enter cheat", init_cheat, NULL);
    } else if (event == kEventTerminate) {
        I_Quit();
    }

    return 0;
}

