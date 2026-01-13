# Makefile for FFmpeg-Rockchip NV12 to MJPEG Test Program

CC = gcc

# By default, this uses the system FFmpeg development packages via pkg-config.
# On many RK boards, the /usr/bin/ffmpeg binary may be a custom build (e.g. with rkmpp)
# while the system -dev packages are an older FFmpeg without mjpeg_rkmpp.
#
# If you have a local FFmpeg build tree (e.g. ffmpeg-rockchip), build against it like:
#   make FFMPEG_BUILD=/home/ainstec/dev/ffmpeg
#
# Convenience: auto-detect $HOME/dev/ffmpeg if it looks like a build output.
FFMPEG_BUILD ?= $(HOME)/dev/ffmpeg

# If auto-detected path doesn't contain built libs, fall back to system FFmpeg.
ifeq ($(wildcard $(FFMPEG_BUILD)/libavcodec/libavcodec.a),)
FFMPEG_BUILD :=
endif

ifeq ($(strip $(FFMPEG_BUILD)),)
CFLAGS = -Wall -Wextra -O2 -fopenmp $(shell pkg-config --cflags libavcodec libavformat libavutil)
LDFLAGS = -fopenmp $(shell pkg-config --libs libavcodec libavformat libavutil)
else
CFLAGS = -Wall -Wextra -O2 -fopenmp -I$(FFMPEG_BUILD)
LDFLAGS = \
	-L$(FFMPEG_BUILD)/libavformat \
	-L$(FFMPEG_BUILD)/libavcodec \
	-L$(FFMPEG_BUILD)/libavutil \
	-L$(FFMPEG_BUILD)/libswresample \
	-L$(FFMPEG_BUILD)/libswscale \
	-Wl,--start-group -lavformat -lavcodec -lavutil -lswresample -lswscale -Wl,--end-group \
	-fopenmp -pthread -lm -latomic -ldl -lz -llzma -lrga -lrockchip_mpp -ldrm
endif

TARGET = nv12_to_mjpeg_test
TARGET2 = codec_benchmark
LIBNAME = libnv12_mjpeg_codec.a

SOURCES = nv12_to_mjpeg_test.c
SOURCES2 = codec_benchmark.c
LIB_SOURCES = nv12_mjpeg_codec.c

OBJECTS = $(SOURCES:.c=.o)
OBJECTS2 = $(SOURCES2:.c=.o)
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)

.PHONY: all clean help install

all: $(TARGET) $(TARGET2)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

# Static library for codec functions
$(LIBNAME): $(LIB_OBJECTS)
	ar rcs $@ $^
	@echo "Static library built: $(LIBNAME)"

# Benchmark program links against the static library
$(TARGET2): $(OBJECTS2) $(LIBNAME)
	$(CC) -o $@ $(OBJECTS2) $(LIBNAME) $(LDFLAGS)
	@echo "Build successful: $(TARGET2)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS2) $(LIB_OBJECTS) $(TARGET) $(TARGET2) $(LIBNAME)
	@echo "Clean complete"

help:
	@echo "FFmpeg-Rockchip NV12 to MJPEG Test - Makefile"
	@echo "=============================================="
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build all test programs and library (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Display this help message"
	@echo "  install  - Install the test programs to /usr/local/bin"
	@echo ""
	@echo "Programs:"
	@echo "  nv12_to_mjpeg_test - Multi-frame encoding test"
	@echo "  codec_benchmark    - Single-frame encode/decode benchmark (uses libnv12_mjpeg_codec.a)"
	@echo ""
	@echo "Library:"
	@echo "  libnv12_mjpeg_codec.a - Static library with codec functions"
	@echo ""
	@echo "Example:"
	@echo "  make"
	@echo "  ./codec_benchmark"
	@echo "  ./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg"

install: $(TARGET) $(TARGET2)
	install -D -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	install -D -m 755 $(TARGET2) /usr/local/bin/$(TARGET2)
	@echo "Installation complete: /usr/local/bin/$(TARGET) and /usr/local/bin/$(TARGET2)"

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@pkg-config --exists libavcodec && echo "✓ libavcodec found" || echo "✗ libavcodec not found"
	@pkg-config --exists libavformat && echo "✓ libavformat found" || echo "✗ libavformat not found"
	@pkg-config --exists libavutil && echo "✓ libavutil found" || echo "✗ libavutil not found"
	@echo ""
	@echo "To install missing dependencies:"
	@echo "  sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev"
