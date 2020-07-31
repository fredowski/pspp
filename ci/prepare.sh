#!/bin/sh -xve

# Prepare the build environment

# Copyright (C) 2020 Free Software Foundation, Inc.
# Released under GNU General Public License, either version 3
# or any later option

cd ..
#Download gnulib
git clone https://git.savannah.gnu.org/git/gnulib.git
pushd gnulib
git checkout 1e972a8a37c153ddc15e604592f84f939eb3c2ad
popd

