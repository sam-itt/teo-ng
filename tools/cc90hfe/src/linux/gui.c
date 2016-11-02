/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2016 Yves Charriau, François Mouret
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
 *  Module     : linux/gui.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par: François Mouret 26/07/2013 31/05/2015
 *
 *  Archive callback.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
   #include <unistd.h>
   #include <stdlib.h>
   #include <locale.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
   #include <X11/Xlib.h>
#endif

#include "defs.h"
#include "main.h"
#include "std.h"
#include "ini.h"
#include "errors.h"
#include "option.h"
#include "encode.h"
#include "cc90.h"
#include "hfe.h"
#include "serial.h"
#include "linux/progress.h"
#include "linux/gui.h"

GtkWidget *main_window;

GtkWidget *archive_button;
GtkWidget *extract_button;
GtkWidget *install_button;
GtkWidget *progress_label;
GtkWidget *progress_bar;
GtkWidget *progress_button;
GtkWidget *not_thomson_side_0;
GtkWidget *not_thomson_side_1;
GtkWidget *retry_label;
GtkWidget *retry_spinbutton;



static void gui_EnableRetry (int flag)
{
    if ((gui.not_thomson_side[0] == TRUE) && (gui.not_thomson_side[1] == TRUE))
    {
        gtk_widget_set_sensitive (retry_label, FALSE);
        gtk_widget_set_sensitive (retry_spinbutton, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive (retry_label, flag);
        gtk_widget_set_sensitive (retry_spinbutton, flag);
    }
}



static void on_spin_value_changed (GtkSpinButton *spin, gpointer user_data)
{
    gui.read_retry_max = (int) gtk_spin_button_get_value (spin);
    /* just in case the value is not an integer */
    gtk_spin_button_set_value (spin, (gdouble)gui.read_retry_max);
}



static gboolean try_to_quit (GtkWidget *widget, GdkEvent *event,
                                   gpointer user_data)
{
    int ret;
    GtkWidget *dialog;

    if (progress_on == TRUE)
    {
        dialog = gtk_message_dialog_new_with_markup (
                             GTK_WINDOW(main_window),
                             GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                             GTK_MESSAGE_INFO,
                             GTK_BUTTONS_OK_CANCEL,
                             is_fr?encode_String(
                             "Un processus est encore en cours d'exécution.\n" \
                             "Voulez-vous vraiment quitter ?")
                            :"A process is still running\n" \
                             "Do you really want to quit ?",
                             NULL);
        ret = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (ret == GTK_RESPONSE_CANCEL)
            return TRUE;
        progress_on = FALSE;
        while (progress_dead == FALSE);
    }
    gtk_main_quit();
    return FALSE;
    (void)widget;
    (void)event;
    (void)user_data;
}



static void not_thomson_side_toggled (GtkToggleButton *button, gpointer user_data)
{
    int *p = user_data;

    *p = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button));
    gui_EnableRetry (TRUE);
}



/* udisplay_Window:
 *   Crée la fenêtre principale.
 */
