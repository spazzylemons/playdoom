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
//   System interface for music.
//


#include "my_stdio.h"

#include "z_zone.h"

#define TSF_IMPLEMENTATION
#define TSF_STATIC

#define TSF_MALLOC(size) Z_Malloc(size, PU_STATIC, 0)
#define TSF_REALLOC Z_Realloc
#define TSF_FREE Z_Free

#include "tsf.h"

#define TML_IMPLEMENTATION
#define TML_STATIC
#define TML_NO_STDIO

#define TML_MALLOC(size) Z_Malloc(size, PU_STATIC, 0)
#define TML_REALLOC Z_Realloc
#define TML_FREE Z_Free

#include "tml.h"

#include "memio.h"
#include "mus2mid.h"
#include "i_sound.h"

#include "playdate.h"

#define MAXMIDLENGTH (96 * 1024)

#define MIXING_FREQ TSF_SAMPLERATE
#define RING_BUFFER_SIZE 4096

static int16_t ring_buffer[RING_BUFFER_SIZE];
static uint16_t ring_write = RING_BUFFER_SIZE / 2;
static uint16_t ring_read = 0;

static tsf *soundfont;
static tml_message *current_song;
static tml_message *current_message;
static boolean should_loop;
static boolean is_paused;
static float msec;

static int SoundCallback(void *userdata, int16_t *left, int16_t *right, int length) {
    length /= 4;

    while (length-- > 0) {
        // Check if we exhausted the buffer
        if (ring_read == ring_write)
            return 0;

        for (int i = 0; i < 4; i++)
            *left++ = ring_buffer[ring_read];

        ring_read = (ring_read + 1) & (RING_BUFFER_SIZE - 1);
    }

    return 1;
}

static boolean I_SF2_InitMusic(void) {
    soundfont = tsf_load_filename("gm.sf2");
    if (soundfont == NULL)
        return false;

    playdate->sound->addSource(SoundCallback, NULL, 0);

    return true;
}

static void I_SF2_ShutdownMusic(void) {}

static void I_SF2_SetMusicVolume(int volume) {
    tsf_set_volume(soundfont, volume / 127.0f);
}

static void I_SF2_PauseSong(void) {
    is_paused = true;
}

static void I_SF2_ResumeSong(void) {
    is_paused = false;
}

static boolean IsMid(byte *mem, int len) {
    return len > 4 && !memcmp(mem, "MThd", 4);
}

static MEMFILE *ConvertMus(byte *musdata, int len)
{
    MEMFILE *instream;
    MEMFILE *outstream;
    int result;

    instream = mem_fopen_read(musdata, len);
    outstream = mem_fopen_write();

    result = mus2mid(instream, outstream);
    mem_fclose(instream);

    if (result) {
        mem_fclose(outstream);
        return NULL;
    }

    return outstream;
}

static void *I_SF2_RegisterSong(void *data, int len) {
    tml_message *result = NULL;

    if (!IsMid(data, len) || len >= MAXMIDLENGTH) {
        MEMFILE *outstream = ConvertMus(data, len);

        if (outstream == NULL) {
            result = NULL;
        } else {
            void *outbuf;
            size_t outsize;
            mem_get_buf(outstream, &outbuf, &outsize);
            result = tml_load_memory(outbuf, outsize);
            mem_fclose(outstream);
        }
    } else {
        result = tml_load_memory(data, len);
    }

    if (result == NULL) {
        printf("I_SF2_RegisterSong: Failed to load MID.\n");
    }

    return result;
}

static void I_SF2_UnRegisterSong(void *handle) {
    if (handle != NULL) {
        tml_free(handle);
    }
}

static void FillBuffer(int nsamples) {
    if (current_song == NULL)
        return;

    //Number of samples to process
    while (nsamples > 0) {
        //Loop through all MIDI messages which need to be played up until the current playback time
        msec += TSF_RENDER_EFFECTSAMPLEBLOCK * (1000.0f / TSF_SAMPLERATE);
        for (;;) {
            if (current_message == NULL) {
                if (should_loop) {
                    current_message = current_song;
                    msec = 0.0f;
                } else {
                    break;
                }
            }

            if (msec < current_message->time)
                break;

            switch (current_message->type)
            {
                case TML_PROGRAM_CHANGE: //channel program (preset) change (special handling for 10th MIDI channel with drums)
                    tsf_channel_set_presetnumber(soundfont, current_message->channel, current_message->program, (current_message->channel == 9));
                    break;
                case TML_NOTE_ON: //play a note
                    tsf_channel_note_on(soundfont, current_message->channel, current_message->key, current_message->velocity / 127.0f);
                    break;
                case TML_NOTE_OFF: //stop a note
                    tsf_channel_note_off(soundfont, current_message->channel, current_message->key);
                    break;
                case TML_PITCH_BEND: //pitch wheel modification
                    tsf_channel_set_pitchwheel(soundfont, current_message->channel, current_message->pitch_bend);
                    break;
                case TML_CONTROL_CHANGE: //MIDI controller messages
                    tsf_channel_midi_control(soundfont, current_message->channel, current_message->control, current_message->control_value);
                    break;
            }

            current_message = current_message->next;
        }

        tsf_render_short(soundfont, &ring_buffer[ring_write]);
        nsamples -= TSF_RENDER_EFFECTSAMPLEBLOCK;
        ring_write = (ring_write + TSF_RENDER_EFFECTSAMPLEBLOCK) & (RING_BUFFER_SIZE - 1);
    }
}

static void I_SF2_StopSong(void) {
    if (current_song != NULL) {
        current_song = current_message = NULL;

        tsf_reset(soundfont);
    }
}

static void I_SF2_PlaySong(void *handle, boolean looping) {
    if (handle == NULL)
        return;

    I_SF2_StopSong();

    current_song = current_message = handle;
    should_loop = looping;
    msec = 0.0f;
}

static boolean I_SF2_MusicIsPlaying(void) {
    return current_message != NULL;
}

static snddevice_t music_opl_devices[] =
{
    SNDDEVICE_ADLIB,
    SNDDEVICE_SB,
};

music_module_t music_sf2_module =
{
    music_opl_devices,
    arrlen(music_opl_devices),
    I_SF2_InitMusic,
    I_SF2_ShutdownMusic,
    I_SF2_SetMusicVolume,
    I_SF2_PauseSong,
    I_SF2_ResumeSong,
    I_SF2_RegisterSong,
    I_SF2_UnRegisterSong,
    I_SF2_PlaySong,
    I_SF2_StopSong,
    I_SF2_MusicIsPlaying,
    NULL,  // Poll
};

void UpdateMusic(void) {
    if (is_paused) {
        return;
    }

    uint16_t new_ring_write = (ring_read + (RING_BUFFER_SIZE / 2) - ring_write) & (RING_BUFFER_SIZE - 1);
    if (new_ring_write > 0) {
        FillBuffer(new_ring_write);
        // printf("%d %d %d\n", ring_write, ring_read, (ring_write - ring_read) & (RING_BUFFER_SIZE - 1));
    }
}
