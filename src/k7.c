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
 *  Copyright (C) 1997-2012 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, Samuel Devulder
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
 *  Module     : k7.c
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou avril 1999
 *  Modifié par: Eric Botcazou 28/10/2003
 *               Samuel Devulder 05/02/2012
 *  Gestion des cassettes du TO8.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "intern/errors.h"
#include "intern/hardware.h"
#include "intern/k7.h"  /* MacOS */
#include "to8.h"


#define COUNTER_RATIO   100

static FILE *k7;
static char k7_filename[FILENAME_LENGTH+1];
static int k7_mode;
static int k7_counter;
static enum {
   READ,
   WRITE
} current_op;



/* DoLoadK7:
 *  Ouvre le fichier et retourne le mode d'ouverture.
 */
static int DoLoadK7(const char filename[], int mode)
{
   FILE *new_k7;

   if (mode == TO8_READ_WRITE) {
      if ((new_k7=fopen(filename, "rb+")) != NULL)
	 goto Success;
      else
	 mode = TO8_READ_ONLY;
   }

   if ((new_k7=fopen(filename, "rb")) != NULL)
      goto Success;

   return ErrorMessage(TO8_CANNOT_OPEN_FILE, NULL);

 Success:
   if (k7)
      fclose(k7);

   k7 = new_k7;
   k7_counter = -1;  /* position du fichier modifiée */

   return mode;
}



/* DoK7Stuff:
 *  Emule le contrôleur du lecteur de cassettes.
 */
void DoK7Stuff(int *br, int *cc)
{
   switch (LOAD_BYTE(0x6029)&0x1F) {

      case 1:
         STORE_BYTE(0x602A, 1);
         *cc&=0xfe;
	 break;

      case 2:
	 if ((k7) && !feof(k7)) {
	    if (current_op == WRITE) {
	       fflush(k7); /* pour se conformer à l'ANSI C */
	       current_op=READ;
	    }

	    *br=fgetc(k7)&0xFF;
	    *cc&=0xfe;

	    k7_counter = -1;  /* position du fichier modifiée */
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      case 4:
	 if (k7 && (k7_mode == TO8_READ_WRITE)) {
	    if (LOAD_BYTE(0x602A) != 4) {
	       int i;

	       if (current_op==READ) {
		  fflush(k7); /* pour se conformer à l'ANSI C */
		  current_op=WRITE;
	       }

	       for (i=0;i<10;i++)
		  fputc(*br, k7);

	       STORE_BYTE(0x602A, 4);

	       k7_counter = -1;  /* position du fichier modifiée */
	    }

	    *cc&=0xfe;
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      case 8:
	 if ((k7) && (k7_mode == TO8_READ_WRITE)) {
	    if (current_op == READ) {
	       fflush(k7); /* pour se conformer à l'ANSI C */
	       current_op = WRITE;
	    }

	    fputc(*br,k7);
	    *cc&=0xfe;

	    k7_counter = -1;  /* position du fichier modifiée */
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



/* InitK7:
 *  Initialise le module K7.
 */
void InitK7(void)
{
    /* Appel routine de gestion K7. */
    mem.mon.bank[0][0x1A59] = TO8_TRAP_CODE;
    mem.mon.bank[0][0x1A5A] = 0x39;

    k7 = NULL;
    k7_filename[0] = '\0';
    k7_mode = TO8_READ_ONLY;
    k7_counter = -1;
    current_op = READ;
}



/**********************************/
/* partie publique                */
/**********************************/


/* LoadK7:
 *  Charge une cassette dans le lecteur et retourne le mode d'ouverture.
 *  Retourne TO8_ERROR en cas d'échec et préserve la cassette précédemment
 *  chargée.
 */
int to8_LoadK7(const char filename[])
{
   int ret = DoLoadK7(filename, k7_mode);

   if (ret != TO8_ERROR) {
      strcpy(k7_filename, filename);
      k7_mode = ret;
   }

   return ret;
}



/* SetK7Mode:
 *  Fixe le mode d'accès à la cassette. Retourne le mode en cas de succès
 *  ou TO8_ERROR en case d'échec.
 */
int to8_SetK7Mode(int mode)
{
   if (k7_mode == mode)
      return k7_mode;

   if (k7) {
      int ret = DoLoadK7(k7_filename, mode);

      if (ret != TO8_ERROR)
	 k7_mode = ret;

      return ret;
   }
   else {
      k7_mode = mode;
      return k7_mode;
   }
}



/* GetK7Counter:
 *  Retourne la valeur du compteur du lecteur.
 */
int to8_GetK7Counter(void)
{
   k7_counter = (k7 ? ftell(k7)/COUNTER_RATIO : 0);
   return k7_counter;
}



/* SetK7Counter:
 *  Fixe la valeur du compteur du lecteur.
 */
void to8_SetK7Counter(int counter)
{
   if (k7) {
      /* Vérifie que le compteur a réellement changé avant de repositionner
	 le fichier, de façon à éviter les écarts de position dûs à l'arrondi
	 lors de la division par COUNTER_RATIO.  */
      if (counter != k7_counter) {
	 fseek(k7, counter*COUNTER_RATIO, SEEK_SET);
	 k7_counter = counter;
      }
   }
}



/* GetK7Filename:
 *  Retourne le nom du fichier utilisé comme cassette.
 */
const char* to8_GetK7Filename(void)
{
   return k7_filename;
}

