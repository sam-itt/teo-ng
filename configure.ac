#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#CFLAGS="-g"
AC_PREREQ([2.69])

#While in dev cycle
#m4_define(teo_major_version,[0])
#m4_define(teo_minor_version,[9999])
#m4_define(teo_micro_version,[0])
#m4_define(teo_nano_version,[0])

# Releases
m4_define(teo_major_version,[1])
m4_define(teo_minor_version,[9])
m4_define(teo_micro_version,[0])
m4_define(teo_nano_version,[0])

#m4_define(teo_version,[0.9999])
#nano intentionaly left out, only used in windows dialog
m4_define([teo_version], [teo_major_version.teo_minor_version.teo_micro_version])
m4_define([teo_version_wrc], [teo_major_version, teo_minor_version, teo_micro_version, teo_nano_version])

AC_INIT([Teo-ng], [teo_version])
AM_INIT_AUTOMAKE([foreign subdir-objects])
#LT_INIT([disable-static],[dlopen])
LT_INIT
AC_CONFIG_SRCDIR([src/teo.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_HOST
AC_DEFINE(TEO_VERSION_STR, "[teo_version]", [Package version constant, as defined in existing code base])
AC_DEFINE(TEO_YEAR_STRING, "2019", [Release year matching package version, as defined in existing code base])
AC_DEFINE(TEO_VERSION_WRC, [teo_version_wrc], [Package version constant, in a format suitable for Win32 resource files])

AC_DEFUN([AC_PROG_GCR], [AC_CHECK_PROG([GCR],[glib-compile-resources],[yes])])
AC_SUBST(WEBSITE_URL,[https://sourceforge.net/projects/teoemulator/])

# For functions like asprintf, get_current_dir_name
AC_DEFINE([_GNU_SOURCE], [], [Enable GNU extensions.])

#compile options
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Build an executable with debug sybmols and all optimization turned off (default=no)])],
            [],
            [enable_debug=no])
AC_ARG_ENABLE([profiling], [AS_HELP_STRING([--enable-profiling], [Build an executable that can be profiled with gprof (default=no)])],
            [],
            [enable_profiling=no])
AC_ARG_ENABLE([optimization], [AS_HELP_STRING([--enable-optimization], [Build an optimized executable. (default=yes)])],
            [],
            [enable_optimization=yes])
AC_ARG_ENABLE([machine_optimization], [AS_HELP_STRING([--enable-machine-optimization], [Build an optimized executable for the local machine using march and mtune=native. (default=no)])],
            [],
            [enable_machine_optimization=no])
AC_ARG_ENABLE([debian_build], [AS_HELP_STRING([--enable-debian-build], [Enable some Debian-specifc part of the codebase. (default=no)])],
            [],
            [debian_build=no])
AC_ARG_ENABLE([extra_tools], [AS_HELP_STRING([--enable-extra-tools], [Enable extra tools (default=no)])],
            [],
            [extra_tools=no])



#options
AC_ARG_ENABLE([gtk_panel], [AS_HELP_STRING([--enable-gtk-panel], [Enable GTK Panel when building against Allegro on Unix])],
            [],
            [enable_gtk_panel=no])

AC_ARG_ENABLE([gtk_debugger], [AS_HELP_STRING([--enable-gtk-debugger], [Enable GTK debugger when building against Allegro on Unix])],
            [],
            [enable_gtk_debugger=no])

#graphic backend
AC_ARG_WITH([gfx_backend], [AS_HELP_STRING([--with-gfx-backend=allegro,gtk-x11,sdl2], [Select graphic backend])],
            [],
            [with_gfx_backend=allegro])

#Documentation
AC_ARG_WITH([doc],
  [AS_HELP_STRING([--with-doc], [Install the documentation. [default=yes]])],
  [AS_CASE([$with_doc],
    [yes|no], [],
              [AC_MSG_ERROR([Unrecognized value '${with_doc}' for --with-doc])]
    )],
  [with_doc=yes])
