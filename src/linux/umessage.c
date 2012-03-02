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
 *  Module     : linux/message.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               François Mouret 30/07/2011
 *
 *  Classe Message d'extension du toolkit GTK+ 2.x/3.x
 */


#ifndef SCAN_DEPEND
   #include <gtk/gtkbutton.h>
   #include <gtk/gtkdialog.h>
   #include <gtk/gtkhbbox.h>
   #include <gtk/gtklabel.h>
   #include <gtk/gtksignal.h>
   #include <gtk/gtkvbox.h>
#endif

#include "linux/message.h"


#if BEFORE_GTK_2_MIN

/* partie privée de la classe Message:
 */
static void message_class_init(MessageClass *class)
{
    (void) class;
}



static void message_init(Message *mesg)
{
    GtkWidget *vbox, *button_box, *button;

    mesg->window.type=GTK_WINDOW_POPUP;
    gtk_window_set_title(GTK_WINDOW(mesg), "Teo");
    gtk_window_set_position(GTK_WINDOW(mesg), GTK_WIN_POS_CENTER);
    gtk_signal_connect_object(GTK_OBJECT(GTK_WINDOW(mesg)), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(mesg));

    vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(GTK_WINDOW(mesg)), vbox);
    gtk_widget_show(vbox);

    mesg->label=gtk_label_new("\0");
    gtk_box_pack_start(GTK_BOX(vbox), mesg->label, TRUE, FALSE, 0);
    gtk_widget_show(mesg->label);

    button_box=gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), button_box, TRUE, FALSE, 0);
    gtk_widget_show(button_box);

    button=gtk_button_new_with_label("OK");
    gtk_container_add(GTK_CONTAINER(button_box), button);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(mesg));
    gtk_widget_show(button);
}



static GType message_get_type(void)
{
    static GType mesg_type = 0;

    if (!mesg_type)
    {
	static const GTypeInfo mesg_info =
	{
	    sizeof(MessageClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) message_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(Message),
	    0,    /* n_preallocs */
	    (GInstanceInitFunc) message_init,
	    NULL
	};

        mesg_type = g_type_register_static (GTK_TYPE_WINDOW,
					    "MessageInfo",
					    &mesg_info,
					    0);
    }

    return mesg_type;
}



/* partie publique de la classe Message:
 */
GtkWidget *message_new(const gchar *label)
{
    Message *mesg=MESSAGE( gtk_type_new (message_get_type() ));

    gtk_label_set_text( GTK_LABEL(mesg->label), label);

    return GTK_WIDGET(mesg);
}

#else

void message_box (const gchar *message, GtkWidget *parent_window)
{
    GtkWidget *dialog = gtk_message_dialog_new  (
                          (GtkWindow *)parent_window,
                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                          GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                          "%s", message);
    gtk_window_set_title ((GtkWindow *)dialog, "Teo");
    (void)gtk_dialog_run ((GtkDialog *)dialog);
    gtk_widget_destroy ((GtkWidget *)dialog);
}

#endif

