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
// Persistent Encoder Context (New API for Resident Services)
// ============================================================================

/**
 * Opaque encoder context for persistent hardware encoding sessions
 * 
 * Create once with encoder_create(), reuse for multiple frames, destroy with encoder_destroy().
 * This eliminates per-frame codec initialization overhead (~30-50% performance improvement).
 */
typedef struct NV12MJPEGEncoder NV12MJPEGEncoder;

/**
 * Create persistent MJPEG encoder with pre-allocated resources
 * 
 * Initializes Rockchip hardware encoder and allocates all buffers upfront.
 * The encoder is bound to specific width/height/quality parameters.
 * To change parameters, destroy and recreate the encoder.
 * 
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param quality Quality parameter (1-31, lower is better quality)
 * @return Encoder context, or NULL on failure
 * 
 * Note: Not thread-safe. Each thread needs its own encoder instance.
 */
NV12MJPEGEncoder* encoder_create(int width, int height, int quality);

/**
 * Encode NV12 frame to MJPEG in user-provided buffer
 * 
 * @param encoder Encoder context from encoder_create()
 * @param nv12_data Input NV12 frame data (width*height*3/2 bytes)
 * @param out_buffer Output buffer (pre-allocated by user)
 * @param buffer_size Size of output buffer in bytes
 * @param out_size Pointer to store actual encoded size
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 *   -EINVAL: Invalid parameters
 *   -ENOMEM: Output buffer too small (check *out_size for required size)
 *   <0: FFmpeg error code
 */
int encoder_encode_to_buffer(NV12MJPEGEncoder* encoder, const uint8_t* nv12_data,
                              uint8_t* out_buffer, size_t buffer_size, size_t* out_size);

/**
 * Get maximum possible output size for encoded MJPEG frame
 * 
 * Use this to allocate output buffer for encoder_encode_to_buffer().
 * Returns conservative upper bound (actual size is usually much smaller).
 * 
 * @param encoder Encoder context
 * @return Maximum output size in bytes
 */
size_t encoder_max_output_size(const NV12MJPEGEncoder* encoder);

/**
 * Destroy encoder and free all resources
 * 
 * @param encoder Encoder context (can be NULL)
 */
void encoder_destroy(NV12MJPEGEncoder* encoder);

// ============================================================================
// Persistent Decoder Context (New API for Resident Services)
// ============================================================================

/**
 * Opaque decoder context for persistent hardware decoding sessions
 * 
 * Create once with decoder_create(), reuse for multiple frames, destroy with decoder_destroy().
 * Decoder can handle varying resolutions automatically.
 */
typedef struct NV12MJPEGDecoder NV12MJPEGDecoder;

/**
 * Create persistent MJPEG decoder with pre-allocated resources
 * 
 * Initializes Rockchip hardware decoder. Decoder adapts to input resolution automatically.
 * 
 * @return Decoder context, or NULL on failure
 * 
 * Note: Not thread-safe. Each thread needs its own decoder instance.
 */
NV12MJPEGDecoder* decoder_create(void);

/**
 * Decode MJPEG frame to NV12 in user-provided buffer
 * 
 * @param decoder Decoder context from decoder_create()
 * @param mjpeg_data Input MJPEG compressed data
 * @param mjpeg_size Size of MJPEG data in bytes
 * @param out_nv12_buffer Output buffer (pre-allocated by user, must be large enough)
 * @param buffer_size Size of output buffer in bytes
 * @param out_width Pointer to store decoded frame width
 * @param out_height Pointer to store decoded frame height
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 *   -EINVAL: Invalid parameters
 *   -ENOMEM: Output buffer too small (need width*height*3/2 bytes)
 *   <0: FFmpeg error code
 * 
 * Note: Ensure output buffer is at least 1920*1080*3/2 bytes for typical use cases
 */
int decoder_decode_from_buffer(NV12MJPEGDecoder* decoder, const uint8_t* mjpeg_data, size_t mjpeg_size,
                                uint8_t* out_nv12_buffer, size_t buffer_size,
                                int* out_width, int* out_height);

/**
 * Destroy decoder and free all resources
 * 
 * @param decoder Decoder context (can be NULL)
 */
void decoder_destroy(NV12MJPEGDecoder* decoder);

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
