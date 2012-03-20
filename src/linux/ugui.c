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
 *  Module     : linux/gui.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 07/2011
 *               François Mouret 08/2011 13/01/2012
 *
 *  Interface utilisateur de l'émulateur basée sur GTK+ 2.x/3.x.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <gdk/gdkx.h>
   #include <gtk/gtk.h>
#endif

#include "linux/display.h"
#include "linux/filentry.h"
#include "linux/graphic.h"
#include "linux/main.h"
#include "linux/message.h"
#include "linux/question.h"
#include "linux/license.h"
#include "to8.h"
#include "commands.xpm"

#define NDISKS       4
#define COUNTER_MAX  999
#define ICON_SIZE   GTK_ICON_SIZE_MENU

struct GADGETLIST {
   int num;
   GtkWidget *root;
   GtkWidget *check;
   GtkWidget *access;
};

static struct GADGETLIST gadget[NDISKS];

/* fenêtre de l'interface utilisateur */
static GtkWidget *window;

/* compteur de cassettes */
static GtkWidget *spinner_cass;

extern int SetInterlaced(int);


/* retrace_callback:
 *  Callback de retraçage de la fenêtre principale.
 */
static GdkFilterReturn retrace_callback(GdkXEvent *xevent, GdkEvent *event, gpointer unused)
{
    XEvent *ev = (XEvent *) xevent;

    if (ev->type == Expose)
        RetraceScreen(ev->xexpose.x, ev->xexpose.y, ev->xexpose.width, ev->xexpose.height);

    (void) event;
    (void) unused;

    return GDK_FILTER_REMOVE;
}



static void do_exit(GtkWidget *button, int command)
{
    if (command != NONE)
        teo.command=command;

    gtk_widget_hide(window);
    gtk_main_quit();

    (void) button;
}



static int do_hide(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    gtk_widget_hide(widget);
    gtk_main_quit();
    return TRUE;

    (void) event;
    (void) user_data;
}



static void question_exit(GtkWidget *button, gpointer unused)
{
    if (question_response (is_fr?"Voulez-vous vraiment quitter l'Ã©mulateur ?":"Do you really want to quit the emulator ?", window) == TRUE)
        do_exit (NULL, QUIT);
    (void) button;
    (void) unused;
}



static void error_message(char *message)
{
    message_box (message, window);
}



static void toggle_button_true(GtkWidget *button, gpointer unused)
{
    teo.exact_speed=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    (void) unused;
}



static void read_filent_memo(GtkFileChooserButton *chooser_button, GtkWidget *entry)
{
    char *filename = file_chooser_get_filename();
    (void)chooser_button;

    if (to8_LoadMemo7(filename) == TO8_ERROR)
    {
        file_chooser_reset_filename (chooser_button);
        error_message(to8_error_msg);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY(entry), to8_GetMemo7Label());
        teo.command=COLD_RESET;
    }
}



static void click_rewind_cass(GtkWidget *button, GtkSpinButton *spinner)
{
   to8_SetK7Counter(0);

   gtk_spin_button_set_value(spinner, to8_GetK7Counter());

   (void) button;
}



static void read_filent_cass(GtkFileChooserButton *chooser_button, GtkToggleButton *button)
{
    char *name = file_chooser_get_filename();
    (void)chooser_button;
    int ret=to8_LoadK7(name);

    if (ret==TO8_ERROR)
    {
        file_chooser_reset_filename (chooser_button);
        error_message(to8_error_msg);
    }
    else
    {
        click_rewind_cass(NULL, GTK_SPIN_BUTTON(spinner_cass));
        if ((ret==TO8_READ_ONLY) && !button->active)
        {
            error_message((is_fr?"Attention: Ã©criture impossible.":"Warning: writing unavailable."));
            gtk_toggle_button_set_active(button, TRUE);
        }
    }
}



