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
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111 USA
dnl


dnl -------------------------------------------------------
dnl @synopsis AC_DEFINE_VERSION
dnl Defines a hexadecimal version number
dnl -------------------------------------------------------

AC_DEFUN([AC_DEFINE_VERSION], [
  IFS=. read major minor revis <<__eof__
$2
__eof__
  test -z "$major" && major=0
  test -z "$minor" && minor=0
  test -z "$revis" && revis=0
  hexversion=`printf '0x%x%02x%02x' $major $minor $revis`
  AC_DEFINE_UNQUOTED($1,[$hexversion],[hexadecimal version number])
])


dnl -------------------------------------------------------
dnl @synopsis AC_DEFINE_INSTALL_PATHS
dnl Define various installation paths
dnl -------------------------------------------------------
AC_DEFUN([AC_DEFINE_INSTALL_PATHS],[
  save_prefix="${prefix}"
  save_exec_prefix="${exec_prefix}"
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix="$prefix"
  DIR_PREFIX="`eval echo \"$prefix\"`"
  AC_DEFINE_UNQUOTED(DIR_PREFIX,["${DIR_PREFIX}"],[directory "prefix"])
  DIR_EXEC_PREFIX="`eval echo \"$exec_prefix\"`"
  AC_DEFINE_UNQUOTED(DIR_EXEC_PREFIX,["${DIR_EXEC_PREFIX}"],[directory "exec_prefix"])
  DIR_BINDIR="`eval echo \"$bindir\"`"
  AC_DEFINE_UNQUOTED(DIR_BINDIR,["${DIR_BINDIR}"],[directory "bindir"])
  DIR_LIBDIR="`eval echo \"$libdir\"`"
  AC_DEFINE_UNQUOTED(DIR_LIBDIR,["${DIR_LIBDIR}"],[directory "libdir"])
  DIR_DATADIR="`eval echo \"$datadir\"`"
  AC_DEFINE_UNQUOTED(DIR_DATADIR,["${DIR_DATADIR}"],[directory "datadir"])
  DIR_MANDIR="`eval echo \"$mandir\"`"
  AC_DEFINE_UNQUOTED(DIR_MANDIR,["${DIR_MANDIR}"],[directory "mandir"])
  prefix="${save_prefix}"
  exec_prefix="${save_exec_prefix}"
])



