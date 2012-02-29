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
 *  Copyright (C) 1997-2011 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume
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
 *  Version    : 1.8.0
 *  Créé par   : Eric Botcazou août 1999
 *  Modifié par: Eric Botcazou 24/10/2003
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011
 *
 *  Gestion de l'émulation sonore du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <fcntl.h>
   #include <errno.h>
   #include <unistd.h>
   #include <sys/ioctl.h>
   #include <sys/soundcard.h>
#endif

#include "linux/main.h"
#include "to8.h"


#define SOUND_FREQ  25600

#define FRAG_EXP 9
#define SOUND_BUFFER_SIZE (1<<FRAG_EXP)
#define DEVNAME "/dev/dsp"

static int audio_fd;
static unsigned char sound_buffer[SOUND_BUFFER_SIZE];
static int last_index;
static unsigned char last_data;



/* PutSoundByte:
 *  Place un octet dans le tampon de streaming audio.
 */
static void PutSoundByte(unsigned long long int clock, unsigned char data)
{
    register int i;
    int index=(clock%TO8_CYCLES_PER_FRAME)*SOUND_FREQ/TO8_CPU_FREQ;

    /* Dans le cas où le nombre de cycles éxécutés pendant une frame dépasse la valeur
       théorique, on bloque l'index à sa valeur maximale */
    if (index < last_index)
	index=SOUND_BUFFER_SIZE;

    for (i=last_index; i<index; i++)
        sound_buffer[i]=last_data;

    last_index=index;
    last_data=data;
}



/* InitSoundError:
 *  Erreur d'initialisation du module de streaming audio.
 */
int InitSoundError (char *error_name, char *error_string)
{
    perror(error_name);
    (void)snprintf(to8_error_msg + strlen(to8_error_msg), TO8_MESSAGE_MAX_LENGTH,
                   "%s : %s", error_name, error_string);
    if (audio_fd >= 0)
        close(audio_fd);
    teo.sound_enabled=0;
    return TO8_ERROR;
}



/* InitSound:
 *  Initialise le module de streaming audio.
 */
int InitSound(void)
{
    int frag=0x10000+FRAG_EXP;
    int format=AFMT_U8;
    int stereo=0;
    int freq=SOUND_FREQ;
    int i;

    to8_PutSoundByte=PutSoundByte;

    if (teo.sound_enabled)
    {
        printf(is_fr?"Initialisation du son...":"Sound initialization...");
        fflush(stdout);

        to8_error_msg[0] = '\0';

        audio_fd = open(DEVNAME, O_WRONLY);
        if (audio_fd < 0)
        {
            switch (errno)
            {
                case EBUSY :
                     for (i=0;(i<10)&&(audio_fd<0);i++) {
                         audio_fd = open (DEVNAME, O_WRONLY);
                         sleep (1);
                     }
                     if (audio_fd < 0)
                         strcat (to8_error_msg,
                              is_fr?"Erreur du pÃ©riphÃ©rique son.\n"
                                   :"Sound peripheral error.\n");
                     break;

                case ENOENT :
                     strcat (to8_error_msg,
                              is_fr?"Installez le package alsa-oss puis lancez \"aoss teo\".\n"
                                   :"Try install package alsa-oss then run \"aoss teo\".\n");
                     break;
            }
                     
            if (audio_fd < 0)
                return InitSoundError (DEVNAME, strerror (errno));
        }

        if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
            return InitSoundError ("SNDCTL_DSP_SETFRAGMENT", strerror (errno));

        if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) == -1)
            return InitSoundError ("SNDCTL_DSP_SETFMT", strerror (errno));

        if (format != AFMT_U8)
            return InitSoundError (is_fr?"Erreur":"Error",
                      is_fr?"son 8-bit non supportÃ©.":"8-bit sound not supported.");

        if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1)
            return InitSoundError ("SNDCTL_DSP_STEREO", strerror (errno));

        if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &freq) == -1)
            return InitSoundError ("SNDCTL_DSP_SPEED", strerror (errno));

        if (freq != SOUND_FREQ)
            return InitSoundError (is_fr?"Erreur":"Error",
                        is_fr?"frÃ©quence requise non supportÃ©e.":"frequency not supported.");

        printf("ok\n");
    }

#ifdef DEBUG
    {
	audio_buf_info info;

	ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info);

	fprintf(stderr, "frequency: %d Hz\n", freq);
	fprintf(stderr, "fragments: %d\n", info.fragstotal);
	fprintf(stderr, "fragment size: %d bytes\n", info.fragsize);
    }
#endif
    return TO8_OK;
}



/* PlaySoundBuffer:
 *  Envoie le tampon de streaming audio à la carte son.
 */
void PlaySoundBuffer(void)
{
    register int i;
    size_t res;

     /* on remplit la fin du buffer avec le dernier byte déposé */
    for (i=last_index; i<SOUND_BUFFER_SIZE; i++)
        sound_buffer[i]=last_data;

    last_index=0;

    res = write(audio_fd, sound_buffer, SOUND_BUFFER_SIZE);
}

