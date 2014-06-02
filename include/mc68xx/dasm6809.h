/*
 *  Module d'émulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou.
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
 *  Module     : dasm6809.h
 *  Version    : 1.4
 *  Créé par   : Sylvain Huet 1996
 *  Modifié par: Eric Botcazou 23/11/2000
 *
 *  Désassembleur du Motorola MC6809E
 *
 *  version 1.0: désassembleur fonctionnel
 *  version 1.1: bugfixes, interface
 *  version 1.2: correction d'un bug d'écriture de 0x10 et 0x11
 *  version 1.3: correction d'un bug des offsets 5-bit négatifs
 *               inclusion des déclarations des tables externes
 *               nouvel identifiant du module: dasm
 *  version 1.4: le buffer d'entrée est en unsigned char
 *               correction d'un bug de TFR et EXG
 *               corrections mineures (MacOS port)
 *  version 1.5: élimination des tabulations dans le texte
 *               clarification du code
 *               structure en entrée, taille en sortie
 *               calcul du nombre de cycles
 *               simplification du code pour PSHS, PSHU, PULS, PULU
 *               simplification du code pour indexé 15 bits
 *               optimisation du code pour indexé
 *               ajustement de l'offset pour indexé avec PCR
 */


#ifndef DASM6809_H
#define DASM6809_H

#define MC6809_DASM_FETCH_SIZE       5

#define MC6809_DASM_BUFFER_SIZE     64

#define MC6809_DASM_ASM_MODE         1
#define MC6809_DASM_BINASM_MODE      2

struct MC6809_DASM {
    unsigned char fetch[MC6809_DASM_FETCH_SIZE]; /* (in)   fetch buffer */
    int   addr;             /* (in)   virtual start address */
    int   mode;             /* (in)   disassembling mode */
    char  str[MC6809_DASM_BUFFER_SIZE];   /* (out)  output string */
    int   cycle1;           /* (out)  Number of primary cycles */
    int   cycle2;           /* (out)  Number of secondary cycles (or -1) */
    int   cycle3;           /* (out)  Number of alternative cycles (or -1) */
};

extern int dasm6809_size[];
extern int dasm6809_addr[];
extern struct MC6809_DASM mc6809_dasm;
extern int  dasm6809_Disassemble (struct MC6809_DASM *mc6809_dasm);

#endif
