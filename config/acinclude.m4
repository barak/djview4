dnl Copyright (c) 2002  Leon Bottou and Yann Le Cun.
dnl Copyright (c) 2001  AT&T
dnl
dnl Most of these macros are derived from macros listed
dnl at the GNU Autoconf Macro Archive
dnl http://www.gnu.org/software/ac-archive/
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl


dnl ------------------------------------------------------- 
dnl @synopsis AC_DEFINE_VERSION
dnl Defines a hexadecimal version number
dnl -------------------------------------------------------

AC_DEFUN([AC_DEFINE_VERSION], [
  IFS=. read major minor revis <<__eof__
$1
__eof__
  test -z "$major" && major=0
  test -z "$minor" && minor=0
  test -z "$revis" && revis=0
  hexversion=`printf '0x%x%02x%02x' $major $minor $revis`
  AC_DEFINE_UNQUOTED(DJVIEW_VERSION,[$hexversion],[hexadecimal version number])
  AC_DEFINE_UNQUOTED(DJVIEW_VERSION_STR, ["$1"], [string])
  AC_DEFINE_UNQUOTED(RC_VERSION,[$major,$minor,$revis,0],[rc file version number])
  AC_DEFINE_UNQUOTED(RC_VERSION_STR,["$major,$minor,$revis,0\0"],[rc file version string])
])


dnl -------------------------------------------------------
dnl @synopsis AC_CHECK_CXX_OPT(OPTION,
dnl               ACTION-IF-OKAY,ACTION-IF-NOT-OKAY)
dnl Check if compiler accepts option OPTION.
dnl -------------------------------------------------------
AC_DEFUN([AC_CHECK_CXX_OPT],[
 opt="$1"
 AC_MSG_CHECKING([if $CXX accepts $opt])
 echo 'void f(){}' > conftest.cc
 if test -z "`${CXX} ${CXXFLAGS} ${OPTS} $opt -c conftest.cc 2>&1`"; then
    AC_MSG_RESULT(yes)
    rm conftest.* 
    $2
 else
    AC_MSG_RESULT(no)
    rm conftest.*
    $3
 fi
])

dnl -------------------------------------------------------
dnl @synopsis AC_CXX_OPTIMIZE
dnl Setup option --enable-debug
dnl Collects optimization/debug option in variable OPTS
dnl Filter options from CFLAGS and CXXFLAGS
dnl -------------------------------------------------------
AC_DEFUN([AC_CXX_OPTIMIZE],[
   AC_ARG_ENABLE(debug,
        AS_HELP_STRING([--enable-debug],
                       [Compile with debugging options (default: no)]),
        [ac_debug=$enableval],[ac_debug=no])
   OPTS=
   AC_SUBST(OPTS)
   saved_CXXFLAGS="$CXXFLAGS"
   saved_CFLAGS="$CFLAGS"
   CXXFLAGS=
   CFLAGS=
   for opt in $saved_CXXFLAGS ; do
     case $opt in
       -g*) test $ac_debug != no && OPTS="$OPTS $opt" ;;
       -O*) ;;
       *) CXXFLAGS="$CXXFLAGS $opt" ;;
     esac
   done
   for opt in $saved_CFLAGS ; do
     case $opt in
       -O*|-g*) ;;
       *) CFLAGS="$CFLAGS $opt" ;;
     esac
   done
   if test x$ac_debug = xno ; then
     OPTS=-DNDEBUG
     AC_CHECK_CXX_OPT([-Wall],[OPTS="$OPTS -Wall"])
     AC_CHECK_CXX_OPT([-O2],[OPTS="$OPTS -O2"])
   else
     AC_CHECK_CXX_OPT([-Wall],[OPTS="$OPTS -Wall"])
   fi
])


dnl ------------------------------------------------------------------
dnl @synopsis AC_PATH_TIFF([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl Process option --with-tiff
dnl Search LIBTIFF. Define HAVE_TIFF.
dnl Set output variable TIFF_CFLAGS and TIFF_LIBS
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PATH_TIFF],
[
  AC_ARG_VAR(TIFF_LIBS, [Libraries for tiff])
  AC_ARG_VAR(TIFF_CFLAGS, [Compilation flags for tiff])
  ac_tiff=no
  AC_ARG_WITH(tiff,
     AS_HELP_STRING([--with-tiff=DIR],
                    [where libtiff is located]),
     [ac_tiff=$withval], [ac_tiff=yes] )
  # Process specification
  if test x$ac_tiff = xyes ; then
     test x${TIFF_LIBS+set} != xset && TIFF_LIBS="-ltiff"
  elif test x$ac_tiff != xno ; then
     test x${TIFF_LIBS+set} != xset && TIFF_LIBS="-L$ac_tiff/lib -ltiff"
     test x${TIFF_CFLAGS+set} != xset && TIFF_CFLAGS="-I$ac_tiff/include"
  fi
  # Try linking
  if test x$ac_tiff != xno ; then
     AC_MSG_CHECKING([for the libtiff library])
     save_CFLAGS="$CFLAGS"
     save_CXXFLAGS="$CXXFLAGS"
     save_LIBS="$LIBS"
     CFLAGS="$CFLAGS $TIFF_CFLAGS"
     CXXFLAGS="$CXXFLAGS $TIFF_CFLAGS"
     LIBS="$LIBS $TIFF_LIBS"
     AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
      [[
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <tiffio.h>
#ifdef __cplusplus
}
#endif
      ]],
      [
TIFFOpen(0,0);
      ])],
      [ac_tiff=yes],
      [ac_tiff=no])
     CFLAGS="$save_CFLAGS"
     CXXFLAGS="$save_CXXFLAGS"
     LIBS="$save_LIBS"
     AC_MSG_RESULT($ac_tiff)
   fi
   # Finish
   if test x$ac_tiff = xno; then
      TIFF_CFLAGS= ; TIFF_LIBS=
      ifelse([$2],,:,[$2])
   else
      AC_DEFINE(HAVE_TIFF,1,[Define if you have libtiff.])
      ifelse([$1],,:,[$1])
   fi
])






