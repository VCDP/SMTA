# 
#   Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
# 
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
# 
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
# 
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
# 
#   Contact Information:
#   Intel Corporation
# 
# 
#  version: SMTA.A.0.9.4-2

DIR = ffmpeg-2.2.3
DST = ../../dist

help:
	@echo "Compiling ffmpeg"
	@echo Use "make asta" or "make snva" or "make xcoder_sw"
vcsa:
ifeq (./lib/libavformat.a, $(wildcard ./lib/libavformat.a))
	@echo "ffmpeg already built"
else
	mkdir -p include lib
	cd $(DIR) && ./configure --disable-programs --disable-doc --disable-avdevice --disable-avfilter --disable-postproc \
		--disable-dxva2 --disable-vaapi --disable-vda --disable-vdpau \
		--disable-swscale --disable-bzlib --disable-swresample --disable-zlib \
		--disable-encoders --disable-hwaccels --disable-muxers --disable-parsers --disable-devices --disable-filters \
		--disable-decoders --enable-decoder=h264 \
		--disable-bsfs --enable-bsf=h264_mp4toannexb \
		--disable-demuxers --enable-demuxer=h264 --enable-demuxer=rtsp \
		--disable-protocols --enable-protocol=file --enable-protocol=rtp \
		--incdir=../include --libdir=../lib --shlibdir=../lib --enable-shared
	$(MAKE) -C $(DIR) install
	cp -d lib/libav* $(DST)
endif

snva:
ifeq (./lib/libavformat.a, $(wildcard ./lib/libavformat.a))
	@echo "ffmpeg already built"
else
	mkdir -p include lib
	cd $(DIR) && ./configure --disable-programs --disable-doc --disable-avdevice \
		--incdir=../include --libdir=../lib --shlibdir=../lib \
		--disable-zlib --disable-iconv --disable-bzlib --disable-muxers \
		--disable-swresample --disable-swscale --disable-postproc --disable-avfilter \
		--disable-dxva2 --disable-vaapi --disable-vda --disable-vdpau --disable-encoders \
		--disable-hwaccels --disable-indevs --disable-devices\
		--disable-parsers --enable-parser=h264 --enable-parser=mpegvideo \
		--enable-parser=vc1 --disable-bsfs --enable-bsf=h264_mp4toannexb --disable-filters \
		--disable-protocols --enable-protocol=file --enable-protocol=rtp \
		--disable-decoders --enable-decoder=h264 --enable-decoder=mpeg2video \
		--enable-decoder=wmv1 --enable-decoder=wmv2 --enable-decoder=wmv3 \
		--disable-dct --disable-dwt --disable-error-resilience --disable-lsp \
		--disable-lzo --disable-mdct --disable-rdft --disable-fft --enable-shared
	$(MAKE) -C $(DIR) -j install
	cp -d lib/libav* $(DST)
endif

asta:
ifeq (./lib/libavformat.a, $(wildcard ./lib/libavformat.a))
	@echo "ffmpeg already built"
else
	mkdir -p include lib
	cd $(DIR) && ./configure --disable-programs --disable-doc --disable-avdevice \
		--incdir=../include --libdir=../lib --shlibdir=../lib \
		--disable-zlib --disable-iconv --disable-bzlib --disable-muxers \
		--disable-swresample --disable-swscale --disable-postproc --disable-avfilter \
		--disable-dxva2 --disable-vaapi --disable-vda --disable-vdpau --disable-encoders \
		--disable-hwaccels --disable-indevs --disable-devices\
		--disable-parsers --enable-parser=h264 --enable-parser=mpegvideo \
		--enable-parser=vc1 --disable-bsfs --enable-bsf=h264_mp4toannexb --disable-filters \
		--disable-protocols --enable-protocol=file --enable-protocol=rtp \
		--disable-decoders --enable-decoder=h264 --enable-decoder=mpeg2video \
		--enable-decoder=wmv1 --enable-decoder=wmv2 --enable-decoder=wmv3
	$(MAKE) -C $(DIR) -j install
endif

xcoder_sw:
ifeq (./lib/libavformat.a, $(wildcard ./lib/libavformat.a))
	@echo "ffmpeg already built"
else
	mkdir -p include lib
	cd $(DIR) && ./configure --enable-gpl --enable-shared --disable-programs --disable-doc \
		--extra-cflags=-I/usr/local/include --extra-ldflags=-L/usr/local/lib \
		--incdir=../include --libdir=../lib --shlibdir=../lib \
		--disable-avdevice --disable-postproc --disable-avfilter --disable-swresample \
		--disable-swscale --disable-dxva2 --disable-vaapi --disable-vda --disable-vdpau \
		--disable-hwaccels --disable-muxers --disable-demuxers --disable-parsers \
		--disable-bsfs --disable-protocols --disable-indevs --disable-outdevs \
		--disable-devices --disable-filters --disable-encoders --disable-decoders \
		--enable-decoder=h264 --enable-decoder=rawvideo \
		--enable-demuxer=h264 \
		--enable-parser=h264 \
		--enable-encoder=h263 --enable-libx264 --enable-encoder=libx264 \
		--disable-iconv --disable-zlib --disable-bzlib
	$(MAKE) -C $(DIR) -j install
	cp -d lib/libav* $(DST)
endif

smta:
ifeq (./ffmpeg-4.0/libavformat/libavformat.so, $(wildcard ./ffmpeg-4.0/libavformat/libavformat.so))
	@echo "ffmpeg already built"
else
	tar xjvf ffmpeg-4.0.tar.bz2
ifeq ($(ver), debug)
	cd ./ffmpeg-4.0 && ./configure --enable-gpl --disable-static --enable-shared --disable-programs \
		--disable-doc \
		--disable-avdevice --disable-avfilter --disable-swresample --disable-swscale\
		--incdir=../$(DST)/include --libdir=../$(DST) --shlibdir=../$(DST) \
		--disable-iconv --disable-zlib --disable-bzlib \
		--enable-debug --disable-optimizations --disable-yasm --disable-asm
else
	cd ./ffmpeg-4.0 && ./configure --prefix=../$(DST) --enable-gpl --disable-static --enable-shared --disable-programs \
		--disable-doc \
		--disable-avdevice --disable-avfilter --disable-swresample --disable-swscale\
		--incdir=../$(DST)/include --libdir=../$(DST) --shlibdir=../$(DST) \
		--disable-iconv --disable-zlib --disable-bzlib
endif
endif
	$(MAKE) -C ./ffmpeg-4.0 -j install

webtv:
ifeq (./lib/libavformat.a, $(wildcard ./lib/libavformat.a))
	@echo "ffmpeg already built"
else
	mkdir -p include lib pro local
	cd $(DIR) && ./configure --enable-gpl --enable-shared --disable-doc \
		--extra-cflags=-I/usr/local/include --extra-ldflags=-L/usr/local/lib \
		--prefix=../local --incdir=../include --libdir=../lib --shlibdir=../lib --datadir=../pro \
		--disable-avdevice --disable-postproc \
		--disable-swscale --disable-dxva2 --disable-vaapi --disable-vda --disable-vdpau \
		--disable-hwaccels --disable-muxers --disable-demuxers --disable-parsers \
		--disable-bsfs --disable-indevs --disable-outdevs \
		--disable-devices --disable-filters --disable-encoders --disable-decoders \
		--enable-protocol=file \
		--enable-protocol=tcp \
		--enable-bsf=h264_mp4toannexb \
		--enable-demuxer=image2 \
		--enable-demuxer=rtsp \
		--enable-muxer=mpegts \
		--enable-muxer=mp4 \
		--enable-muxer=rtp \
		--enable-muxer=rtsp \
		--enable-demuxer=mov \
		--enable-decoder=mjpeg --enable-demuxer=mjpeg --enable-parser=mjpeg \
		--enable-decoder=h264 --enable-decoder=rawvideo \
		--enable-demuxer=h264 \
		--enable-parser=h264 \
		--enable-encoder=h263 --enable-libx264 --enable-encoder=libx264 \
		--disable-iconv --disable-zlib --disable-bzlib
	$(MAKE) -C $(DIR) -j install #--disable-avdevice  --disable-avfilter --disable-swresample --disable-protocols
	cp -d lib/lib* $(DST)
	cp -d local/bin/* $(DST)
endif
clean:
	-rm include lib -rf
	-$(MAKE) -C $(DIR) distclean
	-rm -rf ./ffmpeg-4.0
	-rm $(DST)/libav*
	-rm $(DST)/libsw*
	-rm $(DST)/libpostproc.*
	-rm $(DST)/ff*
	-rm -rf $(DST)/include
	-rm -rf $(DST)/pkgconfig
