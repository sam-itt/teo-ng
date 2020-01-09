AM_CPPFLAGS = @AM_CPPFLAGS@

noinst_LTLIBRARIES = libagfx.la
libagfx_la_CPPFLAGS = $(DEPS_CFLAGS) $(ALLEGRO_CFLAGS) -I$(top_srcdir)/include
libagfx_la_LIBADD = $(DEPS_LIBS) $(ALLEGRO_LIBS)
libagfx_la_SOURCES = \
	amode80.c \
	ajoyint.c \
	atruecol.c \
	amouse.c \
	akeyboard.c \
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
