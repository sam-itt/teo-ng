AM_CPPFLAGS = @AM_CPPFLAGS@

noinst_LTLIBRARIES = libsdlgfx.la
libsdlgfx_la_CPPFLAGS = $(DEPS_CFLAGS) $(SDL2_CFLAGS) -I$(top_srcdir)/include 
libsdlgfx_la_LIBADD = $(DEPS_LIBS) $(SDL2_LIBS)
libsdlgfx_la_SOURCES = \
	teo-sdl.c 
