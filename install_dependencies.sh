#!/bin/bash
set -e

# Print helper messages
echo "========================================"
echo "Starting Rockchip FFmpeg Dependency Installation"
echo "========================================"

# Section 2: Installing Dependencies
echo ">>> Installing apt packages..."
sudo apt-get update
sudo apt-get install -y \
    git \
    build-essential \
    cmake \
    meson \
    pkg-config \
    gcc \
    libdrm-dev \
    ninja-build

# Create dev directory
echo ">>> Creating ~/dev directory..."
mkdir -p ~/dev
cd ~/dev

# Section 3.1: Building and Installing rkmpp
echo ">>> Cloning and building rkmpp (jellyfin-mpp)..."
if [ -d "rkmpp" ]; then
    echo "Directory rkmpp already exists, skipping clone..."
else
    git clone -b jellyfin-mpp --depth=1 https://gitee.com/nyanmisaka/mpp.git rkmpp
fi

cd rkmpp
# IMPORTANT:
# The rkmpp repo *tracks* a top-level "build/" directory containing
# CMake helper scripts like build/cmake/merge_objects.cmake and build/cmake/version.in.
# Do NOT delete it. Use an out-of-source build directory instead.
rm -rf builddir
mkdir builddir && cd builddir

cmake \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TEST=OFF \
    ..

make -j$(nproc)
sudo make install

echo ">>> rkmpp installed successfully."

# Section 3.2: Building and Installing rkrga
echo ">>> Cloning and building rkrga (jellyfin-rga)..."
cd ~/dev

if [ -d "rkrga" ]; then
    echo "Directory rkrga already exists, skipping clone..."
else
    git clone -b jellyfin-rga --depth=1 https://gitee.com/nyanmisaka/rga.git rkrga
fi

cd rkrga
# Clean build if it exists
rm -rf build

meson setup build \
    --prefix=/usr \
    --libdir=lib \
    --buildtype=release \
    --default-library=shared \
    -Dcpp_args=-fpermissive \
    -Dlibdrm=false \
    -Dlibrga_demo=false

meson configure build
ninja -C build
sudo ninja -C build install

echo ">>> rkrga installed successfully."
echo "========================================"
echo "Dependency installation complete!"
echo "========================================"
