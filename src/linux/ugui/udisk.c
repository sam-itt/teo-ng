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
 *  Copyright (C) 1997-2013 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : linux/ugui/udisk.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 07/08/2011 24/03/2012 12/06/2012
 *                               04/11/2012
 *
 *  Gestion des disquettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <unistd.h>
   #include <string.h>
   #include <libgen.h>
   #include <gtk/gtk.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "errors.h"
#include "media/disk/controlr.h"
#include "media/disk.h"
#include "media/disk/daccess.h"
#include "linux/gui.h"


struct FILE_VECTOR {
    int first_file;
    int id;
    int direct;
    int entry_max;
    GtkWidget *combo;
    gulong    combo_id;
    GtkWidget *side_text;
    GtkWidget *side_combo;
    gulong     side_combo_id;
    GtkWidget *check_prot;
    gulong    check_prot_id;
    GtkWidget *emptying_button;
    gulong    emptying_button_id;
    struct DISK_VECTOR *path_list;
};

static struct FILE_VECTOR vector[NBDRIVE];



static void block_all (struct FILE_VECTOR *vector)
{
    g_signal_handler_block (vector->combo, vector->combo_id);
    g_signal_handler_block (vector->side_combo, vector->side_combo_id);
    g_signal_handler_block (vector->check_prot, vector->check_prot_id);
    g_signal_handler_block (vector->emptying_button, vector->emptying_button_id);
}



static void unblock_all (struct FILE_VECTOR *vector)
{
    g_signal_handler_unblock (vector->combo, vector->combo_id);
    g_signal_handler_unblock (vector->side_combo, vector->side_combo_id);
    g_signal_handler_unblock (vector->check_prot, vector->check_prot_id);
    g_signal_handler_unblock (vector->emptying_button, vector->emptying_button_id);
}



/* update_params:
 *  Sauve les paramètres d'un disque.
 */
static void update_params (struct FILE_VECTOR *vector)
{
    int state;
    int combo_index;
    
    if (vector->combo_id != 0)
    {
        block_all (vector);

        state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (vector->check_prot));
        teo.disk[vector->id].write_protect = state;
        combo_index = gtk_combo_box_get_active (GTK_COMBO_BOX (vector->combo));

        if ((combo_index == 0)
         || ((combo_index == 1) && (vector->direct)))
        {
            gtk_widget_set_sensitive (vector->side_combo, FALSE);
            gtk_widget_set_sensitive (vector->check_prot, FALSE);
            gtk_widget_set_sensitive (vector->side_text, FALSE);
            gtk_widget_set_sensitive (vector->emptying_button, FALSE);
        }
        else
        {
            gtk_widget_set_sensitive (vector->side_combo, TRUE);
            gtk_widget_set_sensitive (vector->check_prot, TRUE);
            gtk_widget_set_sensitive (vector->side_text, TRUE);
            gtk_widget_set_sensitive (vector->emptying_button, TRUE);
        }

        unblock_all (vector);
    }
}



/* toggle_check_disk:
 *  Vérifie si l'état de la protection peut être changée.
 */
static void toggle_check_disk(GtkWidget *button, struct FILE_VECTOR *vector)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
        disk_SetProtection (vector->id, TRUE);
    }
    else
    if (disk_SetProtection (vector->id, FALSE)==TRUE)
    {
        ugui_Error (is_fr?"Ecriture impossible sur ce support."
                         :"Writing unavailable on this device.", wControl);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
        teo.disk[vector->id].write_protect = TRUE;
    }
    update_params (vector);
}



/* set_access_mode:
 *  Positionne le checkbox de protection de disquette.
 */
static void set_access_mode (struct FILE_VECTOR *vector)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vector->check_prot), teo.disk[vector->id].write_protect);
    update_params (vector);
}



/* add_combo_entry
 *  Ajoute une entrée dans le combobox si inexistante.
 */
