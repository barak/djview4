#!/bin/bash 

# util
run() { 
    echo 2>&1 "+ $@"
    "$@"
    status=$?
    if test $status -ne 0 ; then 
	echo "[Exit with status $status]"
	exit $status
    fi
}

# test BREWDIR
BREWDIR=${BREWDIR-/usr/local}
if test -d "$BREWDIR/Cellar/djvulibre" ; then
    BREWDIR=$(cd "$BREWDIR" ; pwd)  # remove trailing slash
else
    echo 2>&1 "Please define BREWDIR to the Homebrew base."
    exit 10
fi

# test QTDIR
QTDIR=${QTDIR-/usr/local}
if test -x "$QTDIR/bin/qmake" ; then 
    QTDIR=$(cd "$QTDIR" ; pwd)  # remove trailing slash
else
    echo 2>&1 "Please define QTDIR to the Qt base dir."
    exit 10
fi

# change to this directory and clear DjView.app
run cd $(dirname $0) || exit
test -d DjView.app && run rm -rf DjView.app 

# copy qmake created bundle
run cp -r ../src/djview.app DjView.app || exit
bundle=DjView.app/Contents

# copy skeleton dirs
( cd skel ; find . -type d ) | while read d ; do
    run mkdir -p "$bundle/$d" || exit
done
( cd skel ; find . -type f ) | while read d ; do 
    run cp "skel/$d" "$bundle/$d" || exit
done

# copy djvulibre dir
brewlibdir="$BREWDIR/lib"
brewliblink=$(readlink "$brewlibdir/libdjvulibre.dylib")
djvulibrelibdir=$(cd "$brewlibdir" ; cd $(dirname "$brewliblink") ; pwd)
djvulibre=$(dirname "$djvulibrelibdir")

djvuname=$(dirname "$djvulibre")
djvuname=$(basename "$djvuname")
if test "$djvuname" != "djvulibre" ; then
    echo 2>&1 "Something wrong in the Homebrew links (exiting)"
    exit 10
fi

( cd $djvulibre ; find bin include lib share -type d ) | while read d ; do
    run mkdir -p "$bundle/$d" || exit
done
( cd $djvulibre ; find bin include lib share -type f ) | while read d ; do
    run cp "$djvulibre/$d" "$bundle/$d" || exit
done
( cd $djvulibre ; find bin include lib share -type l ) | while read d ; do
    target=$(readlink "$djvulibre/$d")
    run ln -s "$target"  "$bundle/$d" || exit
done

# misc copies
run cp ../src/djview.1 $bundle/share/man/man1
run cp $bundle/Resources/qt.conf $bundle/MacOS

# merge MacOS, bin, and plugins directories
run mv $bundle/bin/* $bundle/MacOS || exit
run rmdir $bundle/bin || exit
run ln -s ./MacOS $bundle/bin || exit
run ln -s ./MacOS $bundle/plugins || exit

# copy needed homebrew libraries
for lib in $(otool -L $bundle/MacOS/ddjvu | awk '/^\t/{print $1}') ; do
  case "$lib" in 
    $BREWDIR/*) 
        libname=$(basename "$lib")
        test -r "./$bundle/lib/$libname" || \
	    run cp "$lib" "./$bundle/lib/$libname" || exit ;;
  esac
done

# copy needed qt plugins
( cd "$QTDIR" ; \
  ls -1 plugins/{platforms,imageformats,styles,printsupport}/*.dylib | \
  grep -v libqwebgl.dylib | \
  grep -v _debug.dylib ) | \
while read plugin ; do
  run mkdir -p $bundle/$(dirname "$plugin") || exit
  run cp "$QTDIR/$plugin" $bundle/"$plugin" || exit
done



# copy needed libraries  
for loader in \
    $bundle/MacOS/djview \
    $bundle/MacOS/*/*.dylib 
do 
  for lib in $(otool -L $loader | awk '/^\t/{print $1}') 
  do
    if [ $(basename "$lib") != $(basename "$loader") ]
    then
      case "$lib" in 
      $BREWDIR/*) 
        libname=$(basename "$lib")
	test -r "./$bundle/lib/$libname" || \
	    run cp "$lib" "./$bundle/lib/$libname" || exit ;;
      @rpath/*)
        libname=$(basename "$lib")
        lib="$QTDIR/lib${lib/#@rpath//}"
	test -r "./$bundle/lib/$libname" || \
	    run cp "$lib" "./$bundle/lib/$libname" || exit ;;
      $QTDIR/*)
        libname=$(basename "$lib")
	test -r "./$bundle/lib/$libname" || \
	    run cp "$lib" "./$bundle/lib/$libname" || exit ;;
      esac
    fi
  done
done

# copy translations
languages=$(ls -1 ../src/*.qm | sed -e 's/^[^_]*_//' -e 's/\.qm$//')
run rm $bundle/Resources/empty.lproj
run mkdir -p $bundle/Resources/en.lproj || exit
for lang in $languages ; do
    run mkdir -p $bundle/Resources/$lang.lproj || exit
    run cp ../src/djview_$lang.qm $bundle/Resources/$lang.lproj/djview_$lang.qm || exit
    if test -r "$QTDIR/translations/qt_$lang.qm" ; then
      run cp "$QTDIR/translations/qt_$lang.qm" $bundle/Resources/$lang.lproj/ || exit
    fi
    if test -r "$QTDIR/translations/qtbase_$lang.qm" ; then
        run cp "$QTDIR"/translations/qtbase_$lang.qm $bundle/Resources/$lang.lproj/ || exit 
    fi
done

# copy mac tools
mdir=$bundle/Library/Spotlight/DjVu.mdimporter/Contents/MacOS
run mkdir -p $mdir || exit
run cp mdimporter_src/.libs/mdimporter.so $mdir/DjVu || exit
qdir=$bundle/Library/QuickLook/DjVu.qlgenerator/Contents/MacOS
run mkdir -p $qdir || exit
run cp qlgenerator_src/.libs/qlgenerator.so $qdir/DjVu || exit


# util to thread the libraries
fixlib() {
   for n ; do
     bn=`basename $n`
     for l in `otool -L $n | awk '/^\t/ {print $1}'` ; do
       bl=`basename $l`
       r=`dirname $n`
       d='..'
       while ! test -d "$r/$d/lib" ; do d="$d/.."; done;
       if test -r "$r/$d/lib/$bl" ; then
         if test "$bn" = "$bl" ; then
           run install_name_tool -id "@loader_path/$d/lib/$bn" "$n" || exit
         else
           run install_name_tool -change "$l" "@loader_path/$d/lib/$bl" "$n" || exit
         fi
       fi
     done
   done
}

# thread libraries
run chmod -R u+w DjView.app

fixlib $bundle/bin/*
fixlib $bundle/lib/*
fixlib $bundle/plugins/*/*
fixlib $mdir/DjVu
fixlib $qdir/DjVu