dnl ------------------------------------------------------------------
dnl @synopsis AC_PROG_PKG_CONFIG([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl Sets output variables PKG_CONFIG
dnl ------------------------------------------------------------------


AC_DEFUN([AC_PROG_PKG_CONFIG], 
[
  AC_ARG_VAR(PKG_CONFIG,[Location of the pkg-config program.])
  AC_ARG_VAR(PKG_CONFIG_PATH, [Path for pkg-config descriptors.])
  AC_PATH_PROG(PKG_CONFIG, pkg-config)
  if test -z "$PKG_CONFIG" ; then
      ifelse([$2],,:,[$2])
  else
      ifelse([$1],,:,[$1])
  fi
])




dnl ------------------------------------------------------------------
dnl @synopsis AC_PATH_DDJVUUAPI([action-if-found],[action-if-notfound])
dnl Search for ddjvuapi.  Defines HAVE_DDJVUAPI.
dnl Sets output variables DDJVUAPI_CFLAGS and DDJVUAPI_LIBS
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PATH_DDJVUAPI],
[
  AC_REQUIRE([AC_PROG_PKG_CONFIG])        
  AC_ARG_VAR(DDJVUAPI_LIBS, [Libraries for ddjvuapi])
  AC_ARG_VAR(DDJVUAPI_CFLAGS, [Compilation flags for ddjvuapi])
  AC_MSG_CHECKING([for ddjvuapi])
  if test -n "$DDJVUAPI_LIBS" ; then
    AC_MSG_RESULT([user defined])
    AC_MSG_RESULT([using DDJVUAPI_LIBS=$DDJVUAPI_LIBS])
    AC_MSG_RESULT([using DDJVUAPI_CFLAGS=$DDJVUAPI_CFLAGS])
  elif test -x "$PKG_CONFIG" ; then
    if $PKG_CONFIG ddjvuapi ; then
       ac_ddjvuapi=yes
       AC_MSG_RESULT([found])
       DDJVUAPI_LIBS=`$PKG_CONFIG --libs ddjvuapi`
       DDJVUAPI_CFLAGS=`$PKG_CONFIG --cflags ddjvuapi`
       AC_DEFINE(HAVE_DDJVUAPI,1,[Define if you have ddjvuapi.])
       ifelse([$1],,:,[$1])
    else
       AC_MSG_RESULT([not found by pkg-config])
       ac_ddjvuapi=no
       ifelse([$2],,:,[$2])
    fi
  else
    AC_MSG_RESULT([no pkg-config])
    ac_ddjvuapi=no
    ifelse([$2],,:,[$2])
  fi
])



dnl ------------------------------------------------------------------
dnl @synopsis AC_PATH_GLIB([action-if-found],[action-if-notfound])
dnl Search for glib.  Defines HAVE_GLIB.
dnl Sets output variables GLIB_CFLAGS and GLIB_LIBS
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PATH_GLIB],
[
  AC_REQUIRE([AC_PROG_PKG_CONFIG])        
  AC_ARG_VAR(GLIB_LIBS, [Libraries for glib-2.0])
  AC_ARG_VAR(GLIB_CFLAGS, [Compilation flags for glib-2.0])
  AC_MSG_CHECKING([for glib])
  if test -x "$PKG_CONFIG" ; then
    if $PKG_CONFIG glib-2.0 ; then
       ac_glib=yes
       AC_MSG_RESULT([found])
       GLIB_LIBS=`$PKG_CONFIG --libs glib-2.0`
       GLIB_CFLAGS=`$PKG_CONFIG --cflags glib-2.0`
       AC_DEFINE(HAVE_GLIB,1,[Define if you have glib-2.0.])
       ifelse([$1],,:,[$1])
    else
       AC_MSG_RESULT([not found by pkg-config])
       ac_glib=no
       ifelse([$2],,:,[$2])
    fi
  else
    AC_MSG_RESULT([no pkg-config])
    ac_glib=no
    ifelse([$2],,:,[$2])
  fi
])



