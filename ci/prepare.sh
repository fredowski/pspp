#!/bin/sh -xve

# Prepare the build environment

# Copyright (C) 2020 Free Software Foundation, Inc.
# Released under GNU General Public License, either version 3
# or any later option

# Assume that we are in the pspp git source directory
if ! test -f ./Smake; then
    echo "I am not in the pspp git source directory - exiting"
    exit 1
fi

cd ..
#Download gnulib
gnulibver=0edaafc813caff4101c58405c6ab279597afc0b9
curl -o gnulib.zip https://codeload.github.com/coreutils/gnulib/zip/$gnulibver
unzip -q gnulib.zip
rm gnulib.zip
mv gnulib-$gnulibver gnulib

