#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#CFLAGS="-g"
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


##########################
# Checking target platform
##########################

#WIN32/Mingw
AC_MSG_CHECKING([if compiling for Win32])
case "$host_os" in
  *mingw*) 
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" = "xyes"])


####################
# Check for DOS/DJGPP
####################
AC_MSG_CHECKING([if compiling for MS-DOS])
case "$host_os" in
  *djgpp*)
    platform_msdos=yes
    ;;
  *)
    platform_msdos=no
    ;;
esac
AC_MSG_RESULT([$platform_msdos])
AM_CONDITIONAL(PLATFORM_MSDOS, test "x$platform_msdos" = "xyes")

###########################################
# Otherwise just assume it's Linux/Unix
# If we ever need to deal with OS specifics
# just add another platform below
###########################################
AC_MSG_CHECKING([if compiling for Unix])
AS_IF([test "x$platform_win32" != "xyes" && test "x$platform_msdos" != "xyes"],[
    platform_unix=yes
],[
])
AC_MSG_RESULT([$platform_unix])
AM_CONDITIONAL(PLATFORM_UNIX, test "x$platform_unix" = "xyes")


# Checks for libraries.
AS_IF([test "x$platform_win32" = "xyes"],[
   PKG_CHECK_MODULES([DEPS], [allegro zlib])
   PKG_CHECK_MODULES([PNG], [libpng])
   AC_DEFINE([_WIN32_IE],[0x0300],[Minimum support level selection])
   AC_CHECK_TOOL([WINDRES], [windres], [:])
],[test "x$platform_msdos" = "xyes"],[
  PKG_CHECK_MODULES([DEPS], [allegro])
  PKG_CHECK_MODULES([PNG], [libpng])
  AX_CHECK_COMPILE_FLAG([-fgnu89-inline],
    [AX_APPEND_FLAG([-fgnu89-inline])],
    [AC_MSG_WARN([-fgnu89-inline is not supported, linking with allegro might fail with gcc > 2.x])
  ])
],[
  #Implicit Unix
  PKG_CHECK_MODULES([DEPS], [glib-2.0 allegro])
  PKG_CHECK_MODULES([PNG], [libpng])
])

AS_IF([test "x$enable_gtk_panel" != "xno"], [
    AS_IF([test "x$platform_unix" != "xyes"],[
        AC_MSG_ERROR(["GTK panel is not supported on $host_os"])
    ],[]);
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    AM_CONDITIONAL(ENABLE_GTK_PANEL, true)
    AC_DEFINE(ENABLE_GTK_PANEL,[],[Use GTK for GUI])
], [
    AM_CONDITIONAL(ENABLE_GTK_PANEL, false)
])

AS_IF([test "x$enable_gtk_debugger" != "xno"], [
    AS_IF([test "x$platform_unix" != "xyes"],[
        AC_MSG_ERROR(["GTK debugger is not supported on $host_os"])
    ],[]);
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
    AS_IF([test "platform_unix" != "xyes"],[
        AC_MSG_ERROR(["GTK/X11 graphic backend is not supported on $host_os"])
    ],[]);
    AM_CONDITIONAL(GFX_BACKEND_GTK_X11, true)
    AM_CONDITIONAL(GFX_BACKEND_ALLEGRO, false)
    AC_DEFINE(GFX_BACKEND_GTK_X11,[],[Use GTK/X11 for gfx rendering])
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    PKG_CHECK_MODULES([XEXT], [xext])
    PKG_CHECK_MODULES([X11], [x11])
    PKG_CHECK_MODULES([ALSA], [alsa])
   ],
   [
    AC_MSG_ERROR(["Valid values for --with-gfx-backend are 'allegro' and 'gtk-x11'"])
   ]
)


#AC_CONFIG_FILES([Makefile
#                 src/Makefile
#                 src/alleg/Makefile
#                 src/dos/Makefile
#                 src/linux/Makefile
#                 src/win/Makefile])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AM_COND_IF([GFX_BACKEND_ALLEGRO],
           [AC_CONFIG_FILES([src/alleg/Makefile])])
AM_COND_IF([PLATFORM_MSDOS],
           [AC_CONFIG_FILES([src/dos/Makefile])])
AM_COND_IF([PLATFORM_UNIX],
           [AC_CONFIG_FILES([src/linux/Makefile])])
AM_COND_IF([PLATFORM_WIN32],
           [AC_CONFIG_FILES([src/win/Makefile])])

AC_OUTPUT
