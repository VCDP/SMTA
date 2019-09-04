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

#include "h264_fei_encoder_element.h"
#ifdef MSDK_AVC_FEI

#include <assert.h>
#if defined (PLATFORM_OS_LINUX)
#include <unistd.h>
#include <math.h>
#endif

#include "element_common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

DEFINE_MLOGINSTANCE_CLASS(H264FEIEncoderElement, "H264FEIEncoderElement");

unsigned int H264FEIEncoderElement::mNumOfEncChannels = 0;

H264FEIEncoderElement::H264FEIEncoderElement(ElementType type,
                               MFXVideoSession *session,
                               MFXFrameAllocator *pMFXAllocator,
                               CodecEventCallback *callback):
                               m_pPreencSession(session),
                               m_DSSurfaces(PREFER_FIRST_FREE),
                               m_reconSurfaces(PREFER_FIRST_FREE)
{
    initialize(type, session, pMFXAllocator, callback);
    m_preenc = NULL;
    m_encode = NULL;
    m_enc = NULL;
    m_pak = NULL;

    m_picStruct = MFX_PICSTRUCT_UNKNOWN;
    m_numOfFields = 0;
    m_nLastForceKeyFrame = 0;

    m_DSstrength = 0;
    m_picStruct = 0;
    m_refDist = 0;;
    m_gopOptFlag = 0;
    m_gopPicSize = 0;
    m_idrInterval = 0;
    m_numRefFrame = 0;
    m_width = 0;
    m_height = 0;
    m_widthMB = 0;
    m_heightMB = 0;

    reset_res_flag_ = false;
    memset(&m_encCtrl, 0, sizeof(m_encCtrl));
    memset(&m_pipelineCfg, 0, sizeof(m_pipelineCfg));
    memset(&m_DSRequest[0], 0, sizeof(m_DSRequest[0]));
    memset(&m_DSRequest[1], 0, sizeof(m_DSRequest[1]));
    memset(&m_commonFrameInfo, 0, sizeof(m_commonFrameInfo));
    //memset(&m_dsResponse, 0, sizeof(m_dsResponse));
    //memset(&m_reconResponse, 0, sizeof(m_reconResponse));
    memset(&m_bitstream, 0, sizeof(m_bitstream));

    m_maxQueueLength = 0;
    m_frameCount = 0;
    m_numSlice = 0;
    m_frameOrderIdrInDisplayOrder = 0;
    m_frameType = PairU8((mfxU8)MFX_FRAMETYPE_UNKNOWN, (mfxU8)MFX_FRAMETYPE_UNKNOWN);
}
H264FEIEncoderElement::~H264FEIEncoderElement()
{
    if (NULL != m_bitstream.Data) {
        delete[] m_bitstream.Data;
        m_bitstream.Data = NULL;
    }

    if (m_preenc) {
        delete m_preenc;
        m_preenc = NULL;
    }

    if (m_encode) {
        delete m_encode;
        m_encode = NULL;
    }

    if (m_pak) {
        delete m_pak;
        m_pak = NULL;
    }

    if (m_enc) {
        delete m_enc;
        m_enc = NULL;
    }

    //unlock last frames
    m_inputTasks.Clear();

    if (m_preencBufs.buf_list.size())
        m_preencBufs.Clear();
    if (m_encodeBufs.buf_list.size())
        m_encodeBufs.Clear();

    // delete all allocated surfaces
    m_DSSurfaces.DeleteFrames();
    m_reconSurfaces.DeleteFrames();

    if (m_pMFXAllocator) {
        if(m_pipelineCfg.bPreenc && m_DSstrength){
            m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_dsResponse);
        }
        if(m_pipelineCfg.bEnc || m_pipelineCfg.bPak){
            m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_reconResponse);
        }
    }

    m_RefInfo.Clear();
}

bool H264FEIEncoderElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    measurement = ele_param->measurement;

    if (ele_param->EncParams.mfx.CodecId != MFX_CODEC_AVC)
    {
        MLOG_ERROR("PREENC+ENCODE is for H264 encoder only\n");
        return false;
    }

    //set a MFXVideoSession for PREENC, if it is needed.
    if (ele_param->extParams.bPreEnc && (ele_param->extParams.bENC)) {
        m_pPreencSession = ele_param->extParams.pPreencSession;
    }
    // Create and initialize a PREENC element
    if (ele_param->extParams.bPreEnc) {
        if (ele_param->extParams.fei_ctrl.nDSstrength)
            m_DSstrength = ele_param->extParams.fei_ctrl.nDSstrength;

        m_preenc = new H264FEIEncoderPreenc(m_pPreencSession, &m_inputTasks, &m_preencBufs,
                                        &m_encodeBufs, m_bUseOpaqueMemory, &m_pipelineCfg, &m_extParams);
        m_preenc->Init(ele_param);
        m_pipelineCfg.bPreenc = true;
    }

    // Create and initialize a ENCODE element
    if (ele_param->extParams.bEncode) {
        m_encode = new H264FEIEncoderEncode(mfx_session_, &m_preencBufs, &m_encodeBufs, m_bUseOpaqueMemory,
                                        &m_pipelineCfg, &m_encCtrl, &m_extParams);
        m_encode->Init(ele_param);
        m_pipelineCfg.bEncode = true;
    }

    // Create and initialize a ENC element
    if (ele_param->extParams.bENC) {
        m_enc = new H264FEIEncoderENC(mfx_session_, &m_inputTasks, &m_preencBufs, &m_encodeBufs, m_bUseOpaqueMemory,
                                  &m_pipelineCfg, &m_encCtrl, &m_extParams);
        m_enc->Init(ele_param);
        m_pipelineCfg.bEnc = true;
    }

    // Create and initialize a PAK element
    if (ele_param->extParams.bPAK) {
        m_pak = new H264FEIEncoderPAK(mfx_session_, &m_inputTasks, &m_preencBufs, &m_encodeBufs, m_bUseOpaqueMemory,
                                  &m_pipelineCfg, &m_encCtrl, &m_extParams);
        m_pak->Init(ele_param);
        m_pipelineCfg.bPak = true;
    }

    //m_picStruct = 0;
    m_refDist = ele_param->EncParams.mfx.GopRefDist==0? 4: ele_param->EncParams.mfx.GopRefDist;
    m_gopOptFlag = ele_param->EncParams.mfx.GopOptFlag;
    m_gopPicSize = ele_param->EncParams.mfx.GopPicSize==0? 30: ele_param->EncParams.mfx.GopPicSize;
    m_idrInterval = ele_param->EncParams.mfx.IdrInterval;
    m_numRefFrame = (ele_param->EncParams.mfx.GopRefDist+1) > 4? 4: (ele_param->EncParams.mfx.GopRefDist+1);
    m_numSlice = ele_param->EncParams.mfx.NumSlice;
    /* Encoder extension parameters*/
    m_extParams.bMBBRC = ele_param->extParams.bMBBRC;
    m_extParams.nLADepth = ele_param->extParams.nLADepth;
    m_extParams.nMbPerSlice = ele_param->extParams.nMbPerSlice;
    m_extParams.bRefType = ele_param->extParams.bRefType;
    m_extParams.mvoutStream =  ele_param->extParams.mvoutStream;
    m_extParams.mvinStream =  ele_param->extParams.mvinStream;
    m_extParams.mbcodeoutStream =  ele_param->extParams.mbcodeoutStream;
    m_extParams.weightStream =  ele_param->extParams.weightStream;
    m_extParams.nRepartitionCheckEnable = ele_param->extParams.nRepartitionCheckEnable;
    m_extParams.fei_ctrl = ele_param->extParams.fei_ctrl;
    m_extParams.bPreEnc = m_pipelineCfg.bPreenc;
    m_extParams.bEncode = m_pipelineCfg.bEncode;
    m_extParams.bENC = m_pipelineCfg.bEnc;
    m_extParams.bPAK = m_pipelineCfg.bPak;

    //if mfx_video_param_.mfx.NumSlice > 0, don't set MaxSliceSize and give warning.
    if (ele_param->EncParams.mfx.NumSlice && ele_param->extParams.nMaxSliceSize) {
        MLOG_WARNING(" MaxSliceSize is not compatible with NumSlice, discard it\n");
        m_extParams.nMaxSliceSize = 0;
    } else {
        m_extParams.nMaxSliceSize = ele_param->extParams.nMaxSliceSize;
    }

    m_encCtrl.FrameType = MFX_FRAMETYPE_UNKNOWN;
    m_encCtrl.QP = ele_param->EncParams.mfx.QPI;
    m_encCtrl.SkipFrame = 0;
    m_encCtrl.NumPayload = 0;
    m_encCtrl.Payload = NULL;

    output_stream_ = ele_param->output_stream;

    return true;
}

