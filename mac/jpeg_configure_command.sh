#/bin/sh
prefix=${1-/Users/leonb/djvu/INST}
./configure  --prefix=${prefix} \
	'CFLAGS=-arch arm64 -arch x86_64 -mmacos-version-min=10.3'
