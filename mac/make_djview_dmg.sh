#!/bin/bash

cd $(dirname $0)
if ! test -d DjView.app ; then
  echo 2>&1 "Run 'make_djview_bundle.sh' first".
  exit 10
fi

run() {
    echo 2>&1 "+ $@"
    "$@"
    status=$?
    if test $status -ne 0 ; then
        echo "[Exit with status $status]"
        exit $status
    fi
}

dmg=dmg$$
test -d $dmg && run rm -rf $dmg 

trap "rm -rf $dmg 2>/dev/null" 0

run mkdir $dmg || exit
run cp -r DjView.app $dmg || exit
run cp ReadMe.rtf $dmg || exit
book=DjView.app/Contents/share/doc/djvu/djvulibre-book-en.djvu
run ln -s $book $dmg/Manual.djvu || exit
run hdiutil create -ov -srcfolder $dmg DjView.dmg || exit
run rm -rf $dmg
