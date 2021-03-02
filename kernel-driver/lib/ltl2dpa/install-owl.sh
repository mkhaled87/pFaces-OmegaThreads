#!/bin/sh

# install GraalVM
if [[ "$OSTYPE" == *"darwin"* ]]; then
    echo "Installing GraalVM-20.1.0 for MacOS ..."
    GRAALVM_FILE=graalvm-ce-java11-darwin-amd64-20.1.0.tar.gz
    PROFILE_FILE=~/.zshrc 
    INSTALL_PATH=/Library/Java/JavaVirtualMachines
    BIN_PATH=Contents/Home/bin
    HOME_PATH=Contents/Home
else
    echo "Installing GraalVM-20.1.0 for Linux AMD64..."
    GRAALVM_FILE=graalvm-ce-java11-linux-amd64-20.1.0.tar.gz
    PROFILE_FILE=~/.bashrc 
    INSTALL_PATH=/opt
    BIN_PATH=bin
    HOME_PATH=
fi

echo "Downloading GraalVM ... [please be patient as it takes some time]"
GRAALVM_URL=https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-20.1.0/$GRAALVM_FILE
wget -q $GRAALVM_URL
echo "Unpacking/Installing GraalVM ... [Please enter user password if requested]"
tar -zxvf $GRAALVM_FILE 2>/dev/null
rm $GRAALVM_FILE
sudo mv graalvm-ce-java11-20.1.0 $INSTALL_PATH/ 2>/dev/null
rm -rf graalvm-ce-java11-20.1.0
echo "#GraalVM installed by (install-owl.sh)" >> $PROFILE_FILE
echo "export PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:\$PATH" >> $PROFILE_FILE
echo "export JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH" >> $PROFILE_FILE

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
sudo $INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH/gu install native-image

# Install owl/release-20.06.00
OWL_VER=release-20.06.00
echo "Installing OWL/$OWL_VER ..."
rm -rf ./owl
git clone --depth=1 https://gitlab.lrz.de/i7/owl --branch $OWL_VER
cd owl
PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH ./gradlew clean
PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH ./gradlew distZip -x javadoc -Pdisable-pandoc
