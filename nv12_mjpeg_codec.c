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

// ============================================================================
// Encoding Function: NV12 → MJPEG
// ============================================================================

int encode_nv12_to_mjpeg(const uint8_t* nv12_data, int width, int height, const char* output_file, int quality) {
    int ret;
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVStream* stream = NULL;
    AVFrame* frame = NULL;
    AVPacket* pkt = NULL;
    
    // Validate quality parameter
    if (quality < 1 || quality > 31) {
        fprintf(stderr, "Error: quality must be between 1 and 31 (got %d)\n", quality);
        return -3;
    }
    
    // Find hardware MJPEG encoder
    const AVCodec* codec = avcodec_find_encoder_by_name("mjpeg_rkmpp");
    if (!codec) {
        fprintf(stderr, "Hardware MJPEG encoder (mjpeg_rkmpp) not found\n");
        return -1;
    }
    
    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        return -1;
    }
    
    // Configure codec parameters
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->pix_fmt = AV_PIX_FMT_NV12;
    codec_ctx->time_base = (AVRational){1, 30};
    codec_ctx->framerate = (AVRational){30, 1};
    
    // Set quality control parameters
    codec_ctx->qmin = quality;
    codec_ctx->qmax = quality;
    
    // Set hardware encoder options (QP initial value)
    av_opt_set_int(codec_ctx->priv_data, "qp_init", quality, 0);
    
    // Open codec
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(ret));
        avcodec_free_context(&codec_ctx);
        return ret;
    }
    
    // Allocate output context
    avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, output_file);
    if (!fmt_ctx) {
        fprintf(stderr, "Failed to allocate output context\n");
        avcodec_free_context(&codec_ctx);
        return -1;
    }
    
    // Create output stream
    stream = avformat_new_stream(fmt_ctx, codec);
    if (!stream) {
        fprintf(stderr, "Failed to create output stream\n");
        avcodec_free_context(&codec_ctx);
        avformat_free_context(fmt_ctx);
        return -1;
    }
    
    // Copy codec parameters to stream
    avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    stream->time_base = codec_ctx->time_base;
    
    // Open output file
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Failed to open output file: %s\n", av_err2str(ret));
            avcodec_free_context(&codec_ctx);
            avformat_free_context(fmt_ctx);
            return ret;
        }
    }
    
    // Write header
    ret = avformat_write_header(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to write header: %s\n", av_err2str(ret));
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avcodec_free_context(&codec_ctx);
        avformat_free_context(fmt_ctx);
        return ret;
    }
    
    // Allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Failed to allocate frame\n");
        av_write_trailer(fmt_ctx);
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avcodec_free_context(&codec_ctx);
        avformat_free_context(fmt_ctx);
        return -1;
    }
    
    frame->format = AV_PIX_FMT_NV12;
    frame->width = width;
    frame->height = height;
    frame->pts = 0;
    
    // Allocate frame buffers
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to allocate frame buffer: %s\n", av_err2str(ret));
        av_frame_free(&frame);
        av_write_trailer(fmt_ctx);
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avcodec_free_context(&codec_ctx);
        avformat_free_context(fmt_ctx);
        return ret;
    }
    
    // Copy NV12 data to frame
    // Y plane
    const uint8_t* src_y = nv12_data;
    uint8_t* dst_y = frame->data[0];
    for (int y = 0; y < height; y++) {
        memcpy(dst_y + y * frame->linesize[0], src_y + y * width, width);
    }
    
    // UV plane
    const uint8_t* src_uv = nv12_data + width * height;
    uint8_t* dst_uv = frame->data[1];
    for (int y = 0; y < height / 2; y++) {
        memcpy(dst_uv + y * frame->linesize[1], src_uv + y * width, width);
    }
    
    // Allocate packet
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Failed to allocate packet\n");
        av_frame_free(&frame);
        av_write_trailer(fmt_ctx);
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avcodec_free_context(&codec_ctx);
        avformat_free_context(fmt_ctx);
        return -1;
    }
    
    // Send frame to encoder
    ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending frame to encoder: %s\n", av_err2str(ret));
        av_packet_free(&pkt);
        av_frame_free(&frame);
        av_write_trailer(fmt_ctx);
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx->pb);
        }
        avcodec_free_context(&codec_ctx);
        avformat_free_context(fmt_ctx);
        return ret;
    }
    
    // Receive encoded packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            fprintf(stderr, "Error receiving packet from encoder: %s\n", av_err2str(ret));
            av_packet_free(&pkt);
            av_frame_free(&frame);
            av_write_trailer(fmt_ctx);
            if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&fmt_ctx->pb);
            }
            avcodec_free_context(&codec_ctx);
            avformat_free_context(fmt_ctx);
            return ret;
        }
        
        pkt->stream_index = stream->index;
        av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
        
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        if (ret < 0) {
            fprintf(stderr, "Error writing frame: %s\n", av_err2str(ret));
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            av_frame_free(&frame);
            av_write_trailer(fmt_ctx);
            if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&fmt_ctx->pb);
            }
            avcodec_free_context(&codec_ctx);
            avformat_free_context(fmt_ctx);
            return ret;
        }
        
        av_packet_unref(pkt);
    }
    
    // Flush encoder (send NULL frame)
    ret = avcodec_send_frame(codec_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error flushing encoder: %s\n", av_err2str(ret));
    } else {
        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_ctx, pkt);
            if (ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                break;
            }
            
            pkt->stream_index = stream->index;
            av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
            av_interleaved_write_frame(fmt_ctx, pkt);
            av_packet_unref(pkt);
        }
    }
    
    // Write trailer and cleanup
    av_write_trailer(fmt_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    
    avcodec_free_context(&codec_ctx);
    avformat_free_context(fmt_ctx);
    
    return 0;
}

