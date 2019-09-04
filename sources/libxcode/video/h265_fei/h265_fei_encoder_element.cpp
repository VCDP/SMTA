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

#include "h265_fei_encoder_element.h"
#ifdef MSDK_HEVC_FEI

#include <assert.h>
#if defined (PLATFORM_OS_LINUX)
#include <unistd.h>
#include <math.h>
#endif

#include "element_common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

DEFINE_MLOGINSTANCE_CLASS(H265FEIEncoderElement, "H265FEIEncoderElement");

unsigned int H265FEIEncoderElement::mNumOfEncChannels = 0;

H265FEIEncoderElement::H265FEIEncoderElement(ElementType type,
                               MFXVideoSession *session,
                               MFXFrameAllocator *pMFXAllocator,
                               CodecEventCallback *callback):
                               m_pPreencSession(session)
{
    initialize(type, session, pMFXAllocator, callback);

    m_pParamChecker.reset(new HEVCEncodeParamsChecker(session));
    m_preenc = NULL;
    m_encode = NULL;

    m_DSstrength = 0;
    memset(&m_videoParams, 0, sizeof(m_videoParams));
    memset(&m_extParams, 0, sizeof(m_extParams));
    memset(&m_preencCtrl, 0, sizeof(m_preencCtrl));
    memset(&m_encodeCtrl, 0, sizeof(m_encodeCtrl));

    m_frameCount = 0;
    m_frameOrderIdrInDisplayOrder = 0;
    m_pOrderCtrl = NULL;
}
H265FEIEncoderElement::~H265FEIEncoderElement()
{
    if (m_preenc) {
        delete m_preenc;
        m_preenc = NULL;
    }

    if (m_encode) {
        delete m_encode;
        m_encode = NULL;
    }
    if (m_pParamChecker.get())
        m_pParamChecker.release();
}

bool H265FEIEncoderElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    measurement = ele_param->measurement;

    if (ele_param->EncParams.mfx.CodecId != MFX_CODEC_HEVC)
    {
        MLOG_ERROR("PREENC+ENCODE is for H265 encoder only\n");
        return false;
    }

    m_videoParams.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist==0? 4: ele_param->EncParams.mfx.GopRefDist;
    m_videoParams.mfx.GopOptFlag = ele_param->EncParams.mfx.GopOptFlag;
    m_videoParams.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize==0? 30: ele_param->EncParams.mfx.GopPicSize;
    m_videoParams.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
    m_videoParams.mfx.GopRefDist = (ele_param->EncParams.mfx.GopRefDist+1) > 4? 4: (ele_param->EncParams.mfx.GopRefDist+1);
    m_videoParams.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    m_videoParams.mfx.RateControlMethod = ele_param->EncParams.mfx.RateControlMethod;
    m_videoParams.mfx.QPI = ele_param->EncParams.mfx.QPI;
    m_videoParams.mfx.QPB = ele_param->EncParams.mfx.QPB;
    m_videoParams.mfx.QPP = ele_param->EncParams.mfx.QPP;
    m_videoParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    m_videoParams.AsyncDepth = 1;
    m_videoParams.mfx.TargetUsage = 0;
    m_videoParams.mfx.TargetKbps = 0;
    m_videoParams.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
    m_videoParams.mfx.EncodedOrder = 1;

    /* Encoder extension parameters*/
    m_extParams.bMBBRC = ele_param->extParams.bMBBRC;
    m_extParams.nLADepth = ele_param->extParams.nLADepth;
    m_extParams.nMbPerSlice = ele_param->extParams.nMbPerSlice;
    m_extParams.bRefType = ele_param->extParams.bRefType;
    m_extParams.pRefType = ele_param->extParams.pRefType;
    m_extParams.GPB = ele_param->extParams.GPB;
    m_extParams.mvoutStream =  ele_param->extParams.mvoutStream;
    m_extParams.mvinStream =  ele_param->extParams.mvinStream;
    m_extParams.mbcodeoutStream =  ele_param->extParams.mbcodeoutStream;
    m_extParams.weightStream =  ele_param->extParams.weightStream;
    m_extParams.nRepartitionCheckEnable = ele_param->extParams.nRepartitionCheckEnable;
    m_extParams.fei_ctrl = ele_param->extParams.fei_ctrl;
    m_extParams.bQualityRepack = ele_param->extParams.bQualityRepack;

    //if mfx_video_param_.mfx.NumSlice > 0, don't set MaxSliceSize and give warning.
    if (ele_param->EncParams.mfx.NumSlice && ele_param->extParams.nMaxSliceSize) {
        MLOG_WARNING(" MaxSliceSize is not compatible with NumSlice, discard it\n");
        m_extParams.nMaxSliceSize = 0;
    } else {
        m_extParams.nMaxSliceSize = ele_param->extParams.nMaxSliceSize;
    }

    output_stream_ = ele_param->output_stream;
    InitExtBuffers(ele_param);

    if (ele_param->extParams.bPreEnc) {
        if (ele_param->extParams.fei_ctrl.nDSstrength)
            m_DSstrength = ele_param->extParams.fei_ctrl.nDSstrength;

        m_preenc = new H265FEIEncoderPreenc(m_pPreencSession, m_pMFXAllocator, m_bUseOpaqueMemory, m_preencCtrl, &m_extParams);
        m_preenc->Init(ele_param);
        m_extParams.bPreEnc = true;
    }

    // Create and initialize a ENCODE element
    if (ele_param->extParams.bEncode) {
        m_encode = new H265FEIEncoderEncode(mfx_session_, m_bUseOpaqueMemory,
                                         m_encodeCtrl, &m_extParams);
        m_encode->Init(ele_param);
        m_extParams.bEncode = true;
    }

    return true;
}

