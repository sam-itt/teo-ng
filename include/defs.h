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
 *  Copyright (C) 1997-2014 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : intern/defs.h
 *  Version    : 1.8.3
 *  Créé par   : Eric Botcazou octobre 1999
 *  Modifié par: Eric Botcazou 19/11/2006
 *               François Mouret 28/04/2012 20/09/2013 10/05/2014
 *
 *  Définition des structures et constantes internes.
 */


#ifndef DEFS_H
#define DEFS_H

#include "mc68xx/mc6809.h"


#ifdef DJGPP

#ifndef SCAN_DEPEND
   #include <dpmi.h>
#endif

/* from Allegro */
#undef END_OF_FUNCTION
#define END_OF_FUNCTION(x)          void x##_end(void) { }
#undef END_OF_STATIC_FUNCTION
#define END_OF_STATIC_FUNCTION(x)   static void x##_end(void) { }
#undef LOCK_DATA
#define LOCK_DATA(d, s)             _go32_dpmi_lock_data(d, s)
#undef LOCK_CODE
#define LOCK_CODE(c, s)             _go32_dpmi_lock_code(c, s)
#undef LOCK_VARIABLE
#define LOCK_VARIABLE(x)            LOCK_DATA((void *)&x, sizeof(x))
#undef LOCK_FUNCTION
#define LOCK_FUNCTION(x)            LOCK_CODE(x, (long)x##_end - (long)x)

#else

#define END_OF_FUNCTION(x)
#define END_OF_STATIC_FUNCTION(x)
#define LOCK_DATA(d, s)
#define LOCK_CODE(c, s)
#define LOCK_VARIABLE(x)
#define LOCK_FUNCTION(x)

#endif

#ifndef TRUE
#   define TRUE  1
#   define FALSE 0
#else
#   if (TRUE == -1)
#      undef TRUE
#      undef FALSE
#      define TRUE  1
#      define FALSE 0
#   endif
#endif

#ifndef MAX
#   define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#   define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define LEFT_SHADOW_CYCLES    10
#define LEFT_BORDER_CYCLES     2
#define WINDOW_LINE_CYCLES    40
#define RIGHT_BORDER_CYCLES    2
#define RIGHT_SHADOW_CYCLES   10
#define FULL_LINE_CYCLES      (LEFT_SHADOW_CYCLES	\
			       + LEFT_BORDER_CYCLES	\
			       + WINDOW_LINE_CYCLES	\
			       + RIGHT_BORDER_CYCLES	\
			       + RIGHT_SHADOW_CYCLES)
#define LINE_GRANULARITY       2

#define TOP_SHADOW_LINES      40
#define TOP_BORDER_LINES      16
#define WINDOW_LINES         200
#define BOTTOM_BORDER_LINES   16
#define BOTTOM_SHADOW_LINES   40
#define TOP_BORDER             1
#define BOTTOM_BORDER          2   


struct GATE_ARRAY {
    int p_data;
    int p_addr;
    int lgamod;
    int system1;
    int system2;
    int commut;
    int ram_data;
    int cart;
    int lp1;
    int lp2;
    int lp3;
    int lp4;
};

struct EF9369 {
    struct {
        int gr;
        int b;
    } color[16];
    void (*update)(int);
};

typedef unsigned char uint8;  /* unité de mémoire */

struct MEMORY {
    struct {
        int nbank;
        int size;
        uint8 *bank[4];
    } cart;
    struct {
        int nbank;
        int size;
        uint8 *bank[4];
        char filename[4][64];
    } rom;
    struct {
        int nbank;
        int size;
        uint8 *bank[32];
    } ram;
    struct {
        int nbank;
        int size;
        uint8 *bank[2];
        char filename[2][64];
    } mon;
};
        
struct MEMORY_PAGER {
    struct {
        int page;      /* page de la cartouche                */
        int rom_page;  /* page de ROM mappée sur la cartouche */
        int ram_page;  /* page de RAM mappée sur la cartouche */
        void (*update)(void);
    } cart;
    struct {
        int page;       /* page de la mémoire écran           */
        int vram_page;  /* page de mémoire vidéo affichée     */
        void (*update)(void);
    } screen;
    struct {
        int page;
        void (*update)(void);
    } system;
    struct {
        int page;      /* page de RAM de l'espace données     */
        int reg_page;  /* page de RAM en mode registre        */
        int pia_page;  /* page de RAM en mode PIA             */
        void (*update)(void);
    } data;
    struct {
        int page;      /* page du moniteur                    */
        void (*update)(void);
    } mon;
    uint8 *segment[16];  /* 16 segments de 4ko (64ko) adressables */
};

struct MOTHERBOARD {
    mc6809_clock_t exact_clock;
    int direct_screen_mode;
};

#endif

