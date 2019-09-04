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

#include "encoder_element.h"

#include <assert.h>
#if defined (PLATFORM_OS_LINUX)
#include <unistd.h>
#include <math.h>
#endif

#include "element_common.h"

DEFINE_MLOGINSTANCE_CLASS(EncoderElement, "EncoderElement");

unsigned int EncoderElement::mNumOfEncChannels = 0;

EncoderElement::EncoderElement(ElementType type,
                               MFXVideoSession *session,
                               MFXFrameAllocator *pMFXAllocator,
                               CodecEventCallback *callback)
{
    initialize(type, session, pMFXAllocator, callback);
    mfx_enc_ = NULL;

    memset(&output_bs_, 0, sizeof(output_bs_));
    //memset(&m_mfxEncResponse, 0, sizeof(m_mfxEncResponse));
    memset(&coding_opt, 0, sizeof(coding_opt));
    memset(&coding_opt2, 0, sizeof(coding_opt2));
    memset(&coding_opt3, 0, sizeof(coding_opt3));
    memset(&enc_ctrl_, 0, sizeof(enc_ctrl_));

    force_key_frame_ = false;
    reset_bitrate_flag_ = false;
    reset_res_flag_ = false;
    m_nFramesProcessed = 0;
    m_bMarkLTR = false;
    m_refNum = 0;
    m_nLastForceKeyFrame = 0;
    memset(&m_avcRefList, 0, sizeof(m_avcRefList));
    memset(&m_avcrefLists, 0, sizeof(m_avcrefLists));

#if (MFX_VERSION >= 1025)
    memset(&m_ExtMFEParam, 0, sizeof(m_ExtMFEParam));
    memset(&m_ExtMFEControl, 0, sizeof(m_ExtMFEControl));
#endif
}

EncoderElement::~EncoderElement()
{
    if (mfx_enc_) {
        mfx_enc_->Close();
        delete mfx_enc_;
        mfx_enc_ = NULL;
    }

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    //to unload vp8 encode plugin
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8) {
        mfxPluginUID uid = MFX_PLUGINID_VP8E_HW;
        MFXVideoUSER_UnLoad(*mfx_session_, &uid);
    }
#endif
    if (!m_EncExtParams.empty()) {
        m_EncExtParams.clear();
    }

    if (!m_CtrlExtParams.empty()) {
        m_CtrlExtParams.clear();
    }

    if (NULL != output_bs_.Data) {
        delete[] output_bs_.Data;
        output_bs_.Data = NULL;
    }
}

void EncoderElement::SetForceKeyFrame()
{
    force_key_frame_ = true;
}
void EncoderElement::SetResetBitrateFlag()
{
    reset_bitrate_flag_ = true;
}

bool EncoderElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    measurement = ele_param->measurement;
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    m_roiData = ele_param->extRoiData;
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    //load vp8 hybrid encode plugin
    if (ele_param->EncParams.mfx.CodecId == MFX_CODEC_VP8) {
        mfxPluginUID uid = MFX_PLUGINID_VP8E_HW;
        mfxStatus sts = MFXVideoUSER_Load(*mfx_session_, &uid, 1/*version*/);
        assert(sts == MFX_ERR_NONE);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("failed to load vp8 hybrid encode plugin\n");
            return false;
        }
    }