mfxStatus H265FEIEncoderElement::InitExtBuffers(ElementCfg *ele_param)
{
    mfxStatus sts = MFX_ERR_NONE;

    //Preenc ext buffer setting, setup control structures
    if (ele_param->extParams.bPreEnc) {
        m_preencCtrl.Header.BufferId = MFX_EXTBUFF_FEI_PREENC_CTRL;
        m_preencCtrl.Header.BufferSz = sizeof(mfxExtFeiPreEncCtrl);
        m_preencCtrl.PictureType             = MFX_PICSTRUCT_PROGRESSIVE;//GetCurPicType(fieldId);
        m_preencCtrl.RefPictureType[0]       = m_preencCtrl.PictureType;
        m_preencCtrl.RefPictureType[1]       = m_preencCtrl.PictureType;
        m_preencCtrl.DownsampleInput         = MFX_CODINGOPTION_ON;
        // In sample_fei preenc works only in encoded order
        m_preencCtrl.DownsampleReference[0]  = MFX_CODINGOPTION_OFF;
        // so all references would be already downsampled
        m_preencCtrl.DownsampleReference[1]  = MFX_CODINGOPTION_OFF;
        m_preencCtrl.DisableMVOutput         = 1; //disableMVoutPreENC;
        m_preencCtrl.DisableStatisticsOutput = 1; //disableMBStatPreENC;
        m_preencCtrl.SubPelMode              = m_extParams.fei_ctrl.nSubPelMode;
        m_preencCtrl.SubMBPartMask           = 0x00;
        m_preencCtrl.SearchWindow            = m_extParams.fei_ctrl.nSearchWindow;
        if (!m_extParams.fei_ctrl.nSearchWindow)
            m_preencCtrl.SearchWindow            = 5;
        m_preencCtrl.IntraSAD                = 0x02;
        m_preencCtrl.InterSAD                = 0x02;
        m_preencCtrl.IntraPartMask           = m_extParams.fei_ctrl.nIntraPartMask;
        m_preencCtrl.AdaptiveSearch          = m_extParams.fei_ctrl.nAdaptiveSearch;
        //m_preencCtrl.Enable8x8Stat           = false;

        if (ele_param->extParams.bQualityRepack)
            m_preencCtrl.DisableStatisticsOutput = 0;

    }

    // Encode ext buffer setting
    if (ele_param->extParams.bEncode) {

        m_encodeCtrl.Header.BufferId = MFX_EXTBUFF_HEVCFEI_ENC_CTRL;
        m_encodeCtrl.Header.BufferSz = sizeof(mfxExtFeiHevcEncFrameCtrl);

        m_encodeCtrl.SearchPath             = m_extParams.fei_ctrl.nSearchPath;
        m_encodeCtrl.LenSP                  = m_extParams.fei_ctrl.nLenSP;
        m_encodeCtrl.RefWidth               = m_extParams.fei_ctrl.nRefWidth;
        m_encodeCtrl.RefHeight              = m_extParams.fei_ctrl.nRefHeight;
        m_encodeCtrl.SearchWindow           = m_extParams.fei_ctrl.nSearchWindow;
        m_encodeCtrl.NumMvPredictors[0]     = m_extParams.fei_ctrl.nNumMvPredictors[0];
        m_encodeCtrl.NumMvPredictors[1]     = m_extParams.fei_ctrl.nNumMvPredictors[1];
        m_encodeCtrl.MultiPred[0]           = m_extParams.fei_ctrl.bMultiPredL0;
        m_encodeCtrl.MultiPred[1]           = m_extParams.fei_ctrl.bMultiPredL1;
        m_encodeCtrl.SubPelMode             = m_extParams.fei_ctrl.nSubPelMode;
        m_encodeCtrl.AdaptiveSearch         = m_extParams.fei_ctrl.nAdaptiveSearch;
        m_encodeCtrl.MVPredictor            = m_extParams.fei_ctrl.nMVPredictor;
        m_encodeCtrl.PerCuQp                = false;
        m_encodeCtrl.PerCtuInput            = false;
        m_encodeCtrl.ForceCtuSplit          = m_extParams.fei_ctrl.nForceCtuSplit;

        m_encodeCtrl.NumFramePartitions     = m_extParams.fei_ctrl.nNumFramePartitions;
    }
    return sts;
}

