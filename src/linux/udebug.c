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
 *  Copyright (C) 2011 Gilles Fétis
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
 *  Module     : src/linux/udebug.c
 *  Version    : 1.8.0
 *  Créé par   : Gilles Fétis 27/07/2011
 *  Modifié par:
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
#include "linux/filentry.h"
#include "linux/graphic.h"
#include "linux/main.h"
#include "linux/message.h"
#include "linux/question.h"
#include "linux/debugger.h"
#include "to8.h"

#define DEBUG_SPACE 5

#define DEBUG_CMD_STEP 0
#define DEBUG_CMD_STEPOVER 1
#define DEBUG_CMD_RUN 2
#define DEBUG_CMD_BKPT 3
#define DEBUG_CMD_REMBKPT 4

extern char* debug_get_dasm(void);
extern void debug_step(void);
extern void debug_stepover(void);
extern char* debug_get_regs(void);
extern void debug_bkpt(int);
extern void debug_rembkpt(void);

/* fenêtre de l'interface utilisateur */
static GtkWidget * window_debug;
// static GtkEntryBuffer * address_buf;
static GtkWidget *entry_address;
static GtkWidget *label_middle_left,*label_middle_right;



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



static void do_exit_debug(GtkWidget *button, int command)
{
    teo.command=command;

    gtk_widget_hide(window_debug);
    gtk_main_quit();

    (void) button;
}


static int do_hide_debug(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    teo.command=NONE;
    gtk_widget_hide(widget);
    gtk_main_quit();
    return TRUE;

    (void) event;
    (void) user_data;
}



static void do_command_debug(GtkWidget *button, int command)
{
    int addr;
    switch (command) {
    case DEBUG_CMD_STEP:
       debug_step();
       gtk_label_set_text (GTK_LABEL(label_middle_left),debug_get_dasm());
       gtk_label_set_text (GTK_LABEL(label_middle_right),debug_get_regs());
    break;
    case DEBUG_CMD_STEPOVER:
       debug_stepover();
       gtk_label_set_text (GTK_LABEL(label_middle_left),debug_get_dasm());
       gtk_label_set_text (GTK_LABEL(label_middle_right),debug_get_regs());
    break;
    case DEBUG_CMD_RUN:
       teo.command=BREAKPOINT;
       gtk_widget_hide(window_debug);
       gtk_main_quit();
    break;
    case DEBUG_CMD_BKPT:
       addr=0;
       sscanf(gtk_entry_get_text (GTK_ENTRY(entry_address)),"%X",&addr);
       debug_bkpt(addr);
       gtk_label_set_text (GTK_LABEL(label_middle_right),debug_get_regs());
    break;
    case DEBUG_CMD_REMBKPT:
       debug_rembkpt();
       gtk_label_set_text (GTK_LABEL(label_middle_right),debug_get_regs());
    break;
    }

    (void) button;
}



#if !BEFORE_GTK_2_MIN
static void iconify_teo_window (GtkWidget *widget, GdkEvent *event, void *user_data)
{
    if (event->window_state.new_window_state == GDK_WINDOW_STATE_ICONIFIED) {
        gtk_window_iconify (GTK_WINDOW(widget_win));
    }
    (void)widget;
    (void)user_data;
}
#endif