static void add_combo_entry (const char path[], struct FILE_VECTOR *vector)
{
    struct DISK_VECTOR *disk_vector = NULL;

    int index = disk_DiskVectorIndex (vector->path_list, path);

    if (index >= 0)
    {
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), index);
        disk_vector = disk_DiskVectorPtr (vector->path_list, index);
        teo.disk[vector->id].side = disk_vector->side;
        disk[vector->id].side_count = disk_vector->side_count;
    }
    else
    {
        vector->path_list = disk_DiskVectorAppend (vector->path_list, path,
                                                   teo.disk[vector->id].side,
                                                   disk[vector->id].side_count);
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(vector->combo), NULL, basename((char *)path));
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), vector->entry_max);
        vector->entry_max++;
    }
    set_access_mode (vector);
}



/* free_disk_entry:
 *  Libère la mémoire utilisée par la liste des disquettes.
 */
static void free_disk_entry (struct FILE_VECTOR *vector)
{
    disk_DiskVectorFree (vector->path_list);
    vector->path_list=NULL;
    vector->entry_max = 0;
}



/* activate_side_combo:
 *  Activate the combobox for sides.
 */
 /*
static void activate_combo_side (int state, struct FILE_VECTOR *vector)
{
    gtk_widget_set_sensitive (vector->side_combo, state);
    gtk_widget_set_sensitive (vector->side_text, state);
}
*/


/* reset_side_combo:

 *  Reinitialize the combo for sides.
 */
static void reset_side_combo (int selected_side, struct FILE_VECTOR *vector)
{
    int side = 0;
    char *str = NULL;

    if (vector->combo_id != 0)
    {
        block_all (vector);

        gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT(vector->side_combo));
        do
        {
            str = std_strdup_printf ("%d", side);
            gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(vector->side_combo), NULL, str);
            str = std_free (str);
            side++;
        } while (side < disk[vector->id].side_count);
    
        gtk_combo_box_set_active (GTK_COMBO_BOX(vector->side_combo), selected_side);
        teo.disk[vector->id].side = selected_side;

        unblock_all (vector);
    }
}



/* side_combo_changed:
 *  Changement de sélection du side combobox (callback).
 */
static void side_combo_changed (GtkComboBox *combo_box, struct FILE_VECTOR *vector)
{
    int active_row;
    struct DISK_VECTOR *p;

    teo.disk[vector->id].side = gtk_combo_box_get_active (combo_box);
    active_row = gtk_combo_box_get_active (GTK_COMBO_BOX(vector->combo));
    p = disk_DiskVectorPtr (vector->path_list, active_row);
    p->side = teo.disk[vector->id].side;
    dkc->WriteUpdateTrack();
    disk[vector->id].info->track = -1;
//    printf ("teo.disk[%d].side=%d\n", vector->id, teo.disk[vector->id].side);
}



/* init_combo:
 *  Remplit un combo vide.
 */
static void init_combo (struct FILE_VECTOR *vector)
{
    add_combo_entry (is_fr?"(Aucun)":"(None)", vector);
    if (vector->direct)
        add_combo_entry (is_fr?"(AccÃ¨s Direct)"
                              :"(Direct Access)", vector);
    gtk_combo_box_set_active (GTK_COMBO_BOX(vector->combo), 0);
    teo.disk[vector->id].side = 0;
    set_access_mode (vector);
}



/* reset_combo:
 *  Ejecte la disquette et vide le combobox (callback).
 *  (sauf l'entrée "aucune cartouche" et "direct access") 
 */
static void reset_combo (GtkButton *button, struct FILE_VECTOR *vector)
{
    block_all (vector);

    free_disk_entry (vector);
    gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT(vector->combo));
    disk_Eject(vector->id);
    init_combo (vector);
    update_params (vector);

    unblock_all (vector);

    (void)button;
}



/* load_virtual_disk:
 *  Charge une disquette virtuelle.
 */
static int load_virtual_disk (gchar *filename, struct FILE_VECTOR *vector)
{
    int ret = disk_Load (vector->id, filename);

    switch (ret)
    {
        case TEO_ERROR :
            ugui_Error (teo_error_msg, wControl);
            break;

        case TRUE :
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(vector->check_prot), TRUE);
            teo.disk[vector->id].write_protect = TRUE;
            break;

        default : break;
    }
    return ret;
}



