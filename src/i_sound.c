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
// DESCRIPTION:  none
//

#include "my_stdio.h"
#include <stdlib.h>

#include "doomtype.h"

#include "i_sound.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "w_wad.h"
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

#include "playdate.h"

// Whether to vary the pitch of sound effects
// Each game will set the default differently

int snd_pitchshift = -1;

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

static SoundSource *music_source;

typedef struct {
    SamplePlayer *player;
    sfxinfo_t *sfxinfo;
} channel_t;

typedef struct driver_data_s {
    int use_count;
    AudioSample *sample;
} driver_data_t;

#define NUM_CHANNELS 8
static channel_t channels[NUM_CHANNELS];

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

static boolean CacheSFX(sfxinfo_t *sfxinfo)
{
    int lumpnum;
    unsigned int lumplen;
    int samplerate;
    unsigned int length;
    byte *data;

    if (sfxinfo->driver_data != NULL) {
        ++sfxinfo->driver_data->use_count;
        return true;
    }

    // need to load the sound

    lumpnum = sfxinfo->lumpnum;
    data = W_CacheLumpNum(lumpnum, PU_SOUND);
    lumplen = W_LumpLength(lumpnum);

    // Check the header, and ensure this is a valid sound

    if (lumplen < 8
     || data[0] != 0x03 || data[1] != 0x00)
    {
        // Invalid sound

        return false;
    }

    // 16 bit sample rate field, 32 bit length field

    samplerate = (data[3] << 8) | data[2];
    length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

    // If the header specifies that the length of the sound is greater than
    // the length of the lump itself, this is an invalid sound lump

    // We also discard sound lumps that are less than 49 samples long,
    // as this is how DMX behaves - although the actual cut-off length
    // seems to vary slightly depending on the sample rate.  This needs
    // further investigation to better understand the correct
    // behavior.

    if (length > lumplen - 8 || length <= 48)
    {
        return false;
    }

    // The DMX sound library seems to skip the first 16 and last 16
    // bytes of the lump - reason unknown.

    data += 16;
    length -= 32;

    // Store the audio sample. Note - sound sample cannot be deallocated.
    AudioSample *sample = playdate->sound->sample->newSampleFromData(
        data,
        kSound8bitMono,
        samplerate,
        length
    );

    driver_data_t *driver_data = Z_Malloc(sizeof(driver_data_t), PU_STATIC, 0);
    driver_data->use_count = 1;
    driver_data->sample = sample;
    sfxinfo->driver_data = driver_data;

    return true;
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//

void I_InitSound(boolean use_sfx_prefix)
{
    for (int i = 0; i < NUM_CHANNELS; i++) {
        channels[i].player = playdate->sound->sampleplayer->newPlayer();
        channels[i].sfxinfo = NULL;
    }
}

void I_ShutdownSound(void)
{
    for (int i = 0; i < NUM_CHANNELS; i++) {
        playdate->sound->sampleplayer->freePlayer(channels[i].player);
    }
}

int I_GetSfxLumpNum(sfxinfo_t *sfxinfo)
{
    if (sfxinfo->link) {
        sfxinfo = sfxinfo->link;
    }

    char namebuf[9];
    snprintf(namebuf, sizeof(namebuf), "ds%s", sfxinfo->name);
    return W_GetNumForName(namebuf);
}

void I_UpdateSound(void)
{
    for (int handle = 0; handle < NUM_CHANNELS; handle++) {
        // Clean up sounds not playing.
        if (channels[handle].sfxinfo != NULL && !I_SoundIsPlaying(handle))
            I_StopSound(handle);
    }
}

static void CheckVolumeSeparation(int *vol, int *sep)
{
    if (*sep < 0)
    {
        *sep = 0;
    }
    else if (*sep > 254)
    {
        *sep = 254;
    }

    if (*vol < 0)
    {
        *vol = 0;
    }
    else if (*vol > 127)
    {
        *vol = 127;
    }
}

static void SetVolumeAndPan(int handle, int vol, int sep) {
    SamplePlayer *player = channels[handle].player;
    float master = vol / 127.0f;
    float left = ((254 - sep) / 254.0f) * master;
    float right = (sep / 254.0f) * master;
    playdate->sound->sampleplayer->setVolume(player, left, right);
}

void I_UpdateSoundParams(int handle, int vol, int sep)
{
    if (handle < 0 || handle >= NUM_CHANNELS)
        return;

    CheckVolumeSeparation(&vol, &sep);
    SetVolumeAndPan(handle, vol, sep);
}

int I_StartSound(sfxinfo_t *sfxinfo, int handle, int vol, int sep, int pitch)
{
    if (handle < 0 || handle >= NUM_CHANNELS)
        return -1;

    CheckVolumeSeparation(&vol, &sep);

    SamplePlayer *player = channels[handle].player;

    // Stop sound if needed.
    I_StopSound(handle);

    if (!CacheSFX(sfxinfo))
        return -1;

    AudioSample *sample = sfxinfo->driver_data->sample;
    playdate->sound->sampleplayer->setSample(player, sample);

    // TODO calculate pitch
    playdate->sound->sampleplayer->play(player, 1, 1.0f);
    SetVolumeAndPan(handle, vol, sep);

    channels[handle].sfxinfo = sfxinfo;

    return handle;
}

void I_StopSound(int handle)
{
    if (handle < 0 || handle >= NUM_CHANNELS)
        return;

    if (channels[handle].sfxinfo == NULL)
        return;

    SamplePlayer *player = channels[handle].player;
    playdate->sound->sampleplayer->stop(player);

    driver_data_t *driver_data = channels[handle].sfxinfo->driver_data;
    if (driver_data != NULL && --driver_data->use_count == 0) {
        playdate->sound->sample->freeSample(driver_data->sample);
        channels[handle].sfxinfo->driver_data = NULL;

        // move sound into cache so we don't leak memory
        lumpinfo_t *lump = lumpinfo[channels[handle].sfxinfo->lumpnum];
        Z_ChangeTag(lump->cache, PU_CACHE);

        Z_Free(driver_data);
    }

    channels[handle].sfxinfo = NULL;
}

boolean I_SoundIsPlaying(int handle)
{
    if (handle < 0 || handle >= NUM_CHANNELS)
        return false;

    SamplePlayer *player = channels[handle].player;
    return playdate->sound->sampleplayer->isPlaying(player);
}

void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
}