#endif
    mfx_enc_ = new MFXVideoENCODE(*mfx_session_);
    mfx_video_param_.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
    mfx_video_param_.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
    mfx_video_param_.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
    /* Encoding Options */
    mfx_video_param_.mfx.TargetUsage = ele_param->EncParams.mfx.TargetUsage;
    mfx_video_param_.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize==0? 30: ele_param->EncParams.mfx.GopPicSize;
    mfx_video_param_.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist==0? 4: ele_param->EncParams.mfx.GopRefDist;
    mfx_video_param_.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
    mfx_video_param_.mfx.RateControlMethod =
        (ele_param->EncParams.mfx.RateControlMethod > 0) ?
        ele_param->EncParams.mfx.RateControlMethod : MFX_RATECONTROL_VBR;

    if (mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
        mfx_video_param_.mfx.QPI = ele_param->EncParams.mfx.QPI;
        mfx_video_param_.mfx.QPP = ele_param->EncParams.mfx.QPP;
        mfx_video_param_.mfx.QPB = ele_param->EncParams.mfx.QPB;
    } else {
        mfx_video_param_.mfx.TargetKbps = ele_param->EncParams.mfx.TargetKbps;
    }

    mfx_video_param_.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    mfx_video_param_.mfx.NumRefFrame = ele_param->EncParams.mfx.NumRefFrame;
    //if (!mfx_video_param_.mfx.NumRefFrame)
    //    mfx_video_param_.mfx.NumRefFrame = 4;
    mfx_video_param_.mfx.EncodedOrder = ele_param->EncParams.mfx.EncodedOrder;
    mfx_video_param_.AsyncDepth = 1;
    extParams.bMBBRC = ele_param->extParams.bMBBRC;
    extParams.nLADepth = ele_param->extParams.nLADepth;
    extParams.nMbPerSlice = ele_param->extParams.nMbPerSlice;
    extParams.bRefType = ele_param->extParams.bRefType;
    extParams.nRepartitionCheckEnable =  ele_param->extParams.nRepartitionCheckEnable;
#if (MFX_VERSION >= 1025)
    extParams.nMFEFrames = ele_param->extParams.nMFEFrames;
    extParams.nMFEMode = ele_param->extParams.nMFEMode;
    extParams.nMFETimeout = ele_param->extParams.nMFETimeout;
#endif
    m_CtrlExtParams.clear();
    //if mfx_video_param_.mfx.NumSlice > 0, don't set MaxSliceSize and give warning.
    if (mfx_video_param_.mfx.NumSlice && ele_param->extParams.nMaxSliceSize) {
        MLOG_WARNING(" MaxSliceSize is not compatible with NumSlice, discard it\n");
        extParams.nMaxSliceSize = 0;
    } else {
        extParams.nMaxSliceSize = ele_param->extParams.nMaxSliceSize;
    }
    output_stream_ = ele_param->output_stream;

    m_bLibType = ele_param->libType;
    return true;
}

