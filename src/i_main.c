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
//	Main program, simply calls D_DoomMain high level loop.
//

#include "my_stdio.h"

#include "doomkeys.h"
#include "doomstat.h"
#include "i_cheat.h"
#include "i_input.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "s_sound.h"

#include "playdate.h"

PlaydateAPI *playdate;

extern boolean entering_cheat;

static boolean RunInit(void) {
    typedef struct {
        void (*action)(void);
        const char *description;
    } init_func_t;

    // Split the main init into a LOT of smaller functions. If we run all
    // initialization at once, we run the risk of the watchdog timer terminating
    // the game after 10 seconds pass without us finishing. As a bonus of this
    // technique, we can display a loading progress screen.
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

        // No description on this one since it ends up drawing the title screen,
        // so the user likely won't even see this step.
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
        I_DrawLine("Loading, this may take a while.", 80);
        I_DrawLine(message, 100);

        // progress of loading.
        playdate->graphics->fillRect(0, 120, (initIndex * LCD_COLUMNS) / numInitRoutines, 20, kColorBlack);

        // Render now! If we don't flush the screen, the progress bar will be
        // behind, since the display call would only happen after the action
        // is finished.
        playdate->graphics->display();

        initRoutines[initIndex].action();
        ++initIndex;

        // If we're done, we should replace the currently loaded font with the
        // one used for the cheat module.
        if (initIndex == numInitRoutines) {
            I_LoadFont("/System/Fonts/Roobert-24-Medium");
        }

        return true;
    }
}

void D_DoomLoopRun (void);

static int the_argc = 1;
static char *the_argv[] = {"doom", NULL};

static int update(void *userdata) {
    (void) userdata;

    if (!RunInit()) {
        D_DoomLoopRun();

        if (entering_cheat)
            I_DrawCheat();

        playdate->system->drawFPS(0, 0);
    }

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

static void init_cheat(void *userdata) {
    (void) userdata;

    I_InitCheat();
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
        I_LoadFont("/System/Fonts/Asheville-Sans-14-Bold");

        playdate->system->setUpdateCallback(update, NULL);
        playdate->system->addMenuItem("open menu", game_menu, NULL);
        playdate->system->addMenuItem("enter cheat", init_cheat, NULL);
    } else if (event == kEventTerminate) {
        I_Quit();
    }

    return 0;
}

