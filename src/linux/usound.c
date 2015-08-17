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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou août 1999
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011 24/01/2012 15/06/2012
 *                               19/10/2012
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

#include "hardware.h"
#include "std.h"
#include "errors.h"
#include "teo.h"

#define ALSA_DEVNAME "default"         /* nom du périphérique */
#define ALSA_SOUND_FREQ  44100         /* débit */
#define ALSA_CHANNELS 1                            /* nombre de canaux */
#define ALSA_RESAMPLE 1                            /* enable alsa-lib resampling */
#define ALSA_ACCESS SND_PCM_ACCESS_RW_INTERLEAVED  /* type d'accès */
#define ALSA_FRAME_LENGTH  TEO_CYCLES_PER_FRAME

static int last_index = 0;
static unsigned char last_data = 0x00;
static unsigned char *sound_buffer = NULL;

static snd_pcm_t *handle = NULL;
static snd_pcm_hw_params_t *hwparams;
static snd_pcm_sw_params_t *swparams;
static unsigned int rate = ALSA_SOUND_FREQ;
static int data_type;

static snd_pcm_sframes_t period_size;
static int threshold;

void usound_Close (void);



/* sound_error:
 *  Erreur d'initialisation du module de streaming audio.
 */
static int sound_error (const char *error_name, char *error_string)
{
    perror(error_name);

    teo_error_msg = std_free (teo_error_msg);
    teo_error_msg = std_strdup_printf ("%s : %s", error_name, error_string);
    
    usound_Close ();
    teo.setting.sound_enabled=0;
    return TEO_ERROR;
}


/* set_hwparams:
 *  Initialise les paramètres audio hardware.
 */