dnl -------------------------------------------------------
dnl @synopsis AC_CHECK_CXX_OPT(OPTION,
dnl               ACTION-IF-OKAY,ACTION-IF-NOT-OKAY)
dnl Check if compiler accepts option OPTION.
dnl -------------------------------------------------------
AC_DEFUN(AC_CHECK_CXX_OPT,[
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
AC_DEFUN(AC_CXX_OPTIMIZE,[
   AC_ARG_ENABLE(debug,
        AC_HELP_STRING([--enable-debug],
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
     AC_HELP_STRING([--with-tiff=DIR],
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
     AC_TRY_LINK([
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h> 
#include <tiffio.h>
#ifdef __cplusplus
}
#endif ],[
TIFFOpen(0,0);],
       [ac_tiff=yes], [ac_tiff=no] )
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
      AC_MSG_RESULT([setting TIFF_CFLAGS=$TIFF_CFLAGS])
      AC_MSG_RESULT([setting TIFF_LIBS=$TIFF_LIBS])
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
dnl @synopsis AC_PATH_DDJVUAPI([action-if-found],[action-if-notfound])
dnl Search for ddjvuapi.  Defines HAVE_DDJVUAPI.
dnl Sets output variables DDJVUAPI_CFLAGS and DDJVUAPI_LIBS
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PATH_DDJVUAPI],
[
  AC_REQUIRE([AC_PROG_PKG_CONFIG])        
  AC_ARG_VAR(DDJVUAPI_LIBS, [Libraries for ddjvuapi])
  AC_ARG_VAR(DDJVUAPI_CFLAGS, [Compilation flags for ddjvuapi])
  AC_MSG_CHECKING([for ddjvuapi])
  if test -x "$PKG_CONFIG" ; then
    if $PKG_CONFIG ddjvuapi ; then
       ac_ddjvuapi=yes
       AC_MSG_RESULT([found])
       DDJVUAPI_LIBS=`$PKG_CONFIG --libs ddjvuapi`
       AC_MSG_RESULT([setting DDJVUAPI_LIBS=$DDJVUAPI_LIBS])
       DDJVUAPI_CFLAGS=`$PKG_CONFIG --cflags ddjvuapi`
       AC_MSG_RESULT([setting DDJVUAPI_CFLAGS=$DDJVUAPI_CFLAGS])
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
dnl @synopsis AC_PROGS_QT4
dnl Sets output variables QMAKE, MOC, UIC, RCC, LUPDATE, LRELEASE
dnl Prints an error message if QMAKE is not found or not suitable
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PROGS_QT4],
[
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])
  AC_ARG_VAR(QTDIR,[Location of the Qt package.])
  AC_ARG_VAR(QMAKE,[Location of the MOC program.])
  AC_ARG_VAR(MOC,[Location of the MOC program.])
  AC_ARG_VAR(UIC,[Location of the UIC program.])
  AC_ARG_VAR(RCC,[Location of the RCC program.])
  AC_ARG_VAR(LUPDATE,[Location of the LUPDATE program.])
  AC_ARG_VAR(LRELEASE,[Location of the LRELEASE program.])

  path=$PATH
  if test -n "$QTDIR" && test -d "$QTDIR/bin" ; then
    path=$QTDIR/bin:$PATH
  fi
  if test -z "$QMAKE" ; then
    AC_PATH_PROGS([QMAKE], [qmake-qt4 qmake], [], [$path])
  fi
  if test -z "$QMAKE" ; then
    AC_MSG_ERROR([Cannot find Qt4 program qmake. 
Please define variable QMAKE or QTDIR.])
  fi
  mkdir conftest.d
  cat > conftest.d/conftest.pro <<\EOF
changequote(<<, >>)dnl
message(QMAKE_UIC="$$QMAKE_UIC")
message(QMAKE_MOC="$$QMAKE_MOC")
message(QT_VERSION="$$[QT_VERSION]")
message(QT_INSTALL_PREFIX="$$[QT_INSTALL_PREFIX]")
message(QT_INSTALL_DATA="$$[QT_INSTALL_DATA]")
message(QT_INSTALL_BINS="$$[QT_INSTALL_BINS]")
message(QMAKE_RUN_CC="$$QMAKE_RUN_CC")
message(QMAKE_RUN_CXX="$$QMAKE_RUN_CXX")
changequote([, ])dnl
EOF
  if ( cd conftest.d && $QMAKE > conftest.out 2>&1 ) ; then
    sed -e 's/^.*: *//' < conftest.d/conftest.out > conftest.d/conftest.sh
    . conftest.d/conftest.sh
    rm -rf conftest.d
  else
    rm -rf conftest.d
    AC_MSG_ERROR([Cannot successfully run Qt4 program qmake. 
Please define variable QMAKE to a working qmake.])
  fi
  case "$QT_VERSION" in
    4.*)
      AC_MSG_RESULT([Program qmake reports Qt version $QT_VERSION.]) 
      ;;
    *)
      AC_MSG_ERROR([Qt version $QT_VERSION is insufficient.
Please define variable QMAKE to a suitable qmake.])
      ;;
  esac
  if test -z "$QTDIR" && 
     test -n "$QT_INSTALL_DATA" &&
     test -d "$QT_INSTALL_DATA/bin" ; then
    QTDIR="$QT_INSTALL_DATA"
    AC_MSG_RESULT([Defining QTDIR=$QTDIR])
  fi
  path=$PATH
  if test -d "$QT_INSTALL_BINS" ; then
    path=$QT_INSTALL_BINS:$path
  fi
  if test -n "$QTDIR" && test -d "$QTDIR/bin" ; then
    path=$QTDIR/bin:$PATH
  fi
  if test -z "$MOC" ; then
    if test -d "$QTDIR" && test -d "$QTDIR/bin" && test -x "$QTDIR/bin/moc" ; then
      MOC="$QTDIR/bin/moc"
      AC_MSG_RESULT([Defining MOC=$MOC])
    elif test -x "$QMAKE_MOC" ; then
      MOC=$QMAKE_MOC
      AC_MSG_RESULT([Defining MOC=$MOC])
    else
      AC_PATH_PROGS([UIC], [moc-qt4 moc], [], [$path])
    fi
  fi
  if test -z "$UIC" ; then
    if test -d "$QTDIR" && test -d "$QTDIR/bin" && test -x "$QTDIR/bin/uic" ; then
      UIC="$QTDIR/bin/uic"
      AC_MSG_RESULT([Defining UIC=$UIC])
    elif test -x "$QMAKE_UIC" ; then
      UIC=$QMAKE_UIC
      AC_MSG_RESULT([Defining UIC=$UIC])
    else
      AC_PATH_PROGS([UIC], [uic-qt4 uic], [], [$path])
    fi
  fi
  if test -z "$RCC" ; then
    AC_PATH_PROGS([RCC], [rcc], [], [$path])
  fi
  if test -z "$LUPDATE" ; then
    AC_PATH_PROGS([LUPDATE], [lupdate], [], [$path])
  fi
  if test -z "$LRELEASE" ; then
    AC_PATH_PROGS([LRELEASE], [lrelease], [], [$path])
  fi
])


