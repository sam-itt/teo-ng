AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/src

bin_PROGRAMS = teo

if GFX_BACKEND_ALLEGRO
teo_LDADD = $(src_dir)/alleg/libagfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS)
else 
if GFX_BACKEND_SDL2
teo_LDADD = $(src_dir)/sdl2/libsdlgfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS) $(ALSA_LIBS)
else
teo_LDADD = $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS) $(XEXT_LIBS) $(X11_LIBS) $(ALSA_LIBS)
endif #GFX_BACKEND_SDL2
endif #GFX_BACKEND_ALLEGRO
teo_CPPFLAGS = $(DEPS_CFLAGS) $(PNG_CFLAGS) $(GTK_CFLAGS) $(XEXT_CFLAGS) $(X11_CFLAGS) $(ALSA_CFLAGS) -I$(top_srcdir)/include 

teo_SOURCES = \
	ufloppy.c 

if GFX_BACKEND_ALLEGRO
teo_SOURCES += umain.c \
	           ukeybint.c 
endif


if GFX_BACKEND_SDL2
teo_SOURCES += umain-sdl2.c \
	usound.c 
endif


if GFX_BACKEND_GTK_X11
teo_SOURCES += \
	umain-x11.c \
	udisplay.c \
	usound.c \
	ugraphic.c \
	\
	udebug.c \
	udebug/udstatus.c \
	udebug/uddisass.c \
	udebug/udacc.c \
	udebug/udreg.c \
	udebug/udmem.c \
	udebug/udbkpt.c \
	udebug/udtoolb.c \
	\
	ugui.c \
	ugui/usetting.c \
	ugui/umemo.c \
	ugui/uabout.c \
	ugui/ucass.c \
	ugui/udisk.c \
	ugui/uprinter.c
endif


if ENABLE_GTK_DEBUGGER
teo_SOURCES += \
	udebug.c \
	udebug/udstatus.c \
	udebug/uddisass.c \
	udebug/udacc.c \
	udebug/udreg.c \
	udebug/udmem.c \
	udebug/udbkpt.c \
	udebug/udtoolb.c
endif

if ENABLE_GTK_PANEL
teo_SOURCES += \
	ugui.c \
	ugui/usetting.c \
	ugui/umemo.c \
	ugui/uabout.c \
	ugui/ucass.c \
	ugui/udisk.c \
	ugui/uprinter.c
endif
