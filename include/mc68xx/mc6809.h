/*
 *  Module d'�mulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou, 2011 Gilles F�tis
 *                2012 Fran�ois Mouret, 2012 Samuel Devulder.
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
 *  Module     : mc6809.h
 *  Version    : 2.7
 *  Cr�� par   : Sylvain Huet 1996
 *  Modifi� par: Eric Botcazou 30/11/2000
 *               Gille F�tis 27/07/2011
 *               Fran�ois Mouret 07/02/2012 13/04/2014
 *
 *  Emulateur du microprocesseur Motorola MC6809E.
 *
 *  version 1.0: �mulation fonctionnelle
 *  version 2.0: horloge interne, interface
 *  version 2.1: interruption du timer
 *  version 2.2: nouvelles valeurs de retour des fonctions d'�x�cution
 *  version 2.3: encapsulation compl�te du module
 *  version 2.4: ajout d'un masque d'�criture des registres
 *  version 2.5: ajout d'une fonction trace (mode DEBUG)
 *  version 2.6: nouvelles commandes externes (RESET, NMI, FIRQ)
 *               correction mineure du mode index� 5-bit
 *               Fetch devient FetchInstr et utilise des unsigned char
 *               suppression d'un inline inutile
 *  version 2.7: nouvelle interface de manipulation de l'�tat du MC6809E
 *          2.7.1: pr�-incr�ment de cpu_clock pour puls et pulu.
 *          2.7.2: am�nagement pour le debugger.
 *  version 2.8: fusion des tables *code[], taille[], adr[],
 *               cpu_cycles[].
 *               ajout des instructions non standard
 *               gestion des successions de codes Page2 ($10) et Page3
 *               ($11)
 */


#ifndef MC6809_H
#define MC6809_H

#ifdef DEBUG
   #ifndef SCAN_DEPEND
      #include <stdio.h>
   #endif

   extern FILE *mc6809_ftrace;
#endif

typedef unsigned long long int mc6809_clock_t;  /* entier 64-bit */

#define MC6809_TIMER_DISABLED     (mc6809_clock_t) -1

#define MC6809_REGS_CC_FLAG        (1<<0)
#define MC6809_REGS_DP_FLAG        (1<<1)
#define MC6809_REGS_AR_FLAG        (1<<2)
#define MC6809_REGS_BR_FLAG        (1<<3)
#define MC6809_REGS_XR_FLAG        (1<<4)
#define MC6809_REGS_YR_FLAG        (1<<5)
#define MC6809_REGS_UR_FLAG        (1<<6)
#define MC6809_REGS_SR_FLAG        (1<<7) 
#define MC6809_REGS_PC_FLAG        (1<<8)   
#define MC6809_REGS_CPUCLOCK_FLAG  (1<<9)
#define MC6809_REGS_CPUTIMER_FLAG  (1<<10)
#define MC6809_REGS_MAX_FLAG       11

#define MC6809_REGS_ALL_FLAG    MC6809_REGS_CC_FLAG | \
                                MC6809_REGS_DP_FLAG | \
                                MC6809_REGS_AR_FLAG | \
                                MC6809_REGS_BR_FLAG | \
                                MC6809_REGS_XR_FLAG | \
                                MC6809_REGS_YR_FLAG | \
                                MC6809_REGS_UR_FLAG | \
                                MC6809_REGS_SR_FLAG | \
                                MC6809_REGS_PC_FLAG | \
                                MC6809_REGS_CPUCLOCK_FLAG


struct CODES_6809_SPEC {
    char name[6];
    void (*prog)(void);
    int  taille;
    int  adr;
    int  cpu_cycles;
};

struct MC6809_REGS
{
    int cc;
    int dp;
    int ar;
    int br;
    int xr;
    int yr;
    int ur;
    int sr;
    int pc;
    mc6809_clock_t cpu_clock;
    mc6809_clock_t cpu_timer;
};


struct MC6809_INTERFACE
{
    void (*FetchInstr)(int, unsigned char []);
    int  (*LoadByte)(int);
    int  (*LoadWord)(int);
    void (*StoreByte)(int, int);
    void (*StoreWord)(int, int);
    int  (*TrapCallback)(struct MC6809_REGS *);
};
extern struct MC6809_INTERFACE mc6809_interface;


extern void mc6809_Init();
extern void mc6809_GetRegs(struct MC6809_REGS *);
extern void mc6809_SetRegs(const struct MC6809_REGS *, int);
extern void mc6809_SetTimer(mc6809_clock_t, void (*)(void *), void *);
extern void mc6809_Reset(void);
extern void mc6809_FlushExec(void);
extern int  mc6809_StepExec(void);
extern int  mc6809_TimeExec(mc6809_clock_t);
extern mc6809_clock_t mc6809_clock(void);
extern int  mc6809_irq;

#endif
