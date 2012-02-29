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
 *  Module     : mc68xx/mc6809.c
 *  Version    : 2.7.1
 *  Créé par   : Sylvain Huet 1996
 *  Modifié par: Eric Botcazou 30/11/2000
 *               François Mouret 27/09/2006 26/01/2010
 *               Gilles Fétis 27/07/2011
 *
 *  Emulateur du microprocesseur Motorola MC6809E.
 *
 *  version 1.0: émulation fonctionnelle
 *  version 2.0: horloge interne, interface
 *  version 2.1: interruption du timer
 *  version 2.2: nouvelles valeurs de retour des fonctions d'éxécution
 *  version 2.3: encapsulation complète du module
 *  version 2.4: ajout d'un masque d'écriture des registres
 *  version 2.5: ajout d'une fonction trace (mode DEBUG)
 *  version 2.6: nouvelles commandes externes (RESET, NMI, FIRQ)
 *               correction mineure du mode indexé 5-bit
 *               Fetch devient FetchInstr et utilise des unsigned char
 *               suppression d'un inline inutile
 *  version 2.7: nouvelle interface de manipulation de l'état du MC6809E
 *          2.7.1: pré-incrément de cpu_clock pour puls et pulu.
 *          2.7.2: aménagement pour le debugger.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
#endif

#include "mc68xx/mc6809.h"

#ifdef OS_LINUX
extern int check_bkpt(int pc);
#endif

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

static void (*FetchInstr)(int, unsigned char []);
static int  (*LoadByte)(int);
static int  (*LoadWord)(int);
static void (*StoreByte)(int, int);
static void (*StoreWord)(int, int);

static int  (*TrapCallback)(struct MC6809_REGS *);
static void (*TimerCallback)(void *);
static void *timer_data;

static unsigned char fetch_buffer[MC6809_FETCH_BUFFER_SIZE];

/* le caractère 8-bit du MC6809 impose l'utilisation de char
   pour la manipulation des opcodes qui sont des octets signés */
static char *op;
static int ad;
static int *regist[4], *exreg[16];
static int illegal_instruction_flag;

/* variables d'état du MC6809 */
static mc6809_clock_t cpu_clock, cpu_timer;
static int pc,xr,yr,ur,sr,ar,br,dp;
static int res,m1,m2,sign,ovfl,h1,h2,ccrest;



/*************************************************/
/*** gestion du registre d'état (CC) du MC6809 ***/
/*************************************************/

static int getcc(void)
{
    return  ((((h1&15)+(h2&15))&16)<<1)
		|((sign&0x80)>>4)
		|((((res&0xff)==0)&1)<<2)
		|(( ((~(m1^m2))&(m1^ovfl)) &0x80)>>6)
		|((res&0x100)>>8)
		|ccrest;
}


static void setcc(int i)
{
    m1=m2=0;
    res=((i&1)<<8)|(4-(i&4));
    ovfl=(i&2)<<6;
    sign=(i&8)<<4;
    h1=h2=(i&32)>>2;
    ccrest=i&0xd0;
}



/***********************************/
/*** modes d'adressage du MC6809 ***/
/***********************************/

static int direc(void)
	{
	return (dp<<8)+((*op)&255);
	}

static int inher(void)
	{
	return -1;
	}

static int immedc(void)
	{
	return (pc-1)&0xffff;
	}

static int immedl(void)
	{
	return (pc-2)&0xffff;
	}

static int indxp(void)
	{
		int    *x=regist[((*op)&0x60)>>5];
		int    k;
	k=*x;
	*x=((*x)+1)&0xffff;
        cpu_clock+=2;
	return k;
	}

static int indxpp(void)
	{
		int    *x=regist[((*op)&0x60)>>5];
		int    k;
	k=*x;
	*x=((*x)+2)&0xffff;
        cpu_clock+=3;
	return k;
	}

static int indmx(void)
	{
		int    *x=regist[((*op)&0x60)>>5];
	*x=((*x)-1)&0xffff;
        cpu_clock+=2;
	return *x;
	}

static int indmmx(void)
	{
		int    *x=regist[((*op)&0x60)>>5];
	*x=((*x)-2)&0xffff;
        cpu_clock+=3;
	return *x;
	}

static int indx(void)
	{
	return *(regist[((*op)&0x60)>>5]);
	}

static int indax(void)
	{
		char    a=ar;
        cpu_clock++;
        return ((*(regist[((*op)&0x60)>>5]))+a)&0xffff;
	}

static int indbx(void)
	{
		char    b=br;
        cpu_clock++;
        return ((*(regist[((*op)&0x60)>>5]))+b)&0xffff;
	}

static int inder(void)
	{
	return 0;
	}

static int ind1x(void)
	{
		char    del=op[1];
        pc++;pc&=0xffff;
        cpu_clock++;
        return ((*(regist[((*op)&0x60)>>5]))+del)&0xffff;
	}

static int ind2x(void)
	{
		int     del=((op[1]&255)<<8)+(op[2]&255);
        pc+=2;pc&=0xffff;
        cpu_clock+=4;
        return ((*(regist[((*op)&0x60)>>5]))+del)&0xffff;
	}

static int inddx(void)
	{
		int     del=(ar<<8)+br;
        cpu_clock+=4;
        return ((*(regist[((*op)&0x60)>>5]))+del)&0xffff;
	}

static int ind1p(void)
	{
		char    del=op[1];
        pc++;pc&=0xffff;
        cpu_clock++;
        return (pc+del)&0xffff;
	}

static int ind2p(void)
	{
		int     del=((op[1]&255)<<8)+(op[2]&255);
        pc+=2;pc&=0xffff;
        cpu_clock+=5;
        return (pc+del)&0xffff;
	}