mfxStatus H265FEIEncoderElement::InitEncoder(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool encodeOrder = false;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(PREENC_ENCODE_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    //for common used params
    memcpy(&(m_videoParams.mfx.FrameInfo), &(((mfxFrameSurface1 *)buf.msdk_surface)->Info), sizeof(mfxFrameInfo));

    if (m_preenc) {
        sts = m_preenc->InitPreenc(buf);
        if (MFX_ERR_NONE != sts){
            MLOG_ERROR("PREENC create failed %d\n", sts);
            return sts;
        }
    }

    PredictorsRepacking* pRepacker = NULL;

    if (m_extParams.bPreEnc || m_extParams.mvinStream )
    {
        MfxVideoParamsWrapper param;
        if (m_preenc) {
            param = m_preenc->GetVideoParams();
            param.mfx.CodecId = MFX_CODEC_HEVC;
        } else {
            param = GetVideoParams(buf);
        }
        encodeOrder = true;

        pRepacker = new PredictorsRepacking();
        sts = pRepacker->Init(param, m_DSstrength, m_encodeCtrl.NumMvPredictors);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("CreateEncode::pRepacker->Init failed.");
            return sts;
        }

        m_extParams.bQualityRepack ? pRepacker->SetQualityRepackingMode() : pRepacker->SetPerfomanceRepackingMode();

        sts = m_pParamChecker->Query(param);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("m_pParamChecker->Query failed. sts = %d", sts);
            return sts;
        }

        m_pOrderCtrl = new EncodeOrderControl(param, true);
    }
    if (m_encode) {
        sts = m_encode->InitEncode(buf, pRepacker, encodeOrder);
        if (MFX_ERR_NONE > sts){
            MLOG_ERROR("Encode create failed %d\n", sts);
            return sts;
        }

        mfxFrameAllocRequest EncRequest;
        sts = m_encode->QueryIOSurf(&EncRequest);
        MLOG_INFO("Encode suggest surfaces %d\n", EncRequest.NumFrameSuggested);
        assert(sts >= MFX_ERR_NONE);
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(PREENC_ENCODE_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    return sts;
}

int H265FEIEncoderElement::HandleProcessEncode()
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(0 != this->sinkpads_.size());
    //* task = NULL; // encoding task
    mfxFrameSurface1 *pmfxInSurface;
    mfxI32 eos_FrameCount = -1;
    bool is_eos = false;

    while (is_running_) {
        if (eos_FrameCount == -1) { //can't get buf after the end of stream appeared
            if (sinkpad->GetBufData(buf) != 0) {
                // No data, just sleep and wait
                usleep(100);
                continue;
            }
        }

        pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;
        SetThreadName("Preenc+Encode Thread");

        //Init PREENC and ENCODE
        if (!codec_init_) {
            if (buf.msdk_surface == NULL && buf.is_eos) {
                //The first buf encoder received is eos
                return -1;
            }
            sts = InitEncoder(buf);
            if (MFX_ERR_NONE <= sts) {
                codec_init_ = true;

                if (measurement) {
                    measurement->GetLock();
                    measurement->TimeStpStart(PREENC_ENCODE_ENDURATION_TIME_STAMP, this);
                    pipelineinfo einfo;
                    einfo.mElementType = element_type_;
                    einfo.mChannelNum = H265FEIEncoderElement::mNumOfEncChannels;
                    H265FEIEncoderElement::mNumOfEncChannels++;
                    einfo.mCodecName = "HEVC";

                    measurement->SetElementInfo(PREENC_ENCODE_ENDURATION_TIME_STAMP, this, &einfo);
                    measurement->RelLock();
                }
                MLOG_INFO("Encoder element %p init successfully\n", this);

            } else {
                MLOG_ERROR("FEI Encoder element create failed %d\n", sts);
                return -1;
            }
        }

        if (pmfxInSurface) {
            pmfxInSurface->Data.FrameOrder = m_frameCount; // in display order
        }
        m_frameCount++;

        if (buf.is_eos) {
            break;
        }

        HevcTask * task = NULL;
        if (m_pOrderCtrl) {
            task = m_pOrderCtrl->GetTask(pmfxInSurface);
            if (!task)
                continue;
        }

        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpStart(PREENC_ENCODE_FRAME_TIME_STAMP, this);
            measurement->RelLock();
        }

        if (m_preenc) {
            sts = m_preenc->ProcessChainPreenc(task);
            if (sts < MFX_ERR_NONE) {
                MLOG_ERROR("FEI PREENC: ProcessChainPreenc failed. sts = %d", sts);
                break;
            }
        }

        if (m_encode) {
            sts = task ? m_encode->ProcessChainEncode(task, is_eos):m_encode->ProcessChainEncode(pmfxInSurface, is_eos);
            if (sts < MFX_ERR_NONE) {
                MLOG_ERROR("FEI ENCODE: ProcessOneFrame failed.sts = %d", sts);
                break;
            }
        }
        if (is_eos) {
            output_stream_->SetEndOfStream();
            is_running_ = false;
        }
        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpFinish(PREENC_ENCODE_FRAME_TIME_STAMP, this);
            measurement->RelLock();
        }

        if (m_pOrderCtrl) {
            m_pOrderCtrl->FreeTask(task);
        }

        if (sts == MFX_ERR_NONE) {
            sinkpad->ReturnBufToPeerPad(buf);
        }
    }

    if (sts < MFX_ERR_NONE) {
        output_stream_->SetEndOfStream();
        is_running_ = false;
        return -1;
    }

    // encode order
    if (m_pOrderCtrl)
    {
        HevcTask* task;
        while ( NULL != (task = m_pOrderCtrl->GetTask(NULL)))
        {
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(PREENC_ENCODE_FRAME_TIME_STAMP, this);
                measurement->RelLock();
            }
            if (m_preenc)
            {
                sts = m_preenc->ProcessChainPreenc(task);
                //MSDK_CHECK_STATUS(sts, "PreEncFrame drain failed");
                if (sts != MFX_ERR_NONE) {
                    MLOG_ERROR("PreEncFrame drain failed. sts = %d\n", sts);
                    return sts;
                }
            }

            if (m_encode)
            {
                sts = m_encode->ProcessChainEncode(task, is_eos);
                // exit in case of other errors
                if (sts != MFX_ERR_NONE) {
                    MLOG_ERROR("EncodeFrame drain failed. sts = %d\n", sts);
                    return sts;
                }

            }
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(PREENC_ENCODE_FRAME_TIME_STAMP, this);
                measurement->RelLock();
            }
            m_pOrderCtrl->FreeTask(task);
        }
        if (m_preenc && !m_encode)
            output_stream_->SetEndOfStream();
    }

    // display order
    if (m_encode)
    {
        while (MFX_ERR_NONE <= sts)
        {
            sts = m_encode->ProcessChainEncode((mfxFrameSurface1*)NULL, is_eos);
        }

        // MFX_ERR_MORE_DATA indicates that there are no more buffered frames
        // MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
        // exit in case of other errors
        // MSDK_CHECK_STATUS(sts, "EncodeFrame failed");
        if (sts != MFX_ERR_NONE && sts != MFX_ERR_MORE_DATA) {
            MLOG_ERROR("EncodeFrame failed. sts = %d\n", sts);
            return sts;
        }
        output_stream_->SetEndOfStream();
        is_running_ = false;
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(PREENC_ENCODE_ENDURATION_TIME_STAMP, this);
        measurement->RelLock();
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

int H265FEIEncoderElement::HandleProcess()
{
    return HandleProcessEncode();
}

mfxStatus H265FEIEncoderElement::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_encode) {
        sts = m_encode->GetVideoParam(par);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("ENCODE: Get video parameters failed Encode.");
        }
    }

    return sts;
}

