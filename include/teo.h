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
 *                  L'�mulateur Thomson TO8
 *
 *  Copyright (C) 1997-2012 Gilles F�tis, Eric Botcazou, Alexandre Pukall,
 *                          J�r�mie Guillaume, Fran�ois Mouret,
 *                          Samuel Devulder
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
 *  Module     : to8.h
 *  Version    : 1.8.2
 *  Cr�� par   : Gilles F�tis
 *  Modifi� par: Eric Botcazou 26/10/2003
 *               Fran�ois Mouret 30/01/2010 07/08/2011 05/03/2012
 *                               01/11/2012
 *               Gilles F�tis 27/07/2011
 *               Samuel Devulder 05/02/2012
 *
 *  Module de pilotage de l'�mulateur.
 */


#ifndef TO8_H
#define TO8_H

#ifndef MAX_PATH
#   define MAX_PATH 300
#endif

#ifdef UNIX_TOOL
#   define FOLDER_SLASH "/"
#else
#   define FOLDER_SLASH "\\"
#endif

/* param�tres et symboles de l'�mulation */
#define TO8_VERSION_STR "1.8.2"
#define PROG_NAME   "teo"
#define PROG_CLASS  "EmuTO"

#define ALLEGRO_CONFIG_FILE  "allegro.cfg"

#define TO8_FRAME_FREQ         50    /* Hz: fr�quence de trame vid�o */
#define TO8_CYCLES_PER_FRAME   19968 /* dur�e exacte en nb de cycles CPU d'une frame */
#define TO8_CPU_FREQ           (TO8_FRAME_FREQ*TO8_CYCLES_PER_FRAME)  /* Hz */
#define TO8_NCOLORS            16    /* nombre de couleurs de la palette */ 

#define TO8_WINDOW_W           320   /* largeur de la fen�tre de travail */     
#define TO8_BORDER_W           16    /* largeur de la bordure de l'�cran */
#define TO8_SCREEN_W           (TO8_WINDOW_W + TO8_BORDER_W*2)  /* largeur de l'�cran */
#define TO8_WINDOW_H           200   /* hauteur de la fen�tre de travail */
#define TO8_BORDER_H           16    /* hauteur de la bordure de l'�cran */
#define TO8_SCREEN_H           (TO8_WINDOW_H + TO8_BORDER_H*2)  /* hauteur de l'�cran */

#define TO8_GPL_SIZE           8     /* taille d'un Groupe Point Ligne */
#define TO8_WINDOW_GW          (TO8_WINDOW_W/TO8_GPL_SIZE)  /* largeur de la fen�tre de travail en GPL */

#define TO8_CHAR_SIZE          8     /* taille d'un caract�re */
#define TO8_WINDOW_CW          (TO8_WINDOW_W/TO8_CHAR_SIZE)  /* largeur de la fen�tre de travail en CHAR */
#define TO8_BORDER_CW          (TO8_BORDER_W/TO8_CHAR_SIZE)  /* largeur de la bordure de l'�cran en CHAR */
#define TO8_SCREEN_CW          (TO8_SCREEN_W/TO8_CHAR_SIZE)  /* largeur de l'�cran en CHAR */
#define TO8_WINDOW_CH          (TO8_WINDOW_H/TO8_CHAR_SIZE)  /* hauteur de la fen�tre de travail en CHAR */
#define TO8_BORDER_CH          (TO8_BORDER_H/TO8_CHAR_SIZE)  /* hauteur de la bordure de l'�cran en CHAR */
#define TO8_SCREEN_CH          (TO8_SCREEN_H/TO8_CHAR_SIZE)  /* hauteur de l'�cran en CHAR */

#define TO8_LEFT_BORDER        0x40  /* bitmask */
#define TO8_RIGHT_BORDER       0x80  /* bitmask */

#define TO8_COL40              0     /* mode 40 colonnes 16 couleurs */
#define TO8_BITMAP4            0x21  /* mode bitmap 4 couleurs       */
#define TO8_PAGE1              0x24  /* mode commutation page 1      */
#define TO8_PAGE2              0x25  /* mode commutation page 2      */
#define TO8_STACK2             0x26  /* mode superposition 2 pages   */
#define TO8_COL80              0x2A  /* mode 80 colonnes 2 couleurs  */
#define TO8_STACK4             0x3F  /* mode superposition 4 pages   */
#define TO8_BITMAP4b           0x41  /* mode bitmap 4 non document�  */
#define TO8_BITMAP16           0x7B  /* mode bitmap 16 couleurs      */
#define TO8_PALETTE            0xFF  /* mode �cran de la palette     */

