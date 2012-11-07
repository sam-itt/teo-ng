/*
 *  Module d'émulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou, 2011 Gilles Fétis
 *                2012 François Mouret, 2012 Samuel Devulder.
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
 *  Version    : 2.8.0
 *  Créé par   : Sylvain Huet 1996
 *  Modifié par: Eric Botcazou 30/11/2000
 *               François Mouret 27/09/2006 26/01/2010 04/02/2012
 *               Gilles Fétis 27/07/2011
 *               Samuel Devulder 04/02/2012
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
 *  version 2.8: exécution cycle par cycle des instructions
 *               émulation des instructions non standard
 *               émulation des postcodes non standard pour TFR/EXG
 *               émulation des postcodes non standard pour indexé
 *               émulation du postcode 0x00 pour PSHS/PSHU/PULS/PULU
 */

#include "teo.h"


#ifdef OS_LINUX
extern int udebug_Breakpoint (int pc);
#endif

/* broche de demande d'interruption ordinaire */
int mc6809_irq;

#ifdef DEBUG
   FILE *mc6809_ftrace=NULL;
#endif



static void (*FetchInstr)(int, unsigned char []);
static int  (*LoadByte)(int);
static int  (*LoadWord)(int);
static void (*StoreByte)(int, int);
static void (*StoreWord)(int, int);

static int  (*TrapCallback)(struct MC6809_REGS *);
static void (*TimerCallback)(void *);
static void *timer_data;

static struct MC6809_REGS reg_list;

static void (*addr[])(void);

/* le caractère 8-bit du MC6809 impose l'utilisation de char
   pour la manipulation des opcodes qui sont des octets signés */
static char byte;
static int *regist[4], *exreg[16];

/* variables d'état du MC6809 */
static int page,opcode,postcode,address,value;
static int *reg;
static int step;
static mc6809_clock_t cpu_clock, cpu_timer;
static int pc,xr,yr,ur,sr,ar,br,dp,dr;
static int res,m1,m2,sign,ovfl,h1,h2,ccrest;
static int bus;
static int irq_start,irq_run;

static void (*compute_address)(void);
static const int swi_vector[] = { 0xFFFA, 0xFFF4, 0xFFF2 };

/*************************************************/
/*** gestion du registre d'état (CC) du MC6809 ***/
/*************************************************/

static int getcc(void) {
    return  ((((h1&15)+(h2&15))&16)<<1)            /* ..x..... H   */
             |((sign&0x80)>>4)                     /* ....x... N   */
             |((((res&0xff)==0)&1)<<2)             /* .....x.. Z   */
             |(( ((~(m1^m2))&(m1^ovfl))&0x80)>>6) /* ......x. V   */
             |((res&0x100)>>8)                     /* .......x C   */
             |ccrest;                              /* xx.x.... EFI */
}

static void setcc(int i) {
    m1=m2=0;                  
    res=((i&1)<<8)|(4-(i&4)); /* .....x.x ZC  */
    ovfl=(i&2)<<6;            /* ......x. V   */
    sign=(i&8)<<4;            /* ....x... N   */
    h1=h2=(i&32)>>2;          /* ..x..... H   */
    ccrest=i&0xd0;            /* xx.x.... EFI */
}

/* ========================================================== */
/*                     Adressing modes                        */
/* ========================================================== */

static void indxp(void) {  /* ,r+ */
    switch (step) {
    /* [Don't Care] */
    case 5 : reg=regist[(postcode&0x60)>>5];
             address=*reg;
             *reg=((*reg)+1)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    /* Cases 3,4 = [Don't Care] */
    default: break;
    }
}

static void indxpp (void) { /* ,r++ */
    switch (step) {
    /* [Don't Care] */
    case 3 : reg=regist[(postcode&0x60)>>5];
             address=*reg;
             *reg=((*reg)+2)&0xffff;
             break;
    /* [Don't Care] */
    case 6 : step=0x20-(postcode&0x10);
             break;
    /* Cases 4,5 = [Don't Care] */
    default: break;
    }
}

static void indmx(void) {   /* ,-r */
    switch (step) {
    /* [Don't Care] */
    case 5 : reg=regist[(postcode&0x60)>>5];
             *reg=((*reg)-1)&0xffff;
             address=*reg;
             step=0x20-(postcode&0x10);
             break;
    /* Cases 3,4 = [Don't Care] */
    default: break;
    }
}

static void indmmx(void) { /* ,--r */
    switch (step) {
    /* [Don't Care] */
    case 6 : reg=regist[(postcode&0x60)>>5];
             *reg=((*reg)-2)&0xffff;
             address=*reg;
             step=0x20-(postcode&0x10);
             break;
    /* Cases 3,4,5 = [Don't Care] */
    default: break;
    }
}

static void indx0(void) {   /* ,r */
    /* [Don't Care] */
    address=*(regist[(postcode&0x60)>>5]);
    step=0x20-(postcode&0x10);
}

static void indbx(void) {   /* B,r */
    switch (step) {
    /* [Don't Care] */
    case 4 : byte=br;
             address=((*(regist[(postcode&0x60)>>5]))+byte)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    /* Case 3 = [Don't Care] */
    default: break;
    }
}

static void indax(void) {   /* A,r */
    switch (step) {
    /* [Don't Care] */
    case 4 : byte=ar;
             address=((*(regist[(postcode&0x60)>>5]))+byte)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    /* Case 3 = [Don't Care] */
    default: break;
    }
}

static void ind07(void) {   /* ,r (not standard) */
    switch (step) {
    /* [Don't Care] */
    case 4 : address=*(regist[(postcode&0x60)>>5]);
             step=0x20-(postcode&0x10);
             break;
    /* Case 3 = [Don't Care] */
    default: break;
    }
}

static void ind1x(void) {   /* n8,r */
    switch (step) {
    /* [Offset : NNNN+2(3)] */
    case 3 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : address=((*(regist[(postcode&0x60)>>5]))+byte)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    }
}

static void ind2x(void) {   /* n16,r */
    switch (step) {
    /* [Offset High : NNNN+2(3)] */
    case 3 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3(4)] */
    case 4 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 7 : address=((*(regist[(postcode&0x60)>>5]))+value)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    /* Cases 5,6 = [Don't Care] */
    default: break;
    }
}

static void ind0A(void) {    /* pc|$ff (not standard) */
    switch (step) {
    /* [Don't Care] */
    case 7 : address=pc|0xff;
             ar&=LoadByte(pc);
             step=0x20-(postcode&0x10);
             break;
    /* Cases 3,4,5,6 = [Don't Care] */
    default: break;
    }
}

static void inddx(void) {    /* D,r */
    switch (step) {
    /* [Don't Care] */
    case 7 : address=((*(regist[(postcode&0x60)>>5]))+((ar<<8)+br))&0xffff;
             step=0x20-(postcode&0x10);
             break;
    /* Cases 3,4,5,6 = [Don't Care] */
    default: break;
    }
}

static void ind1p(void) {    /* n8,PCR */
    switch (step) {
    /* [Offset : NNNN+2(3)] */
    case 3 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : address=(pc+byte)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    }
}

static void ind2p(void) {    /* n16,PCR */
    switch (step) {
    /* [Offset High : NNNN+2(3)] */
    case 3 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3(4)] */
    case 4 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 8 : address=(pc+value)&0xffff;
             step=0x20-(postcode&0x10);
             break;
    /* Cases 5,6,7 = [Don't Care] */
    default: break;
    }
}

static void ind0E(void) {   /* $ffff (not standard) */
    switch (step) {
    /* [Address High] */
    case 3 : address=0xff00;
             break;
    /* [Address Low] */
    case 4 : address=0xffff;
             break;
    /* [Don't Care] */
    case 8 : step=0x20-(postcode&0x10);
             break;
    /* Cases 5,6,7 = [Don't Care] */
    default: break;
    }
}

