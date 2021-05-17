#!/bin/sh

# CMake cofiguration
BUILDTYPE=Release
KERNEL_NAME=omega

# remove old build binaries
rm -rf kernel-pack/$KERNEL_NAME.driver
rm -rf build

# install owl if needed
if [ -d "./kernel-driver/lib/ltl2dpa/owl/build" ] 
then
    echo "OWL folder is found and we assume it is installed." 
else
    echo "OWL folder not found and we install it ..."
    cd ./kernel-driver/lib/ltl2dpa/
    sh install-owl.sh
    cd ../../..
fi

# building ...
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILDTYPE
cmake --build . --config $BUILDTYPE
cd ..