// ============================================================================
// Decoding Function: MJPEG → NV12
// ============================================================================

int decode_mjpeg_to_nv12(const char* input_file, uint8_t* nv12_data, int* width, int* height) {
    int ret;
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVFrame* frame = NULL;
    AVPacket* pkt = NULL;
    int video_stream_idx = -1;
    
    // Open input file
    ret = avformat_open_input(&fmt_ctx, input_file, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open input file: %s\n", av_err2str(ret));
        return ret;
    }
    
    // Retrieve stream information
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to find stream info: %s\n", av_err2str(ret));
        avformat_close_input(&fmt_ctx);
        return ret;
    }
    
    // Find video stream
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_idx < 0) {
        fprintf(stderr, "Failed to find video stream\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    // Find hardware MJPEG decoder
    const AVCodec* codec = avcodec_find_decoder_by_name("mjpeg_rkmpp");
    if (!codec) {
        fprintf(stderr, "Hardware MJPEG decoder (mjpeg_rkmpp) not found\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    // Copy codec parameters from stream
    ret = avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);
    if (ret < 0) {
        fprintf(stderr, "Failed to copy codec parameters: %s\n", av_err2str(ret));
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return ret;
    }
    
    // Open codec
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(ret));
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return ret;
    }
    
    // Allocate frame and packet
    frame = av_frame_alloc();
    pkt = av_packet_alloc();
    if (!frame || !pkt) {
        fprintf(stderr, "Failed to allocate frame or packet\n");
        if (frame) av_frame_free(&frame);
        if (pkt) av_packet_free(&pkt);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    // Read first frame
    int frame_decoded = 0;
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index != video_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }
        
        // Send packet to decoder
        ret = avcodec_send_packet(codec_ctx, pkt);
        av_packet_unref(pkt);
        
        if (ret < 0) {
            fprintf(stderr, "Error sending packet to decoder: %s\n", av_err2str(ret));
            continue;
        }
        
        // Receive decoded frame
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            fprintf(stderr, "Error receiving frame from decoder: %s\n", av_err2str(ret));
            break;
        }
        
        // Successfully decoded first frame
        frame_decoded = 1;
        break;
    }
    
    if (!frame_decoded) {
        fprintf(stderr, "Failed to decode any frame\n");
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    
    // Extract width and height
    *width = frame->width;
    *height = frame->height;
    
    // Check pixel format
    if (frame->format != AV_PIX_FMT_NV12) {
        fprintf(stderr, "Warning: decoded frame format is not NV12 (got %s)\n",
                av_get_pix_fmt_name(frame->format));
    }
    
    // Copy NV12 data from frame
    // Y plane
    uint8_t* dst_y = nv12_data;
    const uint8_t* src_y = frame->data[0];
    for (int y = 0; y < *height; y++) {
        memcpy(dst_y + y * (*width), src_y + y * frame->linesize[0], *width);
    }
    
    // UV plane
    uint8_t* dst_uv = nv12_data + (*width) * (*height);
    const uint8_t* src_uv = frame->data[1];
    for (int y = 0; y < (*height) / 2; y++) {
        memcpy(dst_uv + y * (*width), src_uv + y * frame->linesize[1], *width);
    }
    
    // Cleanup
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    
    return 0;
}
