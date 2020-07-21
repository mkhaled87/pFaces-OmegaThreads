#!/bin/sh
TARGET_RELEASE=19.06.03

# install java
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Installing JAVA-11 for MacOS (Homebrew) ..."
    brew update
    brew tap homebrew/cask-versions
    brew cask install java11
    echo "export JAVA_HOME=$(/usr/libexec/java_home -v11)" >> ~/.zshrc
    source ~/.zshrc
else
    echo "Installing default JAVA for Ubuntu Linux (apt) ..."
    sudo apt-get install -y default-jre
    sudo apt-get install -y default-jdk
fi
echo "Installing OWL-$TARGET_RELEASE to  ./owl- ..."

# delete any old installation
rm -rf ./owl-$TARGET_RELEASE/
rm -rf ./owl/

# clone OWL
git clone https://gitlab.lrz.de/i7/owl.git ./owl-$TARGET_RELEASE/
cd ./owl-$TARGET_RELEASE/

# checkout the version released with tag "release-$TARGET_RELEASE"
git checkout tags/release-$TARGET_RELEASE
????? remotes/origin/cinterface-state-decomposition ????

# build OWL
./gradlew minimizedDistZip

# build the C++ interface (requires a fix to allow for C++ 11)
cd src/main/cpp/library/
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD:INT=11 ..
make

# unzip the distribution
cd ../../../../../build/distributions
unzip owl-minimized-$TARGET_RELEASE.zip

# wrapping the library
cd ../../../
mkdir ./owl/
mv ./owl-$TARGET_RELEASE/src/main/headers ./owl
mv ./owl/headers ./owl/include
mkdir owl/lib
mv ./owl-$TARGET_RELEASE/src/main/cpp/library/build/libowl.a ./owl/lib/
mkdir owl/java
mv ./owl-$TARGET_RELEASE/build/distributions/owl-minimized-$TARGET_RELEASE/lib/owl-$TARGET_RELEASE-all.jar ./owl/java/owl.jar
rm -rf ./owl-$TARGET_RELEASE