static int set_hwparams (snd_pcm_hw_params_t *params)
{
    int err;
    int dir = 0;
    snd_pcm_uframes_t size;
    unsigned int period_time = (unsigned int)ALSA_FRAME_LENGTH;  /* longueur d'une période en microsecondes */
    unsigned int buffer_time = period_time*6; /* longueur du ring buffer en microsecondes */
    snd_pcm_uframes_t buffer_size;

    /* Initialise la structure */
    if ((err = snd_pcm_hw_params_any (handle, params)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_any())");

    /* Initialise le resample */
    if ((err = snd_pcm_hw_params_set_rate_resample (handle, params, ALSA_RESAMPLE)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_rate_resample())");

    /* Initialise le type d'accès */
    if ((err = snd_pcm_hw_params_set_access (handle, params, ALSA_ACCESS)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_access())");

    /* Initialise le format */
    if ((err = snd_pcm_hw_params_set_format (handle, params, (data_type=SND_PCM_FORMAT_S16))) < 0)
        if ((err = snd_pcm_hw_params_set_format (handle, params, (data_type=SND_PCM_FORMAT_U8))) < 0)
            return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_format())");

    /* Initialise le nombre de canaux */
    if ((err = snd_pcm_hw_params_set_channels (handle, params, ALSA_CHANNELS)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_channels())");

    /* Initialise le débit en données/seconde */
    if ((err = snd_pcm_hw_params_set_rate_near (handle, params, &rate, 0)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_rate_near())");

    /* Initialise le temps du ring-buffer en microsecondes */
    if ((err = snd_pcm_hw_params_set_buffer_time_near (handle, params, &buffer_time, &dir)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_buffer_time_near())");

    /* Lit la taille du ring-buffer */
    if ((err = snd_pcm_hw_params_get_buffer_size (params, &size)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_get_buffer_size())");
    buffer_size = size;

    /* Set period size (frames) */
    if ((err = snd_pcm_hw_params_set_period_time_near (handle, params, &period_time, &dir)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_set_period_time_near())");

    /* Lit la taille de la période en frames */
    if ((err = snd_pcm_hw_params_get_period_size (params, &size, &dir)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params_get_period_size())");
    period_size = size;
    threshold = (buffer_size / period_size) * period_size;

    /* Actualise des paramètres hardware */
    if ((err = snd_pcm_hw_params (handle, params)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_hw_params())");

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
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_sw_params_current())");

    if ((err = snd_pcm_sw_params_set_start_threshold (handle, params, threshold)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_sw_params_set_start_threshold())");

    if ((err = snd_pcm_sw_params_set_avail_min (handle, params, period_size)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_sw_params_set_avail_min())");

    /* Actualise les paramètres software */
    if ((err = snd_pcm_sw_params (handle, params)) < 0)
        return sound_error (snd_strerror(err), "ALSA (snd_pcm_sw_params()");

    return 0;
}



/* put_sound_byte:
 *  Place un octet dans le tampon de streaming audio.
 */
static void put_sound_byte(unsigned long long int clock, unsigned char data)
{
    register int i;
    register char char_data;
    int index= period_size*(clock%TEO_CYCLES_PER_FRAME)/TEO_CYCLES_PER_FRAME;

    /* Dans le cas où le nombre de cycles éxécutés pendant une frame dépasse la valeur
       théorique, on bloque l'index à sa valeur maximale */
    if (index < last_index)
        index=period_size;

    if (sound_buffer != NULL)
    {
        /* fill buffer with sound data */
        switch (data_type)
        {
            case SND_PCM_FORMAT_U8 :
                if ((index-last_index) != 0)
                    memset (sound_buffer+last_index, last_data, index-last_index);
                break;
                
            case SND_PCM_FORMAT_S16 :
                char_data = last_data-128;
                for (i=last_index; i<index; i++)
                    sound_buffer[(i<<1)+1]=char_data;
                break;
        }
    }
    last_index=index;
    last_data=data;
}


/* ------------------------------------------------------------------------- */


/* usound_Close:
 *  Ferme le device audio.
 */
void usound_Close (void)
{
    if (sound_buffer != NULL)
    {
        free(sound_buffer);
        sound_buffer = NULL;
    }
    if (handle != NULL)
    {
        snd_pcm_close(handle);
        handle = NULL;
    }
}



/* usound_Init:
 *  Initialise le module de streaming audio.
 */
int usound_Init(void)
{
    int err;
    int sound_buffer_size;

    teo_PutSoundByte=put_sound_byte;

    if (teo.setting.sound_enabled)
    {
        printf(is_fr?"Initialisation du son (ALSA)..."
                    :"Sound initialization (ALSA)...");

        /* Open PCM device for playback. */
        if ((err = snd_pcm_open(&handle, ALSA_DEVNAME, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
            return sound_error (snd_strerror(err),
                                   is_fr?"Impossible d'ouvrir le pÃ©riphÃ©rique audio"
                                        :"Unable to open audio device");

        /* Initialise les paramètres hardware */
        snd_pcm_hw_params_alloca (&hwparams);
        if (set_hwparams (hwparams) < 0)
            return TEO_ERROR;

        /* Initialise les paramètres software */
        snd_pcm_sw_params_alloca (&swparams);
        if (set_swparams (swparams) < 0)
            return TEO_ERROR;

        /* Alloue le buffer de son */
        sound_buffer_size = period_size * ALSA_CHANNELS * (snd_pcm_format_physical_width(data_type) >> 3);
        sound_buffer = (unsigned char *)calloc (sound_buffer_size, sizeof(unsigned char));
        if (sound_buffer == NULL)
            return sound_error (is_fr?"Erreur audio":"Audio error",
                                is_fr?"MÃ©moire insuffisante pour le buffer"
                                     :"Insufficient memory for buffer");

        snd_pcm_prepare (handle);

        printf("ok\n");
    }
    return 0;
}

static int xrun_recovery(snd_pcm_t *handle, int err)
{
        if (err == -EPIPE) {    /* under-run */
                err = snd_pcm_prepare(handle);
                if (err < 0)
                        printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
                return 0;
        } else if (err == -ESTRPIPE) {
                while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                        sleep(1);       /* wait until the suspend flag is released */
                if (err < 0) {
                        err = snd_pcm_prepare(handle);
                        if (err < 0)
                                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
                }
                return 0;
        }
        return err;
}

static int wait_for_poll(snd_pcm_t *handle, struct pollfd *ufds, unsigned int count)
{
        unsigned short revents;
        while (1) {
                poll(ufds, count, -1);
                snd_pcm_poll_descriptors_revents(handle, ufds, count, &revents);
                if (revents & POLLERR)
                        return -EIO;
                if (revents & POLLOUT)
                        return 0;
        }
}

/* usound_Play:
 *  Envoie le tampon de streaming audio à la carte son.
 */
void usound_Play(void)
{
    int err;
    register int i;
    static struct pollfd *ufds=NULL;
    static int count;

    if (ufds==NULL) {
        count = snd_pcm_poll_descriptors_count (handle);
        if (count <= 0) {
                printf("Invalid poll descriptors count\n");
                return;
        }
        ufds = malloc(sizeof(struct pollfd) * count);
        if (ufds == NULL) {
                printf("No enough memory\n");
                return;
        }
        if ((err = snd_pcm_poll_descriptors(handle, ufds, count)) < 0) {
                printf("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
                return;
        }
    }


    if ((sound_buffer == NULL) || (handle == NULL))
        return;

    switch (data_type)
    {
        case SND_PCM_FORMAT_U8 :
            if ((period_size-last_index) != 0)
                memset (sound_buffer+last_index, last_data, period_size-last_index);
            break;

        case SND_PCM_FORMAT_S16 :
            for (i=last_index; i<period_size; i++)
                sound_buffer[(i<<1)+1]=last_data-128;
            break;
    }

    err = wait_for_poll(handle, ufds, count);
    if (err < 0) {
    	if (snd_pcm_state(handle) == SND_PCM_STATE_XRUN ||
                                    snd_pcm_state(handle) == SND_PCM_STATE_SUSPENDED) {
                                        err = snd_pcm_state(handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
                                        if (xrun_recovery(handle, err) < 0) {
                                                printf("Write error: %s\n", snd_strerror(err));
                                                exit(EXIT_FAILURE);
                                        }
                                        // init = 1;
                                } else {
                                        printf("Wait for poll failed\n");
                                        return;
                                }
    }


    if ((err = snd_pcm_writei(handle, sound_buffer, period_size)) < 0)
    {
        if (err == -EPIPE)
        {
            (void)snd_pcm_prepare(handle);
        }
        else
        if (err == -ESTRPIPE)
        {
            while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                sleep(1); /* wait until the suspend flag is released */
            if (err < 0)
                (void)snd_pcm_prepare(handle);
        }
    }
    /* snd_pcm_recover (handle, err, TRUE); */
    
    last_index=0;
}

