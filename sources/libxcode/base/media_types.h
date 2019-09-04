/*
 * 
 *   Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
 * 
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 * 
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *   Contact Information:
 *   Intel Corporation
 * 
 * 
 *  version: SMTA.A.0.9.4-2
 */
#ifndef MEDIA_TYPES_H
#define MEDIA_TYPES_H

#include <cstdlib>

typedef enum {
    ELEMENT_MODE_UNKNOWN,
    ELEMENT_MODE_ACTIVE,
    ELEMENT_MODE_PASSIVE
} ElementMode;

typedef enum {
    ELEMENT_UNKNOWN,
    ELEMENT_DECODER,
    ELEMENT_ENCODER,
    ELEMENT_VPP,
    ELEMENT_VA,
    ELEMENT_VP8_DEC, //vp8 sw decoder
    ELEMENT_VP8_ENC, //vp8 sw encoder
    ELEMENT_SW_DEC,
    ELEMENT_SW_ENC,
    ELEMENT_YUV_WRITER,
    ELEMENT_YUV_READER,
    ELEMENT_STRING_DEC,
    ELEMENT_BMP_DEC,
    ELEMENT_RENDER,
    ELEMENT_CUSTOM_PLUGIN,
    ELEMENT_HEVC_ENC,
    ELEMENT_AVC_FEI_ENCODER,
    ELEMENT_HEVC_FEI_ENCODER
} ElementType;

typedef struct MediaBuf{
    //for trace
    unsigned long mDecStartTime;
    unsigned long mIsFirstFrame;

    unsigned long mDecOutTime;
    unsigned long mDecToVpp;
    unsigned long mVppTime;
    unsigned long mVppToEnc;
    unsigned long mEncTime;
    unsigned long mDecToEnc;

    unsigned short mWidth;
    unsigned short mHeight;
    void* msdk_surface;
    void* ext_opaque_alloc;

    unsigned is_key_frame;

    // audio element
    unsigned char* payload;
    unsigned int payload_length;
    bool isFirstPacket;
    int  is_eos;
    bool is_decoder_reset;

    MediaBuf():
    mDecStartTime(0),
    mIsFirstFrame(0),
    mDecOutTime(0),
    mDecToVpp(0),
    mVppTime(0),
    mVppToEnc(0),
    mEncTime(0),
    mDecToEnc(0),
    mWidth(0),
    mHeight(0),
    msdk_surface(NULL),
    ext_opaque_alloc(NULL),
    is_key_frame(0),
    payload(NULL),
    payload_length(0),
    isFirstPacket(false),
    is_eos(0),
    is_decoder_reset(false) {
    };
} MediaBuf;
#endif
