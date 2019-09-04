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

#include <assert.h>
#include "msdk_codec.h"
#include "element_common.h"

BaseDecoderElement::BaseDecoderElement()
{
    mfx_session_ = NULL;
    m_pMFXAllocator = NULL;
    codec_init_ = false;
    surface_pool_ = NULL;
    num_of_surf_ = 0;
    callback_ = NULL;
    measurement = NULL;
    m_bUseOpaqueMemory = false;
    m_bLibType = MFX_IMPL_HARDWARE_ANY;

    memset(&ext_opaque_alloc_, 0, sizeof(ext_opaque_alloc_));
    memset(&mfx_video_param_, 0, sizeof(mfx_video_param_));
}
void BaseDecoderElement::initialize(ElementType type,
                                    MFXVideoSession *session,
                                    MFXFrameAllocator *pMFXAllocator,
                                    CodecEventCallback *callback)
{
    element_type_ = type;
    mfx_session_ = session;
    m_pMFXAllocator = pMFXAllocator;
    callback_ = callback;

    if (m_pMFXAllocator) {
        m_bUseOpaqueMemory = false;
    } else {
        m_bUseOpaqueMemory = true;
    }
}

void BaseDecoderElement::recycle(MediaBuf &buf)
{
    //when this assert happens, check if you return the buf the second time or to incorrect element.
    assert(surface_pool_);
    if (buf.msdk_surface != NULL) {
        unsigned int i;
        for (i = 0; i < num_of_surf_; i++) {
            if (surface_pool_[i] == buf.msdk_surface) {
                mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
                msdk_atomic_dec16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
                break;
            }
        }
        //or else the buf is not owned by this element, why did you return it to this element?
        assert(i < num_of_surf_);
    }
}

int BaseDecoderElement::GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool,
                                   mfxU16 nPoolSize)
{
    if (pSurfacesPool) {
        for (mfxU16 i = 0; i < nPoolSize; i++) {
            if (0 == pSurfacesPool[i]->Data.Locked) {
                return i;
            }
        }
    }

    return MFX_ERR_NOT_FOUND;
}
mfxStatus BaseDecoderElement::AllocFrames(mfxFrameAllocRequest *pRequest, bool isDecAlloc)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU16 i;
    pRequest->NumFrameMin = num_of_surf_;
    pRequest->NumFrameSuggested = num_of_surf_;
    //mfxFrameAllocResponse *pResponse = isDecAlloc ? &m_mfxDecResponse : &m_mfxEncResponse;
    mfxFrameAllocResponse *pResponse = &m_mfxDecResponse;

    if (m_bLibType == MFX_IMPL_SOFTWARE)
        pRequest->Type |= MFX_MEMTYPE_SYSTEM_MEMORY;
    if (!m_bUseOpaqueMemory) {
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, pRequest, pResponse);

        if (sts != MFX_ERR_NONE) {
            //MLOG_ERROR("AllocFrame failed %d\n", sts);
            return sts;
        }
    }
    surface_pool_ = new mfxFrameSurface1*[num_of_surf_];

    for (i = 0; i < num_of_surf_; i++) {
        surface_pool_[i] = new mfxFrameSurface1;
        memset(surface_pool_[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(surface_pool_[i]->Info), &(pRequest->Info), sizeof(mfxFrameInfo));

        if (!m_bUseOpaqueMemory) {
            surface_pool_[i]->Data.MemId = pResponse->mids[i];
        }
    }
    return MFX_ERR_NONE;
}

// BaseEncoderElement implement
BaseEncoderElement::BaseEncoderElement()
{
    mfx_session_ = NULL;
    m_pMFXAllocator = NULL;
    codec_init_ = false;
    output_stream_ = NULL;
    callback_ = NULL;
    measurement = NULL;
    m_bUseOpaqueMemory = false;
    m_bLibType = MFX_IMPL_HARDWARE_ANY;
    memset(&ext_opaque_alloc_, 0, sizeof(ext_opaque_alloc_));
    memset(&mfx_video_param_, 0, sizeof(mfx_video_param_));
}
void BaseEncoderElement::initialize(ElementType type,
                                    MFXVideoSession *session,
                                    MFXFrameAllocator *pMFXAllocator,
                                    CodecEventCallback *callback)
{
    element_type_ = type;
    mfx_session_ = session;
    m_pMFXAllocator = pMFXAllocator;
    callback_ = callback;

    if (m_pMFXAllocator) {
        m_bUseOpaqueMemory = false;
    } else {
        m_bUseOpaqueMemory = true;
    }
}

void BaseEncoderElement::SetBitrate(unsigned short bitrate)
{
    mfx_video_param_.mfx.MaxKbps = mfx_video_param_.mfx.TargetKbps = bitrate;
}
mfxStatus BaseEncoderElement::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
        Stream *out_stream)
{
#if defined SUPPORT_SMTA
    mfxU32 nBytesWritten = (mfxU32)out_stream->WriteBlockEx(
                               pMfxBitstream->Data + pMfxBitstream->DataOffset,
                               pMfxBitstream->DataLength, false,
                               pMfxBitstream->FrameType,
                               pMfxBitstream->TimeStamp,
                               pMfxBitstream->DecodeTimeStamp);
#else
    mfxU32 nBytesWritten = (mfxU32)out_stream->WriteBlock(
                               pMfxBitstream->Data + pMfxBitstream->DataOffset,
                               pMfxBitstream->DataLength);
#endif

    if (nBytesWritten != pMfxBitstream->DataLength) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    pMfxBitstream->DataLength = 0;
    return MFX_ERR_NONE;
}

mfxStatus BaseEncoderElement::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
        FILE *fSink)
{
    mfxU32 nBytesWritten = (mfxU32)fwrite(
                               pMfxBitstream->Data + pMfxBitstream->DataOffset,
                               1, pMfxBitstream->DataLength, fSink);

    if (nBytesWritten != pMfxBitstream->DataLength) {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    pMfxBitstream->DataLength = 0;
    return MFX_ERR_NONE;
}


