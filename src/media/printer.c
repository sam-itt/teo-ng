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
 *                          Jérémie Guillaume, François Mouret
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
 *  Module     : printer.c
 *  Version    : 1.8.2
 *  Créé par   : Eric Botcazou 22/03/2001
 *  Modifié par: Eric Botcazou 30/03/2001
 *               François Mouret 14/04/2012 02/11/2012
 *
 *  Emulation des imprimantes.
 */

#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
#endif

#include "teo.h"
#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "media/printer.h"
#include "media/printer/pr90042.h"
#include "media/printer/pr90055.h"
#include "media/printer/pr90582.h"
#include "media/printer/pr906xx.h"
#include "file/bmp.h"
#include "file/png.h"
#include "hardware.h"
#include "std.h"
#include "alleg/gui.h"

#define FONT_CHAR_COUNT   224
#define FONT_CHAR_WIDTH   4
#define FONT_CHAR_SIZE    (FONT_CHAR_WIDTH<<4)
#define BUFFER_HEIGHT     96

struct CHAR_EQUIVALENCE {
    int   chr;
    int   gfx;
    int   txt;
};

static FILE *fp_text = NULL;
static FILE *fp_raw = NULL;
static struct CHAR_EQUIVALENCE *char_equivalence = NULL;

/* font registers */
static int   font_face;
static int   font_count;
static int   font_height;
static int   font_fixed_width;
static int   font_dot_width;
static int   font_dot_height;
static int   font_dblwidth1_shift;
static int   *font_buffer = NULL;
static char  *font_width_buffer = NULL;
static int   font_pica_width;
static int   font_double_width;
static int   font_underline;
static int   font_bold;

/* paper registers */
static int   paper_counter = 0;
static int   paper_x = 0;
static int   paper_y = 0;
static int   paper_width;
static int   paper_height;
static int   paper_width_max;
static char  *paper_buffer = NULL;
static int   paper_opened = FALSE;
static int   paper_left_margin;
static int   paper_lf_height;
static int   paper_pixels_per_inch;

/* counter registers */
static int   counter_length;
static void  (*counter_jump)();

/* gfx registers */
static int   screen_pixel_size;
static void  (*gfx_prog)();
static int   gfx_counter;
static mc6809_clock_t last_data_time = 0;

/* printer registers */
static char  *hard_buffer = NULL;
static int   hard_buffer_height = 0;
static int   printer_opened = FALSE;
struct PRINTER printer;



/* print_raw_char:
 *  Ecrit un caractère brut.
 */
static void print_raw_char (int c)
{
    char *filename = NULL;

    if (!printer.lprt.raw_output)
        return;

    if (fp_raw == NULL)
    {
        filename = std_strdup_printf ("%s%slprt%03d.bin",
                                       printer.lprt.folder,
                                       FOLDER_SLASH,
                                       paper_counter);
        fp_raw = fopen (filename, "wb");
        filename = std_free (filename);
        if (fp_raw == NULL)
            return;
    }
    fputc (c, fp_raw);
    fflush (fp_raw);
}



/* print_text_char:
 *  Ecrit un caractère texte.
 */
static void print_text_char (int c)
{
    char *filename = NULL;

    if (!printer.lprt.txt_output)
        return;

    if (fp_text == NULL)
    {
        filename = std_strdup_printf ("%s%slprt%03d.txt",
                                        printer.lprt.folder,
                                        FOLDER_SLASH,
                                        paper_counter);
        fp_text = fopen (filename, "wb");
        filename = std_free (filename);
        if (fp_text == NULL)
            return;
    }
    fputc (c, fp_text);
    fflush (fp_text);
}



/* print_drawable_text_char:
 *  Ecrit un caractère texte affichable.
 */
static void print_drawable_text_char (int c)
{
    /* convert character if up to 0x7f */
    if (c >= 0xa0)
        c = (char_equivalence != NULL) ? char_equivalence[c-0xa0].txt : 0x20;

    /* write character (manage utf8 characters) */
    if ((c & 0xff0000) != 0)
        print_text_char ((c >> 16) & 0xff);
    if ((c & 0xff00) != 0)
        print_text_char ((c >> 8) & 0xff);
    print_text_char (c & 0xff);
}



/* flush_buffer:
 *  Flush the printer buffer.
 */
