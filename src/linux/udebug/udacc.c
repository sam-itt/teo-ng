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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/udebug/udacc.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 14/07/2016
 *  Modifié par:
 *
 *  Débogueur 6809 - Affichage des accumulateurs.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "mc68xx/mc6809.h"
#include "linux/gui.h"

/* labels */
static GtkWidget *label_cc;
static GtkWidget *label_ar;
static GtkWidget *label_br;
static GtkWidget *label_dp;
static GtkWidget *label_xr;
static GtkWidget *label_yr;
static GtkWidget *label_ur;
static GtkWidget *label_sr;
static GtkWidget *label_pc;

extern void dacc_GetDumpFor16Bits (char *p, int address);



static GtkWidget *label_8_bits_new (GtkWidget *hbox)
{
    GtkWidget *box;
    GtkWidget *label;

    label = gtk_label_new ("");
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
    return label;
}



static GtkWidget *label_16_bits_new (GtkWidget *vbox)
{
    GtkWidget *box;
    GtkWidget *label;

    label = gtk_label_new ("");
    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
    return label;
}



/* init:
 *  Init accumulators area.
 */
static GtkWidget *init(void)
{
    GtkWidget *hbox;
    GtkWidget *vbox;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_box_set_spacing (GTK_BOX (hbox), 20);

    label_cc = label_8_bits_new  (hbox);
    label_ar = label_8_bits_new  (hbox);
    label_br = label_8_bits_new  (hbox);
    label_dp = label_8_bits_new  (hbox);
    label_xr = label_16_bits_new (vbox);
    label_yr = label_16_bits_new (vbox);
    label_ur = label_16_bits_new (vbox);
    label_sr = label_16_bits_new (vbox);
    label_pc = label_16_bits_new (vbox);

    return vbox;
}



static void display_8_bits (GtkWidget *label, char *reg, int value)
{
    char *markup;

    markup = g_markup_printf_escaped ("<b>%s</b> %02X", reg, value);
    gtk_label_set_markup (GTK_LABEL (label), markup);
    gtk_widget_set_name (label, COURIER_DEBUG);
    g_free (markup);
}



static void display_16_bits (GtkWidget *label, char *reg, int value)
{
    char *markup;
    char string[50] = "";

    dacc_GetDumpFor16Bits (string, value);
    markup = g_markup_printf_escaped ("<b>%s</b> %s", reg, string);
    gtk_label_set_markup (GTK_LABEL (label), markup);
    gtk_widget_set_name (label, COURIER_DEBUG);
    g_free (markup);
}



/* display_accumulators:
 *  Display the 6809 registers.
 */
static void display (void)
{
    char *markup;
    char string[50] = "";
    struct MC6809_REGS regs;

    mc6809_GetRegs(&regs);

    sprintf (string, "%02X=%c%c%c%c%c%c%c%c"
                 , regs.cc
                 , regs.cc&0x80 ? 'E' : '.'
                 , regs.cc&0x40 ? 'F' : '.'
                 , regs.cc&0x20 ? 'H' : '.'
                 , regs.cc&0x10 ? 'I' : '.'
                 , regs.cc&0x08 ? 'N' : '.'
                 , regs.cc&0x04 ? 'Z' : '.'
                 , regs.cc&0x02 ? 'V' : '.'
                 , regs.cc&0x01 ? 'C' : '.');
                 
    markup = g_markup_printf_escaped ("<b>CC</b> %s", string);
    gtk_label_set_markup (GTK_LABEL (label_cc), markup);
    gtk_widget_set_name (label_cc, COURIER_DEBUG);
    g_free (markup);

    display_8_bits  (label_ar, "A", regs.ar);
    display_8_bits  (label_br, "B", regs.br);
    display_8_bits  (label_dp, "DP", regs.dp);
    display_16_bits (label_xr, " X", regs.xr);
    display_16_bits (label_yr, " Y", regs.yr);
    display_16_bits (label_ur, " U", regs.ur);
    display_16_bits (label_sr, " S", regs.sr);
    display_16_bits (label_pc, "PC", regs.pc);
}

 
/* ------------------------------------------------------------------------- */


/* udacc_Free:
 *  Free the memory used by the accumulators area.
 */
void udacc_Free(void)
{
}



/* udacc_Init:
 *  Init accumulators area.
 */
GtkWidget *udacc_Init(void)
{
    return init ();
}



/* udacc_Display:
 *  Display accumulators.
 */
void udacc_Display(void)
{
    display ();
}