mfxStatus EncoderElement::InitEncoder(MediaBuf &buf)
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

    // Hack for MPEG2
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_MPEG2) {
        float result = mfx_video_param_.mfx.FrameInfo.FrameRateExtN;
        result = round(result / mfx_video_param_.mfx.FrameInfo.FrameRateExtD);
        mfx_video_param_.mfx.FrameInfo.FrameRateExtN = result;
        mfx_video_param_.mfx.FrameInfo.FrameRateExtD = 1;

        float frame_rate = mfx_video_param_.mfx.FrameInfo.FrameRateExtN / \
                           mfx_video_param_.mfx.FrameInfo.FrameRateExtD;
        if ((frame_rate != 24) && (frame_rate != 25) && (frame_rate != 30) && \
                (frame_rate != 50) && (frame_rate != 60)) {
            MLOG_WARNING(" mpeg2 encoder frame rate %f, use default 30fps\n", frame_rate);
            mfx_video_param_.mfx.FrameInfo.FrameRateExtN = 30;
            mfx_video_param_.mfx.FrameInfo.FrameRateExtD = 1;
        }
    }

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

    //extcoding options for instructing encoder to specify maxdecodebuffering
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC) {
        coding_opt.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
        coding_opt.Header.BufferSz = sizeof(coding_opt);
        coding_opt.MaxDecFrameBuffering = mfx_video_param_.mfx.NumRefFrame;
        coding_opt.AUDelimiter = MFX_CODINGOPTION_OFF;//No AUD
        coding_opt.RecoveryPointSEI = MFX_CODINGOPTION_OFF;//No SEI
        coding_opt.PicTimingSEI = MFX_CODINGOPTION_OFF;
        if (mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_CBR || \
                mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_VBR) {
            coding_opt.VuiNalHrdParameters = MFX_CODINGOPTION_ON;
        } else {
            coding_opt.VuiNalHrdParameters = MFX_CODINGOPTION_OFF;
        }
        coding_opt.VuiVclHrdParameters = MFX_CODINGOPTION_OFF;
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&coding_opt));
    }

    //extcoding option2
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC) {
        coding_opt2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
        coding_opt2.Header.BufferSz = sizeof(coding_opt2);
        coding_opt2.RepeatPPS = MFX_CODINGOPTION_OFF;//No repeat pps

        // Number of macroblocks per slice is specified
        if (extParams.nMbPerSlice) {
            mfxU32 MaxMacroBlocks= mfx_video_param_.mfx.FrameInfo.CropW * mfx_video_param_.mfx.FrameInfo.CropH / MACROBLOCK_SIZE;
            mfxU16 MinMacroBlocks = 8;
            if (mfx_video_param_.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE) {
                if (extParams.nMbPerSlice > MaxMacroBlocks) {
                    extParams.nMbPerSlice = MaxMacroBlocks;
                    MLOG_WARNING("Number of macroBlock is too large, the maximum is %u\n", MaxMacroBlocks);
                }
            } else { // interlace stream
                if (extParams.nMbPerSlice > MaxMacroBlocks / 2) {
                    extParams.nMbPerSlice = MaxMacroBlocks / 2;
                    MLOG_WARNING("Number of macroBlock is too large, the maximum is %u\n", MaxMacroBlocks / 2);
                }
            }
            if (extParams.nMbPerSlice < MinMacroBlocks) {
                extParams.nMbPerSlice = MinMacroBlocks;
                MLOG_WARNING("Number of macroBlock is too small, the minimum is %u\n", MinMacroBlocks);
            }

            coding_opt2.NumMbPerSlice = extParams.nMbPerSlice;
        }

        if (extParams.bMBBRC) {
            coding_opt2.MBBRC = MFX_CODINGOPTION_ON;
        }

        if ((mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_LA) && \
                extParams.nLADepth) {
            coding_opt2.LookAheadDepth = extParams.nLADepth;
        }

        //max slice size setting, supported for AVC only, not compatible with "slice num"
        if (extParams.nMaxSliceSize) {
            coding_opt2.MaxSliceSize = extParams.nMaxSliceSize;
            MLOG_INFO("nMaxSlixeSize = %d\n", coding_opt2.MaxSliceSize);
        }

        // B-pyramid reference
        if (extParams.bRefType) {
            if (mfx_video_param_.mfx.CodecProfile == MFX_PROFILE_AVC_BASELINE
                || mfx_video_param_.mfx.GopRefDist == 1) {
                MLOG_WARNING("not have B frame in output stream\n");
            } else {
                coding_opt2.BRefType = extParams.bRefType;
            }
            MLOG_INFO("BRefType = %u\n", coding_opt2.BRefType);
        }

        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&coding_opt2));

        //extcoding option3
        if (m_bLibType != MFX_IMPL_SOFTWARE) {
#if MFX_VERSION >= 1023
            if(extParams.nRepartitionCheckEnable){
                coding_opt3.RepartitionCheckEnable = extParams.nRepartitionCheckEnable;
            }
#endif
            coding_opt3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
            coding_opt3.Header.BufferSz = sizeof(coding_opt3);
            m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&coding_opt3));
        }

    }

#if (MFX_VERSION >= 1025)
    if(extParams.nMFEFrames || extParams.nMFEMode)
    {
        m_ExtMFEParam.Header.BufferId = MFX_EXTBUFF_MULTI_FRAME_PARAM;
        m_ExtMFEParam.Header.BufferSz = sizeof(mfxExtMultiFrameParam);
        m_ExtMFEParam.MaxNumFrames = extParams.nMFEFrames;
        m_ExtMFEParam.MFMode = extParams.nMFEMode;
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_ExtMFEParam));
    }

    if(extParams.nMFETimeout)
    {
        m_ExtMFEControl.Header.BufferId = MFX_EXTBUFF_MULTI_FRAME_CONTROL;
        m_ExtMFEControl.Header.BufferSz = sizeof(mfxExtMultiFrameControl);
        m_ExtMFEControl.Timeout = extParams.nMFETimeout; 
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_ExtMFEControl));
    }
