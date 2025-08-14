#/bin/sh
prefix=${1-/Users/leonb/djvu/INST}
./autogen.sh --prefix=${prefix} \
	--with-extra-includes=${prefix}/include \
	--with-extra-libraries=${prefix}/lib \
	CFLAGS='-arch arm64 -arch x86_64 -mmacosx-version-min=10.9' \
	CXXFLAGS='-arch arm64 -arch x86_64 -mmacosx-version-min=10.9' 

