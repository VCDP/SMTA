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
EXCLUDE_DIRS =
DIRS := $(shell find . -maxdepth 1 -type d)
DIRS := $(basename $(patsubst ./%,%,$(DIRS)))
DIRS := $(filter-out $(EXCLUDE_DIRS),$(DIRS))

STORM=../test/va/StormApplication ../test/wrapper
STORM_PARALLEL=../test/ParallelTranscodingTopology/
DIRS += ${STORM} ${STORM_PARALLEL}

ASTA_SAMPLES=msdk_dash_xcoder
VCSA_SAMPLES=msdk_comp_sample
WEBTV_SAMPLES=msdk_screencapture_sample
AUDIO_SAMPLES=audio_multi_mixer audio_file_xcoder
XCODER_SAMPLES=msdk_file_xcoder msdk_stream_xcoder msdk_mix_xcoder
XCODER_SW_SAMPLES=msdk_sw_xcoder
FEI_SAMPLES=msdk_video_indexer adaptive_encoder
SNVA_SAMPLES=msdk_va_xcoder $(STORM)
PARALLEL_SAMPLES=msdk_parallel_transcoding $(STORM_PARALLEL)
VIOA_SAMPLES=msdk_alphablend_sample
ON_DEMAND_SERVER_SAMPLES=rtsp_server
RTSP_SERVER=smta_rtsp_server smta_rtsp_server_1ton
SMTA=msdk_smta

help:
	@echo "make asta"
	@echo "make vcsa"
	@echo "make vcsa_audio"
	@echo "make vcsa_video"
	@echo "make adec"
	@echo "make xcoder"
	@echo "make xcoder_sw"
	@echo "make fei"
	@echo "make snva"
	@echo "make parallel"
	@echo "make vioa"
	@echo "make on_demand_server"
	@echo "make rtsp_server"
	@echo "make smta"
	@echo "make webtv"


asta:$(ASTA_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir  STR_ENABLE=1 PIC_ENABLE=1; \
	done

vcsa:$(VCSA_SAMPLES) $(AUDIO_SAMPLES) $(VIOA_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir ADEC_ENABLE=0; \
	done

vcsa_audio:$(AUDIO_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir ADEC_ENABLE=0; \
	done

vcsa_video:$(VCSA_SAMPLES) $(VIOA_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir ADEC_ENABLE=0;\
	done

adec:$(AUDIO_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir ADEC_ENABLE=1; \
	done

snva:$(SNVA_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

parallel:$(PARALLEL_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

xcoder:$(XCODER_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

xcoder_sw:$(XCODER_SW_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

vioa:$(VIOA_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

fei:$(FEI_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

on_demand_server:$(ON_DEMAND_SERVER_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

rtsp_server:$(RTSP_SERVER)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

smta:$(SMTA)
	@for dir in $^; \
		do $(MAKE) -C $$dir AVC_FEI_ENABLE=1 HEVC_ENABLE=1; \
	done

webtv:$(WEBTV_SAMPLES)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

all: asta vcsa xcoder fei snva vioa smta webtv

clean:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir clean; \
	done