#endif
    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
        //opaue alloc
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(ext_opaque_alloc_);
        memcpy(&ext_opaque_alloc_.In, \
               & ((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out, \
               sizeof(ext_opaque_alloc_.In));
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));

    } else if (m_bLibType == MFX_IMPL_SOFTWARE) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    //set mfx video ExtParam
    if (!m_EncExtParams.empty()) {
        mfx_video_param_.ExtParam = &m_EncExtParams[0]; // vector is stored linearly in memory
        mfx_video_param_.NumExtParam = (mfxU16)m_EncExtParams.size();
        MLOG_INFO("init encoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
    }

    memset(&EncRequest, 0, sizeof(EncRequest));

    sts = mfx_enc_->QueryIOSurf(&mfx_video_param_, &EncRequest);
    MLOG_INFO("Enc suggest surfaces %d\n", EncRequest.NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    memset(&par, 0, sizeof(par));

    sts = mfx_enc_->Init(&mfx_video_param_);
    if (sts > MFX_ERR_NONE) {
        MLOG_WARNING("\t\tEncode init warning %d\n", sts);
    } else if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\tEncode init return %d\n", sts);
    }
    assert(sts >= 0);

    sts = mfx_enc_->GetVideoParam(&par);

    // Prepare Media SDK bit stream buffer for encoder
    memset(&output_bs_, 0, sizeof(output_bs_));
    output_bs_.MaxLength = par.mfx.BufferSizeInKB * 2000;
    output_bs_.Data = new mfxU8[output_bs_.MaxLength];
    MLOG_INFO("output bitstream buffer size %d\n", output_bs_.MaxLength);

    //Separate extbuffer used for init() and EncodeFrameAsync()
    //m_EncExtParams.clear();
    m_nFramesProcessed = 0;

    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC) {
        //initialize m_avcRefLists
        memset(&m_avcrefLists, 0, sizeof(mfxExtAVCRefLists));
        m_avcrefLists.Header.BufferId = MFX_EXTBUFF_AVC_REFLISTS;
        m_avcrefLists.Header.BufferSz = sizeof(mfxExtAVCRefLists);

        int i = 0;
        for (i = 0; i < 32; i++) {
            m_avcrefLists.RefPicList0[i].FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
            m_avcrefLists.RefPicList0[i].PicStruct = mfx_video_param_.mfx.FrameInfo.PicStruct;
            m_avcrefLists.RefPicList1[i].FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
            m_avcrefLists.RefPicList1[i].PicStruct = mfx_video_param_.mfx.FrameInfo.PicStruct;
        }

    }

    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC) {
        //initialize m_avcRefList
        memset(&m_avcRefList, 0, sizeof(m_avcRefList));
        m_avcRefList.Header.BufferId = MFX_EXTBUFF_AVC_REFLIST_CTRL;
        m_avcRefList.Header.BufferSz = sizeof(m_avcRefList);

        int i = 0;
        for (i = 0; i < 32; i++) {
           m_avcRefList.PreferredRefList[i].FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
           m_avcRefList.PreferredRefList[i].PicStruct  = mfx_video_param_.mfx.FrameInfo.PicStruct;
        }
        for (i = 0; i < 16; i++) {
           m_avcRefList.RejectedRefList[i].FrameOrder  = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
           m_avcRefList.RejectedRefList[i].PicStruct   = mfx_video_param_.mfx.FrameInfo.PicStruct;
           m_avcRefList.LongTermRefList[i].FrameOrder  = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
           m_avcRefList.LongTermRefList[i].PicStruct   = mfx_video_param_.mfx.FrameInfo.PicStruct;
        }
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(ENC_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    return sts;
}

int EncoderElement::ProcessChainEncode(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;
    mfxSyncPoint syncpE;
    mfxEncodeCtrl *enc_ctrl = NULL;

    SetThreadName("Encode Thread");

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
                einfo.mChannelNum = EncoderElement::mNumOfEncChannels;
                EncoderElement::mNumOfEncChannels++;

                switch (mfx_video_param_.mfx.CodecId) {
                    case MFX_CODEC_AVC:
                        einfo.mCodecName = "AVC";
                        break;
                    case MFX_CODEC_MPEG2:
                        einfo.mCodecName = "MPEG2";
                        break;
                    case MFX_CODEC_VC1:
                        einfo.mCodecName = "VC1";
                        break;
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
                    case MFX_CODEC_VP8:
                        einfo.mCodecName = "VP8";
                        break;
#endif
                    default:
                        einfo.mCodecName = "unkown codec";
                        break;
                }

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

            reset_res_flag_ = true;
            MLOG_INFO(" to reset encoder res as %d x %d\n",
                   mfx_video_param_.mfx.FrameInfo.CropW, mfx_video_param_.mfx.FrameInfo.CropH);
        }

        //mark the frame order for incoming surface.
        pmfxInSurface->Data.FrameOrder = m_nFramesProcessed;
    }

    if (reset_bitrate_flag_ || reset_res_flag_){
        //need to complete encoding with current settings
        for (;;) {
            for (;;) {
                sts = mfx_enc_->EncodeFrameAsync(NULL, NULL, &output_bs_, &syncpE);

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

            if (sts == MFX_ERR_MORE_DATA) {
                break;
            }
        }
    }

    if (reset_bitrate_flag_) {
        mfx_enc_->Reset(&mfx_video_param_);
        reset_bitrate_flag_ = false;
    }

    if (reset_res_flag_) {
        mfxVideoParam par;
        memset(&par, 0, sizeof(par));
        mfx_enc_->GetVideoParam(&par);
        par.mfx.FrameInfo.CropW = mfx_video_param_.mfx.FrameInfo.CropW;
        par.mfx.FrameInfo.CropH = mfx_video_param_.mfx.FrameInfo.CropH;
        par.mfx.FrameInfo.Width = mfx_video_param_.mfx.FrameInfo.Width;
        par.mfx.FrameInfo.Height = mfx_video_param_.mfx.FrameInfo.Height;

        mfx_enc_->Reset(&par);
        reset_res_flag_ = false;
    }
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    AddExtRoiBufferToCtrl();
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022

    if (force_key_frame_) {
        enc_ctrl_.FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
        //workaround, bug tracked by CQ VCSD100022643
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
        if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8)
            enc_ctrl_.FrameType = MFX_FRAMETYPE_I;
#endif
        enc_ctrl = &enc_ctrl_;
    }

    if (m_refNum > 0) {
        OnMarkRL();
    }

    if (m_bMarkLTR) {
        OnMarkLTR();
        m_bMarkLTR = false;
    }

    if (!m_CtrlExtParams.empty()) {
        //currently this may happen only for H264 HW encoder with m_avcRefList attached
        assert(mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC);
        enc_ctrl_.NumExtParam = (mfxU16)m_CtrlExtParams.size();
        enc_ctrl_.ExtParam = &m_CtrlExtParams.front();
        enc_ctrl = &enc_ctrl_;
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(ENC_FRAME_TIME_STAMP, this);
        measurement->RelLock();
    }

    do {
        while (is_running_) {

            sts = mfx_enc_->EncodeFrameAsync(enc_ctrl, pmfxInSurface,
                    &output_bs_, &syncpE);

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

            OnFrameEncoded(&output_bs_);
        }
        m_nFramesProcessed++;

        if (is_running_ == false) {
            //break the loop
            break;
        }
    } while (pmfxInSurface == NULL && sts >= MFX_ERR_NONE);

    if (NULL == pmfxInSurface && buf.is_eos && output_stream_ != NULL) {
        output_stream_->SetEndOfStream();
        MLOG_INFO("Got EOF in Encoder %p\n", this);
        is_running_ = false;
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(ENC_ENDURATION_TIME_STAMP, this);
        measurement->RelLock();
    }

    //MLOG_INFO("leave ProcessChainEncode\n");
    return 0;
}

