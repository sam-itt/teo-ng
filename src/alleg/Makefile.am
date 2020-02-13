AM_CPPFLAGS = @AM_CPPFLAGS@

noinst_LTLIBRARIES = libagfx.la
libagfx_la_CPPFLAGS = $(DEPS_CFLAGS) $(ALLEGRO_CFLAGS) -I$(top_srcdir)/include $(GTK_CFLAGS)
libagfx_la_LIBADD = $(DEPS_LIBS) $(ALLEGRO_LIBS) $(GTK_LIBS)
if PLATFORM_WIN32
libagfx_la_LDFLAGS = -static-libgcc
endif
libagfx_la_SOURCES = \
	amode80.c \
	ajoyint.c \
	atruecol.c \
	amouse.c \
	akeyboard.c \
	akeybint.c \
	agfxdrv.c \
	asound.c \
	acolor8.c \
	amode40.c \
	agui/aabout.c \
	agui/acass.c \
	agui/aprinter.c \
	agui/asetting.c \
	agui/adisk.c \
	agui/amemo.c \
	agui.c \
	afront.c 
