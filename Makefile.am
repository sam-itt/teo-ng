AM_CPPFLAGS = @AM_CPPFLAGS@
ACLOCAL_AMFLAGS = -I m4
teosysdir=$(datadir)/games/teo/system
printerdir=$(teosysdir)/printer
printer612dir=$(teosysdir)/printer/612
printer600dir=$(teosysdir)/printer/600
printer042dir=$(teosysdir)/printer/042
printer582dir=$(teosysdir)/printer/582
printer055dir=$(teosysdir)/printer/055
sysromdir=$(teosysdir)/rom
icondir=$(teosysdir)/icons

teoconfdir=$(sysconfdir)/teo

SUBDIRS =  . src

if BUILD_EXTRA_TOOLS
SUBDIRS += tools
endif

teoconf_DATA = \
	teo.ini
if GFX_BACKEND_ALLEGRO
teoconf_DATA += \
	allegro.cfg \
	teo-keymap-final.ini
endif
if GFX_BACKEND_GTK_X11
teoconf_DATA += \
	teo-gdk-keymap.ini
endif

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
	system/icons/run.ico 


