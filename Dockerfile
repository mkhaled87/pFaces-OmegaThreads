FROM ubuntu:24.04
LABEL maintainer="Mahmoud Khaled <eng.mk@msn.com>"

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV LANG=C.UTF-8

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    curl \
    git \
    unzip \
    tar \
    gzip \
    zlib1g-dev \
    opencl-headers \
    ocl-icd-opencl-dev \
    pocl-opencl-icd \
    clinfo \
    python3 \
    python3-numpy \
    python3-scipy \
    python3-matplotlib \
    && rm -rf /var/lib/apt/lists/*

# Install pFaces v1.5 release
WORKDIR /opt/pfaces
RUN curl -sL https://github.com/parallall/pFaces/releases/download/v1.5.0/pFaces-1.5-Ubuntu22.04.5LTS-x86_64.zip -o pfaces.zip \
    && unzip -q pfaces.zip \
    && rm pfaces.zip \
    && sh ./install.sh

ENV PFACES_ROOT=/opt/pfaces
ENV PFACES_SDK_ROOT=/opt/pfaces/pfaces-sdk
ENV PATH="/opt/pfaces/bin:${PATH}"

# Setup workspace
WORKDIR /opt/pFaces-OmegaThreads
COPY . /opt/pFaces-OmegaThreads

# Build kernel driver + Owl library and run tests
RUN sh ./build.sh && sh ./test.sh

CMD ["sh", "./test.sh"]