mfxStatus H264FEIEncoderElement::SetSequenceParameters()
{
    bool encodeFrameOrder = true;
    bool singleMode = false;
    mfxStatus sts = MFX_ERR_NONE;
    mfxU16 log2frameNumMax;
    mfxU16 numRefActiveP, numRefActiveBL0, numRefActiveBL1, bRefType;
    mfxU16 widthMBpreenc, heightMBpreenc;

    numRefActiveP = 0;
    numRefActiveBL0 = 0;
    numRefActiveBL1 = 0;
    log2frameNumMax = 8;

    if (m_pipelineCfg.bPreenc) {
        m_preenc->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopPicSize, m_gopOptFlag,
                   m_idrInterval, numRefActiveP, numRefActiveBL0, numRefActiveBL1, bRefType, singleMode);
    }

    if (m_pipelineCfg.bEncode) {
        m_encode->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopPicSize, m_gopOptFlag,
                m_idrInterval, numRefActiveP, numRefActiveBL0, numRefActiveBL1, bRefType, singleMode);
    }

    if (m_pipelineCfg.bPak) {
        m_pak->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopPicSize, m_gopOptFlag,
                m_idrInterval, numRefActiveP, numRefActiveBL0, numRefActiveBL1, bRefType, singleMode);
    }

    if (m_pipelineCfg.bEnc) {
        m_enc->GetRefInfo(m_picStruct, m_refDist, m_numRefFrame, m_gopPicSize, m_gopOptFlag,
                m_idrInterval, numRefActiveP, numRefActiveBL0, numRefActiveBL1, bRefType, singleMode);
    }

    m_widthMB         = MSDK_ALIGN16(m_width);
    m_heightMB        = m_numOfFields == 2 ? MSDK_ALIGN32(m_height) : MSDK_ALIGN16(m_height);

    /* Initialize task pool */
    m_inputTasks.Init(encodeFrameOrder,2 + !m_DSstrength, m_refDist, m_gopOptFlag, m_numRefFrame, m_numRefFrame + 1, log2frameNumMax);

    m_taskInitializationParams.PicStruct       = m_picStruct;
    m_taskInitializationParams.GopPicSize      = m_gopPicSize;
    m_taskInitializationParams.GopRefDist      = m_refDist;
    m_taskInitializationParams.NumRefActiveP   = numRefActiveP;
    m_taskInitializationParams.NumRefActiveBL0 = numRefActiveBL0;
    m_taskInitializationParams.NumRefActiveBL1 = numRefActiveBL1;
    m_taskInitializationParams.BRefType        = m_extParams.bRefType;

    m_taskInitializationParams.NumMVPredictorsP = m_extParams.fei_ctrl.bNPredSpecified_Pl0 ?
                    m_extParams.fei_ctrl.nMVPredictors_Pl0 : (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);
    m_taskInitializationParams.NumMVPredictorsBL0 = m_extParams.fei_ctrl.bNPredSpecified_Bl0 ?
                    m_extParams.fei_ctrl.nMVPredictors_Bl0 : (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);
    m_taskInitializationParams.NumMVPredictorsBL1 = m_extParams.fei_ctrl.bNPredSpecified_Bl1 ?
                    m_extParams.fei_ctrl.nMVPredictors_Bl1 : (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);

    m_pipelineCfg.numMB_refPic  = m_pipelineCfg.numMB_frame = (m_widthMB * m_heightMB) >> 8;
    m_pipelineCfg.numMB_refPic /= m_numOfFields;

    // num of MBs in reference frame/field
    m_widthMB  >>= 4;
    m_heightMB >>= m_numOfFields == 2 ? 5 : 4;

    if (m_pipelineCfg.bPreenc) {
        if (m_DSstrength){
            widthMBpreenc   = MSDK_ALIGN16(m_width / m_DSstrength);
            heightMBpreenc  = m_numOfFields == 2 ? MSDK_ALIGN32(m_height / m_DSstrength) :
                           MSDK_ALIGN16(m_height / m_DSstrength);

            m_pipelineCfg.numMB_preenc_refPic  = m_pipelineCfg.numMB_preenc_frame = (widthMBpreenc * heightMBpreenc) >> 8;
            m_pipelineCfg.numMB_preenc_refPic /= m_numOfFields;

            widthMBpreenc  >>= 4;
            heightMBpreenc >>= m_numOfFields == 2 ? 5 : 4;

        } else{
            widthMBpreenc  = m_widthMB;
            heightMBpreenc = m_heightMB;
            m_pipelineCfg.numMB_preenc_frame  = m_pipelineCfg.numMB_frame;
            m_pipelineCfg.numMB_preenc_refPic = m_pipelineCfg.numMB_refPic;
        }
    }

    mfxU16 nmvp_l0 = (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);
    mfxU16 nmvp_l1 = (std::min)(mfxU16(m_numRefFrame*m_numOfFields), MaxFeiEncMVPNum);

    m_pipelineCfg.numOfPredictors[0][0] = nmvp_l0;
    m_pipelineCfg.numOfPredictors[1][0] = nmvp_l0;
    m_pipelineCfg.numOfPredictors[0][1] = nmvp_l1;
    m_pipelineCfg.numOfPredictors[1][1] = nmvp_l1;

    return sts;
}
mfxStatus H264FEIEncoderElement::AllocExtBuffers()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxU32 fieldId = 0;

    //Preenc ext buffer setting, setup control structures
    if (m_pipelineCfg.bPreenc) {
        bool disableMVoutPreENC  = (m_extParams.mvoutStream == NULL) && !(m_pipelineCfg.bEnc || m_pipelineCfg.bEncode);
        bool disableMBStatPreENC = !(m_pipelineCfg.bEnc || m_pipelineCfg.bEncode);
        bool enableMVpredPreENC  = m_extParams.mvinStream != NULL;
        bool enableMBQP          = false;

        bufSet*                      tmpForPreencInit = NULL;
        mfxExtFeiPreEncCtrl*         preENCCtr  = NULL;
        mfxExtFeiPreEncMV*           mvs        = NULL;
        mfxExtFeiPreEncMBStat*       mbdata     = NULL;
        mfxExtFeiPreEncMVPredictors* mvPreds    = NULL;

        int num_buffers = m_maxQueueLength + 4;
        num_buffers = (std::max)(num_buffers, m_maxQueueLength*m_numRefFrame);

        for (int k = 0; k < num_buffers; k++)
        {
            tmpForPreencInit = new bufSet(m_numOfFields);

            for (fieldId = 0; fieldId < m_numOfFields; fieldId++)
            {
                /* We allocate buffer of progressive frame size for the first field if mixed pixstructs are used */
                mfxU32 numMB = m_pipelineCfg.numMB_preenc_refPic;

                if (fieldId == 0){
                    preENCCtr = new mfxExtFeiPreEncCtrl[m_numOfFields];
                    memset(preENCCtr, 0, sizeof(preENCCtr[0])*m_numOfFields);
                }

                preENCCtr[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_CTRL;
                preENCCtr[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncCtrl);
                preENCCtr[fieldId].PictureType             = GetCurPicType(fieldId);
                preENCCtr[fieldId].RefPictureType[0]       = preENCCtr[fieldId].PictureType;
                preENCCtr[fieldId].RefPictureType[1]       = preENCCtr[fieldId].PictureType;
                preENCCtr[fieldId].DownsampleInput         = MFX_CODINGOPTION_ON;
                // In sample_fei preenc works only in encoded order
                preENCCtr[fieldId].DownsampleReference[0]  = MFX_CODINGOPTION_OFF;
                // so all references would be already downsampled
                preENCCtr[fieldId].DownsampleReference[1]  = MFX_CODINGOPTION_OFF;
                preENCCtr[fieldId].DisableMVOutput         = disableMVoutPreENC;
                preENCCtr[fieldId].DisableStatisticsOutput = disableMBStatPreENC;
                preENCCtr[fieldId].FTEnable                = false;
                preENCCtr[fieldId].AdaptiveSearch          = m_extParams.fei_ctrl.nAdaptiveSearch;
                preENCCtr[fieldId].LenSP                   = m_extParams.fei_ctrl.nLenSP;
                preENCCtr[fieldId].MBQp                    = enableMBQP;
                preENCCtr[fieldId].MVPredictor             = enableMVpredPreENC;
                preENCCtr[fieldId].RefHeight               = m_extParams.fei_ctrl.nRefHeight;
                preENCCtr[fieldId].RefWidth                = m_extParams.fei_ctrl.nRefWidth;
                preENCCtr[fieldId].SubPelMode              = m_extParams.fei_ctrl.nSubPelMode;
                preENCCtr[fieldId].SearchWindow            = m_extParams.fei_ctrl.nSearchWindow;
                preENCCtr[fieldId].SearchPath              = m_extParams.fei_ctrl.nSearchPath;
                preENCCtr[fieldId].Qp                      = m_extParams.fei_ctrl.nQP;
                preENCCtr[fieldId].IntraSAD                = 2;
                preENCCtr[fieldId].InterSAD                = 2;
                preENCCtr[fieldId].SubMBPartMask           = m_extParams.fei_ctrl.nSubMBPartMask;
                preENCCtr[fieldId].IntraPartMask           = m_extParams.fei_ctrl.nIntraPartMask;
                preENCCtr[fieldId].Enable8x8Stat           = false;

                if (preENCCtr[fieldId].MVPredictor)
                {
                    if (fieldId == 0){
                        mvPreds = new mfxExtFeiPreEncMVPredictors[m_numOfFields];
                        MSDK_ZERO_ARRAY(mvPreds, m_numOfFields);
                    }

                    if(mvPreds){
                        mvPreds[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MV_PRED;
                        mvPreds[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncMVPredictors);
                        mvPreds[fieldId].NumMBAlloc = numMB;
                        mvPreds[fieldId].MB = new mfxExtFeiPreEncMVPredictors::mfxExtFeiPreEncMVPredictorsMB[mvPreds[fieldId].NumMBAlloc];
                        MSDK_ZERO_ARRAY(mvPreds[fieldId].MB, mvPreds[fieldId].NumMBAlloc);
                    } else {
                        MLOG_WARNING("Alloc Pre MV output buffer failed.\n");
                    }
                }

                if (!preENCCtr[fieldId].DisableMVOutput)
                {
                    if (fieldId == 0){
                        mvs = new mfxExtFeiPreEncMV[m_numOfFields];
                        MSDK_ZERO_ARRAY(mvs, m_numOfFields);
                    }
                    if (mvs) {
                        mvs[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MV;
                        mvs[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncMV);
                        mvs[fieldId].NumMBAlloc = numMB;
                        mvs[fieldId].MB = new mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB[mvs[fieldId].NumMBAlloc];
                        MSDK_ZERO_ARRAY(mvs[fieldId].MB, mvs[fieldId].NumMBAlloc);
                    } else {
                        MLOG_WARNING("Alloc MV output buffer failed.\n");
                    }
                }
                if (!preENCCtr[fieldId].DisableStatisticsOutput)
                {
                    if (fieldId == 0){
                        mbdata = new mfxExtFeiPreEncMBStat[m_numOfFields];
                        MSDK_ZERO_ARRAY(mbdata, m_numOfFields);
                    }

                    if (mbdata) {
                        mbdata[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MB;
                        mbdata[fieldId].Header.BufferSz = sizeof(mfxExtFeiPreEncMBStat);
                        mbdata[fieldId].NumMBAlloc = numMB;
                        mbdata[fieldId].MB = new mfxExtFeiPreEncMBStat::mfxExtFeiPreEncMBStatMB[mbdata[fieldId].NumMBAlloc];
                        MSDK_ZERO_ARRAY(mbdata[fieldId].MB, mbdata[fieldId].NumMBAlloc);
                    } else {
                        MLOG_WARNING("Alloc Pre MB data output buffer failed.\n");
                    }
                }
            } // for (fieldId = 0; fieldId < m_numOfFields; fieldId++)

            for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                tmpForPreencInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(preENCCtr + fieldId));
                tmpForPreencInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(preENCCtr + fieldId));
            }
            if (mvPreds){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForPreencInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(mvPreds + fieldId));
                    tmpForPreencInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(mvPreds + fieldId));
                }
            }
            if (mvs){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForPreencInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(mvs + fieldId));
                }
            }
            if (mbdata){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForPreencInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(mbdata + fieldId));
                    tmpForPreencInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(mbdata + fieldId));
                }
            }
            m_preencBufs.AddSet(tmpForPreencInit);
        } // for (int k = 0; k < num_buffers; k++)

    }

    // Encode ext buffer setting
    if (m_pipelineCfg.bEncode || m_pipelineCfg.bPak || m_pipelineCfg.bEnc) {
        bufSet*                   tmpForInit         = NULL;
        //FEI buffers
        mfxExtFeiEncFrameCtrl*    feiEncCtrl         = NULL;
        mfxExtFeiEncMVPredictors* feiEncMVPredictors = NULL;

        //FEI buffers
        mfxExtFeiPPS*             feiPPS             = NULL;
        mfxExtFeiSliceHeader*     feiSliceHeader     = NULL;

        mfxExtFeiEncMV*           feiEncMV           = NULL;
        mfxExtFeiPakMBCtrl*       feiEncMBCode       = NULL;
        mfxExtPredWeightTable*    feiWeights         = NULL;

        //couple with PREENC
        bool MVPredictors = m_extParams.mvinStream != NULL ||  m_pipelineCfg.bEncode|| m_pipelineCfg.bPreenc;
        //bool MBCtrl       = false;
        bool MVOut        = m_extParams.mvoutStream != NULL;
        bool MBCodeOut    =  m_extParams.mbcodeoutStream != NULL;
        bool Weights      =  m_extParams.weightStream != NULL;

        int num_buffers = m_maxQueueLength;

        for (int k = 0; k < num_buffers; k++)
        {
            tmpForInit = new bufSet(m_numOfFields);

            for (fieldId = 0; fieldId < m_numOfFields; fieldId++)
            {
                /* We allocate buffer of progressive frame size for the first field if mixed picstructs are used */
                mfxU32 numMB =  m_pipelineCfg.numMB_refPic;

                if (m_pipelineCfg.bEnc || m_pipelineCfg.bEncode) {
                    if (fieldId == 0){
                        feiEncCtrl = new mfxExtFeiEncFrameCtrl[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiEncCtrl, m_numOfFields);
                    }

                    if (feiEncCtrl) {
                        feiEncCtrl[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_CTRL;
                        feiEncCtrl[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncFrameCtrl);

                        feiEncCtrl[fieldId].SearchPath             = m_extParams.fei_ctrl.nSearchPath;
                        feiEncCtrl[fieldId].LenSP                  = m_extParams.fei_ctrl.nLenSP;
                        feiEncCtrl[fieldId].SubMBPartMask          = m_extParams.fei_ctrl.nSubMBPartMask;
                        feiEncCtrl[fieldId].MultiPredL0            = m_extParams.fei_ctrl.bMultiPredL0;
                        feiEncCtrl[fieldId].MultiPredL1            = m_extParams.fei_ctrl.bMultiPredL1;
                        feiEncCtrl[fieldId].SubPelMode             = m_extParams.fei_ctrl.nSubPelMode;
                        feiEncCtrl[fieldId].InterSAD               = 2;
                        feiEncCtrl[fieldId].IntraSAD               = 2;
                        feiEncCtrl[fieldId].IntraPartMask          = m_extParams.fei_ctrl.nIntraPartMask;
                        feiEncCtrl[fieldId].DistortionType         = m_extParams.fei_ctrl.bDistortionType;
                        feiEncCtrl[fieldId].RepartitionCheckEnable = m_extParams.fei_ctrl.bRepartitionCheckEnable;
                        feiEncCtrl[fieldId].AdaptiveSearch         = m_extParams.fei_ctrl.nAdaptiveSearch;
                        feiEncCtrl[fieldId].MVPredictor            = MVPredictors;
                        feiEncCtrl[fieldId].NumMVPredictors[0]     = m_pipelineCfg.numOfPredictors[fieldId][0];
                        feiEncCtrl[fieldId].NumMVPredictors[1]     = m_pipelineCfg.numOfPredictors[fieldId][1];
                        feiEncCtrl[fieldId].PerMBQp                = false;
                        feiEncCtrl[fieldId].PerMBInput             = false;
                        feiEncCtrl[fieldId].MBSizeCtrl             = m_extParams.fei_ctrl.bMBSize;
                        feiEncCtrl[fieldId].ColocatedMbDistortion  = m_extParams.fei_ctrl.bColocatedMbDistortion;

                        //Note:(RefHeight x RefWidth) should not exceed 2048 for P frames and 1024 for B frames
                        feiEncCtrl[fieldId].RefHeight    = m_extParams.fei_ctrl.nRefHeight;
                        feiEncCtrl[fieldId].RefWidth     = m_extParams.fei_ctrl.nRefWidth;
                        feiEncCtrl[fieldId].SearchWindow = m_extParams.fei_ctrl.nSearchWindow;
                    } else {
                        MLOG_WARNING("mfxExtFeiEncFrameCtrl alloc failed.");
                    }
                }

                /* PPS Header */
                if (m_pipelineCfg.bEnc || m_pipelineCfg.bPak)
                {
                    if (fieldId == 0){
                        feiPPS = new mfxExtFeiPPS[m_numOfFields]();
                        //MSDK_ZERO_ARRAY(feiPPS, m_numOfFields);
                    }

                    if (feiPPS) {
                        feiPPS[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PPS;
                        feiPPS[fieldId].Header.BufferSz = sizeof(mfxExtFeiPPS);

                        feiPPS[fieldId].SPSId = 0;
                        feiPPS[fieldId].PPSId = 0;

                        /* PicInitQP should be always 26 !!!
                         * Adjusting of QP parameter should be done via Slice header */
                        feiPPS[fieldId].PicInitQP = 26;

                        feiPPS[fieldId].NumRefIdxL0Active = 1;
                        feiPPS[fieldId].NumRefIdxL1Active = 1;

                        feiPPS[fieldId].ChromaQPIndexOffset       = m_extParams.fei_ctrl.nChromaQPIndexOffset;
                        feiPPS[fieldId].SecondChromaQPIndexOffset = m_extParams.fei_ctrl.nSecondChromaQPIndexOffset;
                        feiPPS[fieldId].Transform8x8ModeFlag      = m_extParams.fei_ctrl.bTransform8x8ModeFlag;

    #if MFX_VERSION >= 1023
                        memset(feiPPS[fieldId].DpbBefore, 0xffff, sizeof(feiPPS[fieldId].DpbBefore));
                        memset(feiPPS[fieldId].DpbAfter,  0xffff, sizeof(feiPPS[fieldId].DpbAfter));
    #else
                        memset(feiPPS[fieldId].ReferenceFrames, 0xffff, 16 * sizeof(mfxU16));
    #endif // MFX_VERSION >= 1023
                    } else {
                        MLOG_WARNING("mfxExtFeiPPS alloc failed.");
                    }
                }

                /* Slice Header */
                if (m_pipelineCfg.bEnc || m_pipelineCfg.bPak)
                {
                    if (fieldId == 0){
                        feiSliceHeader = new mfxExtFeiSliceHeader[m_numOfFields]();
                    }

                    if (feiSliceHeader) {
                        feiSliceHeader[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_SLICE;
                        feiSliceHeader[fieldId].Header.BufferSz = sizeof(mfxExtFeiSliceHeader);

                        feiSliceHeader[fieldId].NumSlice = m_numSlice;
                        feiSliceHeader[fieldId].Slice = new mfxExtFeiSliceHeader::mfxSlice[feiSliceHeader[fieldId].NumSlice]();

                        // TODO: Improve slice divider
                        mfxU16 nMBrows = (m_heightMB + feiSliceHeader[fieldId].NumSlice - 1) / feiSliceHeader[fieldId].NumSlice,
                            nMBremain = m_heightMB;

                        for (mfxU16 numSlice = 0; numSlice < feiSliceHeader[fieldId].NumSlice; numSlice++)
                        {
                            feiSliceHeader[fieldId].Slice[numSlice].MBAddress = numSlice*(nMBrows*m_widthMB);
                            feiSliceHeader[fieldId].Slice[numSlice].NumMBs    = (std::min)(nMBrows, nMBremain)*m_widthMB;
                            feiSliceHeader[fieldId].Slice[numSlice].SliceType = 0;
                            feiSliceHeader[fieldId].Slice[numSlice].PPSId     = feiPPS ? feiPPS[fieldId].PPSId : 0;
                            feiSliceHeader[fieldId].Slice[numSlice].IdrPicId  = 0;

                            feiSliceHeader[fieldId].Slice[numSlice].CabacInitIdc = 0;
                            mfxU32 initQP = (m_encCtrl.QP != 0) ? m_encCtrl.QP : 26;
                            feiSliceHeader[fieldId].Slice[numSlice].SliceQPDelta               = mfxI16(initQP - (feiPPS ? feiPPS[fieldId].PicInitQP : 26));
                            feiSliceHeader[fieldId].Slice[numSlice].DisableDeblockingFilterIdc = m_extParams.fei_ctrl.nDisableDeblockingIdc;
                            feiSliceHeader[fieldId].Slice[numSlice].SliceAlphaC0OffsetDiv2     = m_extParams.fei_ctrl.nSliceAlphaC0OffsetDiv2;
                            feiSliceHeader[fieldId].Slice[numSlice].SliceBetaOffsetDiv2        = m_extParams.fei_ctrl.nSliceBetaOffsetDiv2;

                            nMBremain -= nMBrows;
                        }
                    } else {
                        MLOG_WARNING("mfxExtFeiSliceHeader alloc failed.");
                    }
                }

                if (MVPredictors)
                {
                    if (fieldId == 0){
                        feiEncMVPredictors = new mfxExtFeiEncMVPredictors[m_numOfFields]();
                    }

                    if (feiEncMVPredictors) {
                        feiEncMVPredictors[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_MV_PRED;
                        feiEncMVPredictors[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncMVPredictors);
                        feiEncMVPredictors[fieldId].NumMBAlloc = numMB;
                        feiEncMVPredictors[fieldId].MB = new mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB[feiEncMVPredictors[fieldId].NumMBAlloc];
                        MSDK_ZERO_ARRAY(feiEncMVPredictors[fieldId].MB, feiEncMVPredictors[fieldId].NumMBAlloc);
                    } else {
                        MLOG_WARNING("mfxExtFeiEncMVPredictors alloc failed.");
                    }
                }

                if ((m_pipelineCfg.bPak) || (MVOut))
                {
                    MVOut = true; /* ENC_PAK need MVOut and MBCodeOut buffer */
                    if (fieldId == 0){
                        feiEncMV = new mfxExtFeiEncMV[m_numOfFields]();
                    }

                    if (feiEncMV) {
                        feiEncMV[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_ENC_MV;
                        feiEncMV[fieldId].Header.BufferSz = sizeof(mfxExtFeiEncMV);
                        feiEncMV[fieldId].NumMBAlloc = numMB;
                        feiEncMV[fieldId].MB = new mfxExtFeiEncMV::mfxExtFeiEncMVMB[feiEncMV[fieldId].NumMBAlloc]();
                    } else {
                        MLOG_WARNING("mfxExtFeiEncMV alloc failed.");
                    }
                }

                //distortion buffer have to be always provided - current limitation
                if ((m_pipelineCfg.bPak) || (MBCodeOut))
                {
                    MBCodeOut = true; /* ENC_PAK need MVOut and MBCodeOut buffer */
                    if (fieldId == 0){
                        feiEncMBCode = new mfxExtFeiPakMBCtrl[m_numOfFields]();
                    }

                    if (feiEncMBCode) {
                        feiEncMBCode[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_PAK_CTRL;
                        feiEncMBCode[fieldId].Header.BufferSz = sizeof(mfxExtFeiPakMBCtrl);
                        feiEncMBCode[fieldId].NumMBAlloc = numMB;
                        feiEncMBCode[fieldId].MB = new mfxFeiPakMBCtrl[feiEncMBCode[fieldId].NumMBAlloc]();
                    } else {
                        MLOG_WARNING("mfxExtFeiPakMBCtrl alloc failed.");
                    }
                }

                if (Weights)
                {
                    if (fieldId == 0){
                        feiWeights = new mfxExtPredWeightTable[m_numOfFields];
                        MSDK_ZERO_ARRAY(feiWeights, m_numOfFields);
                    }

                    if (feiWeights) {
                        feiWeights[fieldId].Header.BufferId = MFX_EXTBUFF_PRED_WEIGHT_TABLE;
                        feiWeights[fieldId].Header.BufferSz = sizeof(mfxExtPredWeightTable);
                    } else {
                        MLOG_WARNING("mfxExtPredWeightTable alloc failed.");
                    }
                }
            } // for (fieldId = 0; fieldId < m_numOfFields; fieldId++)

            if (feiEncCtrl)
            {
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncCtrl[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncCtrl[fieldId]));
                }
            }
            if (feiPPS){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiPPS[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiPPS[fieldId]));
                }
            }
            if (feiSliceHeader){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiSliceHeader[fieldId]));
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiSliceHeader[fieldId]));
                }
            }
            if (MVPredictors){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    //tmpForInit-> I_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(feiEncMVPredictors[fieldId]);
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMVPredictors[fieldId]));
                }
            }
            if (MVOut){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMV[fieldId]));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMV[fieldId]));
                }
            }
            if (Weights){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    //weight table is never for I frame
                    tmpForInit->PB_bufs.in.Add(reinterpret_cast<mfxExtBuffer*>(&feiWeights[fieldId]));
                }
            }
            if (MBCodeOut){
                for (fieldId = 0; fieldId < m_numOfFields; fieldId++){
                    tmpForInit-> I_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMBCode[fieldId]));
                    tmpForInit->PB_bufs.out.Add(reinterpret_cast<mfxExtBuffer*>(&feiEncMBCode[fieldId]));
                }
            }
            m_encodeBufs.AddSet(tmpForInit);
        } // for (int k = 0; k < num_buffers; k++)
    }

    return sts;
}

