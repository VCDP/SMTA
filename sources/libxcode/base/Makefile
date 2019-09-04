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

ifeq ($(SW_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_SW_CODEC
endif

ifeq ($(VA_ENABLE),1)
EXTRA_FLAGS+= -DENABLE_VA
endif

ifeq ($(SMTA_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_RAW_DECODE
endif

LIBS+=-lpthread -lrt
include ../common.mk
