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
 *                          Jérémie Guillaume, François Mouret,
 *                          Samuel Devulder
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
 *  Module     : to8.c
 *  Version    : 1.8.2
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 03/11/2003
 *               François Mouret 25/09/2006 26/01/2010 18/03/2012
 *                               02/11/2012
 *               Gilles Fétis 27/07/2011
 *               Samuel Devulder 05/02/2012
 *
 *  Module de pilotage de l'émulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "defs.h"
#include "to8.h"
#include "error.h"
#include "image.h"
#include "ini.h"
#include "std.h"
#include "hardware.h"
#include "media/disk.h"
#include "media/joystick.h"
#include "media/keyboard.h"
#include "media/cass.h"
#include "media/mouse.h"
#include "media/printer.h"
#include "mc68xx/mc6809.h"

int is_fr=0;

/* fonctions importables requises */
void (*to8_SetColor)(int, int, int, int);
void (*to8_DrawGPL)(int, int, int, int);
void (*to8_PutSoundByte)(unsigned long long int, unsigned char);
void (*to8_SetPointer)(int);

/* fonctions importables optionnelles */
void (*to8_SetBorderColor)(int, int)=NULL;
void (*to8_DrawBorderLine)(int, int)=NULL;
void (*to8_SetKeyboardLed)(int)=NULL;
void (*to8_SetDiskLed)(int)=NULL;
int  (*to8_DirectReadSector)(int, int, int, int, unsigned char [])=NULL;
int  (*to8_DirectWriteSector)(int, int, int, int, const unsigned char [])=NULL;
int  (*to8_DirectFormatTrack)(int, int, const unsigned char [])=NULL;

/* variables publiques */
int to8_new_video_params;

/* variables privées */
static int to8_alive = 0;



/* LoadFile:
 *  Charge un fichier de taille donnée.
 */
static int LoadFile(const char filename[], unsigned char dest[], int size)
{
    FILE *file;

    if ((file=fopen(filename,"rb")) == NULL)
        return error_Message(TO8_CANNOT_FIND_FILE, filename);

    if (fread(dest, sizeof(char), size, file) != (size_t)size) {
        fclose(file);
        return error_Message(TO8_CANNOT_OPEN_FILE, filename);
    }
    fclose(file);

    return 0;
}



/* InitMemory:
 *  Initialisation de la carte mémoire et chargement des ROMS.
 */
static int InitMemory(void)
{
    register int i;

    /* 64 ko de ROM logiciels */
    for (i=0; i<mem.rom.nbank; i++)
        if ((mem.rom.bank[i] = malloc(mem.rom.size*sizeof(uint8))) == NULL)
            return error_Message(TO8_BAD_ALLOC, NULL); 

    /* 512 ko de RAM */
    for (i=0; i<mem.ram.nbank; i++)
        if ((mem.ram.bank[i] = calloc(mem.ram.size, sizeof(uint8))) == NULL)
            return error_Message(TO8_BAD_ALLOC, NULL);

    /* 16 ko de ROM moniteur */
    for (i=0; i<mem.mon.nbank; i++)
        if ((mem.mon.bank[i] = malloc(mem.mon.size*sizeof(uint8))) == NULL)
            return error_Message(TO8_BAD_ALLOC, NULL);

    for (i=0; i<mem.rom.nbank; i++)
        if (LoadFile(mem.rom.filename[i], mem.rom.bank[i], mem.rom.size) < 0)
            return ERR_ERROR;

    for (i=0; i<mem.mon.nbank; i++)
        if (LoadFile(mem.mon.filename[i], mem.mon.bank[i], mem.mon.size) < 0)
            return ERR_ERROR;

    /* modification de la page d'affichage de la date */
    mem.rom.bank[3][0x25D3]=TO8_TRAP_CODE;
    mem.rom.bank[3][0x2619]=0x21;

    /* modification de la page de réglage de la palette de couleurs */
    mem.rom.bank[3][0x3579]=TO8_TRAP_CODE;
    mem.rom.bank[3][0x3685]=TO8_TRAP_CODE;
    mem.rom.bank[3][0x38EB]=0x7E;
    mem.rom.bank[3][0x38EC]=0x39;
    mem.rom.bank[3][0x38ED]=0x10;
    mem.rom.bank[3][0x395F]=0x12;
    mem.rom.bank[3][0x3960]=0x12;
    mem.rom.bank[3][0x396F]=0x12;
    mem.rom.bank[3][0x3970]=0x12;

    LOCK_DATA(mem.ram.bank[0], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.ram.bank[1], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.ram.bank[2], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.ram.bank[3], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.mon.bank[1], sizeof(uint8)*mem.rom.size);

    return 0;
}