#define TO8_PALETTE_ADDR       0x1A18    /* adresse de transition des palettes */
#define TO8_PALETTE_COL1       0x0000FF  /* triplet RRGGBB couleur 1 */    
#define TO8_PALETTE_COL2       0xD7FBEF  /* triplet RRGGBB couleur 2 */

// #define TO8_MEMO7_LABEL_LENGTH 25

#define TO8_SHIFT_FLAG         (1<<0)
#define TO8_CTRL_FLAG          (1<<1)
#define TO8_ALTGR_FLAG         (1<<2)
#define TO8_NUMLOCK_FLAG       (1<<3)
#define TO8_CAPSLOCK_FLAG      (1<<4)
#define TO8_MAX_FLAG           5

#define TO8_NJOYSTICKS         2
#define TO8_NBUTTONS           2
#define TO8_JOYSTICK_CENTER    0
#define TO8_JOYSTICK_LEFT      (1<<0)
#define TO8_JOYSTICK_RIGHT     (1<<1)
#define TO8_JOYSTICK_UP        (1<<2)
#define TO8_JOYSTICK_DOWN      (1<<3)
#define TO8_JOYSTICK_FIRE_ON   1
#define TO8_JOYSTICK_FIRE_OFF  0

#define TO8_TRAP_CODE          0xCD   /* code pour le trap */

enum teo_command {
    TEO_COMMAND_NONE = 1,
    TEO_COMMAND_PANEL,
    TEO_COMMAND_SCREENSHOT,
    TEO_COMMAND_DEBUGGER,
    TEO_COMMAND_BREAKPOINT,
    TEO_COMMAND_RESET,
    TEO_COMMAND_COLD_RESET,
    TEO_COMMAND_QUIT
};

enum {
    TO8_READ_ONLY = 1,
    TO8_READ_WRITE,
    TO8_MOUSE,
    TO8_LIGHTPEN,
    TO8_PRINTER_ONLINE,
    TO8_PRINTER_LINE_FEED,
    TO8_PRINTER_FORM_FEED 
};

extern int is_fr;

#define NBDRIVE    4

struct EMUTEO {
    int sound_enabled;
    volatile enum teo_command command;
    char *default_folder;
    struct EMUTEO_SETTINGS {
        int  exact_speed;
        int  sound_volume;
        int  sound_enabled;
        int  interlaced_video;
        } setting;
    struct EMUTEO_DISK {
        int  direct_access_allowed;
        int  write_protect;
        char *file;
        } disk[NBDRIVE];
    struct EMUTEO_CASS  {
        int  write_protect;
        char *file;
        } cass;
    struct EMUTEO_MEMO {
        char *file;
        char *label;
        } memo;
    struct EMUTEO_LPRT {
        int  number;
        int  nlq;
        int  dip;
        int  raw_output;
        int  txt_output;
        int  gfx_output;
        char *folder;
        } lprt;
};

extern struct EMUTEO teo;
extern int frame;

/* fonctions importables requises */
extern void (*to8_SetColor)(int index, int r, int g, int b);
extern void (*to8_DrawGPL)(int mode, int addr, int pt, int col);
extern void (*to8_PutSoundByte)(unsigned long long int time, unsigned char value);
extern void (*to8_SetPointer)(int pointer);

/* fonctions importables optionnelles */
extern void (*to8_SetBorderColor)(int mode, int color);
extern void (*to8_DrawBorderLine)(int col, int line);
extern void (*to8_SetKeyboardLed)(int state);
extern void (*to8_SetDiskLed)(int state);
extern int  (*to8_DirectReadSector)(int drive, int track, int sector, int nsects, unsigned char data[]);
extern int  (*to8_DirectWriteSector)(int drive, int track, int sector, int nsects, const unsigned char data[]);
extern int  (*to8_DirectFormatTrack)(int drive, int track, const unsigned char data[]);

/* variables exportables */
extern char *to8_error_msg;
extern int  to8_new_video_params;

/* fonctions exportables */
extern int   to8_Init(int num_joy);
extern void  to8_Exit(void);
extern void  to8_Reset(void);
extern void  to8_ColdReset(void);
extern void  to8_InputReset(int mask, int value);
extern int   to8_DoFrame(int debug);

#endif