AM_CONDITIONAL([INSTALL_DOC], [test "x${with_doc}" = xyes])


# Checks for programs.
# Don't use the default -Wall -g -O2
#save_CFLAGS=$CFLAGS
AC_PROG_CC
CFLAGS="-Wall -O2"
AC_PROG_MAKE_SET



# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h locale.h stddef.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
# Those 2 are based on runtime checks and therefore will always fail when cross compiling. R
# Replaced them with standard checks in AC_CHECK_FUNCS
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([malloc realloc atexit access getcwd memmove memset mkdir setlocale strchr strcspn strpbrk strrchr strstr strtol vasprintf get_current_dir_name scandir alphasort])
AM_GNU_GETTEXT([external])
#AM_GNU_GETTEXT_VERSION([0.19])

# check if the dirent structure of a d_type field and DT_UNKNOWN is defined
AC_MSG_CHECKING(if the dirent structure of a d_type field)
AC_LINK_IFELSE(
[
  AC_LANG_SOURCE([[
    #include <dirent.h>

    int main() {
      struct dirent entry;
      return entry.d_type == DT_UNKNOWN;
    }
  ]])
],[have_dirent_d_type=yes],[have_dirent_d_type=no])
AC_MSG_RESULT($have_dirent_d_type)

if test "$have_dirent_d_type" = yes; then
    AC_DEFINE(HAVE_DIRENT_D_TYPE, 1,
              [Define to 1 if the dirent structure has a d_type field])
fi


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
AS_IF([test "x$platform_win32" = "xyes"],[
    AC_DEFINE(PLATFORM_WIN32,[1],[Building for win32 and alike systems])
],[
    AC_DEFINE(PLATFORM_WIN32,[0],[Building for win32 and alike systems])
])


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
AS_IF([test "x$platform_msdos" = "xyes"],[
    AC_DEFINE(PLATFORM_MSDOS,[1],[Building for MS-DOS and alike systems])
],[
    AC_DEFINE(PLATFORM_MSDOS,[0],[Building for MS-DOS and alike systems])
])


###########################################
# Otherwise just assume it's Linux/Unix
# If we ever need to deal with OS specifics
# just add another platform below
###########################################
AC_MSG_CHECKING([if compiling for Unix])
AS_IF([test "x$platform_win32" != "xyes" && test "x$platform_msdos" != "xyes"],[
    platform_unix=yes
],[
    platform_unix=no
])

AC_MSG_RESULT([$platform_unix])
AM_CONDITIONAL(PLATFORM_UNIX, test "x$platform_unix" = "xyes")
AS_IF([test "x$platform_unix" = "xyes"],[
    AC_DEFINE(PLATFORM_UNIX,[1],[Building for unix-compatible systems])
    AC_DEFINE(DIR_SEPARATOR,['/'],[Char that goes between directories in a path])
],[
    AC_DEFINE(PLATFORM_UNIX,[0],[Building for unix-compatible systems])
    AC_DEFINE(DIR_SEPARATOR,['\\'],[Char that goes between directories in a path])
])

#Win32 specific compiler flags
AS_IF([test "x$platform_win32" = "xyes" && test "x$enable_debug" != "xyes"],[
    CC_CHECK_CFLAGS_APPEND([-mwindows])
])


# Checks for libraries.
AS_IF([test "x$platform_win32" = "xyes"],[
   PKG_CHECK_MODULES([DEPS], [zlib])
   PKG_CHECK_MODULES([PNG], [libpng])
   AC_DEFINE([_WIN32_IE],[0x0300],[Minimum support level selection])
   AC_CHECK_TOOL([WINDRES], [windres], [:])
],[test "x$platform_msdos" = "xyes"],[
  PKG_CHECK_MODULES([PNG], [libpng])
  AX_CHECK_COMPILE_FLAG([-fgnu89-inline],
    [AX_APPEND_FLAG([-fgnu89-inline])],
    [AC_MSG_WARN([-fgnu89-inline is not supported, linking with allegro might fail with gcc > 2.x])
  ])
],[
  #Implicit Unix
  PKG_CHECK_MODULES([DEPS], [glib-2.0])
  PKG_CHECK_MODULES([PNG], [libpng])
])

