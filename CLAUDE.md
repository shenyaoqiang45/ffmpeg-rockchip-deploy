# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains test implementations and examples for encoding NV12 video frames to MJPEG using FFmpeg-Rockchip with Rockchip VEPU hardware acceleration. It includes C, Python, and shell script implementations for testing and demonstrating hardware-accelerated video encoding on Rockchip SoCs (RK3588, RK3588S, RK3576, etc.).

## Build Commands

### C Test Program

```bash
# Build using Makefile
make

# Clean build artifacts
make clean

# Check dependencies
make check-deps

# Install to /usr/local/bin
make install

# Manual compilation
gcc -o nv12_to_mjpeg_test nv12_to_mjpeg_test.c \
    $(pkg-config --cflags --libs libavcodec libavformat libavutil)
```

### Running Tests

```bash
# C program
./nv12_to_mjpeg_test <width> <height> <fps> <output.mjpeg>
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg

# Python script
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --output output.mjpeg
python3 nv12_to_mjpeg_test.py --frames 200 --output output.mjpeg

# Shell script examples (interactive or by number)
./ffmpeg_rockchip_examples.sh
./ffmpeg_rockchip_examples.sh 1  # Run specific example
```

### Verification

```bash
# Check output file
ffprobe output.mjpeg

# Verify hardware encoder availability
ffmpeg -encoders | grep rkmpp
ffmpeg -hide_banner -h encoder=mjpeg_rkmpp
```

## Architecture

### Core Components

**nv12_to_mjpeg_test.c** - C implementation using FFmpeg libav* libraries
- `EncoderContext` struct manages encoder state (format context, codec context, stream)
- `encoder_init()` initializes output container and codec, prefers `mjpeg_rkmpp` hardware encoder
- `generate_nv12_frame()` creates test frames with gradient patterns
- `encode_frame()` sends frames to encoder and writes packets to output
- `flush_encoder()` drains remaining frames from encoder buffer
- Encodes 100 frames by default (FRAME_COUNT constant)

**nv12_to_mjpeg_test.py** - Python implementation using PyAV
- `NV12ToMJPEGEncoder` class encapsulates encoding logic
- Uses av.VideoFrame for frame creation and manipulation
- NumPy arrays for direct plane data access
- Similar workflow to C version but with Python-friendly API

**ffmpeg_rockchip_examples.sh** - Shell script with 8 example scenarios
- Examples 1-7: Various encoding scenarios (CQP, quality, scaling, camera, batch)
- Example 8: Check available encoders
- Interactive menu or command-line mode

### NV12 Format Structure

NV12 is semi-planar YUV 4:2:0:
- Y plane: Full resolution luminance (width × height bytes)
- UV plane: Half resolution, interleaved chrominance (width/2 × height/2 × 2 bytes)
- Total: 1.5 bytes per pixel

For 1920×1080: Y plane = 2,073,600 bytes, UV plane = 518,400 bytes, Total = 2,592,000 bytes/frame

### Hardware Encoder

The `mjpeg_rkmpp` encoder uses Rockchip VEPU hardware acceleration:
- Quality control via `-q:v` (2-31, lower = better) or `-qp_init` (CQP mode)
- Falls back to software `mjpeg` encoder if hardware unavailable
- Can be combined with `scale_rkrga` filter for hardware-accelerated scaling

## FFmpeg Command Patterns

```bash
# Basic NV12 to MJPEG encoding
ffmpeg -f rawvideo -video_size 1920x1080 -pixel_format nv12 -framerate 30 \
    -i input.yuv \
    -c:v mjpeg_rkmpp \
    -q:v 5 \
    output.mjpeg

# Transcode with quality control
ffmpeg -i input.mp4 \
    -c:v mjpeg_rkmpp \
    -qp_init 10 \
    output.mjpeg

# With RGA hardware scaling
ffmpeg -f rawvideo -video_size 3840x2160 -pixel_format nv12 -framerate 30 \
    -i input_4k.yuv \
    -vf "scale_rkrga=1920:1080" \
    -c:v mjpeg_rkmpp \
    -q:v 5 \
    output.mjpeg

# Generate test video
ffmpeg -f lavfi -i testsrc=size=1920x1080:duration=10:rate=30 \
    -pix_fmt nv12 -f rawvideo output.yuv
```

## System Requirements

- Rockchip SoC with VEPU support (RK3588, RK3588S, RK3576, etc.)
- FFmpeg-Rockchip build with rkmpp and rkrga support
- Linux kernel 5.10+
- Dependencies: libavcodec, libavformat, libavutil (for C program)
- Python 3.6+ with PyAV (av) library (for Python script)

## Common Issues

**Hardware encoder not found**: Verify FFmpeg-Rockchip installation with `ffmpeg -encoders | grep rkmpp`

**Permission errors**: Add user to video/audio groups: `sudo usermod -a -G video,audio $USER`

**Segmentation faults**: Rebuild MPP libraries from source

## Development Notes

- All implementations generate synthetic test patterns (gradient-based)
- C program uses FFmpeg's send/receive API for encoding
- Python script uses PyAV's stream.encode() wrapper
- Shell script provides ready-to-run examples for common scenarios
- Quality parameter `-q:v 5` provides good balance between quality and file size