mfxStatus H264FEIEncoderElement::InitEncoder(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest EncRequest;
    mfxFrameAllocRequest pakRequest[2];
    mfxU16 refDist = m_refDist;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(PREENC_ENCODE_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }
    memset(&EncRequest, 0, sizeof(EncRequest));
    memset(&pakRequest[0], 0, sizeof(mfxFrameAllocRequest));
    memset(&pakRequest[1], 0, sizeof(mfxFrameAllocRequest));

    //for common used params
    m_picStruct = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    m_numOfFields = (m_picStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;
    m_preencBufs.num_of_fields = m_encodeBufs.num_of_fields = m_numOfFields;
    m_width = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    m_height = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;


    if (m_preenc) {
        sts = m_preenc->InitPreenc(buf, m_DSRequest);
        if (MFX_ERR_NONE > sts){
            MLOG_ERROR("PREENC create failed %d\n", sts);
            return sts;
        }
        m_commonFrameInfo = (m_preenc->GetCommonVideoParams())->mfx.FrameInfo;
    }

    if (m_encode) {
        sts = m_encode->InitEncode(buf);
        if (MFX_ERR_NONE > sts){
            MLOG_ERROR("Encode create failed %d\n", sts);
            return sts;
        }
        m_commonFrameInfo = (m_encode->GetCommonVideoParams())->mfx.FrameInfo;

        sts = m_encode->QueryIOSurf(&EncRequest);
        MLOG_INFO("Encode suggest surfaces %d\n", EncRequest.NumFrameSuggested);
        assert(sts >= MFX_ERR_NONE);
    }

    if (m_enc) {
        sts = m_enc->InitEnc(buf);
        if (MFX_ERR_NONE > sts){
            MLOG_ERROR("Enc create failed %d\n", sts);
            return sts;
        }
        m_commonFrameInfo = (m_enc->GetCommonVideoParams())->mfx.FrameInfo;
        if (!m_pak) {
            sts = m_enc->QueryIOSurf(pakRequest);
            assert(sts >= MFX_ERR_NONE);
        }
    }

    if (m_pak) {
        sts = m_pak->InitPak(buf);
        if (MFX_ERR_NONE > sts){
            MLOG_ERROR("PAK create failed %d\n", sts);
            return sts;
        }
        m_commonFrameInfo = (m_pak->GetCommonVideoParams())->mfx.FrameInfo;
        sts = m_pak->QueryIOSurf(pakRequest);
        assert(sts >= MFX_ERR_NONE);
    }

    //alloc bitstream;
    if (m_pak || m_enc) {
        mfxVideoParam* par = m_enc? m_enc->GetCommonVideoParams(): m_pak->GetCommonVideoParams();
        // Prepare Media SDK bit stream buffer for encoder
        //memset(&m_bitstream, 0, sizeof(m_bitstream));
        if (par->mfx.FrameInfo.Height * par->mfx.FrameInfo.Width * 4 < par->mfx.BufferSizeInKB * 1024) {
            m_bitstream.MaxLength = par->mfx.BufferSizeInKB * 1024;
        }else{
            m_bitstream.MaxLength = par->mfx.FrameInfo.Height * par->mfx.FrameInfo.Width * 4;
        }
        m_bitstream.Data = new mfxU8[m_bitstream.MaxLength];
        MLOG_INFO("ENC PAK output bitstream buffer size %d\n", m_bitstream.MaxLength);
    }

    //AllocFrames
    m_maxQueueLength = refDist * 2 + 1;
    m_maxQueueLength += m_numRefFrame + 1;
    EncRequest.NumFrameMin = m_maxQueueLength;
    EncRequest.NumFrameSuggested = m_maxQueueLength;
    memcpy(&EncRequest.Info, &m_commonFrameInfo, sizeof(mfxFrameInfo));
    EncRequest.Type |= MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    if (m_preenc)
        EncRequest.Type |= m_DSstrength ? MFX_MEMTYPE_FROM_VPPIN : MFX_MEMTYPE_FROM_ENC;
    if (m_enc || m_pak)
        EncRequest.Type |= (MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK);

    if (m_pipelineCfg.bPreenc && m_DSstrength){
        // these surfaces are input surfaces for PREENC
        m_DSRequest[1].NumFrameMin = m_DSRequest[1].NumFrameSuggested = EncRequest.NumFrameSuggested;
        m_DSRequest[1].Type        = EncRequest.Type | MFX_MEMTYPE_FROM_ENC;
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &(m_DSRequest[1]), &m_dsResponse);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tm_pMFXAllocator->Alloc failed with %d\n", sts);
            return sts;
        }
        m_DSSurfaces.PoolSize = m_dsResponse.NumFrameActual;
        sts = FillSurfacePool(m_DSSurfaces.SurfacesPool, &m_dsResponse, &((m_preenc->m_DSParams).vpp.Out));
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tFillSurfacePool failed with %d\n", sts);
            return sts;
        }
    }

    // ENC PAK
    if (m_enc || m_pak) {
        memcpy(&(pakRequest[1].Info), &m_commonFrameInfo, sizeof(mfxFrameInfo));

        if (m_enc)
        {
            pakRequest[1].Type |= MFX_MEMTYPE_FROM_ENC;
        }

        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &pakRequest[1], &m_reconResponse);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tm_pMFXAllocator->Alloc failed with %d\n", sts);
            return sts;
        }

        m_reconSurfaces.PoolSize = m_reconResponse.NumFrameActual;
        sts = FillSurfacePool(m_reconSurfaces.SurfacesPool, &m_reconResponse, &m_commonFrameInfo);

        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tFillSurfacePool failed with %d\n", sts);
            return sts;
        }
        m_reconSurfaces.UpdatePicStructs(m_picStruct);
    }

    SetSequenceParameters();

    //Allocate ExtBuffers for PREENC and ENCODE ctrl
    AllocExtBuffers();

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(PREENC_ENCODE_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    return sts;
}

