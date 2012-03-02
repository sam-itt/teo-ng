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
 *  Module     : linux/question.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou juillet 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               Gilles Fétis 27/07/2011
 *               François Mouret 30/07/2011
 *
 *  Classe Question d'extension du toolkit GTK+ 2.x/3.x.
 */


#ifndef SCAN_DEPEND
   #include <gtk/gtkbutton.h>
   #include <gtk/gtkdialog.h>
   #include <gtk/gtkhbbox.h>
   #include <gtk/gtklabel.h>
   #include <gtk/gtksignal.h>
   #include <gtk/gtkvbox.h>
#endif

#include "linux/question.h"
#include "linux/main.h"
#include "to8.h"


#if BEFORE_GTK_2_MIN

/* partie privée de la classe Question:
 */
enum {
    CLICKED_YES_SIGNAL,
    CLICKED_NO_SIGNAL,
    LAST_SIGNAL
};

static gint question_signals[LAST_SIGNAL]={0};



static void question_class_init(QuestionClass *class)
{
    GtkObjectClass *object_class;

    object_class=(GtkObjectClass *) class;

    question_signals[CLICKED_YES_SIGNAL] = 
      g_signal_new ("clicked_yes",
		    G_TYPE_FROM_CLASS (object_class),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(QuestionClass, clicked_yes),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    question_signals[CLICKED_NO_SIGNAL] = 
      g_signal_new ("clicked_no",
		    G_TYPE_FROM_CLASS (object_class),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(QuestionClass, clicked_no),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    class->clicked_yes = NULL;
    class->clicked_no = NULL;
}



static void question_click_yes(GtkWidget *button, GtkWidget *quest)
{
     gtk_signal_emit(GTK_OBJECT(quest), question_signals[CLICKED_YES_SIGNAL]);

     gtk_widget_destroy(quest);

    (void) button;
}



static void question_click_no(GtkWidget *button, GtkWidget *quest)
{
     gtk_signal_emit(GTK_OBJECT(quest), question_signals[CLICKED_NO_SIGNAL]);

     gtk_widget_destroy(quest);

     (void) button;
}



static void question_init(Question *quest)
{
    GtkWidget *vbox, *button_box, *button_yes, *button_no;

    quest->window.type=GTK_WINDOW_POPUP;
    gtk_window_set_title(GTK_WINDOW(quest), "Teo - confirmation");
    gtk_window_set_position(GTK_WINDOW(quest), GTK_WIN_POS_CENTER);
    gtk_signal_connect_object(GTK_OBJECT(GTK_WINDOW(quest)), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(quest));

    vbox=gtk_vbox_new(FALSE,5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(GTK_WINDOW(quest)), vbox);
    gtk_widget_show(vbox);

    quest->label=gtk_label_new("\0");
    gtk_box_pack_start(GTK_BOX(vbox), quest->label, TRUE, FALSE, 0);
    gtk_widget_show(quest->label);

    button_box=gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), button_box, TRUE, FALSE, 0);
    gtk_widget_show(button_box);

#if BEFORE_GTK_2_MIN
    button_yes=gtk_button_new_with_label((is_fr?"Oui":"Yes"));
#else
    button_yes=gtk_button_new_from_stock (GTK_STOCK_YES);
#endif
    gtk_container_add(GTK_CONTAINER(button_box), button_yes);
    gtk_signal_connect(GTK_OBJECT(button_yes), "clicked", GTK_SIGNAL_FUNC(question_click_yes), quest);
    gtk_widget_show(button_yes);

#if BEFORE_GTK_2_MIN
    button_no=gtk_button_new_with_label((is_fr?"Non":"No"));
#else
    button_no=gtk_button_new_from_stock (GTK_STOCK_NO);
#endif
    gtk_container_add(GTK_CONTAINER(button_box), button_no);
    gtk_signal_connect(GTK_OBJECT(button_no), "clicked", GTK_SIGNAL_FUNC(question_click_no), quest);
    gtk_widget_show(button_no);
}



static GType question_get_type(void)
{
    static GType quest_type = 0;

    if (!quest_type)
    {
	static const GTypeInfo quest_info =
	{
	  sizeof(QuestionClass),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    (GClassInitFunc) question_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof(Question),
	    0,    /* n_preallocs */
	    (GInstanceInitFunc) question_init,
	    NULL
	};

        quest_type = g_type_register_static (GTK_TYPE_WINDOW,
					     "QuestInfo",
					     &quest_info,
					     0);
    }

    return quest_type;
}



/* partie publique de la classe Question:
 */
GtkWidget *question_new(const gchar *label)
{
    Question *quest=QUESTION( gtk_type_new (question_get_type() ));

    gtk_label_set_text( GTK_LABEL(quest->label), label);

    return GTK_WIDGET(quest);
}

#else

int question_response (const gchar *message, GtkWidget *parent_window)
{
    GtkResponseType response;
    GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow*)parent_window,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                 "%s", message);
    gtk_window_set_title ((GtkWindow *)dialog, "Teo - confirmation");
    response = gtk_dialog_run ((GtkDialog*)dialog);
    gtk_widget_destroy (dialog);
    return (response == GTK_RESPONSE_YES) ? TRUE : FALSE;
}

#endif

