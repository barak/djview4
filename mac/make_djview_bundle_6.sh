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

# prefix for dependent libs
PREFIX=${1-/Users/leonb/djvu/INST}
if test -d "$PREFIX" && test -f "$PREFIX/lib/libdjvulibre.dylib" ; then
    echo 2>&1 "# Using prefix $PREFIX"
else
    echo 2>&1 "Please give a correct install prefix as argument"
    exit 10
fi

# find bundle
if ! test -d ../src/djview.app ; then
    echo 2>&1 "Cannot find ../src/djview.app"
    echo 2>&1 "Are you running this from the 'mac' subdirectory?"
    exit 10
fi

# copy qmake created bundle
test -d DjView.app && run rm -rf DjView.app
run cp -r ../src/djview.app DjView.app || exit
bundle=DjView.app/Contents

# run macdeployqt
run "$PREFIX"/bin/macdeployqt DjView.app -verbose=1

# fix qt.conf
echo "Translations = Resources/translations" >> "$bundle/Resources/qt.conf"

# create subdirs
( cd "$bundle" ;
  run ln -s MacOS bin ;
  run ln -s Frameworks lib ;
  run mkdir share )

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

# copy djvulibre scripts
scripts="djvudigital any2djvu"
for n in $scripts ; do
    run cp "$PREFIX/bin/$n" "$bundle/bin/$n"
done

# copy djvulibre executables
exes="bzz c44 cjb2 cpaldjvu csepdjvu ddjvu djvm djvmcvt \
    djvudump djvuextract djvumake djvups djvused djvuserve djvutxt \
    djvutoxml djvuxmlparser"
for n in $exes ; do
    run cp "$PREFIX/bin/$n" "$bundle/bin/$n"
    fixlib "$bundle/bin/$n"
done

# copy djvulibre support files
run cp -r "$PREFIX/share/djvu" "$bundle/share/."

# copy djvulibre manpages
run mkdir -p "$bundle/share/man/man1"
for n in $scripts $exes djview djvu ; do
    if test -f "$PREFIX/share/man/man1/$n.1" ; then
	run cp "$PREFIX/share/man/man1/$n.1" "$bundle/share/man/man1/."
    elif test -f "../src/$n.1" ; then
	run cp "../src/$n.1" "$bundle/share/man/man1/."
    fi
done

# copy misc djvulibre docs and scripts
run cp skel/setpath.sh "$bundle/."
if test -d "$PREFIX/../djvulibre-3.5" ; then
    run mkdir -p "$bundle/share/doc/djvu"
    for n in "$PREFIX/../djvulibre-3.5/doc"/*; do
	test -f "$n" && run cp "$n" "$bundle/share/doc/djvu/."
    done
fi

# copy translations
languages=$(ls -1 ../src/*.qm | sed -e 's/^[^_]*_//' -e 's/\.qm$//')
test -d $bundle/Resources/empty.lproj && run rm $bundle/Resources/empty.lproj
run mkdir -p $bundle/Resources/en.lproj || exit
for lang in $languages ; do
    run mkdir -p $bundle/Resources/$lang.lproj || exit
    run cp ../src/djview_$lang.qm $bundle/Resources/$lang.lproj/djview_$lang.qm || exit
    if test -r "$PREFIX/translations/qt_$lang.qm" ; then
      run cp "$PREFIX/translations/qt_$lang.qm" $bundle/Resources/$lang.lproj/ || exit
    fi
    if test -r "$PREFIX/translations/qtbase_$lang.qm" ; then
        run cp "$PREFIX"/translations/qtbase_$lang.qm $bundle/Resources/$lang.lproj/ || exit 
    fi
done

# copy mac tools
run mkdir -p "$bundle/Library"

run cp -r skel/Library/Spotlight "$bundle/Library" 
mdir=$bundle/Library/Spotlight/DjVu.mdimporter/Contents/MacOS
run mkdir -p $mdir || exit
run cp mdimporter_src/.libs/mdimporter.so $mdir/DjVu || exit
fixlib $mdir/DjVu

run cp -r skel/Library/QuickLook "$bundle/Library" 
qdir=$bundle/Library/QuickLook/DjVu.qlgenerator/Contents/MacOS
run mkdir -p $qdir || exit
run cp qlgenerator_src/.libs/qlgenerator.so $qdir/DjVu || exit
fixlib $qdir/DjVu