static int indad(void)
	{
        pc+=2;pc&=0xffff;
        cpu_clock+=2;
	return ((op[1]&255)<<8)+(op[2]&255);
	}

static int (*indmod[])(void)=
{indxp,indxpp,indmx,indmmx,indx,indbx,indax,inder
,ind1x,ind2x,inder,inddx,ind1p,ind2p,inder,indad
};


static int indir(void)
{
    int k;

    if ((*op)&0x80)
    {
        k=(*indmod[(*op)&0xf])();

        if ((*op)&0x10)
        {
            cpu_clock+=3;  /* pénalité de 3 cycles pour le mode indirect */
            return LoadWord(k);
        }
        else
            return k; /* non indirect */
    }

    /* 5-bit offset */
    cpu_clock++;

    if ((*op)&0x10)
    {
        if ((*op)&0xf)
            k = (*(regist[((*op)&0x60)>>5]))-((-*op)&0xf);
	else
	    k = (*(regist[((*op)&0x60)>>5]))-16;
    }
    else
        k = (*(regist[((*op)&0x60)>>5]))+((*op)&0xf);

    return k&0xffff;
}

static int etend(void)
	{
	return (((*op)&255)<<8)+(op[1]&255);
	}

static int (*adresc[])(void)=
{direc,inher,inher,immedc,indir,etend,inher};

static int (*adresl[])(void)=
{direc,inher,inher,immedl,indir,etend,inher};



/******************************/
/*** instructions du MC6809 ***/
/******************************/

static void what(void)
{
    illegal_instruction_flag = 1;
}

static void negm(void)             /* H?NxZxVxCx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	m1=val; m2=-val;                /* bit V */
	StoreByte(k,(val=-val)&255);
	ovfl=res=sign=val;
	}

static void comm(void)             /* NxZxV0C1 */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	m1=~m2;
	StoreByte(k,val=(~val)&255);
	sign=val;
	res=sign|0x100; /* bit C a 1 */
	}

static void lsrm(void)             /* N0ZxCx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	res=(val&1)<<8; /* bit C */
	StoreByte(k,val>>=1);
	sign=0;
	res|=val;
	}

static void rorm(void)             /* NxZxCx */
	{
		int    k;
		int     val,i;
	i=val=LoadByte(k=(*adresc[ad])());
	StoreByte(k,val=(val|(res&0x100))>>1);
	sign=val;
	res=((i&1)<<8)|sign;
	}

static void asrm(void)             /* H?NxZxCx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	res=(val&1)<<8;
	StoreByte(k,val=(val>>1)|(val&0x80));
	sign=val;
	res|=sign;
	}

static void aslm(void)             /* H?NxZxVxCx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	m1=m2=val;
	StoreByte(k,val<<=1);
	ovfl=sign=res=val;
	}

static void rolm(void)             /* NxZxVxCx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	m1=m2=val;
	StoreByte(k,val=(val<<1)|((res&0x100)>>8) );
	ovfl=sign=res=val;
	}

static void decm(void)             /* NxZxVx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	m1=val; m2=0x80;
	StoreByte(k,--val);
	ovfl=sign=val&255;
	res=(res&0x100)|sign;
	}

static void incm(void)             /* NxZxVx */
	{
		int    k;
		int     val;
	val=LoadByte(k=(*adresc[ad])());
	m1=val; m2=0;
	StoreByte(k,++val);
	ovfl=sign=val&255;
	res=(res&0x100)|sign;
	}

static void tstm(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=~m2;
	sign=val;
	res=(res&0x100)|sign;
	}

static void jmpm(void)
	{
	pc=(*adresl[ad])();
	}

static void jsrm(void)
	{
		int k;
	k=(*adresl[ad])();
	sr=(sr-2)&0xffff;
	StoreWord(sr,pc);
	pc=k;
	}

static void clrm(void)     /* N0Z1V0C0 */
	{
	StoreByte((*adresc[ad])(),0);
	m1=~m2;
	sign=res=0;
	}

static void nopm(void)
	{
	}

static void synm(void)
	{
        /* non supporté */
        }

static void lbra(void)
	{
	pc=(pc+((*op)<<8)+(op[1]&255))&0xffff;
	}

static void lbsr(void)
	{
	sr=(sr-2)&0xffff;
	StoreWord(sr,pc);
	pc=(pc+((*op)<<8)+(op[1]&255))&0xffff;
	}

static void daam(void)     /* NxZxV?Cx */
	{
		int     i=ar+(res&0x100);
	if (((ar&15)>9)||((h1&15)+(h2&15)>15)) i+=6;
  if (i>0x99) i+=0x60;
	res=sign=i;
	ar=i&255;
	}

static void orcc(void)
	{
	setcc(getcc()|(*op));
	}

static void andc(void)
	{
	setcc(getcc()&(*op));
	}

static void sexm(void)     /* NxZx */
	{
	if (br&0x80) ar=0xff;
		else ar=0;
	sign=br;
	res=(res&0x100)|sign;
	}


static void exgm(void)
	{
		int    k,l;
		int     o1,o2;
		int    *p,*q;

	o1=((*op)&0xf0)>>4;
	o2=(*op)&15;
	if ((p=exreg[o1])) k=*p;
		else    if (o1) k=getcc();
				else k=(ar<<8)+br;
	if ((q=exreg[o2]))
			{
			l=*q;
			*q=k;
			}
		else    if (o2) {
				l=getcc();
				setcc(k);
				}
				else
				{
				l=(ar<<8)+br;
				ar=(k>>8)&255;
				br=k&255;
				}
	if (p) *p=l;
		else    if (o1) setcc(l);
				else
				{
				ar=(l>>8)&255;
				br=l&255;
				}
	}

