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
 *                  L'\E9mulateur Thomson TO8
 *
 *  Copyright (C) 2011-2013 Gilles F\E9tis, Fran\E7ois Mouret
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
 *  Module     : src/linux/ugui/udebug.c
 *  Version    : 1.8.3
 *  Cr\E9\E9 par   : Gilles F\E9tis 27/07/2011
 *  Modifi\E9 par: Fran\E7ois Mouret 18/02/2012 12/06/2012 18/09/2013
 *
 *  D\E9bogueur du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <unistd.h>
   #include <gtk/gtk.h>
   #include <gdk/gdkx.h>
#endif

#include "defs.h"
#include "teo.h"
#include "debug.h"
#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "media/disk/controlr.h"
#include "media/disk.h"
#include "linux/display.h"
#include "linux/gui.h"
#include "linux/graphic.h"


#define DEBUG_SPACE 2

#define DEBUG_CMD_STEP 0
#define DEBUG_CMD_STEPOVER 1
#define DEBUG_CMD_RUN 2
#define DEBUG_CMD_BKPT 3
#define DEBUG_CMD_REMBKPT 4
#define DEBUG_CMD_RESET 5

#define DASM_NLINES 50

#define MAX_BREAKPOINTS  10
static int breakpoint[MAX_BREAKPOINTS]={-1,-1,-1,-1,-1, -1,-1,-1,-1,-1};

static struct MC6809_REGS regs, prev_regs;

static char fetch_buffer[MC6809_FETCH_BUFFER_SIZE]="";
static char string_buffer[MAX(MC6809_DASM_BUFFER_SIZE,128)]="";
static char text_buffer[MAX(MC6809_DASM_BUFFER_SIZE*DASM_NLINES,2048)]="";
static char memory_buffer[8192*(5+3*8+1)]="";

/* fen\EAtre de l'interface utilisateur */
static GtkWidget * wDebug = NULL;
static GtkWidget *notebookDebug;

static GtkWidget *entry_address;

/* labels */
static GtkWidget *label_6809regs,*label_sr_list;

/* text_view */
static GtkWidget *m6809_text_view;
static GtkTextBuffer *m6809_text_buffer;

static GtkWidget *memory_text_view;
static GtkTextBuffer *memory_text_buffer;

static GtkWidget *hardware_text_view;
static GtkTextBuffer *hardware_text_buffer;

static GtkWidget *bplist_text_view;
static GtkTextBuffer *bplist_text_buffer;



/* 
 *  Dump pile systeme
 */
static char* debug_get_sr_list(void) {
    int i,j,adr=0;
    char *ptr = text_buffer;
    *ptr='\0';

    ptr+=sprintf(ptr,is_fr?"Pile (S)ystème :\n"
                          :"(S)ystem stack :\n");
    for(j=0;j<8;j++) {
        for(i=0;i<4;i++) {
            ptr+=sprintf(ptr,"%02X " ,LOAD_BYTE(regs.sr+adr));
            adr++;
        }
        ptr+=sprintf(ptr,"\n");
    }

    return text_buffer;
}        


/* 
 *  Dump memoire
 */
static int memory_dump_start=0;

#define PRINTABLE(A) ((A<32)?'.':((A>126)?'.':A))

static char* debug_get_memory(void) {
    int i,j,adr=memory_dump_start;
    char *ptr = memory_buffer;
    *ptr='\0';

    for(j=0;j<(0x2000)/8;j++) {
        ptr+=sprintf(ptr,"%04X " ,adr);
        for(i=0;i<8;i++) {
            ptr+=sprintf(ptr,"%02X " ,LOAD_BYTE(adr));
            adr++;
        }
	adr-=8;
        for(i=0;i<8;i++) {
            char a=LOAD_BYTE(adr);
            ptr+=sprintf(ptr,"%c" ,PRINTABLE(a));
            adr++;
        }
        ptr+=sprintf(ptr,"\n");
    }

    return memory_buffer;
}  

static void do_memory_set_start (GtkWidget *button, int value)
{
    memory_dump_start=value*0x2000;
    gtk_text_buffer_set_text(memory_text_buffer,debug_get_memory(),-1);
    (void) button;
}