static void flush_buffer (int flush_height)
{
    int x, y;
    int width = paper_width_max>>3;
    int height;
    char *blank_row = calloc (width, 1);
    char *filename = NULL;
#ifdef DJGPP
    int palette[2] = { 0x000000, 0xffffff };
#endif

    if (blank_row == NULL)
        return;

    if (hard_buffer == NULL)
    {
        hard_buffer = calloc ((paper_width_max>>3)*BUFFER_HEIGHT, sizeof(char));
        if (hard_buffer == NULL)
        {
            blank_row = std_free (blank_row);
            return;
        }
        hard_buffer_height = paper_lf_height;
    }

    if (paper_buffer != NULL)
    {
        /* update or-ed datas into printer hard-buffer */
        for (y=0; y<hard_buffer_height; y++)
            for (x=0; x<width; x++)
                 hard_buffer[x+y*width] |= paper_buffer[x+y*width];

        /* update normal datas into printer hard-buffer */
        if (hard_buffer_height < BUFFER_HEIGHT)
           memcpy (hard_buffer+hard_buffer_height*width,
                   paper_buffer+hard_buffer_height*width,
                   BUFFER_HEIGHT-hard_buffer_height);
    }

    /* open file if necessary */
    if (paper_opened == FALSE)
    {
#ifdef DJGPP
        filename = std_strdup_printf ("%s%slprt%03d.bmp",
                                      printer.lprt.folder,
                                      FOLDER_SLASH,
                                      paper_counter);
        (void)bmp_WriteOpen (filename, paper_width_max, paper_height, 2, palette);
#else
        filename = std_strdup_printf ("%s%slprt%03d.png",
                                      printer.lprt.folder,
                                      FOLDER_SLASH,
                                      paper_counter);
        (void)png_WriteOpen (filename, paper_width_max, paper_height);
#endif
        filename = std_free (filename);
        paper_opened = TRUE;
    }

    /* compute number of rows to write */
    height = (flush_height<BUFFER_HEIGHT)?flush_height:BUFFER_HEIGHT;

    /* send rows to write program */
    for (y=0; y<height; y++)
#ifdef DJGPP
        (void)bmp_WriteRow(hard_buffer+y*width, width);
#else
        png_WriteRow (hard_buffer+y*width);
#endif

    /* send eventually blank rows to write program */
    for (y=height; y<flush_height; y++)
#ifdef DJGPP
        (void)bmp_WriteRow (blank_row, width);
#else
        png_WriteRow (blank_row);
#endif

    /* move rows of printer hard-buffer to the top */
    if (height < BUFFER_HEIGHT)
        memmove (hard_buffer,
                 hard_buffer+height*width,
                 (BUFFER_HEIGHT-height)*width);

    /* clear remaining rows */
    if (height != 0)
        memset (hard_buffer+(BUFFER_HEIGHT-height)*width,
                0x00,
                height*width);

    /* update height of printer hard-buffer */
    hard_buffer_height = BUFFER_HEIGHT-height;
    paper_y += height;

    /* close current printer buffer */
    paper_buffer = std_free (paper_buffer);
    blank_row = std_free (blank_row);
}



/* gfx_eject:
 *  Ejecte le tirage graphique.
 */
static void gfx_eject (void)
{
    int i;
    char *blank_row = calloc (paper_width_max>>3, 1);

    if ((printer.lprt.gfx_output != 0) && (paper_opened == TRUE))
    {
        if (blank_row != NULL)
            for (i=paper_y; i<paper_height; i++)
#ifdef DJGPP
                (void)bmp_WriteRow (blank_row, paper_width_max>>3);
#else
                png_WriteRow (blank_row);
#endif

#ifdef DJGPP
        bmp_WriteClose ();
#else
        png_WriteClose ();
#endif
        paper_opened = FALSE;
    }

    paper_x = paper_left_margin;
    paper_y = 0;

    blank_row = std_free (blank_row);
    hard_buffer = std_free (hard_buffer);
    paper_buffer = std_free (paper_buffer);
    paper_counter++;
}



/* print_gfx_line_feed:
 *  Effectue un passage de ligne en mode graphique.
 */
static void print_gfx_line_feed (void)
{
    flush_buffer(paper_lf_height);
    paper_x = paper_left_margin;
    if (paper_y + paper_lf_height > paper_height)
        gfx_eject ();
}



/* draw_dot:
 *  Draw a dot.
 */
