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
  IFS=. read major minor revis <<EOF
$2
EOF
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
  AC_ARG_VAR(TIFF_LIBS)
  AC_ARG_VAR(TIFF_CFLAGS)
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
dnl @synopsis AC_PATH_QT4([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl Define HAVE_QT and set it to QT_VERSION
dnl Set output variables QMAKE, MOC, UIC,
dnl ------------------------------------------------------------------

AC_DEFUN([AC_PATH_QT4],
[
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])
  AC_ARG_VAR(QTDIR,[Location of the Qt package.])
  AC_ARG_VAR(QMAKE,[Location of the MOC program.])
  AC_ARG_VAR(MOC,[Location of the MOC program.])
  AC_ARG_VAR(UIC,[Location of the UIC program.])
])


