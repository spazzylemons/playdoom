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

#include "playdate.h"

// Whether to vary the pitch of sound effects
// Each game will set the default differently

int snd_pitchshift = -1;

// Low-level sound and music modules we are using

static music_module_t *music_module;

// Sound modules
extern music_module_t music_sf2_module;

// For OPL module:

extern opl_driver_ver_t opl_drv_ver;

typedef struct {
    SamplePlayer *player;
    sfxinfo_t *sfxinfo;
} channel_t;

#define NUM_CHANNELS 8
static channel_t channels[NUM_CHANNELS];

// Compiled-in music modules:

static music_module_t *music_modules[] =
{
    &music_sf2_module,
    NULL,
};

typedef struct driver_data_s {
    int use_count;
    AudioSample *sample;
} driver_data_t;

static boolean CacheSFX(sfxinfo_t *sfxinfo)
{
    int lumpnum;
    unsigned int lumplen;
    int samplerate;
    unsigned int length;
    byte *data;

    if (sfxinfo->driver_data != NULL) {
        ++((driver_data_t *) sfxinfo->driver_data)->use_count;
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

// Check if a sound device is in the given list of devices

static boolean SndDeviceInList(snddevice_t device, snddevice_t *list,
                               int len)
{
    int i;

    for (i=0; i<len; ++i)
    {
        if (device == list[i])
        {
            return true;
        }
    }

    return false;
}

// Initialize music.

static void InitMusicModule(void)
{
    int i;

    music_module = NULL;

    for (i=0; music_modules[i] != NULL; ++i)
    {
        // Is the music device in the list of devices supported
        // by this module?

        // Initialize the module

        if (music_modules[i]->Init())
        {
            music_module = music_modules[i];
            return;
        }
    }
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//

void I_InitSound(boolean use_sfx_prefix)
{  

    InitMusicModule();

    for (int i = 0; i < NUM_CHANNELS; i++) {
        channels[i].player = playdate->sound->sampleplayer->newPlayer();
        channels[i].sfxinfo = NULL;
    }
}

void I_ShutdownSound(void)
{
    if (music_module != NULL)
    {
        music_module->Shutdown();
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
}

void I_ShutdownMusic(void)
{

}

void I_SetMusicVolume(int volume)
{
    if (music_module != NULL)
    {
        music_module->SetMusicVolume(volume);
    }
}

void I_PauseSong(void)
{
    if (music_module != NULL)
    {
        music_module->PauseMusic();
    }
}

void I_ResumeSong(void)
{
    if (music_module != NULL)
    {
        music_module->ResumeMusic();
    }
}

void *I_RegisterSong(void *data, int len)
{
    if (music_module != NULL)
    {
        return music_module->RegisterSong(data, len);
    }
    else
    {
        return NULL;
    }
}

void I_UnRegisterSong(void *handle)
{
    if (music_module != NULL)
    {
        music_module->UnRegisterSong(handle);
    }
}

void I_PlaySong(void *handle, boolean looping)
{
    if (music_module != NULL)
    {
        music_module->PlaySong(handle, looping);
    }
}

void I_StopSong(void)
{
    if (music_module != NULL)
    {
        music_module->StopSong();
    }
}

boolean I_MusicIsPlaying(void)
{
    if (music_module != NULL)
    {
        return music_module->MusicIsPlaying();
    }
    else
    {
        return false;
    }
}

void I_BindSoundVariables(void)
{
    M_BindIntVariable("snd_pitchshift",          &snd_pitchshift);
}

