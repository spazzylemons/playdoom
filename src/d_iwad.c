//
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
//     Search for and locate an IWAD file, and initialize according
//     to the IWAD type.
//

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "doomkeys.h"
#include "d_iwad.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

static const iwad_t iwads[] =
{
    { "doom2.wad",    doom2,     commercial, "Doom II" },
    { "plutonia.wad", pack_plut, commercial, "Final Doom: Plutonia Experiment" },
    { "tnt.wad",      pack_tnt,  commercial, "Final Doom: TNT: Evilution" },
    { "doom.wad",     doom,      retail,     "Doom" },
    { "doom1.wad",    doom,      shareware,  "Doom Shareware" },
    { "freedoom2.wad", doom2,    commercial, "Freedoom: Phase 2" },
    { "freedoom1.wad", doom,     retail,     "Freedoom: Phase 1" },
};

// Array of locations to search for IWAD files
//
// "128 IWAD search directories should be enough for anybody".

#define MAX_IWAD_DIRS 128

static boolean iwad_dirs_built = false;
static char *iwad_dirs[MAX_IWAD_DIRS];
static int num_iwad_dirs = 0;

static void AddIWADDir(char *dir)
{
    if (num_iwad_dirs < MAX_IWAD_DIRS)
    {
        iwad_dirs[num_iwad_dirs] = dir;
        ++num_iwad_dirs;
    }
}

// Returns true if the specified path is a path to a file
// of the specified name.

static boolean DirIsFile(char *path, char *filename)
{
    size_t path_len;
    size_t filename_len;

    path_len = strlen(path);
    filename_len = strlen(filename);

    return path_len >= filename_len + 1
        && path[path_len - filename_len - 1] == DIR_SEPARATOR
        && !M_strcasecmp(&path[path_len - filename_len], filename);
}

// Check if the specified directory contains the specified IWAD
// file, returning the full path to the IWAD if found, or NULL
// if not found.

static char *CheckDirectoryHasIWAD(char *dir, char *iwadname)
{
    char *filename; 
    char *probe;

    // As a special case, the "directory" may refer directly to an
    // IWAD file if the path comes from DOOMWADDIR or DOOMWADPATH.

    probe = M_FileCaseExists(dir);
    if (DirIsFile(dir, iwadname) && probe != NULL)
    {
        return probe;
    }

    // Construct the full path to the IWAD if it is located in
    // this directory, and check if it exists.

    if (!strcmp(dir, "."))
    {
        filename = M_StringDuplicate(iwadname);
    }
    else
    {
        filename = M_StringJoin(dir, DIR_SEPARATOR_S, iwadname, NULL);
    }

    probe = M_FileCaseExists(filename);
    if (probe != NULL)
    {
        return probe;
    }

    Z_Free(filename);

    return NULL;
}

// Search a directory to try to find an IWAD
// Returns the location of the IWAD if found, otherwise NULL.

static char *SearchDirectoryForIWAD(char *dir, int mask, GameMission_t *mission)
{
    char *filename;
    size_t i;

    for (i=0; i<arrlen(iwads); ++i) 
    {
        if (((1 << iwads[i].mission) & mask) == 0)
        {
            continue;
        }

        filename = CheckDirectoryHasIWAD(dir, iwads[i].name);

        if (filename != NULL)
        {
            *mission = iwads[i].mission;

            return filename;
        }
    }

    return NULL;
}

// When given an IWAD with the '-iwad' parameter,
// attempt to identify it by its name.

static GameMission_t IdentifyIWADByName(char *name, int mask)
{
    size_t i;
    GameMission_t mission;
    char *p;

    p = strrchr(name, DIR_SEPARATOR);

    if (p != NULL)
    {
        name = p + 1;
    }

    mission = none;

    for (i=0; i<arrlen(iwads); ++i)
    {
        // Check if the filename is this IWAD name.

        // Only use supported missions:

        if (((1 << iwads[i].mission) & mask) == 0)
            continue;

        // Check if it ends in this IWAD name.

        if (!M_strcasecmp(name, iwads[i].name))
        {
            mission = iwads[i].mission;
            break;
        }
    }

    return mission;
}

// Add IWAD directories parsed from splitting a path string containing
// paths separated by PATH_SEPARATOR. 'suffix' is a string to concatenate
// to the end of the paths before adding them.
static void AddIWADPath(char *path, char *suffix)
{
    char *left, *p;

    path = M_StringDuplicate(path);

    // Split into individual dirs within the list.
    left = path;

    for (;;)
    {
        p = strchr(left, PATH_SEPARATOR);
        if (p != NULL)
        {
            // Break at the separator and use the left hand side
            // as another iwad dir
            *p = '\0';

            AddIWADDir(M_StringJoin(left, suffix, NULL));
            left = p + 1;
        }
        else
        {
            break;
        }
    }

    AddIWADDir(M_StringJoin(left, suffix, NULL));

    Z_Free(path);
}

