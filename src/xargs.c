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
 *  Module     : xargs.c
 *  Version    : 1.8.1
 *  Créé par   : Samuel Devulder 30/07/2011
 *  Modifié par: François Mouret 25/04/2012
 *
 *  Module de traitement des arguments supplémentaires en ligne de commande.
 */

 
#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <unistd.h>
#endif

#include "intern/std.h"
#include "intern/xargs.h"
#include "to8.h"

/* exitMsg:
 *   nettoie la structure et affiche le message.
 */
static void exitMsg(xargs *xargs, char *msg) {
    void (*exitMsg)(char*) = xargs->exitMsg; 
    
    xargs_exit(xargs); 
    
    if(exitMsg) (*exitMsg)(msg);
    exit(EXIT_FAILURE);
}


/* rmFile:
 *   efface un fichier
 */
static void rmFile(xargs *xargs, char *path) {
    if(xargs->rmFile)
        (*xargs->rmFile)(path);
    else    exitMsg(xargs, is_fr?"rmFile manquant":"rmFile is missing");
}

/* sysExec:
 *   execution d'une commande dans un dossier
 */
static void sysExec(xargs *xargs, char *cmd, char *dir) {
    if(xargs->sysExec)
        (*xargs->sysExec)(cmd, dir);
    else     exitMsg(xargs, is_fr?"sysExec manquant":"sysExec is missing");
}

/* strEndsWith:
 *   Retourne vrai si la 1ere chaine se termine avec la 2nd. Le 3eme paramètre
 *   indique si la comparaisons est case-sensitive.
 */
static int strEndsWith(const char *str, const char *suffix, const int ignorecase) {
#define eqChr(c,d,i) (i ? tolower(255 & (c)) == tolower(255 & (d)) : (c) == (d))
    const char *s = str;
    const char *t = suffix;
    
    while(*s) ++s; 
    while(*t) ++t;
    while(s>str && t>suffix && eqChr(*s, *t, ignorecase)) --s, --t;
    
    return s>=str && t==suffix && eqChr(*s, *t, ignorecase);
#undef eqChr
}


/* xargs_parse:
 *  parse argc, argv et rempli la structure xargs. Les pointeurs
 *  sur fonctions doivent avoir étés remplis si on veut avoir acces
 *  a certaines fonctionnalités.
 */
void xargs_parse(xargs *xargs, char *fname) {
    int i;
    char *dir;
    char tmp[256];
    char buf[256];
    char *s;

    /* si on a un fichier disque sans option ou un dossier, on trouve le 1er
       disque libre */
    if(std_isdir (fname) || 
       (std_isfile(fname) && strEndsWith(fname,".sap",1))) {
        if(NULL == xargs->disk[0]) xargs->disk[0] = fname; else
        if(NULL == xargs->disk[1]) xargs->disk[1] = fname; else
        if(NULL == xargs->disk[2]) xargs->disk[2] = fname; else
        if(NULL == xargs->disk[3]) xargs->disk[3] = fname; else
        exitMsg(xargs, "All disks used.");
        
    } else if(std_isfile(fname)) {
        /* traitement d'un fichier memo7 sans '-m' */
        if(strEndsWith(fname, ".m7", 1)) {
            if(xargs->memo) exitMsg(xargs, is_fr?"Fichier M7 déjà présent":"M7 file already set."); 
            else xargs->memo = fname;
        } else if(strEndsWith(fname, ".k7", 1)) {
            xargs->k7 = fname;
        } else {
            sprintf(buf, is_fr?"Fichier inconnu: %s":"Unknown file: %s", fname);
            exitMsg(xargs, buf);
        }
    }    
    /* conversion dossier en fichier sap-temporaires */
    if(xargs->sapfs && xargs->tmpFile) {
        for(i=0; i<4; ++i) if(xargs->disk[i] && std_isdir(xargs->disk[i])) {
            dir = xargs->disk[i];
            
            if(NULL == (*xargs->tmpFile)(tmp, sizeof(tmp))) 
                xargs->disk[i] = NULL;
            else {
                xargs->disk[i] = C(NULL, tmp);
                xargs->dir[i] = dir;

                s = std_strdup_printf ("%s -c %s %s", xargs->sapfs, tmp);
                sysExec(xargs, s, dir);
                s = std_free (s);
                
                s = std_strdup_printf ("%s -a %s *%s", xargs->sapfs, tmp, dir);
                sysExec(xargs, s, dir);
                s = std_free (s);
            }
        }
    }
}


/* xargs_exit:
 *   nettoie les données internes
 */
void xargs_exit(xargs *xargs) {
    int i;
    char *cmd;
    
    /* nettoyage des fichiers d'auto-conversion directory->sap. */
    for(i=0; i<4; ++i) if(xargs->sapfs && xargs->dir[i]) {
        cmd = std_strdup_printf ("%s -y %s", xargs->sapfs, xargs->disk[i]);
        sysExec(xargs, cmd, xargs->dir[i]);
        s = std_free (s);
        rmFile(xargs, xargs->disk[i]);
        xargs->disk[i] = std_free(xargs->disk[i]);
    }
}

/* xargs_init:
 *   initialise la structure xargs à vide
 */
void xargs_init(xargs *args) {
    memset(args, 0, sizeof(xargs));
}