static void display_window(void)
{
    GtkWidget *widget, *frame;
    GtkWidget *hbox, *hbox2, *vbox, *vbox2;

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable (GTK_WINDOW(main_window), FALSE);
    gtk_window_set_title (GTK_WINDOW(main_window), "CC90HFE");
    gtk_widget_set_size_request (main_window, 300, -1);
    gtk_widget_add_events (main_window, GDK_CONFIGURE);
    g_signal_connect (G_OBJECT (main_window), "delete-event",
                      G_CALLBACK (try_to_quit), NULL);

    /* boîte verticale principale */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(main_window), vbox);

    /* archiving button */
    archive_button = gtk_button_new_with_label(
                         is_fr?"Copier une disquette vers un fichier HFE"
                              :"Copy a floppy onto a HFE file");
    g_signal_connect(G_OBJECT(archive_button), "clicked",
                     G_CALLBACK(archive_Callback), NULL);
    gtk_box_pack_start( GTK_BOX(vbox), archive_button, FALSE, FALSE, 0);

    /* extracting button */
    extract_button = gtk_button_new_with_label(
                         is_fr?"Copier un fichier HFE vers une disquette"
                              :"Copy a HFE file onto a floppy");
    g_signal_connect(G_OBJECT(extract_button), "clicked",
                     G_CALLBACK(extract_Callback), NULL);
    gtk_box_pack_start( GTK_BOX(vbox), extract_button, FALSE, FALSE, 0);

    /* installing button */
    install_button=gtk_button_new_with_label(
               is_fr?"Installer CC90 sur le Thomson"
                    :"Install CC90 on the Thomson");
    g_signal_connect(G_OBJECT(install_button), "clicked",
                     G_CALLBACK(install_Callback), NULL);
    gtk_box_pack_start( GTK_BOX(vbox), install_button, FALSE, FALSE, 0);

    /* about button */
    widget=gtk_button_new_with_label(is_fr?"A propos...":"About...");
    g_signal_connect(G_OBJECT(widget), "clicked",
                     G_CALLBACK(about_Callback), NULL);
    gtk_box_pack_start( GTK_BOX(vbox), widget, FALSE, FALSE, 0);

    widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);

    /* Frame verticale */
    frame=gtk_frame_new ("Options");
    gtk_box_pack_start (GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    /* boîte verticale */
    vbox2=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_set_homogeneous (GTK_BOX(vbox2), TRUE);
    gtk_container_set_border_width( GTK_CONTAINER(vbox2), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox2);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start (GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

    /* Thomson side 0 disk check box */
    not_thomson_side_0=gtk_check_button_new_with_label(
        is_fr?"La face 0 n'est pas Thomson"
             :"Side 0 is not Thomson like");
    gtk_box_pack_start (GTK_BOX(hbox), not_thomson_side_0, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(not_thomson_side_0),
                                  gui.not_thomson_side[0]);
    g_signal_connect(G_OBJECT(not_thomson_side_0),
                     "toggled",
                     G_CALLBACK(not_thomson_side_toggled),
                     (gpointer)&gui.not_thomson_side[0]);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start (GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

    /* Thomson side 1 disk check box */
    not_thomson_side_1=gtk_check_button_new_with_label(
        is_fr?"La face 1 n'est pas Thomson"
             :"Side 1 is not Thomson like");
    gtk_box_pack_start (GTK_BOX(hbox), not_thomson_side_1, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(not_thomson_side_1),
                                  gui.not_thomson_side[1]);
    g_signal_connect(G_OBJECT(not_thomson_side_1),
                     "toggled",
                     G_CALLBACK(not_thomson_side_toggled),
                     (gpointer)&gui.not_thomson_side[1]);


    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start (GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

    retry_label = gtk_label_new (is_fr?"Nombre de relectures maximum : "
                                      :"Maximum count of rereadings : ");
    gtk_box_pack_start (GTK_BOX(hbox), retry_label, FALSE, FALSE, 0);

    /* Retry number spin button */
    retry_spinbutton = gtk_spin_button_new_with_range ((gdouble)1,
                                                       (gdouble)15,
                                                       (gdouble)1);
    gtk_box_pack_start (GTK_BOX(hbox), retry_spinbutton, FALSE, FALSE, 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(retry_spinbutton),
                               (gdouble)gui.read_retry_max);
    g_signal_connect(G_OBJECT(retry_spinbutton),
                     "value-changed",
                     G_CALLBACK(on_spin_value_changed),
                     (gpointer)NULL);
    gui_EnableRetry (TRUE);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* progress label */
    progress_label=gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX(hbox), progress_label, FALSE, FALSE, 0);

    /* boîte horizontale */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    /* boîte horizontale */
    hbox2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start (GTK_BOX(hbox), hbox2, TRUE, TRUE, 0);

    /* boîte verticale */
    vbox2=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_set_homogeneous (GTK_BOX(vbox2), TRUE);
    gtk_box_pack_start (GTK_BOX(hbox2), vbox2, TRUE, TRUE, 0);
    
    /* progress bar */
    progress_bar = gtk_progress_bar_new ();
    gtk_box_pack_start (GTK_BOX(vbox2), progress_bar, FALSE, FALSE, 0);

    /* boîte horizontale */
    hbox2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start (GTK_BOX(hbox), hbox2, FALSE, FALSE, 0);

    /* stop button */
    progress_button=gtk_button_new ();
    widget = gtk_image_new_from_icon_name ("process-stop", GTK_ICON_SIZE_BUTTON);
    gtk_container_add( GTK_CONTAINER(progress_button), widget);
    g_signal_connect(G_OBJECT(progress_button), "clicked",
                     G_CALLBACK(progress_Stop), NULL);
    gtk_box_pack_start( GTK_BOX(hbox2), progress_button, FALSE, FALSE, 0);

    gui_ResetProgress ();

    gtk_widget_show_all (main_window);
}


#ifdef DEBIAN_BUILD
static void copy_debian_file (const char filename[])
{
    char *src_name = NULL;
    char *dst_name = NULL;
    FILE *src_file = NULL;
    FILE *dst_file = NULL;
    int c;

    src_name = std_strdup_printf ("/usr/share/cc90hfe/%s", filename);
    dst_name = std_ApplicationPath (APPLICATION_DIR, filename);
    if ((src_name != NULL) && (*src_name != '\0')
     && (dst_name != NULL) && (*dst_name != '\0')
     && (access (dst_name, F_OK) < 0))
    {
        src_file = fopen (src_name, "rb");
        dst_file = fopen (dst_name, "wb");

        while ((src_file != NULL)
            && (dst_file != NULL)
            && ((c = fgetc(src_file)) != EOF))
        {
            fputc (c, dst_file);
        }

        src_file = std_fclose (src_file);
        dst_file = std_fclose (dst_file);
    }
    src_name = std_free (src_name);
    dst_name = std_free (dst_name);
}        
#endif


/* ------------------------------------------------------------------------- */


void gui_SetProgressText (char *text)
{
    gtk_label_set_text (GTK_LABEL(progress_label), text);
}


void gui_SetProgressBar (double value)
{
   gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress_bar), value);
}



