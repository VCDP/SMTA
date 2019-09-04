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

LSPCI=lspci
ifeq ("/usr/bin/lspci", "$(wildcard /usr/bin/lspci)")
    LSPCI=/usr/bin/lspci
else ifeq ("/sbin/lspci", "$(wildcard /sbin/lspci)")
    LSPCI=/sbin/lspci
endif

RELEASE_DIR=../../dist
#DEVICEID=$(shell $(LSPCI) -nn | grep VGA | grep Intel | cut -d: -f 4 | cut -d']' -f 1)
MEDIASDK_OUTPUT=/opt/intel
MFX_INCLUDE_PATH=$(MEDIASDK_OUTPUT)/mediasdk/include/mfx

DYN_TARGET= $(RELEASE_DIR)/StreamingMediaTranscoder

FFMPEG_LDLIBS_LIBS=-lavformat -lavcodec -lavutil


#since mss v16.4.2.1 PV, libdispatch_share.a should be used
ifeq ($(LIBDISPATCH_ENABLE),1)
    MEDIASDK_INSTALL_DIR=/opt/intel/mediasdk/lib/lin_x64
    MFX_LIBS=-L$(MEDIASDK_INSTALL_DIR) -ldispatch_shared -Wl,-rpath=$(MEDIASDK_INSTALL_DIR) -lva -lva-drm -lva-x11
    EXTRA_FLAGS+=-DMFX_DISPATCHER_EXPOSED_PREFIX
else ifeq ($(LIBMFX_ENABLE),1)
    MEDIASDK_INSTALL_DIR=/opt/intel/mediasdk/lib64
    MFX_LIBS=-L$(MEDIASDK_INSTALL_DIR) -lmfx -Wl,-rpath=$(MEDIASDK_INSTALL_DIR) -lva -lva-drm -lva-x11
    EXTRA_FLAGS+=-DMFX_LIBRARY_EXPOSED
else
    MEDIASDK_INSTALL_DIR=$(shell find $(MEDIASDK_OUTPUT)/mediasdk/lib64/ -name libmfxhw64.so -exec dirname {} \; | sed -n '1p')
    MFX_LIBS=-L$(MEDIASDK_INSTALL_DIR) -lmfxhw64 -Wl,-rpath=$(MEDIASDK_INSTALL_DIR) -lva -lva-drm -lva-x11
endif

ifeq ($(AVC_FEI_ENABLE),1)
EXTRA_FLAGS+= -DMSDK_AVC_FEI
endif
ifeq ($(HEVC_FEI_ENABLE),1)
EXTRA_FLAGS+= -DMSDK_HEVC_FEI
EXTRA_FLAGS+= -DMFX_VERSION_USE_LATEST
endif

SHARED_LIBS=-L../../dist/ $(FFMPEG_LDLIBS_PATH)\
            -Wl,-rpath='$$ORIGIN' \
            -Wl,-Bdynamic -lxcodevideo -lxcodebase $(FFMPEG_LDLIBS_LIBS) \
            $(MFX_LIBS) -ldl

STATIC_LIBS=-L../../dist/ $(FFMPEG_LDLIBS_PATH) -L$(MEDIASDK_INSTALL_DIR)\
            -Wl,-rpath='$$ORIGIN' \
            -Wl,-Bstatic -lxcodevideo -lxcodebase $(FFMPEG_LDLIBS_LIBS) \
            -Wl,-Bdynamic $(MFX_LIBS) -ldl


INCLUDES_PATH=-I$(MFX_INCLUDE_PATH) \
              -I ../../libxcode \
              -I../../dist/include \
              -I../../imports/platform \
              -I../../imports/platform/linux

CUSTOM_DEFINES =-D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS -DENABLE_RAW_DECODE -DSUPPORT_SMTA -DSUPPORT_INTERLACE

include ../common.mk