static void indad(void) {    /* n16 */
    switch (step) {
    /* [Address High : NNNN+2(3)] */
    case 3 : address=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Address Low : NNNN+3(4)] */
    case 4 : address|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 5 : step=0x20-(postcode&0x10);
             break;
    }
}

static void (*indmod[])(void)= {
    indxp ,   /* 0  ,r+ */
    indxpp,   /* 1  ,r++ */
    indmx ,   /* 2  ,-r */
    indmmx,   /* 3  ,--r */
    indx0 ,   /* 4  ,r */
    indbx ,   /* 5  B,r */
    indax ,   /* 6  A,r */
    ind07 ,   /* 7  ,r (not standard) */
    ind1x ,   /* 8  n8,r */
    ind2x ,   /* 9  n16,r */
    ind0A ,   /* A  pc|$ff (not standard) */
    inddx ,   /* B  D,r */
    ind1p ,   /* C  n8,PCR */
    ind2p ,   /* D  n16,PCR */
    ind0E ,   /* E  $ffff (not standard) */
    indad     /* F  n16 */
};

static void indx(void) {
    switch (step) {
    /* [Post Byte : NNNN+1(2)] */
    case 0x02 : postcode=LoadByte(pc);
                pc=(pc+1)&0xffff;
                if ((postcode&0x80)==0) step=0x17;
                break;
    /* -----  5 bits offset from register addressing ------ */
    /* [Don't Care] */
    case 0x18 : break;
    /* [Don't Care] */
    case 0x19 : address=*(regist[(postcode&0x60)>>5])+(postcode&0x0f)-(postcode&0x10);
                step=0x20; /* address computed */
                break;
    /* ----- indirect addressing ------ */
    /* [Indirect High : XXXX] */
    case 0x11 : value=address;
                address=LoadByte(value)<<8;
                break;
    /* [Indirect Low : XXXX+1] */
    case 0x12 : address|=LoadByte(value+1);
                break;
    /* [Don't Care] */
    case 0x13 : step=0x20; /* address computed */
                break;
    /* compute address */
    default   : (*indmod[postcode&0xf])();
                break;
    }
}

static void extn(void) {
    switch (step) {
    /*[Address High : NNNN+1(2)]*/
    case 2 : address=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /*[Address Low : NNNN+2(3)]*/
    case 3 : address|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : step=0x20; /* address computed */
             break;
    }
}

static void drct(void) {
    switch (step) {
    /*[Address High : NNNN+1(2)]*/
    case 2 : address=(dp<<8)|LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : step=0x20; /* address computed */
             break;
    }
}

static void rela(void) {
}

static void impl(void) {
}

static void imm1(void) {
    address=pc;
    pc=(pc+1)&0xffff;
    step=0x21; /* address computed */
}

static void imm2(void) {
    address=pc;
    pc=(pc+2)&0xffff;
    step=0x21; /* address computed */
}

/******************************/
/*** instructions du MC6809 ***/
/******************************/

/* ========================================================== */
/*       ASL ASR CLR COM DEC INC LSL LSR NEG ROL ROR TST      */
/* ========================================================== */

