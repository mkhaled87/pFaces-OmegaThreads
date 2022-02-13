# Note: 
#	This needs to run against a docker container with at least 6.0 GB of memory.

# iamge
FROM ubuntu:20.04
LABEL MAINTAINER="Mahmoud Khaled, eng.mk@msn.com"
ENV LANG C.UTF-8
ARG DEBIAN_FRONTEND=noninteractive

# update image
RUN apt-get -qq update \
	&& apt-get -qq install -y --no-install-recommends apt-utils \
	&& apt-get -qq -y upgrade

# install required libs/tools
RUN apt-get -qq install -y cmake wget git unzip cmake build-essential libcpprest-dev opencl-c-headers opencl-clhpp-headers ocl-icd-opencl-dev clinfo oclgrind cmake

# install pFaces 1.2.1 
RUN mkdir pfaces \
	&& cd pfaces \
	&& wget https://github.com/parallall/pFaces/releases/download/Release_1.2.1d/pFaces-1.2.1-Ubuntu20.04.zip \
	&& unzip pFaces-1.2.1-Ubuntu20.04.zip
RUN cd /pfaces && sh install.sh

## Install GraalVM (20.0.0.2 AMD64) + native-image
RUN wget -q https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-21.0.0.2/graalvm-ce-java11-linux-amd64-21.0.0.2.tar.gz \
    && tar -zxvf graalvm-ce-java11-linux-amd64-21.0.0.2.tar.gz \
    && rm graalvm-ce-java11-linux-amd64-21.0.0.2.tar.gz \
    && mv graalvm-ce-java11-21.0.0.2 /opt/
ENV PATH=/opt/graalvm-ce-java11-21.0.0.2/bin/:$PATH
ENV JAVA_HOME=/opt/graalvm-ce-java11-21.0.0.2/

RUN wget -q https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-21.0.0.2/native-image-installable-svm-java11-linux-amd64-21.0.0.2.jar \
    && gu -L install native-image-installable-svm-java11-linux-amd64-21.0.0.2.jar \
    && rm native-image-installable-svm-java11-linux-amd64-21.0.0.2.jar \
    && java -version \
    && native-image --version


# install OWL (release-21.0) and OmegaThreads (latest)
RUN git clone --depth=1 https://github.com/mkhaled87/pFaces-OmegaThreads \
	&& cd /pFaces-OmegaThreads/kernel-driver/lib/ltl2dpa \
	&& git clone --depth 1 --branch release-21.0 https://gitlab.lrz.de/i7/owl \
	&& cd owl \
	&& ./gradlew nativeImageDistZip -Pdisable-pandoc \
	&& cp ./build/native-library/libowl.so /usr/lib/
RUN cd /pFaces-OmegaThreads/ \
	&& export PFACES_SDK_ROOT=$PWD/../pfaces/pfaces-sdk/ \
	&& sh build.sh
