# Makefile for FFmpeg-Rockchip NV12 to MJPEG Test Program

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags libavcodec libavformat libavutil)
LDFLAGS = $(shell pkg-config --libs libavcodec libavformat libavutil)

TARGET = nv12_to_mjpeg_test
SOURCES = nv12_to_mjpeg_test.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean help install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

help:
	@echo "FFmpeg-Rockchip NV12 to MJPEG Test - Makefile"
	@echo "=============================================="
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the test program (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Display this help message"
	@echo "  install  - Install the test program to /usr/local/bin"
	@echo ""
	@echo "Example:"
	@echo "  make"
	@echo "  ./nv12_to_mjpeg_test 1920 1080 30 output.mjpeg"

install: $(TARGET)
	install -D -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	@echo "Installation complete: /usr/local/bin/$(TARGET)"

# Check dependencies
check-deps:
	@echo "Checking dependencies..."
	@pkg-config --exists libavcodec && echo "✓ libavcodec found" || echo "✗ libavcodec not found"
	@pkg-config --exists libavformat && echo "✓ libavformat found" || echo "✗ libavformat not found"
	@pkg-config --exists libavutil && echo "✓ libavutil found" || echo "✗ libavutil not found"
	@echo ""
	@echo "To install missing dependencies:"
	@echo "  sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev"
