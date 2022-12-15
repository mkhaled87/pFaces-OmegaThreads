#!/bin/sh

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

# Prepare JAVA paths
JVM_VER=$(cat ./kernel-driver/lib/ltl2dpa/gvm_name.txt)
export JAVA_HOME=$(pwd)/kernel-driver/lib/ltl2dpa/GraalVM/$JVM_VER
echo "JH: $JAVA_HOME"

# building ...
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILDTYPE
cmake --build . --config $BUILDTYPE
cd ..