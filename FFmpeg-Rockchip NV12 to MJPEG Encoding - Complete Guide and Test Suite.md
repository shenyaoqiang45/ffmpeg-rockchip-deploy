# FFmpeg-Rockchip NV12 to MJPEG Encoding - Complete Guide and Test Suite

This repository contains a comprehensive guide and test implementations for encoding NV12 video frames to MJPEG using FFmpeg-Rockchip with Rockchip VEPU hardware acceleration.

## Contents

- **ffmpeg_rockchip_guide.md** - Complete compilation and configuration guide
- **nv12_to_mjpeg_test.c** - C implementation of NV12 to MJPEG encoder
- **nv12_to_mjpeg_test.py** - Python implementation using PyAV
- **ffmpeg_rockchip_examples.sh** - Shell script examples for various encoding scenarios
- **Makefile** - Build configuration for C test program

## Quick Start

### 1. Install FFmpeg-Rockchip

Follow the detailed instructions in `ffmpeg_rockchip_guide.md` for:
- Installing dependencies (rkmpp and rkrga)
- Building FFmpeg with Rockchip support
- Verifying the installation

### 2. Build the C Test Program

```bash
make
```

Or manually:

```bash
gcc -o nv12_to_mjpeg_test nv12_to_mjpeg_test.c \
    $(pkg-config --cflags --libs libavcodec libavformat libavutil)
```

### 3. Run the Test

```bash
# Generate a 1920x1080 MJPEG video at 30 fps
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg
```

### 4. Verify Output

```bash
ffprobe output.mjpeg
```

## Usage Examples

### C Program

```bash
./nv12_to_mjpeg_test <width> <height> <fps> <output_file>

# Example: 1920x1080 at 30 fps
./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg

# Example: 1280x720 at 24 fps
./nv12_to_mjpeg_test 1280 720 24 output.mjpeg
```

### Python Script

```bash
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --output output.mjpeg

# With custom frame count
python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --frames 200 --output output.mjpeg
```

### Shell Script Examples

```bash
chmod +x ffmpeg_rockchip_examples.sh

# Interactive mode
./ffmpeg_rockchip_examples.sh

# Run specific example
./ffmpeg_rockchip_examples.sh 1  # Generate and encode
./ffmpeg_rockchip_examples.sh 2  # CQP encoding
./ffmpeg_rockchip_examples.sh 3  # Quality encoding
./ffmpeg_rockchip_examples.sh 8  # Check available encoders
```

### Direct FFmpeg Commands

```bash
# Generate raw NV12 video and encode to MJPEG
ffmpeg -f lavfi -i testsrc=size=1920x1080:duration=10:rate=30 \
    -pix_fmt nv12 -f rawvideo raw.yuv

ffmpeg -f rawvideo -video_size 1920x1080 -pixel_format nv12 -framerate 30 \
    -i raw.yuv \
    -c:v mjpeg_rkmpp \
    -q:v 5 \
    output.mjpeg

# Transcode existing video to MJPEG
ffmpeg -i input.mp4 \
    -c:v mjpeg_rkmpp \
    -q:v 5 \
    output.mjpeg

# Encode with quality control
ffmpeg -i input.mp4 \
    -c:v mjpeg_rkmpp \
    -qp_init 10 \
    -qp_min 5 \
    -qp_max 20 \
    output.mjpeg
```

## NV12 Format

NV12 is a semi-planar YUV 4:2:0 format with the following structure:

- **Y plane:** Full resolution luminance data
- **UV plane:** Half resolution, interleaved chrominance data (U, V, U, V, ...)

For 1920×1080 resolution:
- Y plane: 1920 × 1080 = 2,073,600 bytes
- UV plane: 960 × 540 = 518,400 bytes
- Total: 2,592,000 bytes per frame (1.5 bytes/pixel)

## Encoder Options

### Quality Control

| Option | Range | Description |
|--------|-------|-------------|
| `-q:v` | 2-31 | Quality level (lower = better) |
| `-qp_init` | -1 to 99 | Initial QP value |
| `-qp_min` | -1 to 99 | Minimum QP value |
| `-qp_max` | -1 to 99 | Maximum QP value |

### Example: Different Quality Levels

```bash
# High quality (smaller file, slower encoding)
ffmpeg -i input.mp4 -c:v mjpeg_rkmpp -q:v 2 output_hq.mjpeg

# Medium quality
ffmpeg -i input.mp4 -c:v mjpeg_rkmpp -q:v 5 output_mq.mjpeg

# Low quality (larger file, faster encoding)
ffmpeg -i input.mp4 -c:v mjpeg_rkmpp -q:v 10 output_lq.mjpeg
```

## Troubleshooting

### Hardware encoder not found

Check if FFmpeg-Rockchip is properly installed:

```bash
ffmpeg -encoders | grep rkmpp
```

If no results, rebuild FFmpeg with Rockchip support.

### Permission denied errors

Add your user to required groups:

```bash
sudo usermod -a -G video,audio $USER
# Log out and back in
```

### Segmentation faults

Ensure MPP libraries are up-to-date:

```bash
cd ~/dev/rkmpp/build
make clean
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

## Performance Tips

1. **Use hardware encoder:** Always prefer `mjpeg_rkmpp` over software encoder
2. **Adjust quality:** Use `-q:v 5-10` for good balance between quality and performance
3. **Monitor resources:** Use `top` to monitor CPU and memory usage
4. **Batch processing:** Process multiple files in parallel for better throughput

## System Requirements

- Rockchip SoC (RK3588, RK3588S, RK3576, etc.)
- Linux kernel 5.10 or later
- At least 2GB RAM
- 2GB free disk space for compilation

## References

- [FFmpeg-Rockchip GitHub](https://github.com/nyanmisaka/ffmpeg-rockchip)
- [Rockchip MPP Documentation](https://opensource.rock-chips.com/wiki_Mpp)
- [FFmpeg Compilation Guide](https://trac.ffmpeg.org/wiki/CompilationGuide)
- [Jellyfin Rockchip Hardware Acceleration](https://jellyfin.org/docs/general/post-install/transcoding/hardware-acceleration/rockchip/)

## License

This guide and test code are provided for educational and development purposes. FFmpeg-Rockchip is licensed under GPL/LGPL. Please refer to the FFmpeg-Rockchip repository for detailed license information.

## Support

For issues with FFmpeg-Rockchip, please refer to:
- [FFmpeg-Rockchip Issues](https://github.com/nyanmisaka/ffmpeg-rockchip/issues)
- [Rockchip Forum](https://forum.rock-chips.com/)
- [Jellyfin Community](https://jellyfin.org/docs/)

## Author

Created with comprehensive research and testing for Rockchip video encoding systems.