/* DoLines:
 *  Fait tourner le MC6809E en le synchronisant sur le
 *  faisceau vidéo ligne par ligne.
 */
static void DoLines(int nlines, mc6809_clock_t *exact_clock)
{
    register int i;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        *exact_clock+=(LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES);
        mc6809_TimeExec(*exact_clock);

        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        *exact_clock+=WINDOW_LINE_CYCLES;
        mc6809_TimeExec(*exact_clock);

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        *exact_clock+=(RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES);
        mc6809_TimeExec(*exact_clock);
    }
}



/* DoLines_debug:
 *  Fait tourner le MC6809E en le synchronisant sur le
 *  faisceau vidéo ligne par ligne.
 */
static int DoLines_debug(int nlines, mc6809_clock_t *exact_clock)
{
    register int i;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        *exact_clock+=(LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES);
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;

        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        *exact_clock+=WINDOW_LINE_CYCLES;
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        *exact_clock+=(RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES);
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
    }
    return 1;
}



/* DoLinesAndRetrace:
 *  Fait tourner le MC6809E en retraçant l'écran ligne par ligne.
 */
static void DoLinesAndRetrace(int nlines, mc6809_clock_t *exact_clock)
{
    register int i,j,k;
             int vram_addr=0;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        if (to8_DrawBorderLine) 
        {
            *exact_clock+=LEFT_SHADOW_CYCLES;
            mc6809_TimeExec(*exact_clock);

            to8_DrawBorderLine(TO8_LEFT_BORDER, TOP_BORDER_LINES+i);
            *exact_clock+=LEFT_BORDER_CYCLES;
	        mc6809_TimeExec(*exact_clock);
        }
        else
        {
            *exact_clock+=(LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES);
            mc6809_TimeExec(*exact_clock);
        }
 
        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        /* on découpe la ligne en petits groupes d'octets dont la
           longueur est LINE_GRANULARITY */
        for (j=0; j<WINDOW_LINE_CYCLES/LINE_GRANULARITY; j++)
        {
            for (k=0; k<LINE_GRANULARITY; k++)
                DrawGPL(vram_addr++);

            *exact_clock+=LINE_GRANULARITY;
            mc6809_TimeExec(*exact_clock);
        }

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        if (to8_DrawBorderLine)
	        to8_DrawBorderLine(TO8_RIGHT_BORDER, TOP_BORDER_LINES+i);

        *exact_clock+=RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES;
        mc6809_TimeExec(*exact_clock);
    }
}


/* DoLinesAndRetrace_debug:
 *  Fait tourner le MC6809E en retraçant l'écran ligne par ligne.
 */
static int DoLinesAndRetrace_debug(int nlines, mc6809_clock_t *exact_clock)
{
    register int i,j,k;
             int vram_addr=0;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        if (to8_DrawBorderLine) 
        {
            *exact_clock+=LEFT_SHADOW_CYCLES;
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;

            to8_DrawBorderLine(TO8_LEFT_BORDER, TOP_BORDER_LINES+i);
            *exact_clock+=+LEFT_BORDER_CYCLES;
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
        }
        else
        {
            *exact_clock+=(LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES);
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
        }
 
        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        /* on découpe la ligne en petits groupes d'octets dont la
           longueur est LINE_GRANULARITY */
        for (j=0; j<WINDOW_LINE_CYCLES/LINE_GRANULARITY; j++)
        {
            for (k=0; k<LINE_GRANULARITY; k++)
                DrawGPL(vram_addr++);

            *exact_clock+=LINE_GRANULARITY;
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
        }

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        if (to8_DrawBorderLine)
	        to8_DrawBorderLine(TO8_RIGHT_BORDER, TOP_BORDER_LINES+i);

        *exact_clock+=RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES;
         if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
    }
    return 1;
}



