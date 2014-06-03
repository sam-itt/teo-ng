/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2014 Yves Charriau, François Mouret
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
 *  Module     : windows/port.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of serial port.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <windows.h>
   #include <windowsx.h>
   #include <assert.h>
   #include <winioctl.h>
#endif

#include "defs.h"
#include "std.h"
#include "errors.h"
#include "serial.h"
#include "encode.h"
#include "main.h"
#include "win/gui.h"
#include "win/resource.h"

struct KEY_ENTRY {
    HKEY   hKey;
    char   *path;
    DWORD  dwIndex;
    struct KEY_ENTRY *prev;
};

static struct STRING_LIST *port_list = NULL;
static struct STRING_LIST *friendly_list = NULL;



/* delete_key_entry:
 *  Efface une entrée pour la recherche de clefs.
 */
static struct KEY_ENTRY *delete_key_entry(struct KEY_ENTRY *kentry)
{
    struct KEY_ENTRY *prev_entry = kentry->prev;

    free(kentry->path);
    free(kentry);
    return prev_entry;
}



/* add_key_entry:
 *  Ajoute une entrée pour la recherche de clefs.
 */
static struct KEY_ENTRY *add_key_entry(struct KEY_ENTRY *kentry, char *name)
{
    struct KEY_ENTRY *new_entry = malloc(sizeof(struct KEY_ENTRY));

    if (new_entry != NULL)
    {
        if (kentry == NULL)
            new_entry->path = std_strdup_printf ("%s",name);
        else
            new_entry->path = std_strdup_printf ("%s\\%s",kentry->path,name);

        new_entry->prev = kentry;
        new_entry->dwIndex = 0;
    }
    return new_entry;
}



/* get_raw_name:
 *  Extrait le nom du port du FriendlyName.
 */
static char *get_raw_name (char *friendlyName)
{
    static char port_name[6];
    char *startPtr, *endPtr;

    port_name[0] = '\0';
    if (((startPtr = strstr (friendlyName, "(COM")) != NULL)
     && ((endPtr = strstr (startPtr, ")")) != NULL))
        (void)snprintf (port_name, (endPtr - 1 - startPtr > 6)
                                  ? 6 : endPtr-1-startPtr, "%s", startPtr+1);
    return port_name;
}



/* friendly_list:
 *  Récupère le nom et le "friendly name" du port pour Windows 9x et +.
 *
 * Le répertoire HKEY_LOCAL_MACHINE\xxxx
 * est visité récursivement
 */
