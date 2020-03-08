AM_CPPFLAGS = @AM_CPPFLAGS@

noinst_LTLIBRARIES = libsdlgfx.la
libsdlgfx_la_CPPFLAGS = $(DEPS_CFLAGS) $(SDL2_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/include/sdl2 -I$(top_srcdir)/include/sdl2/gui
libsdlgfx_la_LIBADD = $(DEPS_LIBS) $(SDL2_LIBS)
libsdlgfx_la_SOURCES = \
	teo-sdl-gfx.c \
	ini.c \
	gfx.c \
	sfront.c \
	teo-sdl-keyboard.c \
	teo-sdl-mouse.c \
	teo-sdl-joystick.c \
	teo-sdl-jmouse.c \
	teo-sdl-vkbd.c \
	teo-sdl-sound.c


libsdlgfx_la_SOURCES += \
	sgui.c \
	sgui/sprinter.c \
	sgui/fileselect.c \
	sgui/scass.c \
	sgui/sabout.c \
	sgui/smemo.c \
	sgui/ssetting.c \
	sgui/alert.c \
	sgui/sdlgui.c \
	sgui/file.c \
	sgui/scandir.c \
	sgui/screen.c \
	sgui/sdisk.c