MfxVideoParamsWrapper H265FEIEncoderElement::GetVideoParams(MediaBuf& buf)
{
    MfxVideoParamsWrapper pars;

    memcpy(&(pars.mfx), &(m_videoParams.mfx), sizeof(mfxInfoMFX));
    pars.AsyncDepth            = 1; // inherited limitation from AVC FEI
    pars.IOPattern             = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    mfxExtCodingOption* pCO = pars.AddExtBuffer<mfxExtCodingOption>();
    if (!pCO) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption");

    pCO->PicTimingSEI = MFX_CODINGOPTION_OFF;

    mfxExtCodingOption2* pCO2 = pars.AddExtBuffer<mfxExtCodingOption2>();
    if (!pCO2) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption2");

    // configure B-pyramid settings
    pCO2->BRefType = m_extParams.bRefType;

    mfxExtCodingOption3* pCO3 = pars.AddExtBuffer<mfxExtCodingOption3>();
    if (!pCO3) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption3");

    pCO3->PRefType             = m_extParams.pRefType;

    std::fill(pCO3->NumRefActiveP,   pCO3->NumRefActiveP + 8,   m_extParams.fei_ctrl.nRefActiveP);
    std::fill(pCO3->NumRefActiveBL0, pCO3->NumRefActiveBL0 + 8, m_extParams.fei_ctrl.nRefActiveBL0);
    std::fill(pCO3->NumRefActiveBL1, pCO3->NumRefActiveBL1 + 8, m_extParams.fei_ctrl.nRefActiveBL1);

    pCO3->GPB = m_extParams.GPB;

    // qp offset per pyramid layer, default is library behavior
    pCO3->EnableQPOffset = MFX_CODINGOPTION_UNKNOWN;

    return pars;
}
#endif //ifdef MSDK_HEVC_FEI
