#!/bin/sh

# install GraalVM
if [ "$OSTYPE" == "darwin"* ]; then
    echo "Installing GraalVM-20.1.0 for MacOS ..."
    GRAALVM_FILE=graalvm-ce-java11-darwin-amd64-20.1.0.tar.gz
    PROFILE_FILE=~/.zshrc 
    INSTALL_PATH=/Library/Java/JavaVirtualMachines
    BIN_PATH=Contents/Home/bin
    HOME_PATH=Contents/Home
else
    echo "Installing GraalVM-20.1.0 for Ubuntu Linux ..."
    GRAALVM_FILE=graalvm-ce-java11-linux-amd64-20.1.0.tar.gz
    PROFILE_FILE=~/.bashrc 
    INSTALL_PATH=/opt
    BIN_PATH=bin
    HOME_PATH=
fi
GRAALVM_URL=https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-20.1.0/$GRAALVM_FILE
wget -q $GRAALVM_URL
tar -zxvf $GRAALVM_FILE
rm $GRAALVM_FILE
sudo mv graalvm-ce-java11-20.1.0 $INSTALL_PATH/
rm -rf graalvm-ce-java11-20.1.0
echo "#GraalVM installed by (install-owl.sh)" >> $PROFILE_FILE
echo "PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:\$PATH" >> $PROFILE_FILE
echo "JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH" >> $PROFILE_FILE
java --version

# install GraalVM Native-Image
echo "Installing GraalVM-20.1.0 Native-Image ..."
sudo $INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH/gu install native-image

# remove old owl
rm -rf ./owl

# Install owl/latest
echo "Installing OWL/latest ..."
git clone https://gitlab.lrz.de/i7/owl 
cd owl
PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:\$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH ./gradlew clean
PATH=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$BIN_PATH:\$PATH JAVA_HOME=$INSTALL_PATH/graalvm-ce-java11-20.1.0/$HOME_PATH ./gradlew distZip -x javadoc -Pdisable-pandoc
