/*
 *    TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *    TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EEEEEEEEEE      OO          OO
 *          TT        EEEEEEEEEE      OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EE              OO          OO
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *          TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
 *
 *                  L'émulateur Thomson TO8
 *
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Module     : linux/sound.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou août 1999
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011 24/01/2012 15/06/2012
 *
 *  Gestion de l'émulation sonore du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <fcntl.h>
   #include <errno.h>
   #include <unistd.h>
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
   #include <alsa/asoundlib.h>
#endif

#include "to8.h"
#include "intern/hardware.h"


enum {
    ALSA_PLAY_NONE = 0,
    ALSA_PLAY_SOUND,
    ALSA_PLAY_CUT,
    ALSA_PLAY_RAMP,
    ALSA_PLAY_END
};

#define ALSA_DEVNAME "default"         /* nom du périphérique */
#define ALSA_SOUND_FREQ  44100         /* débit */
#define ALSA_CHANNELS 1                            /* nombre de canaux */
#define ALSA_RESAMPLE 1                            /* enable alsa-lib resampling */
#define ALSA_ACCESS SND_PCM_ACCESS_RW_INTERLEAVED  /* type d'accès */
#define ALSA_FORMAT SND_PCM_FORMAT_S16          /* format */

static int last_index = 0;
static unsigned char last_data = 0x00;
static int end_data = 0;
static unsigned char *sound_buffer = NULL;
static int sound_buffer_size = 0;

static int play_mode = ALSA_PLAY_NONE;

static snd_pcm_t *handle = NULL;
static snd_pcm_hw_params_t *hwparams;
static snd_pcm_sw_params_t *swparams;
static unsigned int rate = ALSA_SOUND_FREQ;
static snd_pcm_sframes_t period_size;
static int threshold;

static int period_table [TO8_CYCLES_PER_FRAME];

void CloseSound (void);



/* InitSoundError:
 *  Erreur d'initialisation du module de streaming audio.
 */
static int Error (const char *error_name, char *error_string)
{
    perror(error_name);
    (void)snprintf(to8_error_msg + strlen(to8_error_msg), TO8_MESSAGE_MAX_LENGTH,
                   "%s : %s", error_name, error_string);
    CloseSound ();
    return TO8_ERROR;
}



/* set_hwparams:
 *  Initialise les paramètres audio hardware.
 */