/* combo_changed:
 *  Changement de sélection du combobox (callback).
 */
static void combo_changed (GtkComboBox *combo_box, struct FILE_VECTOR *vector)
{
    struct DISK_VECTOR *p=NULL;
    guint active_row = gtk_combo_box_get_active (combo_box);

    if (active_row == 0)
        disk_Eject(vector->id);
    else
    if ((active_row == 1) && (vector->direct))
        (void)daccess_LoadDisk (vector->id, "");
    else
    {
        p = disk_DiskVectorPtr (vector->path_list, active_row);
        (void)load_virtual_disk (p->str, vector);
        reset_side_combo (p->side, vector);
    }
    set_access_mode (vector);
    update_params (vector);
}



/* open_file:
 *  Charge une nouvelle disquette (callback).
 */
static void open_file (GtkButton *button, struct FILE_VECTOR *vector)
{
    GtkFileFilter *filter;
    static GtkWidget *dialog;
    char *folder_name;
    char *file_name;

    if (vector->first_file) {
        dialog = gtk_file_chooser_dialog_new (
                 is_fr?"SÃ©lectionner une disquette":"Select a disk",
                 (GtkWindow *) wControl, GTK_FILE_CHOOSER_ACTION_OPEN,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        filter = gtk_file_filter_new ();
        gtk_file_filter_set_name (filter, is_fr?"Fichiers disquette":"Disk files");
        gtk_file_filter_add_pattern (filter, "*.sap");
        gtk_file_filter_add_pattern (filter, "*.SAP");
        gtk_file_filter_add_pattern (filter, "*.hfe");
        gtk_file_filter_add_pattern (filter, "*.HFE");
        gtk_file_filter_add_pattern (filter, "*.fd");
        gtk_file_filter_add_pattern (filter, "*.FD");
        gtk_file_filter_add_pattern (filter, "*.qd");
        gtk_file_filter_add_pattern (filter, "*.QD");
        gtk_file_chooser_add_filter ((GtkFileChooser *)dialog, filter);

        /* Attend que le dialog ait tout assimilé */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        vector->first_file=0;
    }

    if (teo.disk[vector->id].file != NULL)
    {
        folder_name = std_strdup_printf ("%s", teo.disk[vector->id].file);
        std_CleanPath (folder_name);
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, folder_name);
        folder_name = std_free (folder_name);
    }
    else
    if (teo.default_folder != NULL)
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, teo.default_folder);
    else
    if (access("./disks/", F_OK) == 0)
        (void)gtk_file_chooser_set_current_folder((GtkFileChooser *)dialog, "./disks/");

    if (gtk_dialog_run ((GtkDialog *)dialog) == GTK_RESPONSE_ACCEPT)
    {
        file_name = gtk_file_chooser_get_filename((GtkFileChooser *)dialog);
        if (load_virtual_disk (file_name, vector) >= 0)
        {
            block_all (vector);
 
            add_combo_entry (teo.disk[vector->id].file, vector);
            reset_side_combo (teo.disk[vector->id].side, vector);
            folder_name = gtk_file_chooser_get_current_folder ((GtkFileChooser *)dialog);
            teo.default_folder = std_free(teo.default_folder);
            if (folder_name != NULL)
                teo.default_folder = std_strdup_printf ("%s", folder_name);
            g_free (folder_name);
//            activate_combo_side (TRUE, vector);
            update_params (vector);

            unblock_all (vector);
        }
        g_free (file_name);
    }
    gtk_widget_hide(dialog);
    (void)button;
}


/* ------------------------------------------------------------------------- */


/* udisk_Free:
 *  Libère la mémoire utilisée par les listes de disquettes.
 */
void udisk_Free (void)
{
    int i;

    for (i=0; i<NBDRIVE; i++)
        free_disk_entry (&vector[i]);
}