static void aslm(void) {    /* H?NxZxVxCx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=m2=value;
                    value<<=1;
                    ovfl=sign=res=value;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void asrm(void) {    /* H?NxZxCx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : res=(value&1)<<8;
                    value=(value>>1)|(value&0x80);
                    sign=value;
                    res|=sign;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void clrm(void) {    /* N0Z1V0C0 */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=~m2;
                    sign=res=value=0;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void comm(void) {    /* NxZxV0C1 */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=~m2;
                    value=(~value)&0xff;
                    sign=value;
                    res=sign|0x100; /* bit C a 1 */
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void decm(void) {    /* NxZxVx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=value; m2=0x80;
                    ovfl=sign=(--value)&0xff;
                    res=(res&0x100)|sign;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void incm(void) {    /* NxZxVx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=value; m2=0;
                    ovfl=sign=(++value)&0xff;
                    res=(res&0x100)|sign;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void lsrm(void) {    /* N0ZxCx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : res=(value&1)<<8; /* bit C */
                    value>>=1;
                    sign=0;
                    res|=value;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void negm(void) {    /* H?NxZxVxCx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=value; m2=-value;      /* bit V */
                    value=(-value)&0xff;
                    ovfl=res=sign=value;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void rolm(void) {    /* NxZxVxCx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : m1=m2=value;
                    value=(value<<1)|((res&0x100)>>8);
                    ovfl=sign=res=value;
                    StoreByte(address,value);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void rorm(void) {    /* NxZxCx */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Data (write) : EA] */
        case 0x23 : sign=(value|(res&0x100))>>1;
                    res=((value&1)<<8)|sign;
                    StoreByte(address,sign);
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

static void tstm(void) {    /* NxZxV0 */
    switch (step) {
        /* [Data : EA] */
        case 0x21 : value=LoadByte(address);
                    break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [Don't Care] */
        case 0x23 : m1=~m2;
                    sign=value;
                    res=(res&0x100)|sign;
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

/* ========================================================== */
/*                           JMP JSR                          */
/* ========================================================== */

static void jmpm(void) {
    compute_address();
    if (step==0x20) {
        pc=address;
        step=0;  /* reset fetch */
    }
}

static void jsrm(void) {    /* NxZxV0 */
    switch (step) {
        /* [Don't Care : Sub Address] */
        case 0x21 : break;
        /* [Don't Care] */
        case 0x22 : break;
        /* [PC Low (write) : Stack] */
        case 0x23 : sr=(sr-1)&0xffff;
                    StoreByte(sr,pc);
                    break;
        /* [PC High (write) : Stack] */
        case 0x24 : sr=(sr-1)&0xffff;
                    StoreByte(sr,pc>>8);
                    pc=address;
                    step=0;  /* reset fetch */
                    break;
        /* compute address */
        default   : compute_address();
                    break;
    }
}

/* ========================================================== */
/*                          TFR EXG                           */
/* ========================================================== */

static void tfrm(void) {
        int r1,r2,v1;
    switch (step) {
    /* [Post Byte : NNNN+1] */
    case 2 : postcode=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 6 : bus=0xffff;
             r1=postcode>>4;
             r2=postcode&0xf;
             v1=(r1&8)?((r1==0xa)?getcc()|0xff00:*exreg[r1]|0xff00)
                      :((r1)?*exreg[r1]:(ar<<8)+br);
             if (!r2) { ar=v1>>8; br=v1&0xff; }
             else if (r2==0xa) setcc(v1);
             else *exreg[r2]=(r2&8)?v1&0xff:v1;
             step=0;  /* reset fetch */
             break;
    /* Cases 3,4,5 = [Don't Care] */
    default: break;
    }
}        

static void exgm(void) {
        int r1,r2,v1,v2;
    switch (step) {
    /* [Post Byte : NNNN+1] */
    case 2 : postcode=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 8 : bus=0xffff;
             r1=postcode>>4;
             r2=postcode&0xf;
             v1=(r1&8)?((r1==0xa)?getcc()|0xff00:*exreg[r1]|0xff00)
                      :((r1)?*exreg[r1]:(ar<<8)+br);
             v2=(r2&8)?((r2==0xa)?getcc()|0xff00:*exreg[r2]|0xff00)
                      :((r2)?*exreg[r2]:(ar<<8)+br);
             if (!r1) { ar=v2>>8; br=v2&0xff; }
             else if (r1==0xa) setcc(v2);
             else *exreg[r1]=(r1&8)?v2&0xff:v2;
             if (!r2) { ar=v1>>8; br=v1&0xff; }
             else if (r2==0xa) setcc(v1);
             else *exreg[r2]=(r2&8)?v1&0xff:v1;
             step=0;  /* reset fetch */
             break;
    /* Cases 3,4,5,6,7 = [Don't Care] */
    default: break;
    }
} 

/* ========================================================== */
/*  BCC BCS BEQ BGE BGT BHI BHS BLE BLO BLS BLT BMI BNE BPL   */
/*                      BRA BRN BVC BVS                       */
/* ========================================================== */

static void bras(void) {    /* branch always */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void brns(void) {    /* branch never */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : step=0;  /* reset fetch */
             break;
    }
}

static void bhis(void) {    /* branch if C|Z=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if ((!(res&0x100))&&(res&0xff))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void blss(void) {    /* branch if C|Z=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if ((res&0x100)||(!(res&0xff)))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bccs(void) {    /* branch if C=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (!(res&0x100)) pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void blos(void) {    /* branch if C=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (res&0x100) pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bnes(void) {    /* branch if Z=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (res&0xff) pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void beqs(void) {    /* branch if Z=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (!(res&0xff)) pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bvcs(void) {    /* branch if V=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (((m1^m2)&0x80)||(!((m1^ovfl)&0x80)))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bvss(void) {    /* branch if V=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if ((!((m1^m2)&0x80))&&((m1^ovfl)&0x80))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bpls(void) {    /* branch if N=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (!(sign&0x80)) pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bmis(void) {    /* branch if N=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (sign&0x80) pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bges(void) {    /* branch if N^V=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if (!((sign^((~(m1^m2))&(m1^ovfl)))&0x80))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void blts(void) {    /* branch if N^V=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if ((sign^((~(m1^m2))&(m1^ovfl)))&0x80)
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bgts(void) {    /* branch if Z|(N^V)=0 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if ((res&0xff)&&(!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void bles(void) {    /* branch if Z|(N^V)=1 */
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : if ((!(res&0xff))||((sign^((~(m1^m2))&(m1^ovfl)))&0x80))
                 pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

/* ========================================================== */
/*   LBCC LBCS LBGE LBGT LBHI LBHS LBLE LBLS LBLT LBMI LBNE   */
/*                  LBPL LBRA LBRN LBVC LBVS                  */
/* ========================================================== */

static void lbra(void) {    /* branch always */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 5 : pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    /* Case 4 = [Don't Care] */
    default: break;
    }
}

static void lbrn(void) {    /* branch never */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* Case 4 = [Don't Care] */
    default: step=0;  /* reset fetch */
             break;
    }
}

static void lbhi(void) {    /* branch if c|z=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if ((res&0x100)||(!(res&0xff)))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbls(void) {    /* c|z=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if ((!(res&0x100))&&(res&0xff))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbcc(void) {    /* c=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (res&0x100)
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lblo(void) {    /* c=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (!(res&0x100))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbne(void) {    /* z=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (!(res&0xff))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbeq(void) {    /* z=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (res&0xff)
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbvc(void) {    /* v=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if ((!((m1^m2)&0x80))&&((m1^ovfl)&0x80))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbvs(void) {    /* v=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (((m1^m2)&0x80)||(!((m1^ovfl)&0x80)))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbpl(void) {    /* n=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (sign&0x80)
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbmi(void) {    /* n=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (!(sign&0x80))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbge(void) {    /* n^v=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if ((sign^((~(m1^m2))&(m1^ovfl)))&0x80)
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lblt(void) {    /* n^v=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if (!((sign^((~(m1^m2))&(m1^ovfl)))&0x80))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lbgt(void) {    /* z|(n^v)=0 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if ((!(res&0xff))||((sign^((~(m1^m2))&(m1^ovfl)))&0x80))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

static void lble(void) {    /* z|(n^v)=1 */
    switch (step) {
    /* [Offset High : NNNN+2] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset Low : NNNN+3] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : if ((res&0xff)&&(!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)))
                 step=0;  /* reset fetch */
             break;
    /* Case 5 = [Don't Care] */
    default: pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    }
}

/* ========================================================== */
/*                          BSR LBSR                          */
/* ========================================================== */

static void bsrm(void) {
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : byte=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* Return Address Low : Stack */
    case 6 : sr=(sr-1)&0xffff;
             StoreByte(sr,pc);
             break;
    /* Return Address High : Stack */
    case 7 : sr=(sr-1)&0xffff;
             StoreByte(sr,pc>>8);
             pc=(pc+byte)&0xffff;
             step=0;  /* reset fetch */
             break;
    /* Cases 3,4,5 = [Don't Care] */
    default: break;
    }
}

static void lbsr(void) {
    switch (step) {
    /* [Offset : NNNN+1] */
    case 2 : value=LoadByte(pc)<<8;
             pc=(pc+1)&0xffff;
             break;
    /* [Offset : NNNN+2] */
    case 3 : value|=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* Return Address Low : Stack */
    case 8 : sr=(sr-1)&0xffff;
             StoreByte(sr,pc);
             break;
    /* Return Address High : Stack */
    case 9 : sr=(sr-1)&0xffff;
             StoreByte(sr,pc>>8);
             pc=(pc+value)&0xffff;
             step=0;  /* reset fetch */
             break;
    /* Cases 4,5,6,7 = [Don't Care] */
    default: break;
    }
}

/* ========================================================== */
/*                         LEAX/Y/U/S                         */
/* ========================================================== */

static void leax(void) {    /* Zx */
    switch (step) {
    /* [Don't Care] */
    case 0x21: xr=address;
               res=(res&0x100)|((xr|(xr>>8))&0xff);
               step=0;  /* reset fetch */
               break;
    /* Compute Address */
    default  : compute_address();
               break;
    }
}

static void leay(void) {    /* Zx */
    switch (step) {
    /* [Don't Care] */
    case 0x21: yr=address;
               res=(res&0x100)|((yr|(yr>>8))&0xff);
               step=0;  /* reset fetch */
               break;
    /* Compute Address */
    default  : compute_address();
               break;
    }
}

static void leas(void) {
    switch (step) {
    /* [Don't Care] */
    case 0x21: sr=address;
               step=0;  /* reset fetch */
               break;
    /* Compute Address */
    default  : compute_address();
               break;
    }
}

static void leau(void) {
    switch (step) {
    /* [Don't Care] */
    case 0x21: ur=address;
               step=0;  /* reset fetch */
               break;
    /* Compute Address */
    default  : compute_address();
               break;
    }
}

/* ========================================================== */
/*                        PSHS/U PULS/U                       */
/* ========================================================== */

static void pshsr(void) {
    sr=(sr-1)&0xffff; /* stack-1 */
    if (value&0x100) {
             if (value&0x80) { StoreByte(sr,pc>>8); value^=0x180; } /* [PC High] */
        else if (value&0x40) { StoreByte(sr,ur>>8); value^=0x140; } /* [U High] */
        else if (value&0x20) { StoreByte(sr,yr>>8); value^=0x120; } /* [Y High] */
        else                 { StoreByte(sr,xr>>8); value^=0x110; } /* [X High] */
    }
    else if (value&0x80) { StoreByte(sr,pc); value|=0x100; } /* [PC Low] */ 
    else if (value&0x40) { StoreByte(sr,ur); value|=0x100; } /* [U Low] */
    else if (value&0x20) { StoreByte(sr,yr); value|=0x100; } /* [Y Low] */
    else if (value&0x10) { StoreByte(sr,xr); value|=0x100; } /* [X Low] */
    else if (value&0x08) { StoreByte(sr,dp); value^=0x08; }  /* [DP] */
    else if (value&0x04) { StoreByte(sr,br); value^=0x04; }  /* [B] */
    else if (value&0x02) { StoreByte(sr,ar); value^=0x02; }  /* [A] */
    else                 { StoreByte(sr,getcc()); value^=0x01; }  /* [CC] */
}

static void pshs(void) {
    switch (step) {
    /* Post Byte : NNNN+1] */
    case 2 : value=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 :
    case 4 : break;
    /* [Don't Care] */
    case 5 : if (value==0) step=0;  /* reset fetch if nothing to push */
             break;
    /* push registers */
    default: pshsr();
             if (value==0) step=0;  /* reset fetch if nothing left to push */
             break;
    }
}

static void pulsr(void) {
    if (value&0x100) {
             if (value&0x10) { xr|=LoadByte(sr); value^=0x110; } /* [X Low] */
        else if (value&0x20) { yr|=LoadByte(sr); value^=0x120; } /* [Y Low] */
        else if (value&0x40) { ur|=LoadByte(sr); value^=0x140; } /* [U Low] */
        else                 { pc|=LoadByte(sr); value^=0x180; } /* [PC Low] */
    }
    else if (value&0x01) { setcc(LoadByte(sr)); value^=0x01; } /* [CC] */
    else if (value&0x02) { ar=LoadByte(sr); value^=0x02; }     /* [A] */
    else if (value&0x04) { br=LoadByte(sr); value^=0x04; }     /* [B] */
    else if (value&0x08) { dp=LoadByte(sr); value^=0x08; }     /* [DP] */
    else if (value&0x10) { xr=LoadByte(sr)<<8; value|=0x100; } /* [X High] */
    else if (value&0x20) { yr=LoadByte(sr)<<8; value|=0x100; } /* [Y High] */
    else if (value&0x40) { ur=LoadByte(sr)<<8; value|=0x100; } /* [U High] */
    else                 { pc=LoadByte(sr)<<8; value|=0x100; } /* [PC High] */
    sr=(sr+1)&0xffff; /* stack+1 */
}

static void puls(void) {
    switch (step) {
    /* Post Byte : NNNN+1] */
    case 2 : value=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : break;
    /* [Don't Care] */
    case 4 : if (value==0) step=0x20;  /* skip if nothing to pull */
             break;
    /* [Don't Care] */
    case 0x21 : step=0;  /* reset fetch */
                break;
    /* pull registers */
    default: pulsr();
             if (value==0) step=0x20; /* skip if nothing left to pull */
             break;
    }
}

static void pshu(void) {
    switch (step) {
    /* Post Byte : NNNN+1] */
    case 2 : value=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 :
    case 4 :  break;
    /* [Don't Care] */
    case 5 :  if (value==0) step=0;  /* reset fetch if nothing to push */
              break;
    /* push registers */
    default:
        ur=(ur-1)&0xffff;  /* stack-1 */
        if (value&0x100) {
                 if (value&0x80) { StoreByte(ur,pc>>8); value^=0x180; } /* [PC High] */
            else if (value&0x40) { StoreByte(ur,sr>>8); value^=0x140; } /* [S High] */
            else if (value&0x20) { StoreByte(ur,yr>>8); value^=0x120; } /* [Y High] */
            else                 { StoreByte(ur,xr>>8); value^=0x110; } /* [X High] */
        }
        else if (value&0x80) { StoreByte(ur,pc); value|=0x100; } /* [PC Low] */
        else if (value&0x40) { StoreByte(ur,sr); value|=0x100; } /* [S Low] */
        else if (value&0x20) { StoreByte(ur,yr); value|=0x100; } /* [Y Low] */
        else if (value&0x10) { StoreByte(ur,xr); value|=0x100; } /* [X Low] */
        else if (value&0x08) { StoreByte(ur,dp); value^=0x08; }  /* [DP] */
        else if (value&0x04) { StoreByte(ur,br); value^=0x04; }  /* [B] */
        else if (value&0x02) { StoreByte(ur,ar); value^=0x02; }  /* [A] */
        else                 { StoreByte(ur,getcc()); value^=0x01; } /* [CC] */
        if (value==0) step=0;  /* reset fetch if nothing left to push */
        break;
    }
}

static void pulu(void) {
    switch (step) {
    /* [Post Byte : NNNN+1] */
    case 2 : postcode=LoadByte(pc);
             pc=(pc+1)&0xffff;
             value=postcode;
             break;
    /* [Don't Care] */
    case 3 : break;
    /* [Don't Care] */
    case 4 : if (value==0) step=0x21;  /* skip if nothing to pull */
             break;
    /* [Don't Care] */
    case 0x21 : step=0;  /* reset fetch */
                break;
    /* pull registers */
    default:
        if (value&0x100) {
                 if (value&0x10) { xr|=LoadByte(ur); value^=0x110; } /* [X Low] */
            else if (value&0x20) { yr|=LoadByte(ur); value^=0x120; } /* [Y Low] */
            else if (value&0x40) { sr|=LoadByte(ur); value^=0x140; } /* [S Low] */
            else                 { pc|=LoadByte(ur); value^=0x180; } /* [PC Low] */
        }
        else if (value&0x01) { setcc(LoadByte(ur)); value^=0x01; } /* [CC] */
        else if (value&0x02) { ar=LoadByte(ur); value^=0x02; }     /* [A] */
        else if (value&0x04) { br=LoadByte(ur); value^=0x04; }     /* [B] */
        else if (value&0x08) { dp=LoadByte(ur); value^=0x08; }     /* [DP] */
        else if (value&0x10) { xr=LoadByte(ur)<<8; value|=0x100; } /* [X High] */
        else if (value&0x20) { yr=LoadByte(ur)<<8; value|=0x100; } /* [Y High] */
        else if (value&0x40) { sr=LoadByte(ur)<<8; value|=0x100; } /* [S High] */
        else                 { pc=LoadByte(ur)<<8; value|=0x100; } /* [PC High] */
        ur=(ur+1)&0xffff;  /* stack+1 */
        if (value==0) step=0x20;  /* skip if nothing left to pull */
        break;
    }
}

/* ========================================================== */
/*                            RTI                             */
/* ========================================================== */

static void rtim(void) {
    switch (step) {
    /* [Don't Care : NNNN+1] */
    case 2 : break;
    /* CCR : Stack] */
    case 3 : value=0x01;
             pulsr();
             value=(ccrest&0x80)?0xfe:0x80;
             break;
    /* [Don't Care : Stack] */
    case 0x21 : irq_run=0;
                step=0;  /* reset fetch */
                break;
    /* pull registers */
    default: pulsr();
             if (value==0) step=0x20;  /* skip if nothing left to pull */
             break;
    }
}

/* ========================================================== */
/*                         NOP DAA SEX                        */
/* ========================================================== */

static void nopm(void) {
    /* [Don't Care : NNNN+1] */
    step=0;  /* reset fetch */
}

static void daam(void) {    /* NxZxV?Cx */
        int i=ar+(res&0x100);
    /* [Don't Care : NNNN+1] */
    if (((ar&15)>9)||((h1&15)+(h2&15)>15)) i+=6;
    if (i>0x99) i+=0x60;
    res=sign=i;
    ar=i&0xff;
    step=0;  /* reset fetch */
}

static void sexm(void) {    /* NxZx */
    /* [Don't Care : NNNN+1] */
    ar=(br&0x80)?0xff:0;
    sign=br;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

/* ========================================================== */
/*  ASLA/B ASRA/B CLRA/B COMA/B DECA/B INCA/B LSRA/B NEGA/B   */
/*                     ROLA/B RORA/B TSTA/B                   */
/* ========================================================== */

static void asla(void) {    /* H?NxZxVxCx */
    /* [Don't Care : NNNN+1] */
    m1=m2=ar;
    ar<<=1;
    ovfl=sign=res=ar;
    ar&=0xff;
    step=0;  /* reset fetch */
}

static void aslb(void) {    /* H?NxZxVxCx */
    /* [Don't Care : NNNN+1] */
    m1=m2=br;
    br<<=1;
    ovfl=sign=res=br;
    br&=0xff;
    step=0;  /* reset fetch */
}

static void asra(void) {    /* H?NxZxCx */
    /* [Don't Care : NNNN+1] */
    res=(ar&1)<<8;
    ar=(ar>>1)|(ar&0x80);
    sign=ar;
    res|=sign;
    step=0;  /* reset fetch */
}

static void asrb(void) {    /* H?NxZxCx */
    /* [Don't Care : NNNN+1] */
    res=(br&1)<<8;
    br=(br>>1)|(br&0x80);
    sign=br;
    res|=sign;
    step=0;  /* reset fetch */
}

static void clra(void) {    /* N0Z1V0C0 */
    /* [Don't Care : NNNN+1] */
    ar=0;
    m1=ovfl;
    sign=res=0;
    step=0;  /* reset fetch */
}

static void clrb(void) {    /* N0Z1V0C0 */
    /* [Don't Care : NNNN+1] */
    br=0;
    m1=ovfl;
    sign=res=0;
    step=0;  /* reset fetch */
}

static void coma(void) {    /* NxZxV0C1 */
    /* [Don't Care : NNNN+1] */
    m1=ovfl;
    ar^=0xff;
    sign=ar;
    res=sign|0x100; /* bit C a 1 */
    step=0;  /* reset fetch */
}

static void comb(void) {    /* NxZxV0C1 */
    /* [Don't Care : NNNN+1] */
    m1=ovfl;
    br^=0xff;
    sign=br;
    res=sign|0x100; /* bit C a 1 */
    step=0;  /* reset fetch */
}

static void deca(void) {    /* NxZxVx */
    /* [Don't Care : NNNN+1] */
    m1=ar; m2=0x80;
    ar=(ar-1)&0xff;
    ovfl=sign=ar;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

static void decb(void) {    /* NxZxVx */
    /* [Don't Care : NNNN+1] */
    m1=br; m2=0x80;
    br=(br-1)&0xff;
    ovfl=sign=br;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

static void inca(void) {    /* NxZxVx */
    /* [Don't Care : NNNN+1] */
    m1=ar; m2=0;
    ar=(ar+1)&0xff;
    ovfl=sign=ar;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

static void incb(void) {    /* NxZxVx */
    /* [Don't Care : NNNN+1] */
    m1=br; m2=0;
    br=(br+1)&0xff;
    ovfl=sign=br;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

static void lsra(void) {    /* N0ZxCx */
    /* [Don't Care : NNNN+1] */
    res=(ar&1)<<8;  /* bit C */
    ar>>=1;
    sign=0;
    res|=ar;
    step=0;  /* reset fetch */
}

static void lsrb(void) {    /* N0ZxCx */
    /* [Don't Care : NNNN+1] */
    res=(br&1)<<8;  /* bit C */
    br>>=1;
    sign=0;
    res|=br;
    step=0;  /* reset fetch */
}

static void nega(void) {    /* H?NxZxVxCx */
    /* [Don't Care : NNNN+1] */
    m1=ar; m2=-ar;  /* bit V */
    ar=-ar;
    ovfl=res=sign=ar;
    ar&=0xff;
    step=0;  /* reset fetch */
}

static void negb(void) {    /* H?NxZxVxCx */
    /* [Don't Care : NNNN+1] */
    m1=br; m2=-br;          /* bit V */
    br=-br;
    ovfl=res=sign=br;
    br&=0xff;
    step=0;  /* reset fetch */
}

static void rola(void) {    /* NxZxVxCx */
    /* [Don't Care : NNNN+1] */
    m1=m2=ar;
    ar=(ar<<1)|((res&0x100)>>8);
    ovfl=sign=res=ar;
    ar&=0xff;
    step=0;  /* reset fetch */
}

static void rolb(void) {    /* NxZxVxCx */
    /* [Don't Care : NNNN+1] */
    m1=m2=br;
    br=(br<<1)|((res&0x100)>>8);
    ovfl=sign=res=br;
    br&=0xff;
    step=0;  /* reset fetch */
}

static void rora(void) {    /* NxZxCx */
    /* [Don't Care : NNNN+1] */
    sign=(ar|(res&0x100))>>1;
    res=((ar&1)<<8)|sign;
    ar=sign;
    step=0;  /* reset fetch */
}

static void rorb(void) {    /* NxZxCx */
    /* [Don't Care : NNNN+1] */
    sign=(br|(res&0x100))>>1;
    res=((br&1)<<8)|sign;
    br=sign;
    step=0;  /* reset fetch */
}

static void tsta(void) {    /* NxZxV0 */
    /* [Don't Care : NNNN+1] */
    m1=ovfl;
    sign=ar;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

static void tstb(void) {    /* NxZxV0 */
    /* [Don't Care : NNNN+1] */
    m1=ovfl;
    sign=br;
    res=(res&0x100)|sign;
    step=0;  /* reset fetch */
}

/* ========================================================== */
/*                        ORCC ANDCC                          */
/* ========================================================== */

static void orcc(void) {
    switch (step) {
    /* [data : NNNN+1] */
    case 2 : value=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : setcc(getcc()|value);
             step=0;  /* reset fetch */
             break;
    }
}

static void andc(void) {
    switch (step) {
    /* [data : NNNN+1] */
    case 2 : value=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 3 : setcc(getcc()&value);
             step=0;  /* reset fetch */
             break;
    }
}

/* ========================================================== */
/*                            ABX                             */
/* ========================================================== */

static void abxm(void) {
    switch (step) {
    /* [Don't Care] */
    case 3 : xr=(xr+br)&0xffff;
             step=0;  /* reset fetch */
             break;
    /* Case 2 = [Don't Care] */
    default: break;
    }
}

/* ========================================================== */
/*                            RTS                             */
/* ========================================================== */

static void rtsm(void) {
    switch (step) {
    /* PC High : Stack */
    case 3 : pc=LoadByte(sr)<<8;
             sr=(sr+1)&0xffff;
             break;
    /* PC Low : Stack */
    case 4 : pc|=LoadByte(sr);
             sr=(sr+1)&0xffff;
             break;
    /* Cases 5 = [Don't Care] */
    case 5 : step=0;  /* reset fetch */
             break;
    /* Case 2 = [Don't Care] */
    default: break;
    }
}

/* ========================================================== */
/*                            MUL                             */
/* ========================================================== */

static void mulm(void) {    /* ZxCx */ 
    switch (step) {
    /* [Don't Care] */
    case 11 : value=ar*br;
              ar=(value>>8)&0xff;
              br=value&0xff;
              res=((br&0x80)<<1)|ar|br;  /* c=bit7 de br */
              step=0;  /* reset fetch */
              break;
    /* Case 2->10 = [Don't Care] */
    default : break;
    }
}

/* ========================================================== */
/*                    Software interrupts                     */
/*                       SWI SWI2 SWI3                        */
/* ========================================================== */

static void swim(void) {
    switch (step) {
    /* [Don't Care] */
    case 2  : ccrest|=0x80;
              value=0xff;  /* push all registers */
              break;
    /* [Don't Care] */
    case 3  : break;
    /* [Don't Care] */
    case 16 : ccrest|=0x50;  /* F,I masked */
              break;
    /* [Interrupt Vector High : FFFX] */
    case 17 : pc=LoadByte(swi_vector[page>>8])<<8;
              break;
    /* [Interrupt Vector Low : FFFX+1] */
    case 18 : pc|=LoadByte(swi_vector[page>>8]+1);
              break;
    /* [Don't Care] */
    case 19 : step=0;  /* reset fetch */
              break;
    /* case 4->15 = push registers */
    default : pshsr();
              break;
    }
}

/* ========================================================== */
/*   ADCA/B ADDA/B ANDA/B BITA/B CMPA/B EORA/B LDA/B ORA/B    */
/*                       SBCA/B SUBA/B                        */
/* ========================================================== */
/* Code is written without 'switch' or 'else' because of      */
/* immediate addressing mode : all must be done in 2 cycles.  */

static void adca(void) {    /* HxNxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=h1=ar; m2=value;
        h2=value+((res&0x100)>>8);
        ar+=h2;
        ovfl=res=sign=ar;
        ar&=0xff;
        step=0;  /* reset fetch */
    }
}

static void adcb(void) {    /* HxNxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=h1=br; m2=value;
        h2=value+((res&0x100)>>8);
        br+=h2;
        ovfl=res=sign=br;
        br&=0xff;
        step=0;  /* reset fetch */
    }
}

static void adda(void) {    /* HxNxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=h1=ar; m2=h2=value;
        ar+=value;
        ovfl=res=sign=ar;
        ar&=0xff;
        step=0;  /* reset fetch */
    }
}

static void addb(void) {    /* HxNxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=h1=br; m2=h2=value;
        br+=value;
        ovfl=res=sign=br;
        br&=0xff;
        step=0;  /* reset fetch */
    }
}

static void anda(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        ar&=value;
        sign=ar;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void andb(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        br&=value;
        sign=br;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void bita(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        sign=ar&value;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void bitb(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        sign=br&value;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void cmpa(void) {    /* H?NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ar; m2=-value;
        ovfl=res=sign=ar-value;
        step=0;  /* reset fetch */
    }
}

static void cmpb(void) {    /* H?NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=br; m2=-value;
        ovfl=res=sign=br-value;
        step=0;  /* reset fetch */
    }
}

static void eora(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        ar^=value;
        sign=ar;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void eorb(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        br^=value;
        sign=br;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void ldam(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        sign=ar=LoadByte(address);
        m1=ovfl;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void ldbm(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        sign=br=LoadByte(address);
        m1=ovfl;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void oram(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        ar|=value;
        sign=ar;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void orbm(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ovfl;
        br|=value;
        sign=br;
        res=(res&0x100)|sign;
        step=0;  /* reset fetch */
    }
}

static void sbca(void) {    /* H?NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ar; m2=-value;
        ar-=value+((res&0x100)>>8);
        ovfl=res=sign=ar;
        ar&=0xff;
        step=0;  /* reset fetch */
    }
}

static void sbcb(void) {    /* H?NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=br; m2=-value;
        br-=value+((res&0x100)>>8);
        ovfl=res=sign=br;
        br&=0xff;
        step=0;  /* reset fetch */
    }
}

static void suba(void) {    /* H?NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=ar; m2=-value;
        ar-=value;
        ovfl=res=sign=ar;
        ar&=0xff;
        step=0;  /* reset fetch */
    }
}

static void subb(void) {    /* H?NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    /* [Data : EA] */
    if (step==0x21) {
        value=LoadByte(address);
        m1=br; m2=-value;
        br-=value;
        ovfl=res=sign=br;
        br&=0xff;
        step=0;  /* reset fetch */
    }
}

/* ========================================================== */
/*               STA STB STD STX STY STU STS                  */
/* ========================================================== */

static void stam(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register (Write) : EA] */
    case 0x21 : StoreByte(address,ar);
                sign=ar;
                m1=ovfl;
                res=(res&0x100)|sign;
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

static void stbm(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register (Write) : EA] */
    case 0x21 : StoreByte(address,br);
                sign=br;
                m1=ovfl;
                res=(res&0x100)|sign;
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

static void stdm(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register High (Write) : EA] */
    case 0x21 : StoreByte(address,ar);
                break;
    /* [Register Low (Write) : EA] */
    case 0x22 : StoreByte(address+1,br);
                m1=ovfl;
                sign=ar;
                res=(res&0x100)|ar|br;
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

static void stxm(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register High (Write) : EA] */
    case 0x21 : StoreByte(address,xr>>8);
                break;
    /* [Register Low (Write) : EA] */
    case 0x22 : StoreByte(address+1,xr);
                m1=0; m2=0x80;
                sign=xr>>8;
                res=(res&0x100)|((sign|xr)&0xff);
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

static void stum(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register High (Write) : EA] */
    case 0x21 : StoreByte(address,ur>>8);
                break;
    /* [Register Low (Write) : EA] */
    case 0x22 : StoreByte(address+1,ur);
                m1=ovfl;
                sign=ur>>8;
                res=(res&0x100)|((sign|ur)&0xff);
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

static void stym(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register High (Write) : EA] */
    case 0x21 : StoreByte(address,yr>>8);
                break;
    /* [Register Low (Write) : EA] */
    case 0x22 : StoreByte(address+1,yr);
                m1=ovfl;
                sign=yr>>8;
                res=(res&0x100)|((sign|yr)&0xff);
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

static void stsm(void) {    /* NxZxV0 */
    switch (step) {
    /* [Register High (Write) : EA] */
    case 0x21 : StoreByte(address,sr>>8);
                break;
    /* [Register Low (Write) : EA] */
    case 0x22 : StoreByte(address+1,sr);
                m1=ovfl;
                sign=sr>>8;
                res=(res&0x100)|((sign|sr)&0xff);
                step=0;  /* reset fetch */
                break;
    /* compute address */
    default   : compute_address();
                break;
    }
}

/* ========================================================== */
/*                         LDD/X/Y/U/S                        */
/* ========================================================== */

static void lddm(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Register High : EA] */
    case 0x21 : ar=LoadByte(address);
                break;
    /* [Register Low : EA] */
    case 0x22 : br=LoadByte(address+1);
                m1=ovfl;
                sign=ar;
                res=(res&0x100)|br|ar;
                step=0;  /* reset fetch */
                break;
    }
}

static void ldxm(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Register High : EA] */
    case 0x21 : xr=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : xr|=LoadByte(address+1);
                m1=ovfl;
                sign=xr>>8;
                res=(res&0x100)|((sign|xr)&0xff);
                step=0;  /* reset fetch */
                break;
    }
}

static void ldym(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Register High : EA] */
    case 0x21 : yr=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : yr|=LoadByte(address+1);
                m1=ovfl;
                sign=yr>>8;
                res=(res&0x100)|((sign|yr)&0xff);
                step=0;  /* reset fetch */
                break;
    }
}

static void ldum(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Register High : EA] */
    case 0x21 : ur=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : ur|=LoadByte(address+1);
                m1=ovfl;
                sign=ur>>8;
                res=(res&0x100)|((sign|ur)&0xff);
                step=0;  /* reset fetch */
                break;
    }
}

static void ldsm(void) {    /* NxZxV0 */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Register High : EA] */
    case 0x21 : sr=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : sr|=LoadByte(address+1);
                m1=ovfl;
                sign=sr>>8;
                res=(res&0x100)|((sign|sr)&0xff);
                step=0;  /* reset fetch */
                break;
    }
}

/* ========================================================== */
/*                         CMPD/X/Y/U/S                       */
/* ========================================================== */

static void cmpd(void) {            /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : dr=(ar<<8)+br-value;
                m1=ar; m2=(-value)>>8;
                ovfl=res=sign=dr>>8;
                res|=(dr&0xff);
                step=0;  /* reset fetch */
                break;
    }
}

