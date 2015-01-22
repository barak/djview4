#! /bin/sh

awk -f config/qmsources.awk < src/djview.pro > src/djview.am
awk -f config/qmsources.awk < npdjvu/npdjvu.pro > npdjvu/npdjvu.am

autoreconf -f -i -v --warnings=all || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
