AM_CPPFLAGS = @AM_CPPFLAGS@ 
ACLOCAL_AMFLAGS = -I m4

MKDIR_P = mkdir -p


SUBDIRS = po . data src

if BUILD_EXTRA_TOOLS
SUBDIRS += tools
endif

if INSTALL_DOC
SUBDIRS += doc
endif

EXTRA_DIST=	\
	m4/ChangeLog  \
	LISEZMOI.txt \
	licence-fr.txt \
	licence-en.txt \
	README.txt \
	change-en.log \
	change-fr.log \
	include/to8keys.h \
	include/alleg \
	include/alleg/gui.h \
	include/alleg/color8.h \
	include/alleg/mouse.h \
	include/alleg/joyint.h \
	include/alleg/sound.h \
	include/alleg/gfxdrv.h \
	include/alleg/akeyboard.h \
	include/option.h \
	include/debug.h \
	include/mc68xx \
	include/mc68xx/mc6821.h \
	include/mc68xx/mc6809.h \
	include/mc68xx/mc6804.h \
	include/mc68xx/mc6846.h \
	include/mc68xx/dasm6809.h \
	include/hardware.h \
	include/std.h \
	include/defs.h \
	include/image.h \
	include/dos \
	include/dos/memmng.h \
	include/dos/debug.h \
	include/dos/floppy.h \
	include/main.h \
	include/debug \
	include/debug/debug.h \
	include/media \
	include/media/printer \
	include/media/printer/pr90055.h \
	include/media/printer/pr90042.h \
	include/media/printer/pr90582.h \
	include/media/printer/pr906xx.h \
	include/media/disk.h \
	include/media/mouse.h \
	include/media/cass.h \
	include/media/memo.h \
	include/media/joystick.h \
	include/media/printer.h \
	include/media/disk \
	include/media/disk/daccess.h \
	include/media/disk/fd.h \
	include/media/disk/sap.h \
	include/media/disk/dirdisk.h \
	include/media/disk/hfe.h \
	include/media/keyboard.h \
	include/ini.h \
	include/file \
	include/file/bmp.h \
	include/file/png.h \
	include/teo.h \
	include/win \
	include/win/gui.h \
	include/win/keybint.h \
	include/win/dialog.rh \
	include/linux \
	include/linux/display.h \
	include/linux/gui.h \
	include/linux/graphic.h \
	include/linux/floppy.h \
	include/linux/sound.h \
	include/errors.h 

#end of autotools standard stuff. Bespoke rules to create bin dist for win32/MSDOS
DIR := ${CURDIR}

msdos-bindist:
	$(eval TMPDIR := $(shell mktemp -d))
	@$(MAKE) DESTDIR=${TMPDIR} install
	${MKDIR_P} ${TMPDIR}/TEO
	@mv ${TMPDIR}/etc/teo/* ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/bin/* ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/share/games/teo/system ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/share/games/teo/roms ${TMPDIR}/TEO/
	@sh -c "cd ${TMPDIR} && zip -r ${CURDIR}/teo-msdos.zip TEO/*"
	@rm -Rf ${TMPDIR}

win32-bindist:
	$(eval TMPDIR := $(shell mktemp -d))
	@$(MAKE) DESTDIR=${TMPDIR} install
	${MKDIR_P} ${TMPDIR}/teow
	${MKDIR_P} ${TMPDIR}/teow/doc
	@mv ${TMPDIR}/etc/teo/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/bin/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/lib/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/games/teo/system ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/games/teo/roms ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/doc/teo-ng ${TMPDIR}/teow/doc
	@sh -c "cd ${TMPDIR} && zip -r ${CURDIR}/teo-win32.zip teow/*"
	@rm -Rf ${TMPDIR}

win32-setup:
	@makensis $(top_srcdir)/data/nsis/teo.nsis
	@mv $(top_srcdir)/data/nsis/teo-*.exe .