static void cmpx(void) {    /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : m1=xr>>8; m2=(-value)>>8;
                ovfl=res=sign=(xr-value)>>8;
                res|=(xr-value)&0xff;
                step=0;  /* reset fetch */
                break;
    }
}

static void cmpy(void) {            /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : m1=yr>>8; m2=(-value)>>8;
                ovfl=res=sign=(yr-value)>>8;
                res|=(yr-value)&0xff;
                step=0;  /* reset fetch */
                break;
    }
}

static void cmpu(void) {            /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : m1=ur>>8; m2=(-value)>>8;
                ovfl=res=sign=(ur-value)>>8;
                res|=(ur-value)&0xff;
                step=0;  /* reset fetch */
                break;
    }
}

static void cmps(void) {            /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : m1=sr>>8; m2=(-value)>>8;
                ovfl=res=sign=(sr-value)>>8;
                res|=(sr-value)&0xff;
                step=0;  /* reset fetch */
                break;
    }
}

static void addd(void) {    /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : dr=(ar<<8)+br+value;
                m1=ar; m2=value>>8;
                ar=dr>>8;
                br=dr&0xff;
                ovfl=res=sign=ar;
                res|=br;
                ar&=0xff;
                step=0;  /* reset fetch */
                break;
    }
}

static void subd(void) {    /* NxZxVxCx */
    /* compute address */
    if (step<0x21) compute_address();
    switch (step) {
    /* [Data High : EA] */
    case 0x21 : value=LoadByte(address)<<8;
                break;
    /* [Register Low : EA] */
    case 0x22 : value|=LoadByte(address+1);
                break;
    /* [Don't Care] */
    case 0x23 : dr=(ar<<8)+br-value;
                m1=ar; m2=(-value)>>8;
                ar=dr>>8;
                br=dr&0xff;
                ovfl=res=sign=ar;
                res|=br;
                ar&=0xff;
                step=0;  /* reset fetch */
                break;
    }
}

