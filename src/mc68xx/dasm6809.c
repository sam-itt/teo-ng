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
 *  Module     : mc68xx/dasm6809.c
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
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "mc68xx/dasm6809.h"
#include "mc68xx/mc6809.h"


/* tables des instructions */
/*
extern int taille[];
extern int adr[];
extern int cpu_cycles[];
*/

int taille[]=
{2,2,1,2,2,1,2,2,2,2,2,1,2,2,2,2
,0,0,1,1,1,1,3,3,1,1,2,1,2,1,2,2
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,2,2,2,2,2,2,2,2,1,1,1,1,2,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,2,1,1,2,2,1,2,2,2,2,2,1,2,2,2,2
,3,1,1,3,3,1,3,3,3,3,3,1,3,3,3,3
,2,2,2,3,2,2,2,1,2,2,2,2,3,2,3,1
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
,2,2,2,3,2,2,2,1,2,2,2,2,3,1,3,1
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3

,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,4,1,1,1,1,1,1,1,1,4,1,4,1
,1,1,1,3,1,1,1,1,1,1,1,1,3,1,3,3
,1,1,1,3,1,1,1,1,1,1,1,1,3,1,3,3
,1,1,1,4,1,1,1,1,1,1,1,1,4,1,4,4
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4

,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,4,1,1,1,1,1,1,1,1,4,1,1,1
,1,1,1,3,1,1,1,1,1,1,1,1,3,1,1,1
,1,1,1,3,1,1,1,1,1,1,1,1,3,1,1,1
,1,1,1,4,1,1,1,1,1,1,1,1,4,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};


int adr[]=
{0,1,1,0,0,1,0,0,0,0,0,1,0,0,0,0
,6,6,1,1,1,1,2,2,1,1,3,1,3,1,1,1
,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,4,4,4,4,3,3,3,3,1,1,1,1,3,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,1
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
,3,3,3,3,3,3,3,1,3,3,3,3,3,1,3,1
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5

,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,3,1,1,1,1,1,1,1,1,3,1,3,1
,1,1,1,0,1,1,1,1,1,1,1,1,0,1,0,0
,1,1,1,4,1,1,1,1,1,1,1,1,4,1,4,4
,1,1,1,5,1,1,1,1,1,1,1,1,5,1,5,5
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,5

,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,3,1,1,1,1,1,1,1,1,3,1,1,1
,1,1,1,0,1,1,1,1,1,1,1,1,0,1,1,1
,1,1,1,4,1,1,1,1,1,1,1,1,4,1,1,1
,1,1,1,5,1,1,1,1,1,1,1,1,5,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};