static void tfrm(void)
	{
		int    k;
		int     o1,o2;
		int    *p,*q;

	o1=((*op)&0xf0)>>4;
	o2=(*op)&15;
	if ((p=exreg[o1])) k=*p;
		else    if (o1) k=getcc();
				else k=(ar<<8)+br;
	if ((q=exreg[o2])) *q=k;
		else    if (o2) setcc(k);
				else
				{
				ar=(k>>8)&255;
				br=k&255;
				}
	}

static void bras(void)
	{
	pc+=op[0];
	}
static void brns(void)
	{
	}

static void bhis(void)     /* c|z=0 */
	{
	if ((!(res&0x100))&&(res&0xff)) pc+=op[0];
	}
static void blss(void)     /* c|z=1 */
	{
	if ((res&0x100)||(!(res&0xff))) pc+=op[0];
	}

static void bccs(void)     /* c=0 */
	{
	if (!(res&0x100)) pc+=op[0];
	}
static void blos(void)     /* c=1 */
	{
	if (res&0x100) pc+=op[0];
	}

static void bnes(void)     /* z=0 */
	{
	if (res&0xff) pc+=op[0];
	}
static void beqs(void)     /* z=1 */
	{
	if (!(res&0xff)) pc+=op[0];
	}

static void bvcs(void)     /* v=0 */
	{
	if ( ((m1^m2)&0x80)||(!((m1^ovfl)&0x80)) ) pc+=op[0];
	}
static void bvss(void)     /* v=1 */
	{
	if ( (!((m1^m2)&0x80))&&((m1^ovfl)&0x80) ) pc+=op[0];
	}

static void bpls(void)     /* n=0 */
	{
	if (!(sign&0x80)) pc+=op[0];
	}
static void bmis(void)     /* n=1 */
	{
	if (sign&0x80) pc+=op[0];
	}

static void bges(void)     /* n^v=0 */
	{
	if (!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) pc+=op[0];
	}
static void blts(void)     /* n^v=1 */
	{
	if ((sign^((~(m1^m2))&(m1^ovfl)))&0x80) pc+=op[0];
	}

static void bgts(void)     /* z|(n^v)=0 */
	{
	if ( (res&0xff)
	   &&(!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) ) pc+=op[0];
	}
static void bles(void)     /* z|(n^v)=1 */
	{
	if ( (!(res&0xff))
	   ||((sign^((~(m1^m2))&(m1^ovfl)))&0x80) ) pc+=op[0];
	}


static void leax(void)     /* Zx */
	{
	xr=(*adresc[ad])();
	res=(res&0x100)|((xr|(xr>>8))&255);
	}

static void leay(void)     /* Zx */
	{
	yr=(*adresc[ad])();
	res=(res&0x100)|((yr|(yr>>8))&255);
	}

static void leas(void)
	{
	sr=(*adresc[ad])();
	}

static void leau(void)
	{
	ur=(*adresc[ad])();
	}

static void pshsr(int i)
	{
	if (i&0x80)     {
			sr=(sr-2)&0xffff;
			StoreWord(sr,pc);
                        cpu_clock+=2;
			}
	if (i&0x40)     {
			sr=(sr-2)&0xffff;
			StoreWord(sr,ur);
                        cpu_clock+=2;
			}
	if (i&0x20)     {
			sr=(sr-2)&0xffff;
			StoreWord(sr,yr);
                        cpu_clock+=2;
			}
	if (i&0x10)     {
			sr=(sr-2)&0xffff;
			StoreWord(sr,xr);
                        cpu_clock+=2;
			}
	if (i&0x8)      {
			sr=(sr-1)&0xffff;
			StoreByte(sr,dp);
                        cpu_clock++;
			}
	if (i&0x4)      {
			sr=(sr-1)&0xffff;
			StoreByte(sr,br);
                        cpu_clock++;
			}
	if (i&0x2)      {
			sr=(sr-1)&0xffff;
			StoreByte(sr,ar);
                        cpu_clock++;
			}
	if (i&0x1)      {
			sr=(sr-1)&0xffff;
			StoreByte(sr,getcc());
                        cpu_clock++;
			}
	}

static void pshs(void)
	{
	pshsr(*op);
	}

static void pulsr(int i)
	{
	if (i&0x1)      {
                        cpu_clock++;
			setcc(LoadByte(sr));
			sr=(sr+1)&0xffff;
			}
	if (i&0x2)      {
                        cpu_clock++;
			ar=LoadByte(sr);
			sr=(sr+1)&0xffff;
			}
	if (i&0x4)      {
                        cpu_clock++;
			br=LoadByte(sr);
			sr=(sr+1)&0xffff;
			}
	if (i&0x8)      {
                        cpu_clock++;
			dp=LoadByte(sr);
			sr=(sr+1)&0xffff;
			}
	if (i&0x10)     {
                        cpu_clock+=2;
			xr=LoadWord(sr);
			sr=(sr+2)&0xffff;
			}
	if (i&0x20)     {
                        cpu_clock+=2;
			yr=LoadWord(sr);
			sr=(sr+2)&0xffff;
			}
	if (i&0x40)     {
                        cpu_clock+=2;
			ur=LoadWord(sr);
			sr=(sr+2)&0xffff;
			}
	if (i&0x80)     {
                        cpu_clock+=2;
			pc=LoadWord(sr);
			sr=(sr+2)&0xffff;
			}
	}

static void puls(void)
	{
	pulsr(*op);
	}