static void build_friendly_list (char *dname)
{
    char lpName[MAX_PATH];
    DWORD lpcName;
    char lpFriendly[MAX_PATH];
    DWORD lpcFriendly;
    DWORD lpType;
    struct KEY_ENTRY *hKent = NULL;
    char *portRawName;
    struct STRING_LIST *list = NULL;

    hKent = add_key_entry (hKent, dname);

    while(hKent != NULL)
    {
        /* Ouvre la clef */
        if (RegOpenKeyExA (HKEY_LOCAL_MACHINE, hKent->path, 0,
                         KEY_READ, &hKent->hKey) == ERROR_SUCCESS)
        {
            /* Recherche la clef demandée */
            lpName[0] = '\0';
            lpcName = MAX_PATH;
            if ((RegEnumKeyExA (hKent->hKey, hKent->dwIndex, (LPTSTR)lpName,
                              &lpcName, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
            {
                hKent->dwIndex++;
                hKent = add_key_entry (hKent,lpName);
            }
            else
            {
                /* Capture la valeur FRIENDLYNAME */
                lpType = REG_SZ;
                lpFriendly[0] = '\0';
                lpcFriendly = MAX_PATH;
                if (RegQueryValueExA (hKent->hKey, TEXT("FRIENDLYNAME"), NULL,
                            &lpType, (LPBYTE)lpFriendly, &lpcFriendly) == ERROR_SUCCESS)
                {
                    /* Enregistre si port friendlyname */
                    lpFriendly[lpcFriendly] = '\0';
                    portRawName = get_raw_name (lpFriendly);
                    if (*portRawName != '\0')
                    {
                        for (list = port_list;
                             (list != NULL) && (strcmp (lpFriendly, (char *)list->str) == 0);
                             list = list->next);
                        if ((list == NULL) && (serial_Open (portRawName) == 0))
                        {
                            port_list = std_StringListAppend (port_list, (void *)portRawName);
                            friendly_list = std_StringListAppend (friendly_list, (void *)lpFriendly);
                            serial_Close ();
                        }
                    }
                }
                RegCloseKey (hKent->hKey);
                hKent = delete_key_entry (hKent);
            }
        }
        else if (hKent != NULL) hKent = delete_key_entry(hKent);
    }
}



/* friendly_list_nt4:
 *  Récupère le nom et le "friendly name" du port pour Windows NT4.
 *
 * COM1 à COM15
 */
static void build_friendly_list_nt4 (void)
{
    int i;
    char *port_name = NULL;
    char *friendly_name = NULL;

    for (i=0; i>15; i++)
    {
         port_name = std_strdup_printf ("COM%d", i);
         friendly_name = std_strdup_printf ("%s (COM%d)",
                                       is_fr?"Port de communication"
                                            :"Communication Port", i);
         if (serial_Open (port_name) == 0)
         {
             port_list = std_StringListAppend (port_list, (void *)port_name);
             friendly_list = std_StringListAppend (friendly_list, (void *)friendly_name);
             serial_Close ();
         }
         port_name = std_free (port_name);
         friendly_name = std_free (friendly_name);
    }
}


/* multi_ports_console:
 *  Output in console if several ports found.
 */
static void multi_ports_console (void)
{
    int i;
    int choice;
    int res;
    struct STRING_LIST *list;

    printf ("%s",
         (is_fr)
         ? encode_String(
           "\nDes ports série connectés ont été trouvés.\n" \
           "Veuillez choisir le plus approprié ci-dessous.\n")
         : "\nSome connected serial ports have been found.\n" \
           "Please choose the appropriate one below.\n");

    for (i=1, list=friendly_list; list!=NULL; list=list->next, i++)
        (void)printf ("%d. %s\n", i, list->str);

     do {
         printf ("\n%s", is_fr?"Votre choix : "
                              :"Your choice : ");
         res = scanf (" %d", &choice);
     } while ((res == EOF) && (choice<1) && (choice>=i));

     for (i=1,list=port_list; i!=choice; i++)
         list=list->next;

    gui.port_name = std_free (gui.port_name);
    gui.port_name = std_strdup_printf ("%s", list->str);
}




/*
 * Boîte de dialogue si plusieurs ports série valides
 */
static LRESULT CALLBACK detect_dialog (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int i;
    static HWND cDetectLText;
    static HWND cDetectComboBox;
    struct STRING_LIST *list;
    int index = 0;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            cDetectLText    = GetDlgItem (hDlg, IDC_DETECT_LTEXT);
            cDetectComboBox = GetDlgItem (hDlg, IDC_DETECT_COMBO);

            if (is_fr)
                Static_SetText (cDetectLText,
                    encode_String (
                    "Des ports série connectés ont été trouvés.\n\n" \
                    "Veuillez choisir le plus approprié ci-dessous.\n"));
            else
                Static_SetText (cDetectLText,
                    "Some connected serial ports have been found.\n\n" \
                    "Please choose the appropriate one below.\n");

            for (list=friendly_list; list!=NULL; list=list->next)
                ComboBox_AddString (cDetectComboBox, (char *)list->str);
            ComboBox_SetCurSel (cDetectComboBox, 0);
            break;

        case WM_COMMAND :
            switch(LOWORD(wParam))
            {
                case IDC_OK_BUTTON :
                case IDCANCEL :
                    index = ComboBox_GetCurSel (cDetectComboBox);
                    for (i=1,list=port_list; i<index; i++)
                         list=list->next;
                    gui.port_name = std_free (gui.port_name);
                    gui.port_name = std_strdup_printf ("%s", list->str);
                    EndDialog(hDlg, FALSE);
                    break;
            }
            break;
    }
    return 0;

    (void)lParam;
}



/* detect_port:
 *  Detect the serial port.
 */
static int detect_port (void)
{
    int err = 0;
    OSVERSIONINFO osvi;

    /* Recherche des ports série valides */
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    if ((osvi.dwMajorVersion < 5)                       /* Version Windows */
     && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))   /* NT */
         /* Pas de recherche pour NT4 (COM1 à COM16) */
         build_friendly_list_nt4 ();
    else
    {
         /* Recherche par base de registre pour autres Windows */
         build_friendly_list ("System");
         build_friendly_list ("Enum");
         build_friendly_list ("Hardware");
    }

    switch (std_StringListLength (port_list))
    {
        /* if no device found, error */
        case 0 :
            err = error_Message (CC90HFE_ERROR_PORT_NONE, NULL);
            break;

        case 1 :
            gui.port_name = std_free (gui.port_name);
            gui.port_name = std_strdup_printf ("%s", (char *)port_list->str);
            break;

        default :
            if (windowed_mode != 0)
            {
                (void)MessageBeep (MB_OK);
                (void)DialogBox( hInst, MAKEINTRESOURCE(ID_DETECT),
                         main_window, (DLGPROC)detect_dialog);
            }
            else
                multi_ports_console ();
            break;
    }
    std_StringListFree (port_list);
    port_list = NULL;
    std_StringListFree (friendly_list);
    friendly_list = NULL;
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
     || (*gui.port_name == '\0')
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