int cpu_cycles[]=
{6,2,0,6,6,0,6,6,6,6,6,0,6,6,3,6
,0,0,2,4,0,0,5,9,0,2,3,0,3,2,8,6
,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
,4,4,4,4,5,5,5,5,0,5,3,3,20,11,0,7
,2,0,0,2,2,0,2,2,2,2,2,0,2,2,0,2
,2,0,0,2,2,0,2,2,2,2,2,0,2,2,0,2
,6,0,0,6,6,0,6,6,6,6,6,0,6,6,3,6
,7,0,0,7,7,0,7,7,7,7,7,0,7,7,4,7
,2,2,2,4,2,2,2,0,2,2,2,2,4,7,3,0
,4,4,4,6,4,4,4,4,4,4,4,4,6,7,5,5
,4,4,4,6,4,4,4,4,4,4,4,4,6,7,5,5
,5,5,5,7,5,5,5,5,5,5,5,5,7,8,6,6
,2,2,2,4,2,2,2,0,2,2,2,2,3,0,3,0
,4,4,4,6,4,4,4,4,4,4,4,4,5,5,5,5
,4,4,4,6,4,4,4,4,4,4,4,4,5,5,5,5
,5,5,5,7,5,5,5,5,5,5,5,5,6,6,6,6

,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,5,0,0,0,0,0,0,0,0,5,0,4,0
,0,0,0,7,0,0,0,0,0,0,0,0,7,0,6,6
,0,0,0,7,0,0,0,0,0,0,0,0,7,0,6,6
,0,0,0,8,0,0,0,0,0,0,0,0,8,0,7,7
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,6
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7

,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,5,0,0,0,0,0,0,0,0,5,0,0,0
,0,0,0,7,0,0,0,0,0,0,0,0,7,0,0,0
,0,0,0,7,0,0,0,0,0,0,0,0,7,0,0,0
,0,0,0,8,0,0,0,0,0,0,0,0,8,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


static const char mne[]="\
NEG ??  ??  COM LSR ??  ROR ASR ASL ROL DEC ??  INC TST JMP CLR \
--  --  NOP SYNC??  ??  LBRALBSR??  DAA ORCC??  ANDCSEX EXG TFR \
BRA BRN BHI BLS BCC BLO BNE BEQ BVC BVS BPL BMI BGE BLT BGT BLE \
LEAXLEAYLEASLEAUPSHSPULSPSHUPULU??  RTS ABX RTI CWAIMUL ??  SWI \
NEGA??  ??  COMALSRA??  RORAASRAASLAROLADECA??  INCATSTA??  CLRA\
NEGB??  ??  COMBLSRB??  RORBASRBLSLBROLBDECB??  INCBTSTB??  CLRB\
NEG ??  ??  COM LSR ??  ROR ASR ASL ROL DEC ??  INC TST JMP CLR \
NEG ??  ??  COM LSR ??  ROR ASR ASL ROL DEC ??  INC TST JMP CLR \
SUBACMPASBCASUBDANDABITALDA ??  EORAADCAORA ADDACMPXBSR LDX ??  \
SUBACMPASBCASUBDANDABITALDA STA EORAADCAORA ADDACMPXJSR LDX STX \
SUBACMPASBCASUBDANDABITALDA STA EORAADCAORA ADDACMPXJSR LDX STX \
SUBACMPASBCASUBDANDABITALDA STA EORAADCAORA ADDACMPXJSR LDX STX \
SUBBCMPBSBCBADDDANDBBITBLDB ??  EORBADCBORB ADDBLDD ??  LDU ??  \
SUBBCMPBSBCBADDDANDBBITBLDB STB EORBADCBORB ADDBLDD STD LDU STU \
SUBBCMPBSBCBADDDANDBBITBLDB STB EORBADCBORB ADDBLDD STD LDU STU \
SUBBCMPBSBCBADDDANDBBITBLDB STB EORBADCBORB ADDBLDD STD LDU STU \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  LBRNLBHILBLSLBCCLBLOLBNELBEQLBVCLBVSLBPLLBMILBGELBLTLBGTLBLE\
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  SWI2\
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  CMPD??  ??  ??  ??  ??  ??  ??  ??  CMPY??  LDY ??  \
??  ??  ??  CMPD??  ??  ??  ??  ??  ??  ??  ??  CMPY??  LDY STY \
??  ??  ??  CMPD??  ??  ??  ??  ??  ??  ??  ??  CMPY??  LDY STY \
??  ??  ??  CMPD??  ??  ??  ??  ??  ??  ??  ??  CMPY??  LDY STY \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  LDS ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  LDS STS \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  LDS STS \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  LDS STS \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  SWI3\
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  CMPU??  ??  ??  ??  ??  ??  ??  ??  CMPS??  ??  ??  \
??  ??  ??  CMPU??  ??  ??  ??  ??  ??  ??  ??  CMPS??  ??  ??  \
??  ??  ??  CMPU??  ??  ??  ??  ??  ??  ??  ??  CMPS??  ??  ??  \
??  ??  ??  CMPU??  ??  ??  ??  ??  ??  ??  ??  CMPS??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  ??  \
";

static const char reg[16][3]={"D","X","Y","U","S","PC","?","?","A","B","CC","DP","?","?","?","?"};
static const int  stack_reg[2][8]={ {5,3,2,1,11,9,8,10},
                                    {5,4,2,1,11,9,8,10} };



/* MC6809_Dasm:
 *  Désassemble une instruction 6809 et retourne sa taille.
 *   out_str: chaîne de caractères de sortie
 *   int_str: pointeur sur le buffer d'instructions
 *   addr   : l'adresse 6809 virtuelle où est stocké le code machine
 *   mode   : mode de désassemblage
 */
int MC6809_Dasm(char *out_str, const unsigned char *in_str, int addr, int mode)
{
    register int i;
    int size, offset, opcode=in_str[0];
    char asm_buffer[MC6809_DASM_BUFFER_SIZE/2], *p_buffer=asm_buffer;

    /* codes 0x10 et 0x11 d'instruction longue */
    if (opcode==0x10)
        opcode=256+(in_str++[1]);
    else if (opcode==0x11)
        opcode=512+(in_str++[1]);

    /* affichage du mnémonique */
    strncpy(p_buffer, mne+opcode*4, 4);
    p_buffer+=4;
    if (opcode==0x1c)
        p_buffer+=sprintf(p_buffer, "C");
    else
        p_buffer+=sprintf(p_buffer, " ");
    p_buffer+=sprintf(p_buffer, "  ");
    size=taille[opcode];

    switch (adr[opcode])
    {
        case 0:  /* direct */
            p_buffer+=sprintf(p_buffer,"<%02X", in_str[1]);
            break;

        case 1:  /* inhérent */
            if ((opcode==0x1E) || (opcode==0x1F)) /* TFR et EXG */
                p_buffer+=sprintf(p_buffer,"%s,%s",reg[(in_str[1]>>4)&0xF],reg[in_str[1]&0xF]);
            break;

	case 2: /* relatif */
            p_buffer+=sprintf(p_buffer,"$%04X", addr + size +
                       (size==2 ? (signed char) in_str[1] : (signed char) in_str[1]*256+in_str[2] ));
            break;

	case 3: /* immédiat */
            if ((opcode&0xFC)==0x34)  /* piles S et U */
            {
                int first=1;

                if (opcode&1)
                {        /* on dépile */
                    for (i=7; i>=0; i--)
                        if ((0x80>>i)&in_str[1])
                        {
                            if (first)
                                first=0;
                            else
                                p_buffer+=sprintf(p_buffer,",");

                            p_buffer+=sprintf(p_buffer,"%s",reg[stack_reg[(opcode&2)>>1][i]]);
                        }
                }
                else
                {        /* on empile */
                    for (i=0; i<8; i++)
                        if ((0x80>>i)&in_str[1])
                        {
                            if (first)
                                first=0;
                            else
                                p_buffer+=sprintf(p_buffer,",");

                            p_buffer+=sprintf(p_buffer,"%s",reg[stack_reg[(opcode&2)>>1][i]]);
                        }
                }
            }
            else if (size==2)
                p_buffer+=sprintf(p_buffer,"#$%02X", in_str[1]);
            else
                p_buffer+=sprintf(p_buffer,"#$%04X", in_str[1]*256+in_str[2]);
            break;

	case 4: /* indexé */
            if (in_str[1]&0x80)
            {
                if (in_str[1]&0x10)
                    p_buffer+=sprintf(p_buffer,"[");

                switch (in_str[1]&0xF)
                {
                    case 0x0:
                        p_buffer+=sprintf(p_buffer,",%s+",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x1:
			p_buffer+=sprintf(p_buffer,",%s++",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x2:
			p_buffer+=sprintf(p_buffer,",-%s",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x3:
			p_buffer+=sprintf(p_buffer,",--%s",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x4:
			p_buffer+=sprintf(p_buffer,",%s",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x5:
			p_buffer+=sprintf(p_buffer,"B,%s",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x6:
			p_buffer+=sprintf(p_buffer,"A,%s",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0x8:
                        offset=(signed char) in_str[2];
                        if (offset<0)
                        {
                            p_buffer+=sprintf(p_buffer,"-");
                            offset=-offset;
                        }
			p_buffer+=sprintf(p_buffer,"$%02x,%s",offset,reg[((in_str[1]&0x60)>>5)+1]);
			size++;
			break;

                    case 0x9:
                        offset=(signed char) in_str[2]*256+in_str[3];
                        if (offset<0)
                        {
                            p_buffer+=sprintf(p_buffer,"-");
                            offset=-offset;
                        }
			p_buffer+=sprintf(p_buffer,"$%04x,%s",offset,reg[((in_str[1]&0x60)>>5)+1]);
			size+=2;
			break;

                    case 0xB:
			p_buffer+=sprintf(p_buffer,"D,%s",reg[((in_str[1]&0x60)>>5)+1]);
			break;

                    case 0xC:
                        offset=(signed char) in_str[2];
                        if (offset<0)
                        {
                            p_buffer+=sprintf(p_buffer,"-");
                            offset=-offset;
                        }
			p_buffer+=sprintf(p_buffer,"$%02x,PC",offset);
			size++;
			break;

                    case 0xD:
                        offset=(signed char) in_str[2]*256+in_str[3];
                        if (offset<0)
                        {
                            p_buffer+=sprintf(p_buffer,"-");
                            offset=-offset;
                        }
			p_buffer+=sprintf(p_buffer,"$%04x,PC",offset);
			size+=2;
			break;

                    case 0xF:
			p_buffer+=sprintf(p_buffer,"$%04X", in_str[2]*256+in_str[3]);
			size+=2;
			break;

                }  /* end of switch (in_str[1]&0xF) */

		if (in_str[1]&0x10)
                    p_buffer+=sprintf(p_buffer,"]");
            }
            else
            {
		if (in_str[1]&0x10)
                {
                    p_buffer+=sprintf(p_buffer,"-");
                    offset=((in_str[1]&0xF)^0xF)+1;
                }
                else
                    offset=in_str[1]&0xF;

		p_buffer+=sprintf(p_buffer,"%1x,%s",offset,reg[((in_str[1]&0x60)>>5)+1]);
            }
            break;

	case 5: /* étendu */
            p_buffer+=sprintf(p_buffer,">$%04X", in_str[1]*256+in_str[2]);
            break;

        default: /* erreur */
            p_buffer+=sprintf(p_buffer,"???");
            break;
    }

    out_str+=sprintf(out_str,"%04X  ",addr);

    if (mode == MC6809_DASM_BINASM_MODE)
    {
        if (opcode > 255)  /* instruction longue */
            in_str--;

        for (i=0; i<size; i++)
            out_str+=sprintf(out_str,"%02X ", in_str[i]);

        for (i=size; i<MC6809_FETCH_BUFFER_SIZE; i++)
            out_str+=sprintf(out_str,"   ");

        out_str+=sprintf(out_str," ");
    }

    strcat(out_str, asm_buffer);
    return size;
}
