/*
 * FFmpeg-Rockchip NV12 ↔ MJPEG Codec Benchmark
 * 
 * This program benchmarks hardware-accelerated encoding (NV12 → MJPEG)
 * and decoding (MJPEG → NV12) using Rockchip MPP via FFmpeg.
 * 
 * Tests both single-frame and multi-frame continuous encoding to demonstrate
 * the performance benefits of the new persistent context API.
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
#include <string.h>

#include "nv12_mjpeg_codec.h"

// Constants
#define WIDTH 1600
#define HEIGHT 1200
#define ENCODE_QUALITY 2  // QP=2 for high quality (1-31, lower is better)
#define INPUT_YUV_FILE "test_data/video22_1.yuv"
#define OUTPUT_MJPEG_FILE "output_test.mjpeg"
#define OUTPUT_DECODED_YUV_FILE "output_decoded.yuv"
#define CONTINUOUS_FRAMES 100  // Number of frames for continuous encoding test

// ============================================================================
// Main Function
// ============================================================================

int main(void) {
    int ret;
    uint64_t start_time, end_time;
    double encode_time_ms, decode_time_ms;
    size_t mjpeg_size;
    
    printf("=================================================================\n");
    printf("FFmpeg-Rockchip NV12 ↔ MJPEG Codec Benchmark (New Memory API)\n");
    printf("=================================================================\n");
    printf("Resolution: %dx%d\n", WIDTH, HEIGHT);
    printf("Input YUV:  %s\n", INPUT_YUV_FILE);
    printf("Output Decoded YUV: %s\n", OUTPUT_DECODED_YUV_FILE);
    printf("Quality: QP=%d\n", ENCODE_QUALITY);
    printf("=================================================================\n\n");
    
    // ========================================================================
    // Step 1: Allocate buffers
    // ========================================================================
    
    printf("[1/6] Allocating buffers...\n");
    
    uint8_t* input_nv12 = alloc_nv12_buffer(WIDTH, HEIGHT);
    uint8_t* decoded_nv12 = alloc_nv12_buffer(WIDTH, HEIGHT);
    
    if (!input_nv12 || !decoded_nv12) {
        fprintf(stderr, "Failed to allocate NV12 buffers\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    printf("  ✓ Allocated %zu bytes for each NV12 buffer\n\n", nv12_frame_size(WIDTH, HEIGHT));
    
    // ========================================================================
    // Step 2: Read input NV12 frame
    // ========================================================================
    
    printf("[2/6] Reading input NV12 frame...\n");
    
    ret = read_nv12_from_file(INPUT_YUV_FILE, input_nv12, WIDTH, HEIGHT);
    if (ret < 0) {
        fprintf(stderr, "Failed to read input YUV file\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    printf("  ✓ Read %zu bytes from %s\n\n", nv12_frame_size(WIDTH, HEIGHT), INPUT_YUV_FILE);
    
    // ========================================================================
    // Step 3: Create encoder and decoder contexts
    // ========================================================================
    
    printf("[3/6] Creating encoder and decoder contexts...\n");
    
    NV12MJPEGEncoder* encoder = encoder_create(WIDTH, HEIGHT, ENCODE_QUALITY);
    if (!encoder) {
        fprintf(stderr, "Failed to create encoder\n");
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    printf("  ✓ Encoder created (mjpeg_rkmpp, %dx%d, QP=%d)\n", WIDTH, HEIGHT, ENCODE_QUALITY);
    
    NV12MJPEGDecoder* decoder = decoder_create();
    if (!decoder) {
        fprintf(stderr, "Failed to create decoder\n");
        encoder_destroy(encoder);
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    printf("  ✓ Decoder created (mjpeg_rkmpp)\n\n");
    
    // Allocate MJPEG buffer (in memory, not saved to file)
    size_t mjpeg_buffer_size = encoder_max_output_size(encoder);
    uint8_t* mjpeg_buffer = (uint8_t*)malloc(mjpeg_buffer_size);
    if (!mjpeg_buffer) {
        fprintf(stderr, "Failed to allocate MJPEG buffer\n");
        decoder_destroy(decoder);
        encoder_destroy(encoder);
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    printf("  ✓ Allocated %zu bytes for MJPEG buffer (in memory)\n\n", mjpeg_buffer_size);
    
    // ========================================================================
    // Step 4: Single-frame encode test
    // ========================================================================
    
    printf("[4/6] Single-frame encoding test (NV12 → MJPEG)...\n");
    
    start_time = get_time_ns();
    ret = encoder_encode_to_buffer(encoder, input_nv12, mjpeg_buffer, mjpeg_buffer_size, &mjpeg_size);
    end_time = get_time_ns();
    
    if (ret < 0) {
        fprintf(stderr, "Failed to encode NV12 to MJPEG\n");
        free(mjpeg_buffer);
        decoder_destroy(decoder);
        encoder_destroy(encoder);
        free_nv12_buffer(input_nv12);
        free_nv12_buffer(decoded_nv12);
        return 1;
    }
    
    encode_time_ms = (double)(end_time - start_time) / 1000000.0;
    
    printf("  ✓ Encoding completed\n");
    printf("    - Time: %.3f ms\n", encode_time_ms);
    printf("    - Output size: %zu bytes (MJPEG in memory)\n\n", mjpeg_size);
    
    // ========================================================================
    // Step 5: Single-frame decode test
    // ========================================================================
    
    printf("[5/6] Single-frame decoding test (MJPEG → NV12)...\n");
    
    int decoded_width = 0, decoded_height = 0;
    
    start_time = get_time_ns();
    ret = decoder_decode_from_buffer(decoder, mjpeg_buffer, mjpeg_size, 
                                      decoded_nv12, nv12_frame_size(WIDTH, HEIGHT),
                                      &decoded_width, &decoded_height);
    end_time = get_time_ns();
    
    if (ret < 0) {
        fprintf(stderr, "Failed to decode MJPEG to NV12\n");
        free(mjpeg_buffer);
        decoder_destroy(decoder);
        encoder_destroy(encoder);
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
    } else {
        printf("  ✓ Saved decoded NV12 to %s for verification\n\n", OUTPUT_DECODED_YUV_FILE);
    }
    
    // Data comparison
    size_t nv12_size = nv12_frame_size(WIDTH, HEIGHT);
    size_t diff_count = 0;
    size_t max_diff = 0;
    for (size_t i = 0; i < nv12_size; i++) {
        int diff = abs((int)input_nv12[i] - (int)decoded_nv12[i]);
        if (diff > 0) {
            diff_count++;
            if (diff > max_diff) max_diff = diff;
        }
    }
    
    printf("  Data comparison (input vs decoded):\n");
    printf("    - Different pixels: %zu / %zu (%.2f%%)\n", diff_count, nv12_size, 
           100.0 * diff_count / nv12_size);
    printf("    - Max difference: %zu\n\n", max_diff);
    
    // ========================================================================
    // Step 6: Multi-frame continuous encoding test
    // ========================================================================
    
    printf("[6/6] Multi-frame continuous encoding test (%d frames)...\n", CONTINUOUS_FRAMES);
    
    uint64_t total_encode_time = 0;
    uint64_t total_decode_time = 0;
    
    for (int i = 0; i < CONTINUOUS_FRAMES; i++) {
        // Encode
        start_time = get_time_ns();
        ret = encoder_encode_to_buffer(encoder, input_nv12, mjpeg_buffer, mjpeg_buffer_size, &mjpeg_size);
        end_time = get_time_ns();
        
        if (ret < 0) {
            fprintf(stderr, "Failed to encode frame %d\n", i);
            break;
        }
        total_encode_time += (end_time - start_time);
        
        // Decode
        start_time = get_time_ns();
        ret = decoder_decode_from_buffer(decoder, mjpeg_buffer, mjpeg_size,
                                          decoded_nv12, nv12_frame_size(WIDTH, HEIGHT),
                                          &decoded_width, &decoded_height);
        end_time = get_time_ns();
        
        if (ret < 0) {
            fprintf(stderr, "Failed to decode frame %d\n", i);
            break;
        }
        total_decode_time += (end_time - start_time);
    }
    
    double avg_encode_ms = (double)total_encode_time / CONTINUOUS_FRAMES / 1000000.0;
    double avg_decode_ms = (double)total_decode_time / CONTINUOUS_FRAMES / 1000000.0;
    
    printf("  ✓ Continuous encoding/decoding completed\n");
    printf("    - Average encode time: %.3f ms (%.2f FPS)\n", avg_encode_ms, 1000.0 / avg_encode_ms);
    printf("    - Average decode time: %.3f ms (%.2f FPS)\n\n", avg_decode_ms, 1000.0 / avg_decode_ms);
    
    // ========================================================================
    // Performance Statistics
    // ========================================================================
    
    printf("=================================================================\n");
    printf("Performance Statistics:\n");
    printf("=================================================================\n");
    
    double compression_ratio = (double)nv12_size / (double)mjpeg_size;
    
    printf("Single Frame:\n");
    printf("  Encoding:\n");
    printf("    - Time:        %.3f ms\n", encode_time_ms);
    printf("    - Throughput:  %.2f FPS\n", 1000.0 / encode_time_ms);
    printf("  Decoding:\n");
    printf("    - Time:        %.3f ms\n", decode_time_ms);
    printf("    - Throughput:  %.2f FPS\n", 1000.0 / decode_time_ms);
    printf("  Round-trip:      %.3f ms\n", encode_time_ms + decode_time_ms);
    printf("\n");
    
    printf("Continuous (%d frames):\n", CONTINUOUS_FRAMES);
    printf("  Encoding:\n");
    printf("    - Average time: %.3f ms\n", avg_encode_ms);
    printf("    - Throughput:   %.2f FPS\n", 1000.0 / avg_encode_ms);
    printf("  Decoding:\n");
    printf("    - Average time: %.3f ms\n", avg_decode_ms);
    printf("    - Throughput:   %.2f FPS\n", 1000.0 / avg_decode_ms);
    printf("  Round-trip:       %.3f ms\n", avg_encode_ms + avg_decode_ms);
    printf("\n");
    
    printf("Compression:\n");
    printf("  - Input size:  %zu bytes (NV12)\n", nv12_size);
    printf("  - Output size: %zu bytes (MJPEG)\n", mjpeg_size);
    printf("  - Ratio:       %.2f:1 (%.2f%% of original)\n", 
           compression_ratio, 100.0 / compression_ratio);
    printf("=================================================================\n");
    
    // ========================================================================
    // Cleanup
    // ========================================================================
    
    free(mjpeg_buffer);
    decoder_destroy(decoder);
    encoder_destroy(encoder);
    free_nv12_buffer(input_nv12);
    free_nv12_buffer(decoded_nv12);
    
    printf("\n✓ Benchmark completed successfully\n");
    
    return 0;
}