static void draw_dot (int y, int width, int height)
{
    static int  value;
    static int  row_length;
    static char byte0, byte1;
    static char *p;
    static const int bit_table[] = { 0x0000, 0x8000, 0xc000, 0xe000,
                                     0xf000, 0xf800, 0xfc00, 0xfe00,
                                     0xff00 };

    if (printer.lprt.gfx_output == 0)
        return;

    if (paper_buffer == NULL)
    {
        paper_buffer = calloc ((paper_width_max>>3)*BUFFER_HEIGHT, 1);
        if (paper_buffer == NULL)
            return;
    }

    row_length = paper_width_max >> 3;
    p = paper_buffer + y*row_length + (paper_x>>3);
    value = ((paper_x + width) < paper_width) ? width : paper_width - paper_x;
    value = bit_table[value] >> (paper_x & 7);
    byte0 = (char)(value>>8);
    byte1 = (char)value;

    if ((paper_x + width) < paper_width)
    {
        while (--height >= 0)
        {
            *p |= byte0;
            *(p+1) |= byte1;
            p += row_length;
        }
    }
    else
    {
        while (--height >= 0)
        {
            *p |= byte0;
            p += row_length;
        }
    }
}



/* draw_column:
 *  Draw a column of dots.
 */
static void draw_column (int column, int length, int pixel_width, int pixel_height)
{
    int y;

    for (y = 0; y < length; y++)
        if ((column & (1 << y)) != 0)
            draw_dot (y*pixel_height, pixel_width, pixel_height);

    paper_x += pixel_width;
}



/* print_drawable_gfx_char:
 *  Ecrit un caractère affichable.
 */
static void print_drawable_gfx_char (int c)
{
    int x;
    int x_max;
    int dot_width = font_dot_width << (font_double_width & (~font_dblwidth1_shift));
    int dot_height = font_dot_height << 1;
    int column;

    /* convert char if up to 0x7f */
    if (c >= 0xa0)
        c = (char_equivalence != NULL) ? char_equivalence[c-0xa0].gfx : 0x20;

    /* compute width of char */
    
    x_max = (((font_face & TEO_PRINTER_FBIT_PROPORTIONAL) != 0)
           && (font_width_buffer != NULL))
                   ? (int)font_width_buffer[printer.data-0x20]
                   : font_fixed_width;

    /* CR if End Of Line */
    if ((paper_x + x_max) > paper_width)
        print_gfx_line_feed ();

    /* draw char */
    for (x = 0; x < x_max; x++)
    {
        if (paper_x < paper_width)
        {
            column = (font_buffer == NULL)
                       ? 0
                       : font_buffer[(c-0x20)*FONT_CHAR_SIZE+x];
    
            if (font_underline != 0)
                column |= 1  << (font_height - 1);

            draw_column (column, font_height, dot_width, dot_height);

            if ((font_bold != 0) && (paper_x + x + 2 < paper_width))
            {
                draw_column (column, font_height, 2, dot_height);
                paper_x -= 2;
            }
        }
    }
}



/* read_ascii_counter:
 *  Récupère un compteur ASCII.
 */
static void read_ascii_counter (void)
{
    static int valid = 0;
    static int count = 0;

    count++;
    printer.counter_value = (printer.counter_value * 10) + (printer.data - '0');
    valid += (isdigit((int)printer.data) != 0) ? 1 : 0;
    if (count == counter_length)
    {
        if (valid == counter_length)
            (*counter_jump)();
        else
            printer_Forget ();

        count = 0;
        valid = 0;
    }
}



/* read_binary_counter:
 *  Récupère un compteur binaire.
 */
static void read_binary_counter (void)
{
    static int count = 0;

    count++;
    printer.counter_value = (printer.counter_value << 8) + printer.data;
    if (count == counter_length)
    {
        (*counter_jump)();
        count = 0;
    }
}



/* init_counter:
 *  Initialise une récupération de compteur ASCII ou binaire.
 */
static void init_counter (int length, void (*jump)())
{
    printer.counter_value = 0;
    counter_length = length;
    counter_jump = jump;
}



/* print_gfx8_data:
 *  Ecrit une colonne graphique 8 points.
 */
static void print_gfx8_data (void)
{
    draw_column (printer.data, 8, 2, 4);
}



/* print_gfx16_data:
 *  Ecrit une colonne graphique 16 points.
 */
