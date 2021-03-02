#!/bin/sh

# CMake cofiguration
BUILDTYPE=Release
KERNEL_NAME=omega

# remove old build binaries
rm -rf kernel-pack/$KERNEL_NAME
rm -rf build

# building ...
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILDTYPE
cmake --build . --config $BUILDTYPE
cd ..
rm -rf build