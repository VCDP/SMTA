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

#include "element_factory.h"
#include "decoder_element.h"
#include "encoder_element.h"
#include "vpp_element.h"

#ifdef ENABLE_VPX_CODEC
#include "vp8_decoder_element.h"
#include "vp8_encoder_element.h"
#endif

#ifdef ENABLE_STRING_CODEC
#include "string_decoder_element.h"
#endif

#ifdef ENABLE_PICTURE_CODEC
#include "picture_decoder_element.h"
#endif

#ifdef ENABLE_RAW_DECODE
#include "yuv_reader_element.h"
#endif

#ifdef ENABLE_SW_CODEC
#include "sw_decoder_element.h"
#include "sw_encoder_element.h"
#endif

#ifdef ENABLE_RAW_CODEC
#include "yuv_writer_element.h"
#endif

#ifdef ENABLE_VA
#include "va_element.h"
#endif

#if defined(LIBVA_X11_SUPPORT) || defined(LIBVA_DRM_SUPPORT)
#include "render_element.h"
#endif

#ifdef MSDK_AVC_FEI
#include "h264_fei/h264_fei_encoder_element.h"
#endif

#ifdef MSDK_HEVC_FEI
#include "h265_fei/h265_fei_encoder_element.h"
#endif

#include "hevc_encoder_element.h"

DEFINE_MLOGINSTANCE_CLASS(ElementFactory, "ElementFactory");
MSDKCodec* ElementFactory::createElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator, CodecEventCallback *callback)
{
    MSDKCodec* element = NULL;

    switch (type) {
    case ELEMENT_DECODER:
        element = new DecoderElement(type, session, pMFXAllocator, callback);
        break;
    case ELEMENT_ENCODER:
        element = new EncoderElement(type, session, pMFXAllocator);
        break;
    case ELEMENT_VPP:
        element = new VPPElement(type, session, pMFXAllocator);
        break;
    case ELEMENT_VA:
#ifdef ENABLE_VA
         element = new VAElement(type, session, pMFXAllocator);
#else
         MLOG_INFO("element type ELEMENT_VA is not supported.\n");
#endif
         break;
    case ELEMENT_VP8_DEC:
#ifdef ENABLE_VPX_CODEC
        element = new VP8DecoderElement(type, session, pMFXAllocator, callback);
#else
        MLOG_INFO("element type ELEMENT_VP8_DEC is not supported.\n");
#endif
        break;
    case ELEMENT_VP8_ENC:
#ifdef ENABLE_VPX_CODEC
        element = new VP8EncoderElement(type, session, pMFXAllocator);
#else
        MLOG_INFO("element type ELEMENT_VP8_ENC is not supported.\n");
#endif
        break;
    case ELEMENT_SW_DEC:
#ifdef ENABLE_SW_CODEC
        element = new SWDecoderElement(type, session, pMFXAllocator, callback);
#else
        MLOG_INFO("element type ELEMENT_SW_DEC is not supported.\n");
#endif
        break;
    case ELEMENT_SW_ENC:
#ifdef ENABLE_SW_CODEC
        element = new SWEncoderElement(type, session, pMFXAllocator);
#else
        MLOG_INFO("element type ELEMENT_SW_ENC is not supported.\n");
#endif
        break;
    case ELEMENT_YUV_WRITER:
#ifdef ENABLE_RAW_CODEC
        element = new YUVWriterElement(type, session, pMFXAllocator);
#else
        MLOG_INFO("element type ELEMENT_YUV_WRITER is not supported.\n");
#endif
        break;
    case ELEMENT_YUV_READER:
#ifdef ENABLE_RAW_DECODE
        element = new YUVReaderElement(type, session, pMFXAllocator);
#else
        MLOG_INFO("element type ELEMENT_YUV_READER is not supported.\n");
#endif
        break;
    case ELEMENT_STRING_DEC:
#ifdef ENABLE_STRING_CODEC
        element = new StringDecoderElement(type, session, pMFXAllocator);
#else
        MLOG_INFO("element type ELEMENT_STRING_DEC is not supported.\n");
#endif
        break;
    case ELEMENT_BMP_DEC:
#ifdef ENABLE_PICTURE_CODEC
        element = new PictureDecoderElement(type, session, pMFXAllocator);
#else
        MLOG_INFO("element type ELEMENT_BMP_DEC is not supported.\n");
#endif
        break;
    case ELEMENT_RENDER:
#if defined(LIBVA_X11_SUPPORT) || defined(LIBVA_DRM_SUPPORT)
        element = new RenderElement(type, session, pMFXAllocator);
#else
#endif
        break;
    case ELEMENT_CUSTOM_PLUGIN:
        break;
#if defined(ENABLE_HEVC)
    case ELEMENT_HEVC_ENC:
        element = new HevcEncoderElement(type, session, pMFXAllocator);
        break;
#endif
#ifdef MSDK_AVC_FEI
    case ELEMENT_AVC_FEI_ENCODER:
        element = new H264FEIEncoderElement(type, session, pMFXAllocator);
        break;
#endif
#ifdef MSDK_HEVC_FEI
    case ELEMENT_HEVC_FEI_ENCODER:
        element = new H265FEIEncoderElement(type, session, pMFXAllocator);
        break;
#endif
    default:
        MLOG_INFO("current element type is unknow.\n");
        break;
    }
    return element;
}
