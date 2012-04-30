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
 *  Version    : 1.8.1
 *  Créé par   : Eric Botcazou 22/03/2001
 *  Modifié par: Eric Botcazou 30/03/2001
 *               François Mouret 14/04/2012
 *
 *  Emulation des imprimantes PR90-055, PR90-600 et PR90-612.
 */


#ifndef SCAN_DEPEND
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <ctype.h>
   #include <png.h>
#endif

#include "intern/printer.h"
#include "mc68xx/mc6809.h"
#include "mc68xx/mc6846.h"
#include "intern/hardware.h"

#ifdef UNIX_TOOL
#   define SLASH "/"
#else
#   define SLASH "\\"
#endif

#define FONT_CHAR_MAX (256-32)
#define FONT_WIDTH_MAX  32
#define FONT_BUFFER_SIZE (FONT_CHAR_MAX*sizeof(int)*FONT_WIDTH_MAX)

enum {
    FACE_NORMAL = 0,
    FACE_ITALIC,
    FACE_PROPORTIONAL,
    FACE_SUPERSCRIPT,
    FACE_SUBSCRIPT
};

enum {
    GFX8_RUN = 0,
    GFX16_RUN,
    SCREENPRINT_RUN
};

enum {
    BINARY_VALUE = 0,
    DIGIT_VALUE
};

struct LPRT_COUNTER
{
    int  mode;
    int  count;
    int  length;
    int  value;
    int  valid;
    void (*jump)();
};

struct LPRT_PAPER
{
    int  type;
    int  x;
    int  y;
    int  width;
    int  height;
    int  left_margin;
    int  lf_size;
    int  pixels_per_inch;
    int  chars_per_line;
    char *buffer;
};

struct LPRT_FONT
{
    int  type;
    char name[6];
    int  count;
    int  width;
    int  height;
    int  fixed_width;
    int  dot_width;
    int  dot_height;
    int  *buffer;
    char prop_width[FONT_CHAR_MAX];
};


static FILE *fp_text = NULL;
static FILE *fp_raw = NULL;
static int  file_counter = 0;
static int  data;
static int  pica_width;
static int  double_width;
static int  underline;
static int  bold;
static mc6809_clock_t last_data_time = 0;
static int screenprint_data_delay = 0;
static struct LPRT_PAPER paper;
static struct LPRT_FONT font;
static struct LPRT_COUNTER counter;
static struct LPRT_GUI lprt;
static int  val16 = 0;
static int  flag16 = 0;
static int  gfx7_mode = 0;
static void (*gfx_prog)();
static int  gfx_counter = 0;

static int  printer_open_state = FALSE;
static void (*prog)();
static void (*restart_prog)();

static void pr90055_first (void);
static void pr906xx_first (void);



/* print_raw_char:
 *  Ecrit un caractère brut.
 */