static void synm(void) {
        /* non supporté */
}

static void cwai(void) {
        /* non supporté */
}

/**********************************************/
/*** instructions non documentées du MC6809 ***/
/**********************************************/

static void cd02(void) {  /* NEG if Carry=0, COM otherwise */
     if (res&0x100)
         comm();
     else
         negm();
}

static void cd10(void) { };
static void cd11(void) { };

static void hcfm(void) { /* Halt and Catch Fire */
    /* not supported */
}

static void rset(void) { /* Reset */
    dp=0;
    ccrest|=0x50;
    pc=LoadWord(0xFFFE);
    mc6809_irq=0;
    irq_start=irq_run=0;
    step=0;
}

static void cd18(void) {
    switch (step) {
    /* [Don't Care] */
    case 3 : res=1-((((~(m1^m2))&(m1^ovfl))&0x80)>>7);  /* V -> Z */
             h1=h2=((ccrest&0x10)>>4)*15;               /* I -> H */
             ccrest=sign=m1=m2=ovfl=0;  /* others bits of CC to 0 */
             step=0;  /* reset fetch */
             break;
    /* Case 2 = [Don't Care] */
    default: break;
    }
}

static void cd38(void) {  /* ANDCC 4 cycles */
    switch (step) {
    /* [data : NNNN+1] */
    case 2 : value=LoadByte(pc);
             pc=(pc+1)&0xffff;
             break;
    /* [Don't Care] */
    case 4 : setcc(getcc()&value);
             step=0;  /* reset fetch */
             break;
    /* Case 3 = [Don't Care] */
    default: break;
    }
}

