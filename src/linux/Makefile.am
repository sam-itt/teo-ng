src_dir = $(top_srcdir)/src

bin_PROGRAMS = teo
teo_LDADD = $(src_dir)/alleg/libagfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS)
teo_CPPFLAGS = $(CPP_FLAGS) $(DEPS_CFLAGS) $(PNG_CFLAGS) -I$(top_srcdir)/include

#udisplay.c 
teo_SOURCES = \
	ukeybint.c \
	ufloppy.c \
	umain.c

if FOO
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

if FOO
teo_SOURCES += \
	ugui.c \
	ugui/usetting.c \
	ugui/umemo.c \
	ugui/uabout.c \
	ugui/ucass.c \
	ugui/udisk.c \
	ugui/uprinter.c

endif