static char* debug_get_6809regs(void) {
    char *ptr = text_buffer;
    *ptr='\0';

    ptr+=sprintf(ptr,is_fr?"Registres 6809 :\n"
                          :"6809 registers :\n");

    ptr+=sprintf(ptr, "CC=%02X [%c%c%c%c%c%c%c%c]\n"
                         ,regs.cc
                            ,regs.cc&0x80 ? 'E' : '.'
                            ,regs.cc&0x40 ? 'F' : '.'
                            ,regs.cc&0x20 ? 'H' : '.'
                            ,regs.cc&0x10 ? 'I' : '.'
                            ,regs.cc&0x08 ? 'N' : '.'
                            ,regs.cc&0x04 ? 'Z' : '.'
                            ,regs.cc&0x02 ? 'V' : '.'
                            ,regs.cc&0x01 ? 'C' : '.');

    ptr+=sprintf(ptr, "A= x%02X   %d\n",regs.ar,regs.ar);
    ptr+=sprintf(ptr, "B= x%02X   %d\n",regs.br,regs.br);
    ptr+=sprintf(ptr, "D= x%04X %d\n\n",(regs.ar<<8)|regs.br,(regs.ar<<8)|regs.br);
    ptr+=sprintf(ptr, "DP=x%02X\n\n",regs.dp);
    ptr+=sprintf(ptr, "X= x%04X %d\n",regs.xr,regs.xr);
    ptr+=sprintf(ptr, "Y= x%04X %d\n\n",regs.yr,regs.yr);
    ptr+=sprintf(ptr, "U= x%04X\n",regs.ur);
    ptr+=sprintf(ptr, "S= x%04X\n\n",regs.sr);
    ptr+=sprintf(ptr, "PC=x%04X [x%04X]\n",regs.pc,prev_regs.pc);

    ptr+=sprintf(ptr,is_fr?"Video :\n"
                          :"Video :\n");

    ptr+=sprintf(ptr, "clock=%lld\n"
                         ,regs.cpu_clock);
    ptr+=sprintf(ptr, "frame=%d\n"
                         ,(int)regs.cpu_clock%TEO_CYCLES_PER_FRAME);

    ptr+=sprintf(ptr, "line=%d\n"
                         ,(int)((regs.cpu_clock%TEO_CYCLES_PER_FRAME)/FULL_LINE_CYCLES)-56);

    return text_buffer;
}


static char* debug_get_bplist(void) {
    int i;
    char *ptr = text_buffer;
    *ptr='\0';

    ptr+=sprintf(ptr,is_fr?"BreakPoints :\n"
                          :"BreakPoints :\n");

    for (i=0;i< MAX_BREAKPOINTS;i++) {
        if (breakpoint[i]==-1) break;
    	ptr+=sprintf(ptr,"BKPT[%d]=%04X\n",i,breakpoint[i]);
    }
    return text_buffer;
}


