/*********************************************************
 * cc90hfe (c) Teo Developers
 *********************************************************
 *
 *  Copyright (C) 2012-2017 Fran�ois Mouret
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
 *  Module     : encode.c
 *  Version    : 0.7.0
 *  Cr�� par   : Fran�ois Mouret 27/02/2013
 *  Modifi� par:
 *
 *  Strings encoding.
 */


#include "defs.h"
#include "encode.h"

static int windows1252_to_utf8[] = {
    0xe282ac, 0x000020, 0xe2809a, 0x00c692,
    0xe2809e, 0xe280a6, 0xe280a0, 0xe280a1,
    0x00cb86, 0xe280b0, 0x00c5a0, 0xe280b9,
    0x00c592, 0x000020, 0x00c5bd, 0x000020,
    0x000020, 0xe28098, 0xe28099, 0xe2809c,
    0xe2809d, 0xe280a2, 0xe28093, 0xe28094,
    0x00cb9c, 0xe284a2, 0x00c5a1, 0xe280ba,
    0x00c593, 0x000020, 0x00c5be, 0x00c5b8,
    0x00c2a0, 0x00c2a1, 0x00c2a2, 0x00c2a3,
    0x00c2a4, 0x00c2a5, 0x00c2a6, 0x00c2a7,
    0x00c2a8, 0x00c2a9, 0x00c2aa, 0x00c2ab,
    0x00c2ac, 0x00c2ad, 0x00c2ae, 0x00c2af,
    0x00c2b0, 0x00c2b1, 0x00c2b2, 0x00c2b3,
    0x00c2b4, 0x00c2b5, 0x00c2b6, 0x00c2b7,
    0x00c2b8, 0x00c2b9, 0x00c2ba, 0x00c2bb,
    0x00c2bc, 0x00c2bd, 0x00c2be, 0x00c2bf,
    0x00c380, 0x00c381, 0x00c382, 0x00c383,
    0x00c384, 0x00c385, 0x00c386, 0x00c387,
    0x00c388, 0x00c389, 0x00c38a, 0x00c38b,
    0x00c38c, 0x00c38d, 0x00c38e, 0x00c38f,
    0x00c390, 0x00c391, 0x00c392, 0x00c393,
    0x00c394, 0x00c395, 0x00c396, 0x00c397,
    0x00c398, 0x00c399, 0x00c39a, 0x00c39b,
    0x00c39c, 0x00c39d, 0x00c39e, 0x00c39f,
    0x00c3a0, 0x00c3a1, 0x00c3a2, 0x00c3a3,
    0x00c3a4, 0x00c3a5, 0x00c3a6, 0x00c3a7,
    0x00c3a8, 0x00c3a9, 0x00c3aa, 0x00c3ab,
    0x00c3ac, 0x00c3ad, 0x00c3ae, 0x00c3af,
    0x00c3b0, 0x00c3b1, 0x00c3b2, 0x00c3b3,
    0x00c3b4, 0x00c3b5, 0x00c3b6, 0x00c3b7,
    0x00c3b8, 0x00c3b9, 0x00c3ba, 0x00c3bb,
    0x00c3bc, 0x00c3bd, 0x00c3be, 0x00c3bf
};


static int windows1252_to_PC850[] = {
    0x20, 0x20, 0x20, 0x9f, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0xff, 0xad, 0xbd, 0x9c, 0xcf, 0xbe, 0xdd, 0xf5,
    0xf9, 0xb8, 0xa6, 0xae, 0xaa, 0xf0, 0xa9, 0xee,
    0xf8, 0xf1, 0xfd, 0xfc, 0xef, 0xe6, 0xf4, 0xfa,
    0xf7, 0xfb, 0xa7, 0xaf, 0xac, 0xab, 0xf3, 0xa8,
    0xb7, 0xb5, 0xb6, 0xc7, 0x8e, 0x8f, 0x92, 0x80,
    0xd4, 0x90, 0xd2, 0xd3, 0xde, 0xd6, 0xd7, 0xd8,
    0xd1, 0xa5, 0xe3, 0xe0, 0xe2, 0xe5, 0x99, 0x9e,
    0x9d, 0xeb, 0xe9, 0xea, 0x9a, 0xed, 0xe8, 0xe1,
    0x85, 0xa0, 0x83, 0xc6, 0x84, 0x86, 0x91, 0x87,
    0x8a, 0x82, 0x88, 0x89, 0x8d, 0xa1, 0x8c, 0x8b,
    0xd0, 0xa4, 0x95, 0xa2, 0x93, 0xe4, 0x94, 0xf6,
    0x9b, 0x97, 0xa3, 0x96, 0x81, 0xec, 0xe7, 0x98
};

static int *codeset_table = NULL;



void encode_Set (int encoding)
{
    switch (encoding)
    {
        case CODESET_UTF8 :
            codeset_table = windows1252_to_utf8;
            break;
        
        case CODESET_PC850 :
            codeset_table = windows1252_to_PC850;
            break;

        default:
            codeset_table = NULL;
            break;
    }
    
}




char *encode_String (char *str)
{
    int i,j;
    int code;
    static char result[1000+1] = "";

    j = 0;

    for (i=0; (j<1000) && (str[i]!='\0'); i++)
    {
        if (((int)str[i] & 0xff) < 0x80)
            result[j++] = str[i];
        else
        {
            if (codeset_table != NULL)
                code = codeset_table[((int)str[i] & 0xff) - 0x80];
            else
                code = (int)str[i] & 0xff;

            if ((j<1000) && (((code>>16) & 0xff) != 0))
                result[j++] = (char)((code>>16) & 0xff);
            if ((j<1000) && (((code>>8) & 0xff) != 0))
                result[j++] = (char)((code>>8) & 0xff);
            if ((j<1000) && ((code & 0xff) != 0))
                result[j++] = (char)code;
        }
    }
    result[j] = '\0';

    return result;
}

