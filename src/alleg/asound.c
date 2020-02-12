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
 *  Copyright (C) 1997-2018 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret, Samuel Devulder
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
 *  Module     : alleg/asound.c
 *  Version    : 1.8.5
 *  Créé par   : Eric Botcazou avril 1999
 *  Modifié par: Eric Botcazou 24/09/2001
 *               Samuel Devulder 23/03/2010
 *               François Mouret 08/2011 19/10/2012 28/12/2012 20/11/2017
 *               Samuel Cuella   02/2020
 *
 *  Gestion de l'émulation sonore du TO8.
 */
#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <allegro.h>
#endif

#include "teo.h"
#include "main.h"
#include "gettext.h"


static AUDIOSTREAM *stream;
static int sound_freq;
static int sound_buffer_size;
static unsigned char *sound_buffer;
static int last_index;
static unsigned char last_data;


/* silence_sound:
 *  Silence the sound.
 */
static void silence_sound (void)
{
    /* last_data must be properly initialized, otherwise
     * the sound of other applications is altered */
    last_data = 128;
}



/* voice_get_position_callback:
 *  Helper pour détecter le bon fonctionnement du streaming audio.
 */
static void voice_get_position_callback(void)
{
    if (voice_get_position(stream->voice))
        teo.sound_enabled=TRUE;
}



/* PutSoundByte:
 *  Place un octet dans le tampon arrière du streaming audio.
 */
static void PutSoundByte(unsigned long long int clock, unsigned char data)
{
    int index=(clock%TEO_CYCLES_PER_FRAME)*sound_freq/TEO_CPU_FREQ;

    /* Dans le cas où le nombre de cycles éxécutés pendant une frame dépasse la valeur
       théorique, on bloque l'index à sa valeur maximale */
    if (index < last_index)
        index=sound_buffer_size;

    memset (&sound_buffer[last_index], last_data, index-last_index);

    last_index=index;
    last_data=data;
}


/* ------------------------------------------------------------------------- */


/* asound_Start:
 *  Lance le streaming audio.
 */
void asound_Start(void)
{
    voice_start(stream->voice);
}



/* asound_Stop:
 *  Arrête le streaming audio.
 */
void asound_Stop(void)
{
    voice_stop(stream->voice);
}



/* asound_GetVolume:
 *  Lit le volume du streaming audio.
 */
int asound_GetVolume(void)
{
    return voice_get_volume(stream->voice);
}



/* asound_SetVolume:
 *  Fixe le volume du streaming audio.
 */
void asound_SetVolume(int val)
{
    voice_set_volume(stream->voice, val);
}



/* asound_Play:
 *  Echange les tampons avant et arrière du streaming audio.
 */
void asound_Play(void)
{
    char *buffer_ptr;

    /* on remplit la fin du buffer avec le dernier byte déposé */
    memset (&sound_buffer[last_index], last_data, sound_buffer_size-last_index);

    last_index=0;

    while ((buffer_ptr=get_audio_stream_buffer(stream)) == NULL)
		rest(10); /* 1/2 vbl pour ne pas utiliser 100% de CPU */

    memcpy(buffer_ptr, sound_buffer, sound_buffer_size);
    
    free_audio_stream_buffer(stream);
}



/* asound_Init:
 *  Initialise le module de streaming audio.
 */
void asound_Init(int freq)
{
    sound_freq = freq;
    sound_buffer_size = sound_freq/TEO_FRAME_FREQ;
    sound_buffer = malloc(sizeof(unsigned char)*sound_buffer_size);

    teo_PutSoundByte=PutSoundByte;
    teo_SilenceSound=silence_sound;

    teo.sound_enabled=FALSE;

    main_ConsoleOutput(_("Sound initialization..."));

    /* pas de compensation de volume */
    set_volume_per_voice(0);

    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) == 0)
    {
         /* test de fonctionnement du streaming */
         stream=play_audio_stream(sound_buffer_size, 8, FALSE, sound_freq, 128, 128);
         rest_callback(100, voice_get_position_callback);  /* 100 ms */
         voice_stop(stream->voice);
    }

    silence_sound ();
    last_index = 0;

    main_ConsoleOutput(teo.sound_enabled ? "ok\n" : _("error\n"));
}