static char* hardware_get_regs(void) {
    char *ptr = text_buffer;
    *ptr='\0';

    ptr+=sprintf(ptr,"Interrupt:\n");

    ptr+=sprintf(ptr,"IRQ: %d\n" ,
                     mc6809_irq);

    ptr+=sprintf(ptr,"\nMC6846:\n");

    ptr+=sprintf(ptr," CSR: %02X   CRC: %02X\n",
                     mc6846.csr,
                     mc6846.crc);
    ptr+=sprintf(ptr,"DDRC: %02X   PRC: %02X\n",
                     mc6846.ddrc,
                     mc6846.prc);
    ptr+=sprintf(ptr," TCR: %02X  TMSB: %02X  TLSB: %02X\n",
                     mc6846.tcr,
                     mc6846.tmsb,
                     mc6846.tlsb);

    ptr+=sprintf(ptr,"\nMC6821:\n");

    ptr+=sprintf(ptr,"CRA: %02X  DDRA: %02X  PDRA: %02X\n",
                     pia_int.porta.cr,
                     pia_int.porta.ddr,
                     mc6821_ReadPort(&pia_int.porta));
    ptr+=sprintf(ptr,"CRB: %02X  DDRB: %02X  PDRB: %02X\n",
                     pia_int.portb.cr,
                     pia_int.portb.ddr,
                     mc6821_ReadPort(&pia_int.portb));

    ptr+=sprintf(ptr,"\nMC6821:\n");
    ptr+=sprintf(ptr,"CRA: %02X  DDRA: %02X  PDRA: %02X\n",
                     pia_ext.porta.cr,
                     pia_ext.porta.ddr,
                     mc6821_ReadPort(&pia_ext.porta));
    ptr+=sprintf(ptr,"CRB: %02X  DDRB: %02X  PDRB: %02X\n",
                     pia_ext.portb.cr,
                     pia_ext.portb.ddr,
                     mc6821_ReadPort(&pia_ext.portb));

    ptr+=sprintf(ptr,"\nGate Array:\n");

    ptr+=sprintf(ptr,"P_DATA: %02X   P_ADDR: %02X\n",
                     mode_page.p_data,
                     mode_page.p_addr);
    ptr+=sprintf(ptr,"LGAMOD: %02X     SYS1: %02X\n",
                     mode_page.lgamod,
                     mode_page.system1);
    ptr+=sprintf(ptr,"  SYS2: %02X     DATA: %02X\n",
                     mode_page.system2,
                     mode_page.ram_data);
    ptr+=sprintf(ptr,"  CART: %02X   COMMUT: %02X\n",
                     mode_page.cart,
                     mode_page.commut);
    ptr+=sprintf(ptr,"CMD0: %02X  CMD1: %02X  CMD2: %02X\n",
                     dkc->wr0,
                     dkc->wr1,
                     dkc->wr2);
    ptr+=sprintf(ptr," STAT0: %02X    STAT1: %02X\n",
                     dkc->rr0,
                     dkc->rr1);
    ptr+=sprintf(ptr," WDATA: %02X    RDATA: %02X\n",
                     dkc->wr3,
                     dkc->rr3);
    ptr+=sprintf(ptr,is_fr?"page de ROM cartouche : %d\n"
                          :"ROM cartridge page  : %d\n",
                     mempager.cart.rom_page);
    ptr+=sprintf(ptr,is_fr?"page de RAM cartouche : %d\n"
                          :"RAM cartridge page  : %d\n",
                     mempager.cart.ram_page);
    ptr+=sprintf(ptr,is_fr?"page de VRAM          : %d\n"
                          :"VRAM page           : %d\n",
                     mempager.screen.vram_page);
    ptr+=sprintf(ptr,is_fr?"page de RAM (registre): %d\n"
                          :"RAM page (register) : %d\n",
                     mempager.data.reg_page);
    ptr+=sprintf(ptr,is_fr?"page de RAM (PIA)     : %d\n"
                          :"RAM page (PIA)      : %d\n",
                     mempager.data.pia_page);
    return text_buffer;
}


static char* debug_get_dasm(void) { 
        int i,j,pc;
        char *ptr=text_buffer;
	text_buffer[0]='\0';
        mc6809_GetRegs(&regs);
        pc=regs.pc;
        for (i=0; i<DASM_NLINES; i++)
        {
            for (j=0; j<5; j++)
                fetch_buffer[j]=LOAD_BYTE(pc+j);

            pc=(pc+MC6809_Dasm(string_buffer,
                               (const unsigned char *)fetch_buffer,
                               pc,
                               MC6809_DASM_BINASM_MODE))&0xFFFF;
            if (i==0)
                ptr+=sprintf(ptr, "%s\n", string_buffer);
            else
                ptr+=sprintf(ptr, "%s\n", string_buffer);
        }
        ptr+=sprintf(ptr, "                                                 ");
	return text_buffer;
}


/* update_debug_text:
 *  Mise \E0 jour du texte de la fen\EAtre de debug
 */
