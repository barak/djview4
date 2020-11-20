# -*-Shell-script-*-
#
# DjVuLibre DjView
# Copyright (c) 2006-  Leon Bottou
#
# This software is subject to, and may be distributed under,
# the GNU General Public License, either version 2 of the license 
# or (at your option) any later version.
# The license should have accompanied the software or 
# you may obtain a copy of the license from the 
# Free Software Foundation at http://www.fsf.org .
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# ----------------------------------------
# Initialization
# ----------------------------------------

m4_define([v_maj], [4])
m4_define([v_min], [12])
m4_define([v_ver], [v_maj.v_min])


AC_PREREQ([2.65])
AC_INIT([djview],[v_ver])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_SRCDIR([src/djview.cpp])
AC_CONFIG_HEADER(config.h:config/config.h.in)
AC_PREFIX_DEFAULT(/usr/local)
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.6 subdir-objects dist-bzip2 dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])


# Additional libraries
AC_ARG_WITH(extra-includes,
            AS_HELP_STRING([--with-extra-includes=DIR],[Define an additional directory for include files]),
        [ if test -d "$withval" ; then
            CFLAGS="$CFLAGS -I$withval" 
            CXXFLAGS="$CXXFLAGS -I$withval" 
          else
            AC_MSG_ERROR([Cannot stat directory $withval])
          fi ] )

AC_ARG_WITH(extra-libraries,
            AS_HELP_STRING([--with-extra-libraries=DIR],[Define an additional directory for library files]),
        [ if test -d "$withval" ; then
           LDFLAGS="$LDFLAGS -L$withval"
          else
            AC_MSG_ERROR([Cannot stat directory $withval])
          fi ] )

# Always look into a "gnu" directory.
curwd=`pwd`
if test -d $curwd/gnu/include ; then
   CFLAGS="$CFLAGS -I$curwd/gnu/include"   
   CXXFLAGS="$CXXFLAGS -I$curwd/gnu/include"
fi
if test -d $curwd/gnu/lib ; then
   LDFLAGS="$LDFLAGS -L$curwd/gnu/lib"
fi

# Special cases
ismac=no
case "$host" in
  *-darwin* | *-macos10*)
     ismac=yes
     if test -d /usr/local ; then
       CFLAGS="$CFLAGS -I/usr/local/include"
       CXXFLAGS="$CXXFLAGS -I/usr/local/include"
       CPPFLAGS="$CPPFLAGS -I/usr/local/include"
       LDFLAGS="$LDFLAGS -L/usr/local/lib"
     elif test -d /opt/local ; then
       CFLAGS="$CFLAGS -I/opt/local/include"
       CXXFLAGS="$CXXFLAGS -I/opt/local/include"
       CPPFLAGS="$CPPFLAGS -I/opt/local/include"
       LDFLAGS="$LDFLAGS -L/opt/local/lib"
     elif test -d /sw ; then
       CFLAGS="$CFLAGS -I/sw/include"
       CXXFLAGS="$CXXFLAGS -I/sw/include"
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
AC_PROG_MKDIR_P
AC_PROG_SED
AC_PROG_INSTALL
AC_PATH_PROG(RM, rm)
AC_PATH_PROG(RMDIR, rmdir)
AC_PATH_PROG(TOUCH, touch)
AC_PATH_PROG(MKDIR, mkdir)
AC_PATH_PROG(AWK, awk)

# tweak rm to make libtool happy
AC_PATH_PROG(RM, rm) 
RM="$RM -f"

# Libtool & Compilers
AC_PROG_CC
AC_PROG_CXX
AC_PROG_OBJC
AC_SUBST(LIBTOOL_DEPS)

# C++
AC_LANG(C++)
AC_CXX_OPTIMIZE

# Only nsdejavu needs libtool.
# But some combinations of libtool and autoconf
# break when libtool is in a conditional path.
AC_DISABLE_STATIC
LT_INIT

# SVG conversion tool
AC_PATH_PROG([RSVG], [rsvg-convert], [false])
AC_PATH_PROG([INKSCAPE], [inkscape], [false])
AC_PATH_PROG([CONVERT], [convert], [false])
conversion_tool=no
if test "x${RSVG}" != xfalse ; then
  conversion_tool=rsvg
elif test "x${INKSCAPE}" != xfalse ; then
  conversion_tool=inkscape
elif test "x${CONVERT}" != xfalse ; then
  ( ${CONVERT} --version 2>&1 | grep -q ImageMagick ) && conversion_tool=convert