#define GFX16_PIXEL_SIZE  2
static void print_gfx16_data (void)
{
    static int value = 0;

    if (value >= 0)
        value = ~printer.data;
    else
    {
        value = ~value | (printer.data << 8);
        draw_column (value, 16, GFX16_PIXEL_SIZE, GFX16_PIXEL_SIZE);
        printer_Forget ();
    }
}



/* print_screen_data:
 *  Ecrit un octet graphique 8 points.
 */
static void print_screen_data (void)
{
    int i;
    mc6809_clock_t delay = (printer.screenprint_delay * TEO_CPU_FREQ) / 1000;

    if (mc6809_clock() - last_data_time > delay)
    {
        gfx_counter = 0;
        return;
    }

    last_data_time = mc6809_clock();

    for (i=0x80; i>0; i>>=1)
    {
        if (printer.data & i)
           draw_dot (0, screen_pixel_size, screen_pixel_size);
        paper_x += screen_pixel_size;
    }
    if ((gfx_counter % 40) == 0)
    {
        flush_buffer(screen_pixel_size);
        paper_x = 0;
    }
}



/* print_gfx8_repeat:
 *  Programme la répétition d'une colonne graphique 8 points.
 */
static void print_gfx8_repeat (void)
{
    int i;

    for (i=0; i<printer.counter_value; i++)
        print_gfx8_data ();
    printer_Forget ();
}



/* print_gfx16_repeat:
 *  Programme la répétition d'une colonne graphique 16 points.
 */
static void print_gfx16_repeat (void)
{
    int i;
    static int value = 0;

    if (value >= 0)
        value = ~printer.data;
    else
    {
        value = (~value) | (printer.data << 8);
        for (i=0; i<printer.counter_value; i++)
            draw_column (value, 16, 2, 2);
        printer_Forget ();
    }
}



/* open_printer_file:
 *  Ouvre un fichier imprimante selon l'OS.
 */
static FILE *open_printer_file (char *name)
{
    static FILE *file;
    char *filename = NULL;
    
#ifdef DEBIAN_BUILD
    filename = std_strdup_printf (
                    "/usr/share/teo/system/printer/%s%03d/%s.txt",
                    printer.lprt.number,
                    name);
#else
    filename = std_strdup_printf (
                    "system%sprinter%s%03d%s%s.txt",
                    FOLDER_SLASH,
                    FOLDER_SLASH,
                    printer.lprt.number,
                    FOLDER_SLASH,
                    name);
#endif
    file = fopen (filename, "rb");
    filename = std_free (filename);
    return file;
}



/* open_font:
 *  Ouvre un fichier font.
 */
static FILE *open_font (char *name, char *style)
{
    static FILE *file;
    char  *filename = std_strdup_printf ("%s%s", name, style);

    file = open_printer_file (filename);
    filename = std_free (filename);
    return file;
}



/* load_font:
 *  Charge une matrice de caractères imprimante.
 */