static void toggle_check_cass(GtkWidget *button, GtkSpinButton *spinner)
{
    if ( GTK_TOGGLE_BUTTON(button)->active )
        to8_SetK7Mode(TO8_READ_ONLY);
    else if (to8_SetK7Mode(TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        error_message((is_fr?"Ecriture impossible sur ce support.":"Writing unavailable on this device."));
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button), TRUE);
    }
    gtk_spin_button_set_value(spinner, to8_GetK7Counter());
}



static void change_counter_cass(GtkWidget *widget, GtkSpinButton *spinner)
{
   to8_SetK7Counter(gtk_spin_button_get_value_as_int(spinner));

   (void) widget;
}



static void set_direct_access(GtkWidget *button, struct GADGETLIST *gadget);
static void read_filent_disk(GtkFileChooserButton *chooser_button,  struct GADGETLIST *gadget)
{
    GtkWidget *image;
    int ret=to8_LoadDisk( gadget->num, file_chooser_get_filename() );

    if (gadget->access != NULL)
    {
        image = gtk_image_new_from_stock ("gtk-file", ICON_SIZE);
        gtk_button_set_image(GTK_BUTTON(gadget->access), image);
        g_signal_connect(G_OBJECT(gadget->access), "clicked", G_CALLBACK(set_direct_access), (gpointer) gadget);
    }

    if (ret==TO8_ERROR)
    {
        file_chooser_reset_filename (chooser_button);
        error_message(to8_error_msg);
    }
    else
    {
        if ((ret==TO8_READ_ONLY) && !(GTK_TOGGLE_BUTTON(gadget->check)->active) )
        {
            error_message(is_fr?"Attention: Ã©criture impossible.":"Warning: writing unavailable.");
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gadget->check), TRUE);
        }
    }
    (void)chooser_button;
}



static void toggle_check_disk(GtkWidget *button, int num)
{
    if ( GTK_TOGGLE_BUTTON(button)->active )
        to8_SetDiskMode(num, TO8_READ_ONLY);
    else if (to8_SetDiskMode(num, TO8_READ_WRITE)==TO8_READ_ONLY)
    {
        error_message(is_fr?"Ecriture impossible sur ce support.":"Writing unavailable on this device.");
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button), TRUE);
    }
}


static void set_virtual_access (GtkWidget *button, struct GADGETLIST *gadget)
{
    GtkWidget *image;

    image = gtk_image_new_from_stock ("gtk-file", ICON_SIZE);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(set_direct_access), (gpointer) gadget);

    (void)to8_VirtualSetDrive(gadget->num);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gadget->check), FALSE);
}



static void set_direct_access(GtkWidget *button, struct GADGETLIST *gadget)
{
    GtkWidget *image;

    if (gadget->root)
    {
        image = gtk_image_new_from_stock ("gtk-floppy", ICON_SIZE);
        gtk_button_set_image(GTK_BUTTON(button), image);
        g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(set_virtual_access), (gpointer) gadget);

        if (to8_DirectSetDrive(gadget->num) == TO8_READ_ONLY)
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gadget->check), TRUE);
        else
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(gadget->check), FALSE);
    }
}



static void toggle_check_interlaced (GtkWidget *button, gpointer user_data)
{
    SetInterlaced (GTK_TOGGLE_BUTTON(button)->active ? 1 : 0);
    (void)user_data;
}



static void run_about_window (GtkWidget *button, gpointer user_data)
{
    GtkWidget *about_dialog;
    const gchar *authors_list[] = { "Gilles FÃ©tis", "Eric Botcazou", "Alexandre Pukall", "JÃ©rÃ©mie Guillaume", "FranÃ§ois Mouret", "Samuel Devulder", NULL };

    about_dialog=gtk_about_dialog_new ();
    gtk_window_set_transient_for (GTK_WINDOW(about_dialog), GTK_WINDOW(window));
    gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(about_dialog), "Teo");
    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(about_dialog), TO8_VERSION_STR);
    gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG(about_dialog), "Copyright Â© 1997-2012");
    gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG(about_dialog), "Linux/X11");
    gtk_about_dialog_set_license (GTK_ABOUT_DIALOG(about_dialog), linux_license);
    gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(about_dialog), "http://nostalgies.thomsonistes.org/teo_home.html");
    gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG(about_dialog), is_fr?"Site Web de Nostalgies Thomsonistes":"Web site of Nostalgies Thomsonistes");
    gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(about_dialog), (const gchar **)&authors_list);
    (void)gtk_dialog_run (GTK_DIALOG(about_dialog));
    gtk_widget_destroy (about_dialog);
    (void)button;
    (void)user_data;
}



