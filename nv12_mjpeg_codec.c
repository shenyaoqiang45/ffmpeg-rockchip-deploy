/*
 * NV12 ↔ MJPEG Codec Library Implementation
 * 
 * Hardware-accelerated encoding and decoding using Rockchip MPP
 * via FFmpeg mjpeg_rkmpp encoder/decoder.
 */

#include "nv12_mjpeg_codec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/hwcontext.h>
#include <errno.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// ============================================================================
// Persistent Encoder Context Implementation
// ============================================================================

struct NV12MJPEGEncoder {
    const AVCodec* codec;         // Cached codec pointer
    AVCodecContext* codec_ctx;    // Hardware encoder context (persistent)
    AVFrame* frame;               // Pre-allocated frame with buffers
    AVPacket* pkt;                // Pre-allocated packet
    int width;                    // Configured width
    int height;                   // Configured height
    int quality;                  // Configured quality
    int64_t frame_counter;        // Frame counter for PTS
};

NV12MJPEGEncoder* encoder_create(int width, int height, int quality) {
    int ret;
    
    // Validate parameters
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "Invalid dimensions: %dx%d\n", width, height);
        return NULL;
    }
    if (quality < 1 || quality > 31) {
        fprintf(stderr, "Invalid quality: %d (must be 1-31)\n", quality);
        return NULL;
    }
    
    // Allocate encoder context
    NV12MJPEGEncoder* encoder = (NV12MJPEGEncoder*)calloc(1, sizeof(NV12MJPEGEncoder));
    if (!encoder) {
        fprintf(stderr, "Failed to allocate encoder context\n");
        return NULL;
    }
    
    // Store parameters
    encoder->width = width;
    encoder->height = height;
    encoder->quality = quality;
    encoder->frame_counter = 0;
    
    // Find hardware MJPEG encoder
    encoder->codec = avcodec_find_encoder_by_name("mjpeg_rkmpp");
    if (!encoder->codec) {
        fprintf(stderr, "Hardware MJPEG encoder (mjpeg_rkmpp) not found\n");
        free(encoder);
        return NULL;
    }
    
    // Allocate codec context
    encoder->codec_ctx = avcodec_alloc_context3(encoder->codec);
    if (!encoder->codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        free(encoder);
        return NULL;
    }
    
    // Configure codec parameters
    encoder->codec_ctx->width = width;
    encoder->codec_ctx->height = height;
    encoder->codec_ctx->pix_fmt = AV_PIX_FMT_NV12;
    encoder->codec_ctx->time_base = (AVRational){1, 30};
    encoder->codec_ctx->framerate = (AVRational){30, 1};
    encoder->codec_ctx->gop_size = 1;  // Every frame is a keyframe (required for MJPEG)
    encoder->codec_ctx->max_b_frames = 0;  // No B-frames for MJPEG
    
    // Set quality control parameters - use fixed QP mode
    // 1. Set flag to tell encoder to use fixed quantization parameters
    encoder->codec_ctx->flags |= AV_CODEC_FLAG_QSCALE;
    
    // 2. Set global_quality (critical for quality control)
    // In FFmpeg, global_quality uses lambda units
    encoder->codec_ctx->global_quality = quality * FF_QP2LAMBDA;
    
    // Print quality parameters for debugging
    fprintf(stderr, "[Encoder Config] FF_QP2LAMBDA constant: %d\n", FF_QP2LAMBDA);
    fprintf(stderr, "[Encoder Config] Quality (QP): %d\n", quality);
    fprintf(stderr, "[Encoder Config] global_quality: %d (QP * FF_QP2LAMBDA = %d * %d)\n", 
            encoder->codec_ctx->global_quality, quality, FF_QP2LAMBDA);
    
    // 3. Very important: for some versions of RKMPP, must manually disable rate control
    encoder->codec_ctx->bit_rate = 0;
    encoder->codec_ctx->rc_max_rate = 0;
    encoder->codec_ctx->rc_buffer_size = 0;
    
    fprintf(stderr, "[Encoder Config] Rate control: bit_rate=0, rc_max_rate=0, rc_buffer_size=0\n");
    
    // Set quality bounds to match the fixed QP
    encoder->codec_ctx->qmin = quality;
    encoder->codec_ctx->qmax = quality;
    
    fprintf(stderr, "[Encoder Config] Quality bounds: qmin=%d, qmax=%d\n", 
            encoder->codec_ctx->qmin, encoder->codec_ctx->qmax);
    
    // Set hardware encoder options for Rockchip MPP
    av_opt_set_int(encoder->codec_ctx->priv_data, "qp_init", quality, 0);
    av_opt_set_int(encoder->codec_ctx->priv_data, "qp_min", quality, 0);
    av_opt_set_int(encoder->codec_ctx->priv_data, "qp_max", quality, 0);
    
    fprintf(stderr, "[Encoder Config] Hardware encoder options: qp_init=%d, qp_min=%d, qp_max=%d\n",
            quality, quality, quality);
    
    // Open codec (expensive operation - done once)
    ret = avcodec_open2(encoder->codec_ctx, encoder->codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(ret));
        avcodec_free_context(&encoder->codec_ctx);
        free(encoder);
        return NULL;
    }
    
    // Print actual effective settings after codec is opened
    if (ret >= 0) {
        fprintf(stderr, "[Encoder Config] === After avcodec_open2() ===\n");
        fprintf(stderr, "[Encoder Config] Actual effective bit_rate: %ld bps\n", encoder->codec_ctx->bit_rate);
        fprintf(stderr, "[Encoder Config] Actual effective global_quality: %d\n", encoder->codec_ctx->global_quality);
        fprintf(stderr, "[Encoder Config] Actual effective QScale: %d (global_quality / FF_QP2LAMBDA = %d / %d)\n", 
                encoder->codec_ctx->global_quality / FF_QP2LAMBDA, 
                encoder->codec_ctx->global_quality, FF_QP2LAMBDA);
        fprintf(stderr, "[Encoder Config] Actual effective qmin: %d, qmax: %d\n",
                encoder->codec_ctx->qmin, encoder->codec_ctx->qmax);
    }
    
    // Allocate frame
    encoder->frame = av_frame_alloc();
    if (!encoder->frame) {
        fprintf(stderr, "Failed to allocate frame\n");
        avcodec_free_context(&encoder->codec_ctx);
        free(encoder);
        return NULL;
    }
    
    encoder->frame->format = AV_PIX_FMT_NV12;
    encoder->frame->width = width;
    encoder->frame->height = height;
    
    // Allocate frame buffers (done once, reused)
    ret = av_frame_get_buffer(encoder->frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to allocate frame buffer: %s\n", av_err2str(ret));
        av_frame_free(&encoder->frame);
        avcodec_free_context(&encoder->codec_ctx);
        free(encoder);
        return NULL;
    }
    
    // Allocate packet
    encoder->pkt = av_packet_alloc();
    if (!encoder->pkt) {
        fprintf(stderr, "Failed to allocate packet\n");
        av_frame_free(&encoder->frame);
        avcodec_free_context(&encoder->codec_ctx);
        free(encoder);
        return NULL;
    }
    
    return encoder;
}