#define FONT_STR_LENGTH 150
static void load_font (int face)
{
    FILE  *file = NULL;
    int   i, x, y;
    int   yp = 0;
    char  str[FONT_STR_LENGTH+1] = "";
    char  *p;
    int   char_value;
    int   font_width;
    int   font_condensed_flag;
    int   font_italic_flag;
    int   font_elite_width;
    int   font_dblwidth2_shift;
    char  font_name_list[6][5] = { "pica","ital","elit","cond","scrp","scrp" };

    /* set font quality */
    strcat (str, ((face & TEO_PRINTER_FBIT_NLQ) == 0) ? "s" : "c");
                   
    /* try to open the font */
    if ((file = open_font (font_name_list[face>>3], str)) == NULL)
        if ((file = open_font ("pica", str)) == NULL)
            return;

    /* allocate memory of matrix */
    font_buffer = std_free (font_buffer);
    font_buffer = calloc (FONT_CHAR_COUNT*FONT_CHAR_SIZE*2, sizeof(int));
    if (font_buffer == NULL)
    {
        (void)fclose(file);
        return;
    }

    /* allocate memory of proportionnal sizes */
    font_width_buffer = std_free (font_width_buffer);
    font_width_buffer = malloc (FONT_CHAR_COUNT);
    if (font_width_buffer == NULL)
    {
        (void)fclose(file);
        return;
    }
    memset (font_width_buffer, font_fixed_width, FONT_CHAR_COUNT);

    /* skip file comments */
    while (isdigit((int)*str) == 0)
    {
        if (fgets (str, FONT_STR_LENGTH, file) == NULL)
        {
            (void)fclose(file);
            return;
        }
    }

    /* load font parameters */
    p = str;
    font_face            = face;
    font_width           = (int)strtol (p, &p, 0);
    font_height          = (int)strtol (p, &p, 0);
    font_fixed_width     = (int)strtol (p, &p, 0);
    font_elite_width     = (int)strtol (p, &p, 0);
    font_dot_width       = (int)strtol (p, &p, 0);
    font_dot_height      = (int)strtol (p, &p, 0);
    font_dblwidth1_shift = (int)strtol (p, &p, 0);
    font_dblwidth2_shift = (int)strtol (p, &p, 0);
    font_condensed_flag  = (int)strtol (p, &p, 0);
    font_italic_flag     = (int)strtol (p, &p, 0);

    /* positioning if subscript */
    if ((face & TEO_PRINTER_FACE_FONT_MASK) == TEO_PRINTER_FACE_SUBSCRIPT)
        yp = 7;

    /* set font width and dot width if creating condensed needed */
    if (((face & TEO_PRINTER_FACE_FONT_MASK) == TEO_PRINTER_FACE_CONDENSED)
     && (font_condensed_flag != 0))
    {
        font_fixed_width = font_width;
        font_dot_width = 1;
    }

    /* set font fixed width if creating elite needed */
    if ((face & TEO_PRINTER_FACE_FONT_MASK) == TEO_PRINTER_FACE_ELITE)
        font_fixed_width = font_elite_width;
    
    /* set font fixed width if creating double width needed */
    if ((face & TEO_PRINTER_FBIT_DOUBLE_WIDTH) != 0)
        font_fixed_width <<= (font_dblwidth1_shift | font_dblwidth2_shift);

    /* load matrix */
    for (i=0; i<font_count; i++)
    {
        if (fgets (str, FONT_STR_LENGTH, file) == NULL)
        {
            (void)fclose(file);
            return;
        }

        char_value = i;
        if ((char_value >= (128-32)) && (char_equivalence != NULL))
            char_value = char_equivalence[i-(128-32)].chr+0xa0-32;

        font_width_buffer[char_value] = (char)strtol (str, &p, 0);
        if ((face & TEO_PRINTER_FBIT_DOUBLE_WIDTH) != 0)
            font_width_buffer[char_value] <<= font_dblwidth1_shift;

        for (y=yp; y<font_height+yp; y++)
        {
            if (fgets (str, FONT_STR_LENGTH, file) == NULL)
            {
                (void)fclose(file);
                return;
            }

            if ((font_italic_flag)
             && ((font_face & TEO_PRINTER_FACE_FONT_MASK) == TEO_PRINTER_FACE_ITALIC))
            {
                memmove (str+((font_height+yp-y)/3), str, font_width);
                memset (str, '.', ((font_height+yp-y)/3));
            }

            for (x=0; x<font_width; x++)
            {
                if (str[x] == '0')
                {
                    if ((font_dblwidth1_shift)
                     && ((font_face & TEO_PRINTER_FBIT_DOUBLE_WIDTH) != 0))
                    {
                        font_buffer[i*FONT_CHAR_SIZE+(x*2)] |= 1 << y;
                        if (((x*2)+2) < (font_width*2))
                            font_buffer[i*FONT_CHAR_SIZE+((x*2)+2)] |= 1 << y;
                    }
                    else
                    if ((font_dblwidth2_shift)
                     && ((font_face & TEO_PRINTER_FBIT_DOUBLE_WIDTH) != 0))
                    {
                        font_buffer[i*FONT_CHAR_SIZE+(x*2)] |= 1 << y;
                        if (((x*2)+1) < (font_width*2))
                            font_buffer[i*FONT_CHAR_SIZE+((x*2)+1)] |= 1 << y;
                        if (((x*2)+4) < (font_width*2))
                            font_buffer[i*FONT_CHAR_SIZE+((x*2)+4)] |= 1 << y;
                        if (((x*2)+5) < (font_width*2))
                            font_buffer[i*FONT_CHAR_SIZE+((x*2)+5)] |= 1 << y;
                        x++;
                    }
                    else
                        font_buffer[i*FONT_CHAR_SIZE+x] |= 1 << y;
                }
            }
        }
    }
    font_dblwidth1_shift |= font_dblwidth2_shift;
    font_height += yp;
    (void)fclose(file);
}



/* eject_paper:
 *  Ferme les fichiers ouverts et la mémoire de la sortie graphique.
 */