static void cd42(void) {  /* NEGA/COMA */
    if (res&0x100)
        coma();
    else
        nega();
}

static void cd52(void) {  /* NEGB/COMB */
    if (res&0x100)
        comb();
    else
        negb();
}

static void cd87(void) {
    /* [Don't Care] */
    m1=m2=ovfl=0; /* V=0 */
    res&=0xff00;  /* Z=0 */
    sign|=0x80;   /* N=1   */
    step=0;  /* reset fetch */
}

static void cd8f(void) {
    switch (step) {
    /* [Don't Care : NNNN+1] */
    case 2 : pc=(pc+1)&0xffff;
             break;
    /* [Register Low (write) : NNNN+2] */
    case 3 : StoreByte(pc,xr);
             pc=(pc+1)&0xffff;
             m1=m2=ovfl=0; /* V=0 */
             res&=0xff00;  /* Z=0 */
             sign|=0x80;   /* N=1   */
             step=0;  /* reset fetch */
             break;
    }
}
             
static void cdcf(void) {
    switch (step) {
    /* [Don't Care : NNNN+1] */
    case 2 : pc=(pc+1)&0xffff;
             break;
    /* [Register Low (write) : NNNN+2] */
    case 3 : StoreByte(pc,ur);
             pc=(pc+1)&0xffff;
             m1=m2=ovfl=0; /* V=0 */
             res&=0xff00;  /* Z=0 */
             sign|=0x80;   /* N=1   */
             step=0;  /* reset fetch */
             break;
    }
}

