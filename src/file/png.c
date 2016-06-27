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
 *  Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
 *                          Jérémie Guillaume, François Mouret, Samuel Devulder
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
 *  Module     : png.c
 *  Version    : 1.8.4
 *  Créé par   : François Mouret 24/11/2012
 *  Modifié par: Samuel Devulder 10/02/2013
 *
 *  Gestion des fichiers PNG.
 */

#ifndef SCAN_DEPEND
    #include <stdio.h>
    #include <stdlib.h>
#ifndef DJGPP
   #include <string.h>
   #include <png.h>
   #include <zlib.h>
#endif
#endif

#include "defs.h"
#include "teo.h"
#include "std.h"
#include "errors.h"
#include "file/png.h"

struct FP_PNG {
   FILE *file;
   png_structp png_ptr;
   png_infop info_ptr;
   int setjmp_set;
};

static struct FP_PNG fp_png;


/* png_WriteError:
 *  Generate an error and close a PNG file in write mode.
 */
static int png_WriteError (int err, const char filename[])
{
    error_Message (err, filename);
    png_WriteClose ();
    return TEO_ERROR;
}    



/* png_WriteClose:
 *  Close a PNG file in write mode.
 */
void png_WriteClose (void)
{
    if (fp_png.setjmp_set != 0)
    {
        png_write_end(fp_png.png_ptr, NULL);
        fp_png.setjmp_set = 0;
    }

    if (fp_png.info_ptr != NULL)
    {
        png_free_data(fp_png.png_ptr, fp_png.info_ptr, PNG_FREE_ALL, -1);
        fp_png.info_ptr = NULL;
    }

    if (fp_png.png_ptr != NULL)
    {
        png_destroy_write_struct(&fp_png.png_ptr, (png_infopp)NULL);
        fp_png.png_ptr = NULL;
    }

    if (fp_png.file != NULL)
    {
        fclose (fp_png.file);
        fp_png.file = NULL;
    }
}



/* png_WriteOpen:
 *  Open a PNG file in write mode.
 */
int png_WriteOpen (const char filename[], int width, int height)
{
    int depth = 1;
    int color_type = PNG_COLOR_TYPE_GRAY;

    memset (&fp_png, 0x00, sizeof(struct FP_PNG));

    /* open write mode file */
    fp_png.file = fopen(filename, "wb");
    if (fp_png.file == NULL)
        return png_WriteError (TEO_ERROR_FILE_OPEN, filename);

    /* initialize png write structure */
    fp_png.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (fp_png.png_ptr == NULL)
        return png_WriteError (TEO_ERROR_FILE_WRITE, filename);
    
    /* initialize png info structure */
    fp_png.info_ptr = png_create_info_struct(fp_png.png_ptr);
    if (fp_png.info_ptr == NULL)
        return png_WriteError (TEO_ERROR_FILE_WRITE, filename);
    
    /* setup png exception handling */
    if (setjmp(png_jmpbuf(fp_png.png_ptr)) != 0)
        return png_WriteError (TEO_ERROR_FILE_WRITE, filename);

    fp_png.setjmp_set = 1;

    png_init_io(fp_png.png_ptr, fp_png.file);

    png_set_compression_level(fp_png.png_ptr, Z_BEST_COMPRESSION);

    /* header */
    png_set_IHDR (fp_png.png_ptr, fp_png.info_ptr, width, height,
                  depth, color_type, PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(fp_png.png_ptr, fp_png.info_ptr);
    png_set_invert_mono(fp_png.png_ptr);

    return 0;
}



/* png_WriteRow:
 *  Write a PNG row.
 */
void png_WriteRow (char *ptr)
{
    png_write_row (fp_png.png_ptr, (png_bytep)ptr);
}

