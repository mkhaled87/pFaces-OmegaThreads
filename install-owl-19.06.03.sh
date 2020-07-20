#!/bin/sh

# install java
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Installing JAVA-11 for MacOS (Homebrew) ..."
    brew update
    brew tap homebrew/cask-versions
    brew cask install java11
else
    echo "Installing JAVA-11 for Ubuntu Linux (apt) ..."
    sudo apt-get install -y default-jre
    sudo apt-get install -y default-jdk
fi
echo "Installing OWL-19.06.03 to  ./kernel/lib/ ..."

# delete any old installation
rm -rf kernel/lib/owl/
rm -rf kernel/lib/owl-19.06.03/

# clone OWL
git clone https://gitlab.lrz.de/i7/owl.git ./kernel/lib/owl/
cd kernel/lib/owl/

# checkout the version released with tag "release-19.06.03"
git checkout tags/release-19.06.03

# build OWL
./gradlew minimizedDistZip

# build the C++ interface (requires a fix to allow for C++ 11)
cd src/main/cpp/library/
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD:INT=17 ..
make

# unzip the distribution
cd ../../../../../build/distributions
unzip owl-minimized-19.06.03.zip

# wrapping the library
cd ../../../
mkdir owl-19.06.03/
mv ./owl/src/main/headers ./owl-19.06.03
mv ./owl-19.06.03/headers ./owl-19.06.03/include
mkdir owl-19.06.03/lib
mv ./owl/src/main/cpp/library/build/libowl.a ./owl-19.06.03/lib/
mkdir owl-19.06.03/java
mv ./owl/build/distributions/owl-minimized-19.06.03/lib/owl-19.06.03-all.jar ./owl-19.06.03/java/owl.jar
rm -rf owl