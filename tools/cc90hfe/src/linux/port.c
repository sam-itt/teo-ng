/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2018 Yves Charriau, François Mouret
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
 *  Module     : linux/port.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 31/05/2015
 *
 *  Management of serial port.
 */

#ifndef SCAN_DEPEND
   #include <unistd.h>
   #include <string.h>
   #include <stdlib.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
#endif

#include "defs.h"
//#include "error.h"
#include "std.h"
#include "errors.h"
#include "serial.h"
#include "encode.h"
#include "main.h"
#include "linux/progress.h"
#include "linux/gui.h"



/* port_list:
 *  Look for a valid serial port.
 */
static struct STRING_LIST *port_list (struct STRING_LIST *list, char *path_name)
{
    int access_result;
    char *dev_name = NULL;
    int dev_num = 0;

    do {
        dev_name = std_strdup_printf ("%s%d", path_name, dev_num);
        access_result = access (dev_name, F_OK);
        if (access_result >= 0)
        {
            if (serial_Open (dev_name) == 0)
                list = std_StringListAppend (list, dev_name);
            serial_Close ();
        }
        std_free (dev_name);
        dev_num++;
    } while (access_result >= 0);

    return list;
}


/* multi_ports_console:
 *  Output in console if several ports found.
 */
static void multi_ports_console (struct STRING_LIST *list)
{
    int i;
    int choice;
    int res;
    struct STRING_LIST *list_ptr;

    printf ("%s",
         (is_fr)
         ? encode_String(
           "\nDes ports série connectés ont été trouvés.\n" \
           "Veuillez choisir le plus approprié ci-dessous.\n")
         : "\nSome connected serial ports have been found.\n" \
           "Please choose the appropriate one below.\n");

    for (i=1, list_ptr=list; list_ptr!=NULL; list_ptr=list_ptr->next, i++)
        (void)printf ("%d. %s\n", i, list_ptr->str);

     do {
         printf ("\n%s", is_fr?"Votre choix : "
                              :"Your choice : ");
         res = scanf (" %d", &choice);
     } while ((res == EOF) && (choice<1) && (choice>=i));

     for (i=1,list_ptr=list; i!=choice; i++)
         list_ptr=list_ptr->next;

    gui.port_name = std_free (gui.port_name);
    gui.port_name = std_strdup_printf ("%s", list_ptr->str);
}



/* multi_ports_dialog:
 *  Dialog if several ports found.
 */
static void multi_ports_dialog (struct STRING_LIST *list)
{
    char      *string;
    GtkWidget *dialog;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *area;
    GtkWidget *combo_box;

    /* dialog box */
    dialog = gtk_dialog_new_with_buttons (
                 is_fr?encode_String("Détection du port série")
                      :"Detection of the serial port",
                 GTK_WINDOW(main_window),
                 GTK_DIALOG_MODAL,
                 is_fr?"_Valider":"_OK", GTK_RESPONSE_ACCEPT,
                 NULL);

    gtk_window_set_resizable (GTK_WINDOW(dialog), FALSE);
    area = gtk_dialog_get_content_area (GTK_DIALOG(dialog));

    /* vertical box */
    box=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_container_set_border_width( GTK_CONTAINER(box), 10);
    gtk_container_add( GTK_CONTAINER(area), box);

    /* empty label for port message */
    label = gtk_label_new (NULL);
    gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
    gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);

    /* set label markup */
    if (is_fr)
        gtk_label_set_markup (GTK_LABEL(label),
            encode_String (
            "<b>Des ports série connectés ont été trouvés.</b>\n\n" \
            "Veuillez choisir le plus approprié ci-dessous.\n"));
    else
        gtk_label_set_markup (GTK_LABEL(label),
            "<b>Some connected serial ports have been found.</b>\n\n" \
            "Please choose the appropriate one below.\n");

    /* create and fill combo box */
    combo_box = gtk_combo_box_text_new ();
    for (; list != NULL; list = list->next)
          gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(combo_box), NULL,
                                     list->str);
    gtk_box_pack_start (GTK_BOX(box), combo_box, FALSE, FALSE, 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX(combo_box), 0);

    /* select a port */
    gtk_widget_show_all (dialog);
    (void)gtk_dialog_run (GTK_DIALOG(dialog));
    string = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(combo_box));
    gtk_widget_destroy (dialog);

    gui.port_name = std_free (gui.port_name);
    gui.port_name = std_strdup_printf ("%s", string);
    g_free (string);
}



/*
 * Crée la fenêtre pour la détection des ports série
 */
static int detect_port (void)
{
    int err = 0;
    struct STRING_LIST *list = NULL;

    /* create device list */
    list = port_list (list, "/dev/ttyS");
    list = port_list (list, "/dev/ttyUSB");

    switch (std_StringListLength (list))
    {
        /* if no device found, error */
        case 0 :
            err = error_Message (CC90HFE_ERROR_PORT_NONE, NULL);
            break;

        /* if only 1 device found, take it */
        case 1 :
            gui.port_name = std_free (gui.port_name);
            gui.port_name = std_strdup_printf ("%s", (char *)list->str);
            break;

        /* if several devices found, choose one */
        default :
            if (windowed_mode != 0)
                multi_ports_dialog (list);
            else
                multi_ports_console (list);
            break;
    }
    std_StringListFree (list);
    return err;
}


/* ------------------------------------------------------------------------- */


/* port_Open:
 *  Open the serial port.
 */
int port_Open (void)
{
    int err = 0;

    if ((gui.port_name == NULL)
     || (serial_Open (gui.port_name) < 0))
    {
        serial_Close ();
        err = detect_port ();
    }
    serial_Close ();

    if (err == 0)
        err = serial_Open (gui.port_name);

    return err;
}



/* port_Close:
 *  Close the serial port.
 */
void port_Close (void)
{
    serial_Close ();
}