#ifndef TO8_NO_BORDER

/* DoBorderLinesAndRetrace:
 *  Fait tourner le MC6809E en retraçant le pourtour ligne par ligne.
 */
static void DoBorderLinesAndRetrace(int border, int nlines, mc6809_clock_t *exact_clock)
{
    register int i,j,k;
             int offset=0;

    if (border==BOTTOM_BORDER)
        offset=TOP_BORDER_LINES+WINDOW_LINES;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        *exact_clock+=LEFT_SHADOW_CYCLES;
        mc6809_TimeExec(*exact_clock);

        to8_DrawBorderLine(TO8_LEFT_BORDER, offset+i);
        *exact_clock+=+LEFT_BORDER_CYCLES;
        mc6809_TimeExec(*exact_clock);

        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        /* on découpe la ligne en petits groupes d'octets dont la
           longueur est LINE_GRANULARITY */
        for (j=0; j<WINDOW_LINE_CYCLES/LINE_GRANULARITY; j++)
        {
            for (k=0; k<LINE_GRANULARITY; k++)
                to8_DrawBorderLine(j*LINE_GRANULARITY+k, offset+i);

            *exact_clock+=LINE_GRANULARITY;
            mc6809_TimeExec(*exact_clock);
        }

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        to8_DrawBorderLine(TO8_RIGHT_BORDER, offset+i);
        *exact_clock+=RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES;
        mc6809_TimeExec(*exact_clock);
    }
}



/* DoBorderLinesAndRetrace_debug:
 *  Fait tourner le MC6809E en retraçant le pourtour ligne par ligne.
 */
static int DoBorderLinesAndRetrace_debug (int border, int nlines, mc6809_clock_t *exact_clock)
{
    register int i,j,k;
             int offset=0;

    if (border==BOTTOM_BORDER)
        offset=TOP_BORDER_LINES+WINDOW_LINES;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        *exact_clock+=LEFT_SHADOW_CYCLES;
        if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;

        to8_DrawBorderLine(TO8_LEFT_BORDER, offset+i);
        *exact_clock+=+LEFT_BORDER_CYCLES;
        if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;

        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        /* on découpe la ligne en petits groupes d'octets dont la
           longueur est LINE_GRANULARITY */
        for (j=0; j<WINDOW_LINE_CYCLES/LINE_GRANULARITY; j++)
        {
            for (k=0; k<LINE_GRANULARITY; k++)
                to8_DrawBorderLine(j*LINE_GRANULARITY+k, offset+i);

            *exact_clock+=LINE_GRANULARITY;
            if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
        }

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        to8_DrawBorderLine(TO8_RIGHT_BORDER, offset+i);
        *exact_clock+=RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES;
        if (mc6809_TimeExec_debug(*exact_clock)<0) return 0;
    }
    return 1;
}
#endif



/* ------------------------------------------------------------------------- */


/* Reset:
 *  Simule un appui sur le bouton reset du TO8.
 */
void to8_Reset(void)
{
    mc6809_Reset();
}



/* ColdReset:
 *  Simule un dé/rebranchement du TO8.
 */
