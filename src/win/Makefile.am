AM_CPPFLAGS = @AM_CPPFLAGS@
src_dir = $(top_srcdir)/src

#%.o: %.rc
.rc.o:
	$(WINDRES) --include-dir $(top_srcdir)/include -D_WIN32_IE=0x0300 -o $@ -i $<

bin_PROGRAMS = teo

#if GFX_BACKEND_ALLEGRO only supported backend for win32.
teo_LDADD = $(src_dir)/alleg/libagfx.la $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS)
#else
#teo_LDADD = $(src_dir)/libteo.la $(DEPS_LIBS) $(PNG_LIBS) $(GTK_LIBS) $(XEXT_LIBS) $(X11_LIBS) $(ALSA_LIBS)
#endif
teo_CPPFLAGS = $(DEPS_CFLAGS) $(PNG_CFLAGS) -I$(top_srcdir)/include

teo_SOURCES = \
	wmain.c \
	wgui.c \
	wkeybint.c \
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