void gui_EnableButtons (int flag)
{
    gtk_widget_set_sensitive (archive_button, flag);
    gtk_widget_set_sensitive (extract_button, flag);
    gtk_widget_set_sensitive (install_button, flag);
    gtk_widget_set_sensitive (not_thomson_side_0, flag);
    gtk_widget_set_sensitive (not_thomson_side_1, flag);
    gtk_widget_set_sensitive (progress_button, (flag == TRUE) ? FALSE : TRUE);
    gui_EnableRetry (flag);
}



void gui_ResetProgress (void)
{
    gui_SetProgressText (is_fr?"En attente.":"Waiting.");
    gui_SetProgressBar (0);
    gui_EnableButtons (TRUE);
}



void gui_ProgressUpdate (int percent)
{
    progress_Update (percent);
}



void gui_ErrorDialog (char *message)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup (
                         GTK_WINDOW(main_window),
                         GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_ERROR,
                         GTK_BUTTONS_CLOSE,
                         encode_String(message),
                         NULL);
    (void)gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);
}



int gui_InformationDialog (char *message)
{
    int ret;
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new_with_markup (
                         GTK_WINDOW(main_window),
                         GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                         GTK_MESSAGE_INFO,
                         GTK_BUTTONS_OK_CANCEL,
                         encode_String(message),
                         NULL);
    ret = gtk_dialog_run (GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);
    return (ret==GTK_RESPONSE_OK) ? TRUE : FALSE;
}



/*************************************************************************
 Main :
 *************************************************************************/
int main (int argc, char *argv[])
{
    char *lang;

    /* Repair current language */
    lang=getenv("LANG");
    if (lang==NULL)
        lang="fr_FR";        
    setlocale(LC_ALL, "");
    is_fr = (strncmp(lang,"fr",2)==0) ? 1 : 0;

    /* set current encoding */
    encode_Set (CODESET_UTF8);

#ifdef DEBIAN_BUILD
    copy_debian_file ("cc90.sap");
    copy_debian_file ("cc90.fd");
    copy_debian_file ("cc90.hfe");
#endif    

    main_InitAll ();
    main_ConsoleReadCommandLine (argc, argv);
    if (argc > 1)
    {
        main_Console ();
    }
    else
    {
        XInitThreads ();
        gtk_init (&argc, &argv);
        windowed_mode = 1;
        ini_Load ();
        display_window ();
        gtk_main();
    }
    main_FreeAll ();
    return EXIT_SUCCESS;

}

