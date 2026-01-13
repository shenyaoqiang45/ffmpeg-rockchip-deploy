/*
 * FFmpeg-Rockchip NV12 ↔ MJPEG Codec Benchmark
 * 
 * This program benchmarks hardware-accelerated encoding (NV12 → MJPEG)
 * and decoding (MJPEG → NV12) using Rockchip MPP via FFmpeg.
 * 
 * Resolution: 1600×1200
 * Input: test_data/video22_1.yuv (single frame)
 * 
 * Compilation:
 *   make codec_benchmark
 * 
 * Usage:
 *   ./codec_benchmark
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "nv12_mjpeg_codec.h"

// Constants
#define WIDTH 1600
#define HEIGHT 1200
#define INPUT_YUV_FILE "test_data/video22_1.yuv"
#define OUTPUT_MJPEG_FILE "output_test.mjpeg"
#define OUTPUT_DECODED_YUV_FILE "output_decoded.yuv"

// ============================================================================
// Main Function
// ============================================================================

int main(void) {
    int ret;
    uint64_t start_time, end_time;
    double encode_time_ms, decode_time_ms;
    
    printf("=================================================================\n");
    printf("FFmpeg-Rockchip NV12 ↔ MJPEG Codec Benchmark\n");
    printf("=================================================================\n");
    printf("Resolution: %dx%d\n", WIDTH, HEIGHT);
    printf("Input YUV:  %s\n", INPUT_YUV_FILE);
    printf("Output MJPEG: %s\n", OUTPUT_MJPEG_FILE);
    printf("Output Decoded YUV: %s\n", OUTPUT_DECODED_YUV_FILE);
    printf("=================================================================\n\n");
    
    // ========================================================================
    // Step 1: Allocate buffers
    // ========================================================================
    
    printf("[1/5] Allocating buffers...\n");
    
    uint8_t* input_nv12 = alloc_nv12_buffer(WIDTH, HEIGHT);
    uint8_t* decoded_nv12 = alloc_nv12_buffer(WIDTH, HEIGHT);
    
    if (!input_nv12 || !decoded_nv12) {
        fprintf(stderr, "Failed to allocate buffers\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    printf("  ✓ Allocated %zu bytes for each buffer\n\n", nv12_frame_size(WIDTH, HEIGHT));
    
    // ========================================================================
    // Step 2: Read input NV12 frame
    // ========================================================================
    
    printf("[2/5] Reading input NV12 frame...\n");
    
    ret = read_nv12_from_file(INPUT_YUV_FILE, input_nv12, WIDTH, HEIGHT);
    if (ret < 0) {
        fprintf(stderr, "Failed to read input YUV file\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    printf("  ✓ Read %zu bytes from %s\n\n", nv12_frame_size(WIDTH, HEIGHT), INPUT_YUV_FILE);
    
    // ========================================================================
    // Step 3: Encode NV12 → MJPEG
    // ========================================================================
    
    printf("[3/5] Encoding NV12 → MJPEG (hardware encoder: mjpeg_rkmpp)...\n");
    
    start_time = get_time_ns();
    ret = encode_nv12_to_mjpeg(input_nv12, WIDTH, HEIGHT, OUTPUT_MJPEG_FILE);
    end_time = get_time_ns();
    
    if (ret < 0) {
        fprintf(stderr, "Failed to encode NV12 to MJPEG\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    encode_time_ms = (double)(end_time - start_time) / 1000000.0;
    int64_t mjpeg_size = get_file_size(OUTPUT_MJPEG_FILE);
    
    printf("  ✓ Encoding completed\n");
    printf("    - Time: %.3f ms\n", encode_time_ms);
    printf("    - Output size: %lld bytes\n\n", (long long)mjpeg_size);
    
    // ========================================================================
    // Step 4: Decode MJPEG → NV12
    // ========================================================================
    
    printf("[4/5] Decoding MJPEG → NV12 (hardware decoder: mjpeg_rkmpp)...\n");
    
    int decoded_width = 0, decoded_height = 0;
    
    start_time = get_time_ns();
    ret = decode_mjpeg_to_nv12(OUTPUT_MJPEG_FILE, decoded_nv12, &decoded_width, &decoded_height);
    end_time = get_time_ns();
    
    if (ret < 0) {
        fprintf(stderr, "Failed to decode MJPEG to NV12\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    decode_time_ms = (double)(end_time - start_time) / 1000000.0;
    
    printf("  ✓ Decoding completed\n");
    printf("    - Time: %.3f ms\n", decode_time_ms);
    printf("    - Decoded resolution: %dx%d\n\n", decoded_width, decoded_height);
    
    // Write decoded NV12 to file for verification
    ret = write_nv12_to_file(OUTPUT_DECODED_YUV_FILE, decoded_nv12, decoded_width, decoded_height);
    if (ret < 0) {
        fprintf(stderr, "Warning: Failed to write decoded NV12 to file\n");
    }
    
    // ========================================================================
    // Step 5: Performance Statistics
    // ========================================================================
    
    printf("[5/5] Performance Statistics:\n");
    printf("=================================================================\n");
    
    size_t nv12_size = nv12_frame_size(WIDTH, HEIGHT);
    double compression_ratio = (double)nv12_size / (double)mjpeg_size;
    double encode_fps = 1000.0 / encode_time_ms;
    double decode_fps = 1000.0 / decode_time_ms;
    
    printf("Encoding:\n");
    printf("  - Time:        %.3f ms\n", encode_time_ms);
    printf("  - Throughput:  %.2f FPS\n", encode_fps);
    printf("  - Input size:  %zu bytes (NV12)\n", nv12_size);
    printf("  - Output size: %lld bytes (MJPEG)\n", (long long)mjpeg_size);
    printf("\n");
    
    printf("Decoding:\n");
    printf("  - Time:        %.3f ms\n", decode_time_ms);
    printf("  - Throughput:  %.2f FPS\n", decode_fps);
    printf("\n");
    
    printf("Compression:\n");
    printf("  - Ratio:       %.2f:1 (%.2f%%)\n", compression_ratio, 100.0 / compression_ratio);
    printf("\n");
    
    printf("Total round-trip time: %.3f ms\n", encode_time_ms + decode_time_ms);
    printf("=================================================================\n");
    
    // ========================================================================
    // Cleanup
    // ========================================================================
    
    free_nv12_buffer(input_nv12);
    free_nv12_buffer(decoded_nv12);
    
    printf("\n✓ Benchmark completed successfully\n");
    
    return 0;
}
