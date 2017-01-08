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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : win/wdebug/wdmem.c
 *  Version    : 1.8.4
 *  Créé par   : Gilles Fétis & François Mouret 10/05/2014
 *  Modifié par: François Mouret 15/07/2016
 *
 *  Débogueur 6809 - Affichage de la mémoire.
 */


#ifndef SCAN_DEPEND
    #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "hardware.h"
#include "mc68xx/dasm6809.h"
#include "debug/debug.h"
#include "win/gui.h"

static HFONT hfont_normal = NULL;

static int prev_address = -1;


/* init_addresses_combo:
 *  Initialization of the combobox.
 */
static void init_addresses_combo (HWND hDlg)
{
    int i;
    HWND hwnd;
    char addr[6] = "";
    
    /* initialize the addresses combo */
    hwnd = GetDlgItem(hDlg, IDC_DEBUG_MEM_COMBO);
    ComboBox_ResetContent(hwnd);
    for (i=0; i<8; i++)
    {
        *addr = '\0';
        sprintf (addr, "$%04X", i*0x2000);
        ComboBox_AddItemData(hwnd, addr);
    }
    ComboBox_SetCurSel(hwnd, (teo.debug.memory_address >> 13) & 0x7);

    /* initialize the RAM combo */
    hwnd = GetDlgItem(hDlg, IDC_DEBUG_RAM_COMBO);
    ComboBox_ResetContent(hwnd);
    for (i=0; i<teo.setting.bank_range; i++)
    {
        *addr = '\0';
        sprintf (addr, "%d", i);
        ComboBox_AddItemData(hwnd, addr);
    }
    ComboBox_SetCurSel(hwnd, teo.debug.ram_number);

    /* initialize the video combo */
    hwnd = GetDlgItem(hDlg, IDC_DEBUG_VIDEO_COMBO);
    ComboBox_ResetContent(hwnd);
    ComboBox_AddItemData(hwnd, is_fr?"Forme":"Form");
    ComboBox_AddItemData(hwnd, is_fr?"Couleur":"Colour");
    ComboBox_SetCurSel(hwnd, teo.debug.video_number);

    /* initialize the CART combo */
    hwnd = GetDlgItem(hDlg, IDC_DEBUG_CART_COMBO);
    ComboBox_ResetContent(hwnd);
    for (i=0; i<4; i++)
    {
        *addr = '\0';
        sprintf (addr, "%d", i);
        ComboBox_AddItemData(hwnd, addr);
    }
    ComboBox_AddItemData(hwnd, is_fr?"Memo":"Memo");
    for (i=0; i<teo.setting.bank_range; i++)
    {
        *addr = '\0';
        sprintf (addr, is_fr?"Banque %d":"Bank %d", i);
        ComboBox_AddItemData(hwnd, addr);
    }
    ComboBox_SetCurSel(hwnd, teo.debug.cart_number);

    /* initialize the Monitor combo */
    hwnd = GetDlgItem(hDlg, IDC_DEBUG_MON_COMBO);
    ComboBox_ResetContent(hwnd);
    for (i=0; i<2; i++)
    {
        *addr = '\0';
        sprintf (addr, "%d", i);
        ComboBox_AddItemData(hwnd, addr);
    }
    ComboBox_SetCurSel(hwnd, teo.debug.mon_number);
}



/* set_text:
 *  Set the text in the memory edit control.
 */
static void set_text (HWND hDlg, int address, int dmem_scroll, uint8 *addr_ptr)
{
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_EDIT);
    char *text;

    /* display the new text */
    text = dmem_GetText (address&0xe000, addr_ptr, "\r\n");
    if (text != NULL)
    {
        Edit_SetText (hwnd, text);
        Edit_Scroll (hwnd, dmem_scroll, 0);
        std_free(text);
    }
}



/* display:
 *  Display the disassembling.
 */
