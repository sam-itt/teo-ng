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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2018 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret,
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
 *  Version    : 1.8.5
 *  Cr�� par   : Gilles F�tis
 *  Modifi� par: Eric Botcazou 03/11/2003
 *               Fran�ois Mouret 25/09/2006 26/01/2010 18/03/2012
 *                               02/11/2012 18/09/2013 10/05/2014
 *                               31/07/2016 20/10/2017
 *               Gilles F�tis 27/07/2011
 *               Samuel Devulder 05/02/2012
 *
 *  Module de pilotage de l'�mulateur.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
#endif

#include "defs.h"
#include "teo.h"
#include "errors.h"
#include "image.h"
#include "ini.h"
#include "std.h"
#include "logsys.h"
#include "hardware.h"
#include "media/disk.h"
#include "media/joystick.h"
#include "media/keyboard.h"
#include "media/cass.h"
#include "media/mouse.h"
#include "media/printer.h"
#include "mc68xx/mc6809.h"
#include "mc68xx/mc6804.h"

int is_fr=0;

/* fonctions importables requises */
void (*teo_SetColor)(int, int, int, int);
void (*teo_DrawGPL)(int, int, int, int);
void (*teo_PutSoundByte)(unsigned long long int, unsigned char);
void (*teo_SilenceSound)(void);
void (*teo_SetPointer)(int);

/* fonctions importables optionnelles */
int  (*teo_DebugBreakPoint)(int pc) = NULL;
void (*teo_SetBorderColor)(int, int)=NULL;
void (*teo_DrawBorderLine)(int, int)=NULL;
void (*teo_SetKeyboardLed)(int)=NULL;
void (*teo_SetDiskLed)(int)=NULL;
int  (*teo_DirectIsDiskWritable)(int)=NULL;
int  (*teo_DirectReadSector)(int, int, int, int, unsigned char [])=NULL;
int  (*teo_DirectWriteSector)(int, int, int, int, const unsigned char [])=NULL;
int  (*teo_DirectFormatTrack)(int, int, const unsigned char [])=NULL;

/* variables publiques */
int teo_new_video_params;

/* variables priv�es */
static int teo_alive = 0;



/* LoadFile:
 *  Charge un fichier de taille donn�e.
 */
static int LoadFile(const char filename[], unsigned char dest[], int size)
{
    FILE *file;

    if ((file=fopen(filename,"rb")) == NULL)
        return error_Message(TEO_ERROR_FILE_NOT_FOUND, filename);

    if (fread(dest, sizeof(char), size, file) != (size_t)size) {
        fclose(file);
        return error_Message(TEO_ERROR_FILE_OPEN, filename);
    }
    fclose(file);

    return 0;
}



/* memory_hard_reset:
 *  Reset � froid de toute la RAM.
 */
static void memory_hard_reset(void)
{
    int bank;
    int addr;

    for (bank=0; bank<mem.ram.nbank; bank++)
    {
        for (addr=0; addr<mem.ram.size; addr+=512)
        {
            memset (&mem.ram.bank[bank][addr], 0x00, 128);
            memset (&mem.ram.bank[bank][addr+128], 0xff, 256);
            memset (&mem.ram.bank[bank][addr+384], 0x00, 128);
        }
    }
}



/* InitMemory:
 *  Initialisation de la carte m�moire et chargement des ROMS.
 */
