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
 *  Version    : 1.8.2
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

#include "xargs.h"
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

/* isFile:
 *   detecte si un chemin est un fichier
 */
static int isFile(xargs *xargs, char *path) {
    return xargs->isFile && (*xargs->isFile)(path);
}

/* isDir:
 *   detecte si un chemin est un dossier
 */
static int isDir(xargs *xargs, char *path) {
    return xargs->isDir && (*xargs->isDir)(path);
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

/* cat: 
 *  Concaténation de chaine. Le resultat est alloué sur le tas.
 *  La 1ere chaine est libérée via free() sauf si elle est NULL.
 */
static char *cat(xargs *xargs, char *s1, const char *s2) {
    int l1 = s1 ? strlen(s1) : 0;
    int l2 = s2 ? strlen(s2) : 0;
    char *p = malloc(l1 + l2 + 1);
    if(p == NULL) exitMsg(xargs, is_fr?"Mémoire insuffisante":"Out of memory");
    if(s1) {strcpy(p, s1); free(s1);}
    if(l2)
        strcpy(p + l1, s2);
    else
        p[l1] = '\0';

    return p;
}

/* xargs_parse:
 *  parse argc, argv et rempli la structure xargs. Les pointeurs
 *  sur fonctions doivent avoir été remplis si on veut avoir accès
 *  à certaines fonctionnalités.
 */
void xargs_parse(xargs *xargs, int argc, char **argv) {
    int i;

    for (i=0; i<argc; i++) {
        if (!strcmp(argv[i],"-disk0") && i<argc-1)
            xargs->disk[0] = argv[++i];
        else if (!strcmp(argv[i],"-disk1") && i<argc-1)
            xargs->disk[1] = argv[++i];
        else if (!strcmp(argv[i],"-disk2") && i<argc-1)
            xargs->disk[2] = argv[++i];
        else if (!strcmp(argv[i],"-disk3") && i<argc-1)
            xargs->disk[3] = argv[++i];
        else if (!strcmp(argv[i],"-k7") && i<argc-1) 
            xargs->k7 = argv[++i];
        /* si on a un fichier disque sans option ou un dossier, on trouve le 1er
           disque libre */
        else if(isDir (xargs, argv[i]) || 
               (isFile(xargs, argv[i]) && strEndsWith(argv[i],".sap",1))) {
            if(NULL == xargs->disk[0]) xargs->disk[0] = argv[i]; else
            if(NULL == xargs->disk[1]) xargs->disk[1] = argv[i]; else
            if(NULL == xargs->disk[2]) xargs->disk[2] = argv[i]; else
            if(NULL == xargs->disk[3]) xargs->disk[3] = argv[i]; else
            exitMsg(xargs, "All disks used.");
        } else if(isFile(xargs, argv[i])) {
            /* traitement d'un fichier memo7 sans '-m' */
            if(strEndsWith(argv[i], ".m7", 1)) {
                if(xargs->memo) exitMsg(xargs, is_fr?"Fichier M7 déjà présent":"M7 file already set."); 
                else xargs->memo = argv[i];
            } else if(strEndsWith(argv[i], ".k7", 1)) {
                xargs->k7 = argv[i];
            } else {
                char buf[256];
                sprintf(buf, is_fr?"Fichier inconnu: %s":"Unknown file: %s", argv[i]);
                exitMsg(xargs, buf);
            }
        } else {
            if(xargs->unknownArg)
                i += (*xargs->unknownArg)(argv[i]);
            else {
                char buf[256];
                sprintf(buf, is_fr?"Paramètre inconnu: %s":"Unknown parameter: %s", argv[i]);
                exitMsg(xargs, buf);
            }
        }
    }    
    /* conversion dossier en fichier sap-temporaires */
    if(xargs->sapfs && xargs->tmpFile) {
        for(i=0; i<4; ++i) if(xargs->disk[i] && isDir(xargs, xargs->disk[i])) {
            char *dir = xargs->disk[i];
            char tmp[256];
            
            if(NULL == (*xargs->tmpFile)(tmp, sizeof(tmp))) 
                xargs->disk[i] = NULL;
            else {
                char *s;
#define C(a,b)    cat(xargs, a, b)
                xargs->disk[i] = C(NULL, tmp);
                xargs->dir[i] = dir;

                sysExec(xargs, s =   C(C(C(NULL, xargs->sapfs), " -c "), tmp), dir); free(s);
                sysExec(xargs, s = C(C(C(C(NULL, xargs->sapfs), " -a "), tmp), " *"), dir); free(s);
            }
        }
    }
}

/* xargs_start:
 *   met en place les données de TEO en fonction des arguments
 *   supplémentaires.
 */
void xargs_start(xargs *xargs) {
    int i;
    
    if(xargs->k7) {
        if(TO8_ERROR == to8_SetK7Mode(TO8_READ_ONLY) ||
           TO8_ERROR == to8_LoadK7(xargs->k7)) {
            char buf[256];
            sprintf(buf, is_fr?"Erreur chargement K7: %s":"K7 loading error: %s", xargs->k7);
            exitMsg(xargs, buf);
        }
    }
    
    for(i = 0; i<4; ++i) if(xargs->disk[i]) {
        if(TO8_ERROR == to8_LoadDisk(i, xargs->disk[i])) {
            char buf[256];
            sprintf(buf, is_fr?"Erreur disquette %d: %s":"Diskette error %d: %s", i, xargs->dir[i]?xargs->dir[i]:xargs->disk[i]);
            exitMsg(xargs, buf);
        }
    }
    
    if(xargs->memo)
        to8_LoadMemo7(xargs->memo);
}


/* xargs_exit:
 *   nettoie les données internes
 */
void xargs_exit(xargs *xargs) {
    int i;
    
    /* nettoyage des fichiers d'auto-conversion directory->sap. */
    for(i=0; i<4; ++i) if(xargs->sapfs && xargs->dir[i]) {
        char *cmd = C(C(C(NULL, xargs->sapfs), " -y "), xargs->disk[i]);
        sysExec(xargs, cmd, xargs->dir[i]);
        rmFile(xargs, xargs->disk[i]);
        free(cmd); free(xargs->disk[i]);
    }
}

/* xargs_init:
 *   initialise la structure xargs à vide
 */
void xargs_init(xargs *args) {
    memset(args, 0, sizeof(xargs));
}

