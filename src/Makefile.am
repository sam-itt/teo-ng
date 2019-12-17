AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/src

SUBDIRS = .

if GFX_BACKEND_ALLEGRO
SUBDIRS +=  alleg
endif #GFX_BACKEND_ALLEGRO

if PLATFORM_MSDOS
SUBDIRS +=  dos
else
if PLATFORM_WIN32
SUBDIRS +=  win
else
SUBDIRS += linux
endif #PLATFORM_WIN32
endif #PLATFORM_MSDOS




#if PLATFORM_MSDOS
#SUBDIRS =  . alleg dos
#else
#if PLATFORM_WIN32
#SUBDIRS =  . alleg win
#else
#if GFX_BACKEND_ALLEGRO
#SUBDIRS =  . alleg linux
#else
#SUBDIRS =  . linux
#endif #GFX_BACKEND_ALLEGRO
#endif #PLATFORM_WIN32
#endif #PLATFORM_MSDOS


noinst_LTLIBRARIES = libteo.la
libteo_la_CPPFLAGS = $(DEPS_CFLAGS) $(PNG_CFLAGS) -I$(top_srcdir)/include -DSYSCONFDIR="\"@sysconfdir@\"" -DDATAROOTDIR="\"@datarootdir@\""
libteo_la_LIBADD = $(DEPS_LIBS) $(PNG_LIBS)
libteo_la_SOURCES = \
	mc68xx/dasm6809.c \
	mc68xx/mc6804.c \
	mc68xx/mc6809.c \
	mc68xx/mc6821.c \
	mc68xx/mc6846.c \
	debug/dreg.c \
	debug/dmem.c \
	debug/dacc.c \
	debug/ddisass.c \
	debug/dbkpt.c \
	media/cass.c \
	media/mouse.c \
	media/printer/pr90582.c \
	media/printer/pr906xx.c \
	media/printer/pr90055.c \
	media/printer/pr90042.c \
	media/printer.c \
	media/keyboard.c \
	media/disk.c \
	media/memo.c \
	media/disk/daccess.c \
	media/disk/sap.c \
	media/disk/fd.c \
	media/disk/hfe.c \
	media/disk/controlr/thmfc1.c \
	media/joystick.c \
	file/bmp.c \
	file/png.c \
	image.c \
	hardware.c \
	option.c \
	errors.c \
	std.c \
	ini.c \
	teo.c

EXTRA_DIST = \
	change.log