static void eject_paper (void)
{
    if (printer.lprt.gfx_output && (fp_raw != NULL))
        fp_raw = std_fclose (fp_raw);

    if (printer.lprt.txt_output && (fp_text != NULL))
    {
        printer_LineFeed ();
        fp_text = std_fclose (fp_text);
    }

    gfx_eject();
}



/* reinit_printer:
 *  Réinitialise l'imprimante.
 */
static void reinit_printer (void)
{
    load_font(TEO_PRINTER_FACE_PICA);
    font_pica_width = font_fixed_width*font_dot_width;
    paper_pixels_per_inch = 144 * ((printer.chars_per_line == 40) ? 1 : 2);
    printer_LineFeedPerInch(6);
    paper_left_margin = 0;
    paper_width = font_pica_width*printer.chars_per_line;
    paper_width_max = font_pica_width*80;
    paper_height = ((paper_width_max*29.7)/21)+1;
    font_double_width = 0;
    font_underline = 0;
    font_bold = 0;
    screen_pixel_size = (printer.chars_per_line == 40) ? 2 : 3;
}



/* open_printer:
 *  Ouvre l'imprimante.
 */
static void open_printer (void)
{
    int   i;
    FILE  *file = NULL;
    int   value;
    char  str[FONT_STR_LENGTH+1] = "";
    char  *p;

    eject_paper();

    memcpy (&printer.lprt, &teo.lprt, sizeof (struct EMUTEO_LPRT));
    switch (printer.lprt.number)
    {
        case  42 : pr90042_SetParameters(); break;
        case  55 : pr90055_SetParameters(); break;
        case 582 : pr90582_SetParameters(); break;
        case 600 : pr90600_SetParameters(); break;
        case 612 : pr90612_SetParameters(); break;
    }

    /* allocate char list for gfx */
    char_equivalence = std_free (char_equivalence);
    char_equivalence = malloc ((256-0xa0)*sizeof(struct CHAR_EQUIVALENCE));
    if (char_equivalence != NULL)
    {
        font_count = 0;
        for (i=0; i<(256-0xa0); i++)
        {
            char_equivalence[i].chr = 0x00;
            char_equivalence[i].gfx = 0x20;
            char_equivalence[i].txt = 0x20;
        }

        file = open_printer_file ("table");
        if (file != NULL)
        {
            /* read character parameters */
            while (fgets (str, FONT_STR_LENGTH, file) != NULL)
            {
                if (isdigit((int)*str) != 0)
                {
                    i = (int)strtol (str, &p, 0) - 0xa0;
                    
                    char_equivalence[font_count].chr = i;
                    char_equivalence[i].gfx = font_count+0x80;
                    value = (int)strtol (p, &p, 0);
#ifdef UNIX_TOOL
                    char_equivalence[i].txt = value;
#endif
                    value = (int)strtol (p, &p, 0);
#ifndef UNIX_TOOL
                    char_equivalence[i].txt = value;
#endif
                    font_count++;
                }
            }
            (void)fclose(file);
        }
        font_count += (128-32);
    }

    reinit_printer();
    printer_Forget ();
}


/* ------------------------------------------------------------------------- */


/* printer_ClearBuffer:
 *  Efface la mémoire du tampon.
 */
void printer_ClearBuffer (void)
{
    if (paper_buffer != NULL)
        memset (paper_buffer, 0x00, (paper_width_max>>3)*BUFFER_HEIGHT);
    paper_x = paper_left_margin; 
    
}



/* printer_Gfx7Data:
 *  Ecrit une colonne graphique 7 points.
 */
void printer_Gfx7Data (void)
{
    draw_column (printer.data, 7, 4, 4);
}



/* printer_DrawableChar:
 *  Ecrit un caractère affichable.
 */
void printer_DrawableChar (int data)
{
    /* char filtering */
    if ((data & 0x7f) < 32)
        return;

    if (data == 0x7f)
        data = 0x20;

    print_drawable_text_char (data);
    print_drawable_gfx_char (data);
}



/* printer_AsciiCounter:
 *  Initialise une lecture de compteur.
 */
void printer_DigitCounter (int length, void (*jump)())
{
    init_counter (length, jump);
    printer.prog = read_ascii_counter;
}



/* printer_BinaryCounter:
 *  Initialise une lecture de compteur.
 */
void printer_BinaryCounter (int length, void (*jump)())
{
    init_counter (length, jump);
    printer.prog = read_binary_counter;
}



