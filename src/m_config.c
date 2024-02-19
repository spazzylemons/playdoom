//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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
//    Configuration file interface.
//

#include "my_stdio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "doomtype.h"
#include "doomkeys.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"

#include "z_zone.h"

#include "playdate.h"

//
// DEFAULTS
//

typedef struct
{
    // Name of the variable
    const char *name;

    // Pointer to the location in memory of the variable
    int *location;
} default_t;

typedef struct
{
    default_t *defaults;
    int numdefaults;
    const char *filename;
} default_collection_t;

#define CONFIG_VARIABLE(name) \
    { #name, NULL }

//! @begin_config_file default

static default_t	doom_defaults_list[] =
{
    //!
    // Mouse sensitivity.  This value is used to multiply input mouse
    // movement to control the effect of moving the mouse.
    //
    // The "normal" maximum value available for this through the
    // in-game options menu is 9. A value of 31 or greater will cause
    // the game to crash when entering the options menu.
    //

    CONFIG_VARIABLE(mouse_sensitivity),

    //!
    // Volume of sound effects, range 0-15.
    //

    CONFIG_VARIABLE(sfx_volume),

    //!
    // Volume of in-game music, range 0-15.
    //

    CONFIG_VARIABLE(music_volume),

    //!
    // @game doom
    //
    // If non-zero, messages are displayed on the heads-up display
    // in the game ("picked up a clip", etc).  If zero, these messages
    // are not displayed.
    //

    CONFIG_VARIABLE(show_messages),

    //!
    // Screen size, range 3-11.
    //
    // A value of 11 gives a full-screen view with the status bar not
    // displayed.  A value of 10 gives a full-screen view with the
    // status bar displayed.
    //

    CONFIG_VARIABLE(screenblocks),

    //!
    // Screen detail.  Zero gives normal "high detail" mode, while
    // a non-zero value gives "low detail" mode.
    //

    CONFIG_VARIABLE(detaillevel),

    //!
    // Number of sounds that will be played simultaneously.
    //

    CONFIG_VARIABLE(snd_channels),

    //!
    // If non-zero, sound effects will have their pitch varied up or
    // down by a random amount during play. If zero, sound effects
    // play back at their default pitch.
    //

    CONFIG_VARIABLE(snd_pitchshift),
};

static default_collection_t doom_defaults =
{
    doom_defaults_list,
    arrlen(doom_defaults_list),
    "default.json",
};

// Search a collection for a variable

static default_t *SearchCollection(default_collection_t *collection, char *name)
{
    int i;

    for (i=0; i<collection->numdefaults; ++i) 
    {
        if (!strcmp(name, collection->defaults[i].name))
        {
            return &collection->defaults[i];
        }
    }

    return NULL;
}


static void SaveDefaultCollection(default_collection_t *collection)
{
    SDFile *f = playdate->file->open(collection->filename, kFileWrite);
    if (f == NULL)
        return;

    json_encoder encoder;
    playdate->json->initEncoder(&encoder, (json_writeFunc *) playdate->file->write, f, 1);

    encoder.startTable(&encoder);

    default_t *defaults;
    int i;
    defaults = collection->defaults;
		
    for (i=0 ; i<collection->numdefaults ; i++)
    {
        if (!defaults[i].location)
        {
            continue;
        }

        // Print the name
        encoder.addTableMember(&encoder, defaults[i].name, strlen(defaults[i].name));

        // Print the value
        encoder.writeInt(&encoder, *defaults[i].location);
    }

    encoder.endTable(&encoder);

    // Add a new line to the end
    playdate->file->write(f, "\n", 1);

    playdate->file->flush(f);
    playdate->file->close(f);
}

static void SetVariable(default_t *def, char *value)
{
    *def->location = atoi(value);
}

static boolean ReadUntilDelim(FILE *f, char delim, char *buf, size_t size) {
    int i = 0;
    for (;;) {
        int c = fgetc(f);

        if (c == EOF)
            return false;

        if (i == size)
            return false;

        if (c == delim) {
            buf[i] = 0;
            return true;
        }

        buf[i++] = c;
    }
}

typedef struct {
    default_collection_t *collection;
    int depth;
} decoder_userdata_t;

static void before_sublist(json_decoder *decoder, const char *name, json_value_type type) {
    decoder_userdata_t *userdata = decoder->userdata;
    ++userdata->depth;
}

static void *after_sublist(json_decoder *decoder, const char *name, json_value_type type) {
    decoder_userdata_t *userdata = decoder->userdata;
    --userdata->depth;
    return NULL;
}

static void after_value(json_decoder *decoder, const char *key, json_value value) {
    decoder_userdata_t *userdata = decoder->userdata;

    if (userdata->depth != 1)
        return;

    for (int i = 0; i < userdata->collection->numdefaults; i++) {
        if (!strcmp(key, userdata->collection->defaults[i].name)) {
            if (userdata->collection->defaults[i].location) {
                *userdata->collection->defaults[i].location = json_intValue(value);
                return;
            }
        }
    }
}

static void LoadDefaultCollection(default_collection_t *collection)
{
    SDFile *f = playdate->file->open(collection->filename, kFileReadData);
    if (f == NULL)
        return;

    decoder_userdata_t userdata;
    userdata.collection = collection;
    userdata.depth = 0;

    json_decoder decoder = {
        .willDecodeSublist = before_sublist,
        .didDecodeTableValue = after_value,
        .didDecodeSublist = after_sublist,
        .userdata = &userdata
    };

    playdate->json->decode(&decoder, (json_reader){
        .read = (json_readFunc *) playdate->file->read,
        .userdata = f,
    }, NULL);

    playdate->file->close(f);
}

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
    SaveDefaultCollection(&doom_defaults);
}

//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
    LoadDefaultCollection(&doom_defaults);
}

// Get a configuration file variable by its name

static default_t *GetDefaultForName(char *name)
{
    default_t *result;

    // Try the main list and the extras

    result = SearchCollection(&doom_defaults, name);

    // Not found? Internal error.

    if (result == NULL)
    {
        I_Error("Unknown configuration variable: '%s'", name);
    }

    return result;
}

//
// Bind a variable to a given configuration file variable, by name.
//

void M_BindIntVariable(char *name, int *location)
{
    default_t *variable;

    variable = GetDefaultForName(name);
    variable->location = location;
}

// Set the value of a particular variable; an API function for other
// parts of the program to assign values to config variables by name.

boolean M_SetVariable(char *name, char *value)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->location)
    {
        return false;
    }

    SetVariable(variable, value);

    return true;
}

// Get the value of a variable.

int M_GetIntVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->location)
    {
        return 0;
    }

    return *variable->location;
}

//
// Calculate the path to the directory to use to store save games.
// Creates the directory as necessary.
//

char *M_GetSaveGameDir(char *iwadname)
{
    char *savegamedir;
    char *topdir;
    // ~/.local/share/chocolate-doom/savegames

    topdir = "savegames";
    M_MakeDirectory(topdir);

    // eg. ~/.local/share/chocolate-doom/savegames/doom2.wad/

    savegamedir = M_StringJoin(topdir, DIR_SEPARATOR_S, iwadname,
                                DIR_SEPARATOR_S, NULL);

    M_MakeDirectory(savegamedir);

    return savegamedir;
}

// temporary.
void M_CheckUnboundConfig(void) {
    for (int i = 0; i < doom_defaults.numdefaults; i++) {
        if (!doom_defaults.defaults[i].location) {
            playdate->system->logToConsole("%s UNBOUND\n", doom_defaults.defaults[i].name);
        }
    }
}

