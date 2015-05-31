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
 *  Module     : bmp.c
 *  Version    : 1.8.3
 *  Créé par   : François Mouret 24/11/2012
 *  Modifié par:
 *
 *  Gestion des fichiers BMP.
 */

#ifndef SCAN_DEPEND
    #include <stdio.h>
#endif

#include "defs.h"
#include "teo.h"
#include "errors.h"
#include "std.h"
#include "file/bmp.h"

#define BMP_HEADER_SIZE  54

struct FP_BMP {
    char  *filename;
    int   row_length;
    int   row_number;
    int   biBitCount;
    int   biWidth;
    int   biHeight;
    int   biClrUsed;
    int   biClrImportant;
    int   bfOffBits;
    int   biSizeImage;
};

static struct FP_BMP fp_bmp;
static FILE *file = NULL;



/* fput16: 
 *  Helper pour écrire en little endian un entier 16-bit
 *  quel que soit son format natif.
 */
static int fput16 (int val)
{
    uint8 buffer[2];

    buffer[0] = (uint8)val;
    buffer[1] = (uint8)(val>>8);

    return (int)fwrite(buffer, 1, 2, file) - 2;
}



/* fput32 :
 *  Helper pour écrire en little endian un entier 32-bit
 *  quel que soit son format natif.
 */
static int fput32 (int val)
{
    uint8 buffer[4];

    buffer[0] = (uint8)val;
    buffer[1] = (uint8)(val>>8);
    buffer[2] = (uint8)(val>>16);
    buffer[3] = (uint8)(val>>24);

    return (int)fwrite(buffer, 1, 4, file) - 4;
} 



/* bmp_WriteError:
 *  Generate an error and close a BMP file in write mode.
 */
static int bmp_WriteError (int err, const char filename[])
{
    error_Message (err, filename);
    bmp_WriteClose ();
    file = std_fclose (file);
    return TEO_ERROR;
}    



/* bmp_WriteOpen:
 *  Close a BMP file in write mode.
 */
void bmp_WriteClose (void)
{
    fp_bmp.filename = std_free (fp_bmp.filename);
}



/* bmp_WriteOpen:
 *  Open a BMP file in write mode.
 */
int bmp_WriteOpen (const char filename[], int width, int height,
                                      int nbcolors, int palette[16])
{
    int i;
    int image_size;

    /* set biBitCount */
    switch (nbcolors)
    {
        case  2 : fp_bmp.biBitCount = 1; break;
        case  4 : fp_bmp.biBitCount = 4; break;
        case  8 : fp_bmp.biBitCount = 4; break;
        case 16 : fp_bmp.biBitCount = 4; break;
        default : return bmp_WriteError (TEO_ERROR_BMP_FORMAT, filename);
    }

    /* fill structure */
    fp_bmp.filename   = std_strdup_printf ("%s", filename);
    fp_bmp.row_length = (((width*fp_bmp.biBitCount)+31)>>5)<<2;
    fp_bmp.row_number = height;
    fp_bmp.bfOffBits  = BMP_HEADER_SIZE + (nbcolors * 4);
    image_size        = fp_bmp.row_length * height;

    /* open file */
    if ((file = fopen (filename, "wb")) == NULL)
        return bmp_WriteError (TEO_ERROR_FILE_OPEN, filename);

    /* file header */
    if ((fput16 (0x4D42) != 0)            /* bfType ("BM") */
     || (fput32 (fp_bmp.bfOffBits+fp_bmp.biSizeImage) != 0)  /* bfSize */
     || (fput16 (0) != 0)                 /* bfReserved1 */
     || (fput16 (0) != 0)                 /* bfReserved2 */
     || (fput32 (fp_bmp.bfOffBits) != 0)) /* bfOffBits */
        return bmp_WriteError (TEO_ERROR_FILE_WRITE, filename);

    /* info header */
    if ((fput32 (40) != 0)                /* biSize */
     || (fput32 (width) != 0)             /* biWidth */
     || (fput32 (height) != 0)            /* biHeight */
     || (fput16 (1) != 0)                 /* biPlanes */
     || (fput16 (fp_bmp.biBitCount) != 0) /* biBitCount */
     || (fput32 (0) != 0)                 /* biCompression */
     || (fput32 (image_size) != 0)        /* biSizeImage */  
     || (fput32 (3790) != 0)              /* biXPelsPerMeter */
     || (fput32 (3780) != 0)              /* biYPelsPerMeter */
     || (fput32 (nbcolors) != 0)          /* biClrUsed */
     || (fput32 (nbcolors) != 0))         /* biClrImportant */
        return bmp_WriteError (TEO_ERROR_FILE_WRITE, filename);

    /* color header */
    for (i=nbcolors-1; i>=0; i--)
        if (fput32 (palette[i]) != 0)
            return bmp_WriteError (TEO_ERROR_FILE_WRITE, filename);

    /* bitmap */
    for (i=0; i<image_size; i++)
        if (fputc (0, file) == EOF)
            return bmp_WriteError (TEO_ERROR_FILE_WRITE, filename);

    fflush(file);

    file = std_fclose (file);

    return 0; 
}        



/* bmp_WriteRow:
 *  Write a BMP row.
 */
int bmp_WriteRow (char *ptr, int size)
{
    long int pos;

    fp_bmp.row_number--;
    pos = (long int)(fp_bmp.bfOffBits+fp_bmp.row_length*fp_bmp.row_number);

    if (fp_bmp.filename == NULL)
        return bmp_WriteError (TEO_ERROR_FILE_WRITE, fp_bmp.filename);

    /* open file */
    if ((file = fopen (fp_bmp.filename, "rb+")) == NULL)
        return bmp_WriteError (TEO_ERROR_FILE_WRITE, fp_bmp.filename);

    /* don't overwrite the header !! */
    if (fp_bmp.row_number < 0)
        return bmp_WriteError (TEO_ERROR_FILE_WRITE, fp_bmp.filename);

    /* seek to and write the row */
    if ((fseek (file, pos, SEEK_SET) != 0)
     || (fwrite (ptr, 1, (size_t)size, file) != (size_t)size))
        return bmp_WriteError (TEO_ERROR_FILE_WRITE, fp_bmp.filename);

    file = std_fclose (file);

    return 0;
}