#if MFX_VERSION < 1023
mfxStatus H264FEIEncoderElement::FillRefInfo(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;

    m_RefInfo.Clear();

    iTask* ref_task = NULL;
    mfxFrameSurface1* ref_surface = NULL;
    std::vector<mfxFrameSurface1*>::iterator rslt;
    mfxU32 k = 0, fid, n_l0, n_l1, numOfFields = eTask->m_fieldPicFlag ? 2 : 1;

    for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
    {
        fid = eTask->m_fid[fieldId];
        for (DpbFrame* instance = eTask->m_dpb[fid].Begin(); instance != eTask->m_dpb[fid].End(); instance++)
        {
            ref_task = m_inputTasks.GetTaskByFrameOrder(instance->m_frameOrder);
            MSDK_CHECK_POINTER(ref_task, MFX_ERR_NULL_PTR);

            ref_surface = ref_task->PAK_out.OutSurface; // this is shared output surface for ENC reference / PAK reconstruct
            MSDK_CHECK_POINTER(ref_surface, MFX_ERR_NULL_PTR);

            rslt = std::find(m_RefInfo.reference_frames.begin(), m_RefInfo.reference_frames.end(), ref_surface);

            if (rslt == m_RefInfo.reference_frames.end()){
                m_RefInfo.state[fieldId].dpb_idx.push_back((mfxU16)m_RefInfo.reference_frames.size());
                m_RefInfo.reference_frames.push_back(ref_surface);
            }
            else{
                m_RefInfo.state[fieldId].dpb_idx.push_back(static_cast<mfxU16>(std::distance(m_RefInfo.reference_frames.begin(), rslt)));
            }
        }

        /* in some cases l0 and l1 lists are equal, if so same ref lists for l0 and l1 should be used*/
        n_l0 = eTask->GetNBackward(fieldId);
        n_l1 = eTask->GetNForward(fieldId);

        if (!n_l0 && eTask->m_list0[fid].Size() && !(eTask->m_type[fid] & MFX_FRAMETYPE_I))
        {
            n_l0 = eTask->m_list0[fid].Size();
        }

        if (!n_l1 && eTask->m_list1[fid].Size() && (eTask->m_type[fid] & MFX_FRAMETYPE_B))
        {
            n_l1 = eTask->m_list1[fid].Size();
        }

        k = 0;
        for (mfxU8 const * instance = eTask->m_list0[fid].Begin(); k < n_l0 && instance != eTask->m_list0[fid].End(); instance++)
        {
            ref_task = m_inputTasks.GetTaskByFrameOrder(eTask->m_dpb[fid][*instance & 127].m_frameOrder);
            MSDK_CHECK_POINTER(ref_task, MFX_ERR_NULL_PTR);

            ref_surface = ref_task->PAK_out.OutSurface; // this is shared output surface for ENC reference / PAK reconstruct
            MSDK_CHECK_POINTER(ref_surface, MFX_ERR_NULL_PTR);

            rslt = std::find(m_RefInfo.reference_frames.begin(), m_RefInfo.reference_frames.end(), ref_surface);

            if (rslt == m_RefInfo.reference_frames.end()){
                return MFX_ERR_MORE_SURFACE; // surface from L0 list not in DPB (should never happen)
            }
            else{
                m_RefInfo.state[fieldId].l0_idx.push_back(static_cast<mfxU16>(std::distance(m_RefInfo.reference_frames.begin(), rslt)));
            }

            //m_ref_info.state[fieldId].l0_idx.push_back(*instance & 127);
            m_RefInfo.state[fieldId].l0_parity.push_back((*instance) >> 7);
            k++;
        }

        k = 0;
        for (mfxU8 const * instance = eTask->m_list1[fid].Begin(); k < n_l1 && instance != eTask->m_list1[fid].End(); instance++)
        {
            ref_task = m_inputTasks.GetTaskByFrameOrder(eTask->m_dpb[fid][*instance & 127].m_frameOrder);
            MSDK_CHECK_POINTER(ref_task, MFX_ERR_NULL_PTR);

            ref_surface = ref_task->PAK_out.OutSurface; // this is shared output surface for ENC reference / PAK reconstruct
            MSDK_CHECK_POINTER(ref_surface, MFX_ERR_NULL_PTR);

            rslt = std::find(m_RefInfo.reference_frames.begin(), m_RefInfo.reference_frames.end(), ref_surface);

            if (rslt == m_RefInfo.reference_frames.end()){
                return MFX_ERR_MORE_SURFACE; // surface from L0 list not in DPB (should never happen)
            }
            else{
                m_RefInfo.state[fieldId].l1_idx.push_back(static_cast<mfxU16>(std::distance(m_RefInfo.reference_frames.begin(), rslt)));
            }

            //m_ref_info.state[fieldId].l1_idx.push_back(*instance & 127);
            m_RefInfo.state[fieldId].l1_parity.push_back((*instance) >> 7);
            k++;
        }
    }

    return sts;
}
#endif // MFX_VERSION < 1023

