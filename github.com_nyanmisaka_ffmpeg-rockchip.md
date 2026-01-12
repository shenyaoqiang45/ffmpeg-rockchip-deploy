# GitHub - nyanmisaka/ffmpeg-rockchip: FFmpeg with async and zero-copy Rockchip MPP & RGA support

**URL:** https://github.com/nyanmisaka/ffmpeg-rockchip

---

Skip to content
Navigation Menu
Platform
Solutions
Resources
Open Source
Enterprise
Pricing
Sign in
Sign up
nyanmisaka
/
ffmpeg-rockchip
Public
Notifications
Fork 183
 Star 1.1k
Code
Issues
5
Pull requests
Projects
Wiki
Security
Insights
nyanmisaka/ffmpeg-rockchip
 master
6 Branches
0 Tags
Code
Folders and files
Name	Last commit message	Last commit date

Latest commit
nyanmisaka
fixup! lavc/rkmppdec: refactor RKMPP decoders and extend codecs
2eb0b5d
 · 
History
112,692 Commits


compat
	
configure: Set WIN32_LEAN_AND_MEAN at configure time
	


doc
	
Update for 6.1
	


ffbuild
	
configure: probe RISC-V Vector extension
	


fftools
	
fftools/ffmpeg_mux_init: Restrict disabling automatic copying of meta…
	


libavcodec
	
fixup! lavc/rkmppdec: refactor RKMPP decoders and extend codecs
	


libavdevice
	
v4l2: use always channel 0 if driver does not return it. Always ignor…
	


libavfilter
	
fixup! lavf/rkrga: add RKRGA scale, vpp and overlay filter
	


libavformat
	
libavformat/vvc: Make probe more conservative
	


libavutil
	
fixup! lavu: add RKMPP hwcontext
	


libpostproc
	
Bump versions prior to 6.1
	


libswresample
	
Bump versions prior to 6.1
	


libswscale
	
fixup! lsws/input: add NV15 and NV20 formats support
	


presets
	
presets: remove moldering iPod presets
	


tests
	
lavu: add NV20 bitstream format support
	


tools
	
tools/target_dec_fuzzer: Adjust threshold for CSCD
	


.gitattributes
	
fate: add SCC test
	


.gitignore
	
gitignore: add config_components.h
	


.mailmap
	
mailmap: stop git lying about who I commit things as
	


.travis.yml
	
Merge commit '899ee03088d55152a48830df0899887f055da1de'
	


CONTRIBUTING.md
	
Add CONTRIBUTING.md
	


COPYING.GPLv2
	
Rename COPYING.GPL --> COPYING.GPLv2 and COPYING.LGPL --> COPYING.LGP…
	


COPYING.GPLv3
	
Add configure option to upgrade (L)GPL to version 3.
	


COPYING.LGPLv2.1
	
cosmetics: Delete empty lines at end of file.
	


COPYING.LGPLv3
	
Add configure option to upgrade (L)GPL to version 3.
	


CREDITS
	
Use https for repository links
	


Changelog
	
Changelog: mark 6.0
	


INSTALL.md
	
INSTALL.md: Fix Markdown formatting
	


LICENSE.md
	
avfilter/vf_geq: Relicense to LGPL
	


MAINTAINERS
	
MAINTAINERS: add entry for jpegxl*
	


Makefile
	
tools: Don't include the direct library names when linking
	


README.md
	
fixup! README: update for ffmpeg-rockchip
	


RELEASE
	
Update for 6.1
	


RELEASE_NOTES
	
RELEASE_NOTES: Based on the version from 5.1
	


configure
	
lavc/rkmppdec: add RKMPP MJPEG decoder
	
Repository files navigation
README
Contributing
License
GPL-2.0 license
GPL-3.0 license
LGPL-3.0 license
ffmpeg-rockchip

This project aims to provide full hardware transcoding pipeline in FFmpeg CLI for Rockchip platforms that support MPP (Media Process Platform) and RGA (2D Raster Graphic Acceleration). This includes hardware decoders, encoders and filters. A typical target platform is RK3588/3588s based devices.

Hightlights
MPP decoders support up to 8K 10-bit H.264, HEVC, VP9 and AV1 decoding
MPP decoders support producing AFBC (ARM Frame Buffer Compression) image
MPP decoders support de-interlace using IEP (Image Enhancement Processor)
MPP decoders support allocator half-internal and pure-external modes
MPP encoders support up to 8K H.264 and HEVC encoding
MPP encoders support async encoding, AKA frame-parallel
MPP encoders support consuming AFBC image
RGA filters support image scaling and pixel format conversion
RGA filters support image cropping
RGA filters support image transposing
RGA filters support blending two images
RGA filters support async operation
RGA filters support producing and consuming AFBC image
Zero-copy DMA in above stages
How to use

The documentation is available on the Wiki page of this project.

Codecs and filters
Decoders/Hwaccel
 V..... av1_rkmpp            Rockchip MPP (Media Process Platform) AV1 decoder (codec av1)
 V..... h263_rkmpp           Rockchip MPP (Media Process Platform) H263 decoder (codec h263)
 V..... h264_rkmpp           Rockchip MPP (Media Process Platform) H264 decoder (codec h264)
 V..... hevc_rkmpp           Rockchip MPP (Media Process Platform) HEVC decoder (codec hevc)
 V..... mjpeg_rkmpp          Rockchip MPP (Media Process Platform) MJPEG decoder (codec mjpeg)
 V..... mpeg1_rkmpp          Rockchip MPP (Media Process Platform) MPEG1VIDEO decoder (codec mpeg1video)
 V..... mpeg2_rkmpp          Rockchip MPP (Media Process Platform) MPEG2VIDEO decoder (codec mpeg2video)
 V..... mpeg4_rkmpp          Rockchip MPP (Media Process Platform) MPEG4 decoder (codec mpeg4)
 V..... vp8_rkmpp            Rockchip MPP (Media Process Platform) VP8 decoder (codec vp8)
 V..... vp9_rkmpp            Rockchip MPP (Media Process Platform) VP9 decoder (codec vp9)

Encoders
 V..... h264_rkmpp           Rockchip MPP (Media Process Platform) H264 encoder (codec h264)
 V..... hevc_rkmpp           Rockchip MPP (Media Process Platform) HEVC encoder (codec hevc)
 V..... mjpeg_rkmpp          Rockchip MPP (Media Process Platform) MJPEG encoder (codec mjpeg)

Filters
 ... overlay_rkrga     VV->V      Rockchip RGA (2D Raster Graphic Acceleration) video compositor
 ... scale_rkrga       V->V       Rockchip RGA (2D Raster Graphic Acceleration) video resizer and format converter
 ... vpp_rkrga         V->V       Rockchip RGA (2D Raster Graphic Acceleration) video post-process (scale/crop/transpose)

Important
Rockchip BSP/vendor kernel is necessary, 5.10 and 6.1 are two tested versions.
For the supported maximum resolution and FPS you can refer to the datasheet or TRM.
User MUST be granted permission to access these device files.
# DRM allocator
/dev/dri

# DMA_HEAP allocator
/dev/dma_heap

# RGA filters
/dev/rga

# MPP codecs
/dev/mpp_service

# Optional, for compatibility with older kernels and socs
/dev/iep
/dev/mpp-service
/dev/vpu_service
/dev/vpu-service
/dev/hevc_service
/dev/hevc-service
/dev/rkvdec
/dev/rkvenc
/dev/vepu
/dev/h265e

Todo
Support MPP VP8 video encoder
...
Acknowledgments

@hbiyik @HermanChen @rigaya

FFmpeg README

FFmpeg is a collection of libraries and tools to process multimedia content such as audio, video, subtitles and related metadata.

Libraries
libavcodec provides implementation of a wider range of codecs.
libavformat implements streaming protocols, container formats and basic I/O access.
libavutil includes hashers, decompressors and miscellaneous utility functions.
libavfilter provides means to alter decoded audio and video through a directed graph of connected filters.
libavdevice provides an abstraction to access capture and playback devices.
libswresample implements audio mixing and resampling routines.
libswscale implements color conversion and scaling routines.
Tools
ffmpeg is a command line toolbox to manipulate, convert and stream multimedia content.
ffplay is a minimalistic multimedia player.
ffprobe is a simple analysis tool to inspect multimedia content.
Additional small tools such as aviocat, ismindex and qt-faststart.
Documentation

The offline documentation is available in the doc/ directory.

The online documentation is available in the main website and in the wiki.

Examples

Coding examples are available in the doc/examples directory.

License

FFmpeg codebase is mainly LGPL-licensed with optional components licensed under GPL. Please refer to the LICENSE file for detailed information.

Contributing

Patches should be submitted to the ffmpeg-devel mailing list using git format-patch or git send-email. Github pull requests should be avoided because they are not part of our review process and will be ignored.

About

FFmpeg with async and zero-copy Rockchip MPP & RGA support

Topics
linux arm video ffmpeg multimedia encoder decoder filter video-processing rockchip mpp arm64 transcode rga rkmpp rk3588 rk3588s librga rk35xx rkrga
Resources
 Readme
License
 Unknown and 3 other licenses found
Contributing
 Contributing
 Activity
Stars
 1.1k stars
Watchers
 24 watching
Forks
 183 forks
Report repository


Releases
No releases published


Packages
No packages published



Contributors
3
nyanmisaka Nyanmisaka
boogieeeee
rigaya rigaya


Languages
C
91.0%
 
Assembly
7.1%
 
Makefile
1.3%
 
C++
0.2%
 
Objective-C
0.2%
 
Cuda
0.1%
 
Other
0.1%
Footer
© 2026 GitHub, Inc.
Footer navigation
Terms
Privacy
Security
Status
Community
Docs
Contact
Manage cookies
Do not share my personal information