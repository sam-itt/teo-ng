AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/tools/cc90hfe

SUBDIRS = .

if PLATFORM_WIN32
SUBDIRS +=  win
endif #PLATFORM_WIN32
if PLATFORM_UNIX
SUBDIRS += linux
endif #PLATFORM_UNIX

noinst_LTLIBRARIES = libcc90hfe.la
libcc90hfe_la_CPPFLAGS = -I$(src_dir)/include
libcc90hfe_la_SOURCES = \
    cc90.c    \
    hfe.c     \
    ini.c     \
    option.c  \
    errors.c  \
    encode.c  \
    std.c     \
    main.c

