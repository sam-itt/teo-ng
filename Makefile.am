AM_CPPFLAGS = @AM_CPPFLAGS@
ACLOCAL_AMFLAGS = -I m4

MKDIR_P = mkdir -p


SUBDIRS =  . data src

if BUILD_EXTRA_TOOLS
SUBDIRS += tools
endif

if INSTALL_DOC
SUBDIRS += doc
endif

EXTRA_DIST=
	misc/pack/repo/windows/inno/teo-small-img.bmp \
	misc/pack/repo/windows/inno/teo-setup.iss \
	misc/pack/repo/windows/inno/teo-big-img.bmp \
	misc/pack/repo/windows/nsis/teo-small-img.bmp \
	misc/pack/repo/windows/nsis/teo-big-img.bmp \
	misc/pack/repo/windows/nsis/teo.nsis.in

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
	@mv ${TMPDIR}/usr/share/doc/teo ${TMPDIR}/teow/doc
	@sh -c "cd ${TMPDIR} && zip -r ${CURDIR}/teo-win32.zip teow/*"
	@rm -Rf ${TMPDIR}

win32-setup:
	@makensis $(top_srcdir)/data/nsis/teo.nsis
	@mv $(top_srcdir)/data/nsis/teo-*.exe .
