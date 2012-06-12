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
 *  Copyright (C) 2011-2012 Gilles Fétis, François Mouret
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
 *  Version    : 1.8.1
 *  Créé par   : Gilles Fétis 27/07/2011
 *  Modifié par: François Mouret 18/02/2012  12/06/2012
 *
 *  Débogueur du TO8.
 */


#ifndef SCAN_DEPEND
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif

#include "linux/display.h"
#include "linux/gui.h"
#include "linux/graphic.h"
#include "to8.h"
#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "to8dbg.h"

#define DEBUG_SPACE 2

#define DEBUG_CMD_STEP 0
#define DEBUG_CMD_STEPOVER 1
#define DEBUG_CMD_RUN 2
#define DEBUG_CMD_BKPT 3
#define DEBUG_CMD_REMBKPT 4

#define DASM_NLINES 100

#define MAX_BREAKPOINTS  4
static int breakpoint[MAX_BREAKPOINTS]={-1,-1,-1,-1};

static struct MC6809_REGS regs, prev_regs;

static char fetch_buffer[MC6809_FETCH_BUFFER_SIZE]="";
static char string_buffer[MAX(MC6809_DASM_BUFFER_SIZE,128)]="";
static char text_buffer[MAX(MC6809_DASM_BUFFER_SIZE*DASM_NLINES,2048)]="";

/* fenêtre de l'interface utilisateur */
static GtkWidget * wDebug;
// static GtkEntryBuffer * address_buf;
static GtkWidget *entry_address;
static GtkWidget *label_middle_left,*label_middle_right;
static GtkWidget *label_6809regs,*label_sr_list;


static char* debug_get_sr_list(void) {
    int i;
    char *ptr = text_buffer;

    *ptr='\0';
    for(i=0;i<15;i++)
        ptr+=sprintf(ptr,"%02X " ,LOAD_BYTE(regs.sr+i));
    return text_buffer;
}        


static char* debug_get_6809regs(void) {
    sprintf(text_buffer, "%02X [%c%c%c%c%c%c%c%c] %02X %02X %02X %04X %04X %04X %04X %04X [%04X]"
                         ,regs.cc
                            ,regs.cc&0x80 ? 'E' : '.'
                            ,regs.cc&0x40 ? 'F' : '.'
                            ,regs.cc&0x20 ? 'H' : '.'
                            ,regs.cc&0x10 ? 'I' : '.'
                            ,regs.cc&0x08 ? 'N' : '.'
                            ,regs.cc&0x04 ? 'Z' : '.'
                            ,regs.cc&0x02 ? 'V' : '.'
                            ,regs.cc&0x01 ? 'C' : '.'
                         ,regs.ar,regs.br
                         ,regs.dp
                         ,regs.xr,regs.yr
                         ,regs.ur,regs.sr
                         ,regs.pc,prev_regs.pc);
    return text_buffer;
}