static int set_hwparams (snd_pcm_hw_params_t *params)
{
    int err;
    int dir = 0;
    snd_pcm_uframes_t size;
    unsigned int period_time = (unsigned int)TO8_CYCLES_PER_FRAME;  /* longueur d'une période en microsecondes */
    unsigned int buffer_time = period_time*6; /* longueur du ring buffer en microsecondes */
    snd_pcm_uframes_t buffer_size;

    /* Initialise la structure */
    if ((err = snd_pcm_hw_params_any (handle, params)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_any())");

    /* Initialise le resample */
    if ((err = snd_pcm_hw_params_set_rate_resample (handle, params, ALSA_RESAMPLE)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_rate_resample())");

    /* Initialise le type d'accès */
    if ((err = snd_pcm_hw_params_set_access (handle, params, ALSA_ACCESS)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_access())");

    /* Initialise le format */
    if ((err = snd_pcm_hw_params_set_format (handle, params, ALSA_FORMAT)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_format())");

    /* Initialise le nombre de canaux */
    if ((err = snd_pcm_hw_params_set_channels (handle, params, ALSA_CHANNELS)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_channels())");

    /* Initialise le débit en données/seconde */
    if ((err = snd_pcm_hw_params_set_rate_near (handle, params, &rate, 0)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_rate_near())");

    /* Initialise le temps du ring-buffer en microsecondes */
    if ((err = snd_pcm_hw_params_set_buffer_time_near (handle, params, &buffer_time, &dir)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_buffer_time_near())");

    /* Lit la taille du ring-buffer */
    if ((err = snd_pcm_hw_params_get_buffer_size (params, &size)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_get_buffer_size())");
    buffer_size = size;

    /* Set period size (frames) */
    if ((err = snd_pcm_hw_params_set_period_time_near (handle, params, &period_time, &dir)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_period_time_near())");

    /* Lit la taille de la période en frames */
    if ((err = snd_pcm_hw_params_get_period_size (params, &size, &dir)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params_get_period_size())");
    period_size = size;
    threshold = (buffer_size / period_size) * period_size;

    /* Actualise des paramètres hardware */
    if ((err = snd_pcm_hw_params (handle, params)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_hw_params())");

    return 0;
}



/* set_swparams:
 *  Initialise les paramètres audio software.
 */
static int set_swparams (snd_pcm_sw_params_t *params)
{
    int err;

    /* Récupère le swparams courant */
    if ((err = snd_pcm_sw_params_current (handle, params)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_sw_params_current())");

    if ((err = snd_pcm_sw_params_set_start_threshold (handle, params, threshold)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_sw_params_set_start_threshold())");

    if ((err = snd_pcm_sw_params_set_avail_min (handle, params, period_size)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_sw_params_set_avail_min())");

    /* Actualise les paramètres software */
    if ((err = snd_pcm_sw_params (handle, params)) < 0)
        return Error (snd_strerror(err), "ALSA (snd_pcm_sw_params()");

    return 0;
}



/* PutSoundByte:
 *  Place un octet dans le tampon de streaming audio.
 */
static void PutSoundByte(unsigned long long int clock, unsigned char data)
{
    register int i;
    register const char char_data = last_data-128;
    float delta, new_data=end_data;
    int index=period_table[clock%TO8_CYCLES_PER_FRAME];

    /* Dans le cas où le nombre de cycles éxécutés pendant une frame dépasse la valeur
       théorique, on bloque l'index à sa valeur maximale */
    if (index < last_index)
        index=period_size;

    if (sound_buffer != NULL)
    {
        if ((play_mode != ALSA_PLAY_SOUND)
         && (play_mode != ALSA_PLAY_CUT)
         && (index-last_index>1))
        {
            /* create start ramp */
            delta=(float)(((char_data<<8)-new_data)/((index-last_index)>>1));
            for (i=last_index; i<((last_index+index)>>1); i++)
            {
                new_data+=delta;
                sound_buffer[i<<1]=(char)new_data;
                sound_buffer[(i<<1)+1]=(char)((int)new_data>>8);
            }
            last_index=(index+last_index)>>1;
        }

        /* fill buffer with sound data */
        for (i=last_index; i<index; i++)
            sound_buffer[(i<<1)+1]=char_data;
        play_mode = ALSA_PLAY_SOUND;
    }

    last_index=index;
    end_data=char_data<<8;
    last_data=data;
}



/* CloseSound:
 *  Ferme le device audio.
 */
void CloseSound (void) {
    if (sound_buffer != NULL) {
        free(sound_buffer);
        sound_buffer = NULL;
    }
    if (handle != NULL) {
        snd_pcm_close(handle);
        handle = NULL;
    }
    teo.sound_enabled=0;
}



/* InitSound:
 *  Initialise le module de streaming audio.
 */
int InitSound(void)
{
    int i;
    int err;

    to8_PutSoundByte=PutSoundByte;

    if (teo.sound_enabled)
    {
        printf(is_fr?"Initialisation du son (ALSA)...":"Sound initialization (ALSA)...");

        to8_error_msg[0] = '\0';

        /* Open PCM device for playback. */
        if ((err = snd_pcm_open(&handle, ALSA_DEVNAME, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
            return Error (snd_strerror(err),
                                   is_fr?"Impossible d'ouvrir le pÃ©riphÃ©rique audio"
                                        :"Unable to open audio device");

        /* Initialise les paramètres hardware */
        snd_pcm_hw_params_alloca (&hwparams);
        if (set_hwparams (hwparams) != 0)
            return TO8_ERROR;

        snd_pcm_prepare (handle);

        /* Initialise les paramètres software */
        snd_pcm_sw_params_alloca (&swparams);
        if (set_swparams (swparams) != 0)
            return TO8_ERROR;

        /* Alloue le buffer de son */
        sound_buffer_size = period_size * ALSA_CHANNELS * (snd_pcm_format_physical_width(ALSA_FORMAT) >> 3);
        sound_buffer = (unsigned char *)calloc (sound_buffer_size, sizeof(unsigned char));
        if (sound_buffer == NULL)
            return Error (is_fr?"Erreur audio":"Audio error",
                            is_fr?"MÃ©moire insuffisante pour le buffer"
                                 :"Insufficient memory for buffer");

        /* Crée la table des offsets */
        for (i=0; i<TO8_CYCLES_PER_FRAME; i++)
            period_table[i] = (period_size*i)/TO8_CYCLES_PER_FRAME;

        play_mode = ALSA_PLAY_NONE;

        printf("ok\n");
    }
    return TO8_OK;
}



static int play_stream (void)
{
    int err;

    while ((err = snd_pcm_writei (handle, sound_buffer, period_size)) < 0)
    {
        if (err == -EAGAIN)
            continue;

        if (err == -EPIPE)
        {
            snd_pcm_prepare(handle);
            break;
        }
    }
        
    return (err > 0) ? err : period_size;
}



/* PlaySoundBuffer:
 *  Envoie le tampon de streaming audio à la carte son.
 */
int PlaySoundBuffer(void)
{
    register int i;
    static int size = 0;
    int start_data;

    if ((sound_buffer == NULL) || (handle == NULL))
        return ALSA_PLAY_NONE;

    switch (play_mode)
    {
        /* on remplit la fin du buffer avec la dernière valeur déposée */
        case ALSA_PLAY_SOUND :
            for (i=last_index; i<period_size; i++)
                sound_buffer[(i<<1)+1]=last_data-128;
            size += play_stream ();
            if (size >= threshold)
                size = 0;
            end_data = (last_data-128)<<8;
            play_mode = ALSA_PLAY_CUT;
            break;

        /* on fait tendre la valeur vers 0 et jusqu'au seuil de déclenchement */
        case ALSA_PLAY_CUT  :
        case ALSA_PLAY_RAMP :
            play_mode = ALSA_PLAY_RAMP;
            start_data = end_data;
            for (i=0; i<period_size; i++)
            {
                if (end_data<0) end_data++;
                if (end_data>0) end_data--;
                sound_buffer[i<<1]=end_data;
                sound_buffer[(i<<1)+1]=end_data>>8;
            }
            size += play_stream ();
            if (size >= threshold) 
            {
                if (start_data == 0)
                    play_mode = ALSA_PLAY_END;
                size = 0;
            }
            break;

        /* on stoppe la génération du son */
        case ALSA_PLAY_END :
            play_mode=ALSA_PLAY_NONE;
            break;
    }
    memset (sound_buffer, 0x00, sound_buffer_size); 
    last_index=0;

    return play_mode;
}