/* printer_Forget:
 *  Oublie la commande courante.
 */
void printer_Forget (void)
{
    printer.prog = printer.restart_prog;
}



/* printer_LineFeedPerInch:
 *  Taille d'interligne spéciale.
 */
void printer_LineFeedPerInch (int nblines)
{
    paper_lf_height = paper_pixels_per_inch / nblines;
    printer_Forget();
}



/* printer_LineFeed144:
 *  Taille d'interligne par 144ème de pouce.
 */
void printer_LineFeed144 (void)
{
    paper_lf_height = (printer.counter_value * paper_pixels_per_inch) / 144;
    printer_Forget();
}



/* printer_LeftMargin:
 *  Marge gauche en caractères.
 */
void printer_LeftMargin (void)
{
    int value = printer.counter_value * font_pica_width;
    
    if (value < 936)
        paper_left_margin = value;
    printer_Forget();
}



/* printer_DotPrintPosition:
 *  Colonne de début d'impression en points.
 */
void printer_DotPrintPosition (void)
{
    int value = paper_left_margin + (printer.counter_value * 2);

    if (value < paper_width)
        paper_x = value;
    printer_Forget ();
}



/* printer_SpaceDot:
 *  Espacement en points.
 */
void printer_SpaceDot (void)
{
    paper_x += printer.counter_value;
    printer_Forget ();
}



/* printer_Gfx8:
 *  Programme l'impression de colonnes graphiques 8 points.
 */
void printer_Gfx8 (void)
{
    gfx_prog = print_gfx8_data;
    gfx_counter = printer.counter_value;
    printer_Forget ();
}



/* printer_Gfx16:
 *  Programme l'impression de colonnes graphiques 16 points.
 */
void printer_Gfx16 (void)
{
    gfx_prog = print_gfx16_data;
    gfx_counter = printer.counter_value << 1;
    printer_Forget ();
}



/* printer_ScreenPrint:
 *  Active la copie graphique d'écran.
 */
void printer_ScreenPrint (void)
{
    last_data_time = mc6809_clock();
    gfx_prog = print_screen_data;
    gfx_counter = 8000;
}



/* printer_Gfx8Repeat:
 *  Programme la répétition d'une colonne graphique 8 points.
 */
void printer_Gfx8Repeat (void)
{
    printer.prog = print_gfx8_repeat;
}



/* printer_Gfx16Repeat:
 *  Programme la répétition d'une colonne graphique 16 points.
 */
void printer_Gfx16Repeat (void)
{
    printer.prog = print_gfx16_repeat;
}



/* printer_PrintPosition:
 *  Position d'impression graphique.
 */
void printer_PrintPosition (void)
{
    if (printer.counter_value < 960)
        paper_x = printer.counter_value << 1;
    printer.mode7 >>= 1;
    printer_Forget();
}



/* printer_CharPositionning:
 *  Positionne l'impression à partir du caractère spécifié.
 */
void printer_CharPositionning (void)
{
    int value = paper_left_margin + (printer.counter_value * ((font_fixed_width
                                    * font_dot_width) << font_double_width));
    if (value < paper_width)
        paper_x = value;
    printer_Forget ();
}



/* printer_PicaPositionning:
 *  Positionne l'impression à partir du caractère 'Pica' spécifié.
 */
void printer_PicaPositionning (void)
{
    paper_x = paper_left_margin + (printer.counter_value * font_pica_width);
    if (paper_x > paper_width)
        paper_x = 0;
    printer.mode7 >>= 1;
    printer_Forget ();
}



/* printer_LineFeed:
 *  Passe à la ligne suivante.
 */
void printer_LineFeed (void)
{
    if (printer.mode7 == 0)
    {
        print_gfx_line_feed ();
#ifndef UNIX_TOOL
        print_text_char (0x0d);
#endif
        print_text_char (0x0a);
    }
    printer.mode7 >>= 1;
}



/* printer_LineStart:
 *  Retour en début de ligne courante.
 */
void printer_LineStart (void)
{
    paper_x = 0;
    printer.mode7 >>= 1;
}    



/* printer_LineStartDip:
 *  Retour en début de ligne courante selon le dip.
 */
void printer_LineStartDip (void)
{
    if (printer.lprt.dip == FALSE)
        printer_LineStart ();
    else
        printer_LineFeed ();
}    



/* printer_FormFeed:
 *  Passe la page.
 */