/*******************************/
/*** systèmes d'interruption ***/
/*******************************/

static void do_nmi(void) {
    switch (step) {
    /* [Don't Care] */
    case 1    : ccrest|=0x80;
                value=0xff;      /* push all registers */
                break;
    /* [Don't Care] */
    case 2    :
    case 3    : break;
    /* [Don't Care] */
    case 0x21 : ccrest|=0x50;
                break;
    /* [PC High : FFFX] */
    case 0x22 : pc=LoadByte(0xFFFC)<<8;
                break;
    /* [PC Low : FFFX+1] */
    case 0x23 : pc|=LoadByte(0xFFFD);
                break;
    /* [Don't Care] */
    case 0x24 : step=0;  /* reset fetch */
                break;
    /* push all registers */
    default   : pshsr();
                if (value==0) step=0x20;
                break;
    }
}

static void do_irq(void) {
    switch (step) {
    /* [Don't Care] */
    case 1    : ccrest|=0x80;
                value=0xff;      /* push all registers */
                break;
    /* [Don't Care] */
    case 2    :
    case 3    : break;
    /* [Don't Care] */
    case 0x21 : ccrest|=0x10;
                break;
    /* [PC High : FFFX] */
    case 0x22 : pc=LoadByte(0xFFF8)<<8;
                break;
    /* [PC Low : FFFX+1] */
    case 0x23 : pc|=LoadByte(0xFFF9);
                break;
    /* [Don't Care] */
    case 0x24 : irq_start=0;
                step=0;  /* reset fetch */
                break;
    /* push all registers */
    default   : pshsr();
                if (value==0) step=0x20;
                break;
    }
}

static void do_firq(void) {
    switch (step) {
    /* [Don't Care] */
    case 1 : ccrest&=0x7f;
             value=0x81;
             break;
    /* push PC,CC */
    case 4 :
    case 5 :
    case 6 : pshsr();
             break;
    /* [Don't Care] */
    case 7 : ccrest|=0x50;
             break;
    /* [PC High : FFFX] */
    case 8 : pc=LoadByte(0xFFF6)<<8;
             break;
    /* [PC Low : FFFX+1] */
    case 9 : pc|=LoadByte(0xFFF7);
             break;
    /* [Don't Care] */
    case 10: step=0;  /* reset fetch */
             break;
    /* Cases 2,3 = [Don't Care] */
    default: break;
    }
}