fi
AM_CONDITIONAL([HAVE_CONVERSION_RSVG], [test x${conversion_tool} = xrsvg])
AM_CONDITIONAL([HAVE_CONVERSION_INKSCAPE], [test x${conversion_tool} = xinkscape])
AM_CONDITIONAL([HAVE_CONVERSION_CONVERT], [test x${conversion_tool} = xconvert])
AM_CONDITIONAL([HAVE_CONVERSION_TOOL], [test x${conversion_tool} != xno])


# ----------------------------------------
# Header Files
# ----------------------------------------

AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_HEADER_SYS_WAIT     
AC_CHECK_HEADERS(unistd.h string.h strings.h getopt.h)


# ----------------------------------------
# Functions
# ----------------------------------------

AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_CHECK_FUNCS(sigaction strerror waitpid wait3)


# ----------------------------------------
# Packages
# ----------------------------------------

AC_PROG_PKG_CONFIG([],[
  AC_MSG_ERROR([Cannot find required program pkg-config.])
])

AC_PROGS_QT([],[
  AC_MSG_ERROR([Cannot find required Qt programs.])
])

AC_PATH_DDJVUAPI([],[
  AC_MSG_ERROR([Cannot find required package ddjvuapi.])
])

AC_PATH_TIFF([],[
  AC_MSG_WARN([Cannot find libtiff (required for tiff export).])
])


# ----------------------------------------
# nsdejavu
# ----------------------------------------

ac_nsdejavu=yes
AC_ARG_ENABLE(nsdejavu,
    AS_HELP_STRING([--disable-nsdejavu],
        [Do not compile the nsdejavu browser plugin. @<:@default=enabled@:>@]),
    [ac_nsdejavu=$enableval])

if test "$ac_nsdejavu" != no ; then
    AC_PATH_X
    AC_PATH_XTRA
    if test "$no_x" = yes; then
        ac_nsdejavu=no
        AC_MSG_WARN([Disabling plugin compilation (missing X11 include files).])
    fi
fi

if test "$ac_nsdejavu" != no ; then
    ac_xt=yes
    ac_glib=yes
    AC_PATH_GLIB([],[ac_glib=no])
    savedCFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS ${X_CFLAGS}"
    AC_CHECK_HEADER(X11/Shell.h, [], [ac_xt=no])
    AC_CHECK_HEADER(X11/IntrinsicP.h, [], [ac_xt=no])
    CFLAGS="$savedCFLAGS"
    if test "$ac_xt" != no ; then
        AC_DEFINE(HAVE_XT,1,[Define if you have xt include files.])
        if test "$ac_glib" = no ; then
            AC_MSG_WARN([GLib not found. Compiling plugin without XEmbed.])
        fi
    elif test "$ac_glib" != no ; then
        AC_MSG_WARN([Xt not found. Compiling plugin with XEmbed only.])
    else
        ac_nsdejavu=no
        AC_MSG_WARN([Disabling plugin compilation (need Xt and/or Glib2.)])
    fi
fi

if test "$ac_nsdejavu" != no && test "$qtversion" == "qt4" ; then
    ac_qt_x11=no
    AC_LANG_PUSH(C++)
    saved_CXXFLAGS=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS -I$QT_INSTALL_HEADERS -I$QT_INSTALL_HEADERS/QtCore"
    AC_CHECK_DECL(Q_WS_X11, ac_qt_x11=yes, , [#include <QtGlobal>])
    CXXFLAGS=$saved_CXXFLAGS
    AC_LANG_POP(C++)
    if test "$ac_qt_x11" != yes ; then
        ac_nsdejavu=no
        AC_MSG_WARN([Disabling plugin compilation (need X11-based version of Qt.)])
        AC_MSG_NOTICE([If you have an X11-supporting Qt, try:])
        AC_MSG_NOTICE([  CPPFLAGS=-D__USE_WS_X11__ ./configure ...])
    fi 
fi

if test "$ac_nsdejavu" != no ; then
    # prepare flags
    NSDEJAVU_LIBS=
    NSDEJAVU_CFLAGS="${GLIB_CFLAGS} ${X_CFLAGS}"
    XT_LIBS=
    if test "$ac_xt" = yes ; then
        XT_LIBS="$X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS"
        XT_LIBS="`echo ${XT_LIBS} | sed -e 's/-lXext/-lXt -lXext/'`"
    fi
    case "$host" in
        *-cygwin* | *-mingw*)
            NSDEJAVU_LIBS="-no-undefined $GLIB_LIBS $XT_LIBS"
            ;;
        *-darwin* | *-macos10*)
            NSDEJAVU_LIBS="$GLIB_LIBS $XT_LIBS"
            ;;
    esac
    AC_SUBST(NSDEJAVU_CFLAGS)
    AC_SUBST(NSDEJAVU_LIBS)