size_t encoder_max_output_size(const NV12MJPEGEncoder* encoder) {
    if (!encoder) {
        return 0;
    }
    // Conservative estimate: assume worst case of uncompressed size
    // In practice, MJPEG compression is usually 5:1 to 20:1
    return (size_t)encoder->width * encoder->height * 3 / 2;
}

int encoder_encode_to_buffer(NV12MJPEGEncoder* encoder, const uint8_t* nv12_data,
                              uint8_t* out_buffer, size_t buffer_size, size_t* out_size) {
    int ret;
    uint64_t t_start, t_end, t_total_start;
    
    t_total_start = get_time_ns();
    
    // Validate parameters
    if (!encoder || !nv12_data || !out_buffer || !out_size) {
        return -EINVAL;
    }
    
    // Make frame writable (in case it was used before)
    t_start = get_time_ns();
    ret = av_frame_make_writable(encoder->frame);
    t_end = get_time_ns();
    fprintf(stderr, "[Perf] av_frame_make_writable: %.3f ms\n", (t_end - t_start) / 1000000.0);
    if (ret < 0) {
        fprintf(stderr, "Failed to make frame writable: %s\n", av_err2str(ret));
        return ret;
    }
    
    // Print encoder configuration and buffer info
    fprintf(stderr, "[Encoder] Quality: QP=%d, Resolution: %dx%d\n", 
            encoder->quality, encoder->width, encoder->height);
    fprintf(stderr, "[Encoder] Y linesize: %d (width: %d, padding: %d bytes)\n",
            encoder->frame->linesize[0], encoder->width, 
            encoder->frame->linesize[0] - encoder->width);
    fprintf(stderr, "[Encoder] UV linesize: %d (width: %d, padding: %d bytes)\n",
            encoder->frame->linesize[1], encoder->width,
            encoder->frame->linesize[1] - encoder->width);
    
    // Copy NV12 data to frame using bulk copy
    // Y plane
    const uint8_t* src_y = nv12_data;
    uint8_t* dst_y = encoder->frame->data[0];
    fprintf(stderr, "[Encoder] Y plane: bulk copy %d bytes (linesize=%d, width=%d)\n", 
            encoder->width * encoder->height, encoder->frame->linesize[0], encoder->width);
    t_start = get_time_ns();
    memcpy(dst_y, src_y, encoder->width * encoder->height);
    t_end = get_time_ns();
    fprintf(stderr, "[Perf] Y plane memcpy: %.3f ms (%.2f GB/s)\n", 
            (t_end - t_start) / 1000000.0,
            (encoder->width * encoder->height) / ((t_end - t_start) / 1e9) / 1e9);
    
    // UV plane
    const uint8_t* src_uv = nv12_data + encoder->width * encoder->height;
    uint8_t* dst_uv = encoder->frame->data[1];
    fprintf(stderr, "[Encoder] UV plane: bulk copy %d bytes (linesize=%d, width=%d)\n",
            encoder->width * encoder->height / 2, encoder->frame->linesize[1], encoder->width);
    t_start = get_time_ns();
    memcpy(dst_uv, src_uv, encoder->width * encoder->height / 2);
    t_end = get_time_ns();
    fprintf(stderr, "[Perf] UV plane memcpy: %.3f ms (%.2f GB/s)\n",
            (t_end - t_start) / 1000000.0,
            (encoder->width * encoder->height / 2) / ((t_end - t_start) / 1e9) / 1e9);
    
    // Update PTS
    encoder->frame->pts = encoder->frame_counter++;
    
    // Send frame to encoder
    t_start = get_time_ns();
    ret = avcodec_send_frame(encoder->codec_ctx, encoder->frame);
    t_end = get_time_ns();
    fprintf(stderr, "[Perf] avcodec_send_frame: %.3f ms\n", (t_end - t_start) / 1000000.0);
    if (ret < 0) {
        fprintf(stderr, "Error sending frame to encoder: %s\n", av_err2str(ret));
        return ret;
    }
    
    // Receive encoded packet
    t_start = get_time_ns();
    ret = avcodec_receive_packet(encoder->codec_ctx, encoder->pkt);
    t_end = get_time_ns();
    fprintf(stderr, "[Perf] avcodec_receive_packet: %.3f ms\n", (t_end - t_start) / 1000000.0);
    if (ret == AVERROR(EAGAIN)) {
        // Hardware encoder needs flush - send NULL frame to flush
        fprintf(stderr, "[Encoder] Flushing encoder...\n");
        t_start = get_time_ns();
        ret = avcodec_send_frame(encoder->codec_ctx, NULL);
        if (ret < 0) {
            fprintf(stderr, "Error flushing encoder: %s\n", av_err2str(ret));
            return ret;
        }
        
        // Try to receive packet again after flush
        ret = avcodec_receive_packet(encoder->codec_ctx, encoder->pkt);
        t_end = get_time_ns();
        fprintf(stderr, "[Perf] Flush + receive: %.3f ms\n", (t_end - t_start) / 1000000.0);
        if (ret < 0) {
            fprintf(stderr, "Error receiving packet after flush: %s\n", av_err2str(ret));
            return ret;
        }
    } else if (ret == AVERROR_EOF) {
        fprintf(stderr, "Error: encoder returned EOF\n");
        return ret;
    } else if (ret < 0) {
        fprintf(stderr, "Error receiving packet from encoder: %s\n", av_err2str(ret));
        return ret;
    }
    
    // Check if output buffer is large enough
    if ((size_t)encoder->pkt->size > buffer_size) {
        *out_size = encoder->pkt->size;  // Return required size
        av_packet_unref(encoder->pkt);
        fprintf(stderr, "Output buffer too small: need %d bytes, have %zu bytes\n", 
                encoder->pkt->size, buffer_size);
        return -ENOMEM;
    }
    
    // Copy encoded data to output buffer
    t_start = get_time_ns();
    memcpy(out_buffer, encoder->pkt->data, encoder->pkt->size);
    t_end = get_time_ns();
    fprintf(stderr, "[Perf] Output memcpy: %.3f ms (%d bytes)\n", 
            (t_end - t_start) / 1000000.0, encoder->pkt->size);
    *out_size = encoder->pkt->size;
    
    // Unreference packet for next use
    av_packet_unref(encoder->pkt);
    
    uint64_t t_total_end = get_time_ns();
    fprintf(stderr, "[Perf] === TOTAL encoding time: %.3f ms ===\n", 
            (t_total_end - t_total_start) / 1000000.0);
    
    return 0;
}

void encoder_destroy(NV12MJPEGEncoder* encoder) {
    if (!encoder) {
        return;
    }
    
    if (encoder->pkt) {
        av_packet_free(&encoder->pkt);
    }
    if (encoder->frame) {
        av_frame_free(&encoder->frame);
    }
    if (encoder->codec_ctx) {
        avcodec_free_context(&encoder->codec_ctx);
    }
    
    free(encoder);
}

// ============================================================================
// Persistent Decoder Context Implementation
// ============================================================================

struct NV12MJPEGDecoder {
    const AVCodec* codec;         // Cached codec pointer
    AVCodecContext* codec_ctx;    // Hardware decoder context (persistent)
    AVFrame* frame;               // Pre-allocated frame
    AVPacket* pkt;                // Pre-allocated packet
    AVBufferRef* hw_device_ctx;   // Hardware device context for rkmpp
};

NV12MJPEGDecoder* decoder_create(void) {
    int ret;
    
    // Allocate decoder context
    NV12MJPEGDecoder* decoder = (NV12MJPEGDecoder*)calloc(1, sizeof(NV12MJPEGDecoder));
    if (!decoder) {
        fprintf(stderr, "Failed to allocate decoder context\n");
        return NULL;
    }
    
    // Find hardware MJPEG decoder
    decoder->codec = avcodec_find_decoder_by_name("mjpeg_rkmpp");
    if (!decoder->codec) {
        fprintf(stderr, "Hardware MJPEG decoder (mjpeg_rkmpp) not found\n");
        free(decoder);
        return NULL;
    }
    
    // Create hardware device context for rkmpp
    ret = av_hwdevice_ctx_create(&decoder->hw_device_ctx, AV_HWDEVICE_TYPE_RKMPP,
                                  NULL, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to create rkmpp hardware device context: %s\n", av_err2str(ret));
        free(decoder);
        return NULL;
    }
    
    // Allocate codec context
    decoder->codec_ctx = avcodec_alloc_context3(decoder->codec);
    if (!decoder->codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        av_buffer_unref(&decoder->hw_device_ctx);
        free(decoder);
        return NULL;
    }
    
    // Set hardware device context
    decoder->codec_ctx->hw_device_ctx = av_buffer_ref(decoder->hw_device_ctx);
    
    // Note: For hardware decoders, don't pre-set pix_fmt
    // The decoder will determine the format from the input stream
    
    // Open codec (expensive operation - done once)
    ret = avcodec_open2(decoder->codec_ctx, decoder->codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(ret));
        av_buffer_unref(&decoder->hw_device_ctx);
        avcodec_free_context(&decoder->codec_ctx);
        free(decoder);
        return NULL;
    }
    
    // Allocate frame
    decoder->frame = av_frame_alloc();
    if (!decoder->frame) {
        fprintf(stderr, "Failed to allocate frame\n");
        avcodec_free_context(&decoder->codec_ctx);
        free(decoder);
        return NULL;
    }
    
    // Allocate packet
    decoder->pkt = av_packet_alloc();
    if (!decoder->pkt) {
        fprintf(stderr, "Failed to allocate packet\n");
        av_frame_free(&decoder->frame);
        avcodec_free_context(&decoder->codec_ctx);
        free(decoder);
        return NULL;
    }
    
    return decoder;
}