static void InitGUI_title (const char version_name[], GtkWidget *vbox_window)
{
    GtkWidget *hbox;
    GtkWidget *widget;
    GdkPixbuf *pixbuf;

    /* boîte horizontale du titre */
    hbox=gtk_hbox_new(TRUE,0);
    gtk_box_pack_start( GTK_BOX(vbox_window), hbox, TRUE, FALSE, 0);
    gtk_widget_show(hbox);

    /* création du pixbuf */
    pixbuf=gdk_pixbuf_new_from_xpm_data ((const char **)commands_xpm);

    /* instance du pixbuf */
    widget=gtk_image_new_from_pixbuf (pixbuf);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);

    (void)version_name;
}



static void InitGUI_settings_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *widget;
    GSList *group_speed;
    GtkWidget *frame;

    /* frame des commandes et réglages */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"RÃ©glages":"Settings"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);
    gtk_widget_show(frame);
    gtk_widget_show(widget);
    
    /* boîte verticale associée à la frame des commandes et réglages */
    vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);

    /* boîte horizontale de la vitesse */
    hbox=gtk_hbox_new(FALSE,5);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* label de la vitesse */
    widget=gtk_label_new((is_fr?"Vitesse de l'Ã©mulation:":"Emulation speed:"));
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);

    /* bouton de vitesse maximale */
    widget=gtk_radio_button_new_with_label(NULL, (is_fr?"maximale":"fast"));
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);

    /* bouton de vitesse exacte */
    group_speed=gtk_radio_button_get_group (GTK_RADIO_BUTTON(widget));
    widget=gtk_radio_button_new_with_label(group_speed, (is_fr?"exacte":"exact"));
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_button_true), (gpointer)NULL);
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), teo.exact_speed ? TRUE : FALSE);
    gtk_box_pack_end( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);

    /* boîte horizontale du mode entrelacé */
    hbox=gtk_hbox_new(FALSE,5);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* checkbox du mode entrelacé */
    widget=gtk_check_button_new_with_label((is_fr?"Affichage vidÃ©o entrelacÃ©":"Interlaced video display"));
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), FALSE);
    g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(toggle_check_interlaced), (gpointer)NULL);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);
}



static void InitGUI_commands (GtkWidget *vbox_window)
{
    GtkWidget *vbox;
    GtkWidget *widget;

    /* boîte verticale associée à la frame des commandes et réglages */
    vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(vbox_window), vbox);
    gtk_widget_show(vbox);

    /* bouton de réinitialisation */
    widget=gtk_button_new_with_label((is_fr?"RÃ©initialiser le TO8":"TO8 warm reset"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) RESET);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);

    /* bouton de redémarrage à froid */
    widget=gtk_button_new_with_label((is_fr?"RedÃ©marrer Ã  froid le TO8":"TO8 cold reset"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) COLD_RESET);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);

}



