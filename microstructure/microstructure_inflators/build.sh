#!/bin/bash
# Simple build script for microstructure_inflators
# This script builds the essential tools needed for eFlesh
set -e  # Exit on any error

JOBS=12
for arg in "$@"; do
  case $arg in
    cpu_nodes=*) JOBS="${arg#*=}";;
  esac
done
[[ "$JOBS" =~ ^[0-9]+$ ]] || JOBS=8
[ "$JOBS" -lt 1 ] && JOBS=1
J="-j$JOBS"

echo "Using $JOBS CPU nodes for building."
echo "Building microstructure_inflators..."
install_boost() {
    local BOOST_VERSION="1.83.0"
    local BOOST_DIR="$(xdg-user-dir DOWNLOAD)/boost_${BOOST_VERSION//./_}"
    local BOOST_INSTALL_DIR="$HOME/.local/boost_${BOOST_VERSION//./_}"
    
    echo "Setting up Boost $BOOST_VERSION..."
    
    if [ ! -d "$BOOST_DIR" ]; then
        echo "Downloading Boost $BOOST_VERSION..."
        cd "$(xdg-user-dir DOWNLOAD)"
        wget "https://sourceforge.net/projects/boost/files/boost/$BOOST_VERSION/boost_${BOOST_VERSION//./_}.tar.gz/download" -O "boost_${BOOST_VERSION//./_}.tar.gz"
        tar -xzf "boost_${BOOST_VERSION//./_}.tar.gz"
        rm "boost_${BOOST_VERSION//./_}.tar.gz"
    fi
    
    # Build Boost if not already built
    if [ ! -d "$BOOST_INSTALL_DIR" ]; then
        echo "Building Boost $BOOST_VERSION..."
        cd "$BOOST_DIR"
        
        # Bootstrap
        ./bootstrap.sh --prefix="$BOOST_INSTALL_DIR"
        
        ./b2 --prefix="$BOOST_INSTALL_DIR" \
             --with-iostreams \
             --with-system \
             --with-filesystem \
             --with-thread \
             --with-chrono \
             --with-atomic \
             --with-date_time \
             --with-regex \
             --with-serialization \
             --with-program_options \
             --with-test \
             $J \
             install
        
        echo "Boost installed to: $BOOST_INSTALL_DIR"
    else
        echo "Boost already built at: $BOOST_INSTALL_DIR"
    fi
    
    export BOOST_ROOT="$BOOST_INSTALL_DIR"
}
install_system_deps() {
    echo "Checking system dependencies..."
    
    local missing_deps=()
    
    if ! pkg-config --exists gmp 2>/dev/null; then
        missing_deps+=("gmp")
    fi
    
    if ! pkg-config --exists mpfr 2>/dev/null; then
        missing_deps+=("mpfr")
    fi
    
    if ! pkg-config --exists CGAL 2>/dev/null && ! find /usr -name "*CGAL*" -type d 2>/dev/null | grep -q CGAL; then
        missing_deps+=("cgal")
    fi
    
    if ! pkg-config --exists eigen3 2>/dev/null; then
        missing_deps+=("eigen")
    fi
    
    if [ ${#missing_deps[@]} -eq 0 ]; then
        echo "All system dependencies are already installed!"
        return 0
    fi
    
    echo "Installing missing dependencies: ${missing_deps[*]}"
    
    if command -v apt-get >/dev/null 2>&1; then
        echo "Installing dependencies via apt..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            libgmp-dev \
            libmpfr-dev \
            libcgal-dev \
            libeigen3-dev \
            libsuitesparse-dev \
            libboost-all-dev
    elif command -v dnf >/dev/null 2>&1; then
        echo "Installing dependencies via dnf..."
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            gmp-devel \
            mpfr-devel \
            CGAL-devel \
            eigen3-devel \
            suitesparse-devel \
            boost-devel
    elif command -v pacman >/dev/null 2>&1; then
        echo "Installing dependencies via pacman..."
        sudo pacman -S --noconfirm \
            base-devel \
            cmake \
            gmp \
            mpfr \
            cgal \
            eigen \
            suitesparse \
            boost
    else
        echo "Error: Could not detect package manager. Please install dependencies manually."
        exit 1
    fi
}
install_tbb() {
    local TBB_VERSION="2021.12.0"
    local TBB_DIR="$(xdg-user-dir DOWNLOAD)/oneTBB-$TBB_VERSION"
    local TBB_INSTALL_DIR="$HOME/.local/tbb_$TBB_VERSION"
    
    echo "Setting up Intel TBB $TBB_VERSION..."
    
    if [ ! -d "$TBB_DIR" ]; then
        echo "Downloading Intel TBB $TBB_VERSION..."
        cd "$(xdg-user-dir DOWNLOAD)"
        wget "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v$TBB_VERSION.tar.gz"
        tar -xzf "v$TBB_VERSION.tar.gz"
        rm "v$TBB_VERSION.tar.gz"
    fi
    
    # Build TBB if not already built
    if [ ! -d "$TBB_INSTALL_DIR" ]; then
        echo "Building Intel TBB $TBB_VERSION..."
        cd "$TBB_DIR"
        
        mkdir -p build
        cd build
        
        cmake -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX="$TBB_INSTALL_DIR" \
              -DTBB_BUILD_SHARED=ON \
              -DTBB_BUILD_STATIC=OFF \
              -DTBB_BUILD_TBBMALLOC=ON \
              -DTBB_BUILD_TBBMALLOC_PROXY=OFF \
              -DTBB_BUILD_TESTS=OFF \
              -DTBB_BUILD_BENCHMARKS=OFF \
              -DTBB_BUILD_EXAMPLES=OFF \
              -DTBB_STRICT=OFF \
              -DCMAKE_CXX_FLAGS="-Wno-error=array-bounds -Wno-array-bounds" \
              ..
        
        make $J
        make install
        
        echo "TBB installed to: $TBB_INSTALL_DIR"
    else
        echo "TBB already built at: $TBB_INSTALL_DIR"
    fi
    
    export TBB_ROOT="$TBB_INSTALL_DIR"
}

ORIGINAL_DIR="$(cd "$(dirname "$0")" && pwd)"

install_system_deps
install_boost
install_tbb

echo "ORIGINAL_DIR is: $ORIGINAL_DIR"
cd "$ORIGINAL_DIR"
echo "Current directory after cd: $(pwd)"
echo "TBB_ROOT: $TBB_ROOT"
echo "BOOST_ROOT: $BOOST_ROOT"
echo "Fixing CMake version requirements..."
find . -name "CMakeLists.txt" -exec sed -i 's/cmake_minimum_required(VERSION 3\.1)/cmake_minimum_required(VERSION 3.10)/g' {} \;
find . -name "CMakeLists.txt" -exec sed -i 's/cmake_minimum_required(VERSION 3\.2)/cmake_minimum_required(VERSION 3.10)/g' {} \;
find . -name "CMakeLists.txt" -exec sed -i 's/cmake_minimum_required(VERSION 3\.3)/cmake_minimum_required(VERSION 3.10)/g' {} \;
find . -name "CMakeLists.txt" -exec sed -i 's/cmake_minimum_required(VERSION 3\.4)/cmake_minimum_required(VERSION 3.10)/g' {} \;
mkdir -p build
cd build
echo "Building in directory: $(pwd)"
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.1 \
      -DCMAKE_POLICY_DEFAULT_CMP0074=NEW \
      -DCMAKE_POLICY_DEFAULT_CMP0135=NEW \
      -DBoost_NO_SYSTEM_PATHS=ON \
      -DBOOST_ROOT="$BOOST_ROOT" \
      -DTBB_ROOT="$TBB_ROOT" \
      -DMICRO_WITH_TBB=ON \
      -DCMAKE_PREFIX_PATH="$TBB_ROOT" \
      ..
sleep 2
echo "Building essential tools..."
make $J stitch_cells_cli
make $J cut_cells_cli  
make $J stack_cells
echo "Build completed successfully!"
echo "Built tools:"
echo "  - stitch_cells_cli"
echo "  - cut_cells_cli"
echo "  - stack_cells"
