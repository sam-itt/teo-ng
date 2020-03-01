AM_CPPFLAGS = @AM_CPPFLAGS@ 
ACLOCAL_AMFLAGS = -I m4

MKDIR_P = mkdir -p
LN_S = ln -s

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
	include/alleg/akeybint.h \
	include/alleg/afront.h \
	include/alleg/gui.h \
	include/alleg/color8.h \
	include/alleg/mouse.h \
	include/alleg/joyint.h \
	include/alleg/sound.h \
	include/alleg/gfxdrv.h \
	include/alleg/akeyboard.h \
	include/option.h \
	include/mc68xx/mc6821.h \
	include/mc68xx/mc6809.h \
	include/mc68xx/mc6804.h \
	include/mc68xx/mc6846.h \
	include/mc68xx/dasm6809.h \
	include/gettext.h \
	include/hardware.h \
	include/to8dbg.h \
	include/std.h \
	include/defs.h \
	include/image.h \
	include/dos/memmng.h \
	include/dos/debug.h \
	include/dos/floppy.h \
	include/main.h \
	include/debug/debug.h \
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
	include/media/disk/daccess.h \
	include/media/disk/fd.h \
	include/media/disk/sap.h \
	include/media/disk/dirdisk.h \
	include/media/disk/hfe.h \
	include/media/keyboard.h \
	include/ini.h \
	include/file/bmp.h \
	include/file/png.h \
	include/teo.h \
	include/sdl2/teo-sdl-log.h \
	include/sdl2/teo-sdl-jmouse.h \
	include/sdl2/sfront.h \
	include/sdl2/sfront-bindings.h \
	include/sdl2/teo-sdl-vkbd.h \
	include/sdl2/teo-sdl-sound.h \
	include/sdl2/teo-sdl-mouse.h \
	include/sdl2/teo-sdl-gfx.h \
	include/sdl2/gfx.h \
	include/sdl2/ini.h \
	include/sdl2/teo-sdl-joystick.h \
	include/sdl2/gui/scandir.h \
	include/sdl2/gui/file.h \
	include/sdl2/gui/font5x8.h \
	include/sdl2/gui/sdlgui.h \
	include/sdl2/gui/dialog.h \
	include/sdl2/gui/font10x16.h \
	include/sdl2/gui/screen.h \
	include/sdl2/sdl-keyboard.h \
	include/win/gui.h \
	include/win/keybint.h \
	include/win/dialog.rh \
	include/linux/display.h \
	include/linux/gui.h \
	include/linux/ufront.h \
	include/linux/graphic.h \
	include/linux/floppy.h \
	include/linux/sound.h \
	include/logsys.h \
	include/errors.h \
	include/og-xbox/crtdefs.h \
	include/og-xbox/sys/types.h \
	include/og-xbox/sys/stat.h \
	include/og-xbox/_mingw_off_t.h \
	include/og-xbox/dirent.h \
	include/og-xbox/io.h 

#end of autotools standard stuff. Bespoke rules to create bin dist for win32/MSDOS
DIR := ${CURDIR}

msdos-bindist:
	$(eval TMPDIR := $(shell mktemp -d))
	@rm ${CURDIR}/teo-msdos.zip 2>/dev/null || true
	@$(MAKE) DESTDIR=${TMPDIR} install
	${MKDIR_P} ${TMPDIR}/TEO
	@mv ${TMPDIR}/etc/teo/* ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/bin/* ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/share/locale/* ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/share/games/teo/system ${TMPDIR}/TEO/
	@mv ${TMPDIR}/usr/share/games/teo/roms ${TMPDIR}/TEO/
	@sh -c "cd ${TMPDIR} && zip -r ${CURDIR}/teo-msdos.zip TEO/*"
	@rm -Rf ${TMPDIR}

win32-bindist:
	$(eval TMPDIR := $(shell mktemp -d))
	@rm ${CURDIR}/teo-win32.zip 2>/dev/null || true
	@$(MAKE) DESTDIR=${TMPDIR} install
	${MKDIR_P} ${TMPDIR}/teow
	${MKDIR_P} ${TMPDIR}/teow/doc
	@mv ${TMPDIR}/etc/teo/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/bin/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/lib/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/locale/* ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/games/teo/*.bmp ${TMPDIR}/teow 2>/dev/null || true
	@mv ${TMPDIR}/usr/share/games/teo/system ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/games/teo/roms ${TMPDIR}/teow
	@mv ${TMPDIR}/usr/share/doc/teo-ng ${TMPDIR}/teow/doc
	@sh -c "cd ${TMPDIR} && zip -r ${CURDIR}/teo-win32.zip teow/*"
	@rm -Rf ${TMPDIR}

win32-setup:
	@makensis $(top_srcdir)/data/nsis/teo.nsis
	@mv $(top_srcdir)/data/nsis/teo-*.exe .

macos-bundle:
	$(eval TMPDIR := $(shell mktemp -d))
	@rm -Rf ${CURDIR}/Teo-ng.dmg 2>/dev/null || true
	@$(MAKE) DESTDIR=${TMPDIR} install
	${MKDIR_P} ${TMPDIR}/image
	${LN_S} /Applications ${TMPDIR}/image/
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app/Contents
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app/Contents/MacOS
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app/Contents/Libraries
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app/Contents/Resources
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app/Contents/Resources/share
	${MKDIR_P} ${TMPDIR}/image/Teo-ng.app/Contents/Resources/etc
	@cp data/teo.icns ${TMPDIR}/image/Teo-ng.app/Contents/Resources
	@cp data/Info.plist ${TMPDIR}/image/Teo-ng.app/Contents
	@plutil -convert binary1 ${TMPDIR}/image/Teo-ng.app/Contents/Info.plist
	@mv ${TMPDIR}/usr/bin/teo ${TMPDIR}/image/Teo-ng.app/Contents/MacOS
	@mv ${TMPDIR}/usr/share/locale ${TMPDIR}/image/Teo-ng.app/Contents/Resources/share
	@mv ${TMPDIR}/usr/share/games/teo ${TMPDIR}/image/Teo-ng.app/Contents/Resources
	@mv ${TMPDIR}/etc/teo ${TMPDIR}/image/Teo-ng.app/Contents/Resources/etc/
	@dylibbundler -od -b -x ${TMPDIR}/image/Teo-ng.app/Contents/MacOS/teo \
	                     -d ${TMPDIR}/image/Teo-ng.app/Contents/Libraries \
	                     -p "@executable_path/../Libraries"
	@hdiutil create -volname @PACKAGE_NAME@ -srcfolder ${TMPDIR}/image -ov -format UDZO @PACKAGE@.dmg
	@rm -Rf ${TMPDIR}
