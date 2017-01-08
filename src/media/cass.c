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
 *  Copyright (C) 1997-2017 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
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
 *  Module     : cass.c
 *  Version    : 1.8.4
 *  Créé par   : Eric Botcazou avril 1999
 *  Modifié par: Eric Botcazou 28/10/2003
 *               Samuel Devulder 05/02/2012
 *               François Mouret 25/04/2012 29/09/2012
 *
 *  Gestion des cassettes.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <string.h>
#endif

#include "media/cass.h"  /* MacOS */
#include "errors.h"
#include "hardware.h"
#include "main.h"
#include "std.h"
#include "teo.h"


#define COUNTER_RATIO   100

static FILE *cass;
static int cass_counter;
static enum {
   READ,
   WRITE
} current_op;



/* DoLoadCass:
 *  Ouvre le fichier et retourne le mode d'ouverture.
 */
static int DoLoadCass(const char filename[], int protection)
{
   FILE *new_cass;

   if (protection == FALSE) {
      if ((new_cass=fopen(filename, "rb+")) != NULL)
	 goto Success;
      else
	 protection = TRUE;
   }

   if ((new_cass=fopen(filename, "rb")) != NULL)
      goto Success;

   return error_Message(TEO_ERROR_FILE_OPEN, filename);

 Success:
   if (cass)
      fclose(cass);

   cass = new_cass;
   cass_counter = -1;  /* position du fichier modifiée */

   return protection;
}


/* ------------------------------------------------------------------------- */


/* cass_Event:
 *  Emule le contrôleur du lecteur de cassettes.
 */
void cass_Event (int *br, int *cc)
{
   switch (LOAD_BYTE(0x6029)&0x1F) {

      case 1:
         STORE_BYTE(0x602A, 1);
         *cc&=0xfe;
	 break;

      case 2:
	 if ((cass) && !feof(cass)) {
	    if (current_op == WRITE) {
	       fflush(cass); /* pour se conformer à l'ANSI C */
	       current_op=READ;
	    }

	    *br=fgetc(cass)&0xFF;
	    *cc&=0xfe;

	    cass_counter = -1;  /* position du fichier modifiée */
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      case 4:
	 if (cass && (teo.cass.write_protect == FALSE)) {
	    if (LOAD_BYTE(0x602A) != 4) {
	       int i;

	       if (current_op==READ) {
		  fflush(cass); /* pour se conformer à l'ANSI C */
		  current_op=WRITE;
	       }

	       for (i=0;i<10;i++)
		  fputc(*br, cass);

	       STORE_BYTE(0x602A, 4);

	       cass_counter = -1;  /* position du fichier modifiée */
	    }

	    *cc&=0xfe;
	 }
	 else {
	    STORE_BYTE(0x602A, 0x80);
	    *cc|=1;
	 }
	 break;

      case 8:
	 if ((cass) && (teo.cass.write_protect == FALSE)) {
	    if (current_op == READ) {
	       fflush(cass); /* pour se conformer à l'ANSI C */
	       current_op = WRITE;
	    }

	    fputc(*br,cass);
	    *cc&=0xfe;

	    cass_counter = -1;  /* position du fichier modifiée */
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



/* cass_Init:
 *  Initialise le module Cass.
 */
void cass_Init(void)
{
    /* Appel routine de gestion Cass. */
    mem.mon.bank[0][0x1A59] = TEO_TRAP_CODE;
    mem.mon.bank[0][0x1A5A] = 0x39;

    cass = NULL;
    cass_counter = -1;
    current_op = READ;
}



/* cass_IsCass:
 *  Vérifie la validité du fichier cassette.
 */
int cass_IsCass (const char filename[])
{
    return (std_IsFile (filename) == TRUE) ? 0 : TEO_ERROR;
}



/* cass_Eject:
 *  Ejecte la cassette.
 */
void cass_Eject(void)
{
    cass = std_fclose(cass);
    teo.cass.file = std_free (teo.cass.file);
    cass_counter = -1;
    current_op = READ;
}



/* cass_Load:
 *  Charge une cassette dans le lecteur et retourne le mode d'ouverture.
 *  Retourne TEO_ERROR en cas d'échec et préserve la cassette précédemment
 *  chargée.
 */
int cass_Load(const char filename[])
{
   int ret = DoLoadCass(filename, teo.cass.write_protect);

   if (ret >= 0) {
      teo.cass.file = std_free (teo.cass.file);
      teo.cass.file = std_strdup_printf ("%s", filename);
      teo.cass.write_protect = ret;
   }

   return ret;
}



/* Premier chargement de la cassette */
void cass_FirstLoad (void)
{
    char *s = NULL;

    if (teo.cass.file !=NULL) {
        s = std_strdup_printf ("%s", teo.cass.file);
        teo.cass.file = std_free (teo.cass.file);
        if (s != NULL)
            if (cass_Load (s) < 0)
                main_DisplayMessage (teo_error_msg);
        s = std_free (s);
    }
}



/* cass_SetProtection:
 *  Fixe le mode d'accès à la cassette. Retourne le mode en cas de succès
 *  ou TEO_ERROR en case d'échec.
 */
int cass_SetProtection(int protection)
{
   if (teo.cass.write_protect == protection)
      return protection;

   if (cass) {
      int ret = DoLoadCass(teo.cass.file, protection);

      if (ret >= 0)
	 teo.cass.write_protect = ret;

      return ret;
   }
   else {
      teo.cass.write_protect = protection;
      return protection;
   }
}



/* cass_GetCounter:
 *  Retourne la valeur du compteur du lecteur.
 */
int cass_GetCounter(void)
{
   cass_counter = (cass ? ftell(cass)/COUNTER_RATIO : 0);
   return cass_counter;
}



/* cass_SetCounter:
 *  Fixe la valeur du compteur du lecteur.
 */
void cass_SetCounter(int counter)
{
   if (cass) {
      /* Vérifie que le compteur a réellement changé avant de repositionner
	 le fichier, de façon à éviter les écarts de position dûs à l'arrondi
	 lors de la division par COUNTER_RATIO.  */
      if (counter != cass_counter) {
	 fseek(cass, counter*COUNTER_RATIO, SEEK_SET);
	 cass_counter = counter;
      }
   }
}

