/*
 * NV12 ↔ MJPEG Codec Library Header
 * 
 * Hardware-accelerated encoding and decoding using Rockchip MPP
 * via FFmpeg mjpeg_rkmpp encoder/decoder.
 */

#ifndef NV12_MJPEG_CODEC_H
#define NV12_MJPEG_CODEC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Memory Management Functions
// ============================================================================

/**
 * Allocate NV12 buffer
 * 
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @return Pointer to allocated buffer, or NULL on failure
 * 
 * Note: Caller must free the buffer using free_nv12_buffer()
 */
uint8_t* alloc_nv12_buffer(int width, int height);

/**
 * Free NV12 buffer allocated by alloc_nv12_buffer()
 * 
 * @param buffer Pointer to buffer to free (can be NULL)
 */
void free_nv12_buffer(uint8_t* buffer);

// ============================================================================
// File I/O Functions
// ============================================================================

/**
 * Read single NV12 frame from YUV file
 * 
 * @param filename Input YUV file path
 * @param buffer Pre-allocated buffer (must be at least width*height*3/2 bytes)
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @return 0 on success, negative error code on failure
 */
int read_nv12_from_file(const char* filename, uint8_t* buffer, int width, int height);

/**
 * Write NV12 frame to YUV file
 * 
 * @param filename Output YUV file path
 * @param buffer NV12 frame data
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @return 0 on success, negative error code on failure
 */
int write_nv12_to_file(const char* filename, const uint8_t* buffer, int width, int height);

// ============================================================================
// Encoding Function: NV12 → MJPEG
// ============================================================================

/**
 * Encode single NV12 frame to MJPEG using Rockchip hardware encoder
 * 
 * Uses mjpeg_rkmpp encoder for hardware acceleration.
 * 
 * @param nv12_data Input NV12 frame data (must be width*height*3/2 bytes)
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param output_file Output MJPEG file path
 * @param quality Quality parameter (1-31, lower is better quality)
 *                1-5:   Extremely high quality (3-10:1 compression)
 *                6-12:  High quality (10-20:1 compression)
 *                13-20: Medium quality (20-40:1 compression)
 *                21-31: Low quality (40-100:1 compression)
 *                Recommended: 2 for industrial/medical applications
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 *   -1: Encoder not found
 *   -2: Failed to allocate codec context
 *   -3: Invalid quality parameter
 *   <0: FFmpeg error code (use av_err2str() to decode)
 */
int encode_nv12_to_mjpeg(const uint8_t* nv12_data, int width, int height, const char* output_file, int quality);

// ============================================================================
// Decoding Function: MJPEG → NV12
// ============================================================================

/**
 * Decode single MJPEG frame to NV12 using Rockchip hardware decoder
 * 
 * Uses mjpeg_rkmpp decoder for hardware acceleration.
 * 
 * @param input_file Input MJPEG file path
 * @param nv12_data Output buffer (must be pre-allocated with sufficient size)
 * @param width Pointer to store decoded frame width
 * @param height Pointer to store decoded frame height
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 *   -1: Decoder not found or stream not found
 *   -2: Failed to allocate codec context
 *   <0: FFmpeg error code (use av_err2str() to decode)
 * 
 * Note: nv12_data buffer must be large enough to hold width*height*3/2 bytes
 */
int decode_mjpeg_to_nv12(const char* input_file, uint8_t* nv12_data, int* width, int* height);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get high-resolution timestamp in nanoseconds
 * 
 * Uses CLOCK_MONOTONIC for precise timing measurements.
 * 
 * @return Current time in nanoseconds
 */
uint64_t get_time_ns(void);

/**
 * Get file size in bytes
 * 
 * @param filename File path to check
 * @return File size in bytes, or -1 on error
 */
int64_t get_file_size(const char* filename);

/**
 * Calculate NV12 frame size in bytes
 * 
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @return Frame size in bytes (width * height * 3 / 2)
 */
static inline size_t nv12_frame_size(int width, int height) {
    return (size_t)width * (size_t)height * 3 / 2;
}

#ifdef __cplusplus
}
#endif

#endif // NV12_MJPEG_CODEC_H
