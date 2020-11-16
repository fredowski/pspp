#!/bin/sh -xve

# Build ssw

# Copyright (C) 2020 Free Software Foundation, Inc.
# Released under GNU General Public License, either version 3
# or any later option

bundleinstall=/usr
cd ..

#Download and install spread-sheet-widget
sswversion=0.7
curl -o ssw.tgz http://alpha.gnu.org/gnu/ssw/spread-sheet-widget-$sswversion.tar.gz
tar -xzf ssw.tgz
cd spread-sheet-widget-$sswversion
./configure --prefix=$bundleinstall
sudo make install
cd ..
