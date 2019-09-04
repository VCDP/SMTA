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

#include "yuv_writer_element.h"
#ifdef ENABLE_RAW_CODEC
#include <assert.h>
#include <math.h>
#include <stdio.h>
#if defined (PLATFORM_OS_LINUX)
#include <unistd.h>
#endif
#include "base/partially_linear_bitrate.h"
#include "element_common.h"

DEFINE_MLOGINSTANCE_CLASS(YUVWriterElement, "YUVWriterElement");

unsigned int YUVWriterElement::mNumOfEncChannels = 0;

YUVWriterElement::YUVWriterElement(ElementType type,
                                   MFXVideoSession *session,
                                   MFXFrameAllocator *pMFXAllocator,
                                   CodecEventCallback *callback)
{
    initialize(type, session, pMFXAllocator);
    user_enc_ = NULL;

    memset(&output_bs_, 0, sizeof(output_bs_));
    //memset(&m_mfxEncResponse, 0, sizeof(m_mfxEncResponse));
    m_nFramesProcessed = 0;
}

YUVWriterElement::~YUVWriterElement()
{
    if (user_enc_) {
        user_enc_->Close();
        MFXVideoUSER_Unregister(*mfx_session_, 0);
        delete user_enc_;
        user_enc_ = NULL;
    }

    if (!m_EncExtParams.empty()) {
        m_EncExtParams.clear();
    }

    if (NULL != output_bs_.Data) {
        delete[] output_bs_.Data;
        output_bs_.Data = NULL;
    }
}

bool YUVWriterElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    measurement = ele_param->measurement;
    user_enc_ = new YUVWriterPlugin();
    mfxPlugin enc_plugin = make_mfx_plugin_adapter((MFXGenericPlugin*)user_enc_);
    MFXVideoUSER_Register(*mfx_session_, 0, &enc_plugin);
    mfx_video_param_.AsyncDepth = 1;
    output_stream_ = ele_param->output_stream;
    return true;
}

