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
//	Cheat sequence checking.
//



#include <string.h>

#include "doomtype.h"
#include "m_cheat.h"

char last_cheat[MAX_CHEAT_LEN];

//
// CHEAT SEQUENCE PACKAGE
//

//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
int
cht_CheckCheat
(cheatseq_t*	cht)
{
    // if we make a short sequence on a cheat with parameters, this 
    // will not work in vanilla doom.  behave the same.

    size_t chars_read = 0;
    size_t param_chars_read = 0;

    for (int i = 0; i < MAX_CHEAT_LEN && last_cheat[i]; i++) {
        char key = last_cheat[i];

        if (cht->parameter_chars > 0 && strlen(cht->sequence) < cht->sequence_len)
            return false;
        
        if (chars_read < strlen(cht->sequence))
        {
            // still reading characters from the cheat code
            // and verifying.  reset back to the beginning 
            // if a key is wrong

            if (key == cht->sequence[chars_read])
                ++chars_read;
            else
                chars_read = 0;
            
            param_chars_read = 0;
        }
        else if (param_chars_read < cht->parameter_chars)
        {
            // we have passed the end of the cheat sequence and are 
            // entering parameters now 
            
            cht->parameter_buf[param_chars_read] = key;
            
            ++param_chars_read;
        }

        if (chars_read >= strlen(cht->sequence)
        && param_chars_read >= cht->parameter_chars)
        {
            chars_read = param_chars_read = 0;

            return true;
        }
    }
    
    // cheat not matched yet

    return false;
}

void
cht_GetParam
( cheatseq_t*	cht,
  char*		buffer )
{
    memcpy(buffer, cht->parameter_buf, cht->parameter_chars);
}


