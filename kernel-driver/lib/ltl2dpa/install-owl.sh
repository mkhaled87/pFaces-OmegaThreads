#!/bin/sh

# checks
if ! command -v wget &> /dev/null
then
    echo "WGET is not installed. Please install it first."
    exit
fi
if ! command -v unzip &> /dev/null
then
    echo "UNZIP is not installed. Please install it first."
    exit
fi

# install GraalVM
if [[ "$OSTYPE" == *"darwin"* ]]; then
    echo "Installing GraalVM-21.0.0.2 for MacOS ..."
    GRAALVM_FILE=graalvm-ce-java11-darwin-amd64-21.0.0.2.tar.gz
    PROFILE_FILE=~/.zshrc 
    INSTALL_PATH=/Library/Java/JavaVirtualMachines
    BIN_PATH=Contents/Home/bin
    HOME_PATH=Contents/Home
else
    echo "Installing GraalVM-21.0.0.2 for Linux AMD64..."
    GRAALVM_FILE=graalvm-ce-java11-linux-amd64-21.0.0.2.tar.gz
    PROFILE_FILE=~/.bashrc 
    INSTALL_PATH=/opt
    BIN_PATH=bin
    HOME_PATH=
fi

echo "Downloading GraalVM ... [please be patient as it takes some time]"
GRAALVM_URL=https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-21.0.0.2/$GRAALVM_FILE
wget -q $GRAALVM_URL
echo "Unpacking/Installing GraalVM ... [Please enter user password if requested]"
tar -zxvf $GRAALVM_FILE 2>/dev/null
rm $GRAALVM_FILE
sudo mv graalvm-ce-java11-21.0.0.2 $INSTALL_PATH/ 2>/dev/null
rm -rf graalvm-ce-java11-21.0.0.2
echo "#GraalVM installed by (install-owl.sh)" >> $PROFILE_FILE
echo "export PATH=$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$BIN_PATH:\$PATH" >> $PROFILE_FILE
echo "export JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$HOME_PATH" >> $PROFILE_FILE

echo "GraalVM installed, checking JAVA ... "
export PATH="$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$BIN_PATH:$PATH"
export JAVA_HOME="$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$HOME_PATH"
echo "PATH: "
echo $PATH
echo "JAVA_HOME: "
echo $JAVA_HOME
JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$HOME_PATH java --version

# install GraalVM Native-Image
echo "Installing GraalVM-21.0.0.2 Native-Image ..."
sudo $INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$BIN_PATH/gu install native-image

# Install owl/release-21.0
OWL_VER=release-21.0
echo "Installing OWL/$OWL_VER ..."
rm -rf ./owl
git clone --depth=1 https://gitlab.lrz.de/i7/owl --branch $OWL_VER
cd owl
PATH=$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$BIN_PATH:$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-21.0.0.2/$HOME_PATH ./gradlew nativeImageDistZip -Pdisable-pandoc
