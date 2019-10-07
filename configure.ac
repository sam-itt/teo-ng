#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])


m4_define(teo_version,[0.9999])


AC_INIT([Teo], [teo_version])
AM_INIT_AUTOMAKE([foreign subdir-objects])
#LT_INIT([disable-static],[dlopen])
LT_INIT
AC_CONFIG_SRCDIR([src/teo.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_HOST

#platform
case $host_os in
  *mingw*) mingw=true ;;
  *djgpp*) djgpp=true ;;
  *) mingw=false ;;
esac
AM_CONDITIONAL([MINGW], [test "x$mingw" = xtrue])
AM_CONDITIONAL([TARGET_DJGPP], [test "x$djgpp" = xtrue])


#options
AC_ARG_ENABLE([gtk_panel], [AS_HELP_STRING([--enable-gtk-panel], [Enable GTK Panel when building against Allegro on Unix])],
            [],
            [enable_gtk_panel=no])

AC_ARG_ENABLE([gtk_debugger], [AS_HELP_STRING([--enable-gtk-debugger], [Enable GTK debugger when building against Allegro on Unix])],
            [],
            [enable_gtk_debugger=no])

#graphic backed
AC_ARG_WITH([gfx_backend], [AS_HELP_STRING([--with-gfx-backend=allegro,gtk-x11], [Select graphic backend])],
            [],
            [with_gfx_backend=allegro])


# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h locale.h stddef.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit getcwd memmove memset mkdir setlocale strchr strcspn strpbrk strrchr strstr strtol])





AS_IF([test "x$mingw" = "xtrue"],[
   AC_MSG_NOTICE([Mingw32 detected, not running libs detection])
   PKG_CHECK_MODULES([DEPS], [allegro zlib])
   PKG_CHECK_MODULES([PNG], [libpng])
   AC_DEFINE([_WIN32_IE],[0x0300],[Minimum support level selection])
   AC_CHECK_TOOL([WINDRES], [windres], [:])
#   AC_DEFINE(__MINGW32__)
#   AC_SUBST([DEPS_LIBS],[-lalleg44 -mwindows -ldxapi -ldinput -lddraw -ldxguid -ldsound -lgdi32 -lole32 -lwinmm -lshell32 -lkernel32 -luser32 -lcomdlg32 -lcomctl32 -lpng -lz])
#   AC_SUBST([DEPS_CFLAGS],[])
],[
   PKG_CHECK_MODULES([DEPS], [glib-2.0 allegro])
   PKG_CHECK_MODULES([PNG], [libpng])
])


AS_IF([test "x$enable_gtk_panel" != "xno"], [
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    AM_CONDITIONAL(ENABLE_GTK_PANEL, true)
    AC_DEFINE(ENABLE_GTK_PANEL,[],[Use GTK for GUI])
], [
    AM_CONDITIONAL(ENABLE_GTK_PANEL, false)
])

AS_IF([test "x$enable_gtk_debugger" != "xno"], [
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    AM_CONDITIONAL(ENABLE_GTK_DEBUGGER, true)
    AC_DEFINE(ENABLE_GTK_DEBUGGER,[],[Use GTK for debugger GUI])
], [
    AM_CONDITIONAL(ENABLE_GTK_DEBUGGER, false)
])

AS_CASE(["x$with_gfx_backend"],
   ["xallegro"],[
    AM_CONDITIONAL(GFX_BACKEND_ALLEGRO, true)
    AM_CONDITIONAL(GFX_BACKEND_GTK_X11, false)
    AC_DEFINE(GFX_BACKEND_ALLEGRO,[],[Use Allegro for gfx rendering])
   ],
   ["xgtk-x11"],[
    AM_CONDITIONAL(GFX_BACKEND_GTK_X11, true)
    AM_CONDITIONAL(GFX_BACKEND_ALLEGRO, false)
    AC_DEFINE(GFX_BACKEND_GTK_X11,[],[Use Allegro for gfx rendering])
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    PKG_CHECK_MODULES([XEXT], [xext])
    PKG_CHECK_MODULES([X11], [x11])
    PKG_CHECK_MODULES([ALSA], [alsa])
   ],
   [
    AC_MSG_ERROR(["Valid values for --with-gfx-backend are 'allegro' and 'gtk-x11'"])
   ]
)


AM_CONDITIONAL([FOO], [test 1 = 2])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/alleg/Makefile
                 src/linux/Makefile
                 src/win/Makefile])

AC_OUTPUT
