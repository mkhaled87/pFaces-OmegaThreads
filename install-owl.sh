#!/bin/sh

# delete any old installation
rm -rf kernel/lib/owl/

# clone OWL
git clone https://gitlab.lrz.de/i7/owl.git ./kernel/lib/owl/
cd kernel/lib/owl/

# checkout the version released with tag "release-19.06.03"
git checkout tags/release-19.06.03

# build OWL
./gradlew distZip

# build the C++ interface (requires a fix to allow for C++ 11)
cd src/main/cpp/library/
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD:INT=17 ..
make

