AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/tools/cc90hfe

.rc.o:
	$(WINDRES) --include-dir $(src_dir)/include -D_WIN32_IE=0x0300 -o $@ -i $<

bin_PROGRAMS = cc90hfe
cc90hfe_CPPFLAGS = -I$(src_dir)/include
#I couldn't get it to work on real windows without -static. (worked on Win though)
cc90hfe_LDFLAGS = -static
cc90hfe_LDADD = $(src_dir)/src/libcc90hfe.la -lole32 -loleaut32 -luser32 -lshell32 -lcomdlg32 -lcomctl32 -lgdi32
cc90hfe_SOURCES = \
    progress.c    \
    serial.c      \
    port.c        \
    resource.rc   \
    gui.c         \
    gui/archive.c \
    gui/extract.c \
    gui/install.c \
    gui/about.c