static void (*code[])(void)=
{negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,cd10,cd11,nopm,synm,hcfm,hcfm,lbra,lbsr,cd18,daam,orcc,nopm,andc,sexm,exgm,tfrm
,bras,brns,bhis,blss,bccs,blos,bnes,beqs,bvcs,bvss,bpls,bmis,bges,blts,bgts,bles
,leax,leay,leas,leau,pshs,puls,pshu,pulu,cd38,rtsm,abxm,rtim,cwai,mulm,rset,swim
,nega,nega,cd42,coma,lsra,lsra,rora,asra,asla,rola,deca,deca,inca,tsta,clra,clra
,negb,negb,cd52,comb,lsrb,lsrb,rorb,asrb,aslb,rolb,decb,decb,incb,tstb,clrb,clrb
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,suba,cmpa,sbca,subd,anda,bita,ldam,cd87,eora,adca,oram,adda,cmpx,bsrm,ldxm,cd8f
,suba,cmpa,sbca,subd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpx,jsrm,ldxm,stxm
,suba,cmpa,sbca,subd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpx,jsrm,ldxm,stxm
,suba,cmpa,sbca,subd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpx,jsrm,ldxm,stxm
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,cd87,eorb,adcb,orbm,addb,lddm,hcfm,ldum,cdcf
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
/* page 2 */
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,cd10,cd11,nopm,synm,hcfm,hcfm,lbra,lbsr,cd18,daam,orcc,nopm,andc,sexm,exgm,tfrm
,lbra,lbrn,lbhi,lbls,lbcc,lblo,lbne,lbeq,lbvc,lbvs,lbpl,lbmi,lbge,lblt,lbgt,lble
,leax,leay,leas,leau,pshs,puls,pshu,pulu,cd38,rtsm,abxm,rtim,cwai,mulm,rset,swim
,nega,nega,cd42,coma,lsra,lsra,rora,asra,asla,rola,deca,deca,inca,tsta,clra,clra
,negb,negb,cd52,comb,lsrb,lsrb,rorb,asrb,aslb,rolb,decb,decb,incb,tstb,clrb,clrb
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,suba,cmpa,sbca,cmpd,anda,bita,ldam,cd87,eora,adca,oram,adda,cmpy,bsrm,ldym,cd8f
,suba,cmpa,sbca,cmpd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpy,jsrm,ldym,stym
,suba,cmpa,sbca,cmpd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpy,jsrm,ldym,stym
,suba,cmpa,sbca,cmpd,anda,bita,ldam,stam,eora,adca,oram,adda,cmpy,jsrm,ldym,stym
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,cd87,eorb,adcb,orbm,addb,lddm,hcfm,ldsm,cdcf
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldsm,stsm
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldsm,stsm
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldsm,stsm
/* page 3 */
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,cd10,cd11,nopm,synm,hcfm,hcfm,lbra,lbsr,cd18,daam,orcc,nopm,andc,sexm,exgm,tfrm
,bras,brns,bhis,blss,bccs,blos,bnes,beqs,bvcs,bvss,bpls,bmis,bges,blts,bgts,bles
,leax,leay,leas,leau,pshs,puls,pshu,pulu,cd38,rtsm,abxm,rtim,cwai,mulm,rset,swim
,nega,nega,cd42,coma,lsra,lsra,rora,asra,asla,rola,deca,deca,inca,tsta,clra,clra
,negb,negb,cd52,comb,lsrb,lsrb,rorb,asrb,aslb,rolb,decb,decb,incb,tstb,clrb,clrb
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,negm,negm,cd02,comm,lsrm,lsrm,rorm,asrm,aslm,rolm,decm,decm,incm,tstm,jmpm,clrm
,suba,cmpa,sbca,cmpu,anda,bita,ldam,cd87,eora,adca,oram,adda,cmps,bsrm,ldxm,cd8f
,suba,cmpa,sbca,cmpu,anda,bita,ldam,stam,eora,adca,oram,adda,cmps,jsrm,ldxm,stxm
,suba,cmpa,sbca,cmpu,anda,bita,ldam,stam,eora,adca,oram,adda,cmps,jsrm,ldxm,stxm
,suba,cmpa,sbca,cmpu,anda,bita,ldam,stam,eora,adca,oram,adda,cmps,jsrm,ldxm,stxm
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,cd87,eorb,adcb,orbm,addb,lddm,hcfm,ldum,cdcf
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
,subb,cmpb,sbcb,addd,andb,bitb,ldbm,stbm,eorb,adcb,orbm,addb,lddm,stdm,ldum,stum
/* interrupts */
,do_irq,do_firq,do_nmi
};

static void (*addr[])(void)=
{drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct
,impl,impl,impl,impl,impl,impl,rela,rela,impl,impl,imm1,imm1,imm1,impl,impl,impl
,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela,rela
,indx,indx,indx,indx,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl
,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl
,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl,impl
,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx
,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn
,imm1,imm1,imm1,imm2,imm1,imm1,imm1,imm1,imm1,imm1,imm1,imm1,imm2,rela,imm2,imm2
,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct
,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx
,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn
,imm1,imm1,imm1,imm2,imm1,imm1,imm1,imm1,imm1,imm1,imm1,imm1,imm2,rela,imm2,imm2
,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct,drct
,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx,indx
,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn,extn
};


static void mc6809_Step(void) {
    if (step==1) {     /* [Fetch OpCode] */
        if (!page) {
#ifdef DEBUG
            if (mc6809_ftrace)
                fprintf(mc6809_ftrace, "pc: %04X\n", pc);
#endif
            if (cpu_clock>=cpu_timer)
                TimerCallback(timer_data);

            if ((mc6809_irq!=0)&&((ccrest&0x10)==0)) {
                irq_start=irq_run=1;
                opcode=0x300;
                do_irq();
            }
        }
        if (!irq_start) {
            opcode=LoadByte(pc);
            pc=(pc+1)&0xffff;
            if(opcode==TO8_TRAP_CODE) {
                mc6809_GetRegs(&reg_list);
                opcode=TrapCallback(&reg_list);
                mc6809_SetRegs(&reg_list, 0x1FF);
            }
        }
        switch(opcode) {
        case 0x10 : page=0x100; step=0; break;
        case 0x11 : page=0x200; step=0; break;
        default   : compute_address=addr[opcode&0xff]; break;
        }
    } else {
        (*(code[opcode+page]))();
        if (step==0)  /* fetch reset */
            page=0;
    }        
}


/************************************************/
/*** Interface publique de l'émulateur MC6809 ***/
/************************************************/

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
        exreg[i] = &bus;

    exreg[1] =  &xr;
    exreg[2] =  &yr;
    exreg[3] =  &ur;
    exreg[4] =  &sr;
    exreg[5] =  &pc;
    exreg[8] =  &ar;
    exreg[9] =  &br;
    exreg[11] = &dp;

    page      = 0;
    step      = 1;
    irq_start = 0;
    irq_run   = 0;
    cpu_clock = 0;
    cpu_timer = MC6809_TIMER_DISABLED;

    FetchInstr    = interface->FetchInstr;
    LoadByte      = interface->LoadByte;
    LoadWord      = interface->LoadWord;
    StoreByte     = interface->StoreByte;
    StoreWord     = interface->StoreWord;
    TrapCallback  = interface->TrapCallback;
}


/* Reset:
 *  Remet à zéro le CPU (envoie un signal sur la broche RESET du MC6809).
 */
void mc6809_Reset(void)
{
    rset();
    step=1;
}


/* FlushExec:
 * Achève l'exécution d'une instruction et/ou
 * d'une interruption
 */
void mc6809_FlushExec(void)
{
    while ((step!=1)||(irq_run!=0))
    {
        mc6809_Step();
        step++;     
        cpu_clock++;
    }
}


/* StepExec:
 *  Exécute un nombre donné d'instructions et retourne le
 *  nombre de cycles nécessaires à leur éxécution.
 */
int mc6809_StepExec(unsigned int ninst)
{
    mc6809_clock_t start_clock=cpu_clock;
    register unsigned int i=0;

    while (i!=ninst)
    {
        mc6809_Step();
        if ((step==0)&&(opcode<0x300)) i++;
        step++;     
        cpu_clock++;
    }
    return cpu_clock-start_clock;
}


/* TimeExec:
 *  Fait tourner le MC6809 jusqu'à un instant donné
 */
void mc6809_TimeExec(mc6809_clock_t time_limit)
{
    while (cpu_clock<time_limit)
    {
        mc6809_Step();
        step++;
        cpu_clock++;
    }
}


/* TimeExec_debug:
 *  Fait tourner le MC6809 jusqu'à un instant donné et
 *  retourne le nombre d'instructions éxécutées.
 */
int mc6809_TimeExec_debug(mc6809_clock_t time_limit)
{
    int ninst=0;

    while (cpu_clock<time_limit)
    {
        mc6809_Step();
        if ((step==0)&&(opcode<0x300)) ninst++;
        step++;     
        cpu_clock++;        
#ifdef OS_LINUX
	if (udebug_Breakpoint (pc&0xFFFF)) return -1;
#endif
    }
    return ninst;
}