static void pshu(void)
	{
		int     i=*op;

	if (i&0x80)     {
			ur=(ur-2)&0xffff;
			StoreWord(ur,pc);
                        cpu_clock+=2;
			}
	if (i&0x40)     {
			ur=(ur-2)&0xffff;
			StoreWord(ur,sr);
                        cpu_clock+=2;
			}
	if (i&0x20)     {
			ur=(ur-2)&0xffff;
			StoreWord(ur,yr);
                        cpu_clock+=2;
			}
	if (i&0x10)     {
			ur=(ur-2)&0xffff;
			StoreWord(ur,xr);
                        cpu_clock+=2;
			}
	if (i&0x8)      {
			ur=(ur-1)&0xffff;
			StoreByte(ur,dp);
                        cpu_clock++;
			}
	if (i&0x4)      {
			ur=(ur-1)&0xffff;
			StoreByte(ur,br);
                        cpu_clock++;
			}
	if (i&0x2)      {
			ur=(ur-1)&0xffff;
			StoreByte(ur,ar);
                        cpu_clock++;
			}
	if (i&0x1)      {
			ur=(ur-1)&0xffff;
			StoreByte(ur,getcc());
                        cpu_clock++;
			}
	}

static void pulu(void)
	{
		int     i=*op;

	if (i&0x1)      {
                        cpu_clock++;
			setcc(LoadByte(ur));
			ur=(ur+1)&0xffff;
			}
	if (i&0x2)      {
                        cpu_clock++;
			ar=LoadByte(ur);
			ur=(ur+1)&0xffff;
			}
	if (i&0x4)      {
                        cpu_clock++;
			br=LoadByte(ur);
			ur=(ur+1)&0xffff;
			}
	if (i&0x8)      {
                        cpu_clock++;
			dp=LoadByte(ur);
			ur=(ur+1)&0xffff;
			}
	if (i&0x10)     {
                        cpu_clock+=2;
			xr=LoadWord(ur);
			ur=(ur+2)&0xffff;
			}
	if (i&0x20)     {
                        cpu_clock+=2;
			yr=LoadWord(ur);
			ur=(ur+2)&0xffff;
			}
	if (i&0x40)     {
                        cpu_clock+=2;
			sr=LoadWord(ur);
			ur=(ur+2)&0xffff;
			}
	if (i&0x80)     {
                        cpu_clock+=2;
			pc=LoadWord(ur);
			ur=(ur+2)&0xffff;
			}
	}

static void rtsm(void)
	{
	pc=LoadWord(sr);
	sr=(sr+2)&0xffff;
	}

static void abxm(void)
	{
	xr=(xr+br)&0xffff;
	}

static void rtim(void)
	{
	pulsr(1);
	if (ccrest&0x80) pulsr(0xfe);
		else    pulsr(0x80);
	}

static void cwai(void)
	{
        /* non supporté */
        }

static void mulm(void)             /* ZxCx */ 
	{
		int    k;
	k=ar*br;
	ar=(k>>8)&255;
	br=k&255;
	res=((br&0x80)<<1)|( (k|(k>>8)) &255);  /* c=bit7 de br */
	}

static void swim(void)
	{
	ccrest|=0x80;
	pshsr(0xff);
	ccrest|=0x50;
	pc=LoadWord(0xFFFA);
	}

static void nega(void)             /* H?NxZxVxCx */
	{
	m1=ar; m2=-ar;          /* bit V */
	ar=-ar;
	ovfl=res=sign=ar;
	ar&=255;
	}

static void coma(void)             /* NxZxV0C1 */
	{
	m1=ovfl;
	ar=(~ar)&255;
	sign=ar;
	res=sign|0x100; /* bit C a 1 */
	}

static void lsra(void)             /* N0ZxCx */
	{
	res=(ar&1)<<8;  /* bit C */
	ar>>=1;
	sign=0;
	res|=ar;
	}

static void rora(void)             /* NxZxCx */
	{
		int     i;
	i=ar;
	ar=(ar|(res&0x100))>>1;
	sign=ar;
	res=((i&1)<<8)|sign;
	}

static void asra(void)             /* H?NxZxCx */
	{
	res=(ar&1)<<8;
	ar=(ar>>1)|(ar&0x80);
	sign=ar;
	res|=sign;
	}

static void asla(void)             /* H?NxZxVxCx */
	{
	m1=m2=ar;
	ar<<=1;
	ovfl=sign=res=ar;
	ar&=255;
	}

static void rola(void)             /* NxZxVxCx */
	{
		int     i;
	i=ar;
	m1=m2=ar;
	ar=(ar<<1)|((res&0x100)>>8);
	ovfl=sign=res=ar;
	ar&=255;
	}

static void deca(void)             /* NxZxVx */
	{
	m1=ar; m2=0x80;
	ar=(ar-1)&255;
	ovfl=sign=ar;
	res=(res&0x100)|sign;
	}

static void inca(void)             /* NxZxVx */
	{
	m1=ar; m2=0;
	ar=(ar+1)&255;
	ovfl=sign=ar;
	res=(res&0x100)|sign;
	}

static void tsta(void)             /* NxZxV0 */
	{
	m1=ovfl;
	sign=ar;
	res=(res&0x100)|sign;
	}

static void clra(void)             /* N0Z1V0C0 */
	{
	ar=0;
	m1=ovfl;
	sign=res=0;
	}

static void negb(void)             /* H?NxZxVxCx */
	{
	m1=br; m2=-br;          /* bit V */
	br=-br;
	ovfl=res=sign=br;
	br&=255;
	}

static void comb(void)             /* NxZxV0C1 */
	{
	m1=ovfl;
	br=(~br)&255;
	sign=br;
	res=sign|0x100; /* bit C a 1 */
	}

