#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gdice], [0.1.4], [https://github.com/fluks/gdice/issues], [], \
    [https://github.com/fluks/gdice])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC

# Checks for libraries.

GLIB_GSETTINGS

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([inttypes.h libintl.h limits.h malloc.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 res/Makefile
                 po/Makefile.in])

AC_ARG_ENABLE([gstreamer],
    [AS_HELP_STRING([--disable-gstreamer], [disable gstreamer])], ,
    [PKG_CHECK_MODULES([GSTREAMER], [gstreamer-1.0],
        [AC_DEFINE([HAVE_GSTREAMER], [1], [Defined if have gstreamer.])])])

PKG_CHECK_MODULES([GTK], [gtk+-3.0])
PKG_CHECK_MODULES([GLIB], [glib-2.0])

LIBS="$GTK_LIBS $GLIB_LIBS $GSTREAMER_LIBS"
AC_SUBST([AM_CPPFLAGS],
    ['$(GTK_CFLAGS) $(GLIB_CFLAGS) $(GSTREAMER_CFLAGS)'])

GETTEXT_PACKAGE=gdice
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [The domain to use with gettext])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])
# Internationalization.
IT_PROG_INTLTOOL([0.35.0])

PROGRAMNAME_LOCALEDIR=[${datadir}/locale]
AC_SUBST(PROGRAMNAME_LOCALEDIR)

AC_OUTPUT
