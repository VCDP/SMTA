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

DIRS = imports libxcode app
SRCS = libxcode app

ifeq (README, $(wildcard README))
	TARGET=help
else
ifeq (README.ADEC, $(wildcard README.ADEC))
	TARGET=adec
endif

ifeq (README.VCSA, $(wildcard README.VCSA))
	TARGET=vcsa
endif

ifeq (README.VCSA_AUDIO, $(wildcard README.VCSA_AUDIO))
	TARGET=vcsa_audio
endif

ifeq (README.VCSA_VIDEO, $(wildcard README.VCSA_VIDEO))
	TARGET=vcsa_video
endif

ifeq (README.ASTA, $(wildcard README.ASTA))
	TARGET=asta
endif

ifeq (README.XCODER, $(wildcard README.XCODER))
	TARGET=xcoder
endif

ifeq (README.XCODER_SW, $(wildcard README.XCODER_SW))
	TARGET=xcoder_sw
endif

ifeq (README.SNVA, $(wildcard README.SNVA))
	TARGET=snva
endif

ifeq (README.VIOA, $(wildcard README.VIOA))
	TARGET=vioa
endif
endif

all:$(TARGET)

help:
	@echo "Usage: make <TARGET> [ver=release/debug]"
	@echo "Description:"
	@echo "       TARGET : asta, to build ASTA codes"
	@echo "              : vcsa, to build VCSA without IPP"
	@echo "              : vcsa_audio, to build VCSA without IPP and video module"
	@echo "              : vcsa_video, to build VCSA without IPP and audio module"
	@echo "              : adec, to build VCSA with IPP"
	@echo "              : xcoder, to build file and stream codes"
	@echo "              : xcoder_sw, to build software file codes"
	@echo "              : fei, to build FEI samples"
	@echo "              : snva, to build SNVA codes."
	@echo "              : parallel, to build parallel transcoding codes."
	@echo "              : vioa, to build VIOA codes."
	@echo "              : on_demand_server, to build On Demand Server."
	@echo "              : rtsp_server, to build output RTSP Server."
	@echo "              : smta, to build SMTA."
	@echo "              : smta_audio, to build SMTA Audio Library."
	@echo "              : webtv, to build webtv code."
	@echo "              : clean, to clean the builds except imports directory"
	@echo "              : distclean, to clean all the builds"
	@echo "       ver=release/debug, release or debug version to build"
	@echo "Example:"
	@echo "       \"make vcsa ver=debug\", to build VCSA debug version"

asta:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir asta ASTA_CONFIG_ENABLE=1; \
	done

vcsa:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vcsa RENDER_ENABLE=1 LIBDISPATCH_ENABLE=1; \
	done

adec:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir adec; \
	done

vcsa_audio:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vcsa_audio; \
	done

vcsa_video:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vcsa_video RENDER_ENABLE=1 LIBDISPATCH_ENABLE=1; \
	done

xcoder:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir xcoder RAW_ENABLE=1 STR_ENABLE=1; \
	done

xcoder_sw:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir xcoder_sw RAW_ENABLE=1 SW_ENABLE=1; \
	done

fei:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir fei; \
	done

snva:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir snva VA_ENABLE=1; \
	done

parallel:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir parallel VA_ENABLE=1; \
	done

vioa:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vioa; \
	done

on_demand_server:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir on_demand_server; \
	done

rtsp_server:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir rtsp_server; \
	done

smta:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir smta ADEC_ENABLE=1  LIBMFX_ENABLE=1 ver=${ver}; \
	done

smta_audio:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir smta_audio ADEC_ENABLE=1 LIBDISPATCH_ENABLE=1; \
	done

webtv:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir webtv LIBDISPATCH_ENABLE=1; \
	done

distclean:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir clean; \
	done

clean:
	@for dir in $(SRCS); \
		do $(MAKE) -C $$dir clean; \
	done
