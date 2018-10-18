#!/bin/bash
rm -r build
mkdir build
cd build
../SDL2-2.0.7/configure --disable-shared --disable-sndio --disable-haptic --prefix=$(pwd)
make
make install