void to8_ColdReset(void)
{
    /* initialisation du PIA 6846 système */
    mc6846_Init(&mc6846, 0x80, 0xE5, 0x3D);

    /* initialisation du PIA 6821 système */
    mc6821_Init(&pia_int.porta, 0, 0);

    /* les bits 3-7 (émulés sur le TO8) sont à 1 en entrée
       (voir le manuel technique TO8/9/9+ page 44) */
    mc6821_Init(&pia_int.portb, 0, 0xFC);

    /* initialisation du PIA 6821 musique et jeux */
    mc6821_Init(&pia_ext.porta, 0xC0, 0xFF);
    mc6821_Init(&pia_ext.portb, 0xC0, 0xCF);

    /* initialisation du gate array mode page */
    memset(&mode_page, 0, sizeof(struct GATE_ARRAY));

    /* initialisation des pages mémoire */
    mempager.cart.page     = 0;
    mempager.cart.rom_page = 0;
    mempager.cart.ram_page = 0;
    mempager.cart.update();

    mempager.screen.page  = 1;
    mempager.screen.vram_page = 0;
    mempager.screen.update();

    mempager.system.page = 1;
    mempager.system.update();

    mempager.data.reg_page = 0;
    mempager.data.pia_page = 2;
    mempager.data.update();
    
    mempager.mon.page = 0;
    mempager.mon.update();

    /* initialisation du gate array contrôleur de disquettes */
    memset(&disk_ctrl, 0, sizeof(struct DISK_CTRL));

    /* flags de reset à froid */
    STORE_WORD(0x5FC1, 0);  /* menu principal */
    STORE_WORD(0x60FE, 0);  /* moniteur       */

    mc6809_Reset();
}




/* DoFrame:
 *  Fait tourner le TO8 pendant une trame vidéo.
 */
int to8_DoFrame(int debug)
{
    screen_clock=mb.exact_clock;

    if (debug == TRUE)
    {
        if (to8_new_video_params)
        {
            to8_new_video_params=FALSE;
            mb.direct_screen_mode=FALSE;

            /* début de la frame vidéo: bordure haute de l'écran */
#ifndef TO8_NO_BORDER
            if (to8_DrawBorderLine)
            {
                if (DoLines_debug(TOP_SHADOW_LINES, &mb.exact_clock) == 0)
                    return 0;
                if (DoBorderLinesAndRetrace_debug(TOP_BORDER, TOP_BORDER_LINES, &mb.exact_clock) == 0)
                    return 0;
            }
            else
#endif
                if (DoLines_debug(TOP_SHADOW_LINES+TOP_BORDER_LINES, &mb.exact_clock) == 0)
                    return 0;

            /* fenêtre centrale de l'écran */
            mode_page.lp4|=0x80;
            if (DoLinesAndRetrace_debug(WINDOW_LINES, &mb.exact_clock) == 0)
                return 0;
            mode_page.lp4&=0x7F;

            /* bordure du bas de l'écran et remontée du faisceau */
#ifndef TO8_NO_BORDER
            if (to8_DrawBorderLine)
            {
                if (DoBorderLinesAndRetrace_debug(BOTTOM_BORDER, BOTTOM_BORDER_LINES, &mb.exact_clock) == 0)
                    return 0;
    	        if (DoLines_debug(BOTTOM_SHADOW_LINES, &mb.exact_clock) == 0)
    	            return 0;
            }
            else
#endif
                if (DoLines_debug(BOTTOM_BORDER_LINES+BOTTOM_SHADOW_LINES, &mb.exact_clock) == 0)
                    return 0;
        }
        else
        {
            /* début de la frame vidéo: bordure haute de l'écran */
            if (DoLines_debug(TOP_SHADOW_LINES+TOP_BORDER_LINES, &mb.exact_clock) == 0)
                return 0;

            /* fenêtre centrale de l'écran */
            mode_page.lp4|=0x80;

            if (mb.direct_screen_mode)
            {
                if (DoLines_debug(WINDOW_LINES, &mb.exact_clock) == 0)
                    return 0;
            }
            else
            {
                mb.direct_screen_mode=TRUE;
                if (DoLinesAndRetrace_debug(WINDOW_LINES, &mb.exact_clock) == 0)
                    return 0;
            }

            mode_page.lp4&=0x7F;

            /* bordure du bas de l'écran et remontée du faisceau */
            if (DoLines_debug(BOTTOM_BORDER_LINES+BOTTOM_SHADOW_LINES, &mb.exact_clock) == 0)
                return 0;
        }
    }
    else
    {
        if (to8_new_video_params)
        {
            to8_new_video_params=FALSE;
            mb.direct_screen_mode=FALSE;

            /* début de la frame vidéo: bordure haute de l'écran */
#ifndef TO8_NO_BORDER
            if (to8_DrawBorderLine)
            {
                DoLines(TOP_SHADOW_LINES, &mb.exact_clock);
                DoBorderLinesAndRetrace(TOP_BORDER, TOP_BORDER_LINES, &mb.exact_clock);
            }
            else
#endif
                DoLines(TOP_SHADOW_LINES+TOP_BORDER_LINES, &mb.exact_clock);

            /* fenêtre centrale de l'écran */
            mode_page.lp4|=0x80;
            DoLinesAndRetrace(WINDOW_LINES, &mb.exact_clock);
            mode_page.lp4&=0x7F;

            /* bordure du bas de l'écran et remontée du faisceau */
#ifndef TO8_NO_BORDER
            if (to8_DrawBorderLine)
            {
                DoBorderLinesAndRetrace(BOTTOM_BORDER, BOTTOM_BORDER_LINES, &mb.exact_clock);
    	        DoLines(BOTTOM_SHADOW_LINES, &mb.exact_clock);
            }
            else
#endif
                DoLines(BOTTOM_BORDER_LINES+BOTTOM_SHADOW_LINES, &mb.exact_clock);
        }
        else
        {
            /* début de la frame vidéo: bordure haute de l'écran */
            DoLines(TOP_SHADOW_LINES+TOP_BORDER_LINES, &mb.exact_clock);

            /* fenêtre centrale de l'écran */
            mode_page.lp4|=0x80;

            if (mb.direct_screen_mode)
                DoLines(WINDOW_LINES, &mb.exact_clock);
            else
            {
                mb.direct_screen_mode=TRUE;
                DoLinesAndRetrace(WINDOW_LINES, &mb.exact_clock);
            }

            mode_page.lp4&=0x7F;

            /* bordure du bas de l'écran et remontée du faisceau */
            DoLines(BOTTOM_BORDER_LINES+BOTTOM_SHADOW_LINES, &mb.exact_clock);
        }
    }
    return 1;
}



