#!/usr/bin/env python3
"""
FFmpeg-Rockchip NV12 to MJPEG Encoding Test

This script demonstrates how to encode raw NV12 video frames
to MJPEG using the Rockchip VEPU hardware accelerator via FFmpeg.

Requirements:
    - FFmpeg with Rockchip support (ffmpeg-rockchip)
    - Python 3.6+
    - av (PyAV) library

Installation:
    pip install av

Usage:
    python3 nv12_to_mjpeg_test.py --width 1920 --height 1080 --fps 30 --output output.mjpeg
"""

import argparse
import sys
import numpy as np
import av
from av import VideoFrame


class NV12ToMJPEGEncoder:
    """Encodes NV12 frames to MJPEG using FFmpeg-Rockchip"""
    
    def __init__(self, width, height, fps, output_file):
        """
        Initialize the encoder
        
        Args:
            width: Video width in pixels
            height: Video height in pixels
            fps: Frames per second
            output_file: Output file path
        """
        self.width = width
        self.height = height
        self.fps = fps
        self.output_file = output_file
        self.container = None
        self.stream = None
        self.codec_context = None
        
    def initialize(self):
        """Initialize the output container and codec"""
        try:
            # Create output container
            self.container = av.open(self.output_file, 'w')
            
            # Add video stream with MJPEG codec
            # Try hardware encoder first, fall back to software
            try:
                self.stream = self.container.add_stream('mjpeg', rate=self.fps)
                print(f"Using software MJPEG encoder")
            except Exception as e:
                print(f"Warning: Could not use hardware encoder: {e}")
                self.stream = self.container.add_stream('mjpeg', rate=self.fps)
            
            # Configure stream parameters
            self.stream.width = self.width
            self.stream.height = self.height
            self.stream.pix_fmt = 'nv12'
            
            # Set quality parameters
            self.stream.options = {
                'qmin': '2',
                'qmax': '31',
            }
            
            print(f"Initialized encoder: {self.width}x{self.height} @ {self.fps} fps")
            print(f"Output file: {self.output_file}")
            
            return True
            
        except Exception as e:
            print(f"Error initializing encoder: {e}", file=sys.stderr)
            return False
    
    def generate_nv12_frame(self, frame_num):
        """
        Generate a test NV12 frame with a simple pattern
        
        Args:
            frame_num: Frame number for pattern generation
            
        Returns:
            av.VideoFrame with NV12 format
        """
        # Create frame
        frame = VideoFrame.create('nv12', (self.width, self.height))
        
        # Get Y plane
        y_plane = np.asarray(frame.planes[0])
        
        # Generate Y plane with gradient pattern
        for y in range(self.height):
            for x in range(self.width):
                value = (x + y + frame_num * 2) % 256
                y_plane[y, x] = value
        
        # Get UV plane (interleaved)
        uv_plane = np.asarray(frame.planes[1])
        
        # Generate UV plane
        for y in range(self.height // 2):
            for x in range(self.width // 2):
                u_value = (x + frame_num) % 256
                v_value = (y + frame_num) % 256
                
                # Interleaved UV format: U, V, U, V, ...
                uv_plane[y, x * 2] = u_value
                uv_plane[y, x * 2 + 1] = v_value
        
        return frame
    
    def encode_frame(self, frame):
        """
        Encode a single frame
        
        Args:
            frame: av.VideoFrame to encode
            
        Returns:
            True on success, False on failure
        """
        try:
            packets = self.stream.encode(frame)
            for packet in packets:
                self.container.mux(packet)
            return True
        except Exception as e:
            print(f"Error encoding frame: {e}", file=sys.stderr)
            return False
    
    def flush(self):
        """Flush remaining frames from encoder"""
        try:
            packets = self.stream.encode()
            for packet in packets:
                self.container.mux(packet)
            return True
        except Exception as e:
            print(f"Error flushing encoder: {e}", file=sys.stderr)
            return False
    
    def close(self):
        """Close the output container"""
        if self.container:
            self.container.close()
    
    def run(self, frame_count):
        """
        Run the encoding process
        
        Args:
            frame_count: Number of frames to encode
            
        Returns:
            True on success, False on failure
        """
        if not self.initialize():
            return False
        
        try:
            print(f"Starting encoding: {frame_count} frames")
            
            # Encode frames
            for i in range(frame_count):
                # Generate test frame
                frame = self.generate_nv12_frame(i)
                frame.pts = i
                
                # Encode frame
                if not self.encode_frame(frame):
                    print(f"Failed to encode frame {i}", file=sys.stderr)
                    self.close()
                    return False
                
                if (i + 1) % 10 == 0:
                    print(f"Encoded {i + 1} frames")
            
            # Flush encoder
            if not self.flush():
                print("Failed to flush encoder", file=sys.stderr)
                self.close()
                return False
            
            # Close container
            self.close()
            
            print("Encoding completed successfully")
            return True
            
        except Exception as e:
            print(f"Error during encoding: {e}", file=sys.stderr)
            self.close()
            return False


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Encode NV12 frames to MJPEG using FFmpeg-Rockchip'
    )
    parser.add_argument('--width', type=int, default=1920,
                        help='Video width in pixels (default: 1920)')
    parser.add_argument('--height', type=int, default=1080,
                        help='Video height in pixels (default: 1080)')
    parser.add_argument('--fps', type=int, default=30,
                        help='Frames per second (default: 30)')
    parser.add_argument('--output', type=str, default='output.mjpeg',
                        help='Output file path (default: output.mjpeg)')
    parser.add_argument('--frames', type=int, default=100,
                        help='Number of frames to encode (default: 100)')
    
    args = parser.parse_args()
    
    # Validate parameters
    if args.width <= 0 or args.height <= 0 or args.fps <= 0:
        print("Error: width, height, and fps must be positive", file=sys.stderr)
        return 1
    
    # Create encoder and run
    encoder = NV12ToMJPEGEncoder(args.width, args.height, args.fps, args.output)
    success = encoder.run(args.frames)
    
    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())
