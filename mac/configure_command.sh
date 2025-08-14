#/bin/sh
./configure --prefix=/Users/leonb/djvu/INST \
	--with-extra-includes=/Users/leonb/djvu/INST/include \
	--with-extra-libraries=/Users/leonb/djvu/INST/lib \
	CFLAGS='-arch arm64 -arch x86_64' \
	CXXFLAGS='-arch arm64 -arch x86_64'

