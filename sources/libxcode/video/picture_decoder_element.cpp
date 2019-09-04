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
#include "picture_decoder_element.h"
#ifdef ENABLE_PICTURE_CODEC
#include <assert.h>
#include <unistd.h>
#include "element_common.h"

DEFINE_MLOGINSTANCE_CLASS(PictureDecoderElement, "PictureDecoderElement");
PictureDecoderElement::PictureDecoderElement(ElementType type,
                                             MFXVideoSession *session,
                                             MFXFrameAllocator *pMFXAllocator,
                                             CodecEventCallback *callback)
{
    initialize(type, session, pMFXAllocator);
    input_pic_ = NULL;
    user_dec_ = NULL;
    is_eos_ = false;
}
PictureDecoderElement::~PictureDecoderElement()
{
    if (user_dec_) {
        user_dec_->Close();
        MFXVideoUSER_Unregister(*mfx_session_, 0);
        delete user_dec_;
        user_dec_ = NULL;
    }

    if (!m_DecExtParams.empty()) {
        m_DecExtParams.clear();
    }

    if (surface_pool_) {
        for (unsigned int i = 0; i < num_of_surf_; i++) {
            if (surface_pool_[i]) {
                delete surface_pool_[i];
                surface_pool_[i] = NULL;
            }
        }
        delete surface_pool_;
        surface_pool_ = NULL;
    }

    if (m_pMFXAllocator && !m_bUseOpaqueMemory ) {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxDecResponse);
        m_pMFXAllocator = NULL;
    }
}

bool PictureDecoderElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        MLOG_ERROR("Picture decoder initialize params is null\n");
        return false;
    }

    measurement = ele_param->measurement;
    user_dec_ = new PicDecPlugin();
    mfxPlugin dec_plugin = make_mfx_plugin_adapter((MFXGenericPlugin*)user_dec_);
    MFXVideoUSER_Register(*mfx_session_, 0, &dec_plugin);

    if (!m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    }

    mfx_video_param_.AsyncDepth = 1;
    input_pic_ = ele_param->input_pic;

    return true;
}

int PictureDecoderElement::Recycle(MediaBuf &buf)
{
    recycle(buf);

    if (user_dec_) {
        if (buf.is_eos) {
           MLOG_INFO("Now set EOS to bitmap decoder.\n");
           is_eos_ = true;
        }
    }

    return 0;
}
mfxStatus PictureDecoderElement::InitDecoder()
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = user_dec_->DecodeHeader(input_pic_, &mfx_video_param_);
    if (MFX_ERR_NONE != sts) {
        MLOG_ERROR("DecodeHeader return %d, failed.\n", sts);
        return sts;
    }
    mfxFrameAllocRequest DecRequest;
    sts = user_dec_->QueryIOSurf(&mfx_video_param_, NULL, &DecRequest);

    if (MFX_ERR_NONE != sts) {
        MLOG_ERROR("QueryIOSurf return %d, failed.\n", sts);
        return sts;
    }

    num_of_surf_ = DecRequest.NumFrameSuggested + 10;
    sts = AllocFrames(&DecRequest, true);

    if (m_bUseOpaqueMemory) {
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
        mfxExtBuffer *pExtParamsDec = (mfxExtBuffer *)&ext_opaque_alloc_;
        ext_opaque_alloc_.Out.Surfaces = surface_pool_;
        ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
        ext_opaque_alloc_.Out.Type = DecRequest.Type;
        mfx_video_param_.ExtParam = &pExtParamsDec;
        mfx_video_param_.NumExtParam = 1;
    }

    // Initialize the picture decoder
    user_dec_->SetAllocPoint(m_pMFXAllocator);
    sts = user_dec_->Init(&mfx_video_param_);

    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("decoder init failed. return %d\n", sts);
    }

    return sts;
}

int PictureDecoderElement::HandleProcessDecode()
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf buf;
    MediaPad *srcpad = *(this->srcpads_.begin());
    mfxSyncPoint syncpD;
    //mfxFrameSurface1 *pmfxOutSurface = NULL;
    //bool bEndOfDecoder = false;

    while (is_running_) {
        usleep(100);

        if (!codec_init_) {
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(DEC_INIT_TIME_STAMP, this);
                measurement->RelLock();
            }

            sts = InitDecoder();

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(DEC_INIT_TIME_STAMP, this);
                measurement->RelLock();
            }

            if (MFX_ERR_MORE_DATA == sts) {
                continue;
            } else if (MFX_ERR_NONE == sts) {
                codec_init_ = true;

                if (measurement) {
                    measurement->GetLock();
                    pipelineinfo einfo;
                    einfo.mElementType = element_type_;
                    einfo.mChannelNum = PictureDecoderElement::mNumOfDecChannels;
                    PictureDecoderElement::mNumOfDecChannels++;
                    //if (input_mp_) {
                    //    einfo.mInputName.append(input_mp_->GetInputName());
                    //}
                    measurement->SetElementInfo(DEC_ENDURATION_TIME_STAMP, this, &einfo);
                    measurement->RelLock();
                }
            } else {
                MLOG_ERROR("Decode create failed: %d\n", sts);
                return -1;
            }
        }

        // Read es stream into bit stream
        int nIndex = 0;

        while (is_running_) {
            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);
            if (MFX_ERR_NOT_FOUND == nIndex) {
                usleep(10000);
            } else {
                break;
            }
        }

        // Check again in case for a invalid surface.
        if (!is_running_) {
            break;
        }

        if (is_eos_) {
            MLOG_INFO("bitmap decoder needs to stop now.\n");
            break;
        }

        sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                         (mfxHDL*)&input_pic_, 1, (mfxHDL*)&surface_pool_[nIndex], 1, &syncpD);

        if (MFX_ERR_NONE < sts && syncpD) {
            sts = MFX_ERR_NONE;
        }
        if (MFX_ERR_NONE == sts) {
            buf.msdk_surface = surface_pool_[nIndex];

            if (buf.msdk_surface) {
                mfxFrameSurface1* msdk_surface =
                 (mfxFrameSurface1*)buf.msdk_surface;
                msdk_atomic_inc16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
            }
            buf.mWidth = mfx_video_param_.mfx.FrameInfo.CropW;
            buf.mHeight = mfx_video_param_.mfx.FrameInfo.CropH;
            if (m_bUseOpaqueMemory) {
                buf.ext_opaque_alloc = &ext_opaque_alloc_;
            }
            sts = mfx_session_->SyncOperation(syncpD, 60000);
            if (MFX_ERR_NONE == sts) {
                srcpad->PushBufToPeerPad(buf);
            } else {
                if (buf.msdk_surface) {
                    mfxFrameSurface1* msdk_surface =
                     (mfxFrameSurface1*)buf.msdk_surface;
                    msdk_atomic_dec16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
                }
            }
        }
    }

    if (!is_running_) {
        buf.msdk_surface = NULL;
        buf.is_eos = 1;
        MLOG_INFO("This video stream will be stoped.\n");
        srcpad->PushBufToPeerPad(buf);
    } else {
        is_running_ = false;
    }

    return 0;
}

int PictureDecoderElement::HandleProcess()
{
    return HandleProcessDecode();
}
unsigned int PictureDecoderElement::mNumOfDecChannels = 0;
#endif //ENABLE_PICTURE_CODEC
