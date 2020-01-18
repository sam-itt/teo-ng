AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/src

#%.o: %.rc
.rc.o:
	$(WINDRES) --include-dir $(top_srcdir)/include --include-dir $(top_srcdir) -DHAVE_CONFIG_H -D_WIN32_IE=0x0300 -o $@ -i $<

bin_PROGRAMS = teo

if GFX_BACKEND_ALLEGRO
teo_LDADD = $(src_dir)/alleg/libagfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS)
else
if GFX_BACKEND_SDL2
teo_LDADD = $(src_dir)/sdl2/libsdlgfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) -lcomdlg32 -lcomctl32 
endif #GFX_BACKEND_SDL2
endif
teo_CPPFLAGS = $(DEPS_CFLAGS) $(PNG_CFLAGS) $(SDL2_CFLAGS) -I$(top_srcdir)/include

teo_SOURCES = \
	wmain.c \
	wgui.c \
	wdebug.c \
	wdialog.rc \
	wdebug/wdtoolb.c \
	wdebug/wdacc.c \
	wdebug/wdbkpt.c \
	wdebug/wdreg.c \
	wdebug/wddisass.c \
	wdebug/wdstatus.c \
	wdebug/wdmem.c \
	wgui/wcass.c \
	wgui/wprinter.c \
	wgui/wabout.c \
	wgui/wmemo.c \
	wgui/wdisk.c \
	wgui/wsetting.c 

EXTRA_DIST= \
	panel.bmp \
	thomson.bmp \
	to8.bmp \
	thomson.ico \
	wgui/clearlst.ico \
	wgui/folder.ico \
	wdebug/leave.ico \
	wdebug/stepover.ico \
	wdebug/step.ico \
	wdebug/run.ico
