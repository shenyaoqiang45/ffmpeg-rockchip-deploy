/*
 * FFmpeg-Rockchip NV12 to MJPEG Encoding Test
 * 
 * This program demonstrates how to encode raw NV12 video frames
 * to MJPEG using the Rockchip VEPU hardware accelerator via FFmpeg.
 * 
 * Compilation:
 *   gcc -o nv12_to_mjpeg_test nv12_to_mjpeg_test.c \
 *       $(pkg-config --cflags --libs libavcodec libavformat libavutil)
 * 
 * Usage:
 *   ./nv12_to_mjpeg_test <width> <height> <fps> <output.mjpeg>
 *   Example: ./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>

#define FRAME_COUNT 100  // Number of frames to encode

typedef struct {
    int width;
    int height;
    int fps;
    const char *output_file;
    AVFormatContext *fmt_ctx;
    AVStream *stream;
    AVCodecContext *codec_ctx;
    int frame_number;
} EncoderContext;

/**
 * Generate a test NV12 frame with a simple pattern
 * NV12 format: Y plane followed by interleaved UV plane
 */
static void generate_nv12_frame(AVFrame *frame, int frame_num) {
    int width = frame->width;
    int height = frame->height;
    uint8_t *y_data = frame->data[0];
    uint8_t *uv_data = frame->data[1];
    int y_linesize = frame->linesize[0];
    int uv_linesize = frame->linesize[1];
    
    // Generate Y plane with a gradient pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Create a moving pattern based on frame number
            uint8_t value = (uint8_t)((x + y + frame_num * 2) % 256);
            y_data[y * y_linesize + x] = value;
        }
    }
    
    // Generate UV plane with a simpler pattern
    // UV plane is half the height and width of Y plane
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            uint8_t u_value = (uint8_t)((x + frame_num) % 256);
            uint8_t v_value = (uint8_t)((y + frame_num) % 256);
            
            // Interleaved UV format: U, V, U, V, ...
            uv_data[y * uv_linesize + x * 2] = u_value;
            uv_data[y * uv_linesize + x * 2 + 1] = v_value;
        }
    }
}

/**
 * Initialize the encoder context and prepare for encoding
 */
static int encoder_init(EncoderContext *ctx) {
    int ret;
    
    // Allocate output context
    avformat_alloc_output_context2(&ctx->fmt_ctx, NULL, NULL, ctx->output_file);
    if (!ctx->fmt_ctx) {
        fprintf(stderr, "Failed to allocate output context\n");
        return -1;
    }
    
    // Find the MJPEG encoder (prefer hardware-accelerated version)
    const AVCodec *codec = avcodec_find_encoder_by_name("mjpeg_rkmpp");
    if (!codec) {
        fprintf(stderr, "Hardware MJPEG encoder not found, falling back to software encoder\n");
        codec = avcodec_find_encoder_by_name("mjpeg");
    }
    if (!codec) {
        fprintf(stderr, "MJPEG encoder not found\n");
        return -1;
    }
    
    fprintf(stdout, "Using encoder: %s\n", codec->name);
    
    // Create codec context
    ctx->codec_ctx = avcodec_alloc_context3(codec);
    if (!ctx->codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        return -1;
    }
    
    // Configure codec parameters
    ctx->codec_ctx->width = ctx->width;
    ctx->codec_ctx->height = ctx->height;
    ctx->codec_ctx->pix_fmt = AV_PIX_FMT_NV12;
    ctx->codec_ctx->time_base = (AVRational){1, ctx->fps};
    ctx->codec_ctx->framerate = (AVRational){ctx->fps, 1};
    
    // Set quality for MJPEG (lower is better quality, range 2-31)
    ctx->codec_ctx->qmin = 2;
    ctx->codec_ctx->qmax = 31;
    
    // For hardware encoder, set additional options
    if (strstr(codec->name, "rkmpp")) {
        av_opt_set_int(ctx->codec_ctx->priv_data, "qp_init", 10, 0);
        fprintf(stdout, "Configured hardware MJPEG encoder options\n");
    }
    
    // Open codec
    ret = avcodec_open2(ctx->codec_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(ret));
        return ret;
    }
    
    // Create output stream
    ctx->stream = avformat_new_stream(ctx->fmt_ctx, codec);
    if (!ctx->stream) {
        fprintf(stderr, "Failed to create output stream\n");
        return -1;
    }
    
    // Copy codec parameters to stream
    avcodec_parameters_from_context(ctx->stream->codecpar, ctx->codec_ctx);
    ctx->stream->time_base = ctx->codec_ctx->time_base;
    
    // Print format information
    av_dump_format(ctx->fmt_ctx, 0, ctx->output_file, 1);
    
    // Open output file
    if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ctx->fmt_ctx->pb, ctx->output_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Failed to open output file: %s\n", av_err2str(ret));
            return ret;
        }
    }
    
    // Write header
    ret = avformat_write_header(ctx->fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to write header: %s\n", av_err2str(ret));
        return ret;
    }
    
    ctx->frame_number = 0;
    return 0;
}

/**
 * Encode a single frame
 */
