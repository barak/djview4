# -*-Shell-script-*-
#
# DjVuLibre DjView
# Copyright (c) 2006-  Leon Bottou
#
# This software is subject to, and may be distributed under,
# the GNU General Public License, Version 2.
# The license should have accompanied the software or 
# you may obtain a copy of the license from the 
# Free Software Foundation at http://www.fsf.org .
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# $Id$
#
# ----------------------------------------
# Initialization
# ----------------------------------------

AC_PREREQ(2.50)
AC_INIT(djview, 4.0.0)
AC_REVISION($Id$)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(README)
AC_CONFIG_HEADER(config.h:config/config.h.in)
AC_PREFIX_DEFAULT(/usr/local)

# Additional libraries
AC_ARG_WITH(extra-includes,
            AC_HELP_STRING([--with-extra-includes=DIR],
                       [Define an additional directory for include files]),
        [ if test -d "$withval" ; then
            CPPFLAGS="$CPPFLAGS -I$withval" 
          else
            AC_MSG_ERROR([Cannot stat directory $withval])
          fi ] )

AC_ARG_WITH(extra-libraries,
            AC_HELP_STRING([--with-extra-libraries=DIR],
                       [Define an additional directory for library files]),
        [ if test -d "$withval" ; then
           LDFLAGS="$LDFLAGS -L$withval"
          else
            AC_MSG_ERROR([Cannot stat directory $withval])
          fi ] )

# Always look into a "gnu" directory.
curwd=`pwd`
if test -d $curwd/gnu/include ; then
   CPPFLAGS="$CPPFLAGS -I$curwd/gnu/include"
fi
if test -d $curwd/gnu/lib ; then
   LDFLAGS="$LDFLAGS -L$curwd/gnu/lib"
fi

# Special cases
case "$host" in
  *-darwin* | *-macos10*)
     if test -d /opt/local ; then
       CPPFLAGS="$CPPFLAGS -I/opt/local/include"
       LDFLAGS="$LDFLAGS -L/opt/local/lib"
     elif test -d /sw ; then
       CPPFLAGS="$CPPFLAGS -I/sw/include"
       LDFLAGS="$LDFLAGS -L/sw/lib"
     fi
  ;;
esac



# ----------------------------------------
# Programs
# ----------------------------------------

# Does make set variable $(MAKE)
AC_PROG_MAKE_SET

# Programs
AC_PROG_LN_S
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(AR, ar)
AC_PATH_PROG(TOUCH, touch)
AC_PATH_PROG(MKDIR, mkdir)
AC_PATH_PROG(PKGCONFIG, pkg-config)
AC_PROG_INSTALL

# Libtool & Compilers
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CXX
AC_SUBST(LIBTOOL_DEPS)

# C++
AC_LANG(C++)
AC_CXX_OPTIMIZE

# ----------------------------------------
# Header Files
# ----------------------------------------

AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT     
AC_CHECK_HEADERS(unistd.h)


# ----------------------------------------
# Functions
# ----------------------------------------

AC_CHECK_FUNCS(sigaction waitpid wait3)


# ----------------------------------------
# Packages
# ----------------------------------------

AC_PATH_QT4
AC_PATH_TIFF

# ----------------------------------------
# How to compile and install nsdejavu
# ----------------------------------------

AC_PATH_X
AC_PATH_XTRA
NSDEJAVU_CFLAGS=
NSDEJAVU_LIBS=
NSDEJAVU_CFLAGS=${X_CFLAGS}
case "$host" in
 *-cygwin* | *-mingw*)
   NSDEJAVU_LIBS="`echo ${X_LIBS} | sed -e 's/-lXext/-lXt -lXext/'`"
   NSDEJAVU_LIBS="-no-undefined $NSDEJAVU_LIBS"
   ;;
esac
AC_SUBST(NSDEJAVU_CFLAGS)
AC_SUBST(NSDEJAVU_LIBS)

if test -z "$INSTALL_SHLIB" ; then
  INSTALL_SHLIB="${INSTALL_DATA}"
  case "$host" in
    *-cygwin* | *-mingw*)
        INSTALL_SHLIB="cp"
        ;;
  esac
fi
AC_SUBST(INSTALL_SHLIB)


# ----------------------------------------
# End
# ----------------------------------------

# Define installation paths and version string
AC_DEFINE_INSTALL_PATHS
AC_DEFINE_VERSION([DJVIEW_VERSION], [${PACKAGE_VERSION}])

# Output files
AC_CONFIG_FILES(Makefile)
AC_OUTPUT

# Run qmake

