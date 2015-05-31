/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2015 François Mouret
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
 *  Module     : option.c
 *  Version    : 0.7.0
 *  Créé par   : François Mouret 27/02/2013
 *  Modifié par:
 *
 *  Management of the command line.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <sys/stat.h>
   #include <unistd.h>
#endif

#include "defs.h"
#include "main.h"
#include "std.h"
#include "option.h"
#include "ini.h"
#include "encode.h"
#include "errors.h"


enum {
  WRONG_OPTION = 0,
  SHORT_OPTION,
  LONG_OPTION
};

struct OPTION_ENTRY *help_option = NULL;
struct OPTION_ENTRY *prog_option = NULL;



/* help_display:
 *  Affiche une liste d'aide.
 */
static void help_display (struct OPTION_ENTRY option[])
{
    int i;
    size_t size;

    for (i=0; option[i].long_name!=NULL; i++)
    {
        size = 0;
        printf ("  ");
        if (option[i].short_name != '\0')
            size += printf ("-%c, ", option[i].short_name);
        size += printf ("--%s", option[i].long_name);
        if (option[i].argument != NULL)
            size += printf (" <%s>",option[i].argument);
        if (option[i].comment != NULL) {
           do { size += printf (" "); } while (size < 19);
           (void)printf (" %s", encode_String(option[i].comment));
        }
        (void)printf ("\n");
    }
}



/* help:
 *  Affiche l'aide.
 */
static void help (char *program_name)
{
    printf ("\n%s:\n", is_fr?"Utilisation":"Usage");
    printf ("    %s [OPTION...] %s\n\n", program_name,
                           is_fr?"[FICHIER HFE]":"[HFE FILE]");
    printf ("%s:\n", is_fr?"Options de l'aide":"Help Options");
    help_display (help_option);
    printf ("\n");
    printf ("%s:\n", is_fr?"Options de l'application":"Application Options");
    help_display (prog_option);
    printf ("\n");
    main_FreeAll ();
    exit (EXIT_SUCCESS);
}



/* get_string:
 *  Traite une liste d'options.
 */
static char *get_string (struct OPTION_ENTRY *option, char *str)
{
    char *p, *s, *tmp_s;
    char **reg;

    if (str != NULL)
    {
        s = std_strdup_printf ("%s", str);
        if ((s != NULL)
         && (*s == '"')
         && ((p = strrchr (s, '"')) != NULL)
         && (p > s))
        {
            *p = '\0';
            tmp_s = std_strdup_printf ("%s", s+1);
            s = std_free (s);
            s = tmp_s;
        }
        reg = option->reg;
        *reg = (char*)std_free (*reg);
        *reg = std_strdup_printf ("%s",s);
    }
    return str;
}



/* option_check:
 *  Traite une liste d'options.
 */
static char *option_check (int argc, char *argv[],
                          char *internal_prog_name,
                          struct OPTION_ENTRY option[],
                          struct STRING_LIST **remain_option)
{
    char *s = NULL;
    int i, op;
    int *d;
    int option_size;
    int option_i = 0;
    int arg_i;

    for (i=1; i<argc; i++)
    {
        if (argv[i][0] == '-')
        {
            option_size = WRONG_OPTION;
            for (op=0; (option[op].long_name != NULL) && (option_size == WRONG_OPTION); op++)
            {
                option_i = op;
                if (strlen(argv[i]) == 2)
                {
                    if (argv[i][1] == option[op].short_name)
                        option_size = SHORT_OPTION;
                }
                else
                if (argv[i][1] == '-')
                {
                    if (strncmp (&argv[i][2], option[op].long_name,
                             strlen (option[op].long_name)) == 0)
                        option_size = LONG_OPTION;
                }
            }
            
            if (option_size == WRONG_OPTION)
                return std_strdup_printf ("%s : %s", argv[i],
                              is_fr?"Option inconnue":"Unknown option");

            switch (option[option_i].type) {
            case OPTION_ARG_FILENAME :
            case OPTION_ARG_STRING :
                if (option_size == SHORT_OPTION) {
                    s = std_free(s);
                    if (++i < argc)
                        s = get_string (&option[option_i], argv[i]);
                } else {
                   arg_i = strlen (option[option_i].long_name) + 2;
                   if (argv[i][arg_i] == '=')
                       s = get_string (&option[option_i], std_strdup_printf ("%s", &argv[i][arg_i+1]));
                }
                if (s==NULL)
                     return std_strdup_printf ("%s %s",
                              is_fr?"Argument manquant pour":"Missing argument for",
                              argv[i]);
                break;

            case OPTION_ARG_INT :
                d = option[option_i].reg;
                *d = (int)strtol (argv[i+1],NULL,10);
                if ((*d < option[option_i].min_value)
                 || (*d > option[option_i].max_value))
                     return std_strdup_printf ("%s %s (%d,%d)",
                              is_fr?"Mauvaise valeur pour":"Bad value for",
                              argv[i],
                              option[option_i].min_value,
                              option[option_i].max_value
                              );
                i++;
                break;

            case OPTION_ARG_BOOL :
                d = option[option_i].reg;
                *d = TRUE;
                break;

            case OPTION_ARG_HELP :
                help (internal_prog_name);
                break;

            default : return std_strdup_printf ("%s %s",
                              is_fr?"Type inconnu pour":"Unknown type for",
                              argv[i]);
            }
        }
        else
            *remain_option = std_StringListAppend (*remain_option, argv[i]);
    }
    return NULL;
}


/* ------------------------------------------------------------------------- */


/* option_Parse:
 *  Traite les options.
 */
char *option_Parse (int argc, char *argv[],
                          char *internal_prog_name,
                          struct OPTION_ENTRY prog_option_list[],
                          struct STRING_LIST **remain_option)
{
    struct OPTION_ENTRY help_option_list[] = {
        { "help", 'h', OPTION_ARG_HELP, NULL,
          is_fr?"Aide de ce programme":"Help of this program", NULL },
        { NULL, 0, 0, NULL, NULL, NULL }
    };

    help_option = help_option_list;
    prog_option = prog_option_list;
    
    error_msg = std_free (error_msg);
    error_msg = option_check (argc, argv, internal_prog_name,
                              help_option, remain_option);
    if (error_msg != NULL) {
        error_msg = std_free (error_msg);
        error_msg = option_check (argc, argv, internal_prog_name,
                                      prog_option, remain_option);
    }
    return error_msg;
}