mfxStatus YUVWriterElement::InitEncoder(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest EncRequest;
    mfxVideoParam par;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(ENC_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    mfx_video_param_.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    mfx_video_param_.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;

    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        mfx_video_param_.mfx.FrameInfo.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        mfx_video_param_.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }
    mfx_video_param_.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    mfx_video_param_.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    mfx_video_param_.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    mfx_video_param_.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    mfx_video_param_.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    mfx_video_param_.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
    mfx_video_param_.mfx.FrameInfo.Width         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width;
    mfx_video_param_.mfx.FrameInfo.Height        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height;

    if ((mfx_video_param_.mfx.RateControlMethod != MFX_RATECONTROL_CQP) && \
            !mfx_video_param_.mfx.TargetKbps) {
        mfx_video_param_.mfx.TargetKbps = CalDefBitrate(mfx_video_param_.mfx.CodecId, \
            mfx_video_param_.mfx.TargetUsage, \
            mfx_video_param_.mfx.FrameInfo.Width, \
            mfx_video_param_.mfx.FrameInfo.Height, \
            1.0 * mfx_video_param_.mfx.FrameInfo.FrameRateExtN / \
            mfx_video_param_.mfx.FrameInfo.FrameRateExtD);
        MLOG_WARNING(" invalid setup bitrate,use default %d Kbps\n", \
               mfx_video_param_.mfx.TargetKbps);
    }

    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
        //opaue alloc
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(ext_opaque_alloc_);
        memcpy(&ext_opaque_alloc_.In, \
               & ((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out, \
               sizeof(((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out));
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));

    } else {

        MLOG_INFO("------------Encoder using video memory\n");
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    //set mfx video ExtParam
    if (!m_EncExtParams.empty()) {
        mfx_video_param_.ExtParam = &m_EncExtParams.front(); // vector is stored linearly in memory
        mfx_video_param_.NumExtParam = (mfxU16)m_EncExtParams.size();
        MLOG_INFO("init encoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
    }

    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = user_enc_->QueryIOSurf(&mfx_video_param_, \
                &EncRequest, NULL);

    MLOG_INFO("Enc suggest surfaces %d\n", EncRequest.NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    memset(&par, 0, sizeof(par));

    sts = user_enc_->Init(&mfx_video_param_);
    user_enc_->SetAllocPoint(m_pMFXAllocator);
    sts = user_enc_->GetVideoParam(&par);

    // Prepare Media SDK bit stream buffer for encoder
    memset(&output_bs_, 0, sizeof(output_bs_));
    output_bs_.MaxLength = par.mfx.BufferSizeInKB * 2000;
    output_bs_.Data = new mfxU8[output_bs_.MaxLength];
    MLOG_INFO("output bitstream buffer size %d\n", output_bs_.MaxLength);

    m_EncExtParams.clear();
    m_nFramesProcessed = 0;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(ENC_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    return sts;
}

int YUVWriterElement::ProcessChainEncode(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;
    mfxSyncPoint syncpE;
    //mfxEncodeCtrl *enc_ctrl = NULL;

    if (!codec_init_) {
        if (buf.msdk_surface == NULL && buf.is_eos) {
            //The first buf encoder received is eos
            return -1;
        }
        sts = InitEncoder(buf);

        if (MFX_ERR_NONE == sts) {
            codec_init_ = true;

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(ENC_ENDURATION_TIME_STAMP, this);
                pipelineinfo einfo;
                einfo.mElementType = element_type_;
                einfo.mChannelNum = YUVWriterElement::mNumOfEncChannels;
                YUVWriterElement::mNumOfEncChannels++;

                einfo.mCodecName = "YUV";
                measurement->SetElementInfo(ENC_ENDURATION_TIME_STAMP, this, &einfo);
                measurement->RelLock();
            }

            MLOG_INFO("Encoder %p init successfully\n", this);
        } else {
            MLOG_ERROR("encoder create failed %d\n", sts);
            return -1;
        }
    }

    // to check if there is res change in incoming surface
    if (pmfxInSurface) {
        if (mfx_video_param_.mfx.FrameInfo.Width  != pmfxInSurface->Info.Width ||
            mfx_video_param_.mfx.FrameInfo.Height != pmfxInSurface->Info.Height) {
            mfx_video_param_.mfx.FrameInfo.Width   = pmfxInSurface->Info.Width;
            mfx_video_param_.mfx.FrameInfo.Height  = pmfxInSurface->Info.Height;
            mfx_video_param_.mfx.FrameInfo.CropW   = pmfxInSurface->Info.CropW;
            mfx_video_param_.mfx.FrameInfo.CropH   = pmfxInSurface->Info.CropH;

            //reset_res_flag_ = true;
            MLOG_INFO(" to reset encoder res as %d x %d\n",
                   mfx_video_param_.mfx.FrameInfo.CropW, mfx_video_param_.mfx.FrameInfo.CropH);
        }

        //mark the frame order for incoming surface.
        pmfxInSurface->Data.FrameOrder = m_nFramesProcessed;
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(ENC_FRAME_TIME_STAMP, this);
        measurement->RelLock();
    }

    do {
        while (is_running_) {

            sts = MFXVideoUSER_ProcessFrameAsync(*mfx_session_,
                                                (mfxHDL *) &pmfxInSurface, 1, (mfxHDL *) &output_bs_, 1, &syncpE);

            if (MFX_ERR_NONE < sts && !syncpE) {
                if (MFX_WRN_DEVICE_BUSY == sts) {
                    usleep(20);
                }
            } else if (MFX_ERR_NONE < sts && syncpE) {
                // ignore warnings if output is available
                sts = MFX_ERR_NONE;
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else {
                break;
            }
        }

        //Klockwork issue #531: if is_running_ == false in this do-while loop, syncpE is uninitialized.
        if (MFX_ERR_NONE == sts && is_running_) {
            sts = mfx_session_->SyncOperation(syncpE, 60000);

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(ENC_FRAME_TIME_STAMP, this);
                measurement->RelLock();
            }
            if (output_stream_) {
                sts = WriteBitStreamFrame(&output_bs_, output_stream_);
            }

            //OnFrameEncoded(&output_bs_);
            m_nFramesProcessed++;
        }

        if (is_running_ == false) {
            //break the loop
            break;
        }
    } while (pmfxInSurface == NULL && sts >= MFX_ERR_NONE);

    if (NULL == pmfxInSurface && buf.is_eos && output_stream_ != NULL) {
        output_stream_->SetEndOfStream();

        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpFinish(ENC_ENDURATION_TIME_STAMP, this);
            measurement->RelLock();
        }

        MLOG_INFO("Got EOF in Encoder %p\n", this);
        is_running_ = false;
    }

    //MLOG_INFO("leave ProcessChainEncode\n");
    return 0;
}

int YUVWriterElement::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    if (buf.msdk_surface == NULL && buf.is_eos) {
        MLOG_INFO("Got EOF in element %p, type is %d\n", this, element_type_);
    }
    return ProcessChainEncode(buf);
}
int YUVWriterElement::HandleProcessEncode()
{
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(sinkpad != *(this->sinkpads_.end()));

    while (is_running_) {
        if (sinkpad->GetBufData(buf) != 0) {
            // No data, just sleep and wait
            usleep(100);
            continue;
        }

        int ret = ProcessChain(sinkpad, buf);
        if (ret == 0) {
            sinkpad->ReturnBufToPeerPad(buf);
        }
    }

    if (!is_running_) {
        while (sinkpad->GetBufData(buf) == 0) {
            sinkpad->ReturnBufToPeerPad(buf);
        }
    } else {
        is_running_ = false;
    }

    return 0;
}
int YUVWriterElement::HandleProcess()
{
    return HandleProcessEncode();
}
#endif //ENABLE_RAW_CODEC
