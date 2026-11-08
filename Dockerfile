FROM ubuntu:24.04
LABEL MAINTAINER="Mahmoud Khaled, eng.mk@msn.com"

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV LANG=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    curl \
    git \
    unzip \
    python3 \
    opencl-headers \
    ocl-icd-opencl-dev \
    pocl-opencl-icd \
    clinfo \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Build pFaces core and SDK from source
WORKDIR /opt
RUN git clone --depth 1 https://github.com/parallall/pFaces.git pFaces \
    && cmake -S /opt/pFaces -B /opt/pFaces/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /opt/pFaces/build --parallel

ENV PFACES_ROOT=/opt/pFaces
ENV PFACES_SDK_ROOT=/opt/pFaces/pfaces-sdk
ENV PATH="/opt/pFaces/bin:${PATH}"

# Build OmegaThreads kernel driver + bundled Owl native library
WORKDIR /opt/pFaces-OmegaThreads
COPY . /opt/pFaces-OmegaThreads
RUN sh build.sh