void I_InitMusic(void)
{
    soundfont = tsf_load_filename("gm.sf2");
    if (soundfont != NULL)
        music_source = playdate->sound->addSource(SoundCallback, NULL, 0);
}

void I_ShutdownMusic(void)
{
    if (soundfont != NULL) {
        playdate->sound->removeSource(music_source);
        playdate->system->realloc(music_source, 0);

        tsf_close(soundfont);
    }
}

void I_SetMusicVolume(int volume)
{
    if (soundfont != NULL)
        tsf_set_volume(soundfont, volume / 127.0f);
}

void I_PauseSong(void)
{
    is_paused = true;
}

void I_ResumeSong(void)
{
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

void *I_RegisterSong(void *data, int len)
{
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
        printf("I_RegisterSong: Failed to load MID.\n");
    }

    return result;
}

void I_UnRegisterSong(void *handle)
{
    if (handle != NULL) {
        tml_free(handle);
    }
}

void I_PlaySong(void *handle, boolean looping)
{
    if (soundfont != NULL && handle != NULL)
    {
        I_StopSong();

        current_song = current_message = handle;
        should_loop = looping;
        msec = 0.0f;
    }
}

void I_StopSong(void)
{
    if (current_song != NULL) {
        current_song = current_message = NULL;

        tsf_reset(soundfont);
    }
}

boolean I_MusicIsPlaying(void)
{
    return current_message != NULL;
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

void UpdateMusic(void) {
    if (is_paused) {
        return;
    }

    uint16_t new_ring_write = (ring_read + (RING_BUFFER_SIZE / 2) - ring_write) & (RING_BUFFER_SIZE - 1);
    if (new_ring_write > 0) {
        FillBuffer(new_ring_write);
    }
}

void I_BindSoundVariables(void)
{
    M_BindIntVariable("snd_pitchshift",          &snd_pitchshift);
}