dnl ------------------------------------------------------------------
dnl @synopsis AC_PROGS_QT4
dnl Sets output variables QMAKE, MOC, UIC, RCC, LUPDATE, LRELEASE
dnl Prints an error message if QMAKE is not found or not suitable
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PROGS_QT],
[
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])
  AC_ARG_VAR(QMAKE,[Location of the QMAKE program.])
  AC_ARG_VAR(QMAKESPEC,[Location of the QMAKE specifications.])
  AC_ARG_VAR(MOC,[Location of the MOC program.])
  AC_ARG_VAR(UIC,[Location of the UIC program.])
  AC_ARG_VAR(RCC,[Location of the RCC program.])
  AC_ARG_VAR(LUPDATE,[Location of the LUPDATE program.])
  AC_ARG_VAR(LRELEASE,[Location of the LRELEASE program.])
  AC_ARG_VAR(QTDIR,[Location of the Qt package (deprecated).])

  path=$PATH
  if test -n "$QTDIR" && test -d "$QTDIR/include" ; then
    if test -d "$QTDIR/include/Qt" ; then :
    elif test -f "$QTDIR/include/qobject.h" ; then
      AC_MSG_ERROR([We want Qt4 or Qt5 but your QTDIR variable points to a Qt3 install.
Please check variables QTDIR, QMAKE, QMAKESPEC, etc..
Unsetting them is better than setting them wrong.])
    fi
  fi
  if test -n "$QTDIR" && test -d "$QTDIR/bin" ; then
    path=$QTDIR/bin:$PATH
  fi
  if test -z "$QMAKE" ; then
    AC_PATH_PROGS([QMAKE], [qmake], [], [$path])
  fi
  if test -z "$QMAKE" ; then
    AC_MSG_ERROR([Cannot find the Qt program qmake. 
Please define variable QMAKE and possibly QMAKESPEC.
Defining QTDIR can help although it is deprecated.])
  fi
  mkdir conftest.d
  cat > conftest.d/conftest.pro <<\EOF
changequote(<<, >>)dnl
message(QMAKE_UIC="$$QMAKE_UIC")dnl qt4 only
message(QMAKE_MOC="$$QMAKE_MOC")dnl qt4 only
message(QT_VERSION="$$[QT_VERSION]")
message(QT_INSTALL_PREFIX="$$[QT_INSTALL_PREFIX]")
message(QT_INSTALL_DATA="$$[QT_INSTALL_DATA]")
message(QT_INSTALL_HEADERS="$$[QT_INSTALL_HEADERS]")
message(QT_INSTALL_BINS="$$[QT_INSTALL_BINS]")
changequote([, ])dnl
EOF
  if ( cd conftest.d && $QMAKE > conftest.out 2>&1 ) ; then
    sed -e 's/^.*: *//' < conftest.d/conftest.out > conftest.d/conftest.sh
    . conftest.d/conftest.sh
    rm -rf conftest.d
  else
    rm -rf conftest.d
    AC_MSG_ERROR([Cannot successfully run Qt program qmake. 
Please define variable QMAKE to a working qmake.
If you define QMAKESPEC, make sure it is correct.])
  fi
  AC_MSG_CHECKING([Qt version])
  case "$QT_VERSION" in
    4.*)
      AC_MSG_RESULT([qt4 ($QT_VERSION)]) 
      qtversion=qt4
      ;;
    5.*)
      AC_MSG_RESULT([qt5 ($QT_VERSION)]) 
      qtversion=qt5
      ;;
    *)
      AC_MSG_RESULT([$QT_VERSION]) 
      AC_MSG_ERROR([Unrecognized Qt version. Please define variable QMAKE.])
      ;;
  esac
  test -z "$QTDIR" && test -n "$QT_INSTALL_DATA" && \
    test -d "$QT_INSTALL_DATA/bin" && QTDIR="$QT_INSTALL_DATA"
  path=$PATH
  test -n "$QTDIR" && test -d "$QTDIR/bin" && path=$QTDIR/bin:$path
  test -d "$QT_INSTALL_BINS" && path=$QT_INSTALL_BINS:$path
  if test `basename "$QMAKE"` = qmake-$qtversion ; then
    altmoc="moc-${qtversion}"
    altuic="uic-${qtversion}"
    altrcc="rcc-${qtversion}"
    altlupdate="lupdate-${qtversion}"
    altlrelease="lrelease-${qtversion}"
  else
    AC_MSG_CHECKING([for real qmake path])
    test -x "$QT_INSTALL_BINS/qmake" && QMAKE="$QT_INSTALL_BINS/qmake"
    AC_MSG_RESULT([$QMAKE])
  fi
  test -x "$QMAKE_MOC" && test -z "$MOC" && MOC="$QMAKE_MOC"
  test -x "$QMAKE_UIC" && test -z "$UIC" && UIC="$QMAKE_UIC"
  AC_PATH_PROGS([MOC], [$altmoc moc], [], [$path])
  AC_PATH_PROGS([UIC], [$altuic uic], [], [$path])
  AC_PATH_PROGS([RCC], [$altrcc rcc], [], [$path])
  AC_PATH_PROGS([LUPDATE], [${altlupdate} lupdate], [], [$path])
  AC_PATH_PROGS([LRELEASE], [${altlrelease} lrelease], [], [$path])
  PATH=$path
])


