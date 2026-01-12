# FFmpeg-Rockchip: Configuration, Compilation, and NV12 to MJPEG Encoding Guide

This comprehensive guide provides step-by-step instructions for configuring and compiling FFmpeg with Rockchip hardware acceleration support. It includes detailed information about the MJPEG encoder and provides multiple test implementations for encoding NV12 video frames to MJPEG using the Rockchip VEPU hardware accelerator.

## 1. Introduction to FFmpeg-Rockchip

FFmpeg-Rockchip is a specialized fork of the popular FFmpeg multimedia framework, optimized specifically for Rockchip SoCs (System-on-Chip). It leverages the Rockchip Media Process Platform (MPP) and RGA (2D Raster Graphic Acceleration) to provide a complete hardware transcoding pipeline. This enables high-performance, zero-copy video processing, making it ideal for embedded systems, surveillance applications, and other resource-constrained devices. [1]

### Key Features

The FFmpeg-Rockchip project provides several important capabilities for hardware-accelerated video processing:

*   **Hardware-accelerated decoding:** Supports multiple codecs including H.264, HEVC, VP9, and AV1 at resolutions up to 8K with 10-bit color depth.
*   **Hardware-accelerated encoding:** Provides encoders for H.264, HEVC, and MJPEG with support for various rate control modes.
*   **RGA-accelerated filtering:** Includes filters for scaling, cropping, format conversion, and image composition.
*   **Zero-copy memory management:** Minimizes data transfers between the CPU and hardware accelerators, reducing latency and power consumption.
*   **Async encoding:** Supports frame-parallel encoding for improved throughput.
*   **AFBC support:** Can produce and consume ARM Frame Buffer Compression (AFBC) images for improved memory efficiency.

## 2. Prerequisites and System Requirements

Before beginning the compilation process, ensure that your system meets the following requirements:

### Hardware Requirements

*   A Rockchip-based device (e.g., RK3588, RK3588S, RK3576)
*   Sufficient storage space (at least 2GB for compilation)
*   Adequate RAM (at least 2GB recommended)

### Software Requirements

*   Linux-based operating system (Debian, Ubuntu, or Rockchip BSP)
*   Rockchip BSP/vendor kernel (versions 5.10 and 6.1 are tested)
*   Development tools and libraries

### Installing Dependencies

```bash
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
```

### Device File Permissions

The FFmpeg-Rockchip implementation requires access to specific device files. Ensure your user has appropriate permissions:

```bash
# DRM allocator
/dev/dri

# DMA_HEAP allocator
/dev/dma_heap

# RGA filters
/dev/rga

# MPP codecs
/dev/mpp_service

# Optional (for compatibility with older kernels)
/dev/iep
/dev/mpp-service
/dev/vpu_service
/dev/vpu-service
/dev/hevc_service
/dev/hevc-service
/dev/rkvdec
/dev/rkvenc
/dev/vepu
/dev/h265e
```

To grant permissions, you can add your user to the appropriate groups or use `sudo` when running FFmpeg.

## 3. Building and Installing Dependencies

FFmpeg-Rockchip relies on two critical libraries for hardware acceleration: `rkmpp` (Media Process Platform) and `rkrga` (RGA acceleration). These must be compiled and installed from source before building FFmpeg itself.

### 3.1. Building and Installing `rkmpp`

The `rkmpp` library provides the interface to the Rockchip MPP, which handles video encoding and decoding operations.

```bash
mkdir -p ~/dev && cd ~/dev
git clone -b jellyfin-mpp --depth=1 https://gitee.com/nyanmisaka/mpp.git rkmpp
cd rkmpp
mkdir build && cd build

cmake \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TEST=OFF \
    ..

make -j$(nproc)
sudo make install
```

This process will compile the MPP library and install it to `/usr/lib` and `/usr/include`.

### 3.2. Building and Installing `rkrga`

The `rkrga` library provides the interface to the Rockchip RGA, which handles 2D graphics acceleration operations such as scaling and format conversion.

```bash
cd ~/dev
git clone -b jellyfin-rga --depth=1 https://gitee.com/nyanmisaka/rga.git rkrga
cd rkrga
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
```

## 4. Building FFmpeg-Rockchip

With the dependencies in place, you can now clone and build FFmpeg-Rockchip itself.

### 4.1. Cloning the Repository

```bash
cd ~/dev
git clone --depth=1 https://github.com/nyanmisaka/ffmpeg-rockchip.git ffmpeg
cd ffmpeg
```

### 4.2. Configuring FFmpeg

The configure script determines which features and components will be compiled into FFmpeg. The following parameters are essential for Rockchip hardware acceleration:

```bash
./configure \
    --prefix=/usr \
    --enable-gpl \
    --enable-version3 \
    --enable-libdrm \
    --enable-rkmpp \
    --enable-rkrga
```

**Important Configuration Notes:**

*   `--enable-gpl`: Enables GPL-licensed components. This is recommended but optional; you can use `--enable-version3` with LGPL v3 instead if preferred.
*   `--enable-version3`: Enables version 3 of GPL/LGPL licenses.
*   `--enable-libdrm`: Enables DRM (Direct Rendering Manager) support for memory allocation.
*   `--enable-rkmpp`: Enables Rockchip MPP support for encoding and decoding.
*   `--enable-rkrga`: Enables Rockchip RGA support for filtering and scaling.

### 4.3. Compiling FFmpeg

```bash
make -j$(nproc)
```

The compilation process may take 10-30 minutes depending on your system's performance.

### 4.4. Installing FFmpeg

```bash
sudo make install
```

This will install FFmpeg binaries and libraries to the prefix directory (`/usr` in this case).

### 4.5. Verifying the Installation

After installation, verify that the Rockchip-specific components are available:

```bash
ffmpeg -decoders | grep rkmpp
ffmpeg -encoders | grep rkmpp
ffmpeg -filters | grep rkrga
```

You should see output similar to:

```
V..... h264_rkmpp           Rockchip MPP (Media Process Platform) H264 decoder (codec h264)
V..... hevc_rkmpp           Rockchip MPP (Media Process Platform) HEVC decoder (codec hevc)
V..... mjpeg_rkmpp          Rockchip MPP (Media Process Platform) MJPEG decoder (codec mjpeg)
...
V..... h264_rkmpp           Rockchip MPP (Media Process Platform) H264 encoder (codec h264)
V..... hevc_rkmpp           Rockchip MPP (Media Process Platform) HEVC encoder (codec hevc)
V..... mjpeg_rkmpp          Rockchip MPP (Media Process Platform) MJPEG encoder (codec mjpeg)
...
... scale_rkrga       V->V       Rockchip RGA (2D Raster Graphic Acceleration) video resizer and format converter
... vpp_rkrga         V->V       Rockchip RGA (2D Raster Graphic Acceleration) video post-process (scale/crop/transpose)
```

## 5. MJPEG Encoder Configuration and Usage

The Rockchip MJPEG encoder (`mjpeg_rkmpp`) is a hardware-accelerated implementation of the MJPEG codec. It is particularly useful for streaming applications, surveillance systems, and other scenarios where individual frame compression is beneficial.

### 5.1. Supported Pixel Formats

The MJPEG encoder supports the following input pixel formats:

| Format | Description |
|--------|-------------|
| `yuv420p` | YUV 4:2:0 planar |
| `yuvj420p` | YUV 4:2:0 planar (JPEG range) |
| `yuv422p` | YUV 4:2:2 planar |
| `yuvj422p` | YUV 4:2:2 planar (JPEG range) |
| `yuv444p` | YUV 4:4:4 planar |
| `yuvj444p` | YUV 4:4:4 planar (JPEG range) |
| `nv12` | YUV 4:2:0 semi-planar (Y plane followed by interleaved UV) |
| `nv21` | YUV 4:2:0 semi-planar (Y plane followed by interleaved VU) |
| `nv16` | YUV 4:2:2 semi-planar |
| `nv24` | YUV 4:4:4 semi-planar |

### 5.2. Encoder Options

The MJPEG encoder provides several options for controlling the encoding process:

```bash
ffmpeg -hide_banner -h encoder=mjpeg_rkmpp
```

Key options include:

| Option | Type | Range | Default | Description |
|--------|------|-------|---------|-------------|
| `-qp_init` | int | -1 to 99 | -1 | Initial QP/Q_Factor value |
| `-qp_max` | int | -1 to 99 | -1 | Maximum QP/Q_Factor value |
| `-qp_min` | int | -1 to 99 | -1 | Minimum QP/Q_Factor value |
| `-chroma_fmt` | int | auto/420/422/444 | auto | Output chroma format |
| `-q:v` | int | 2 to 31 | - | Quality level (lower is better) |

### 5.3. Basic Encoding Examples

#### Example 1: Encode NV12 raw video to MJPEG

```bash
ffmpeg -f rawvideo -video_size 1920x1080 -pixel_format nv12 -framerate 30 \
    -i input.yuv \
    -c:v mjpeg_rkmpp \
    -q:v 5 \
    output.mjpeg
```

#### Example 2: Transcode existing video to MJPEG

```bash
ffmpeg -i input.mp4 \
    -c:v mjpeg_rkmpp \
    -q:v 5 \
    output.mjpeg
```

#### Example 3: Encode with quality control

```bash
ffmpeg -i input.mp4 \
    -c:v mjpeg_rkmpp \
    -qp_init 10 \
    -qp_min 5 \
    -qp_max 20 \
    output.mjpeg
```

## 6. NV12 Format Overview

NV12 is a semi-planar YUV 4:2:0 format commonly used in video processing. Understanding its structure is essential for working with raw video data.

### 6.1. NV12 Memory Layout

NV12 consists of two planes:

1. **Y plane:** Contains luminance (brightness) information for all pixels
2. **UV plane:** Contains interleaved chrominance (color) information for 2x2 pixel blocks

For a 1920×1080 resolution video:

*   Y plane size: 1920 × 1080 = 2,073,600 bytes
*   UV plane size: 960 × 540 = 518,400 bytes (each U and V component is half the resolution)
*   Total frame size: 2,592,000 bytes (1.5 bytes per pixel)

### 6.2. NV12 Byte Order

The UV plane uses interleaved format:

```
UV plane: U0 V0 U1 V1 U2 V2 ... (for each 2×2 block of Y pixels)
```

## 7. Test Code Implementations

This section provides multiple implementations for testing NV12 to MJPEG encoding using FFmpeg-Rockchip.

### 7.1. C Implementation

A complete C implementation is provided in `nv12_to_mjpeg_test.c`. This program demonstrates:

*   Allocating and initializing FFmpeg codec contexts
*   Generating test NV12 frames
*   Encoding frames using the hardware encoder
*   Proper error handling and resource cleanup

**Compilation:**

```bash
gcc -o nv12_to_mjpeg_test nv12_to_mjpeg_test.c \
    $(pkg-config --cflags --libs libavcodec libavformat libavutil)
```

**Usage:**

```bash
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg
```

### 7.2. Python Implementation

A Python implementation is provided in `nv12_to_mjpeg_test.py`. This script uses the PyAV library to interface with FFmpeg.

**Installation:**

```bash
pip install av numpy
```

**Usage:**

```bash
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --output output.mjpeg
```

### 7.3. Shell Script Examples

Multiple shell script examples are provided in `ffmpeg_rockchip_examples.sh`, including:

*   Basic NV12 to MJPEG encoding
*   Encoding with different quality levels
*   Batch encoding
*   Real-time camera encoding
*   Encoding with RGA scaling

**Usage:**

```bash
chmod +x ffmpeg_rockchip_examples.sh
./ffmpeg_rockchip_examples.sh
```

## 8. Troubleshooting and Common Issues

### Issue: Hardware encoder not found

**Solution:** Verify that the Rockchip MPP libraries are properly installed:

```bash
ldconfig -p | grep mpp
pkg-config --list-all | grep mpp
```

### Issue: Permission denied when accessing device files

**Solution:** Grant appropriate permissions to device files:

```bash
sudo usermod -a -G video,audio $USER
# Log out and log back in for changes to take effect
```

### Issue: Segmentation fault during encoding

**Solution:** Ensure that the MPP runtime and headers are up-to-date. Older versions can cause undefined behavior:

```bash
# Rebuild and reinstall MPP
cd ~/dev/rkmpp/build
make clean
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

### Issue: Low encoding performance

**Solution:** Verify that the hardware encoder is being used and check system load:

```bash
# Check if hardware encoder is being used
ffmpeg -i input.mp4 -c:v mjpeg_rkmpp -v verbose output.mjpeg 2>&1 | grep -i "hardware\|rkmpp"

# Monitor system performance
top
```

## 9. Performance Considerations

When using FFmpeg-Rockchip for MJPEG encoding, consider the following factors:

*   **Resolution:** Higher resolutions require more processing power. Check your device's datasheet for maximum supported resolutions.
*   **Frame rate:** Increasing the frame rate linearly increases the processing load.
*   **Quality settings:** Lower quality values result in smaller file sizes but require more encoding time.
*   **Async encoding:** Enable async encoding for improved throughput when encoding multiple streams.

## 10. References

[1] nyanmisaka/ffmpeg-rockchip. (n.d.). GitHub. Retrieved January 12, 2026, from https://github.com/nyanmisaka/ffmpeg-rockchip

[2] Rockchip Media Process Platform (MPP). (n.d.). Rockchip Open Source. Retrieved January 12, 2026, from https://opensource.rock-chips.com/wiki_Mpp

[3] FFmpeg Compilation Guide. (n.d.). FFmpeg Wiki. Retrieved January 12, 2026, from https://trac.ffmpeg.org/wiki/CompilationGuide

[4] Jellyfin Hardware Acceleration - Rockchip. (n.d.). Jellyfin Documentation. Retrieved January 12, 2026, from https://jellyfin.org/docs/general/post-install/transcoding/hardware-acceleration/rockchip/