void InitDEBUG(void)
{
    GtkWidget *vbox_window;
    GtkWidget *hbox_top;
    GtkWidget *but_quit,*but_step,*but_stepover,*but_run,*but_bkpt,*but_rembkpt;

    GtkWidget *hbox_middle;

    GtkWidget *hbox_bottom;

    printf("Init debugger...");

    /* fenêtre d'affichage */
    window_debug=gtk_window_new(GTK_WINDOW_TOPLEVEL);
#if BEFORE_GTK_2_MIN
    gtk_window_set_position(GTK_WINDOW(window_debug), GTK_WIN_POS_CENTER);
#else
    gtk_window_set_transient_for (GTK_WINDOW(window_debug), GTK_WINDOW(widget_win));
    gtk_window_set_position(GTK_WINDOW(window_debug), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_destroy_with_parent (GTK_WINDOW(window_debug), TRUE);
    gtk_window_set_modal (GTK_WINDOW(window_debug), TRUE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(window_debug), TRUE);
    xgtk_signal_connect(window_debug, "window-state-event", iconify_teo_window, (gpointer)NULL);
#endif
    gtk_window_set_title(GTK_WINDOW(window_debug), is_fr?"Teo - DÃ©bogueur":"Teo - Debugger");
    xgtk_signal_connect(window_debug, "delete-event", do_hide_debug, (gpointer)NULL);
    gtk_widget_realize(window_debug);

    /* boîte verticale associée à la fenêtre */
    vbox_window=gtk_vbox_new(FALSE,DEBUG_SPACE);
    gtk_container_set_border_width( GTK_CONTAINER(vbox_window), DEBUG_SPACE);
    gtk_container_add( GTK_CONTAINER(window_debug), vbox_window);
    gtk_widget_show(vbox_window);

    /* boîte horizontale de la barre de boutons */
    hbox_top=gtk_hbox_new(FALSE,DEBUG_SPACE);
    gtk_box_pack_start( GTK_BOX(vbox_window), hbox_top, TRUE, FALSE, DEBUG_SPACE);
    gtk_widget_show(hbox_top);

    but_quit=gtk_button_new_with_label("Quit");
    xgtk_signal_connect( but_quit, "clicked", do_exit_debug, (gpointer) NONE);
    gtk_box_pack_start( GTK_BOX(hbox_top), but_quit, TRUE, FALSE, 0);
    gtk_widget_show(but_quit);

    but_step=gtk_button_new_with_label("Step");
    gtk_box_pack_start( GTK_BOX(hbox_top), but_step, TRUE, FALSE, 0);
    xgtk_signal_connect(but_step, "clicked", do_command_debug, (gpointer) DEBUG_CMD_STEP);
    gtk_widget_show(but_step);


    but_stepover=gtk_button_new_with_label("Step over");
    gtk_box_pack_start( GTK_BOX(hbox_top), but_stepover, TRUE, FALSE, 0);
    xgtk_signal_connect(but_stepover, "clicked", do_command_debug, (gpointer) DEBUG_CMD_STEPOVER);
    gtk_widget_show(but_stepover);

    but_run=gtk_button_new_with_label("Run");
    gtk_box_pack_start( GTK_BOX(hbox_top), but_run, TRUE, FALSE, 0);
    xgtk_signal_connect(but_run, "clicked", do_command_debug, (gpointer) DEBUG_CMD_RUN);
    gtk_widget_show(but_run);

    but_bkpt=gtk_button_new_with_label("Set BKPT");
    gtk_box_pack_start( GTK_BOX(hbox_top), but_bkpt, TRUE, FALSE, 0);
    xgtk_signal_connect(but_bkpt, "clicked", do_command_debug, (gpointer) DEBUG_CMD_BKPT);
    gtk_widget_show(but_bkpt);

//    address_buf=gtk_entry_buffer_new("0000",6);
//    entry_address=gtk_entry_new_with_buffer(address_buf);
    entry_address=gtk_entry_new();
    gtk_box_pack_start( GTK_BOX(hbox_top), entry_address, TRUE, FALSE, 0);
    gtk_widget_show(entry_address);

    but_rembkpt=gtk_button_new_with_label(is_fr?"Supprimer les BKPT":"Remove all BKPT");
    gtk_box_pack_start( GTK_BOX(hbox_top), but_rembkpt, TRUE, FALSE, 0);
    xgtk_signal_connect(but_rembkpt, "clicked", do_command_debug, (gpointer) DEBUG_CMD_REMBKPT);
    gtk_widget_show(but_rembkpt);

    /* boîte horizontale du milieu */
    hbox_middle=gtk_hbox_new(FALSE,DEBUG_SPACE);
    gtk_box_pack_start( GTK_BOX(vbox_window), hbox_middle, TRUE, FALSE, DEBUG_SPACE);
    gtk_widget_show(hbox_middle);

    label_middle_left=gtk_label_new("dasm");
    gtk_box_pack_start( GTK_BOX(hbox_middle), label_middle_left, TRUE, FALSE, DEBUG_SPACE);
    gtk_widget_show(label_middle_left);

    label_middle_right=gtk_label_new("regs");
    gtk_box_pack_start( GTK_BOX(hbox_middle), label_middle_right, TRUE, FALSE, DEBUG_SPACE);
    gtk_widget_show(label_middle_right);

    /* boîte horizontale du bas */
    hbox_bottom=gtk_hbox_new(FALSE,DEBUG_SPACE);
    gtk_box_pack_start( GTK_BOX(vbox_window), hbox_bottom, TRUE, FALSE, DEBUG_SPACE);
    gtk_widget_show(hbox_bottom);

    /* Attend la fin du travail de GTK */
    while (gtk_events_pending ())
        gtk_main_iteration ();

    /* mise en place d'un hook pour assurer le retraçage de la fenêtre
       principale de l'émulateur */
#if BEFORE_GTK_2_MIN
    gdk_window_add_filter(gdk_window_foreign_new(screen_win), retrace_callback, NULL);
#else
    gdk_window_add_filter(GTK_WIDGET(widget_win)->window, retrace_callback, NULL);
#endif
    printf("ok\n");
}



/* ControlPanel:
 *  Affiche le panneau de contrôle.
 */
void DebugPanel(void)
{
    gtk_label_set_text (GTK_LABEL(label_middle_left),debug_get_dasm());
    gtk_label_set_text (GTK_LABEL(label_middle_right),debug_get_regs());

    /* affichage de la fenêtre principale et de ses éléments */
    gtk_widget_show(window_debug);

    /* boucle de gestion des évènements */
    gtk_main();
}


/* 
 ===============================================================================
*/

#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"
#include "to8dbg.h"

#define DASM_NLINES 24

#define MAX_BREAKPOINTS  4
static int breakpoint[MAX_BREAKPOINTS]={-1,-1,-1,-1};

static struct MC6809_REGS regs, prev_regs;

static char fetch_buffer[MC6809_FETCH_BUFFER_SIZE]="",
            dasm_buffer[MC6809_DASM_BUFFER_SIZE]="";
static char dasm_buffer2[MC6809_DASM_BUFFER_SIZE*DASM_NLINES]="";

static char regs_buffer[128]="";
static char regs_buffer2[2048]="";

static void DisplayRegs(void)
{
    int i;
    regs_buffer2[0]='\0';
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	if (breakpoint[i]==-1) break;
    	sprintf(regs_buffer,"BKPT[%d]=%04X\n",i,breakpoint[i]);
    	strcat(regs_buffer2,regs_buffer);
	}

    sprintf(regs_buffer,"PC: %04X [%04X]\nDP: %02X\n",regs.pc,prev_regs.pc,regs.dp);
    strcat(regs_buffer2,regs_buffer);
    sprintf(regs_buffer," A: %02X\nB: %02X\n",regs.ar,regs.br);
    strcat(regs_buffer2,regs_buffer);
    sprintf(regs_buffer," X: %04X\nY: %04X\n",regs.xr,regs.yr);
    strcat(regs_buffer2,regs_buffer);
    sprintf(regs_buffer," U: %04X\nS: %04X\n",regs.ur,regs.sr);
    strcat(regs_buffer2,regs_buffer);
    sprintf(regs_buffer,"CC: %c%c%c%c%c%c%c%c   IRQ: %d\n" ,regs.cc&0x80 ? 'E' : '.'
                                             ,regs.cc&0x40 ? 'F' : '.'
                                             ,regs.cc&0x20 ? 'H' : '.'
                                             ,regs.cc&0x10 ? 'I' : '.'
                                             ,regs.cc&0x08 ? 'N' : '.'
                                             ,regs.cc&0x04 ? 'Z' : '.'
                                             ,regs.cc&0x02 ? 'V' : '.'
                                             ,regs.cc&0x01 ? 'C' : '.'
                                             ,mc6809_irq);

    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"[S ] [S+1] [S+2] [S+3] [S+4]\n");
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer," %2X   %2X    %2X    %2X    %2X\n",LOAD_BYTE(regs.sr),LOAD_BYTE(regs.sr+1),
              LOAD_BYTE(regs.sr+2),LOAD_BYTE(regs.sr+3),LOAD_BYTE(regs.sr+4));

    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer," CSR: %02X   CRC: %02X\n",mc6846.csr,mc6846.crc);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"DDRC: %02X   PRC: %02X\n",mc6846.ddrc,mc6846.prc);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer," TCR: %02X  TMSB: %02X  TLSB: %02X\n",mc6846.tcr,mc6846.tmsb,
      mc6846.tlsb);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"CRA: %02X  DDRA: %02X  PDRA: %02X\n",pia_int.porta.cr,
      pia_int.porta.ddr,mc6821_ReadPort(&pia_int.porta));
    strcat(regs_buffer2,regs_buffer);


    sprintf(regs_buffer,"CRB: %02X  DDRB: %02X  PDRB: %02X\n",pia_int.portb.cr,
      pia_int.portb.ddr,mc6821_ReadPort(&pia_int.portb));
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"CRA: %02X  DDRA: %02X  PDRA: %02X\n",pia_ext.porta.cr,
      pia_ext.porta.ddr,mc6821_ReadPort(&pia_ext.porta));
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"CRB: %02X  DDRB: %02X  PDRB: %02X\n",pia_ext.portb.cr,
      pia_ext.portb.ddr,mc6821_ReadPort(&pia_ext.portb));
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"P_DATA: %02X   P_ADDR: %02X\n",mode_page.p_data,mode_page.p_addr);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"LGAMOD: %02X     SYS1: %02X\n",mode_page.lgamod,mode_page.system1);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"  SYS2: %02X     DATA: %02X\n",mode_page.system2,mode_page.ram_data);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"  CART: %02X   COMMUT: %02X\n",mode_page.cart,mode_page.commut);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,"CMD0: %02X  CMD1: %02X  CMD2: %02X\n", disk_ctrl.cmd0,
      disk_ctrl.cmd1, disk_ctrl.cmd2);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer," STAT0: %02X    STAT1: %02X\n",disk_ctrl.stat0, disk_ctrl.stat1);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer," WDATA: %02X    RDATA: %02X\n",disk_ctrl.wdata, disk_ctrl.rdata);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,is_fr?"page de ROM cartouche : %d\n":"ROM cartridge page  : %d\n", mempager.cart.rom_page);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,is_fr?"page de RAM cartouche : %d\n":"RAM cartridge page  : %d\n", mempager.cart.ram_page);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,is_fr?"page de VRAM          : %d\n":"VRAM page           : %d\n", mempager.screen.vram_page);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,is_fr?"page de RAM (registre): %d\n":"RAM page (register) : %d\n", mempager.data.reg_page);
    strcat(regs_buffer2,regs_buffer);

    sprintf(regs_buffer,is_fr?"page de RAM (PIA)     : %d\n":"RAM page (PIA)      : %d\n", mempager.data.pia_page);
    strcat(regs_buffer2,regs_buffer);
}



