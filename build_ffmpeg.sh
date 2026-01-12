#!/bin/bash
set -e

# Print helper messages
echo "========================================"
echo "Starting FFmpeg-Rockchip Build"
echo "========================================"

# Navigate to dev directory
mkdir -p ~/dev
cd ~/dev

# Section 4.1: Cloning the Repository
echo ">>> Cloning ffmpeg-rockchip..."
if [ -d "ffmpeg" ]; then
    echo "Directory ffmpeg already exists, skipping clone..."
else
    git clone --depth=1 https://github.com/nyanmisaka/ffmpeg-rockchip.git ffmpeg
fi

cd ffmpeg

# Section 4.2: Configuring FFmpeg
echo ">>> Configuring FFmpeg..."
./configure \
    --prefix=/usr \
    --enable-gpl \
    --enable-version3 \
    --enable-libdrm \
    --enable-rkmpp \
    --enable-rkrga

# Section 4.3: Compiling FFmpeg
echo ">>> Compiling FFmpeg (this may take 10-30 minutes)..."
make -j$(nproc)

# Section 4.4: Installing FFmpeg
echo ">>> Installing FFmpeg..."
sudo make install

# Section 4.5: Verification hint
echo "========================================"
echo "FFmpeg build and install complete!"
echo "Run the following to verify installation:"
echo "  ffmpeg -decoders | grep rkmpp"
echo "  ffmpeg -encoders | grep rkmpp"
echo "  ffmpeg -filters | grep rkrga"
echo "========================================"
