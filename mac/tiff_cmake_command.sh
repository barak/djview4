#!/bin/sh
prefix=${1-/Users/leonb/djvu/INST}
cmake -G Ninja \
	-DCMAKE_INSTALL_PREFIX="${prefix}" \
	-DCMAKE_PREFIX_PATH="${prefix}" \
	-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
	-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
	-DCMAKE_IGNORE_PATH="${HOME}/homebrew/bin;${HOME}/homebrew/include;${HOME}/homebrew/lib" \
	..
