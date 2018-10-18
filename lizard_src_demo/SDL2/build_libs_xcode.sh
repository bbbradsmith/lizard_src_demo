#!/bin/bash
rm -r build
mkdir build
cd build
CC=$PWD/../SDL2-2.0.7/build-scripts/gcc-fat.sh CXX=$PWD/../SDL2-2.0.7/build-scripts/g++-fat.sh ../SDL2-2.0.7/configure --disable-shared --prefix=$(pwd)
make
make install