static char* debug_get_regs(void) {
    int i;
    char *ptr = text_buffer;
    *ptr='\0';
    for (i=0;i< MAX_BREAKPOINTS;i++) {
        if (breakpoint[i]==-1) break;
    	ptr+=sprintf(ptr,"BKPT[%d]=%04X\n",i,breakpoint[i]);
    }
    ptr+=sprintf(ptr,"IRQ: %d\n" ,mc6809_irq);
    ptr+=sprintf(ptr," CSR: %02X   CRC: %02X\n",mc6846.csr,mc6846.crc);
    ptr+=sprintf(ptr,"DDRC: %02X   PRC: %02X\n",mc6846.ddrc,mc6846.prc);
    ptr+=sprintf(ptr," TCR: %02X  TMSB: %02X  TLSB: %02X\n",mc6846.tcr,mc6846.tmsb, mc6846.tlsb);
    ptr+=sprintf(ptr,"CRA: %02X  DDRA: %02X  PDRA: %02X\n",pia_int.porta.cr,
                          pia_int.porta.ddr,mc6821_ReadPort(&pia_int.porta));
    ptr+=sprintf(ptr,"CRB: %02X  DDRB: %02X  PDRB: %02X\n",pia_int.portb.cr,
                          pia_int.portb.ddr,mc6821_ReadPort(&pia_int.portb));
    ptr+=sprintf(ptr,"CRA: %02X  DDRA: %02X  PDRA: %02X\n",pia_ext.porta.cr,
                          pia_ext.porta.ddr,mc6821_ReadPort(&pia_ext.porta));
    ptr+=sprintf(ptr,"CRB: %02X  DDRB: %02X  PDRB: %02X\n",pia_ext.portb.cr,
                          pia_ext.portb.ddr,mc6821_ReadPort(&pia_ext.portb));
    ptr+=sprintf(ptr,"P_DATA: %02X   P_ADDR: %02X\n",mode_page.p_data,mode_page.p_addr);
    ptr+=sprintf(ptr,"LGAMOD: %02X     SYS1: %02X\n",mode_page.lgamod,mode_page.system1);
    ptr+=sprintf(ptr,"  SYS2: %02X     DATA: %02X\n",mode_page.system2,mode_page.ram_data);
    ptr+=sprintf(ptr,"  CART: %02X   COMMUT: %02X\n",mode_page.cart,mode_page.commut);
    ptr+=sprintf(ptr,"CMD0: %02X  CMD1: %02X  CMD2: %02X\n", disk_ctrl.cmd0, disk_ctrl.cmd1, disk_ctrl.cmd2);
    ptr+=sprintf(ptr," STAT0: %02X    STAT1: %02X\n",disk_ctrl.stat0, disk_ctrl.stat1);
    ptr+=sprintf(ptr," WDATA: %02X    RDATA: %02X\n",disk_ctrl.wdata, disk_ctrl.rdata);
    ptr+=sprintf(ptr,is_fr?"page de ROM cartouche : %d\n":"ROM cartridge page  : %d\n", mempager.cart.rom_page);
    ptr+=sprintf(ptr,is_fr?"page de RAM cartouche : %d\n":"RAM cartridge page  : %d\n", mempager.cart.ram_page);
    ptr+=sprintf(ptr,is_fr?"page de VRAM          : %d\n":"VRAM page           : %d\n", mempager.screen.vram_page);
    ptr+=sprintf(ptr,is_fr?"page de RAM (registre): %d\n":"RAM page (register) : %d\n", mempager.data.reg_page);
    ptr+=sprintf(ptr,is_fr?"page de RAM (PIA)     : %d\n":"RAM page (PIA)      : %d\n", mempager.data.pia_page);
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

            pc=(pc+MC6809_Dasm(string_buffer,(const unsigned char *)fetch_buffer,pc,MC6809_DASM_BINASM_MODE))&0xFFFF;
            if (i==0)
                ptr+=sprintf(ptr, "%s\n", string_buffer);
            else
                ptr+=sprintf(ptr, "%s\n", string_buffer);
        }
        ptr+=sprintf(ptr, "                                                 ");
	return text_buffer;
}


/* update_debug_text:
 *  Mise à jour du texte de la fenêtre de debug
 */
static void update_debug_text (void)
{
    char *markup;

    mc6809_GetRegs(&regs);
    markup = g_markup_printf_escaped ("<span face=\"Courier\">%s</span>", debug_get_6809regs());
    gtk_label_set_markup (GTK_LABEL (label_6809regs), markup);
    g_free (markup);
    markup = g_markup_printf_escaped ("<span face=\"Courier\"><b>S&gt;</b> %s</span>", debug_get_sr_list());
    gtk_label_set_markup (GTK_LABEL (label_sr_list), markup);
    g_free (markup);
    markup = g_markup_printf_escaped ("<span face=\"Courier\">%s</span>", debug_get_dasm());
    gtk_label_set_markup (GTK_LABEL (label_middle_left), markup);
    g_free (markup);
    markup = g_markup_printf_escaped ("<span face=\"Courier\">%s</span>", debug_get_regs());
    gtk_label_set_markup (GTK_LABEL (label_middle_right), markup);
    g_free (markup);
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
       update_debug_text ();
       break;

    case DEBUG_CMD_STEPOVER:
       debug_stepover();
       update_debug_text ();
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
    }

    (void) button;
}