char* debug_get_regs(void) {
        mc6809_GetRegs(&regs);
        DisplayRegs();
	return regs_buffer2;
}

char* debug_get_dasm(void) { 
        int i,j,pc;
	dasm_buffer2[0]='\0';
        mc6809_GetRegs(&regs);
        pc=regs.pc;
        for (i=0; i<DASM_NLINES; i++)
        {
            for (j=0; j<5; j++)
                fetch_buffer[j]=LOAD_BYTE(pc+j);

            pc=(pc+MC6809_Dasm(dasm_buffer,(const unsigned char *)fetch_buffer,pc,MC6809_DASM_BINASM_MODE))&0xFFFF;
            strcat(dasm_buffer2,dasm_buffer);
            strcat(dasm_buffer2,"\n");
        }
	return dasm_buffer2;
}

void
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

void
debug_step(void) {
            mc6809_StepExec(1);
}

void debug_bkpt(int addr) {
    int i;
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	if (breakpoint[i]==-1) break;
    }
    if (i==MAX_BREAKPOINTS) i=(MAX_BREAKPOINTS-1);
    breakpoint[i]=addr;
}

void debug_rembkpt(void) {
    int i;
    for (i=0;i< MAX_BREAKPOINTS;i++) {
	breakpoint[i]=-1;
	}
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