fi

AM_CONDITIONAL([WANT_NSDEJAVU], [test "x${ac_nsdejavu}" != "xno"])


# ----------------------------------------
# npdjvu
# ----------------------------------------

ac_npdjvu=no
AC_ARG_ENABLE(npdjvu,
    AS_HELP_STRING([--enable-npdjvu],
        [Compile the npdjvu browser plugin (experimental). @<:@default=disabled@:>@]),
    [ac_npdjvu=$enableval])

AM_CONDITIONAL([WANT_NPDJVU], [test "x${ac_npdjvu}" != "xno"])

# ----------------------------------------
# mac
# ----------------------------------------

ac_mac=no
AC_ARG_ENABLE(mac,
    AS_HELP_STRING([--enable-mac],
        [Compile MacOS X specific tools @<:@default=disabled@:>@]),
    [ac_mac=$enableval])

if test x$ac_mac != xno && test x$ismac != xyes ; then
    AC_MSG_WARN([Disabling mac support (not a mac).)])
    ac_mac=no
fi

if test x$ac_mac != xno && test x$ismac = xyes ; then
    # hack to avoid redistributing avx specific binaries.						  
    AC_CHECK_CXX_OPT([-mno-avx],[MORE_CFLAGS="$MORE_CFLAGS -mno-avx"])
fi

						      
AM_CONDITIONAL([WANT_MAC], [test "x${ac_mac}" != "xno"])

# ----------------------------------------
# Desktop files
# ----------------------------------------

ac_desktopfiles=yes
test x$ismac = xyes && ac_desktopfiles=no
AC_ARG_ENABLE(desktopfiles,
    AS_HELP_STRING([--disable-desktopfiles], 
        [Do not install icons and mime types. @<:@kefault=enabled@:>@]),
    [ac_desktopfiles=$enableval])

AM_CONDITIONAL([WANT_DESKTOPFILES], [test "x${ac_desktopfiles}" != "xno"])

# ----------------------------------------
# MISC
# ----------------------------------------

# Define version string

AC_DEFINE_VERSION([${PACKAGE_VERSION}])

# ----------------------------------------
# Prepare qmake arguments
# ----------------------------------------

QMAKELIBS="$DDJVUAPI_LIBS $TIFF_LIBS $LIBS"
QMAKEFLAGS="$DDJVUAPI_CFLAGS $TIFF_CFLAGS $MORE_CFLAGS"
AC_SUBST(QMAKELIBS)
AC_SUBST(QMAKEFLAGS)

QMAKEARG="CONFIG+=autoconf"

if test "$ac_debug" = yes ; then
  QMAKEARG="$QMAKEARG CONFIG+=debug"
else
  QMAKEARG="$QMAKEARG CONFIG+=release"
fi

QMAKEARG="$QMAKEARG PACKAGE_VERSION=${PACKAGE_VERSION}"
if test -n "$QMAKESPEC" ; then
    QMAKEARG="-spec $QMAKESPEC $QMAKEARG"
fi

AC_SUBST(QMAKEARG)


# ----------------------------------------
# Outputs
# ----------------------------------------

AC_CONFIG_FILES([
Makefile
src/Makefile
nsdejavu/Makefile
desktopfiles/Makefile
npdjvu/Makefile
mac/Makefile
mac/mdimporter_src/Makefile
mac/qlgenerator_src/Makefile
])


AC_OUTPUT

# ----------------------------------------
# Fin
# ----------------------------------------

# Final message

echo
echo "$PACKAGE $VERSION"
echo
echo "Configuration Options Summary:"
echo
echo "  OS...................: ${host_os}"
echo "  Qt version...........: ${qtversion} (${QT_VERSION})"
echo "  TIFF support.........: ${ac_tiff}"
echo "  Desktop files........: ${ac_desktopfiles}"
echo "  Icon generation......: ${conversion_tool}"
echo "  Mac specific tools...: ${ac_mac}"
echo "  NSDEJAVU plugin......: ${ac_nsdejavu}"
echo "  NPDJVU plugin........: ${ac_npdjvu}"
echo
echo "Compilation............: make (or gmake)"
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo

