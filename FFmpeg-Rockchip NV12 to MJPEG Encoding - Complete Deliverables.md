# FFmpeg-Rockchip NV12 to MJPEG Encoding - Complete Deliverables

## Overview

This comprehensive package contains a complete guide and multiple test implementations for encoding NV12 video frames to MJPEG using FFmpeg-Rockchip with Rockchip VEPU hardware acceleration. All materials are production-ready and thoroughly documented.

## Files Included

### 1. Documentation Files

#### ffmpeg_rockchip_guide.md (14 KB)
**Complete compilation and configuration guide with 10 sections:**

- Introduction to FFmpeg-Rockchip and key features
- Prerequisites and system requirements
- Detailed dependency installation (rkmpp and rkrga)
- Step-by-step FFmpeg-Rockchip compilation
- MJPEG encoder configuration and options
- NV12 format overview and memory layout
- Test code implementations overview
- Troubleshooting guide for common issues
- Performance considerations and optimization
- Complete references and citations

**Key Sections:**
- 3.1: Building and installing rkmpp library
- 3.2: Building and installing rkrga library
- 4: Building FFmpeg-Rockchip with proper configure options
- 5: MJPEG encoder configuration with supported formats
- 6: NV12 format technical details
- 7: Test code implementations
- 8: Troubleshooting and common issues
- 9: Performance considerations

#### README.md (5.6 KB)
**Quick start guide and usage reference:**

- Quick start instructions (3 simple steps)
- Usage examples for all implementations
- NV12 format reference table
- Encoder options and quality settings
- Troubleshooting tips
- Performance optimization guide
- System requirements checklist
- References and support links

### 2. Test Implementations

#### nv12_to_mjpeg_test.c (11 KB)
**Complete C implementation of NV12 to MJPEG encoder:**

**Features:**
- Full FFmpeg API usage demonstration
- NV12 frame generation with moving pattern
- Hardware encoder initialization and configuration
- Proper error handling with descriptive messages
- Resource cleanup and memory management
- Packet encoding and output handling
- Frame flushing mechanism
- Support for hardware and software fallback

**Key Functions:**
- `generate_nv12_frame()` - Generate test NV12 frames with patterns
- `encoder_init()` - Initialize FFmpeg codec context and output file
- `encode_frame()` - Encode a single frame
- `flush_encoder()` - Flush remaining frames
- `encoder_cleanup()` - Clean up resources
- `run_encoding()` - Main encoding loop

**Compilation:**
```bash
gcc -o nv12_to_mjpeg_test nv12_to_mjpeg_test.c \
    $(pkg-config --cflags --libs libavcodec libavformat libavutil)
```

**Usage:**
```bash
./nv12_to_mjpeg_test <width> <height> <fps> <output.mjpeg>
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg
```

**Output:** 100 test frames encoded to MJPEG file

#### nv12_to_mjpeg_test.py (7.3 KB)
**Python implementation using PyAV library:**

**Features:**
- Object-oriented encoder design
- NumPy-based frame generation
- Hardware encoder detection with fallback
- Command-line argument parsing
- Comprehensive error handling
- Progress reporting

**Key Classes:**
- `NV12ToMJPEGEncoder` - Main encoder class with methods:
  - `initialize()` - Set up encoder
  - `generate_nv12_frame()` - Create test frames
  - `encode_frame()` - Encode single frame
  - `flush()` - Flush encoder
  - `close()` - Clean up resources
  - `run()` - Main encoding loop

**Requirements:**
```bash
pip install av numpy
```

**Usage:**
```bash
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --output output.mjpeg
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --frames 200 --output output.mjpeg
```

**Command-line Options:**
- `--width` (default: 1920) - Video width
- `--height` (default: 1080) - Video height
- `--fps` (default: 30) - Frames per second
- `--output` (default: output.mjpeg) - Output file
- `--frames` (default: 100) - Number of frames to encode

#### ffmpeg_rockchip_examples.sh (8.5 KB)
**Shell script with 8 comprehensive examples:**

**Examples Included:**

1. **Generate and Encode NV12 to MJPEG**
   - Generate raw NV12 video
   - Encode using hardware encoder
   - Output file information

2. **Encode with CQP Rate Control**
   - Constant Quantization Parameter mode
   - Fixed quality encoding
   - Useful for consistent output

3. **Encode with Quality Parameter**
   - Variable quality encoding
   - High-quality output
   - Optimized for visual quality

4. **Transcode Video File to MJPEG**
   - Convert existing video files
   - Automatic sample video generation
   - Full transcoding pipeline

5. **Batch Encoding with Different Quality Levels**
   - Encode same source with multiple QP values
   - Compare file sizes and quality
   - Performance analysis

6. **Real-time Camera Encoding**
   - Capture from /dev/video0
   - Real-time MJPEG encoding
   - Streaming-ready output

7. **Encoding with RGA Scaling**
   - 4K to 1080p downscaling
   - Hardware-accelerated scaling
   - Demonstrates RGA filter usage

8. **Check Available Encoders**
   - List available MJPEG encoders
   - Display encoder options
   - Verify hardware support

**Usage:**
```bash
chmod +x ffmpeg_rockchip_examples.sh
./ffmpeg_rockchip_examples.sh              # Interactive mode
./ffmpeg_rockchip_examples.sh 1            # Run example 1
./ffmpeg_rockchip_examples.sh 8            # Check encoders
```

### 3. Build Configuration

#### Makefile (1.7 KB)
**Build automation for C test program:**

**Targets:**
- `make` or `make all` - Build the test program (default)
- `make clean` - Remove build artifacts
- `make install` - Install to /usr/local/bin
- `make help` - Display help information
- `make check-deps` - Verify FFmpeg dependencies

**Features:**
- Automatic dependency detection using pkg-config
- Optimized compilation flags (-O2)
- Warning flags for code quality
- Dependency checking

**Usage:**
```bash
make                    # Build
make clean              # Clean
make install            # Install
make check-deps         # Check dependencies
make help               # Show help
```

## Quick Start Guide

### Step 1: Install FFmpeg-Rockchip (Complete Process)

```bash
# Install system dependencies
sudo apt-get update
sudo apt-get install -y git build-essential cmake meson pkg-config gcc libdrm-dev ninja-build

# Create development directory
mkdir -p ~/dev && cd ~/dev

# Build and install rkmpp
git clone -b jellyfin-mpp --depth=1 https://gitee.com/nyanmisaka/mpp.git rkmpp
cd rkmpp && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON -DBUILD_TEST=OFF ..
make -j$(nproc)
sudo make install
cd ~/dev

# Build and install rkrga
git clone -b jellyfin-rga --depth=1 https://gitee.com/nyanmisaka/rga.git rkrga
cd rkrga
meson setup build --prefix=/usr --libdir=lib --buildtype=release \
      --default-library=shared -Dcpp_args=-fpermissive -Dlibdrm=false -Dlibrga_demo=false
meson configure build
ninja -C build
sudo ninja -C build install
cd ~/dev

# Build FFmpeg-Rockchip
git clone --depth=1 https://github.com/nyanmisaka/ffmpeg-rockchip.git ffmpeg
cd ffmpeg
./configure --prefix=/usr --enable-gpl --enable-version3 \
            --enable-libdrm --enable-rkmpp --enable-rkrga
make -j$(nproc)
sudo make install

# Verify installation
ffmpeg -encoders | grep rkmpp
```

### Step 2: Build Test Program

```bash
make
```

### Step 3: Run Test

```bash
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg
```

### Step 4: Verify Output

```bash
ffprobe output.mjpeg
ffplay output.mjpeg
```

## Technical Specifications

### NV12 Format Details

| Aspect | Specification |
|--------|---------------|
| Format Name | NV12 (Semi-planar YUV 4:2:0) |
| Y Plane | Full resolution (1920×1080) |
| UV Plane | Half resolution (960×540) |
| Interleaving | U, V, U, V, ... |
| Bytes per Pixel | 1.5 |
| Frame Size (1920×1080) | 2,592,000 bytes |

### MJPEG Encoder Options

| Option | Type | Range | Default | Purpose |
|--------|------|-------|---------|---------|
| `-q:v` | int | 2-31 | - | Quality level (lower = better) |
| `-qp_init` | int | -1 to 99 | -1 | Initial QP value |
| `-qp_min` | int | -1 to 99 | -1 | Minimum QP value |
| `-qp_max` | int | -1 to 99 | -1 | Maximum QP value |
| `-chroma_fmt` | int | auto/420/422/444 | auto | Output chroma format |

### Supported Input Formats

- NV12, NV21, NV16, NV24
- YUV420p, YUV422p, YUV444p
- YUVJ420p, YUVJ422p, YUVJ444p
- RGB24, BGR24, RGBA, BGRA
- And other FFmpeg-supported formats

## Performance Characteristics

| Metric | Value |
|--------|-------|
| Maximum Resolution | 8K (depending on hardware) |
| Frame Rate | Variable (depends on resolution and quality) |
| Quality Range | QP 2-31 (lower = better) |
| Latency | Low (hardware-accelerated) |
| CPU Usage | Minimal (offloaded to VEPU) |
| Memory Usage | Efficient (zero-copy where possible) |

## System Requirements

| Requirement | Specification |
|-------------|---------------|
| **Hardware** | Rockchip SoC (RK3588, RK3588S, RK3576, etc.) |
| **OS** | Linux with Rockchip BSP kernel |
| **Kernel Version** | 5.10 or later |
| **RAM** | At least 2GB |
| **Storage** | 2GB for compilation |
| **Build Tools** | gcc, cmake, meson, pkg-config, git |

## Troubleshooting Guide

### Issue: Hardware encoder not found

**Solution:**
```bash
ffmpeg -encoders | grep rkmpp
# If no results, rebuild FFmpeg with Rockchip support
```

### Issue: Permission denied errors

**Solution:**
```bash
sudo usermod -a -G video,audio $USER
# Log out and back in
```

### Issue: Segmentation faults

**Solution:**
```bash
# Rebuild MPP with latest version
cd ~/dev/rkmpp/build
make clean
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

## Usage Examples

### C Program Examples

```bash
# Basic usage
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg

# Different resolutions
./nv12_to_mjpeg_test 1280 720 24 output_720p.mjpeg
./nv12_to_mjpeg_test 3840 2160 30 output_4k.mjpeg
```

### Python Script Examples

```bash
# Default parameters
python3 nv12_to_mjpeg_test.py

# Custom parameters
python3 nv12_to_mjpeg_test.py --width 1280 --height 720 --fps 24 --frames 150

# High quality
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --frames 300
```

### Direct FFmpeg Commands

```bash
# Encode raw NV12
ffmpeg -f rawvideo -video_size 1920x1080 -pixel_format nv12 -framerate 30 \
    -i input.yuv -c:v mjpeg_rkmpp -q:v 5 output.mjpeg

# Transcode video
ffmpeg -i input.mp4 -c:v mjpeg_rkmpp -q:v 5 output.mjpeg

# With quality control
ffmpeg -i input.mp4 -c:v mjpeg_rkmpp -qp_init 10 -qp_min 5 -qp_max 20 output.mjpeg
```

## References and Resources

1. **FFmpeg-Rockchip GitHub**
   - URL: https://github.com/nyanmisaka/ffmpeg-rockchip
   - Content: Source code, issues, wiki

2. **Rockchip MPP Documentation**
   - URL: https://opensource.rock-chips.com/wiki_Mpp
   - Content: Technical specifications, API reference

3. **FFmpeg Compilation Guide**
   - URL: https://trac.ffmpeg.org/wiki/CompilationGuide
   - Content: Build instructions, configuration options

4. **Jellyfin Rockchip Hardware Acceleration**
   - URL: https://jellyfin.org/docs/general/post-install/transcoding/hardware-acceleration/rockchip/
   - Content: Real-world usage examples, performance tips

## Support and Issues

- **FFmpeg-Rockchip Issues:** https://github.com/nyanmisaka/ffmpeg-rockchip/issues
- **Rockchip Forum:** https://forum.rock-chips.com/
- **FFmpeg Support:** https://ffmpeg.org/support.html

## License

This documentation and test code are provided for educational and development purposes. FFmpeg-Rockchip is licensed under GPL/LGPL. Please refer to the FFmpeg-Rockchip repository for detailed license information.

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-12 | Initial release with complete documentation and test implementations |

---

**Package Contents Summary:**
- 1 Comprehensive guide (14 KB)
- 1 Quick start README (5.6 KB)
- 1 C implementation (11 KB)
- 1 Python implementation (7.3 KB)
- 1 Shell script with 8 examples (8.5 KB)
- 1 Makefile for building (1.7 KB)
- **Total:** 6 files, ~48 KB of code and documentation

**Status:** Complete, tested, and production-ready
**Created:** January 12, 2026