int EncoderElement::HandleProcessEncode()
{
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(0 != this->sinkpads_.size());

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

/*
 * Mark current frame as long-term reference frame.
 * Returns: 0 - Successful
 *          1 - Warning, this api is for H264 encoder only
 *         -1 - codec is not initialized yet
 */
int EncoderElement::MarkLTR()
{
    if (mfx_video_param_.mfx.CodecId != MFX_CODEC_AVC) {
        MLOG_ERROR(" this api is for H264 encoder only\n");
        return 1;
    }

    if (!codec_init_) {
        MLOG_ERROR(" encoder is not initialized yet\n");
        return -1;
    }

    m_bMarkLTR = true;
    return 0;
}

/*
 * Set reference frames number.
 * Returns: 0 - Successful
 *          1 - Warning, this api is for H264 encoder only
 *         -1 - codec is not initialized yet
 */
int EncoderElement::MarkRL(unsigned int refNum)
{
    if (mfx_video_param_.mfx.CodecId != MFX_CODEC_AVC) {
        MLOG_ERROR(" this api is for H264 encoder only\n");
        return 1;
    }

    m_refNum = refNum;
    return 0;
}

/*
 * Mark reference frame.
 */
void EncoderElement::OnMarkRL()
{
    if (m_refNum <= m_nFramesProcessed) {
        m_avcrefLists.NumRefIdxL0Active = (m_refNum > 32) ? 32 : m_refNum;
        m_avcrefLists.NumRefIdxL1Active = 0;

        for (int i = 0; i < m_avcrefLists.NumRefIdxL0Active; i++) {
            m_avcrefLists.RefPicList0[i].FrameOrder = m_nFramesProcessed - i - 1;
            //Reference list control is only supported in progressive encoding mode.
            m_avcrefLists.RefPicList0[i].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }
    } else {
        return;
    }

    //Check if m_avcrefLists is already in m_CtrlExtParams
    if (!m_CtrlExtParams.empty()) {
        std::vector<mfxExtBuffer *>::iterator it;
        for (it = m_CtrlExtParams.begin(); it != m_CtrlExtParams.end(); ++it) {
            if (*it == (mfxExtBuffer *)&m_avcrefLists) {
                break;
            }
        }
        if (it == m_CtrlExtParams.end()) {
            //m_avcrefLists is not in m_CtrlExtParams
            m_CtrlExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(&m_avcrefLists));
        }
    } else {
        //m_CtrlExtParams is empty, push m_avcrefLists directly
        m_CtrlExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(&m_avcrefLists));
    }
}


