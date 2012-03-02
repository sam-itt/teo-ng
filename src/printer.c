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
 *  Copyright (C) 1997-2001 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : printer.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 22/03/2001
 *  Modifié par: Eric Botcazou 30/03/2001
 *
 *  Emulation de l'imprimante PR 90-612.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "intern/hardware.h"


/* variables générales */
static int printer_online = FALSE;
static int printer_nlq = FALSE;
static int graphic_mode_enabled = FALSE;
static int graphic_mode = 0;
static int data = 0;
static int last_data_time = 0;

/* mode texte */
static FILE *paper = NULL;
static int sheet_counter = 0;

/* mode graphique */
static FILE *gfx_paper = NULL;
static int gfx_sheet_counter = 0;
static int line_counter = 0;
static int data_counter = 0;



/**********************************/
/******** sortie graphique ********/
/**********************************/


/* fputw: 
 *  Helper pour écrire en little endian un entier 16-bit
 *  quel que soit son format natif.
 */
static void fputw(int val, FILE *file)
{
    unsigned char buffer[2];

    buffer[0] = (unsigned char) val;
    buffer[1] = (unsigned char) (val>>8);

    fwrite(buffer, 1, 2, file);
}



/* fputl:
 *  Helper pour écrire en little endian un entier 32-bit
 *  quel que soit son format natif.
 */
static void fputl(int val, FILE *file)
{
    unsigned char buffer[4];

    buffer[0] = (unsigned char) val;
    buffer[1] = (unsigned char) (val>>8);
    buffer[2] = (unsigned char) (val>>16);
    buffer[3] = (unsigned char) (val>>24);

    fwrite(buffer, 1, 4, file);
} 


#define GFX_WIDTH  320


/* OpenGraphicMode:
 *  Ouvre le mode d'impression graphique.
 */
static int OpenGraphicMode(void)
{
    char filename[13];

    sprintf(filename, "pict%03d.bmp", gfx_sheet_counter);
    gfx_paper = fopen(filename, "wb");

    if (!gfx_paper)
    {
        graphic_mode = FALSE;
        return -1;
    }

    /* file header */
    fputw(0x4D42, gfx_paper);      /* bfType ("BM") */
    fputl(0, gfx_paper);           /* bfSize        */
    fputw(0, gfx_paper);           /* bfReserved1   */
    fputw(0, gfx_paper);           /* bfReserved2   */
    fputl(62, gfx_paper);          /* bfOffBits     */

    /* info header */
    fputl(40, gfx_paper);          /* biSize          */
    fputl(GFX_WIDTH, gfx_paper);   /* biWidth         */
    fputl(0, gfx_paper);           /* biHeight        */
    fputw(1, gfx_paper);           /* biPlanes        */
    fputw(1, gfx_paper);           /* biBitCount      */
    fputl(0, gfx_paper);           /* biCompression   */
    fputl(0, gfx_paper);           /* biSizeImage     */
    fputl(3790, gfx_paper);        /* biXPelsPerMeter */
    fputl(3780, gfx_paper);        /* biYPelsPerMeter */
    fputl(0, gfx_paper);           /* biClrUsed       */
    fputl(0, gfx_paper);           /* biClrImportant  */

    /* color header */ 
    fputl(0xFFFFFF, gfx_paper);    /* bcRGBBackground */
    fputl(0, gfx_paper);           /* bcRGBForeground */

    line_counter = 0;
    data_counter = 0;

    return 0;
}



/* PutGraphicData:
 *  Imprime un octet graphique.
 */
static void PutGraphicData(int data)
{
    if (!gfx_paper)
        if (OpenGraphicMode() != 0)
            return;

    fputc(data, gfx_paper);

    data_counter++;
    if (data_counter == GFX_WIDTH/8)
    {
        line_counter++;
        data_counter = 0;
    }
}



/* CloseGraphicMode:
 *  Ferme le mode d'impression graphique.
 */
static void CloseGraphicMode(void)
{
    char filename[13];
    int i;

    if (!gfx_paper)
        return;

    if (data_counter)
    {
        for (i=data_counter; i<GFX_WIDTH/8; i++)
            fputc(0xFF, gfx_paper);

        line_counter++;
    }

    fclose(gfx_paper);

    sprintf(filename, "pict%03d.bmp", gfx_sheet_counter);

    gfx_paper = fopen(filename, "rb+");
    fseek(gfx_paper, 2, SEEK_SET);
    fputl(62+GFX_WIDTH/8*line_counter, gfx_paper);   /* bfSize      */
    fseek(gfx_paper, 22, SEEK_SET);
    fputl(line_counter, gfx_paper);                  /* biHeight    */
    fseek(gfx_paper, 34, SEEK_SET);
    fputl(GFX_WIDTH/8*line_counter, gfx_paper);      /* biSizeImage */

    fclose(gfx_paper);
    gfx_paper = NULL;
    gfx_sheet_counter++;
}