//
// Build a list of IWAD files
//

static void BuildIWADDirList(void)
{
    if (iwad_dirs_built)
    {
        return;
    }

    // Look in the current directory.  Doom always does this.
    AddIWADDir(".");

    // Don't run this function again.

    iwad_dirs_built = true;
}

//
// Searches WAD search paths for an WAD with a specific filename.
// 

char *D_FindWADByName(char *name)
{
    char *path;
    char *probe;
    int i;
    
    // Absolute path?

    probe = M_FileCaseExists(name);
    if (probe != NULL)
    {
        return probe;
    }

    BuildIWADDirList();

    // Search through all IWAD paths for a file with the given name.

    for (i=0; i<num_iwad_dirs; ++i)
    {
        // As a special case, if this is in DOOMWADDIR or DOOMWADPATH,
        // the "directory" may actually refer directly to an IWAD
        // file.

        probe = M_FileCaseExists(iwad_dirs[i]);
        if (DirIsFile(iwad_dirs[i], name) && probe != NULL)
        {
            return probe;
        }

        // Construct a string for the full path

        path = M_StringJoin(iwad_dirs[i], DIR_SEPARATOR_S, name, NULL);

        probe = M_FileCaseExists(path);
        if (probe != NULL)
        {
            return probe;
        }

        Z_Free(path);
    }

    // File not found

    return NULL;
}

//
// D_TryWADByName
//
// Searches for a WAD by its filename, or passes through the filename
// if not found.
//

char *D_TryFindWADByName(char *filename)
{
    char *result;

    result = D_FindWADByName(filename);

    if (result != NULL)
    {
        return result;
    }
    else
    {
        return filename;
    }
}

//
// FindIWAD
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWADs).
//

char *D_FindIWAD(int mask, GameMission_t *mission)
{
    char *result;
    char *iwadfile;
    int iwadparm;
    int i;

    // Check for the -iwad parameter

    //!
    // Specify an IWAD file to use.
    //
    // @arg <file>
    //

    iwadparm = M_CheckParmWithArgs("-iwad", 1);

    if (iwadparm)
    {
        // Search through IWAD dirs for an IWAD with the given name.

        iwadfile = myargv[iwadparm + 1];

        result = D_FindWADByName(iwadfile);

        if (result == NULL)
        {
            I_Error("IWAD file '%s' not found!", iwadfile);
        }
        
        *mission = IdentifyIWADByName(result, mask);
    }
    else
    {
        // Search through the list and look for an IWAD

        result = NULL;

        BuildIWADDirList();
    
        for (i=0; result == NULL && i<num_iwad_dirs; ++i)
        {
            result = SearchDirectoryForIWAD(iwad_dirs[i], mask, mission);
        }
    }

    return result;
}

// Find all IWADs in the IWAD search path matching the given mask.

const iwad_t **D_FindAllIWADs(int mask)
{
    const iwad_t **result;
    int result_len;
    char *filename;
    int i;

    result = Z_Malloc(sizeof(iwad_t *) * (arrlen(iwads) + 1), PU_STATIC, 0);
    result_len = 0;

    // Try to find all IWADs

    for (i=0; i<arrlen(iwads); ++i)
    {
        if (((1 << iwads[i].mission) & mask) == 0)
        {
            continue;
        }

        filename = D_FindWADByName(iwads[i].name);

        if (filename != NULL)
        {
            result[result_len] = &iwads[i];
            ++result_len;
        }
    }

    // End of list

    result[result_len] = NULL;

    return result;
}

//
// Get the IWAD name used for savegames.
//

char *D_SaveGameIWADName(GameMission_t gamemission)
{
    size_t i;

    // Determine the IWAD name to use for savegames.
    // This determines the directory the savegame files get put into.
    //
    // Note that we match on gamemission rather than on IWAD name.
    // This ensures that doom1.wad and doom.wad saves are stored
    // in the same place.

    for (i=0; i<arrlen(iwads); ++i)
    {
        if (gamemission == iwads[i].mission)
        {
            return iwads[i].name;
        }
    }

    // Default fallback:

    return "unknown.wad";
}

char *D_SuggestIWADName(GameMission_t mission, GameMode_t mode)
{
    int i;

    for (i = 0; i < arrlen(iwads); ++i)
    {
        if (iwads[i].mission == mission && iwads[i].mode == mode)
        {
            return iwads[i].name;
        }
    }

    return "unknown.wad";
}

char *D_SuggestGameName(GameMission_t mission, GameMode_t mode)
{
    int i;

    for (i = 0; i < arrlen(iwads); ++i)
    {
        if (iwads[i].mission == mission
         && (mode == indetermined || iwads[i].mode == mode))
        {
            return iwads[i].description;
        }
    }

    return "Unknown game?";
}

