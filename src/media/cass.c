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
 *  Module     : cass.c
 *  Version    : 1.8.2
 *  Cr�� par   : Eric Botcazou avril 1999
 *  Modifi� par: Eric Botcazou 28/10/2003
 *               Samuel Devulder 05/02/2012
 *               Fran�ois Mouret 25/04/2012 29/09/2012
 *
 *  Gestion des cassettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "intern/errors.h"
#include "intern/hardware.h"
#include "intern/std.h"
#include "intern/cass.h"  /* MacOS */
#include "to8.h"


#define COUNTER_RATIO   100

static FILE *cass;
static int cass_mode;
static int cass_counter;
static enum {
   READ,
   WRITE
} current_op;



/* DoLoadCass:
 *  Ouvre le fichier et retourne le mode d'ouverture.
 */
static int DoLoadCass(const char filename[], int mode)
{
   FILE *new_cass;

   if (mode == TO8_READ_WRITE) {
      if ((new_cass=fopen(filename, "rb+")) != NULL)
	 goto Success;
      else
	 mode = TO8_READ_ONLY;
   }

   if ((new_cass=fopen(filename, "rb")) != NULL)
      goto Success;

   return ErrorMessage(TO8_CANNOT_OPEN_FILE, NULL);

 Success:
   if (cass)
      fclose(cass);

   cass = new_cass;
   cass_counter = -1;  /* position du fichier modifi�e */

   return mode;
}



/* DoCassStuff:
 *  Emule le contr�leur du lecteur de cassettes.
 */
void DoCassStuff(int *br, int *cc)
{
   switch (LOAD_BYTE(0x6029)&0x1F) {

      case 1:
         STORE_BYTE(0x602A, 1);
         *cc&=0xfe;
	 break;

      case 2:
	 if ((cass) && !feof(cass)) {
	    if (current_op == WRITE) {
	       fflush(cass); /* pour se conformer � l'ANSI C */
	       current_op=READ;
	    }

	    *br=fgetc(cass)&0xFF;
	    *cc&=0xfe;

	    cass_counter = -1;  /* position du fichier modifi�e */
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      case 4:
	 if (cass && (cass_mode == TO8_READ_WRITE)) {
	    if (LOAD_BYTE(0x602A) != 4) {
	       int i;

	       if (current_op==READ) {
		  fflush(cass); /* pour se conformer � l'ANSI C */
		  current_op=WRITE;
	       }

	       for (i=0;i<10;i++)
		  fputc(*br, cass);

	       STORE_BYTE(0x602A, 4);

	       cass_counter = -1;  /* position du fichier modifi�e */
	    }

	    *cc&=0xfe;
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      case 8:
	 if ((cass) && (cass_mode == TO8_READ_WRITE)) {
	    if (current_op == READ) {
	       fflush(cass); /* pour se conformer � l'ANSI C */
	       current_op = WRITE;
	    }

	    fputc(*br,cass);
	    *cc&=0xfe;

	    cass_counter = -1;  /* position du fichier modifi�e */
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      default:
	 STORE_BYTE(0x602A, 0x10);
	 *cc&=0xfe;
	 break;
   }
}



/* InitCass:
 *  Initialise le module Cass.
 */
void InitCass(void)
{
    /* Appel routine de gestion Cass. */
    mem.mon.bank[0][0x1A59] = TO8_TRAP_CODE;
    mem.mon.bank[0][0x1A5A] = 0x39;

    cass = NULL;
    cass_mode = TO8_READ_ONLY;
    cass_counter = -1;
    current_op = READ;
}



/**********************************/
/* partie publique                */
/**********************************/


/* EjectCass:
 *  Ejecte la cassette.
 */
void to8_EjectCass(void)
{
    cass = std_fclose(cass);
    teo.cass.file = std_free (teo.cass.file);
    cass_counter = -1;
    current_op = READ;
}


/* LoadCass:
 *  Charge une cassette dans le lecteur et retourne le mode d'ouverture.
 *  Retourne TO8_ERROR en cas d'�chec et pr�serve la cassette pr�c�demment
 *  charg�e.
 */
int to8_LoadCass(const char filename[])
{
   int ret = DoLoadCass(filename, cass_mode);

   if (ret != TO8_ERROR) {
      teo.cass.file = std_free (teo.cass.file);
      teo.cass.file = std_strdup_printf ("%s", filename);
      cass_mode = ret;
   }

   return ret;
}



/* SetCassMode:
 *  Fixe le mode d'acc�s � la cassette. Retourne le mode en cas de succ�s
 *  ou TO8_ERROR en case d'�chec.
 */
int to8_SetCassMode(int mode)
{
   if (cass_mode == mode)
      return cass_mode;

   if (cass) {
      int ret = DoLoadCass(teo.cass.file, mode);

      if (ret != TO8_ERROR)
	 cass_mode = ret;

      return ret;
   }
   else {
      cass_mode = mode;
      return cass_mode;
   }
}



/* GetCassCounter:
 *  Retourne la valeur du compteur du lecteur.
 */
int to8_GetCassCounter(void)
{
   cass_counter = (cass ? ftell(cass)/COUNTER_RATIO : 0);
   return cass_counter;
}



/* SetCassCounter:
 *  Fixe la valeur du compteur du lecteur.
 */
void to8_SetCassCounter(int counter)
{
   if (cass) {
      /* V�rifie que le compteur a r�ellement chang� avant de repositionner
	 le fichier, de fa�on � �viter les �carts de position d�s � l'arrondi
	 lors de la division par COUNTER_RATIO.  */
      if (counter != cass_counter) {
	 fseek(cass, counter*COUNTER_RATIO, SEEK_SET);
	 cass_counter = counter;
      }
   }
}