AS_IF([test "x$enable_gtk_panel" != "xno"], [
    AS_IF([test "x$platform_unix" != "xyes"],[
        AC_MSG_ERROR(["GTK panel is not supported on $host_os"])
    ],[]);
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    AM_CONDITIONAL(ENABLE_GTK_PANEL, true)
    AC_DEFINE(ENABLE_GTK_PANEL,[],[Use GTK for GUI])
    AC_PROG_GCR
    if test "x$GCR" != "xyes" ; then
        AC_MSG_ERROR([glib-compile-resource needed for GTK/X11 backend and for GTK Panel])
    fi
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
    AM_CONDITIONAL(GFX_BACKEND_SDL2, false)
    AC_DEFINE(GFX_BACKEND_ALLEGRO,[],[Use Allegro for gfx rendering])
    AC_SUBST(GFX_BACKEND,[allegro])
    PKG_CHECK_MODULES([ALLEGRO], [allegro])
    PKG_CHECK_MODULES([PNG], [libpng])
    AS_IF([test "x$platform_unix" = "xyes"],[
        AC_DEFINE(ALLEGRO_UNIX,[],[Building for Allegro on Unix])
    ],[]);
   ],
   ["xgtk-x11"],[
    AS_IF([test "x$platform_unix" != "xyes"],[
        AC_MSG_ERROR(["GTK/X11 graphic backend is not supported on $host_os"])
    ],[]);
    AM_CONDITIONAL(GFX_BACKEND_GTK_X11, true)
    AM_CONDITIONAL(GFX_BACKEND_ALLEGRO, false)
    AM_CONDITIONAL(GFX_BACKEND_SDL2, false)
    AC_DEFINE(GFX_BACKEND_GTK_X11,[],[Use GTK/X11 for gfx rendering])
    AC_SUBST(GFX_BACKEND,[gtk_x11])
    PKG_CHECK_MODULES([GTK], [gtk+-3.0])
    PKG_CHECK_MODULES([XEXT], [xext])
    PKG_CHECK_MODULES([X11], [x11])
    PKG_CHECK_MODULES([ALSA], [alsa])
    AC_PROG_GCR
    if test "x$GCR" != "xyes" ; then
        AC_MSG_ERROR([glib-compile-resource needed for GTK/X11 backend and for GTK Panel])
    fi
   ],
   ["xsdl2"],[
    AM_CONDITIONAL(GFX_BACKEND_GTK_X11, false)
    AM_CONDITIONAL(GFX_BACKEND_ALLEGRO, false)
    AM_CONDITIONAL(GFX_BACKEND_SDL2, true)
    AC_DEFINE(GFX_BACKEND_SDL2,[],[Use SDL2 for gfx rendering])
    AC_SUBST(GFX_BACKEND,[sdl2])
    PKG_CHECK_MODULES([SDL2], [sdl2])
    AC_DEFINE(WITH_SDL2,[1],[Use SDL2 features that were not in SDL1.2])
   ],
   [
    AC_MSG_ERROR(["Valid values for --with-gfx-backend are 'allegro','gtk-x11' and 'sdl2'"])
   ]
)

