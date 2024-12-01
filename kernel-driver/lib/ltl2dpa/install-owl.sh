#!/bin/sh

# install GraalVM
if [[ "$OSTYPE" == *"darwin"* ]]; then
    echo "Installing GraalVM-20.1.0 for MacOS ..."
    GRAALVM_FILE=graalvm-ce-java11-darwin-amd64-20.1.0.tar.gz
    BIN_PATH=Contents/Home/bin
    HOME_PATH=Contents/Home
else
    echo "Installing GraalVM-20.1.0 for Linux AMD64..."
    GRAALVM_FILE=graalvm-ce-java11-linux-amd64-20.1.0.tar.gz
    BIN_PATH=bin
    HOME_PATH=
fi
INSTALL_PATH=$(pwd)/GraalVM
GVM_SETUP_FILE=./gvm_name.txt
mkdir -p $INSTALL_PATH

echo "Downloading GraalVM ... [please be patient as it takes some time]"
GRAALVM_URL=https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-20.1.0/$GRAALVM_FILE
wget -q $GRAALVM_URL
echo "Unpacking/Installing GraalVM ... [Please enter user password if requested]"
tar -zxvf $GRAALVM_FILE 2>/dev/null
rm $GRAALVM_FILE
mv graalvm-ce-java11-20.1.0 $INSTALL_PATH/ 2>/dev/null
rm -rf graalvm-ce-java11-20.1.0
echo "graalvm-ce-java11-20.1.0/$HOME_PATH" > $GVM_SETUP_FILE

echo "GraalVM installed, checking JAVA ... "
export PATH="$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:$PATH"
export JAVA_HOME="$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH"
echo "PATH: "
echo $PATH
echo "JAVA_HOME: "
echo $JAVA_HOME
JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH java --version

# install GraalVM Native-Image
echo "Installing GraalVM-20.1.0 Native-Image ..."
$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH/gu install native-image

# Install owl/release-20.06.00
#OWL_VER=release-20.06.00
echo "Installing OWL/$OWL_VER ..."
rm -rf ./owl
#git clone --depth=1 https://gitlab.lrz.de/i7/owl --branch $OWL_VER
mkdir owl
cd owl
unzip ../owl-release-20.06.00.zip
PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH ./gradlew clean
PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH ./gradlew distZip -x javadoc -Pdisable-pandoc
