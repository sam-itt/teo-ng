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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret,
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
 *  Version    : 1.8.3
 *  Créé par   : Gilles Fétis
 *  Modifié par: Eric Botcazou 26/10/2003
 *               François Mouret 30/01/2010 07/08/2011 05/03/2012
 *                               01/11/2012 20/09/2013 13/04/2014
 *               Gilles Fétis 27/07/2011
 *               Samuel Devulder 05/02/2012
 *
 *  Module de pilotage de l'émulateur.
 */


#ifndef TEO_H
#define TEO_H

#ifndef MAX_PATH
#   define MAX_PATH 300
#endif

#ifdef UNIX_TOOL
#   define FOLDER_SLASH "/"
#else
#   define FOLDER_SLASH "\\"
#endif

/* paramètres et symboles de l'émulation */
#define APPLICATION_DIR "teo"
#define TEO_YEAR_STRING "2015"
#define TEO_VERSION_STR "1.8.3"
#define PROG_NAME   "teo"
#define PROG_CLASS  "EmuTO"

#define ALLEGRO_CONFIG_FILE  "allegro.cfg"

#define TEO_FRAME_FREQ         50    /* Hz: fréquence de trame vidéo */
#define TEO_CYCLES_PER_FRAME   19968 /* durée exacte en nb de cycles CPU d'une frame */
#define TEO_CPU_FREQ           (TEO_FRAME_FREQ*TEO_CYCLES_PER_FRAME)  /* Hz */
#define TEO_NCOLORS            16    /* nombre de couleurs de la palette */ 

#define TEO_WINDOW_W           320   /* largeur de la fenêtre de travail */     
#define TEO_BORDER_W           16    /* largeur de la bordure de l'écran */
#define TEO_SCREEN_W           (TEO_WINDOW_W + TEO_BORDER_W*2)  /* largeur de l'écran */
#define TEO_WINDOW_H           200   /* hauteur de la fenêtre de travail */
#define TEO_BORDER_H           16    /* hauteur de la bordure de l'écran */
#define TEO_SCREEN_H           (TEO_WINDOW_H + TEO_BORDER_H*2)  /* hauteur de l'écran */

#define TEO_GPL_SIZE           8     /* taille d'un Groupe Point Ligne */
#define TEO_WINDOW_GW          (TEO_WINDOW_W/TEO_GPL_SIZE)  /* largeur de la fenêtre de travail en GPL */

#define TEO_CHAR_SIZE          8     /* taille d'un caractère */
#define TEO_WINDOW_CW          (TEO_WINDOW_W/TEO_CHAR_SIZE)  /* largeur de la fenêtre de travail en CHAR */
#define TEO_BORDER_CW          (TEO_BORDER_W/TEO_CHAR_SIZE)  /* largeur de la bordure de l'écran en CHAR */
#define TEO_SCREEN_CW          (TEO_SCREEN_W/TEO_CHAR_SIZE)  /* largeur de l'écran en CHAR */
#define TEO_WINDOW_CH          (TEO_WINDOW_H/TEO_CHAR_SIZE)  /* hauteur de la fenêtre de travail en CHAR */
#define TEO_BORDER_CH          (TEO_BORDER_H/TEO_CHAR_SIZE)  /* hauteur de la bordure de l'écran en CHAR */
#define TEO_SCREEN_CH          (TEO_SCREEN_H/TEO_CHAR_SIZE)  /* hauteur de l'écran en CHAR */

#define TEO_LEFT_BORDER        0x40  /* bitmask */
#define TEO_RIGHT_BORDER       0x80  /* bitmask */

#define TEO_COL40              0     /* mode 40 colonnes 16 couleurs */
#define TEO_BITMAP4            0x21  /* mode bitmap 4 couleurs       */
#define TEO_PAGE1              0x24  /* mode commutation page 1      */
#define TEO_PAGE2              0x25  /* mode commutation page 2      */
#define TEO_STACK2             0x26  /* mode superposition 2 pages   */
#define TEO_COL80              0x2A  /* mode 80 colonnes 2 couleurs  */
#define TEO_STACK4             0x3F  /* mode superposition 4 pages   */
#define TEO_BITMAP4b           0x41  /* mode bitmap 4 non documenté  */
#define TEO_BITMAP16           0x7B  /* mode bitmap 16 couleurs      */
#define TEO_PALETTE            0xFF  /* mode écran de la palette     */