static int InitMemory(void)
{
    register int i;

    /* 64 ko de ROM logiciels */
    for (i=0; i<mem.rom.nbank; i++)
        if ((mem.rom.bank[i] = malloc(mem.rom.size*sizeof(uint8))) == NULL)
            return error_Message(TEO_ERROR_ALLOC, NULL); 

    /* 512 ko de RAM */
    for (i=0; i<mem.ram.nbank; i++)
        if ((mem.ram.bank[i] = calloc(mem.ram.size, sizeof(uint8))) == NULL)
            return error_Message(TEO_ERROR_ALLOC, NULL);
    memory_hard_reset();

    /* 16 ko de ROM moniteur */
    for (i=0; i<mem.mon.nbank; i++)
        if ((mem.mon.bank[i] = malloc(mem.mon.size*sizeof(uint8))) == NULL)
            return error_Message(TEO_ERROR_ALLOC, NULL);

    for (i=0; i<mem.rom.nbank; i++){
        if (LoadFile(mem.rom.filename[i], mem.rom.bank[i], mem.rom.size) < 0){
            log_msgf(LOG_ERROR,"Couldn't load file %s\n",mem.rom.filename[i]);
            return TEO_ERROR;
        }
    }

    for (i=0; i<mem.mon.nbank; i++){
        if (LoadFile(mem.mon.filename[i], mem.mon.bank[i], mem.mon.size) < 0){
            log_msgf(LOG_ERROR,"Couldn't load file %s\n",mem.rom.filename[i]);
            return TEO_ERROR;
        }
    }

    /* modification de la page d'affichage de la date */
    mem.rom.bank[3][0x25D3]=TEO_TRAP_CODE;
    mem.rom.bank[3][0x2619]=0x21;

    /* modification de la page de r�glage de la palette de couleurs */
    mem.rom.bank[3][0x3579]=TEO_TRAP_CODE;
    mem.rom.bank[3][0x3685]=TEO_TRAP_CODE;
    mem.rom.bank[3][0x38EB]=0x7E;
    mem.rom.bank[3][0x38EC]=0x39;
    mem.rom.bank[3][0x38ED]=0x10;
    mem.rom.bank[3][0x395F]=0x12;
    mem.rom.bank[3][0x3960]=0x12;
    mem.rom.bank[3][0x396F]=0x12;
    mem.rom.bank[3][0x3970]=0x12;

    /* special treatement for the reset */
    mem.mon.bank[0][0x1DC8]=TEO_TRAP_CODE;

    LOCK_DATA(mem.ram.bank[0], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.ram.bank[1], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.ram.bank[2], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.ram.bank[3], sizeof(uint8)*mem.ram.size);
    LOCK_DATA(mem.mon.bank[0], sizeof(uint8)*mem.rom.size);
    LOCK_DATA(mem.mon.bank[1], sizeof(uint8)*mem.rom.size);

    return 0;
}



/* DoLines:
 *  Fait tourner le MC6809E en le synchronisant sur le
 *  faisceau vid�o ligne par ligne.
 */
static int DoLines(int nlines, mc6809_clock_t *exact_clock)
{
    register int i;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        *exact_clock+=(LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES);
        if (mc6809_TimeExec(*exact_clock)<0) return 0;

        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        *exact_clock+=WINDOW_LINE_CYCLES;
        if (mc6809_TimeExec(*exact_clock)<0) return 0;

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        *exact_clock+=(RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES);
        if (mc6809_TimeExec(*exact_clock)<0) return 0;
    }
    return 1;
}



/* DoLinesAndRetrace:
 *  Fait tourner le MC6809E en retra�ant l'�cran ligne par ligne.
 */
static int DoLinesAndRetrace(int nlines, mc6809_clock_t *exact_clock)
{
    register int i,j,k;
             int vram_addr=0;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        if (teo_DrawBorderLine) 
        {
            *exact_clock+=LEFT_SHADOW_CYCLES;
            if (mc6809_TimeExec(*exact_clock)<0) return 0;

            teo_DrawBorderLine(TEO_LEFT_BORDER, TOP_BORDER_LINES+i);
            *exact_clock+=LEFT_BORDER_CYCLES;
            if (mc6809_TimeExec(*exact_clock)<0) return 0;
        }
        else
        {
            *exact_clock+=(LEFT_SHADOW_CYCLES+LEFT_BORDER_CYCLES);
            if (mc6809_TimeExec(*exact_clock)<0) return 0;
        }
 
        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        /* on d�coupe la ligne en petits groupes d'octets dont la
           longueur est LINE_GRANULARITY */
        for (j=0; j<WINDOW_LINE_CYCLES/LINE_GRANULARITY; j++)
        {
            for (k=0; k<LINE_GRANULARITY; k++)
                DrawGPL(vram_addr++);

            *exact_clock+=LINE_GRANULARITY;
            if (mc6809_TimeExec(*exact_clock)<0) return 0;
        }

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        if (teo_DrawBorderLine)
	        teo_DrawBorderLine(TEO_RIGHT_BORDER, TOP_BORDER_LINES+i);

        *exact_clock+=RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES;
         if (mc6809_TimeExec(*exact_clock)<0) return 0;
    }
    return 1;
}



#ifndef TEO_NO_BORDER

/* DoBorderLinesAndRetrace:
 *  Fait tourner le MC6809E en retra�ant le pourtour ligne par ligne.
 */
