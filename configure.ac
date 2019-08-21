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


PKG_CHECK_MODULES([DEPS], [glib-2.0 allegro])
PKG_CHECK_MODULES([PNG], [libpng])


AM_CONDITIONAL([FOO], [test 1 = 2])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/alleg/Makefile
                 src/linux/Makefile])
AC_OUTPUT
