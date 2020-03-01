if DEBIAN_BUILD
teodatadir=$(datadir)/teo
else
teodatadir=$(datadir)/games/teo
endif
teosysdir=$(teodatadir)/system
printerdir=$(teosysdir)/printer
printer612dir=$(teosysdir)/printer/612
printer600dir=$(teosysdir)/printer/600
printer042dir=$(teosysdir)/printer/042
printer582dir=$(teosysdir)/printer/582
printer055dir=$(teosysdir)/printer/055
sysromdir=$(teosysdir)/rom
icondir=$(teosysdir)/icons

userromdir=$(teodatadir)/roms
memodir=$(userromdir)/memo
cassdir=$(userromdir)/cass
diskdir=$(userromdir)/disk

teoconfdir=$(sysconfdir)/teo

appdir=$(datadir)/applications
metainfodir=$(datadir)/metainfo
pixmapdir=$(datadir)/pixmaps
mimedir=$(datadir)/mime/packages


bindatadir = $(bindir)
libdatadir = $(libdir)

teodata_DATA = \
	empty.hfe
if GFX_BACKEND_SDL2
teodata_DATA += \
	keyboard.bmp \
	led-on.bmp \
	led-off.bmp
endif

teoconf_DATA = \
	.keep \
	teo.ini
if GFX_BACKEND_ALLEGRO
teoconf_DATA += \
	allegro.cfg \
	akeymap.ini
endif
if GFX_BACKEND_GTK_X11
teoconf_DATA += \
	gdk-keymap.ini
endif
if GFX_BACKEND_SDL2
teoconf_DATA += \
	sdl-keymap.ini
endif

if PLATFORM_MSDOS
bindata_DATA = \
	cwsdpmi.exe
endif


if PLATFORM_WIN32
libdata_DATA = \
	libpng3.dll \
	libpng16.dll \
	zlib1.dll \
	libiconv-2.dll \
	libintl-8.dll \
	libgcc_s_sjlj-1.dll
if GFX_BACKEND_ALLEGRO
libdata_DATA += \
	alleg40.dll \
	alleg44.dll
endif

if GFX_BACKEND_SDL2
libdata_DATA += \
	SDL2.dll
endif

endif #PLATFORM_WIN32


printer612_DATA = \
	system/printer/612/elitc.txt \
	system/printer/612/table.txt \
	system/printer/612/italc.txt \
	system/printer/612/picac.txt \
	system/printer/612/picas.txt \
	system/printer/612/itals.txt \
	system/printer/612/scrpc.txt

printer600_DATA = \
	system/printer/600/elitc.txt \
	system/printer/600/italc.txt \
	system/printer/600/picac.txt \
	system/printer/600/picas.txt \
	system/printer/600/itals.txt \
	system/printer/600/scrpc.txt

printer042_DATA = \
	system/printer/042/picas.txt

printer582_DATA = \
	system/printer/582/conds.txt \
	system/printer/582/table.txt \
	system/printer/582/italc.txt \
	system/printer/582/picac.txt \
	system/printer/582/picas.txt \
	system/printer/582/scrpc.txt 

printer055_DATA = \
	system/printer/055/picas.txt 

sysrom_DATA = \
	system/rom/basic512.rom \
	system/rom/expl.rom \
	system/rom/extramon.rom \
	system/rom/monitor2.rom \
	system/rom/basic1.rom \
	system/rom/monitor1.rom 

icon_DATA = \
	system/icons/leave.ico \
	system/icons/stepover.ico \
	system/icons/step.ico \
	system/icons/run.ico 


EXTRA_DIST = \
	nsis/teo-small-img.bmp \
	nsis/teo-big-img.bmp \
	nsis/teo.nsis.in \
	system/printer/612/elitc.txt \
	system/printer/612/table.txt \
	system/printer/612/italc.txt \
	system/printer/612/picac.txt \
	system/printer/612/picas.txt \
	system/printer/612/itals.txt \
	system/printer/612/scrpc.txt \
	system/printer/600/elitc.txt \
	system/printer/600/italc.txt \
	system/printer/600/picac.txt \
	system/printer/600/picas.txt \
	system/printer/600/itals.txt \
	system/printer/600/scrpc.txt \
	system/printer/042/picas.txt \
	system/printer/582/conds.txt \
	system/printer/582/table.txt \
	system/printer/582/italc.txt \
	system/printer/582/picac.txt \
	system/printer/582/picas.txt \
	system/printer/582/scrpc.txt \
	system/printer/055/picas.txt \
	system/rom/basic512.rom \
	system/rom/expl.rom \
	system/rom/extramon.rom \
	system/rom/monitor2.rom \
	system/rom/basic1.rom \
	system/rom/monitor1.rom \
	system/icons/leave.ico \
	system/icons/stepover.ico \
	system/icons/step.ico \
	system/icons/run.ico \
	com.teo-ng.Teo.appdata.xml.in \
	com.teo-ng.Teo.png \
	com.teo-ng.Teo.desktop.in \
	teo.xml \
	teo.ini \
	empty.hfe \
	keyboard.bmp \
	led-on.bmp \
	led-off.bmp \
	sdl-keymap.ini \
	allegro.cfg \
	language.dat \
	keyboard.dat \
	akeymap.ini \
	gdk-keymap.ini \
	teo.icns \
	Info.plist.in \
	cc90hfe/cc90.hfe \
	cc90hfe/cc90.fd \
	cc90hfe/cc90.sap \
	cc90hfe/cc90hfe.desktop \
	cc90hfe/cc90hfe.png \
	libpng3.dll \
	cwsdpmi.exe \
	zlib1.dll \
	libpng16.dll \
	alleg44.dll \
	alleg40.dll \
	SDL2.dll \
	libiconv-2.dll \
	libintl-8.dll \
	libgcc_s_sjlj-1.dll \
	.keep

userrom_DATA = \
	.keep

memo_DATA = \
	.keep

cass_DATA = \
	.keep

disk_DATA = \
	.keep

app_DATA = \
	com.teo-ng.Teo.desktop
if BUILD_CC90
app_DATA += \
	cc90hfe/cc90hfe.desktop
endif
metainfo_DATA = \
	com.teo-ng.Teo.appdata.xml
pixmap_DATA = \
	com.teo-ng.Teo.png
if BUILD_CC90
pixmap_DATA += \
	cc90hfe/cc90hfe.png
endif
mime_DATA = \
	teo.xml

install-data-hook:
if PLATFORM_UNIX
	sed -i" " "s@\$${romsdir}@$(userromdir)@g" $(DESTDIR)/$(teoconfdir)/teo.ini
else
	sed -i" " "s@\$${romsdir}@@g" $(DESTDIR)/$(teoconfdir)/teo.ini
endif