static void lsrb(void)             /* N0ZxCx */
	{
	res=(br&1)<<8;  /* bit C */
	br>>=1;
	sign=0;
	res|=br;
	}

static void rorb(void)             /* NxZxCx */
	{
		int     i;
	i=br;
	br=(br|(res&0x100))>>1;
	sign=br;
	res=((i&1)<<8)|sign;
	}

static void asrb(void)             /* H?NxZxCx */
	{
	res=(br&1)<<8;
	br=(br>>1)|(br&0x80);
	sign=br;
	res|=sign;
	}

static void aslb(void)             /* H?NxZxVxCx */
	{
	m1=m2=br;
	br<<=1;
	ovfl=sign=res=br;
	br&=255;
	}

static void rolb(void)             /* NxZxVxCx */
	{
		int     i;
	i=br;
	m1=m2=br;
	br=(br<<1)|((res&0x100)>>8);
	ovfl=sign=res=br;
	br&=255;
	}

static void decb(void)             /* NxZxVx */
	{
	m1=br; m2=0x80;
	br=(br-1)&255;
	ovfl=sign=br;
	res=(res&0x100)|sign;
	}

static void incb(void)             /* NxZxVx */
	{
	m1=br; m2=0;
	br=(br+1)&255;
	ovfl=sign=br;
	res=(res&0x100)|sign;
	}

static void tstb(void)             /* NxZxV0 */
	{
	m1=ovfl;
	sign=br;
	res=(res&0x100)|sign;
	}

static void clrb(void)             /* N0Z1V0C0 */
	{
	br=0;
	m1=ovfl;
	sign=res=0;
	}

static void suba(void)             /* H?NxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ar; m2=-val;
	ar-=val;
	ovfl=res=sign=ar;
	ar&=255;
	}

static void cmpa(void)             /* H?NxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ar; m2=-val;
	ovfl=res=sign=ar-val;
	}

static void sbca(void)             /* H?NxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ar; m2=-val;
	ar-=val+((res&0x100)>>8);
	ovfl=res=sign=ar;
	ar&=255;
	}

static void subd(void)             /* NxZxVxCx */
	{
		int    dr,val;
	val=LoadWord((*adresl[ad])());
	m1=ar; m2=(-val)>>8;
	dr=(ar<<8)+br-val;
	ar=dr>>8;
	br=dr&255;
	ovfl=res=sign=ar;
	res|=br;
	ar&=255;
	}

static void anda(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	ar&=val;
	sign=ar;
	res=(res&0x100)|sign;
	}

static void bita(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	sign=ar&val;
	res=(res&0x100)|sign;
	}

static void ldam(void)             /* NxZxV0 */
	{
	sign=ar=LoadByte((*adresc[ad])());
	m1=ovfl;
	res=(res&0x100)|sign;
	}

static void stam(void)             /* NxZxV0 */
	{
	StoreByte((*adresc[ad])(),ar);
	sign=ar;
	m1=ovfl;
	res=(res&0x100)|sign;
	}

static void eora(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	ar^=val;
	sign=ar;
	res=(res&0x100)|sign;
	}

static void adca(void)             /* HxNxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=h1=ar; m2=val;
	h2=val+((res&0x100)>>8);
	ar+=h2;
	ovfl=res=sign=ar;
	ar&=255;
	}

static void oram(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	ar|=val;
	sign=ar;
	res=(res&0x100)|sign;
	}

static void adda(void)             /* HxNxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=h1=ar; m2=h2=val;
	ar+=val;
	ovfl=res=sign=ar;
	ar&=255;
	}

static void cmpx(void)             /* NxZxVxCx */
	{
		int    val;
	val=LoadWord((*adresl[ad])());
	m1=xr>>8; m2=(-val)>>8;
	ovfl=res=sign=(xr-val)>>8;
	res|=(xr-val)&255;
	}

static void bsrm(void)
	{
	sr=(sr-2)&0xffff;
	StoreWord(sr,pc);
	pc=(pc+op[0])&0xffff;
	}

static void ldxm(void)     /* NxZxV0 */
	{
	xr=LoadWord((*adresl[ad])());
	m1=ovfl;
	sign=xr>>8;
	res=(res&0x100)|((sign|xr)&255);
	}

static void stxm(void)             /* NxZxV0 */
	{
	StoreWord((*adresl[ad])(),xr);
	m1=0; m2=0x80;
	sign=xr>>8;
	res=(res&0x100)|((sign|xr)&255);
	}

static void subb(void)             /* H?NxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=br; m2=-val;
	br-=val;
	ovfl=res=sign=br;
	br&=255;
	}

static void cmpb(void)             /* H?NxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=br; m2=-val;
	ovfl=res=sign=br-val;
	}

static void sbcb(void)             /* H?NxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=br; m2=-val;
	br-=val+((res&0x100)>>8);
	ovfl=res=sign=br;
	br&=255;
	}

static void addd(void)             /* NxZxVxCx */
	{
		int    dr,val;
	val=LoadWord((*adresl[ad])());
	m1=ar; m2=val>>8;
	dr=(ar<<8)+br+val;
	ar=dr>>8;
	br=dr&255;
	ovfl=res=sign=ar;
	res|=br;
	ar&=255;
	}

static void andb(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	br&=val;
	sign=br;
	res=(res&0x100)|sign;
	}

static void bitb(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	sign=br&val;
	res=(res&0x100)|sign;
	}

static void ldbm(void)             /* NxZxV0 */
	{
	sign=br=LoadByte((*adresc[ad])());
	m1=ovfl;
	res=(res&0x100)|sign;
	}

static void stbm(void)             /* NxZxV0 */
	{
	StoreByte((*adresc[ad])(),br);
	sign=br;
	m1=ovfl;
	res=(res&0x100)|sign;
	}