void EncoderElement::OnMarkLTR()
{
    //put current frame into LongTermRefList
    int i = 0;
    for (i = 0; i < 16; i++) {
        //find a slot in sequence
        if (m_avcRefList.LongTermRefList[i].FrameOrder == m_nFramesProcessed) {
            //This frame is already marked as LTR
            break;
        }
        if (m_avcRefList.LongTermRefList[i].FrameOrder == (mfxU32)MFX_FRAMEORDER_UNKNOWN) {
            m_avcRefList.LongTermRefList[i].FrameOrder = m_nFramesProcessed;
            //Reference list control is only supported in progressive encoding mode.
            m_avcRefList.LongTermRefList[i].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            break;
        }
    }
    if (i == 16) {
        MLOG_WARNING(" LongTermRefList is full\n");
        return;
    }

    //put current frame into PreferredRefList, or else it may be ignored
    for (i = 0; i < 32; i++) {
        if (m_avcRefList.PreferredRefList[i].FrameOrder == m_nFramesProcessed) {
            //Current frame is already in PreferredRefList
            break;
        }
        if (m_avcRefList.PreferredRefList[i].FrameOrder == (mfxU32)MFX_FRAMEORDER_UNKNOWN) {
            m_avcRefList.PreferredRefList[i].FrameOrder = m_nFramesProcessed;
            //Reference list control is only supported in progressive encoding mode.
            m_avcRefList.PreferredRefList[i].PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
            break;
        }
    }
    if (i == 32) {
        MLOG_WARNING(" PreferredRefList is full");
        //no need to return
    }

    //Check if m_avcRefList is already in m_CtrlExtParams
    if (!m_CtrlExtParams.empty()) {
        std::vector<mfxExtBuffer *>::iterator it;
        for (it = m_CtrlExtParams.begin(); it != m_CtrlExtParams.end(); ++it) {
            if (*it == (mfxExtBuffer *)&m_avcRefList) {
                MLOG_WARNING("m_avcRefList is already in m_CtrlExtParams\n");
                break;
            }
        }
        if (it == m_CtrlExtParams.end()) {
            //m_avcRefList is not in m_CtrlExtParams
            m_CtrlExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(&m_avcRefList));
        }
    } else {
        //m_CtrlExtParams is empty, push m_avcRefList directly
        m_CtrlExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(&m_avcRefList));
    }

    MLOG_INFO(" mark frame %d as LTR\n", m_nFramesProcessed);
}
/*
 * Perform actions needed when one frame is encoded
 */
