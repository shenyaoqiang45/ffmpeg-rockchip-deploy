#!/bin/bash

# FFmpeg-Rockchip MJPEG Encoding Examples
# This script provides various examples of encoding NV12 video to MJPEG
# using the Rockchip VEPU hardware accelerator

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
print_header() {
    echo -e "${GREEN}=== $1 ===${NC}"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Example 1: Generate raw NV12 video and encode to MJPEG
example_generate_and_encode() {
    print_header "Example 1: Generate and Encode NV12 to MJPEG"
    
    local width=1920
    local height=1080
    local fps=30
    local duration=10
    local output="output_example1.mjpeg"
    
    print_info "Generating raw NV12 video (${width}x${height}, ${fps}fps, ${duration}s)"
    
    # Generate raw NV12 video using ffmpeg
    ffmpeg -f lavfi -i testsrc=size=${width}x${height}:duration=${duration}:rate=${fps} \
           -pix_fmt nv12 -f rawvideo raw_input.yuv
    
    print_info "Encoding NV12 to MJPEG using hardware encoder"
    
    # Encode raw NV12 to MJPEG using hardware encoder
    ffmpeg -f rawvideo -video_size ${width}x${height} -pixel_format nv12 -framerate ${fps} \
           -i raw_input.yuv \
           -c:v mjpeg_rkmpp \
           -q:v 5 \
           -y ${output}
    
    print_info "Output saved to: ${output}"
    print_info "File size: $(du -h ${output} | cut -f1)"
}

# Example 2: Encode with CQP (Constant Quantization Parameter) rate control
example_cqp_encoding() {
    print_header "Example 2: MJPEG Encoding with CQP Rate Control"
    
    local width=1280
    local height=720
    local fps=24
    local output="output_cqp.mjpeg"
    
    print_info "Generating test video"
    ffmpeg -f lavfi -i testsrc=size=${width}x${height}:duration=5:rate=${fps} \
           -pix_fmt nv12 -f rawvideo raw_input_cqp.yuv
    
    print_info "Encoding with CQP (QP=10)"
    ffmpeg -f rawvideo -video_size ${width}x${height} -pixel_format nv12 -framerate ${fps} \
           -i raw_input_cqp.yuv \
           -c:v mjpeg_rkmpp \
           -qp_init 10 \
           -y ${output}
    
    print_info "Output saved to: ${output}"
}

# Example 3: Encode with quality parameter
example_quality_encoding() {
    print_header "Example 3: MJPEG Encoding with Quality Parameter"
    
    local width=1920
    local height=1080
    local fps=30
    local output="output_quality.mjpeg"
    
    print_info "Generating test video"
    ffmpeg -f lavfi -i testsrc=size=${width}x${height}:duration=5:rate=${fps} \
           -pix_fmt nv12 -f rawvideo raw_input_quality.yuv
    
    print_info "Encoding with quality setting (q:v=3, high quality)"
    ffmpeg -f rawvideo -video_size ${width}x${height} -pixel_format nv12 -framerate ${fps} \
           -i raw_input_quality.yuv \
           -c:v mjpeg_rkmpp \
           -q:v 3 \
           -y ${output}
    
    print_info "Output saved to: ${output}"
}

# Example 4: Encode from video file
example_transcode_to_mjpeg() {
    print_header "Example 4: Transcode Video to MJPEG"
    
    local input_file="input_video.mp4"
    local output="output_transcoded.mjpeg"
    
    # Check if input file exists
    if [ ! -f "${input_file}" ]; then
        print_info "Creating sample input video"
        ffmpeg -f lavfi -i testsrc=duration=5:rate=30 \
               -f lavfi -i sine=f=440:d=5 \
               -pix_fmt yuv420p \
               -y ${input_file}
    fi
    
    print_info "Transcoding ${input_file} to MJPEG"
    ffmpeg -i ${input_file} \
           -c:v mjpeg_rkmpp \
           -q:v 5 \
           -y ${output}
    
    print_info "Output saved to: ${output}"
}

# Example 5: Batch encoding with different quality levels
example_batch_encoding() {
    print_header "Example 5: Batch Encoding with Different Quality Levels"
    
    local width=1280
    local height=720
    local fps=25
    local input_yuv="raw_input_batch.yuv"
    
    print_info "Generating test video"
    ffmpeg -f lavfi -i testsrc=size=${width}x${height}:duration=3:rate=${fps} \
           -pix_fmt nv12 -f rawvideo ${input_yuv}
    
    # Encode with different quality levels
    for qp in 5 10 15 20; do
        local output="output_qp${qp}.mjpeg"
        print_info "Encoding with QP=${qp}"
        
        ffmpeg -f rawvideo -video_size ${width}x${height} -pixel_format nv12 -framerate ${fps} \
               -i ${input_yuv} \
               -c:v mjpeg_rkmpp \
               -qp_init ${qp} \
               -y ${output}
        
        local size=$(du -h ${output} | cut -f1)
        print_info "Output: ${output} (${size})"
    done
}

# Example 6: Real-time encoding from camera
example_camera_encoding() {
    print_header "Example 6: Real-time Camera Encoding to MJPEG"
    
    local output="output_camera.mjpeg"
    local duration=10
    
    print_info "Encoding from camera for ${duration} seconds"
    print_info "Note: This requires a connected camera device"
    
    # Capture from /dev/video0 and encode to MJPEG
    ffmpeg -f v4l2 -video_size 1920x1080 -pixel_format nv12 -framerate 30 \
           -i /dev/video0 \
           -t ${duration} \
           -c:v mjpeg_rkmpp \
           -q:v 5 \
           -y ${output} 2>/dev/null || {
        print_error "Camera device not found or not accessible"
        print_info "Make sure /dev/video0 exists and you have permission to access it"
    }
}

# Example 7: Encoding with RGA scaling
example_with_scaling() {
    print_header "Example 7: Encoding with RGA Scaling"
    
    local width=3840
    local height=2160
    local output_width=1920
    local output_height=1080
    local fps=30
    local output="output_scaled.mjpeg"
    
    print_info "Generating 4K test video"
    ffmpeg -f lavfi -i testsrc=size=${width}x${height}:duration=5:rate=${fps} \
           -pix_fmt nv12 -f rawvideo raw_input_4k.yuv
    
    print_info "Encoding with scaling: ${width}x${height} -> ${output_width}x${output_height}"
    ffmpeg -f rawvideo -video_size ${width}x${height} -pixel_format nv12 -framerate ${fps} \
           -i raw_input_4k.yuv \
           -vf "scale_rkrga=${output_width}:${output_height}" \
           -c:v mjpeg_rkmpp \
           -q:v 5 \
           -y ${output}
    
    print_info "Output saved to: ${output}"
}

# Example 8: Check available encoders
example_check_encoders() {
    print_header "Example 8: Check Available Encoders"
    
    print_info "Available MJPEG encoders:"
    ffmpeg -hide_banner -encoders | grep mjpeg
    
    print_info "Detailed MJPEG hardware encoder options:"
    ffmpeg -hide_banner -h encoder=mjpeg_rkmpp 2>/dev/null || {
        print_error "Hardware MJPEG encoder not available"
        print_info "Available MJPEG encoder options:"
        ffmpeg -hide_banner -h encoder=mjpeg
    }
}

# Main menu
show_menu() {
    echo ""
    echo "FFmpeg-Rockchip MJPEG Encoding Examples"
    echo "========================================"
    echo "1. Generate and encode NV12 to MJPEG"
    echo "2. Encode with CQP rate control"
    echo "3. Encode with quality parameter"
    echo "4. Transcode video file to MJPEG"
    echo "5. Batch encoding with different quality levels"
    echo "6. Real-time camera encoding"
    echo "7. Encoding with RGA scaling"
    echo "8. Check available encoders"
    echo "0. Exit"
    echo ""
}

# Main script
main() {
    if [ $# -eq 0 ]; then
        # Interactive mode
        while true; do
            show_menu
            read -p "Select an example (0-8): " choice
            
            case $choice in
                1) example_generate_and_encode ;;
                2) example_cqp_encoding ;;
                3) example_quality_encoding ;;
                4) example_transcode_to_mjpeg ;;
                5) example_batch_encoding ;;
                6) example_camera_encoding ;;
                7) example_with_scaling ;;
                8) example_check_encoders ;;
                0) print_info "Exiting"; exit 0 ;;
                *) print_error "Invalid option" ;;
            esac
            
            read -p "Press Enter to continue..."
        done
    else
        # Command-line mode
        case $1 in
            1) example_generate_and_encode ;;
            2) example_cqp_encoding ;;
            3) example_quality_encoding ;;
            4) example_transcode_to_mjpeg ;;
            5) example_batch_encoding ;;
            6) example_camera_encoding ;;
            7) example_with_scaling ;;
            8) example_check_encoders ;;
            *) print_error "Invalid example number"; exit 1 ;;
        esac
    fi
}

main "$@"