static void eorb(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	br^=val;
	sign=br;
	res=(res&0x100)|sign;
	}

static void adcb(void)             /* HxNxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=h1=br; m2=val;
	h2=val+((res&0x100)>>8);
	br+=h2;
	ovfl=res=sign=br;
	br&=255;
	}

static void orbm(void)             /* NxZxV0 */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=ovfl;
	br|=val;
	sign=br;
	res=(res&0x100)|sign;
	}

static void addb(void)             /* HxNxZxVxCx */
	{
		int     val;
	val=LoadByte((*adresc[ad])());
	m1=h1=br; m2=h2=val;
	br+=val;
	ovfl=res=sign=br;
	br&=255;
	}

static void lddm(void)             /* NxZxV0 */
	{
		int    dr;
	dr=LoadWord((*adresl[ad])());
	m1=ovfl;
	ar=(dr>>8)&255;
	br=dr&255;
	sign=ar;
	res=(res&0x100)|br|ar;
	}

static void stdm(void)             /* NxZxV0 */
	{
	StoreWord((*adresl[ad])(),(ar<<8)+br);
	m1=ovfl;
	sign=ar;
	res=(res&0x100)|ar|br;
	}

static void ldum(void)     /* NxZxV0 */
	{
	ur=LoadWord((*adresl[ad])());
	m1=ovfl;
	sign=ur>>8;
	res=(res&0x100)|((sign|ur)&255);
	}

static void stum(void)             /* NxZxV0 */
	{
	StoreWord((*adresl[ad])(),ur);
	m1=ovfl;
	sign=ur>>8;
	res=(res&0x100)|((sign|ur)&255);
	}

static void lbrn(void)
{
}