/* udisk_Init:
 *  Initialise la frame du notebook pour les disquettes.
 */
void udisk_Init (GtkWidget *notebook)
{
    int i;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *image;
    GtkWidget *frame;
    char *str;

    /* frame de la cartouche */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Disquettes":"Disks"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* boîte verticale associée à la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    for (i=0; i<NBDRIVE; i++)
    {
        memset (&vector[i], 0x00, sizeof (struct FILE_VECTOR));
        vector[i].id=i;
        vector[i].first_file=1;

        /* boîte horizontale */
        hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start( GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        /* label */
        widget=gtk_label_new((str = g_strdup_printf("%d:",i)));
        g_free (str);
        gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE,0);

        /* bouton de vidange */
        vector[i].emptying_button = gtk_button_new ();
        image = gtk_image_new_from_stock ("gtk-clear", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(vector[i].emptying_button), image);
        gtk_box_pack_start( GTK_BOX(hbox), vector[i].emptying_button, FALSE, FALSE, 0);
        gtk_widget_set_tooltip_text (vector[i].emptying_button,
                                     is_fr?"Vide la liste des fichiers"
                                          :"Empty the file list");
        vector[i].emptying_button_id = g_signal_connect(
                                          G_OBJECT(vector[i].emptying_button),
                                          "clicked",
                                          G_CALLBACK(reset_combo),
                                          (gpointer)&vector[i]);

        /* boutons protection de la disquette */
        vector[i].check_prot=gtk_check_button_new_with_label("prot.");
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(vector[i].check_prot),
                                      teo.disk[i].write_protect);
        gtk_box_pack_end( GTK_BOX(hbox), vector[i].check_prot, FALSE, TRUE,0);
        vector[i].check_prot_id = g_signal_connect(
                                         G_OBJECT(vector[i].check_prot),
                                         "toggled",
                                         G_CALLBACK(toggle_check_disk),
                                         (gpointer)&vector[i]);

        /* combobox pour le rappel de disquette */
        vector[i].combo=gtk_combo_box_text_new();
        gtk_box_pack_start( GTK_BOX(hbox), vector[i].combo, TRUE, TRUE,0);
        vector[i].direct=teo.disk[i].direct_access_allowed;
        init_combo (&vector[i]);
        if (teo.disk[i].file != NULL)
            add_combo_entry (teo.disk[i].file, &vector[i]);
        vector[i].combo_id = g_signal_connect (
                                         G_OBJECT(vector[i].combo),
                                         "changed",
                                         G_CALLBACK(combo_changed),
                                         (gpointer)&vector[i]);

        /* bouton d'ouverture de fichier */
        widget = gtk_button_new ();
        image = gtk_image_new_from_stock ("gtk-floppy", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(widget), image);
        gtk_widget_set_tooltip_text (widget, is_fr?"Ouvrir un fichier disquette"
                                                  :"Open a disk file");
        gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
        (void)g_signal_connect(G_OBJECT(widget),
                               "clicked",
                               G_CALLBACK(open_file),
                               (gpointer)&vector[i]);

        /* label pour la face de disquette */
        vector[i].side_text=gtk_label_new((is_fr)?"Face":"Side");
        gtk_widget_set_tooltip_text (vector[i].side_text,
                                     is_fr?"Choisir une face"
                                          :"Choose a side");
        gtk_box_pack_start( GTK_BOX(hbox), vector[i].side_text, FALSE, FALSE,0);

        /* combobox pour la face de disquette */
        vector[i].side_combo=gtk_combo_box_text_new();
        gtk_box_pack_start( GTK_BOX(hbox), vector[i].side_combo, FALSE, FALSE,0);
        vector[i].side_combo_id = g_signal_connect (
                                         G_OBJECT(vector[i].side_combo),
                                         "changed",
                                         G_CALLBACK(side_combo_changed),
                                         (gpointer)&vector[i]);

        reset_side_combo (teo.disk[i].side, &vector[i]);
        update_params (&vector[i]);
    }
}