/**********************************/
/********* sortie texte ***********/
/**********************************/


/* OpenTextMode:
 *  Ouvre le mode d'impression texte.
 */
static int OpenTextMode(void)
{
    char filename[13];

    sprintf(filename, "sheet%03d.txt", sheet_counter);
    paper = fopen(filename, "w");

    if (!paper)
        return -1;

    return 0;
}



/* PutTextData:
 *  Imprime un octet de texte.
 */
static void PutTextData(int data)
{
    if (!paper)
        if (OpenTextMode() != 0)
            return;

    fputc(data, paper);
}



/* CloseTextMode:
 *  Ferme le mode d'impression texte.
 */
static void CloseTextMode(void)
{
    if (!paper)
        return;

    fputc('\n', paper);
    fclose(paper);
    paper = NULL;
    sheet_counter++;
}



/**********************************/
/********** commandes *************/
/**********************************/


/* WriteData:
 *  Ecrit un octet sur le port de donnée.
 */
void pr90612_WriteData(int mask, int value)
{
    data = (value & mask) | (data & (mask^0xFF));
}



/* EjectPaper:
 *  Ejecte le papier de l'imprimante.
 */
void pr90612_EjectPaper(void)
{
    if (graphic_mode)
        CloseGraphicMode();
    else
        CloseTextMode();
}


#define GRAPHIC_MODE_DELAY  0.08*TO8_CPU_FREQ


/* SetStrobe:
 *  Change l'état de la STROBE.
 */
void pr90612_SetStrobe(int state)
{
    if (!printer_online)
        return;

    if (state)
    {
        mc6846.prc &= 0xBF;  /* BUSY à 0 */
        return;
    }

    if (graphic_mode)
    {
        if ((mc6809_clock() - last_data_time) < GRAPHIC_MODE_DELAY)
        {
            last_data_time = mc6809_clock();
            PutGraphicData(data);
            mc6846.prc |= 0x40;  /* BUSY à 1 */
            return;
        }
        else
        {
            last_data_time = 0;
            CloseGraphicMode();
            graphic_mode = FALSE;
        }
    }
    
    /* text mode */
    switch (data)
    {
        case 7:  /* Copy */
            if (!graphic_mode && graphic_mode_enabled)
            {
                CloseTextMode();
                graphic_mode = TRUE;
                last_data_time = mc6809_clock();
            }
            else
                PutTextData(data);
            break;

        case 12:  /* Form Feed */
            CloseTextMode();
            break;

        default:
            PutTextData(data);
            break;
    }

    mc6846.prc |= 0x40;  /* BUSY à 1 */
}



/**********************************/
/******** partie publique *********/
/**********************************/


/* PrinterSendCommand:
 *  Envoie une commande à l'imprimante.
 */
int to8_PrinterSendCommand(int command)
{
    switch (command)
    {
        case TO8_PRINTER_ONLINE:
            if (printer_online)
            {
                mc6846.prc |= 0x40;  /* BUSY à 1 */
                printer_online = FALSE;
                printer_nlq = FALSE;
            }
            else
                printer_online = TRUE;

            break;

        case TO8_PRINTER_LINE_FEED:
            if (!printer_online && paper)
                fputs("\n\r", paper);
            break;

        case TO8_PRINTER_FORM_FEED: /* ou NLQ */
            if (printer_online)
                printer_nlq = !printer_nlq;
            else 
                pr90612_EjectPaper();
            break;
    }

    return 0;
}



/* PrinterGetState:
 *  Lit l'état des voyants de l'imprimante.
 */
void to8_PrinterGetState(int leds[3])
{
   leds[0] = printer_nlq;
   leds[1] = TRUE;
   leds[2] = printer_online;
}



/* PrinterEnableGraphicMode:
 *  Active/désactive le mode d'impression graphique.
 */
void to8_PrinterEnableGraphicMode(int state)
{
    if (!state)
        CloseGraphicMode();

    graphic_mode_enabled = state;
}