static void InitGUI_cartridge_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *entry_name;
    GtkWidget *chooser_button;
    GtkWidget *frame;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Cartouche":"Cartridge"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);
    gtk_widget_show(frame);
    gtk_widget_show(widget);

    /* boîte verticale associée à la frame de la cartouche */
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);

    /* boîte horizontale de la cartouche */
    hbox=gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* entrée fichier de la cartouche */
    chooser_button = file_chooser_button_new( "m7",
                     is_fr?"SÃ©lectionner une cartouche":"Select a cartridge",
                     is_fr?"Fichiers cartouche (.m7)":"Cartridge files (.m7)",
                     "*.m7|*.M7", to8_GetMemo7Filename(), "./memo7/", window, hbox);
    gtk_box_pack_start( GTK_BOX(hbox), chooser_button, TRUE, TRUE, 0);
    gtk_widget_show(chooser_button);

    /* boîte horizontale du nom de la cartouche */
    hbox=gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* label du nom de la cartouche */
    widget=gtk_label_new((is_fr?"Nom":"Name:"));
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    gtk_widget_show(widget);

    /* entrée du nom de la cartouche */
    entry_name=gtk_entry_new ();
    if (strlen(to8_GetMemo7Label()))
        gtk_entry_set_text (GTK_ENTRY(entry_name), to8_GetMemo7Label());
    g_signal_connect(G_OBJECT(chooser_button), "file-set", G_CALLBACK(read_filent_memo), (gpointer)entry_name);
    gtk_box_pack_start( GTK_BOX(hbox), entry_name, TRUE, TRUE, 0);
    gtk_widget_show(entry_name);
}



static void InitGUI_cassette_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *widget;
    GtkWidget *frame;
    GtkWidget *check_button;
    GtkObject *adjustment;

    /* frame de la cassette */
    frame=gtk_frame_new("prot.");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Cassette":"Tape"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);
    gtk_widget_show(frame);
    gtk_widget_show(widget);

    /* boîte verticale associée à la frame de la cassette */
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);
    gtk_widget_show(vbox);

    /* première boîte horizontale de la cassette */
    hbox=gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* molette du compteur de cassette */
    adjustment = gtk_adjustment_new (0, 0, COUNTER_MAX, 1, 10, 0);
    spinner_cass = gtk_spin_button_new ( GTK_ADJUSTMENT(adjustment), 0.5, 0);

    /* bouton protection de la cassette */
    check_button=gtk_check_button_new();
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(check_button), TRUE);
    g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(toggle_check_cass), (gpointer) spinner_cass);
    gtk_box_pack_end( GTK_BOX(hbox), check_button, FALSE, TRUE, 0);
    gtk_widget_show(check_button);

    /* entrée fichier de la cassette */
    widget = file_chooser_button_new( "k7",
                     is_fr?"SÃ©lectionner une cassette":"Select a tape",
                     is_fr?"Fichiers cassette (.k7)":"Tape files (.k7)",
                     "*.k7|*.K7", to8_GetK7Filename(), "./k7/", window, hbox);
    g_signal_connect(G_OBJECT(widget), "file-set", G_CALLBACK(read_filent_cass), (gpointer) check_button);

    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, 0);
    gtk_widget_show(widget);

    /* seconde boîte horizontale de la cassette */
    hbox=gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    /* molette du compteur de cassette */
    widget=gtk_label_new((is_fr?"Compteur:":"Counter:"));
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gtk_widget_show(widget);

    /* Connecter le signal directement au spin button ne marche pas. */
    g_signal_connect(G_OBJECT(adjustment), "value_changed", G_CALLBACK(change_counter_cass), (gpointer) spinner_cass);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner_cass), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), spinner_cass, FALSE, FALSE, 0);
    gtk_widget_show(spinner_cass);

    /* bouton rembobinage */
    widget=gtk_button_new_with_label((is_fr?"Rembobiner la cassette":"Rewind tape"));
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(click_rewind_cass), (gpointer) spinner_cass);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    gtk_widget_show(widget);
}