/* InputReset:
 *  Remet à zéro les périphériques d'entrée.
 */
void to8_InputReset(int mask, int value)
{
    keyboard_Reset(mask, value);
    joystick_Reset();
    mouse_Reset();
}



/* Exit:
 *  Arrête l'émulateur et restitue les ressources utilisées.
 *  (il n'est pas nécessaire de l'appeler explicitement, elle est
 *   automatiquement invoquée à la sortie du programme)
 */
void to8_Exit(void)
{
    register int i;

    if (!to8_alive)
        return;

    /* Sauvegarde de l'état de l'émulateur */
    ini_Save();
    image_Save ("autosave.img");

    /* Referme l'imprimante */
    printer_Close();

    /* Nettoyage des disquettes temporaires */
    disk_UnloadAll();

    /* on libère la mémoire */
    for (i=0; i<mem.rom.nbank; i++)
        mem.rom.bank[i] = std_free (mem.rom.bank[i]);

    for (i=0; i<mem.ram.nbank; i++)
        mem.ram.bank[i] = std_free (mem.ram.bank[i]);

    for (i=0; i<mem.mon.nbank; i++)
        mem.mon.bank[i] = std_free (mem.mon.bank[i]);

    for (i=0; i<mem.cart.nbank; i++)
        mem.cart.bank[i] = std_free (mem.cart.bank[i]);

    /* Libère l'occupation du message d'erreur */
    to8_error_msg = std_free (to8_error_msg);

    to8_alive = FALSE;
}



/* Init:
 *  Initialise l'émulateur et réserve les ressources nécessaires.
 */
int to8_Init(int num_joy)
{
    /* on détecte les instances multiples */
    if (to8_alive)
        return error_Message(TO8_MULTIPLE_INIT, NULL);

    hardware_Init();

    if (InitMemory() < 0)
    {
        to8_Exit();
        return ERR_ERROR;
    }

    if (keyboard_Init(num_joy) < 0)
    {
       to8_Exit();
       return ERR_ERROR;
    }

    joystick_Init();
    mouse_Init();
    disk_Init();
    cass_Init();
    printer_Init();

    to8_alive = TRUE;
    atexit(to8_Exit);

    to8_new_video_params = FALSE;
    return 0;
}