static void display (HWND hDlg, int address, uint8 *addr_ptr)
{ 
    int index;
    int dmem_line = (address & 0x1fff) >> 3;
    int dmem_scroll;
    RECT rect;
    int visible_line_count;
    int visible_line_first;
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_EDIT);

    /* compute scroll delta */
    Edit_GetRect(hwnd, &rect);
    visible_line_count = (int)((rect.bottom-rect.top)/FIXED_WIDTH_FONT_HEIGHT);
    visible_line_first = (int)Edit_GetFirstVisibleLine(hwnd);

    /* address < first visible line */
    if (dmem_line < visible_line_first)
        dmem_scroll = dmem_line;
    else
    /* address inside visible window */
    if (dmem_line < (visible_line_first+visible_line_count))
        dmem_scroll = visible_line_first;
    else
    /* address > last visible line */
        dmem_scroll = dmem_line-visible_line_count+1;

    set_text(hDlg, address, dmem_scroll, addr_ptr);

    /* highlight selection */
    index = Edit_LineIndex (hwnd, dmem_line);
    index += 6 + (address&0x07)*3;
    Edit_SetSel (hwnd, index, index+2);
}


/* ------------------------------------------------------------------------- */


/* wdmem_Init:
 *  Init memory area.
 */
void wdmem_Init(HWND hDlg)
{ 
    HWND hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_EDIT);

    hfont_normal = wdebug_GetNormalFixedWidthHfont();
    if (hfont_normal != NULL)
        SetWindowFont(hwnd, hfont_normal, TRUE);

    hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_LTEXT);
    SetWindowText(hwnd, is_fr?"Adresse:":"Address:");
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_CART_LTEXT);
    SetWindowText(hwnd, is_fr?"Cartouche:":"Cartridge:");
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_VIDEO_LTEXT);
    SetWindowText(hwnd, is_fr?"Vidéo:":"Video:");
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_RAM_LTEXT);
    SetWindowText(hwnd, is_fr?"RAM:":"RAM:");
    hwnd = GetDlgItem (hDlg, IDC_DEBUG_MON_LTEXT);
    SetWindowText(hwnd, is_fr?"Monit:":"Monit:");

    init_addresses_combo(hDlg);
    prev_address = -1;
}



/* wdmem_StepDisplay:
 *  Display memory for disassembling.
 */
void wdmem_StepDisplay(HWND hDlg, int address)
{
    HWND hwnd;
    int index = 0;

    if  (address == -1)
        address = prev_address;

    if (address != -1)
    {
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_MEM_COMBO);
        ComboBox_SetCurSel(hwnd, (address >> 13) & 0x0f);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_RAM_COMBO);
        ComboBox_SetCurSel(hwnd, mempager.data.reg_page);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_VIDEO_COMBO);
        ComboBox_SetCurSel(hwnd, 1-mempager.screen.page);
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_MON_COMBO);
        ComboBox_SetCurSel(hwnd, mempager.mon.page);
        if (mode_page.cart&0x20)
            index = 5+mempager.cart.ram_page;
        else
        if (mc6846.prc&4)
            index = mempager.cart.rom_page;
        else
            index = 4;
        hwnd = GetDlgItem (hDlg, IDC_DEBUG_CART_COMBO);
        ComboBox_SetCurSel(hwnd, index);
        display(hDlg, address, NULL);
    }
}



/* wdmem_Display:
 *  Display choosen memory slice.
 */
void wdmem_Display(HWND hDlg)
{
    set_text (hDlg,
              teo.debug.memory_address,
              (teo.debug.memory_address&0x1fff)>>3,
              dmem_GetDisplayPointer());
}



/* wdmem_Exit:
 *  Exit the memory area.
 */
void wdmem_Exit(HWND hDlg)
{
    if (hfont_normal != NULL)
    {
        (void)DeleteObject((HGDIOBJ)hfont_normal);
        hfont_normal = NULL;
    }

    (void)hDlg;
}
