#!/bin/sh

# check
if ! command -v cmake &> /dev/null
then
    echo "CMAKE is not installed. Please install it first."
    exit
fi

# Configurations
BUILDTYPE=Release
KERNEL_NAME=omega
CLEAN_BUILD=false

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

# remove old build binaries
if [ $CLEAN_BUILD = true ]
then 
    rm -rf kernel-pack/$KERNEL_NAME.driver
    rm -rf build
fi

# building ...
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILDTYPE
cmake --build . --config $BUILDTYPE
cd ..