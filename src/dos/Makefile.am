AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/src

bin_PROGRAMS = teo

#if GFX_BACKEND_ALLEGRO only supported backend for win32.
teo_LDADD = $(src_dir)/alleg/libagfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS)
#else
#teo_LDADD = $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS) $(XEXT_LIBS) $(X11_LIBS) $(ALSA_LIBS)
#endif
teo_CPPFLAGS = $(DEPS_CFLAGS) $(PNG_CFLAGS) -I$(top_srcdir)/include

teo_SOURCES = \
	dmain.c \
	ddebug.c \
	dmemmng.c \
	dvga.c \
	dfloppy.c
