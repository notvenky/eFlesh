FROM ubuntu:24.04

# docker for generating the thimble sensor

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# --------------------------------------------------------
# system dependencies
# --------------------------------------------------------
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    python3 \
    python3-pip \
    python3-venv \
    libgmp-dev \
    libmpfr-dev \
    libcgal-dev \
    libeigen3-dev \
    libsuitesparse-dev \
    libboost-all-dev \
    blender \
    python-is-python3 \
    vim \
    && rm -rf /var/lib/apt/lists/*

# --------------------------------------------------------
# python deps
# --------------------------------------------------------
RUN pip3 install --break-system-packages \
    numpy \
    scipy \
    matplotlib \
    meshio \
    tqdm \
    pyyaml


WORKDIR /workspace

CMD ["/bin/bash"]