static void update_debug_text (void)
{
    char *markup;
    char *string;

    mc6809_GetRegs(&regs);

    string = "<span face=\"Courier\" font=\"9\">%s</span>";
    markup = g_markup_printf_escaped (string, debug_get_6809regs());
    gtk_label_set_markup (GTK_LABEL (label_6809regs), markup);
    g_free (markup);

    string = "<span face=\"Courier\" font=\"9\">%s</span>";
    markup = g_markup_printf_escaped (string, debug_get_sr_list());
    gtk_label_set_markup (GTK_LABEL (label_sr_list), markup);
    g_free (markup);


    gtk_text_buffer_set_text(hardware_text_buffer,hardware_get_regs(),-1);

    gtk_text_buffer_set_text(m6809_text_buffer,debug_get_dasm(),-1);

    gtk_text_buffer_set_text(memory_text_buffer,debug_get_memory(),-1);

    gtk_text_buffer_set_text(bplist_text_buffer,debug_get_bplist(),-1);
}



static void
debug_stepover(void) {
	    int pc;
	    int watch=0;
            mc6809_GetRegs(&regs);

                    pc = regs.pc;
                    do
                    {
                        mc6809_GetRegs(&prev_regs);
                        mc6809_StepExec(1);
                        mc6809_GetRegs(&regs);
                    } while (((regs.pc<pc) || (regs.pc>pc+5)) && ((watch++)<20000));
}

static void
debug_step(void) {
            mc6809_StepExec(1);
}

static void debug_bkpt(int addr) {
    int i;
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	if (breakpoint[i]==-1) break;
    }
    if (i==MAX_BREAKPOINTS) i=(MAX_BREAKPOINTS-1);
    breakpoint[i]=addr;
}

static void debug_rembkpt(void) {
    int i;
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	breakpoint[i]=-1;
	}
}


static void do_command_debug (GtkWidget *button, int command)
{
    int addr;
    switch (command)
    {
    case DEBUG_CMD_STEP:
        debug_step();
        update_debug_text();
        break;

    case DEBUG_CMD_STEPOVER:
        debug_stepover();
        update_debug_text();
        break;

    case DEBUG_CMD_BKPT:
        if (strlen(gtk_entry_get_text (GTK_ENTRY(entry_address))) != 0)
        {
            addr=0;
            sscanf(gtk_entry_get_text (GTK_ENTRY(entry_address)),"%X",&addr);
            gtk_entry_set_text (GTK_ENTRY(entry_address), "");
            debug_bkpt(addr);
            update_debug_text ();
        }
        break;

    case DEBUG_CMD_REMBKPT:
        debug_rembkpt();
        update_debug_text ();
        break;

    case DEBUG_CMD_RESET:
        teo_Reset();
        update_debug_text ();
        break;
    }

    (void) button;
}


/* u6809_Init:
 *  Initialise la frame du notebook pour le code.
 */
void u6809_Init (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *widget;
    GtkWidget *frame;

    PangoFontDescription *font;

    font = pango_font_description_new ();
    pango_font_description_set_family (font, "courier");
    pango_font_description_set_size (font, 9 * PANGO_SCALE);

    /* frame */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Désassemblage":"Unassembly"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* bo\EEte verticale associ\E9e \E0 la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);

    m6809_text_buffer=gtk_text_buffer_new(NULL);

    gtk_text_buffer_set_text(m6809_text_buffer,debug_get_dasm(),-1);
    m6809_text_view=gtk_text_view_new_with_buffer(m6809_text_buffer);
    gtk_widget_modify_font (GTK_WIDGET (m6809_text_view), font);
    gtk_text_view_set_editable(GTK_TEXT_VIEW (m6809_text_view),FALSE);
 
    widget=gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_min_content_height((GtkScrolledWindow*)widget,400);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(widget),
                                          m6809_text_view);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, TRUE, DEBUG_SPACE);


}

/* uMemory_Init:
 *  Initialise la frame du notebook pour le code.
 */
void uMemory_Init (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *frame;
    int i;

    PangoFontDescription *font;

    font = pango_font_description_new ();
    pango_font_description_set_family (font, "courier");
    pango_font_description_set_size (font, 9 * PANGO_SCALE);

    /* frame */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Mémoire":"Memory"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* bo\EEte verticale associ\E9e \E0 la frame */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(hbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), hbox);

    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    gtk_box_pack_start( GTK_BOX(hbox), vbox, TRUE, TRUE, DEBUG_SPACE);

    for (i=0;i<8;i++) {
    sprintf(text_buffer,"0x%04X",i*0x2000);
    widget=gtk_button_new_with_label(text_buffer);
    gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK (do_memory_set_start),
                     (gpointer) i);
    }

    memory_text_buffer=gtk_text_buffer_new(NULL);

    memory_text_view=gtk_text_view_new_with_buffer(memory_text_buffer);
    gtk_widget_modify_font (GTK_WIDGET (memory_text_view), font);
    gtk_text_view_set_editable(GTK_TEXT_VIEW (memory_text_view),FALSE);

    widget=gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_min_content_height((GtkScrolledWindow*)widget,400);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(widget),
                                          memory_text_view);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, TRUE, DEBUG_SPACE);
}

/* uHardware_Init:
 *  Initialise la frame du notebook pour le code.
 */
void uHardware_Init (GtkWidget *notebook)
{
    GtkWidget *vbox;
    GtkWidget *widget;
    GtkWidget *frame;

    PangoFontDescription *font;

    font = pango_font_description_new ();
    pango_font_description_set_family (font, "courier");
    pango_font_description_set_size (font, 9 * PANGO_SCALE);

    /* frame */
    frame=gtk_frame_new("");
    gtk_frame_set_shadow_type( GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_frame_set_label_align( GTK_FRAME(frame), 0.985, 0.0);
    widget=gtk_label_new((is_fr?"Circuits":"Chips"));
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frame, widget);

    /* bo\EEte verticale associ\E9e \E0 la frame */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
    gtk_container_add( GTK_CONTAINER(frame), vbox);


    hardware_text_buffer=gtk_text_buffer_new(NULL);

    gtk_text_buffer_set_text(hardware_text_buffer,hardware_get_regs(),-1);
    hardware_text_view=gtk_text_view_new_with_buffer(hardware_text_buffer);
    gtk_widget_modify_font (GTK_WIDGET (hardware_text_view), font);
    gtk_text_view_set_editable(GTK_TEXT_VIEW (hardware_text_view),FALSE);

    widget=gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_min_content_height((GtkScrolledWindow*)widget,400);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(widget),
                                          hardware_text_view);
    gtk_box_pack_start( GTK_BOX(vbox), widget, TRUE, TRUE, DEBUG_SPACE);

}

/* ------------------------------------------------------------------------- */





/* udebug_Init:
 *  Cr\E9e la fen\EAtre de dialogue du debugger
 */