static int DoBorderLinesAndRetrace(int border, int nlines, mc6809_clock_t *exact_clock)
{
    register int i,j,k;
             int offset=0;

    if (border==BOTTOM_BORDER)
        offset=TOP_BORDER_LINES+WINDOW_LINES;

    for (i=0; i<nlines; i++)
    {
        /* bordure gauche de la ligne */
        *exact_clock+=LEFT_SHADOW_CYCLES;
        if (mc6809_TimeExec(*exact_clock)<0) return 0;

        teo_DrawBorderLine(TEO_LEFT_BORDER, offset+i);
        *exact_clock+=LEFT_BORDER_CYCLES;
        if (mc6809_TimeExec(*exact_clock)<0) return 0;

        /* partie centrale de la ligne */
        mode_page.lp4|=0x20;

        /* on d�coupe la ligne en petits groupes d'octets dont la
           longueur est LINE_GRANULARITY */
        for (j=0; j<WINDOW_LINE_CYCLES/LINE_GRANULARITY; j++)
        {
            for (k=0; k<LINE_GRANULARITY; k++)
                teo_DrawBorderLine(j*LINE_GRANULARITY+k, offset+i);

            *exact_clock+=LINE_GRANULARITY;
            if (mc6809_TimeExec(*exact_clock)<0) return 0;
        }

        mode_page.lp4&=0xDF;

        /* bordure droite de la ligne */
        teo_DrawBorderLine(TEO_RIGHT_BORDER, offset+i);
        *exact_clock+=RIGHT_BORDER_CYCLES+RIGHT_SHADOW_CYCLES;
        if (mc6809_TimeExec(*exact_clock)<0) return 0;
    }
    return 1;
}
#endif



/* ------------------------------------------------------------------------- */


/* teo_Reset:
 *  Simulate a warm reset.
 */
void teo_Reset(void)
{
    /* Back to 40 columns display */
    mode_page.lgamod=0x00;
    teo_new_video_params=TRUE;

    if (teo_SilenceSound != NULL)
        teo_SilenceSound();

    mc6809_Reset();
}



/* teo_ColdReset:
 *  Simulate a soft cold reset.
 */
void teo_ColdReset(void)
{
    int drive;

    /* initialisation du PIA 6846 syst�me */
    mc6846_Init(&mc6846, 0x80, 0xE5, 0x3D);

    /* initialisation du PIA 6804 syst�me */
    mc6804_Init(&mc6846);

    /* initialisation du PIA 6821 syst�me */
    mc6821_Init(&pia_int.porta, 0, 0);

    /* les bits 3-7 (�mul�s sur le TO8) sont � 1 en entr�e
       (voir le manuel technique TO8/9/9+ page 44) */
    mc6821_Init(&pia_int.portb, 0, 0xFC);

    /* initialisation du PIA 6821 musique et jeux */
    mc6821_Init(&pia_ext.porta, 0xC0, 0xFF);
    mc6821_Init(&pia_ext.portb, 0xC0, 0xCF);

    /* initialisation du gate array mode page */
    memset(&mode_page, 0, sizeof(struct GATE_ARRAY));

    /* initialisation des pages m�moire */
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

    STORE_BYTE(0x60FF, 0x00);

    for (drive=0; drive<NBDRIVE; drive+=2)
    {
        disk[drive].drv->track.curr = 0;
        disk[drive].drv->track.last = TEO_DISK_INVALID_NUMBER;
    }

    teo_Reset();
}



/* teo_FullReset:
 *  Simulate an electrical cold reset.
 */
void teo_FullReset(void)
{
    memory_hard_reset();
    teo_ColdReset();
}



/* DoFrame:
 *  Fait tourner le TO8 pendant une trame vid�o.
 */