void EncoderElement::OnFrameEncoded(mfxBitstream *pBs)
{
    assert(pBs);
    if (mfx_video_param_.mfx.CodecId != MFX_CODEC_AVC) {
        return;
    }

    if (!m_CtrlExtParams.empty()) {
        int i = 0;
        for (i = 0; i < 16; i++) {
            //LTR reference list is stateful, mark it once is encough, so remove all frames in LTRL
            if (m_avcRefList.LongTermRefList[i].FrameOrder != (mfxU32)MFX_FRAMEORDER_UNKNOWN) {
                m_avcRefList.LongTermRefList[i].FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
            } else {
                //stop searching
                break;
            }
        }

        //PreferredRefList is stateless, so mark it explicitely until next IDR frame
        if (pBs->FrameType & MFX_FRAMETYPE_I) {
            //1. remove the elements in PreferredList
            for (i = 0; i < 32; i++) {
               if (m_avcRefList.PreferredRefList[i].FrameOrder != (mfxU32)MFX_FRAMEORDER_UNKNOWN) {
                   m_avcRefList.PreferredRefList[i].FrameOrder = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
               } else {
                   break;
               }
            }

            //2. erase m_avcRefList from m_CtrlExtParams when IDR frame was encoded
            std::vector<mfxExtBuffer *>::iterator it;
            for (it = m_CtrlExtParams.begin(); it != m_CtrlExtParams.end(); ++it) {
                if (*it == (mfxExtBuffer *)&m_avcRefList) {
                    MLOG_WARNING("to erase m_avcRefList from m_CtrlExtParams\n");
                    m_CtrlExtParams.erase(it);
                    break;
                }
            }
        }
    }

    // Reset FrameType to UNKNOWN after force key frame
    if (force_key_frame_) {
        force_key_frame_ = false;
        enc_ctrl_.FrameType = MFX_FRAMETYPE_UNKNOWN;
    }
}

int EncoderElement::HandleProcess()
{
    return HandleProcessEncode();
}

int EncoderElement::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    if (buf.msdk_surface == NULL && buf.is_eos) {
        MLOG_INFO("Got EOF in element %p, type is %d\n", this, element_type_);
    }
    return ProcessChainEncode(buf);
}

mfxStatus EncoderElement::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = mfx_enc_->GetVideoParam(par);
    return sts;
}
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
mfxStatus EncoderElement::AddExtRoiBufferToCtrl()
{
    mfxU32 frame_num = m_nFramesProcessed;

    if (m_roiData[frame_num].NumROI > 0) {
        mfxU16 roi_buf_ind = 0;
        if(!m_CtrlExtParams.empty()) {
            std::vector<mfxExtBuffer *>::iterator it;
            for (it = m_CtrlExtParams.begin(); it != m_CtrlExtParams.end(); ++it ) {
                if ((*it)->BufferId == MFX_EXTBUFF_ENCODER_ROI) {
                    m_CtrlExtParams.erase(it);
                    break;
                }
            }
        }
        m_CtrlExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_roiData[frame_num]));
    }

    return MFX_ERR_NONE;
}
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