static void print_raw_char (int c)
{
    char path[MAX_PATH+1] = "";

    if (!lprt.raw_output)
        return;

    if (fp_raw == NULL)
    {
#ifdef DJGPP
        (void)sprintf (path, "%s%slprt%03d.bin", lprt.folder, SLASH, file_counter);
#else
        (void)snprintf (path, MAX_PATH, "%s%slprt%03d.bin", lprt.folder, SLASH, file_counter);
#endif
        fp_raw = fopen (path, "wb");
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
    char path[MAX_PATH+1] = "";

    if (!lprt.txt_output)
        return;

    if (fp_text == NULL)
    {
#ifdef DJGPP
        (void)sprintf (path, "%s%slprt%03d.txt", lprt.folder, SLASH, file_counter);
#else
        (void)snprintf (path, MAX_PATH, "%s%slprt%03d.txt", lprt.folder, SLASH, file_counter);
#endif
        fp_text = fopen (path, "wb");
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
    if (c >= 0xa0)
    {
        switch (c)
        {
            case 0xa1 : c = (int)'ä'; break;
            case 0xa2 : c = (int)'à'; break;
            case 0xa3 : c = (int)'â'; break;
            case 0xa4 : c = (int)'É'; break;
            case 0xa5 : c = (int)'È'; break;
            case 0xa6 : c = (int)'é'; break;
            case 0xa7 : c = (int)'è'; break;
            case 0xa8 : c = (int)'ê'; break;
            case 0xab : c = (int)'ß'; break;
            case 0xac : c = (int)'ë'; break;
            case 0xad : c = (int)'î'; break;
            case 0xae : c = (int)'ï'; break;
            case 0xb1 : c = (int)'Ä'; break;
            case 0xb2 : c = (int)'Ö'; break;
            case 0xb3 : c = (int)'ô'; break;
            case 0xb4 : c = (int)'Ü'; break;
            case 0xb5 : c = (int)'ü'; break;
            case 0xb6 : c = (int)'ù'; break;
            case 0xb7 : c = (int)'û'; break;
            case 0xb9 : c = (int)'£'; break;
            case 0xbb : c = (int)'§'; break;
            case 0xc2 : c = (int)'¡'; break;
            case 0xc3 : c = (int)'¿'; break;
            case 0xc4 : c = (int)'Ñ'; break;
            case 0xc5 : c = (int)'ñ'; break;
            case 0xd0 : c = (int)'ö'; break;
            case 0xd1 : c = (int)'ç'; break;
            case 0xd2 : c = (int)'°'; break;
            default   : c = (int)' '; break;
        }
    }
    print_text_char (c);
}


#ifdef DJGPP

/* fputw: 
 *  Helper pour écrire en little endian un entier 16-bit
 *  quel que soit son format natif.
 */
static void fputw(int val, FILE *file)
{
    unsigned char buffer[2];

    buffer[0] = (unsigned char) val;
    buffer[1] = (unsigned char) (val>>8);

    fwrite(buffer, 1, 2, file);
}



/* fputl:
 *  Helper pour écrire en little endian un entier 32-bit
 *  quel que soit son format natif.
 */
static void fputl(int val, FILE *file)
{
    unsigned char buffer[4];

    buffer[0] = (unsigned char) val;
    buffer[1] = (unsigned char) (val>>8);
    buffer[2] = (unsigned char) (val>>16);
    buffer[3] = (unsigned char) (val>>24);

    fwrite(buffer, 1, 4, file);
} 



/* gfx_eject:
 *  Ejecte le tirage BMP.
 */
static void gfx_eject (void)
{
    FILE *file;
    int y;
    int ppw = paper.width/8;
    int pph = paper.height;
    char path[MAX_PATH+1] = "";

    paper.x = paper.left_margin;
    paper.y = 0;

    if (!lprt.gfx_output || (paper.buffer == NULL))
        return;

    (void)sprintf (path, "%s%slprt%03d.bmp", lprt.folder, SLASH, file_counter);

    file = fopen (path, "wb");
    if (file == NULL)
        return;

    /* file header */
    fputw(0x4D42, file);      /* bfType ("BM")  */
    fputl(62+ppw*pph, file);  /* bfSize         */
    fputw(0, file);           /* bfReserved1    */
    fputw(0, file);           /* bfReserved2    */
    fputl(62, file);          /* bfOffBits      */

    /* info header */
    fputl(40, file);          /* biSize          */
    fputl(paper.width, file); /* biWidth         */
    fputl(pph, file);         /* biHeight        */
    fputw(1, file);           /* biPlanes        */
    fputw(1, file);           /* biBitCount      */
    fputl(0, file);           /* biCompression   */
    fputl(ppw*pph, file);     /* biSizeImage */  
    fputl(3790, file);        /* biXPelsPerMeter */
    fputl(3780, file);        /* biYPelsPerMeter */
    fputl(0, file);           /* biClrUsed       */
    fputl(0, file);           /* biClrImportant  */

     /* color header */ 
    fputl(0xFFFFFF, file);    /* bcRGBBackground */
    fputl(0, file);           /* bcRGBForeground */

    /* bitmap */
    for (y=pph-1; y>=0; y--)
    {
        (void)fwrite (paper.buffer+y*ppw, 1, ppw, file);
        fflush(file);
    }

    fclose(file);
    free (paper.buffer);
    paper.buffer = NULL;
}

#else

/* gfx_eject:
 *  Ejecte le tirage PNG.
 */
static void gfx_eject (void)
{
    FILE *file;
    int y;
    int ppw = paper.width/8;
    int pph = paper.height;
    png_structp png_ptr;
    png_infop info_ptr;
    char path[MAX_PATH+1] = "";
    

    paper.x = paper.left_margin;
    paper.y = 0;

    if (!lprt.gfx_output || (paper.buffer == NULL))
        return;

    (void)snprintf (path, MAX_PATH, "%s%slprt%03d.png", lprt.folder, SLASH, file_counter);
	
    file = fopen(path, "wb");
    if (file != NULL)
    {
       /* initialize png write structure */
       png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
       if (png_ptr != NULL)
       {
          /* initialize png info structure */
          info_ptr = png_create_info_struct(png_ptr);
          if (info_ptr != NULL)
          {
             /* setup png exception handling */
             if (setjmp(png_jmpbuf(png_ptr)) == 0)
             {
                png_init_io(png_ptr, file);

                /* header */
                png_set_IHDR (png_ptr, info_ptr, paper.width, paper.height,
                             1, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
                png_write_info(png_ptr, info_ptr);
                png_set_invert_mono(png_ptr);

                /* bitmap */
                for (y=0; y<pph; y++)
                    png_write_row (png_ptr, (png_bytep)(paper.buffer+y*ppw));

                /* finished write */
                png_write_end(png_ptr, NULL);
             }
             png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
          }
          png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
       }
       fclose (file);
    }
    free (paper.buffer);
    paper.buffer = NULL;
}

#endif


/* print_gfx_line_feed:
 *  Effectue un passage de ligne en mode graphique.
 */
static void print_gfx_line_feed (void)
{
    paper.y += paper.lf_size;
    paper.x = paper.left_margin;
    if (paper.y + paper.lf_size > paper.height)
    {
        gfx_eject ();
        file_counter++;
    }
}



/* draw_pixel:
 *  Ecrit un pixel.
 */
static void draw_pixel (int y, int width, int height)
{
    int  pixel;
    char *p;
    char *pline;
    int px, py;

    if (!lprt.gfx_output)
        return;

    if (paper.buffer == NULL)
    {
        paper.buffer = calloc (1, (paper.width/8)*paper.height);
        if (paper.buffer == NULL)
            return;
    }

    pline = paper.buffer + ((paper.y + y) * (paper.width >> 3));

    for (px=0; px<width; px++)
    {
        if ((paper.x + px) < paper.width)
        {
            p = pline + ((paper.x + px) >> 3);
            pixel = 1 << (~(paper.x + px) & 7);
            for (py=0; py<height; py++)
            {
                if ((paper.y + py + y) < paper.height)
                    *p |= pixel;
                p += (paper.width >> 3);
            }
        }
    }
}



/* draw_column:
 *  Ecrit une colonne de points.
 */
static void draw_column (int column, int length,
                           int pixel_width, int pixel_height)
{
    int y;

    for (y = 0; y < length; y++)
    {
        if ((column & (1 << y)) != 0)
        {
            draw_pixel (y * pixel_height, pixel_width, pixel_height);
        }
    }
    paper.x += pixel_width;
}



/* print_drawable_gfx_char:
 *  Ecrit un caractère affichable.
 */
static void print_drawable_gfx_char (int c)
{
    int x;
    int x_max;
    int dot_width = font.dot_width << double_width;
    int dot_height = font.dot_height << 1;
    int column;

    /* special chars */
    if (c >= 0xa0)
    {
        switch (c)
        {
            case 0xa1 : c = 0x80; break;
            case 0xa2 : c = 0x81; break;
            case 0xa3 : c = 0x82; break;
            case 0xa4 : c = 0x83; break;
            case 0xa5 : c = 0x84; break;
            case 0xa6 : c = 0x85; break;
            case 0xa7 : c = 0x86; break;
            case 0xa8 : c = 0x87; break;
            case 0xab : c = 0x88; break;
            case 0xac : c = 0x89; break;
            case 0xad : c = 0x8a; break;
            case 0xae : c = 0x8b; break;
            case 0xb1 : c = 0x8c; break;
            case 0xb2 : c = 0x8d; break;
            case 0xb3 : c = 0x8e; break;
            case 0xb4 : c = 0x8f; break;
            case 0xb5 : c = 0x90; break;
            case 0xb6 : c = 0x91; break;
            case 0xb7 : c = 0x92; break;
            case 0xb9 : c = 0x93; break;
            case 0xbb : c = 0x94; break;
            case 0xc2 : c = 0x95; break;
            case 0xc3 : c = 0x96; break;
            case 0xc4 : c = 0x97; break;
            case 0xc5 : c = 0x98; break;
            case 0xd0 : c = 0x99; break;
            case 0xd1 : c = 0x9a; break;
            case 0xd2 : c = 0x9b; break;
            default   : c = 0x20; break;
        }
    }

    /* compute width of char */
    x_max = (font.type == FACE_PROPORTIONAL) ? (int)font.prop_width[data-0x20] : font.fixed_width;

    /* CR if End Of Line */
    if (paper.x + x_max > paper.width)
        print_gfx_line_feed ();

    /* 2 pixels more if italic */
    if (font.type == FACE_ITALIC)
        x_max += 2;

    /* draw char */
    for (x = 0; x < x_max; x++)
    {
        if (paper.x + x < paper.width)
        {
            column = (font.buffer == NULL) ? 0 : font.buffer[(c-0x20)*FONT_WIDTH_MAX+x];
    
            if (underline != 0)
                column |= 1  << (font.height - 1);

            draw_column (column, font.height, dot_width, dot_height);

            if ((bold != 0) && (paper.x + x + 2 < paper.width))
            {
                draw_column (column, font.height, 2, dot_height);
                paper.x -= 2;
            }
        }
    }
    /* restore cursor if italic */
    if (font.type == FACE_ITALIC)
        paper.x -= 2;
}



/* print_drawable_char:
 *  Ecrit un caractère affichable.
 */
static void print_drawable_char (void)
{
     int c = data;

    /* char filtering */
    if (((c & 0x7f) < 32)
     || ((c >= 0xa0) && (lprt.number != 612)))
        return;

    if (c == 0x7f)
        c = 0x20;

    print_drawable_text_char (c);
    print_drawable_gfx_char (c);
}



/* PR_forget:
 *  Oublie la commande courante.
 */
static void PR_forget (void)
{
    prog = restart_prog;
}



/* read_value:
 *  Récupère un compteur.
 */
static void read_counter (void)
{
    counter.count++;
    switch (counter.mode)
    {
        case DIGIT_VALUE :
            counter.value = (counter.value * 10) + (data - '0');
            counter.valid += (isdigit((int)data) != 0) ? 1 : 0;
            break;
        case BINARY_VALUE :
            counter.value = (counter.value << 8) + data;
            counter.valid++;
            break;
    }
    if (counter.count == counter.length)
    {
        if (counter.valid == counter.length)
            (*counter.jump)();
        else
            PR_forget ();
    }
}



/* set_read_counter:
 *  Initialise une lecture de compteur.
 */
static void set_read_counter (int mode, int length, void (*jump)())
{
    counter.value = 0;
    counter.count = 0;
    counter.valid = 0;
    counter.mode = mode;
    counter.length = length;
    counter.jump = jump;
    prog = read_counter;
}



/* PR_left_margin:
 *  Marge gauche en caractères.
 */
static void PR_left_margin (void)
{
    int value = counter.value * pica_width;
    
    if (value < 936)
        paper.left_margin = value;
    PR_forget();
}



/* PR_dot_print_position:
 *  Colonne de début d'impression en points.
 */
static void PR_dot_print_position (void)
{
    int value = paper.left_margin + (counter.value * 2);

    if (value < paper.width)
        paper.x = value;
    PR_forget ();
}



/* PR_space_dot:
 *  Espacement en points.
 */
static void PR_space_dot (void)
{
    paper.x += counter.value;
    PR_forget ();
}



/* PR_line_feed_per_inch:
 *  Taille d'interligne spéciale.
 */
static void PR_line_feed_per_inch (int nblines)
{
    paper.lf_size = paper.pixels_per_inch / nblines;
    PR_forget();
}



/* PR_line_feed_144:
 *  Taille d'interligne par 144ème de pouce.
 */
static void PR_line_feed_144 (void)
{
    paper.lf_size = (counter.value * paper.pixels_per_inch) / 144;
    PR_forget();
}



/* print_gfx7_data:
 *  Ecrit une colonne graphique 7 points.
 */
static void print_gfx7_data (void)
{
    draw_column (data, 7, 4, 4);
}



/* PR_gfx7:
 *  Programme le mode graphique 7 points.
 */
static void PR_gfx7 (void)
{
    gfx7_mode = 0x80;
    PR_forget ();
}



/* print_gfx8_data:
 *  Ecrit une colonne graphique 8 points.
 */
static void print_gfx8_data (void)
{
    draw_column (data, 8, 2, 4);
}



/* PR_gfx8:
 *  Programme l'impression de colonnes graphiques 8 points.
 */
static void PR_gfx8 (void)
{
    gfx_prog = print_gfx8_data;
    gfx_counter = counter.value;
    PR_forget ();
}



/* print_gfx16_data:
 *  Ecrit une colonne graphique 16 points.
 */
static void print_gfx16_data (void)
{
    if ((flag16 ^= 1) == 1)
    {
        val16 = data;
        gfx_counter++;
    }
    else
    {
        val16 |= data << 8;
        draw_column (val16, 16, 2, 2);
    }
}



/* PR_gfx16:
 *  Programme l'impression de colonnes graphiques 16 points.
 */
static void PR_gfx16 (void)
{
    gfx_prog = print_gfx16_data;
    gfx_counter = counter.value;
    PR_forget ();
}



/* print_screen_data:
 *  Ecrit un octet graphique 8 points.
 */
static void print_screen_data (void)
{
    int i;
    int pixel_size = (lprt.number >= 600) ? 3 : 2;
    mc6809_clock_t delay = (screenprint_data_delay * TO8_CPU_FREQ) / 1000;

    if (mc6809_clock() - last_data_time > delay)
    {
        gfx_counter = 0;
        return;
    }

    last_data_time = mc6809_clock();

    for (i=0x80; i>0; i>>=1)
    {
        if (data & i)
           draw_pixel (0, pixel_size, pixel_size);
        paper.x += pixel_size;
    }
    if ((gfx_counter % 40) == 0)
    {
        paper.x = 0;
        paper.y += pixel_size;
    }
}



/* PR_screenprint:
 *  Active la copie graphique d'écran.
 */
static void PR_screenprint (void)
{
    last_data_time = mc6809_clock();
    gfx_prog = print_screen_data;
    gfx_counter = 8000;
}



/* PR_repeat_gfx7:
 *  Programme la répétition d'une colonne graphique 7 points.
 */
static void PR_repeat_gfx7 (void)
{
    int i;

    for (i=0; i<counter.value; i++)
        print_gfx7_data ();
    PR_forget ();
}
          


/* PR_repeat_gfx8:
 *  Programme la répétition d'une colonne graphique 8 points.
 */
static void PR_repeat_gfx8 (void)
{
    int i;

    for (i=0; i<counter.value; i++)
        print_gfx8_data ();
    PR_forget ();
}
          


/* PR_repeat_gfx16:
 *  Programme la répétition d'une colonne graphique 16 points.
 */
static void PR_repeat_gfx16 (void)
{
    int i;

    val16 = ((val16 << 8) | data) & 0xffff;

    if ((flag16 ^= 1) == 0)
        return;

    for (i=0; i<counter.value; i++)
        draw_column (val16, 16, 2, 2);
    PR_forget ();
}



/* PR_print_position:
 *  Position d'impression graphique.
 */
static void PR_print_position (void)
{
    if (counter.value < 960)
        paper.x = counter.value * 2;
    PR_forget();
}



/* PR_char_positionning:
 *  Positionne l'impression à partir du caractère spécifié.
 */
static void PR_char_positionning (void)
{
    int value = paper.left_margin + (counter.value * ((font.fixed_width
                                    * font.dot_width) << double_width));
    if (value < paper.width)
        paper.x = value;
    PR_forget ();
}



/* PR_pica_positionning:
 *  Positionne l'impression à partir du caractère 'Pica' spécifié.
 */
static void PR_pica_positionning (void)
{
    paper.x = paper.left_margin + (counter.value * pica_width);
    if (paper.x > paper.width)
        paper.x = 0;
    PR_forget ();
}



/* PR_line_feed:
 *  Passe à la ligne suivante.
 */
static void PR_line_feed (void)
{
    if (gfx7_mode == 0)
    {
        print_gfx_line_feed ();
        print_text_char (0x0d);
        print_text_char (0x0a);
    }
}        



/* PR_line_start:
 *  Retour en début de ligne courante.
 */
static void PR_line_start (void)
{
    paper.x = 0;
}    



/* PR_line_start:
 *  Retour en début de ligne courante.
 */
static void PR_line_start_dip (void)
{
    if (lprt.dip == FALSE)
        PR_line_start ();
    else
        PR_line_feed ();
}    



/* PR_form_feed:
 *  Passe la page.
 */
static void PR_form_feed (void)
{
    int i;

    gfx_eject ();

    for (i=0; i<12; i++)
    {
        print_text_char (0x0d);
        print_text_char (0x0a);
    }
    file_counter++;
} 



/* load_font:
 *  Charge un générateur de caractères imprimante.
 */
static void load_font (char *filename, int face)
{
    FILE *file;
    int  i, x, y;
    int  xp = 0, yp = 0;
    char str[150+1] = "";
    char *p;
    char *res;

    if (font.buffer == NULL)
    {
        font.buffer = malloc (FONT_CHAR_MAX*sizeof(int)*FONT_WIDTH_MAX);
        if (font.buffer == NULL)
            return;
    }

    /* open file */
#ifdef DJGPP
    (void)sprintf (str, "fonts%s%s%03d.txt", SLASH, filename, lprt.number);
#else
    (void)snprintf (str, 150, "fonts%s%s%03d.txt", SLASH, filename, lprt.number);
#endif
    
    if ((face == FACE_SUBSCRIPT) || (face == FACE_SUPERSCRIPT))
    {
#ifdef DJGPP
        (void)sprintf (str, "fonts%s%s.txt", SLASH, filename);
#else
        (void)snprintf (str, 150, "fonts%s%s.txt", SLASH, filename);
#endif
    
        if (face == FACE_SUBSCRIPT) yp = 7;
    }
    file = fopen (str, "rb");
    if (file == NULL)
        return;

    /* skip comments */
    while ((*str < '0') || (*str > '9'))
        res=fgets (str, 150, file);

    /* load font parameters */
    font.type = face;
#ifdef DJGPP
    (void)sprintf (font.name, "%s", filename);
#else
    (void)snprintf (font.name, 6, "%s", filename);
#endif
    font.count  = (int)strtol (str, &p, 10);
    font.width  = (int)strtol (p, &p, 10);
    font.height = (int)strtol (p, &p, 10);
    font.fixed_width = (int)strtol (p, &p, 10);
    font.dot_width = (int)strtol (p, &p, 10);
    font.dot_height = (int)strtol (p, &p, 10);
    memset (font.prop_width, font.fixed_width, FONT_CHAR_MAX);
    memset (font.buffer, 0x00, FONT_CHAR_MAX*sizeof(int)*FONT_WIDTH_MAX);

    /* load matrix */
    for (i=0; i<font.count; i++)
    {
        res=fgets (str, 150, file);
        font.prop_width[i] = (char)strtol (str, &p, 10);

        for (y=yp; y<font.height+yp; y++)
        {
             res=fgets (str, 150, file);

             if (face == FACE_ITALIC)
                 xp = (font.height+yp-1-y)/2;

             for (x=xp; x<font.width+xp; x++)
                  if (str[x-xp] == '0')
                      font.buffer[i*FONT_WIDTH_MAX+x] |= 1 << y;
        }
    }
    font.height += yp;
    fclose(file);
}




/* PR_load_font:
 *  Charge un jeu de caractères d'imprimante.
 */
static void PR_load_font (char *filename, int face)
{
    load_font (filename, face);
    PR_forget ();
}



/* eject_paper:
 *  Ferme les fichiers ouverts et la mémoire de la sortie graphique.
 */
static void eject_paper (void)
{
    if (lprt.gfx_output && (fp_raw != NULL))
    {
        fclose (fp_raw);
        fp_raw = NULL;
    }
        
    if (lprt.txt_output && (fp_text != NULL))
    {
        PR_line_feed ();
        fclose (fp_text);
        fp_text = NULL;
    }

    gfx_eject();
    
    file_counter++;
}



/* PR_page_length:
 *  Nombre de lignes par page.
 */
static void PR_page_length (void)
{
    if ((counter.value > 0) && (counter.value < 100))
    {
        eject_paper();
        paper.height = counter.value * (paper.pixels_per_inch / 6);
    }
    PR_forget();
}



/* PR_underline:
 *  Passe en mode souligné.
 */
static void PR_underline (void)
{
    underline = 1;
    PR_forget();
}



/* PR_no_underline:
 *  Quitte le mode souligné.
 */
static void PR_no_underline (void)
{
    underline = 0;
    PR_forget();
}



/* PR_bold:
 *  Passe en mode caractères gras.
 */
static void PR_bold (void)
{
    bold = 1;
    PR_forget();
}



/* PR_thin:
 *  Passe en mode caractères maigres.
 */
static void PR_thin (void)
{
    bold = 0;
    PR_forget();
}



/* PR_double_width:
 *  Passe en mode double largeur.
 */
static void PR_double_width (void)
{
    double_width = 1;
    if (strcmp (font.name, "picas") == 0)
        load_font("elons", FACE_NORMAL);
    PR_forget();
}



/* PR_simple_width:
 *  Passe en mode simple largeur.
 */
static void PR_simple_width (void)
{
    double_width = 0;
    if (strcmp (font.name, "elons") == 0)
        load_font("picas", FACE_NORMAL);
    PR_forget();
}



/* reinit_printer:
 *  Réinitialise l'imprimante.
 */
static void reinit_printer (void)
{
    eject_paper();
    load_font("picas", FACE_NORMAL);
    pica_width = 24;
    paper.pixels_per_inch = 288;
    PR_line_feed_per_inch(6);
    paper.left_margin = 0;
    paper.width = pica_width*paper.chars_per_line;
    paper.height = ((paper.width*29.7)/21)+1;
    double_width = 0;
    underline = 0;
    bold = 0;
}



/* PR_reset:
 *  Réinitialise l'imprimante.
 */
static void PR_reset (void)
{
    reinit_printer();
    PR_forget();
}


/* ----------------------- PR90-600 / PR90-612 ----------------------- */


/* clear_gfx7_mode:
 *  Sort du mode gfx7.
 */
void clear_gfx7_mode(void)
{
    if (gfx7_mode != 0)
    {
        gfx7_mode = 0;
        PR_line_feed ();
    }
}


/* pr906xx_escape:
 *  Traite le code introduit par ESC pour les PR90-600 et PR90-612.
 */
static void pr906xx_escape (void)
{
    switch (data)
    {
        case 16 : set_read_counter (BINARY_VALUE, 2, PR_print_position); return;
    }

    clear_gfx7_mode();
    
    switch ((char)data)
    {
        case 14 : PR_double_width(); break;
        case 15 : PR_simple_width(); break;
        case 'N': PR_load_font((lprt.nlq) ? "picac" : "picas", FACE_NORMAL); break;
        case 'E': PR_load_font((lprt.nlq) ? "elitc" : "elits", FACE_NORMAL); break;
        case 'C': PR_load_font("condc", FACE_NORMAL); break;
        case 'b': PR_load_font((lprt.nlq) ? "picac" : "picas", FACE_ITALIC); break;
        case 'p': PR_load_font((lprt.nlq) ? "picac" : "picas", FACE_PROPORTIONAL); break;
        case 'H': PR_load_font("picac", FACE_NORMAL); break;
        case 'Q': PR_load_font("elitc", FACE_NORMAL); break;
        case 'B': PR_load_font("picac", FACE_ITALIC); break;
        case 'P': PR_load_font("picac", FACE_PROPORTIONAL); break;
        case 'U': PR_load_font("tiny" , FACE_SUPERSCRIPT); break;
        case 'D': PR_load_font("tiny" , FACE_SUBSCRIPT); break;
        case '6': PR_line_feed_per_inch(6); break;
        case '8': PR_line_feed_per_inch(8); break;
        case '9': PR_line_feed_per_inch(9); break;
        case '7': PR_line_feed_per_inch(12); break;
        case 'T': set_read_counter (DIGIT_VALUE, 2, PR_line_feed_144); break;
        case 'Z': set_read_counter (DIGIT_VALUE, 3, PR_page_length); break;
        case 'X': PR_underline(); break;
        case 'Y': PR_no_underline(); break;
        case '#': PR_bold(); break;
        case '$': PR_thin(); break;
        case 'G': set_read_counter (DIGIT_VALUE, 3, PR_gfx8); break;
        case 'I': set_read_counter (DIGIT_VALUE, 3, PR_gfx16); break;
        case 'V': set_read_counter (DIGIT_VALUE, 3, PR_repeat_gfx8); break;
        case 'W': set_read_counter (DIGIT_VALUE, 3, PR_repeat_gfx16); break;
        case '@': PR_reset(); break;
        case 'L': set_read_counter (DIGIT_VALUE , 3, PR_left_margin); break;
        case 'F': set_read_counter (DIGIT_VALUE , 3, PR_dot_print_position); break;
        case 'S': set_read_counter (DIGIT_VALUE , 1, PR_space_dot); break;
        default : PR_forget(); break;
    }
}



/* pr906xx_first:
 *  Traite le code de tête pour les PR90-600 et PR90-612.
 */
static void pr906xx_first (void)
{
    val16 = 0;
    flag16 = 0;

    switch (data)
    {
        case 10 : PR_line_feed(); return;
        case 13 : PR_line_start_dip(); return;
        case 20 : PR_line_start(); return;
        case 16 : set_read_counter (DIGIT_VALUE, 2, PR_pica_positionning); return;
        case 28 : PR_repeat_gfx7(); return;
        case 27 : prog = pr906xx_escape; return;
    }

    clear_gfx7_mode();

    switch (data)
    {
        case 12 : PR_form_feed(); break;
        case 14 : PR_double_width(); break;
        case 15 : PR_simple_width(); break;
        case 24 : break;
        case 7  : PR_screenprint(); break;
        case 8  : PR_gfx7(); break;
        case 18 : set_read_counter (DIGIT_VALUE, 3, PR_char_positionning); break;
        default : print_drawable_char (); break;
    }
}


/* ---------------------------- PR90-055 ---------------------------- */


/* PR_mo_line_feed:
 *  Passe un groupe d'interligne.
 */
static void PR_mo_line_feed (void)
{
    int i;

    for (i=data&0x7f; i>0; i--)
         PR_line_feed ();

    PR_forget ();
}



/* pr90055_escape:
 *  Traite le code introduit par ESC pour la PR90-055.
 */
static void pr90055_escape (void)
{
    switch ((char)data)
    {
        case '9': PR_line_feed_per_inch(9); break;
        case '6': PR_line_feed_per_inch(6); break;
        default : PR_forget(); break;
    }
}



/* pr90055_first:
 *  Traite le code de tête pour la PR90-055.
 */
static void pr90055_first (void)
{
    switch (data)
    {
        case 13 : PR_line_start(); break;
        case 20 : PR_line_start(); break;
        case 10 : PR_line_feed(); break;
        case 18 : PR_line_feed(); break;
        case 12 : PR_form_feed(); break;
        case 14 : PR_double_width(); break;
        case 15 : PR_simple_width(); break;
        case 7  : PR_screenprint(); break;
        case 27 : prog = pr90055_escape; break;
        case 11 : prog = PR_mo_line_feed; break;
        default : print_drawable_char (); break;
    }
}


/* --------------------- */

/* printer_Open:
 *  Ouvre l'imprimante.
 */
static void printer_Open (void)
{
    memcpy (&lprt, &gui->lprt, sizeof (struct LPRT_GUI));
    switch (lprt.number)
    {
        case  55 : paper.chars_per_line = 40;
                   screenprint_data_delay = 100;
                   prog = pr90055_first;
                   break;

        case 600 : paper.chars_per_line = 80;
                   screenprint_data_delay = 80;
                   prog = pr906xx_first;
                   break;

        case 612 : paper.chars_per_line = 80;
                   screenprint_data_delay = 100;
                   prog = pr906xx_first;
                   break;
    }
    reinit_printer();
    restart_prog = prog;
}


/**********************************/
/******** partie publique *********/
/**********************************/


/* printer_WriteData:
 *  Ecrit un octet sur le port de donnée.
 */
void printer_WriteData(int mask, int value)
{
    data = (value & mask) | (data & (mask^0xFF));
}



/* printer_Close:
 *  Ferme l'imprimante.
 */
void printer_Close(void)
{
    eject_paper ();
    if (font.buffer != NULL)
    {
        free (font.buffer);
        font.buffer = NULL;
    }
    prog = restart_prog;
    printer_open_state = FALSE;
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
            printer_Open();
            printer_open_state = TRUE;
        }
        return;
    }

    if (printer_open_state == FALSE)
        return;

    mc6846.prc |= 0x40;  /* BUSY à 1 */

    /* print data if RAW mode selected */
    if (lprt.raw_output)
        print_raw_char (data);

    /* print data if GFX mode with counter */
    if (gfx_counter != 0)
    {
        gfx_counter--;
        (*gfx_prog)();
        return;
    }

    /* print data if GFX mode 7 dots */
    if (((gfx7_mode & data) != 0) && (prog != read_counter))
    {
        print_gfx7_data ();
        return;
    }

    /* print data */
    (*prog)();
}



/* InitPrinter:
 *  Initialise l'imprimante.
 */
void InitPrinter(void)
{
    /* trap pour récuparation de RS.STA */
    mem.mon.bank[0][0x1B65]=TO8_TRAP_CODE;

    memset (&paper, 0x00, sizeof (struct LPRT_PAPER));
    memset (&font, 0x00, sizeof (struct LPRT_FONT));
    gui->lprt.number = 55;
    gui->lprt.gfx_output = TRUE;
}