int teo_DoFrame(void)
{
    screen_clock=mb.exact_clock;

    if (teo_new_video_params)
    {
        teo_new_video_params=FALSE;
        mb.direct_screen_mode=FALSE;

        /* d�but de la frame vid�o: bordure haute de l'�cran */
#ifndef TEO_NO_BORDER
        if (teo_DrawBorderLine)
        {
            if (DoLines(TOP_SHADOW_LINES, &mb.exact_clock) == 0)
                return 0;
            if (DoBorderLinesAndRetrace(TOP_BORDER, TOP_BORDER_LINES, &mb.exact_clock) == 0)
                return 0;
        }
        else
#endif
            if (DoLines(TOP_SHADOW_LINES+TOP_BORDER_LINES, &mb.exact_clock) == 0)
                return 0;

        /* fen�tre centrale de l'�cran */
        mode_page.lp4|=0x80;
        if (DoLinesAndRetrace(WINDOW_LINES, &mb.exact_clock) == 0)
            return 0;
        mode_page.lp4&=0x7F;

        /* bordure du bas de l'�cran et remont�e du faisceau */
#ifndef TEO_NO_BORDER
        if (teo_DrawBorderLine)
        {
            if (DoBorderLinesAndRetrace(BOTTOM_BORDER, BOTTOM_BORDER_LINES, &mb.exact_clock) == 0)
                return 0;
            if (DoLines(BOTTOM_SHADOW_LINES, &mb.exact_clock) == 0)
                return 0;
            }
            else
#endif
            if (DoLines(BOTTOM_BORDER_LINES+BOTTOM_SHADOW_LINES, &mb.exact_clock) == 0)
                return 0;
    }
    else
    {
        /* d�but de la frame vid�o: bordure haute de l'�cran */
        if (DoLines(TOP_SHADOW_LINES+TOP_BORDER_LINES, &mb.exact_clock) == 0)
            return 0;

        /* fen�tre centrale de l'�cran */
        mode_page.lp4|=0x80;

        if (mb.direct_screen_mode)
        {
            if (DoLines(WINDOW_LINES, &mb.exact_clock) == 0)
                return 0;
        }
        else
        {
            mb.direct_screen_mode=TRUE;
            if (DoLinesAndRetrace(WINDOW_LINES, &mb.exact_clock) == 0)
                return 0;
        }

        mode_page.lp4&=0x7F;

        /* bordure du bas de l'�cran et remont�e du faisceau */
        if (DoLines(BOTTOM_BORDER_LINES+BOTTOM_SHADOW_LINES, &mb.exact_clock) == 0)
            return 0;
    }
    return 1;
}



/* teo_FlushFrame:
 *  Complete the frame.
 */
void teo_FlushFrame(void)
{
    if ((mb.exact_clock%TEO_CYCLES_PER_FRAME) != 0LL)
    {
        mb.exact_clock += TEO_CYCLES_PER_FRAME-
                          (mb.exact_clock%TEO_CYCLES_PER_FRAME);
        mc6809_TimeExec(mb.exact_clock);
    }
}



/* InputReset:
 *  Remet � z�ro les p�riph�riques d'entr�e.
 */
void teo_InputReset(int mask, int value)
{
    keyboard_Reset(mask, value);
    joystick_Reset();
    mouse_Reset();
}



/* Exit:
 *  Arr�te l'�mulateur et restitue les ressources utilis�es.
 *  (il n'est pas n�cessaire de l'appeler explicitement, elle est
 *   automatiquement invoqu�e � la sortie du programme)
 */
void teo_Exit(void)
{
    register int i;

    if (!teo_alive)
        return;

    /* Referme l'imprimante */
    printer_Close();

    /* on lib�re la m�moire */
    for (i=0; i<mem.rom.nbank; i++)
        mem.rom.bank[i] = std_free (mem.rom.bank[i]);

    for (i=0; i<mem.ram.nbank; i++)
        mem.ram.bank[i] = std_free (mem.ram.bank[i]);

    for (i=0; i<mem.mon.nbank; i++)
        mem.mon.bank[i] = std_free (mem.mon.bank[i]);

    for (i=0; i<mem.cart.nbank; i++)
        mem.cart.bank[i] = std_free (mem.cart.bank[i]);

    /* Free the disks structure */
    disk_Free();

    /* Lib�re l'occupation du message d'erreur */
    teo_error_msg = std_free (teo_error_msg);

    teo_alive = FALSE;
}



/* Init:
 *  Initialise l'�mulateur et r�serve les ressources n�cessaires.
 */
int teo_Init(int num_joy)
{
    /* on d�tecte les instances multiples */
    if (teo_alive)
        return error_Message(TEO_ERROR_MULTIPLE_INIT, NULL);

    hardware_Init();

    if (InitMemory() < 0)
    {
        teo_Exit();
        return TEO_ERROR;
    }

    if (keyboard_Init(num_joy) < 0)
    {
        teo_Exit();
        return TEO_ERROR;
    }

    joystick_Init();
    mouse_Init();

    if (disk_Init() < 0)
    {
        teo_Exit();
        return TEO_ERROR;
    }

    cass_Init();
    printer_Init();

    teo_alive = TRUE;
    atexit(teo_Exit);

    teo_new_video_params = FALSE;
    return 0;
}

