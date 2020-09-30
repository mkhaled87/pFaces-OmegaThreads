# iamge
FROM ubuntu:18.04
MAINTAINER Mahmoud Khaled eng.mk@msn.com

# set time/language
ENV TZ=Europe/Kiev
ENV LANG C.UTF-8
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# update image
RUN apt-get -qq update \
	&& apt-get -qq install -y --no-install-recommends apt-utils \
	&& apt-get -qq -y upgrade

# install required libs/tools
RUN apt-get -qq install -y wget git unzip build-essential libcpprest-dev opencl-c-headers  opencl-clhpp-headers ocl-icd-opencl-dev clinfo oclgrind cmake

# install pfaces
RUN mkdir pfaces \
	&& cd pfaces \
	&& wget https://github.com/parallall/pFaces/releases/download/Release_1.1.0d/pFaces-1.1.0-Ubuntu18.04.zip \
	&& unzip pFaces-1.1.0-Ubuntu18.04.zip
RUN cd /pfaces && sh install.sh

## Install GraalVM (20.1 AMD64)
RUN wget -q https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-20.1.0/graalvm-ce-java11-linux-amd64-20.1.0.tar.gz \
    && echo '18f2dc19652c66ccd5bd54198e282db645ea894d67357bf5e4121a9392fc9394 graalvm-ce-java11-linux-amd64-20.1.0.tar.gz' | sha256sum --check \
    && tar -zxvf graalvm-ce-java11-linux-amd64-20.1.0.tar.gz \
    && rm graalvm-ce-java11-linux-amd64-20.1.0.tar.gz \
    && mv graalvm-ce-java11-20.1.0 /opt/
ENV PATH=/opt/graalvm-ce-java11-20.1.0/bin/:$PATH
ENV JAVA_HOME=/opt/graalvm-ce-java11-20.1.0/
RUN gu install native-image

# install OWL (release-20.06.00) and OmegaThreads (latest)
RUN git clone --depth=1 https://github.com/mkhaled87/pFaces-OmegaThreads \
	&& cd /pFaces-OmegaThreads/kernel-driver/lib/ltl2dpa \
	&& git clone --depth 1 --branch release-20.06.00 https://gitlab.lrz.de/i7/owl \
	&& cd owl \
	&& ./gradlew clean \
	&& ./gradlew distZip -x javadoc -Pdisable-pandoc	
RUN cp /pFaces-OmegaThreads/kernel-driver/lib/ltl2dpa/owl/build/native-library/libowl.so /usr/lib/
RUN cd /pFaces-OmegaThreads/ \
	&& export PFACES_SDK_ROOT=$PWD/../pfaces/pfaces-sdk/ \
	&& make