static void InitGUI_disk_notebook_frame (GtkWidget *notebook, int direct_disk_support)
{
    int i;
    GtkWidget *label_disk;
    GtkWidget *frame_disk;
    GtkWidget *vbox_disk;
    GtkWidget *hbox_disk[NDISKS];
    GtkWidget *image;
    
    memset (gadget, 0x00, sizeof(struct GADGETLIST)*NDISKS);

    /* frame des disquettes */
    frame_disk=gtk_frame_new("prot.");
    gtk_frame_set_shadow_type( GTK_FRAME(frame_disk), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame_disk), 0.985, 0.0);
    label_disk=gtk_label_new((is_fr?"Disquettes":"Disks"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame_disk, label_disk);
    gtk_widget_show(frame_disk);
    gtk_widget_show(label_disk);

    /* boîte verticale associée à la frame des disquettes */
    vbox_disk=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox_disk), 5);
    gtk_container_add( GTK_CONTAINER(frame_disk), vbox_disk);
    gtk_widget_show(vbox_disk);

    /* boîtes horizontales des disquettes */
    for (i=0; i<NDISKS; i++)
    {
        char num[3] = "";

        hbox_disk[i]=gtk_hbox_new(FALSE, 10);
        gtk_box_pack_start( GTK_BOX(vbox_disk), hbox_disk[i], TRUE, TRUE, 0);
        gtk_widget_show(hbox_disk[i]);

        /* boutons protection des disquettes */
        gadget[i].num=i;
        gadget[i].check=gtk_check_button_new();
        g_signal_connect(G_OBJECT(gadget[i].check), "toggled", G_CALLBACK(toggle_check_disk), (gpointer) i);
        gtk_box_pack_end( GTK_BOX(hbox_disk[i]), gadget[i].check, FALSE, TRUE,0);
        gtk_widget_show(gadget[i].check);

        /* entrée fichier des disquettes */
        sprintf(num, "%d:", i);
        gadget[i].root = file_chooser_button_new( num,
                     is_fr?"SÃ©lectionner une disquette":"Select a disk",
                     is_fr?"Fichiers disquette (.sap)":"Disk files (.sap)",
                     "*.sap|*.SAP", to8_GetDiskFilename(i), "./disks/", window, hbox_disk[i]);
        g_signal_connect(G_OBJECT(gadget[i].root), "file-set", G_CALLBACK(read_filent_disk), (gpointer) &gadget[i]);
        gtk_box_pack_start( GTK_BOX(hbox_disk[i]), gadget[i].root, TRUE, TRUE, 0);
        gtk_widget_show(gadget[i].root);
    }

    if (direct_disk_support)
    {
        /* bouton lecture directe */
        for (i=0; i<4; i++)
        {

            if ((direct_disk_support & (1<<i)) == 0)
            {
                gadget[i].root=NULL;
                gadget[i].check=NULL;
            }
            else
            {
                gadget[i].access = gtk_button_new ();
                image = gtk_image_new_from_stock ("gtk-file", ICON_SIZE);
                gtk_button_set_image(GTK_BUTTON(gadget[i].access), image);
                g_signal_connect(G_OBJECT(gadget[i].access), "clicked", G_CALLBACK(set_direct_access), (gpointer) &gadget[i]);
                gtk_box_pack_start( GTK_BOX(hbox_disk[i]), gadget[i].access, FALSE, FALSE, 0);
                gtk_widget_show(gadget[i].access);
            }
        }
    }
}


#if 0
static void InitGUI_printer_notebook_frame (GtkWidget *notebook)
{
    GtkWidget *label_printer;
    GtkWidget *frame_printer;

    /* frame de l'imprimante */
    frame_printer=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame_printer), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame_printer), 0.985, 0.0);
    label_printer=gtk_label_new("Imprimante");
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame_printer, label_printer);
    gtk_widget_show(frame_printer);
    gtk_widget_show(label_printer);
}
#endif



static void InitGUI_notebook (GtkWidget *vbox_window, int direct_disk_support)
{
    GtkWidget *notebook;

    /* notebook des périphériques */
    notebook=gtk_notebook_new();
    gtk_box_pack_start( GTK_BOX(vbox_window), notebook, TRUE, FALSE, 0);
    gtk_widget_show(notebook);
    InitGUI_settings_notebook_frame (notebook);
    InitGUI_disk_notebook_frame (notebook, direct_disk_support);
    InitGUI_cassette_notebook_frame (notebook);
    InitGUI_cartridge_notebook_frame (notebook);
#if 0
    InitGUI_printer_notebook_frame (notebook);
#endif
    /* on affiche la page des disquettes au démarrage */
    gtk_notebook_set_page( GTK_NOTEBOOK(notebook), 0);
}