static int encode_frame(EncoderContext *ctx, AVFrame *frame) {
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Failed to allocate packet\n");
        return -1;
    }
    
    int ret = avcodec_send_frame(ctx->codec_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending frame to encoder: %s\n", av_err2str(ret));
        av_packet_free(&pkt);
        return ret;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx->codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            fprintf(stderr, "Error receiving packet from encoder: %s\n", av_err2str(ret));
            av_packet_free(&pkt);
            return ret;
        }
        
        // Set packet stream index
        pkt->stream_index = ctx->stream->index;
        
        // Rescale packet timestamp
        av_packet_rescale_ts(pkt, ctx->codec_ctx->time_base, ctx->stream->time_base);
        
        // Write packet to output
        ret = av_interleaved_write_frame(ctx->fmt_ctx, pkt);
        if (ret < 0) {
            fprintf(stderr, "Error writing frame: %s\n", av_err2str(ret));
            av_packet_free(&pkt);
            return ret;
        }
        
        av_packet_unref(pkt);
    }
    
    av_packet_free(&pkt);
    return 0;
}

/**
 * Flush remaining frames from encoder
 */
static int flush_encoder(EncoderContext *ctx) {
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Failed to allocate packet\n");
        return -1;
    }
    
    // Send NULL frame to signal end of input
    int ret = avcodec_send_frame(ctx->codec_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error flushing encoder: %s\n", av_err2str(ret));
        av_packet_free(&pkt);
        return ret;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx->codec_ctx, pkt);
        if (ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            fprintf(stderr, "Error receiving packet during flush: %s\n", av_err2str(ret));
            av_packet_free(&pkt);
            return ret;
        }
        
        pkt->stream_index = ctx->stream->index;
        av_packet_rescale_ts(pkt, ctx->codec_ctx->time_base, ctx->stream->time_base);
        
        ret = av_interleaved_write_frame(ctx->fmt_ctx, pkt);
        if (ret < 0) {
            fprintf(stderr, "Error writing flushed frame: %s\n", av_err2str(ret));
            av_packet_free(&pkt);
            return ret;
        }
        
        av_packet_unref(pkt);
    }
    
    av_packet_free(&pkt);
    return 0;
}

/**
 * Cleanup encoder resources
 */
static void encoder_cleanup(EncoderContext *ctx) {
    if (ctx->fmt_ctx) {
        av_write_trailer(ctx->fmt_ctx);
        
        if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&ctx->fmt_ctx->pb);
        }
        
        avformat_free_context(ctx->fmt_ctx);
        ctx->fmt_ctx = NULL;
    }
    
    if (ctx->codec_ctx) {
        avcodec_free_context(&ctx->codec_ctx);
        ctx->codec_ctx = NULL;
    }
}

/**
 * Main encoding loop
 */
static int run_encoding(EncoderContext *ctx) {
    int ret;
    
    // Initialize encoder
    ret = encoder_init(ctx);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize encoder\n");
        return ret;
    }
    
    fprintf(stdout, "Starting encoding: %dx%d @ %d fps, %d frames\n",
            ctx->width, ctx->height, ctx->fps, FRAME_COUNT);
    
    // Allocate frame
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Failed to allocate frame\n");
        encoder_cleanup(ctx);
        return -1;
    }
    
    frame->format = AV_PIX_FMT_NV12;
    frame->width = ctx->width;
    frame->height = ctx->height;
    
    // Allocate frame buffer
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to allocate frame buffer: %s\n", av_err2str(ret));
        av_frame_free(&frame);
        encoder_cleanup(ctx);
        return ret;
    }
    
    // Encode frames
    for (int i = 0; i < FRAME_COUNT; i++) {
        // Generate test frame
        generate_nv12_frame(frame, i);
        frame->pts = i;
        
        // Encode frame
        ret = encode_frame(ctx, frame);
        if (ret < 0) {
            fprintf(stderr, "Failed to encode frame %d\n", i);
            av_frame_free(&frame);
            encoder_cleanup(ctx);
            return ret;
        }
        
        if ((i + 1) % 10 == 0) {
            fprintf(stdout, "Encoded %d frames\n", i + 1);
        }
    }
    
    // Flush encoder
    ret = flush_encoder(ctx);
    if (ret < 0) {
        fprintf(stderr, "Failed to flush encoder\n");
        av_frame_free(&frame);
        encoder_cleanup(ctx);
        return ret;
    }
    
    // Cleanup
    av_frame_free(&frame);
    encoder_cleanup(ctx);
    
    fprintf(stdout, "Encoding completed successfully\n");
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <width> <height> <fps> <output.mjpeg>\n", argv[0]);
        fprintf(stderr, "Example: %s 1920 1080 30 output.mjpeg\n", argv[0]);
        return 1;
    }
    
    EncoderContext ctx = {0};
    ctx.width = atoi(argv[1]);
    ctx.height = atoi(argv[2]);
    ctx.fps = atoi(argv[3]);
    ctx.output_file = argv[4];
    
    if (ctx.width <= 0 || ctx.height <= 0 || ctx.fps <= 0) {
        fprintf(stderr, "Invalid parameters: width, height, and fps must be positive\n");
        return 1;
    }
    
    int ret = run_encoding(&ctx);
    return ret == 0 ? 0 : 1;
}