int decoder_decode_from_buffer(NV12MJPEGDecoder* decoder, const uint8_t* mjpeg_data, size_t mjpeg_size,
                                uint8_t* out_nv12_buffer, size_t buffer_size,
                                int* out_width, int* out_height) {
    int ret;
    
    // Validate parameters
    if (!decoder || !mjpeg_data || !out_nv12_buffer || !out_width || !out_height) {
        return -EINVAL;
    }
    if (mjpeg_size == 0) {
        return -EINVAL;
    }
    
    // Wrap input data in packet (no copy - just reference)
    decoder->pkt->data = (uint8_t*)mjpeg_data;
    decoder->pkt->size = mjpeg_size;
    
    // Send packet to decoder
    ret = avcodec_send_packet(decoder->codec_ctx, decoder->pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending packet to decoder: %s\n", av_err2str(ret));
        decoder->pkt->data = NULL;  // Don't free user's data
        decoder->pkt->size = 0;
        return ret;
    }
    
    // Reset packet data pointer (we don't own it)
    decoder->pkt->data = NULL;
    decoder->pkt->size = 0;
    
    // Receive decoded frame
    ret = avcodec_receive_frame(decoder->codec_ctx, decoder->frame);
    if (ret == AVERROR(EAGAIN)) {
        // Decoder needs more data (shouldn't happen with single MJPEG frame)
        fprintf(stderr, "Decoder needs more data (EAGAIN)\n");
        return ret;
    } else if (ret < 0) {
        fprintf(stderr, "Error receiving frame from decoder: %s\n", av_err2str(ret));
        return ret;
    }
    
    // Extract dimensions
    *out_width = decoder->frame->width;
    *out_height = decoder->frame->height;
    
    // Check if output buffer is large enough
    size_t required_size = (size_t)(*out_width) * (*out_height) * 3 / 2;
    if (buffer_size < required_size) {
        fprintf(stderr, "Output buffer too small: need %zu bytes, have %zu bytes\n", 
                required_size, buffer_size);
        return -ENOMEM;
    }
    
    // Check pixel format
    if (decoder->frame->format != AV_PIX_FMT_NV12) {
        fprintf(stderr, "Warning: decoded frame format is not NV12 (got %s)\n",
                av_get_pix_fmt_name(decoder->frame->format));
        return -EINVAL;
    }
    
    // Copy NV12 data from frame to output buffer
    // Y plane - use bulk copy if possible, otherwise per-row copy
    uint8_t* dst_y = out_nv12_buffer;
    const uint8_t* src_y = decoder->frame->data[0];
    if (decoder->frame->linesize[0] == *out_width) {
        // Bulk copy - no padding
        memcpy(dst_y, src_y, (*out_width) * (*out_height));
    } else {
        // Per-row copy with OpenMP parallelization
        #pragma omp parallel for if(*out_height > 480)
        for (int y = 0; y < *out_height; y++) {
            memcpy(dst_y + y * (*out_width), src_y + y * decoder->frame->linesize[0], *out_width);
        }
    }
    
    // UV plane - use bulk copy if possible, otherwise per-row copy
    uint8_t* dst_uv = out_nv12_buffer + (*out_width) * (*out_height);
    const uint8_t* src_uv = decoder->frame->data[1];
    if (decoder->frame->linesize[1] == *out_width) {
        // Bulk copy - no padding
        memcpy(dst_uv, src_uv, (*out_width) * (*out_height) / 2);
    } else {
        // Per-row copy with OpenMP parallelization
        #pragma omp parallel for if(*out_height > 480)
        for (int y = 0; y < (*out_height) / 2; y++) {
            memcpy(dst_uv + y * (*out_width), src_uv + y * decoder->frame->linesize[1], *out_width);
        }
    }
    
    // Unreference frame for next use
    av_frame_unref(decoder->frame);
    
    return 0;
}

