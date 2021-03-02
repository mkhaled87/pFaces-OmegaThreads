#!/bin/sh

# CMake cofiguration
BUILDTYPE=Release
KERNEL_NAME=omega

# remove old build binaries
rm -rf kernel-pack/$KERNEL_NAME
rm -rf build

# rebuild owl if needed
ifneq ($(wildcard ./kernel-driver/lib/ltl2dpa/owl/.*),)
	$(info OWL folder is found and we assume it is installed.)
else
    $(info OWL folder not found and we install it ...)
    cd ./kernel-driver/lib/ltl2dpa/
    sh install-owl.sh
endif

# building ...
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILDTYPE
cmake --build . --config $BUILDTYPE
cd ..
rm -rf build