#define TEO_PALETTE_ADDR       0x1A18    /* adresse de transition des palettes */
#define TEO_PALETTE_COL1       0x0000FF  /* triplet RRGGBB couleur 1 */    
#define TEO_PALETTE_COL2       0xD7FBEF  /* triplet RRGGBB couleur 2 */

#define TEO_NJOYSTICKS         2
#define TEO_NBUTTONS           2
#define TEO_JOYSTICK_CENTER    0
#define TEO_JOYSTICK_LEFT      (1<<0)
#define TEO_JOYSTICK_RIGHT     (1<<1)
#define TEO_JOYSTICK_UP        (1<<2)
#define TEO_JOYSTICK_DOWN      (1<<3)
#define TEO_JOYSTICK_FIRE_ON   1
#define TEO_JOYSTICK_FIRE_OFF  0

#define TEO_TRAP_CODE          0xCD   /* code pour le trap */

enum teo_command {
    TEO_COMMAND_NONE = 1,
    TEO_COMMAND_PANEL,
    TEO_COMMAND_SCREENSHOT,
    TEO_COMMAND_DEBUGGER,
    TEO_COMMAND_BREAKPOINT,
    TEO_COMMAND_RESET,
    TEO_COMMAND_COLD_RESET,
    TEO_COMMAND_FULL_RESET,
    TEO_COMMAND_QUIT
};

enum {
    TEO_STATUS_MOUSE = 1,
    TEO_STATUS_LIGHTPEN
};

extern int is_fr;

#define NBDRIVE        4
#define NBBREAKPOINT   10
#define MAX_BREAKPOINTS 16

struct EMUTEO {
    int sound_enabled;
    volatile enum teo_command command;
    char *default_folder;
    struct EMUTEO_SETTINGS {
        int  exact_speed;
        int  sound_volume;
        int  sound_enabled;
        int  interlaced_video;
        int  bank_range;
        } setting;
    struct EMUTEO_DISK {
        int  direct_access_allowed;
        int  side;
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
    struct EMUTEO_DEBUG {
        int  window_maximize;
        int  window_x;
        int  window_y;
        int  window_width;
        int  window_height;
        int  breakpoint[MAX_BREAKPOINTS];
        int  extra_first_line;
        int  memory_address;
        int  ram_number;
        int  mon_number;
        int  video_number;
        int  cart_number;
        } debug;
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
extern void (*teo_SetColor)(int index, int r, int g, int b);
extern void (*teo_DrawGPL)(int mode, int addr, int pt, int col);
extern void (*teo_PutSoundByte)(unsigned long long int time, unsigned char value);
extern void (*teo_SetPointer)(int pointer);

/* fonctions importables optionnelles */
extern int  (*teo_DebugBreakPoint)(int pc);
extern void (*teo_SetBorderColor)(int mode, int color);
extern void (*teo_DrawBorderLine)(int col, int line);
extern void (*teo_SetKeyboardLed)(int state);
extern void (*teo_SetDiskLed)(int state);
extern int  (*teo_DirectReadSector)(int drive, int track, int sector, int nsects, unsigned char data[]);
extern int  (*teo_DirectWriteSector)(int drive, int track, int sector, int nsects, const unsigned char data[]);
extern int  (*teo_DirectFormatTrack)(int drive, int track, const unsigned char data[]);
extern int  (*teo_DirectIsDiskWritable)(int drive);

/* variables exportables */
extern char *teo_error_msg;
extern int  teo_new_video_params;

/* fonctions exportables */
extern int   teo_Init(int num_joy);
extern void  teo_Exit(void);
extern void  teo_Reset(void);
extern void  teo_ColdReset(void);
extern void  teo_FullReset(void);
extern void  teo_InputReset(int mask, int value);
extern int   teo_DoFrame(void);
extern void  teo_FlushFrame(void);

#endif

