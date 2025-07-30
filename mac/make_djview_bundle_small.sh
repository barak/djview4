#!/bin/bash 

echo "This version makes a bundle that does not contain libraries."
echo "It must find them at runtime in the expected places."


run() { 
    echo 2>&1 "+ $@"
    "$@"
    status=$?
    if test $status -ne 0 ; then 
	echo "[Exit with status $status]"
	exit $status
    fi
}

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

# copy djview translations only
languages=$(ls -1 ../src/*.qm | sed -e 's/^[^_]*_//' -e 's/\.qm$//')
test -d $bundle/Resources/empty.lproj && run rm $bundle/Resources/empty.lproj
run mkdir -p $bundle/Resources/en.lproj || exit
for lang in $languages ; do
    run mkdir -p $bundle/Resources/$lang.lproj || exit
    run cp ../src/djview_$lang.qm $bundle/Resources/$lang.lproj/djview_$lang.qm || exit
done

# copy mac tools
run mkdir -p "$bundle/Library"

run cp -r skel/Library/Spotlight "$bundle/Library" 
mdir=$bundle/Library/Spotlight/DjVu.mdimporter/Contents/MacOS
run mkdir -p $mdir || exit
run cp mdimporter_src/.libs/mdimporter.so $mdir/DjVu || exit
# fixlib $mdir/DjVu

run cp -r skel/Library/QuickLook "$bundle/Library" 
qdir=$bundle/Library/QuickLook/DjVu.qlgenerator/Contents/MacOS
run mkdir -p $qdir || exit
run cp qlgenerator_src/.libs/qlgenerator.so $qdir/DjVu || exit
# fixlib $qdir/DjVu



