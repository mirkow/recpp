FROM ubuntu:20.04

RUN apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get -y -qq install \
    apt-utils \
    apt-file \
    bison \
    bzip2 \
    curl \
    flex \
    git \
    gnupg \
    libgmp3-dev \
    libmpc-dev \
    libmpfr-dev \
    nano \
    openssh-server \
    rsync \
    texinfo \
    vim \
    xz-utils \
    # Essential packages for remote debugging and login in
    # more info under https://github.com/shuhaoliu/docker-clion-dev/blob/master/Dockerfile
    gdb \
    gdbserver \
    # PC-Lint Dependencies
    python \
    python3-pip \
    python3-regex \
    python3-yaml \
    # Build Tools
    build-essential \
    cmake \
    cmake-curses-gui \
    # Doxygen
    doxygen \
    dia \
    graphviz \
    mscgen \
    zlib1g-dev \
    wget \    
    libtinfo-dev \
    llvm-10 \
    clang-10 \
    clang-format-10 \
    clang-tidy-10 \
    libclang-10-dev \
    unzip \
    htop \
    && apt-file update && apt-get clean

RUN mkdir /downloads


RUN pip3 install cmake-format

RUN cd /downloads/ && wget https://github.com/sigurd-dev/mkblob/raw/master/binary_X86_64/mkblob_1.04-1_amd64.deb
RUN dpkg -i /downloads/mkblob_1.04-1_amd64.deb 

RUN cd /downloads && wget https://github.com/clangd/clangd/releases/download/11.0.0/clangd-linux-11.0.0.zip 
RUN unzip /downloads/clangd-linux-11.0.0.zip -d /downloads && mv /downloads/clangd_11.0.0/bin/clangd /usr/bin/clangd-11
RUN ln -s /usr/bin/clangd-11 /usr/bin/clangd

# RUN cd /downloads && wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.0/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz \
#     && tar -xf /downloads/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz -C /usr --strip 1 \
#     && rm /downloads/clang+llvm-11.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz