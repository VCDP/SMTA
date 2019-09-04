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
EXCLUDE_DIRS=gpac
DIRS:=$(shell find . -maxdepth 1 -type d)
DIRS:=$(basename $(patsubst ./%,%,$(DIRS)))
DIRS:=$(filter-out $(EXCLUDE_DIRS),$(DIRS))

ASTA_DEP=freetype2 #gpac
VCSA_WAV_DEP=libvpx speex freetype2
VCSA_AUD_DEP=speex
VCSA_VID_DEP=libvpx freetype2
ADEC_DEP=ipp speex
XCODER_DEP=live555 freetype2
XCODER_SW_DEP=ffmpeg
SNVA_DEP=ffmpeg opencv
PARALLEL_DEP=ffmpeg opencv
VIOA_DEP=libvpx freetype2
ON_DEMAND_SERVER_DEP=live555
SMTA=ffmpeg
RTSP_SERVER=live555
SMTA_AUDIO=ipp
WEBTV_DEP=ffmpeg

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
	@echo "make smta_audio"
	@echo "make webtv"

asta:$(ASTA_DEP)
	$(MAKE) -C ffmpeg asta
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

fei:
	@echo "fei feature"

vcsa:$(VCSA_WAV_DEP)
	$(MAKE) -C ffmpeg vcsa
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

vcsa_audio:$(VCSA_AUD_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

vcsa_video:$(VCSA_VID_DEP)
	$(MAKE) -C ffmpeg vcsa
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

adec:$(ADEC_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

xcoder:$(XCODER_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

xcoder_sw:$(XCODER_SW_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir xcoder_sw; \
	done

snva:$(SNVA_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir snva; \
	done

parallel:$(PARALLEL_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir snva; \
	done

vioa:$(VIOA_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

on_demand_server:$(ON_DEMAND_SERVER_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done
	@echo "Building On Demand Server Done"

rtsp_server:$(RTSP_SERVER)
	#$(MAKE) -C ffmpeg smta ver=${ver};
	@for dir in $^; \
		do $(MAKE) -C $$dir ; \
	done
	@echo "Building SMTA"
smta:$(SMTA)
	$(MAKE) -C ffmpeg smta ver=${ver};
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done
	@echo "Building SMTA"

smta_audio:$(SMTA_AUDIO)
	@for dir in $^; \
		do $(MAKE) -C $$dir ; \
	done
	@echo "Building SMTA AUDIO"

webtv:$(WEBTV_DEP)
	$(MAKE) -C ffmpeg webtv
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done
	@echo "Building WebTV"

clean:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir clean; \
	done
