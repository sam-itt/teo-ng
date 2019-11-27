AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/tools/cc90hfe

bin_PROGRAMS = cc90hfe
cc90hfe_CPPFLAGS = $(GTK_CFLAGS) -I$(src_dir)/include
cc90hfe_LDADD = $(GTK_LIBS) $(X11_LIBS) $(src_dir)/src/libcc90hfe.la
cc90hfe_SOURCES = \
    progress.c    \
    serial.c      \
    port.c        \
    gui.c         \
    gui/archive.c \
    gui/extract.c \
    gui/install.c \
    gui/about.c