void printer_FormFeed (void)
{
    int i;

    gfx_eject ();

    for (i=0; i<12; i++)
    {
#ifndef UNIX_TOOL
        print_text_char (0x0d);
#endif
        print_text_char (0x0a);
    }
}



/* printer_SelectFont:
 *  Charge un jeu de caractères d'imprimante.
 */
void printer_SelectFont (int face)
{
    load_font (face);
    printer_Forget ();
}



/* printer_PageLength:
 *  Nombre de lignes par page.
 */
void printer_PageLength (void)
{
    if ((printer.counter_value > 0) && (printer.counter_value < 100))
    {
        eject_paper();
        paper_height = printer.counter_value * (paper_pixels_per_inch / 6);
    }
    printer_Forget();
}



/* printer_Underline:
 *  Passe en mode souligné.
 */
void printer_Underline (void)
{
    font_underline = 1;
    printer_Forget();
}



/* printer_NoUnderline:
 *  Quitte le mode souligné.
 */
void printer_NoUnderline (void)
{
    font_underline = 0;
    printer_Forget();
}



/* printer_Bold:
 *  Passe en mode caractères gras.
 */
void printer_Bold (void)
{
    font_bold = 1;
    printer_Forget();
}



/* printer_Thin:
 *  Passe en mode caractères maigres.
 */
void printer_Thin (void)
{
    font_bold = 0;
    printer_Forget();
}



/* printer_DoubleWidth:
 *  Passe en mode double largeur.
 */
void printer_DoubleWidth (void)
{
    font_double_width = 1;
    if ((font_face & TEO_PRINTER_FBIT_NLQ) == 0)
        load_font(font_face | TEO_PRINTER_FBIT_DOUBLE_WIDTH);
    printer_Forget();
}



/* printer_SimpleWidth:
 *  Passe en mode simple largeur.
 */
void printer_SimpleWidth (void)
{
    font_double_width = 0;
    if ((font_face & TEO_PRINTER_FBIT_NLQ) == 0)
        load_font(font_face & ~TEO_PRINTER_FBIT_DOUBLE_WIDTH);
    printer_Forget();
}



/* printer_Reset:
 *  Réinitialise l'imprimante.
 */
void printer_Reset (void)
{
    reinit_printer();
    printer_Forget();
}



/* printer_ClearGfxMode7:
 *  Sort du mode gfx7.
 */
void printer_ClearGfxMode7(void)
{
    if (printer.mode7 != 0)
    {
        printer.mode7 = 0;
        printer_LineFeed ();
    }
}



/* printer_WriteData:
 *  Ecrit un octet sur le port de donnée.
 */
void printer_WriteData(int mask, int value)
{
    printer.data = (value & mask) | (printer.data & (mask^0xFF));
}



/* printer_Close:
 *  Ferme l'imprimante.
 */
void printer_Close(void)
{
    eject_paper ();
    font_buffer = std_free (font_buffer);
    font_width_buffer = std_free (font_width_buffer);
    printer_Forget ();
    printer_opened = FALSE;
    mc6846.prc &= 0xBF;  /* BUSY à 0 */
}



/* printer_SetStrobe:
 *  Change l'état de la STROBE.
 */
void printer_SetStrobe(int state)
{
    mc6846.prc &= 0xBF;  /* BUSY à 0 */

    if (state)
    {
        if ((LOAD_BYTE(0x602B) & 0x40) != 0)
        {
            open_printer();
            printer_opened = TRUE;
        }
        return;
    }

    if (printer_opened == FALSE)
        return;

    mc6846.prc |= 0x40;  /* BUSY à 1 */

    /* print data if RAW mode selected */
    if (printer.lprt.raw_output)
        print_raw_char (printer.data);

    /* print data if GFX mode with counter */
    if (gfx_counter != 0)
    {
        gfx_counter--;
        (*gfx_prog)();
        return;
    }

    /* print data if GFX mode 7 dots */
    if ((printer.mode7 & printer.data) != 0)
    {
        printer_Gfx7Data ();
        return;
    }

    /* print data */
    (*printer.prog)();
}



/* printer_Init:
 *  Initialise l'imprimante.
 */
void printer_Init(void)
{
    /* trap to get RS.STA value */
    mem.mon.bank[0][0x1B65]=TEO_TRAP_CODE;

    mc6846.prc &= 0xBF;  /* BUSY à 0 */
}