static void InitDEBUG(void)
{
    GtkWidget *content_area;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *widget;
    gchar *markup;

    /* fenêtre d'affichage */
    wDebug = gtk_dialog_new_with_buttons (
                    is_fr?"Teo - DÃ©bogueur":"Teo - Debugger",
                    GTK_WINDOW(wMain),
                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_STOCK_APPLY, GTK_RESPONSE_ACCEPT,
                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                    NULL);
    gtk_window_set_resizable (GTK_WINDOW(wDebug), FALSE);
    content_area = gtk_dialog_get_content_area (GTK_DIALOG(wDebug));

    /* boîte horizontale de la barre de boutons */
    hbox=gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing ( GTK_BOX(hbox), 10);
    gtk_button_box_set_layout ((GtkButtonBox *)hbox, GTK_BUTTONBOX_START);
    gtk_box_pack_start( GTK_BOX(content_area), hbox, FALSE, FALSE, 0);

    widget=gtk_button_new_with_label("Step");
    gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK (do_command_debug), (gpointer) DEBUG_CMD_STEP);

    widget=gtk_button_new_with_label("Step over");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK (do_command_debug), (gpointer) DEBUG_CMD_STEPOVER);

    widget=gtk_button_new_with_label("Set BKPT");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(do_command_debug), (gpointer) DEBUG_CMD_BKPT);

//    address_buf=gtk_entry_buffer_new("0000",6);
//    entry_address=gtk_entry_new_with_buffer(address_buf);
    entry_address=gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(entry_address), 4);
    gtk_entry_set_width_chars (GTK_ENTRY(entry_address), 4);
    g_signal_connect(G_OBJECT(entry_address), "activate", G_CALLBACK(do_command_debug), (gpointer) DEBUG_CMD_BKPT);
    gtk_box_pack_start( GTK_BOX(hbox), entry_address, TRUE, FALSE, 0);

    widget=gtk_button_new_with_label("Reset BKPT");
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, 0);
    g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK (do_command_debug), (gpointer) DEBUG_CMD_REMBKPT);
    
    /* boîte verticale des textes */
    vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start( GTK_BOX(content_area), vbox, FALSE, FALSE, DEBUG_SPACE);

    /* label des registres */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
    widget=gtk_label_new("");
    markup = g_markup_printf_escaped ("<span face=\"Courier\"><b>%s</b></span>",
             "CC            A  B  DP  X    Y    U    S    PC");
    gtk_label_set_markup (GTK_LABEL (widget), markup);
    g_free (markup);
    gtk_box_pack_start( GTK_BOX(hbox), widget, FALSE, FALSE, 0);
    
    /* widget pour les registres 6809 */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
    label_6809regs=gtk_label_new("");
    gtk_box_pack_start( GTK_BOX(hbox), label_6809regs, FALSE, FALSE, 0);
    
    /* label pour lecontenu de la pile S */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start( GTK_BOX(vbox), hbox, TRUE, FALSE, 0);
    label_sr_list=gtk_label_new("");
    markup = g_markup_printf_escaped ("<span face=\"Courier\"><b>S</b>:</span>");
    gtk_label_set_markup (GTK_LABEL (label_sr_list), markup);
    g_free (markup);
    gtk_box_pack_start( GTK_BOX(hbox), label_sr_list, FALSE, FALSE, 0);
    
    /* boîte horizontale du milieu */
    hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,DEBUG_SPACE);
    gtk_box_pack_start( GTK_BOX(content_area), hbox, TRUE, FALSE, DEBUG_SPACE);

    label_middle_left=gtk_label_new("dasm");
    widget=gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(widget), label_middle_left);
    gtk_label_set_selectable (GTK_LABEL(label_middle_left), TRUE);
    gtk_box_pack_start( GTK_BOX(hbox), widget, TRUE, FALSE, DEBUG_SPACE);

    label_middle_right=gtk_label_new("regs");
    gtk_label_set_selectable (GTK_LABEL(label_middle_right), TRUE);
    gtk_box_pack_start( GTK_BOX(hbox), label_middle_right, TRUE, FALSE, DEBUG_SPACE);

    /* affiche tout l'intérieur */
    gtk_widget_show_all (content_area);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();
}


/* DebugPanel:
 *  Affiche le panneau du debug.
 */
void DebugPanel(void)
{
    gint response;

    /* Initialise la fenêtre */
    InitDEBUG();

    /* actualise le texte à afficher */
    update_debug_text ();
    
    /* affichage de la fenêtre principale et de ses éléments */
    gtk_widget_show(wDebug);

    /* gestion des évènements */
    response = gtk_dialog_run (GTK_DIALOG(wDebug));
    switch (response)
    {
        case GTK_RESPONSE_ACCEPT: teo.command=BREAKPOINT; break; /* ????????? */
        case GTK_RESPONSE_CANCEL: teo.command=NONE      ; break;
   }
   gtk_widget_destroy (wDebug);
}


int 
check_bkpt(int pc) {
    int i;
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	if (breakpoint[i]==-1) break;
	if (breakpoint[i]==pc) return 1;
    }
    return 0;
}