void decoder_destroy(NV12MJPEGDecoder* decoder) {
    if (!decoder) {
        return;
    }
    
    if (decoder->pkt) {
        av_packet_free(&decoder->pkt);
    }
    if (decoder->frame) {
        av_frame_free(&decoder->frame);
    }
    if (decoder->codec_ctx) {
        avcodec_free_context(&decoder->codec_ctx);
    }
    if (decoder->hw_device_ctx) {
        av_buffer_unref(&decoder->hw_device_ctx);
    }
    
    free(decoder);
}

// ============================================================================
// Memory Management Functions
// ============================================================================

uint8_t* alloc_nv12_buffer(int width, int height) {
    size_t buffer_size = nv12_frame_size(width, height);
    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate NV12 buffer (%zu bytes)\n", buffer_size);
        return NULL;
    }
    return buffer;
}

void free_nv12_buffer(uint8_t* buffer) {
    if (buffer) {
        free(buffer);
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int64_t get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

// ============================================================================
// File I/O Functions
// ============================================================================

int read_nv12_from_file(const char* filename, uint8_t* buffer, int width, int height) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open input file: %s\n", filename);
        return -1;
    }
    
    size_t frame_size = nv12_frame_size(width, height);
    size_t bytes_read = fread(buffer, 1, frame_size, fp);
    fclose(fp);
    
    if (bytes_read != frame_size) {
        fprintf(stderr, "Failed to read complete frame: expected %zu bytes, got %zu bytes\n",
                frame_size, bytes_read);
        return -1;
    }
    
    return 0;
}

int write_nv12_to_file(const char* filename, const uint8_t* buffer, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open output file: %s\n", filename);
        return -1;
    }
    
    size_t frame_size = nv12_frame_size(width, height);
    size_t bytes_written = fwrite(buffer, 1, frame_size, fp);
    fclose(fp);
    
    if (bytes_written != frame_size) {
        fprintf(stderr, "Failed to write complete frame: expected %zu bytes, wrote %zu bytes\n",
                frame_size, bytes_written);
        return -1;
    }
    
    return 0;
}