static void udebug_Init(void)
{
    GtkWidget *content_area;
    GtkWidget *hbox;
    GtkWidget *mainhbox;
    GtkWidget *vbox;
    GtkWidget *widget;

    GtkWidget *buttonBox;
    GtkWidget *leftBox;
    GtkWidget *rightBox;

    PangoFontDescription *font;

    font = pango_font_description_new ();
    pango_font_description_set_family (font, "courier");
    pango_font_description_set_size (font, 9 * PANGO_SCALE);


    /* fen\EAtre d'affichage */
    wDebug = gtk_dialog_new_with_buttons (
                    is_fr?"Teo - Débogueur":"Teo - Debugger",
                    GTK_WINDOW(wMain),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    (is_fr?"Continuer":"Run"), GTK_RESPONSE_ACCEPT,
                    (is_fr?"Fin de debug":"End debug"), GTK_RESPONSE_CANCEL,
                    NULL);
    gtk_window_set_resizable (GTK_WINDOW(wDebug), FALSE);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG(wDebug));


    /* bo\EEte verticale des textes */
    mainhbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(content_area), mainhbox, FALSE, FALSE, DEBUG_SPACE);

    /* notebook */
    notebookDebug=gtk_notebook_new();

    u6809_Init(notebookDebug);
    uMemory_Init(notebookDebug); 
    uHardware_Init(notebookDebug);

    gtk_box_pack_start( GTK_BOX(mainhbox), notebookDebug, TRUE, FALSE, 0);
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_pack_start( GTK_BOX(mainhbox), vbox, TRUE, FALSE, 0);

    /*  la barre de boutons */
    buttonBox=gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing ( GTK_BOX(buttonBox), 10);
    gtk_button_box_set_layout ((GtkButtonBox *)buttonBox, GTK_BUTTONBOX_START);
    gtk_box_pack_start( GTK_BOX(vbox), buttonBox, FALSE, FALSE, 0);

    widget=gtk_button_new_with_label("Step");
    gtk_box_pack_start(GTK_BOX(buttonBox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK (do_command_debug),
                     (gpointer) DEBUG_CMD_STEP);

    widget=gtk_button_new_with_label("Step over");
    gtk_box_pack_start( GTK_BOX(buttonBox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK (do_command_debug),
                     (gpointer) DEBUG_CMD_STEPOVER);

    widget=gtk_button_new_with_label("Reset");
    gtk_box_pack_start( GTK_BOX(buttonBox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK (do_command_debug),
                     (gpointer) DEBUG_CMD_STEPOVER);

    

    /* boite sous les boutons */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    /* on divise en 2 colonnes */
    leftBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_pack_start( GTK_BOX(hbox), leftBox, FALSE, FALSE, 0);

    rightBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_pack_start( GTK_BOX(hbox), rightBox, FALSE, FALSE, 0);

    /* widget pour les registres 6809 */
    label_6809regs=gtk_label_new("");
    gtk_box_pack_start( GTK_BOX(leftBox), label_6809regs, FALSE, FALSE, 0);
    
    /* label pour lecontenu de la pile S */
    label_sr_list=gtk_label_new("");
    gtk_box_pack_start( GTK_BOX(leftBox), label_sr_list, FALSE, FALSE, 0);
    
    /* break points list and buttons */
    bplist_text_buffer=gtk_text_buffer_new(NULL);

    gtk_text_buffer_set_text(bplist_text_buffer,debug_get_bplist(),-1);
    bplist_text_view=gtk_text_view_new_with_buffer(bplist_text_buffer);
    gtk_widget_modify_font (GTK_WIDGET (bplist_text_view), font);
    gtk_text_view_set_editable(GTK_TEXT_VIEW (bplist_text_view),FALSE);

    gtk_box_pack_start( GTK_BOX(rightBox), bplist_text_view, FALSE, FALSE, DEBUG_SPACE);

    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start( GTK_BOX(rightBox), hbox, FALSE, FALSE, 0);

    widget=gtk_button_new_with_label("Add BP");
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK(do_command_debug),
                     (gpointer) DEBUG_CMD_BKPT);

    entry_address=gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(entry_address), 4);
    gtk_entry_set_width_chars (GTK_ENTRY(entry_address), 4);
    g_signal_connect(G_OBJECT(entry_address),
                              "activate",
                              G_CALLBACK(do_command_debug),
                              (gpointer) DEBUG_CMD_BKPT);
    gtk_box_pack_start( GTK_BOX(hbox), entry_address, FALSE, FALSE, 0);

    widget=gtk_button_new_with_label("Remove all BP");
    gtk_box_pack_start( GTK_BOX(rightBox), widget, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget),
                     "clicked",
                     G_CALLBACK (do_command_debug),
                     (gpointer) DEBUG_CMD_REMBKPT);


    /* affiche tout l'int\E9rieur */
    gtk_widget_show_all (content_area);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();
}


/* udebug_Panel:
 *  Affiche le panneau du debug.
 */
int udebug_Panel(void)
{
    int debug = FALSE;
    gint response;

    /* Initialise la fen\EAtre */
    if (wDebug == NULL)
        udebug_Init();

    /* actualise le texte \E0 afficher */
    update_debug_text ();
    
    /* gestion des \E9v\E8nements */
    response = gtk_dialog_run (GTK_DIALOG(wDebug));
    debug = FALSE;
    switch (response)
    {
        case GTK_RESPONSE_ACCEPT: debug = TRUE; break;
        case GTK_RESPONSE_CANCEL: teo.command=TEO_COMMAND_NONE ; break;
   }
   gtk_widget_hide (wDebug);
   return debug;
}


/* udebug_Breakpoint:
 *  Rep\E8re si le PC est au breakpoint
 */
int udebug_Breakpoint(int pc) {
    int i;
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	if (breakpoint[i]==-1) break;
	if (breakpoint[i]==pc) return 1;
    }
    return 0;
}