if test "x$enable_extra_tools" = "xyes"; then
    AM_CONDITIONAL(BUILD_EXTRA_TOOLS, true)
    if test "x$platform_unix" = "xyes"; then
        PKG_CHECK_MODULES([GTK], [gtk+-3.0],[
            has_gtk=yes
        ],[
            has_gtk=no
        ])
        PKG_CHECK_MODULES([X11], [x11],[
                has_x11=yes
        ],[
                has_x11=no
        ])
        PKG_CHECK_MODULES([GTK], [gtk+-3.0])
        AM_CONDITIONAL(BUILD_CC90, [test "x$has_gtk" = "xyes" && test "x$has_x11" = "xyes"])
    else
        if test "x$platform_win32" = "xyes"; then
            AM_CONDITIONAL(BUILD_CC90,true)
        else
            AM_CONDITIONAL(BUILD_CC90, false)
        fi
    fi
else
    AM_CONDITIONAL(BUILD_EXTRA_TOOLS, false)
    AM_CONDITIONAL(BUILD_CC90, false)
fi


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

# Debian-specific portions
AS_IF([test "x$enable_debian_build" = "xyes"],[
    AM_CONDITIONAL(DEBIAN_BUILD, true)
],[
    AM_CONDITIONAL(DEBIAN_BUILD, false)
])



################################
# Compile flags, debug/profile
################################
AC_MSG_CHECKING([Computing E_AM_CFLAGS])
E_AM_CFLAGS="-Wall"
OPTS_FLAGS="-O2 -funroll-loops -ffast-math"
if test "x$enable_debug" = "xyes"; then
    enable_optimization=no
    enable_machine_optimization=no
    E_AM_CFLAGS+=" -O0 -g3 -DDEBUG"
fi
if test "x$enable_profiling" = "xyes"; then
    E_AM_CFLAGS+=" -pg"
fi
if test "x$enable_optimization" = "xyes"; then
    E_AM_CFLAGS+=" $OPTS_FLAGS"
fi
if test "x$enable_machine_optimization" = "xyes"; then
    E_AM_CFLAGS+=" $OPTS_FLAGS -march=native -mtune=native"
fi
#TODO: Look into this
if test "x$enable_debian_build" = "xyes"; then
    E_AM_CFLAGS+=" -DDEBIAN_BUILD"
fi


AC_MSG_RESULT([$E_AM_CFLAGS])
AC_SUBST(AM_CFLAGS, [$E_AM_CFLAGS])


AC_CONFIG_FILES([Makefile
                 po/Makefile.in
                 src/Makefile
                 data/Makefile])
AM_COND_IF([GFX_BACKEND_ALLEGRO],
           [AC_CONFIG_FILES([src/alleg/Makefile])])
AM_COND_IF([GFX_BACKEND_SDL2],
           [AC_CONFIG_FILES([src/sdl2/Makefile])])
AM_COND_IF([PLATFORM_MSDOS],
           [AC_CONFIG_FILES([src/dos/Makefile])])
AM_COND_IF([PLATFORM_UNIX],
           [AC_CONFIG_FILES([src/linux/Makefile])])
AM_COND_IF([PLATFORM_WIN32],[
    AC_CONFIG_FILES([src/win/Makefile])
    AC_CONFIG_FILES([data/nsis/teo.nsis])
    AC_CONFIG_FILES([src/win/wdialog.rc])
])
AM_COND_IF([INSTALL_DOC],[
    AC_CONFIG_FILES([doc/Makefile])
])
AM_COND_IF([BUILD_EXTRA_TOOLS],
           [AC_CONFIG_FILES([tools/Makefile
                             tools/cc90hfe/Makefile
                             tools/cc90hfe/src/Makefile
                             tools/k7tools/Makefile
                             tools/sap/Makefile
           ])])

AM_COND_IF([BUILD_EXTRA_TOOLS],[
    AM_COND_IF([PLATFORM_WIN32],[
        AC_CONFIG_FILES([tools/cc90hfe/src/win/Makefile])
    ])
])
AM_COND_IF([BUILD_EXTRA_TOOLS],[
    AM_COND_IF([PLATFORM_UNIX],[
        AC_CONFIG_FILES([tools/cc90hfe/src/linux/Makefile])
    ])
])

AC_OUTPUT
