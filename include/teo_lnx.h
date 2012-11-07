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
 *  Module     : intern/std.h
 *  Version    : 1.8.2
 *  Créé par   : François Mouret 28/09/2012
 *  Modifié par:
 *
 *  Inclusion des headers spécifiques à Windows.
 */


#ifndef TEO_LNX_H
#define TEO_LNX_H

#include "teo.h"

#ifndef SCAN_DEPEND
   #include <locale.h>
   #include <fcntl.h>
   #include <signal.h>
   #include <errno.h>
   #include <libgen.h>
   #include <gtk/gtk.h>
   #include <gtk/gtkx.h>
   #include <gdk/gdkx.h>
   #include <X11/Xlib.h>
   #include <X11/XKBlib.h>
   #include <X11/Xutil.h>
   #include <X11/keysym.h>
   #include <X11/Xresource.h>
   #include <X11/extensions/XShm.h>
   #include <sys/ioctl.h>
   #include <sys/ipc.h>
   #include <sys/types.h>
   #include <sys/shm.h>
   #include <sys/time.h>
   #include <linux/fd.h>
   #include <linux/fdreg.h>
/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
   #include <alsa/asoundlib.h>
#endif

#include "linux/gui.h"
#include "linux/floppy.h"
#include "linux/display.h"
#include "linux/graphic.h"
#include "linux/sound.h"

#endif