int H264FEIEncoderElement::HandleProcessEncode()
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(0 != this->sinkpads_.size());
    iTask* eTask = NULL; // encoding task
    mfxFrameSurface1 *pmfxInSurface;
    mfxI32 eos_FrameCount = -1;
    bool is_eos = false;
    bool swap_fields =false;

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
                sts = MFX_ERR_UNKNOWN;
                break;
            }
            sts = InitEncoder(buf);

            if (MFX_ERR_NONE == sts) {
                codec_init_ = true;

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(PREENC_ENCODE_ENDURATION_TIME_STAMP, this);
                pipelineinfo einfo;
                einfo.mElementType = element_type_;
                einfo.mChannelNum = H264FEIEncoderElement::mNumOfEncChannels;
                H264FEIEncoderElement::mNumOfEncChannels++;
                einfo.mCodecName = "AVC";

                measurement->SetElementInfo(PREENC_ENCODE_ENDURATION_TIME_STAMP, this, &einfo);
                measurement->RelLock();
            }
            MLOG_INFO("PreencEncoder %p init successfully\n", this);

            } else {
                MLOG_ERROR("PreencEncoder create failed %d\n", sts);
                break;
            }
        }

        m_frameType = GetFrameType(m_frameCount - m_frameOrderIdrInDisplayOrder);

        if (m_frameType[0] & MFX_FRAMETYPE_IDR)
            m_frameOrderIdrInDisplayOrder = m_frameCount;

        if (pmfxInSurface) {
            pmfxInSurface->Data.FrameOrder = m_frameCount; // in display order
        }
        m_frameCount++;

        if(pmfxInSurface){
            swap_fields = (pmfxInSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF) && !(pmfxInSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
            if (swap_fields){
                std::swap(m_frameType[0], m_frameType[1]);
            }
        }

        if (!buf.is_eos && pmfxInSurface) {
            if ((pmfxInSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && (pmfxInSurface->Info.PicStruct != m_picStruct)) {
                m_taskInitializationParams.PicStruct = m_picStruct;
            } else {
                m_taskInitializationParams.PicStruct  = pmfxInSurface->Info.PicStruct & 0xf;
            }
            m_picStruct = m_taskInitializationParams.PicStruct;
        }else {
            m_taskInitializationParams.PicStruct  = 0;
            eos_FrameCount = m_frameCount - 1;
        }

        if (m_DSstrength && !buf.is_eos) {
            m_taskInitializationParams.DSsurface = m_DSSurfaces.GetFreeSurface_FEI();
            if (!m_taskInitializationParams.DSsurface){
                MLOG_ERROR("failed to allocate frame for DS with error MFX_ERR_MEMORY_ALLOC\n");
                sts = MFX_ERR_NULL_PTR;
                break;
            }
        }

        if (!buf.is_eos) { //don't add task after the end of stream appeared
            m_taskInitializationParams.InputSurf  = pmfxInSurface;
            m_taskInitializationParams.FrameType  = m_frameType;
            m_taskInitializationParams.FrameCount = m_frameCount - 1;
            m_taskInitializationParams.FrameOrderIdrInDisplayOrder = m_frameOrderIdrInDisplayOrder;
            m_taskInitializationParams.SingleFieldMode = false;

            m_inputTasks.AddTask(new iTask(m_taskInitializationParams));
        }
        eTask = m_inputTasks.GetTaskToEncode(false);

        // No frame to process right now (bufferizing B-frames, need more input frames)
        if (!eTask && !buf.is_eos) continue;

        //process last frame if it's B-frame
        if (!eTask && buf.is_eos && (m_inputTasks.CountUnencodedFrames()) == 0 ) {
            output_stream_->SetEndOfStream();
            is_running_ = false;
            break;

        } else if (!eTask && buf.is_eos) {
            eTask = m_inputTasks.GetTaskToEncode(true);
        }

       if ((m_enc || m_pak) ) {
            mfxFrameSurface1 *reconSurf = m_reconSurfaces.GetFreeSurface_FEI();
            MSDK_CHECK_POINTER(reconSurf, MFX_ERR_MEMORY_ALLOC);
            if(eTask){
                eTask->SetReconSurf(reconSurf);
            }
        }

        //buf.is_eos = 0;
        if (eTask && eTask->m_frameOrder == eos_FrameCount && (m_inputTasks.CountUnencodedFrames()) == 1){
            is_eos = true;
        }

        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpStart(PREENC_ENCODE_FRAME_TIME_STAMP, this);
            measurement->RelLock();
        }

        if (m_preenc) {
            if (eTask && eTask->PREENC_in.InSurface && eTask->ENC_in.InSurface) {
                sts = m_preenc->ProcessChainPreenc(eTask);
                if (sts < MFX_ERR_NONE) {
                    MLOG_ERROR("FEI PREENC: ProcessChainPreenc failed.");
                    break;
                }

            }
        }

        if ((m_enc || m_pak)) {
            if(eTask){
                eTask->PAK_out.Bs = &m_bitstream;
                eTask->bufs = m_encodeBufs.GetFreeSet();
                //SDK_CHECK_POINTER(eTask->bufs, MFX_ERR_NULL_PTR);
                if (!eTask->bufs) {
                    sts = MFX_ERR_NULL_PTR;
                    break;
                }
            }

#if MFX_VERSION >= 1023
            // Fill information about DPB states and reference lists
            sts = m_RefInfo.Fill(eTask, &m_inputTasks);
            if (sts < MFX_ERR_NONE)
                MLOG_ERROR("RefInfo.Fill failed, sts = %d\n", sts);
#else
            sts = FillRefInfo(eTask); // get info to fill reference structures
            if (sts < MFX_ERR_NONE)
                MLOG_ERROR("FillRefInfo failed");
#endif // MFX_VERSION >= 1023
        }
        if (m_enc) {
            sts = m_enc->ProcessOneFrame(eTask, &m_RefInfo);
            if (sts < MFX_ERR_NONE) {
                MLOG_ERROR("FEI ENC: ProcessOneFrame failed.");
                break;;
            }
        }
        if (m_pak) {
            sts = m_pak->ProcessOneFrame(eTask, &m_RefInfo);
            if (sts < MFX_ERR_NONE) {
                MLOG_ERROR("FEI PAK: ProcessOneFrame failed.");
                break;
            }
        }

        if (m_encode) {
            sts = m_encode->ProcessChainEncode(eTask, is_eos);
            if (sts < MFX_ERR_NONE) {
                MLOG_ERROR("FEI ENCODE: ProcessOneFrame failed.");
                break;;
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


        //restore surface for buf
        if (eTask) {
            buf.msdk_surface = eTask->ENC_in.InSurface ? eTask->ENC_in.InSurface : eTask->PREENC_in.InSurface;
        }
        if (sts == MFX_ERR_NONE) {
            sinkpad->ReturnBufToPeerPad(buf);
        }

        m_inputTasks.UpdatePool();
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(PREENC_ENCODE_ENDURATION_TIME_STAMP, this);
        measurement->RelLock();
    }

    if (is_running_) {
        output_stream_->SetEndOfStream();
        is_running_ = false;
    }
    while (sinkpad->GetBufData(buf) == 0) {
        sinkpad->ReturnBufToPeerPad(buf);
    }

    if (sts == MFX_ERR_NONE)
        return 0;
    else
        return -1;

}

int H264FEIEncoderElement::HandleProcess()
{
    return HandleProcessEncode();
}

mfxStatus H264FEIEncoderElement::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_encode) {
        sts = m_encode->GetVideoParam(par);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("ENCODE: Get video parameters Failed.");
        }
    }

    if (m_enc) {
        sts = m_enc->GetVideoParam(par);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("ENC: Get video parameters Failed.");
        }
    }

    if (m_pak) {
        sts = m_pak->GetVideoParam(par);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("PAK: Get video parameters Failed.");
        }
    }

    return sts;
}
mfxStatus H264FEIEncoderElement::FillSurfacePool(mfxFrameSurface1* & surfacesPool, mfxFrameAllocResponse* allocResponse, mfxFrameInfo* FrameInfo)
{
    mfxStatus sts = MFX_ERR_NONE;

    surfacesPool = new mfxFrameSurface1[allocResponse->NumFrameActual];
    MSDK_ZERO_ARRAY(surfacesPool, allocResponse->NumFrameActual);

    for (int i = 0; i < allocResponse->NumFrameActual; i++)
    {
        memcpy(&surfacesPool[i].Info, FrameInfo, sizeof(mfxFrameInfo));
        surfacesPool[i].Data.MemId = allocResponse->mids[i]; //m_bUseHWmemory==TRUE
    }
    return sts;
}
inline mfxU16 H264FEIEncoderElement::GetCurPicType(mfxU32 fieldId)
{

    switch (m_picStruct & 0xf)
    {
    case MFX_PICSTRUCT_FIELD_TFF:
        return mfxU16(fieldId ? MFX_PICTYPE_BOTTOMFIELD : MFX_PICTYPE_TOPFIELD);

    case MFX_PICSTRUCT_FIELD_BFF:
        return mfxU16(fieldId ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);

    case MFX_PICSTRUCT_PROGRESSIVE:
    default:
        return MFX_PICTYPE_FRAME;
    }
}
PairU8 H264FEIEncoderElement::GetFrameType(mfxU32 frameOrder)
{
    mfxU16 refDist, gopOptFlag, gopSize, idrInterval;
    refDist = m_refDist;
    gopOptFlag = m_gopOptFlag;
    gopSize = m_gopPicSize;
    idrInterval = m_idrInterval;

    static mfxU32 idrPicDist = (gopSize != 0xffff) ? (std::max)(mfxU32(gopSize * (idrInterval + 1)), mfxU32(1))
                                                     : 0xffffffff; // infinite GOP

    if (frameOrder % idrPicDist == 0)
        return ExtendFrameType(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);

    if (frameOrder % gopSize == 0)
        return ExtendFrameType(MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF);

    if (frameOrder % gopSize % refDist == 0)
        return ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);

    if ((gopOptFlag & MFX_GOP_STRICT) == 0)
        if (((frameOrder + 1) % gopSize == 0 && (gopOptFlag & MFX_GOP_CLOSED)) ||
            (frameOrder + 1) % idrPicDist == 0)
            return ExtendFrameType(MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF); // switch last B frame to P frame

    return ExtendFrameType(MFX_FRAMETYPE_B);
}

PairU8 ExtendFrameType(mfxU32 type)
{
    mfxU32 type1 = type & 0xff;
    mfxU32 type2 = type >> 8;

    if (type2 == 0)
    {
        type2 = type1 & ~MFX_FRAMETYPE_IDR; // second field can't be IDR

        if (type1 & MFX_FRAMETYPE_I)
        {
            type2 &= ~MFX_FRAMETYPE_I;
            type2 |= MFX_FRAMETYPE_P;
        }
    }

    return PairU8(type1, type2);
}
#endif //ifdef MSDK_AVC_FEI
