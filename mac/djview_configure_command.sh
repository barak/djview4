#/bin/sh
prefix=${1-/Users/leonb/djvu/INST}
./autogen.sh --prefix=${prefix} \
	--with-extra-includes=${prefix}/include \
	--with-extra-libraries=${prefix}/lib \
	--disable-nsdejavu \
	--enable-mac \
	CFLAGS='-arch arm64 -arch x86_64' \
	OBJCFLAGS='-arch arm64 -arch x86_64' \
	CXXFLAGS='-arch arm64 -arch x86_64' \
	QMAKE=${prefix}/bin/qmake \
	PKG_CONFIG_PATH=${prefix}/lib/pkgconfig

