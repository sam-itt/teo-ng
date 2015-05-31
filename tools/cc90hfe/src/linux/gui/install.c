/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2015 Yves Charriau, François Mouret
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
 *  Module     : linux/gui/install.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 31/05/2015
 *
 *  Install callback.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <sys/stat.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
#endif

#include "defs.h"
#include "std.h"
#include "cc90.h"
#include "encode.h"
#include "errors.h"
#include "main.h"
#include "linux/gui.h"


char program_text[] = 
    "<span font-family=\"Courier\"><b>" \
    "0 'SAVE\"INSTALL.BAS\",A\n" \
    "5 '\n" \
    "10 DATA \"8EE7E0CE45001A50\",&amp;H3D2\n" \
    "15 DATA \"4F5FED02CC03FFED\",&amp;H458\n" \
    "20 DATA \"84CC043CED028D2F\",&amp;H33B\n" \
    "25 DATA \"1F988D2B33CB8D27\",&amp;H321\n" \
    "30 DATA \"1F988D23E7C04A26\",&amp;H37E\n" \
    "35 DATA \"F9E684C42026FACC\",&amp;H533\n" \
    "40 DATA \"0A02E700C6031E88\",&amp;H262\n" \
    "45 DATA \"1F884A26F58D0827\",&amp;H2C8\n" \
    "50 DATA \"D5E784ECC36ECB34\",&amp;H55C\n" \
    "55 DATA \"02C601E784E6842B\",&amp;H3C9\n" \
    "60 DATA \"FCE6842BF8C6801E\",&amp;H4ED\n" \
    "65 DATA \"881F88A600485624\",&amp;H297\n" \
    "70 DATA \"F63582\",&amp;H1AD\n" \
    "75 '\n" \
    "80 LOCATE,,0:CLS:CONSOLE,,1\n" \
    "85 D=PEEK(&amp;HFFF2)\n" \
    "90 IF D&lt;128 THEN D=16384 ELSE D=0\n" \
    "95 A=D\n" \
    "100 FOR I=1 TO 13\n" \
    "105  READ A$,C:R=0\n" \
    "110  FOR J=1 TO LEN(A$)-1 STEP2\n" \
    "115   V=VAL(\"&amp;H\"+MID$(A$,J,2))\n" \
    "120   R=R+V\n" \
    "125   POKE A,V\n" \
    "130   A=A+1\n" \
    "135  NEXTJ\n" \
    "140  IF R&lt;&gt;C THEN PRINT\"Error line\";I;\"of datas (&amp;H\";HEX$(R);\"&lt;&gt;&amp;H\";HEX$(C);\")\":END\n" \
    "145 NEXTI\n" \
    "150 '\n" \
    "155 A=D/256\n" \
    "160 POKE D+1,&amp;HA7+A:POKE D+4,&amp;H05+A\n" \
    "165 EXEC D" \
    "</b></span>";


/* install_dialog:
 *  Run the install dialog.
 */
static int install_dialog (void)
{
    int response;
    GtkWidget *area;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *frame;
    GtkWidget *dialog;
    GtkWidget *scrollWindow;
    char *string;

    dialog = gtk_dialog_new_with_buttons (
                   is_fr?"Installer CC90 sans disquette"
                        :"Install Cc90 without disk",
                   GTK_WINDOW(main_window),
                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                   is_fr?"_Annuler":"_Cancel", GTK_RESPONSE_REJECT,
                   is_fr?"_Valider":"_Ok", GTK_RESPONSE_ACCEPT,
                   NULL);
    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_REJECT);
    gtk_window_set_resizable (GTK_WINDOW(dialog), FALSE);
    area = gtk_dialog_get_content_area (GTK_DIALOG(dialog));

    /* boîte verticale */
    vbox=gtk_box_new (GTK_ORIENTATION_VERTICAL,5);
    gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
    gtk_container_add (GTK_CONTAINER(area), vbox);

    /* boîte horizontale */
    hbox=gtk_box_new (GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* up label */
    label = gtk_label_new ("");
    gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
    gtk_label_set_max_width_chars (GTK_LABEL(label), 53);
    string = g_strdup_printf (
         "<i>%s</i>",
         is_fr?encode_String ("Tout d'abord, le programme INSTALL.BAS " \
                              "doit tourner sur le Thomson connecté. " \
                              "Tapez-le à la main puis sauvez-le ou " \
                              "récupérez-le à partir de la disquette " \
                              "de CC90.")
         :"First of all, the INSTALL.BAS program " \
          "must run on the connected Thomson. " \
          "Handwrite it and then save it or retrieve " \
          " it from the CC90 virtual disk."
          );
    gtk_label_set_markup (GTK_LABEL(label), string);
    g_free (string);
    gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_box_new (GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* program label */
    label = gtk_label_new ("");
    gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
    gtk_label_set_markup (GTK_LABEL(label), program_text);
    gtk_label_set_selectable (GTK_LABEL(label), TRUE);
    scrollWindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrollWindow), label);
    gtk_scrolled_window_set_min_content_width (
          GTK_SCROLLED_WINDOW (scrollWindow), 300);
    gtk_scrolled_window_set_min_content_height (
          GTK_SCROLLED_WINDOW (scrollWindow), 200);
    frame = gtk_frame_new (NULL);
    gtk_container_add(GTK_CONTAINER(frame), scrollWindow);
    gtk_box_pack_start (GTK_BOX(hbox), frame, TRUE, TRUE, 0);

    /* boîte horizontale */
    hbox=gtk_box_new (GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* down label */
    label = gtk_label_new ("");
    string = g_strdup_printf ("<i>%s.</i>",
         is_fr?encode_String("Validez lorsque vous êtes prêt")
              :"Click OK when you are ready");
    gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
    gtk_label_set_max_width_chars (GTK_LABEL(label), 53);
    gtk_label_set_markup (GTK_LABEL(label), string);
    g_free (string);
    gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    response = gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    return (response == GTK_RESPONSE_ACCEPT) ? TRUE : FALSE;
}


/* ------------------------------------------------------------------------- */


void install_Callback (GtkButton *button, gpointer user_data)
{
    if (install_dialog () == TRUE)
        if (cc90_Install() < 0)
            gui_ErrorDialog (error_msg);

    (void)user_data;
    (void)button;
}