static void lbhi(void)     /* c|z=0 */
{
    if ((!(res&0x100))&&(res&0xff))
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbls(void)     /* c|z=1 */
{
    if ((res&0x100)||(!(res&0xff)))
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbcc(void)     /* c=0 */
{
    if (!(res&0x100))
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lblo(void)     /* c=1 */
{
    if (res&0x100)
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbne(void)     /* z=0 */
{
    if (res&0xff)
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbeq(void)     /* z=1 */
{
    if (!(res&0xff))
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbvc(void)     /* v=0 */
{
    if ( ((m1^m2)&0x80)||(!((m1^ovfl)&0x80)) )
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbvs(void)     /* v=1 */
{
    if ( (!((m1^m2)&0x80))&&((m1^ovfl)&0x80) )
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbpl(void)     /* n=0 */
{
    if (!(sign&0x80))
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbmi(void)     /* n=1 */
{
    if (sign&0x80)
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbge(void)     /* n^v=0 */
{
    if (!((sign^((~(m1^m2))&(m1^ovfl)))&0x80))
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lblt(void)     /* n^v=1 */
{
    if ((sign^((~(m1^m2))&(m1^ovfl)))&0x80)
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lbgt(void)     /* z|(n^v)=0 */
{
    if ( (res&0xff)&&(!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) )
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void lble(void)     /* z|(n^v)=1 */
{
    if ( (!(res&0xff))||((sign^((~(m1^m2))&(m1^ovfl)))&0x80) )
    {
        pc=(pc+(op[0]<<8)+(op[1]&255))&0xffff;
        cpu_clock++;
    }
}

static void swi2(void)
	{
	ccrest|=0x80;
	pshsr(0xff);
	pc=LoadWord(0xFFF4);
	}

static void swi3(void)
	{
	ccrest|=0x80;
	pshsr(0xff);
	pc=LoadWord(0xFFF2);
	}

static void cmpd(void)             /* NxZxVxCx */
	{
		int    dr,val;
	val=LoadWord((*adresl[ad])());
	m1=ar; m2=(-val)>>8;
	dr=(ar<<8)+br-val;
	ovfl=res=sign=dr>>8;
	res|=(dr&255);
	}

static void cmpy(void)             /* NxZxVxCx */
	{
		int    val;
	val=LoadWord((*adresl[ad])());
	m1=yr>>8; m2=(-val)>>8;
	ovfl=res=sign=(yr-val)>>8;
	res|=(yr-val)&255;
	}

static void cmpu(void)             /* NxZxVxCx */
	{
		int    val;
	val=LoadWord((*adresl[ad])());
	m1=ur>>8; m2=(-val)>>8;
	ovfl=res=sign=(ur-val)>>8;
	res|=(ur-val)&255;
	}

static void cmps(void)             /* NxZxVxCx */
	{
		int    val;
	val=LoadWord((*adresl[ad])());
	m1=sr>>8; m2=(-val)>>8;
	ovfl=res=sign=(sr-val)>>8;
	res|=(sr-val)&255;
	}

static void ldym(void)     /* NxZxV0 */
	{
	yr=LoadWord((*adresl[ad])());
	m1=ovfl;
	sign=yr>>8;
	res=(res&0x100)|((sign|yr)&255);
	}

static void stym(void)             /* NxZxV0 */
	{
	StoreWord((*adresl[ad])(),yr);
	m1=ovfl;
	sign=yr>>8;
	res=(res&0x100)|((sign|yr)&255);
	}

static void ldsm(void)     /* NxZxV0 */
	{
	sr=LoadWord((*adresl[ad])());
	m1=ovfl;
	sign=sr>>8;
	res=(res&0x100)|((sign|sr)&255);
	}

static void stsm(void)             /* NxZxV0 */
	{
	StoreWord((*adresl[ad])(),sr);
	m1=ovfl;
	sign=sr>>8;
	res=(res&0x100)|((sign|sr)&255);
	}

static void (*code[])(void);

static void trap(void)
{
    struct MC6809_REGS regs;
    int r;

    mc6809_GetRegs(&regs);

    r=TrapCallback(&regs);

    mc6809_SetRegs(&regs, 0x1FF);

    ad=adr[r];
    cpu_clock+=cpu_cycles[r];
    pc+=taille[r]-1;

    (*code[r])();
}

static void cd10(void)
{
    int r=((*(op++))&0xFF)+256;

    ad=adr[r];
    cpu_clock+=cpu_cycles[r];
    pc+=taille[r];

    (*code[r])();
}

static void cd11(void)
{
    int r=((*(op++))&0xFF)+512;

    ad=adr[r];
    cpu_clock+=cpu_cycles[r];
    pc+=taille[r];

    (*code[r])();
}


static void (*code[])(void)=
{negm,what,trap,comm,lsrm,what,rorm,asrm,aslm,rolm,decm,what,incm,tstm,jmpm,clrm
,cd10,cd11,nopm,synm,what,what,lbra,lbsr,what,daam,orcc,what,andc,sexm,exgm,tfrm
,bras,brns,bhis,blss,bccs,blos,bnes,beqs,bvcs,bvss,bpls,bmis,bges,blts,bgts,bles
,leax,leay,leas,leau,pshs,puls,pshu,pulu,what,rtsm,abxm,rtim,cwai,mulm,what,swim
,nega,what,what,coma,lsra,what,rora,asra,asla,rola,deca,what,inca,tsta,what,clra
,negb,what,what,comb,lsrb,what,rorb,asrb,aslb,rolb,decb,what,incb,tstb,what,clrb
,negm,what,what,comm,lsrm,what,rorm,asrm,aslm,rolm,decm,what,incm,tstm,jmpm,clrm
,negm,what,what,comm,lsrm,what,rorm,asrm,aslm,rolm,decm,what,incm,tstm,jmpm,clrm
,suba,cmpa,sbca,subd,anda,bita,ldam,what,eora,adca,oram,adda,cmpx,bsrm,ldxm,what
,suba,cmpa,sbca,subd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpx,jsrm,ldxm,stxm
,suba,cmpa,sbca,subd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpx,jsrm,ldxm,stxm
,suba,cmpa,sbca,subd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpx,jsrm,ldxm,stxm
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,what,eorb,adcb,orbm,addb,lddm,what,ldum,what
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum

,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,lbrn,lbhi,lbls,lbcc,lblo,lbne,lbeq,lbvc,lbvs,lbpl,lbmi,lbge,lblt,lbgt,lble
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,swi2
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,cmpd,what,what,what,what,what,what,what,what,cmpy,what,ldym,what
,what,what,what,cmpd,what,what,what,what,what,what,what,what,cmpy,what,ldym,stym
,what,what,what,cmpd,what,what,what,what,what,what,what,what,cmpy,what,ldym,stym
,what,what,what,cmpd,what,what,what,what,what,what,what,what,cmpy,what,ldym,stym
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,ldsm,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,ldsm,stsm
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,ldsm,stsm
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,ldsm,stsm

,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,swi3
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,cmpu,what,what,what,what,what,what,what,what,cmps,what,what,what
,what,what,what,cmpu,what,what,what,what,what,what,what,what,cmps,what,what,what
,what,what,what,cmpu,what,what,what,what,what,what,what,what,cmps,what,what,what
,what,what,what,cmpu,what,what,what,what,what,what,what,what,cmps,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what,what
,what,what,what,cmpu,what,what,what,what,what,what,what,what,cmps,what,what,what
};


static inline void do_nmi(void)
{
    ccrest|=0x80;  /* E à 1 */
    pshsr(0xff);
    ccrest|=0x50;
    cpu_clock+=7;  /* 2 + 5 pour pshsr */
    pc=LoadWord(0xFFFC);
}


static inline void do_irq(void)
{
    if (!(ccrest&0x10)) /* si I à 0 */
    {
        ccrest|=0x80;  /* E à 1 */
        pshsr(0xff);
        ccrest|=0x10;
        cpu_clock+=7;  /* 2 + 5 pour pshsr */
        pc=LoadWord(0xFFF8);
    }
}


static inline void do_firq(void)
{
    if (!(ccrest&0x40)) /* si F à 0 */
    {
        ccrest&=0x7F;  /* E à 0 */
        pshsr(0x81);
        ccrest|=0x50;
        cpu_clock+=7;  /* 2 + 5 pour pshsr */
        pc=LoadWord(0xFFF6);
    }
}



/************************************************/
/*** Interface publique de l'émulateur MC6809 ***/
/************************************************/

/* broche de demande d'interruption ordinaire */
int mc6809_irq;

#ifdef DEBUG
   FILE *mc6809_ftrace=NULL;
#endif



/* clock:
 *  Retourne la valeur de l'horloge du CPU.
 */
mc6809_clock_t mc6809_clock(void)
{
    return cpu_clock;
}



/* GetRegs:
 *  Retourne le contenu des registres du CPU.
 */
void mc6809_GetRegs(struct MC6809_REGS *regs)
{
    regs->cc        = getcc();
    regs->dp        = dp;
    regs->ar        = ar;
    regs->br        = br;
    regs->xr        = xr;
    regs->yr        = yr;
    regs->ur        = ur;
    regs->sr        = sr;
    regs->pc        = pc;
    regs->cpu_clock = cpu_clock;
    regs->cpu_timer = cpu_timer;
}



/* SetRegs:
 *  Modifie le contenu des registres du CPU.
 */
void mc6809_SetRegs(const struct MC6809_REGS *regs, int flags)
{
    if (flags&MC6809_REGS_CC_FLAG)
        setcc(regs->cc);

    if (flags&MC6809_REGS_DP_FLAG)
        dp=regs->dp;

    if (flags&MC6809_REGS_AR_FLAG)
        ar=regs->ar;

    if (flags&MC6809_REGS_BR_FLAG)
        br=regs->br;

    if (flags&MC6809_REGS_XR_FLAG)
        xr=regs->xr;

    if (flags&MC6809_REGS_YR_FLAG)
        yr=regs->yr;

    if (flags&MC6809_REGS_UR_FLAG)
        ur=regs->ur;

    if (flags&MC6809_REGS_SR_FLAG)
        sr=regs->sr;

    if (flags&MC6809_REGS_PC_FLAG)
        pc=regs->pc;

    if (flags&MC6809_REGS_CPUCLOCK_FLAG)
        cpu_clock=regs->cpu_clock;

    if (flags&MC6809_REGS_CPUTIMER_FLAG)
        cpu_timer=regs->cpu_timer;
}



/* SetTimer:
 *  Installe un callback appelé par le CPU à expiration de la période spécifiée.
 */
void mc6809_SetTimer(mc6809_clock_t time, void (*func)(void *), void *data)
{
    cpu_timer     = time;
    TimerCallback = func;
    timer_data    = data;
}



/* Reset:
 *  Remet à zéro le CPU (envoie un signal sur la broche RESET du MC6809).
 */
void mc6809_Reset(void)
{
    dp=0;
    ccrest|=0x50;
    pc=LoadWord(0xFFFE);
    mc6809_irq=0;
}



/* Init:
 *  Initialise l'émulation du MC6809.
 */
void mc6809_Init(const struct MC6809_INTERFACE *interface)
{
    int i;

    regist[0] = &xr;
    regist[1] = &yr;
    regist[2] = &ur;
    regist[3] = &sr;

    for(i=0; i<16; i++)
        exreg[i] = NULL;

    exreg[1] =  &xr;
    exreg[2] =  &yr;
    exreg[3] =  &ur;
    exreg[4] =  &sr;
    exreg[5] =  &pc;
    exreg[8] =  &ar;
    exreg[9] =  &br;
    exreg[11] = &dp;

    cpu_clock = 0;
    cpu_timer = MC6809_TIMER_DISABLED;

    FetchInstr    = interface->FetchInstr;
    LoadByte      = interface->LoadByte;
    LoadWord      = interface->LoadWord;
    StoreByte     = interface->StoreByte;
    StoreWord     = interface->StoreWord;
    TrapCallback  = interface->TrapCallback;
}



/* StepExec:
 *  Exécute un nombre donné d'instructions et retourne le
 *  nombre de cycles nécessaires à leur éxécution.
 */
int mc6809_StepExec(unsigned int ninst)
{
    mc6809_clock_t start_clock=cpu_clock;
    register unsigned int i;
                      int r;

    for (i=0; i<ninst; i++)
    {
        if (cpu_clock>=cpu_timer)
            TimerCallback(timer_data);

        if (mc6809_irq)
            do_irq();

#ifdef DEBUG
        if (mc6809_ftrace)
            fprintf(mc6809_ftrace, "pc: %04X\n", pc);
#endif

        /* on remplit le buffer de fetch */
        FetchInstr(pc, fetch_buffer);
        op=(char*)fetch_buffer;

        /* on décode l'instruction */
        r=(*(op++))&0xFF;
        ad=adr[r];
        cpu_clock+=cpu_cycles[r];
        pc+=taille[r];

        /* on éxécute l'instruction */
        (*code[r])();
    }

    return cpu_clock-start_clock;
}



/* TimeExec:
 *  Fait tourner le MC6809 jusqu'à un instant donné et
 *  retourne le nombre d'instructions éxécutées.
 */
int mc6809_TimeExec(mc6809_clock_t time_limit)
{
    int r, ninst=0;

    while (cpu_clock<time_limit)
    {
        if (cpu_clock>=cpu_timer)
            TimerCallback(timer_data);

        if (mc6809_irq)
            do_irq();

#ifdef DEBUG
        if (mc6809_ftrace)
            fprintf(mc6809_ftrace, "pc: %04X\n", pc);
#endif

        /* on remplit le buffer de fetch */
        FetchInstr(pc, fetch_buffer);
        op=(char*)fetch_buffer;

        /* on décode l'instruction */
        r=(*(op++))&0xFF;

        ad=adr[r];
        cpu_clock+=cpu_cycles[r];
        pc+=taille[r];

        /* on éxécute l'instruction */
        (*code[r])();
        ninst++;
    }

    return ninst;
}

/* TimeExec:
 *  Fait tourner le MC6809 jusqu'à un instant donné et
 *  retourne le nombre d'instructions éxécutées.
 */
int mc6809_TimeExec_debug(mc6809_clock_t time_limit)
{
    int r, ninst=0;

    while (cpu_clock<time_limit)
    {
        if (cpu_clock>=cpu_timer)
            TimerCallback(timer_data);

        if (mc6809_irq)
            do_irq();

#ifdef DEBUG
        if (mc6809_ftrace)
            fprintf(mc6809_ftrace, "pc: %04X\n", pc);
#endif

        /* on remplit le buffer de fetch */
        FetchInstr(pc, fetch_buffer);
        op=(char*)fetch_buffer;

        /* on décode l'instruction */
        r=(*(op++))&0xFF;

        ad=adr[r];
        cpu_clock+=cpu_cycles[r];
        pc+=taille[r];

        /* on éxécute l'instruction */
        (*code[r])();
        ninst++;
#ifdef OS_LINUX
	if (check_bkpt(pc&0xFFFF)) return -1;
#endif
    }

    return ninst;
}