static void InitGUI_buttons (GtkWidget *vbox_window)
{
    GtkWidget *hbox;
    GtkWidget *widget;

    /* boîte horizontale des boutons de sortie */
    hbox=gtk_hbutton_box_new();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);
    gtk_box_pack_start( GTK_BOX(vbox_window), hbox, TRUE, FALSE, 0);
    gtk_widget_show(hbox);

    /* bouton à propos */
    widget=gtk_button_new_from_stock (GTK_STOCK_ABOUT);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(run_about_window), (gpointer) NULL);
    gtk_container_add( GTK_CONTAINER(hbox), widget);
    gtk_widget_show(widget);
    
    /* bouton retour */
    widget=gtk_button_new_from_stock (GTK_STOCK_OK);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_exit), (gpointer) NONE);
    gtk_container_add( GTK_CONTAINER(hbox), widget);
    gtk_widget_show(widget);

    /* bouton quitter */
    widget=gtk_button_new_from_stock (GTK_STOCK_QUIT);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(question_exit), (gpointer)NULL);
    gtk_container_add( GTK_CONTAINER(hbox), widget);
    gtk_widget_show(widget);
}



static void iconify_teo_window (GtkWidget *widget, GdkEvent *event, void *user_data)
{
    if (event->window_state.new_window_state == GDK_WINDOW_STATE_ICONIFIED) {
        gtk_window_iconify (GTK_WINDOW(widget_win));
    }
    (void)widget;
    (void)user_data;
}



/* InitGUI:
 *  Initialise le module interface utilisateur.
 */
void InitGUI(const char version_name[], int direct_disk_support)
{
    GtkWidget *vbox_window;

    printf("Initialisation de l'interface..."); fflush (stdout);

    /* fenêtre d'affichage */
    window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(widget_win));
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_destroy_with_parent (GTK_WINDOW(window), TRUE);
    gtk_window_set_modal (GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(window), TRUE);
    g_signal_connect(G_OBJECT(window), "window-state-event", G_CALLBACK(iconify_teo_window), (gpointer)NULL);
    gtk_window_set_resizable (GTK_WINDOW(window), FALSE);
    gtk_window_set_title(GTK_WINDOW(window), (is_fr?"Teo - Panneau de contrÃ´le":"Teo - Control panel"));
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(do_hide), (gpointer)NULL);
    gtk_widget_realize(window); /* nécessaire pour charger le pixmap */

    /* boîte verticale associée à la fenêtre */
    vbox_window=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox_window), 5);
    gtk_container_add( GTK_CONTAINER(window), vbox_window);

    /* crée toutes les widgets de la fenêtre */
    InitGUI_title (version_name, vbox_window);
    InitGUI_commands (vbox_window);
    InitGUI_notebook (vbox_window, direct_disk_support);
    InitGUI_buttons (vbox_window);

    gtk_widget_show(vbox_window);
    /* mise en place d'un hook pour assurer le retraçage de la fenêtre
       principale de l'émulateur */
    gdk_window_add_filter(GTK_WIDGET(widget_win)->window, retrace_callback, NULL);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    printf("ok\n");
}



/* ControlPanel:
 *  Affiche le panneau de contrôle.
 */
void ControlPanel(void)
{
    /* Mise à jour du compteur de cassettes */
    gtk_spin_button_set_value( GTK_SPIN_BUTTON(spinner_cass), to8_GetK7Counter());

    /* affichage de la fenêtre principale et de ses éléments */
    gtk_widget_show(window);

    /* initialisation de la commande */
    teo.command = NONE;
    
    /* boucle de gestion des évènements */
    gtk_main();
}

