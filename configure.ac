#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([Cavepilot], [0.5.0], [lasso@spacecentre.se], [cavepilot])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([Cavepilot.hh.in])
AC_CONFIG_HEADER([Cavepilot.hh])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_CANONICAL_HOST

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gethostbyname memset socket strchr strncasecmp])

# not a beauty, but it does its job...
case "$host" in
    *-*-cygwin*)
        INETLIB="-lwsock32"
	AC_DEFINE_UNQUOTED(_WINDOWS, [], [Cygwin build?])
        ;;
    *)
        INETLIB=""
        ;;
esac
AC_SUBST(INETLIB)

# this can be set to try the game without 'make install' (mainly for devel)
AC_ARG_ENABLE([devel], AC_HELP_STRING([--enable-devel], [For development to try the game without installing it]), [DEVEL=yes], [DEVEL=no])

if test "$DEVEL" = "yes"; then
   AC_DEFINE([_DEVEL], [], [For development to try the game without installing it])
fi

# Check for SDL
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION,
            :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
