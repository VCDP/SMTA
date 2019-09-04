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

#include "h264_fei_encoder_encode.h"
#ifdef MSDK_AVC_FEI

//#include "msdk_codec.h"

DEFINE_MLOGINSTANCE_CLASS(H264FEIEncoderEncode, "H264FEIEncoderEncode");

H264FEIEncoderEncode::H264FEIEncoderEncode(MFXVideoSession *session,
                                   bufList* ext_buf,
                                   bufList* enc_ext_buf,
                                   bool UseOpaqueMemory,
                                   PipelineCfg* cfg,
                                   mfxEncodeCtrl* encodeCtrl,
                                   EncExtParams* extParams)
    :m_session(session),
     m_pipelineCfg(cfg),
     m_extBufs(ext_buf),
     m_encodeBufs(enc_ext_buf),
     m_encodeCtrl(encodeCtrl),
     m_extParams(extParams),
     m_bUseOpaqueMemory(UseOpaqueMemory)
{
    memset(&output_bs_, 0, sizeof(output_bs_));
    memset(&m_encodeParams, 0, sizeof(mfxVideoParam));
    memset(&m_extFEIParam, 0, sizeof(m_extFEIParam));
    memset(&m_extOpaqueAlloc, 0, sizeof(m_extOpaqueAlloc));
    memset(&m_extFEIParam, 0, sizeof(m_extFEIParam));
    memset(&m_codingOpt3, 0, sizeof(m_codingOpt3));
    memset(&m_codingOpt2, 0, sizeof(m_codingOpt2));
#if (MFX_VERSION >= 1025)
    memset(&m_multiFrame, 0, sizeof(m_multiFrame));
    memset(&m_multiFrameControl, 0, sizeof(m_multiFrameControl));

    m_nMFEFrames = 0;
    m_nMFEMode = 0;
    m_nMFETimeout = 0;
#endif

    reset_res_flag_ = false;

    m_outputStream = NULL;
    m_pMvoutStream = NULL,
    m_pMvinStream = NULL,
    m_pMBcodeoutStream = NULL;
    m_pWeightStream = NULL;

    m_encode = NULL;    
}
H264FEIEncoderEncode::~H264FEIEncoderEncode()
{
    mfxExtFeiSliceHeader* pSlices = NULL;

    if (m_encode) {
        m_encode->Close();
        delete m_encode;
        m_encode = NULL;
    }

    if (NULL != output_bs_.Data) {
        delete[] output_bs_.Data;
        output_bs_.Data = NULL;
    }

    for (mfxU32 i = 0; i < m_EncodeExtParams.size(); ++i)
    {
        switch (m_EncodeExtParams[i]->BufferId)
        {
        case MFX_EXTBUFF_FEI_PARAM:
        case MFX_EXTBUFF_CODING_OPTION2:
        case MFX_EXTBUFF_CODING_OPTION3:
        break;
        case MFX_EXTBUFF_FEI_SLICE:
        {
            mfxExtFeiSliceHeader* ptr = reinterpret_cast<mfxExtFeiSliceHeader*>(m_EncodeExtParams[i]);
            MSDK_SAFE_DELETE_ARRAY(ptr->Slice);
            if (!pSlices) { pSlices = ptr; }
        }
        break;
        }
    }
    MSDK_SAFE_DELETE_ARRAY(pSlices);
}

bool H264FEIEncoderEncode::Init(ElementCfg *ele_param)
{
    m_outputStream = ele_param->output_stream;
    m_encode = new MFXVideoENCODE(*m_session);
    /* Encoding Options */
    m_encodeParams.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
    m_encodeParams.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
    m_encodeParams.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
    m_encodeParams.mfx.TargetUsage = 0;
    m_encodeParams.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize==0? 30: ele_param->EncParams.mfx.GopPicSize;
    m_encodeParams.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist==0? 4: ele_param->EncParams.mfx.GopRefDist;
    m_encodeParams.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
    m_encodeParams.mfx.GopOptFlag = ele_param->EncParams.mfx.GopOptFlag;

    if (ele_param->EncParams.mfx.RateControlMethod != MFX_RATECONTROL_CQP) {
        MLOG_ERROR("PREENC+ENCODE only supports CQP for BRC\n");
        return false;
    }

    m_encodeParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;

    m_encodeParams.mfx.TargetKbps = 0;
    m_encodeParams.mfx.QPI = ele_param->EncParams.mfx.QPI;
    m_encodeParams.mfx.QPP = ele_param->EncParams.mfx.QPP;
    m_encodeParams.mfx.QPB = ele_param->EncParams.mfx.QPB;

    m_encodeParams.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    //NumRefFrame is used to define the size of iTask_pool, so should be defined by GopRefDist
    m_encodeParams.mfx.NumRefFrame =  (m_encodeParams.mfx.GopRefDist+1) > 4? 4: (m_encodeParams.mfx.GopRefDist+1);
    //preenc exists, so encode should be Encoded Order
    m_encodeParams.mfx.EncodedOrder = 1;
    m_encodeParams.AsyncDepth = 1;

    m_pMvoutStream = ele_param->extParams.mvoutStream;
    m_pMvinStream = ele_param->extParams.mvinStream;
    m_pMBcodeoutStream = ele_param->extParams.mbcodeoutStream;
    m_pWeightStream = ele_param->extParams.weightStream;
#if (MFX_VERSION >= 1025)
    m_nMFEFrames = ele_param->extParams.nMFEFrames;
    m_nMFEMode = ele_param->extParams.nMFEMode;
    m_nMFETimeout = ele_param->extParams.nMFETimeout;
#endif

    return true;
}
mfxStatus H264FEIEncoderEncode::InitEncode(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    //mfxFrameAllocRequest EncRequest;
    mfxVideoParam par;

    m_encodeParams.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    m_encodeParams.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;
    m_encodeParams.mfx.FrameInfo.BitDepthLuma  = 8;
    m_encodeParams.mfx.FrameInfo.BitDepthChroma = 8;

    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        m_encodeParams.mfx.FrameInfo.PicStruct = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        m_encodeParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    m_extParams->fei_ctrl.nRefActiveBL1 = ((m_encodeParams.mfx.FrameInfo.PicStruct) & MFX_PICSTRUCT_PROGRESSIVE) ? 1
                                           : ((m_extParams->fei_ctrl.nRefActiveBL1 >2 || m_extParams->fei_ctrl.nRefActiveBL1 == 0) ? 2 : m_extParams->fei_ctrl.nRefActiveBL1);
    m_encodeParams.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    m_encodeParams.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    m_encodeParams.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    m_encodeParams.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    m_encodeParams.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    m_encodeParams.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
    m_encodeParams.mfx.FrameInfo.Width         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width;
    m_encodeParams.mfx.FrameInfo.Height        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height;

#if (MFX_VERSION >= 1025)
    if (m_nMFEMode != 0 || m_nMFEFrames != 0)
    {
        // configure multiframe mode and number of frames
        // either of mfeMode and numMfeFrames must be set
        // if one of them not set - zero is passed to Media SDK and used internal default value
        m_multiFrame.Header.BufferId = MFX_EXTBUFF_MULTI_FRAME_PARAM;
        m_multiFrame.Header.BufferSz = sizeof(mfxExtMultiFrameParam);
        m_multiFrame.MFMode = m_nMFEMode;
        m_multiFrame.MaxNumFrames  = m_nMFEFrames;
        m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_multiFrame));
        if(m_nMFETimeout)
        {
            //set default timeout per session if passed from commandline
            m_multiFrameControl.Header.BufferId = MFX_EXTBUFF_MULTI_FRAME_CONTROL;
            m_multiFrameControl.Header.BufferSz = sizeof(mfxExtMultiFrameControl);
            m_multiFrameControl.Timeout = m_nMFETimeout;
            m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_multiFrameControl));
        }
    }
    else if (m_nMFETimeout != 0 && m_nMFEMode == 0 && m_nMFETimeout == 0)
    {
        MLOG_WARNING("MFE not enabled, to enable MFE specify mfe_mode and/or mfe_frames\n");
    }
#endif

    //extcoding option2
    if (m_encodeParams.mfx.CodecId == MFX_CODEC_AVC) {
        m_codingOpt2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
        m_codingOpt2.Header.BufferSz = sizeof(m_codingOpt2);
        m_codingOpt2.UseRawRef       = MFX_CODINGOPTION_OFF;

        // B-pyramid reference
        if (m_extParams->bRefType) {
            if (m_encodeParams.mfx.CodecProfile == MFX_PROFILE_AVC_BASELINE
                || m_encodeParams.mfx.GopRefDist == 1) {
                MLOG_WARNING("not have B frame in output stream\n");
            } else {
                m_codingOpt2.BRefType = m_extParams->bRefType;
            }
            MLOG_INFO("BRefType = %u\n", m_codingOpt2.BRefType);
        }

        m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_codingOpt2));
    }

    //extcoding option3
    if (m_encodeParams.mfx.CodecId == MFX_CODEC_AVC) {
        m_codingOpt3.Header.BufferId    = MFX_EXTBUFF_CODING_OPTION3;
        m_codingOpt3.Header.BufferSz    = sizeof(m_codingOpt3);
        m_codingOpt3.NumRefActiveP[0]   = m_extParams->fei_ctrl.nRefActiveP;
        m_codingOpt3.NumRefActiveBL0[0] = m_extParams->fei_ctrl.nRefActiveBL0;
        m_codingOpt3.NumRefActiveBL1[0] = m_extParams->fei_ctrl.nRefActiveBL1;
        m_codingOpt3.WeightedPred = (m_pWeightStream != NULL) ? MFX_WEIGHTED_PRED_EXPLICIT:MFX_WEIGHTED_PRED_UNKNOWN;

        m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_codingOpt3));
    }

    if (m_bUseOpaqueMemory) {
        MLOG_INFO("opaque memory");
        m_encodeParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
        //opaue alloc
        m_extOpaqueAlloc.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        m_extOpaqueAlloc.Header.BufferSz = sizeof(m_extOpaqueAlloc);
        memcpy(&m_extOpaqueAlloc.In, \
               & ((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out, \
               sizeof(m_extOpaqueAlloc.In));
        m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_extOpaqueAlloc));

    } else {
        MLOG_INFO("video memory");
        //memset(&m_extFEIParam, 0, sizeof(m_extFEIParam));
        m_extFEIParam.Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
        m_extFEIParam.Header.BufferSz = sizeof (mfxExtFeiParam);
        m_extFEIParam.Func = MFX_FEI_FUNCTION_ENCODE;
        m_extFEIParam.SingleFieldProcessing = MFX_CODINGOPTION_OFF;
        m_EncodeExtParams.push_back((mfxExtBuffer *)&m_extFEIParam);
        m_encodeParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    /* Create extension buffer to init deblocking parameters */
    MLOG_INFO(" feiEncCtrl m_extParams->fei_ctrl.DisableDeblockingIdc: %d, m_extParams->fei_ctrl.SliceAlphaC0OffsetDiv2: %d, m_extParams->fei_ctrl.SliceBetaOffsetDiv2: %d\n",
            m_extParams->fei_ctrl.nDisableDeblockingIdc,
            m_extParams->fei_ctrl.nSliceAlphaC0OffsetDiv2,
            m_extParams->fei_ctrl.nSliceBetaOffsetDiv2
        );
    if (m_extParams->fei_ctrl.nDisableDeblockingIdc
             || m_extParams->fei_ctrl.nSliceAlphaC0OffsetDiv2
             || m_extParams->fei_ctrl.nSliceBetaOffsetDiv2)
    {
        mfxU16 numFields = (m_encodeParams.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;

        mfxExtFeiSliceHeader* pSliceHeader = new mfxExtFeiSliceHeader[numFields];
        MSDK_ZERO_ARRAY(pSliceHeader, numFields);

        for (mfxU16 fieldId = 0; fieldId < numFields; ++fieldId)
        {
            pSliceHeader[fieldId].Header.BufferId = MFX_EXTBUFF_FEI_SLICE;
            pSliceHeader[fieldId].Header.BufferSz = sizeof(mfxExtFeiSliceHeader);

            pSliceHeader[fieldId].NumSlice = m_encodeParams.mfx.NumSlice;

            pSliceHeader[fieldId].Slice = new mfxExtFeiSliceHeader::mfxSlice[pSliceHeader[fieldId].NumSlice]();

            for (mfxU16 sliceNum = 0; sliceNum < pSliceHeader[fieldId].NumSlice; ++sliceNum)
            {
                pSliceHeader[fieldId].Slice[sliceNum].DisableDeblockingFilterIdc = m_extParams->fei_ctrl.nDisableDeblockingIdc;
                pSliceHeader[fieldId].Slice[sliceNum].SliceAlphaC0OffsetDiv2     = m_extParams->fei_ctrl.nSliceAlphaC0OffsetDiv2;
                pSliceHeader[fieldId].Slice[sliceNum].SliceBetaOffsetDiv2        = m_extParams->fei_ctrl.nSliceBetaOffsetDiv2;
            }
            m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&pSliceHeader[fieldId]));
        }
    }

    if (!m_extParams->bPreEnc && m_pMvinStream != NULL) //not load if we couple with PREENC
    {

        /* Alloc temporal buffers */
        if (m_extParams->fei_ctrl.bRepackPreencMV)
        {
            mfxU32 n_MB = m_pipelineCfg->numMB_frame;
            m_tmpForReading.resize(n_MB);
        }
    }

    //set mfx video ExtParam
    if (!m_EncodeExtParams.empty()) {
        // vector is stored linearly in memory
        m_encodeParams.ExtParam = &m_EncodeExtParams[0];
        m_encodeParams.NumExtParam = (mfxU16)m_EncodeExtParams.size();
        MLOG_INFO("init encoder, ext param number is %d\n", m_encodeParams.NumExtParam);
    }

    sts = m_encode->Init(&m_encodeParams);
    if (sts > MFX_ERR_NONE) {
        MLOG_WARNING("\t\tEncode init warning %d\n", sts);
    } else if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\tEncode init return %d\n", sts);
    }
    assert(sts >= 0);

    memset(&par, 0, sizeof(par));
    sts = m_encode->GetVideoParam(&par);

    // Prepare Media SDK bit stream buffer for encoder
    memset(&output_bs_, 0, sizeof(output_bs_));
    if (par.mfx.FrameInfo.Height * par.mfx.FrameInfo.Width * 4 < par.mfx.BufferSizeInKB * 1024) {
        output_bs_.MaxLength = par.mfx.BufferSizeInKB * 1024;
    }else{
        output_bs_.MaxLength = par.mfx.FrameInfo.Height * par.mfx.FrameInfo.Width * 4;
    }
    output_bs_.Data = new mfxU8[output_bs_.MaxLength];
    MLOG_INFO("output bitstream buffer size %d\n", output_bs_.MaxLength);

    return sts;
}

mfxStatus H264FEIEncoderEncode::QueryIOSurf(mfxFrameAllocRequest* request)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_encode->QueryIOSurf(&m_encodeParams, request);
    MLOG_INFO("Encode suggest surfaces %d\n", request->NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    return sts;
}

void H264FEIEncoderEncode::GetRefInfo(
    mfxU16 & picStruct,
    mfxU16 & refDist,
    mfxU16 & numRefFrame,
    mfxU16 & gopSize,
    mfxU16 & gopOptFlag,
    mfxU16 & idrInterval,
    mfxU16 & numRefActiveP,
    mfxU16 & numRefActiveBL0,
    mfxU16 & numRefActiveBL1,
    mfxU16 & bRefType,
    bool   & bSigleFieldProcessing)
{
#if 0
    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
            case MFX_EXTBUFF_CODING_OPTION2:
            {
                mfxExtCodingOption2* ptr = reinterpret_cast<mfxExtCodingOption2*>(m_InitExtParams[i]);
                bRefType = ptr->BRefType;
            }
            break;

            case MFX_EXTBUFF_CODING_OPTION3:
            {
                mfxExtCodingOption3* ptr = reinterpret_cast<mfxExtCodingOption3*>(m_InitExtParams[i]);
                numRefActiveP   = ptr->NumRefActiveP[0];
                numRefActiveBL0 = ptr->NumRefActiveBL0[0];
                numRefActiveBL1 = ptr->NumRefActiveBL1[0];
            }
            break;

            case MFX_EXTBUFF_FEI_PARAM:
            {
                mfxExtFeiParam* ptr = reinterpret_cast<mfxExtFeiParam*>(m_InitExtParams[i]);
                m_bSingleFieldMode = bSigleFieldProcessing = ptr->SingleFieldProcessing == MFX_CODINGOPTION_ON;
            }
            break;

            default:
                break;
        }
    }
#endif
    picStruct   = m_encodeParams.mfx.FrameInfo.PicStruct;
    refDist     = m_encodeParams.mfx.GopRefDist;
    numRefFrame = m_encodeParams.mfx.NumRefFrame;
    gopSize     = m_encodeParams.mfx.GopPicSize;
    gopOptFlag  = m_encodeParams.mfx.GopOptFlag;
    idrInterval = m_encodeParams.mfx.IdrInterval;
}

mfxStatus H264FEIEncoderEncode::InitEncodeFrameParams(mfxFrameSurface1 * encodeSurface,PairU8 frameType,iTask * eTask)
{
    mfxStatus sts = MFX_ERR_NONE;
    bufSet * freeSet = m_encodeBufs->GetFreeSet();
    MSDK_CHECK_POINTER(freeSet, MFX_ERR_NULL_PTR);

    eTask->bufs = freeSet;
    mfxU8 ffid = !(encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF);
    m_encodeCtrl->FrameType = eTask->m_fieldPicFlag ? createType(*eTask) : ExtractFrameType(*eTask);

    /* Alloc temporal buffers */
    if (m_extParams->fei_ctrl.bRepackPreencMV && !m_tmpForReading.size())
    {
        mfxU32 n_MB = m_pipelineCfg->numMB_frame;
        m_tmpForReading.resize(n_MB);
    }

    mfxU32 feiEncCtrlId = ffid, pMvPredId = ffid, fieldId = 0, pWeightsId = ffid;
    for (std::vector<mfxExtBuffer*>::iterator it = freeSet->PB_bufs.in.buffers.begin();
        it != freeSet->PB_bufs.in.buffers.end(); ++it)
    {
        switch ((*it)->BufferId)
        {
        case MFX_EXTBUFF_FEI_ENC_CTRL:
        {
            mfxExtFeiEncFrameCtrl* feiEncCtrl = reinterpret_cast<mfxExtFeiEncFrameCtrl*>(*it);

            // adjust ref window size if search window is 0
            if (feiEncCtrl->SearchWindow == 0)
            {
                bool adjust_window = (frameType[feiEncCtrlId] & MFX_FRAMETYPE_B) && m_extParams->fei_ctrl.nRefHeight * m_extParams->fei_ctrl.nRefWidth > 1024;

                feiEncCtrl->RefHeight = adjust_window ? 32 : m_extParams->fei_ctrl.nRefHeight;
                feiEncCtrl->RefWidth  = adjust_window ? 32 : m_extParams->fei_ctrl.nRefWidth;
            }

            feiEncCtrl->MVPredictor = (!(frameType[feiEncCtrlId] & MFX_FRAMETYPE_I)) ? 1 : 0;

            /* Set number to actual ref number for each field.
            Driver requires these fields to be zero in case of feiEncCtrl->MVPredictor == false
            but MSDK lib will adjust them to zero if application doesn't
            */
            feiEncCtrl->NumMVPredictors[0] = feiEncCtrl->MVPredictor * m_pipelineCfg->numOfPredictors[fieldId][0];
            feiEncCtrl->NumMVPredictors[1] = feiEncCtrl->MVPredictor * m_pipelineCfg->numOfPredictors[fieldId][1];
            fieldId++;

            feiEncCtrlId = 1 - feiEncCtrlId; // set to sfid
        }
        break;

        case MFX_EXTBUFF_FEI_ENC_MV_PRED:
            if (!m_extParams->bPreEnc && m_pMvinStream)
            {

                mfxExtFeiEncMVPredictors* pMvPredBuf = reinterpret_cast<mfxExtFeiEncMVPredictors*>(*it);

                if (!(eTask->m_type[pMvPredId] & MFX_FRAMETYPE_I))
                {
                    if (m_extParams->fei_ctrl.bRepackPreencMV)
                    {
                        std::vector<mfxI16> tmpForMedian;
                        tmpForMedian.resize(16);

                        if(m_pMvinStream->ReadBlock(&m_tmpForReading[0], sizeof(m_tmpForReading[0])*pMvPredBuf->NumMBAlloc ) != sizeof(m_tmpForReading[0])*pMvPredBuf->NumMBAlloc){
                             MLOG_ERROR("m_pMvinStream: ReadBlock failed.\n");
                             return MFX_ERR_MORE_DATA;
                        }
                        repackPreenc2Enc(&m_tmpForReading[0], pMvPredBuf->MB, pMvPredBuf->NumMBAlloc, &tmpForMedian[0]);
                    }
                    else {
                        if(m_pMvinStream->ReadBlock(pMvPredBuf->MB, sizeof(pMvPredBuf->MB[0])*pMvPredBuf->NumMBAlloc ) != sizeof(pMvPredBuf->MB[0])*pMvPredBuf->NumMBAlloc){
                             MLOG_ERROR("m_pMvinStream: ReadBlock failed.\n");
                             return MFX_ERR_MORE_DATA;
                        }
                    }
                }
                else{
                    int shft = sizeof(mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB);
                    if(!!( m_pMvinStream->SetStreamAttribute(FILE_CUR, shft*pMvPredBuf->NumMBAlloc))){
                         MLOG_ERROR("m_pMvinStream: SetStreamAttribute failed.\n");
                         return MFX_ERR_MORE_DATA;
                    }
                }
            }
            pMvPredId = 1 - pMvPredId; // set to sfid
        break;
        case MFX_EXTBUFF_PRED_WEIGHT_TABLE:
            if (m_pWeightStream)
            {
                mfxExtPredWeightTable* feiWeightTable = reinterpret_cast<mfxExtPredWeightTable*>(*it);

                if ((eTask->m_type[pWeightsId] & MFX_FRAMETYPE_P) || (eTask->m_type[pWeightsId] & MFX_FRAMETYPE_B))
                {
                    if ((m_pWeightStream->ReadBlock(&(feiWeightTable->LumaLog2WeightDenom),   sizeof(mfxU16)) != sizeof(mfxU16))
                               || (m_pWeightStream->ReadBlock(&(feiWeightTable->ChromaLog2WeightDenom),   sizeof(mfxU16)) != sizeof(mfxU16))
                               || (m_pWeightStream->ReadBlock(feiWeightTable->LumaWeightFlag,   sizeof(mfxU16) * 2 * 32) != sizeof(mfxU16) * 2 * 32) //[list][list entry]
                               || (m_pWeightStream->ReadBlock(feiWeightTable->ChromaWeightFlag,   sizeof(mfxU16) * 2 * 32) != sizeof(mfxU16) * 2 * 32) //[list][list entry]
                               || (m_pWeightStream->ReadBlock(feiWeightTable->Weights,   sizeof(mfxU16) * 2 * 32 * 3 * 2) != sizeof(mfxU16) * 2 * 32 * 3 * 2) //[list][list entry][Y, Cb, Cr][weight, offset]
                               ) {
                        MLOG_ERROR("m_pWeightStream: ReadBlock failed");
                        return MFX_ERR_MORE_DATA;
                    }
                }
                else{
                    unsigned int seek_size = sizeof(mfxU16) + sizeof(mfxU16) + sizeof(mfxU16) * 2 * 32
                                             + sizeof(mfxU16) * 2 * 32 + sizeof(mfxI16) * 2 * 32 * 3 * 2;

                    if (m_pWeightStream->SetStreamAttribute(FILE_CUR, seek_size) < 0) {
                        MLOG_ERROR("m_pWeightStream: SeekBlock failed");
                        return MFX_ERR_MORE_DATA;
                    }
                }
            }
            pWeightsId = 1 - pWeightsId; // set to sfid
            break;
        }
    }

    // Add input buffers
    bool is_I_frame = (encodeSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) && (frameType[ffid] & MFX_FRAMETYPE_I);

    m_encodeCtrl->NumExtParam = is_I_frame ? freeSet->I_bufs.in.NumExtParam() : freeSet->PB_bufs.in.NumExtParam();
    m_encodeCtrl->ExtParam = is_I_frame ? freeSet->I_bufs.in.ExtParam() : freeSet->PB_bufs.in.ExtParam();

    return sts;
}

mfxStatus H264FEIEncoderEncode::ProcessChainEncode(iTask* eTask, bool is_eos)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = eTask->ENC_in.InSurface ? eTask->ENC_in.InSurface : eTask->PREENC_in.InSurface;
    mfxSyncPoint syncpE;

    // to check if there is res change in incoming surface
    if (pmfxInSurface) {
        if (m_encodeParams.mfx.FrameInfo.Width  != pmfxInSurface->Info.Width ||
            m_encodeParams.mfx.FrameInfo.Height != pmfxInSurface->Info.Height) {
            m_encodeParams.mfx.FrameInfo.Width   = pmfxInSurface->Info.Width;
            m_encodeParams.mfx.FrameInfo.Height  = pmfxInSurface->Info.Height;
            m_encodeParams.mfx.FrameInfo.CropW   = pmfxInSurface->Info.CropW;
            m_encodeParams.mfx.FrameInfo.CropH   = pmfxInSurface->Info.CropH;

            reset_res_flag_ = true;
            MLOG_INFO(" to reset encoder res as %d x %d\n",
                   m_encodeParams.mfx.FrameInfo.CropW, m_encodeParams.mfx.FrameInfo.CropH);
        }
    }

    if ( reset_res_flag_){
        //need to complete encoding with current settings
        for (;;) {
            for (;;) {
                sts = m_encode->EncodeFrameAsync(NULL, NULL, &output_bs_, &syncpE);

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

    if (reset_res_flag_) {
        mfxVideoParam par;
        memset(&par, 0, sizeof(par));
        m_encode->GetVideoParam(&par);
        par.mfx.FrameInfo.CropW = m_encodeParams.mfx.FrameInfo.CropW;
        par.mfx.FrameInfo.CropH = m_encodeParams.mfx.FrameInfo.CropH;
        par.mfx.FrameInfo.Width = m_encodeParams.mfx.FrameInfo.Width;
        par.mfx.FrameInfo.Height = m_encodeParams.mfx.FrameInfo.Height;

        m_encode->Reset(&par);
        reset_res_flag_ = false;
    }

    PairU8 frameType = eTask->m_type;
    if (pmfxInSurface) {
        sts = InitEncodeFrameParams(pmfxInSurface, frameType, eTask);
        if (sts < MFX_ERR_NONE) {MLOG_ERROR("\t\tFEI ENCODE: InitFrameParams failed with %d\n", sts); return sts;}
    }

    do {
        while (1) {

            if (pmfxInSurface){
                pmfxInSurface->Data.DataFlag = 1;
                pmfxInSurface->Info.BitDepthLuma = 8;
                pmfxInSurface->Info.BitDepthChroma = 8;
            }

            sts = m_encode->EncodeFrameAsync(m_encodeCtrl, pmfxInSurface,
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
                // Allocate more bitstream buffer memory here
                sts = AllocateSufficientBuffer();
                break;
            } else {
                break;
            }
        }

        //Klockwork issue #531: if is_running_ == false in this do-while loop, syncpE is uninitialized.
        if (MFX_ERR_NONE == sts) {
            sts = m_session->SyncOperation(syncpE, 60000);
            if (m_outputStream) {
                sts = WriteBitStreamFrame(&output_bs_, m_outputStream);
            }

            sts = FlushOutput(eTask);
        }
    } while (pmfxInSurface == NULL && sts >= MFX_ERR_NONE);

    if (is_eos && sts == MFX_ERR_MORE_DATA) {
        return MFX_ERR_NONE;
    }

    //MLOG_INFO("leave ProcessChainEncode\n");
    return sts;
}

mfxStatus H264FEIEncoderEncode::FlushOutput(iTask* eTask)
{
    mfxExtBuffer** output_buffers   = NULL;
    mfxU32         n_output_buffers = 0;
    mfxExtFeiEncMV::mfxExtFeiEncMVMB tmpMBencMV;
    /* Default values for I-frames */
    memset(&tmpMBencMV, 0x8000, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB));

    if (eTask)
    {
        MSDK_CHECK_POINTER(eTask->bufs, MFX_ERR_NULL_PTR);
        output_buffers   = eTask->bufs->PB_bufs.out.buffers.data();
        n_output_buffers = eTask->bufs->PB_bufs.out.buffers.size();
    }
    else
    {
        // In case of draining frames from encoder in display-order mode
        output_buffers   = output_bs_.ExtParam;
        n_output_buffers = output_bs_.NumExtParam;
    }

    mfxU32 mvBufCounter = 0;

    for (mfxU32 i = 0; i < n_output_buffers; ++i)
    {
        MSDK_CHECK_POINTER(output_buffers[i], MFX_ERR_NULL_PTR);

        switch (output_buffers[i]->BufferId)
        {
        case MFX_EXTBUFF_FEI_ENC_MV:

            if (m_pMvoutStream)
            {
                mfxExtFeiEncMV* mvBuf = reinterpret_cast<mfxExtFeiEncMV*>(output_buffers[i]);
                if (!(extractType(output_bs_.FrameType, mvBufCounter) & MFX_FRAMETYPE_I)){
                    if(m_pMvoutStream->WriteBlock(mvBuf->MB, sizeof(mvBuf->MB[0])*mvBuf->NumMBAlloc, false ) != sizeof(mvBuf->MB[0])*mvBuf->NumMBAlloc){
                        MLOG_ERROR("\t\t WriteBlock return Error \n");
                        return MFX_ERR_MORE_DATA;
                    }
                }
                else
                {
                    for (mfxU32 k = 0; k < mvBuf->NumMBAlloc; k++){
                        if(m_pMvoutStream->WriteBlock(&tmpMBencMV, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB), false ) != sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB)){
                            MLOG_ERROR("\t\t WriteBlock return Error \n");
                            return MFX_ERR_MORE_DATA;
                        }
                    }
                }
                ++mvBufCounter;
            }
            break;
        case MFX_EXTBUFF_FEI_PAK_CTRL:
            if (m_pMBcodeoutStream)
            {
                mfxExtFeiPakMBCtrl* mbcodeBuf = reinterpret_cast<mfxExtFeiPakMBCtrl*>(output_buffers[i]);
                if(m_pMBcodeoutStream->WriteBlock(mbcodeBuf->MB, sizeof(mbcodeBuf->MB[0])*mbcodeBuf->NumMBAlloc, false ) != sizeof(mbcodeBuf->MB[0])*mbcodeBuf->NumMBAlloc){
                    MLOG_ERROR("\t\t WriteBlock return Error \n");
                    return MFX_ERR_MORE_DATA;
                }
            }
            break;
        } // switch (output_buffers[i]->BufferId)
    } // for (mfxU32 i = 0; i < n_output_buffers; ++i)

    return MFX_ERR_NONE;
}

mfxStatus H264FEIEncoderEncode::AllocateSufficientBuffer()
{
    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);

    // find out the required buffer size
    mfxStatus sts = m_encode->GetVideoParam(&par);
   if (sts < MFX_ERR_NONE) {MLOG_ERROR("AllocateSufficientBuffer failed with %d\n", sts); return sts;}

    // reallocate bigger buffer for output
    sts = ExtendMfxBitstream(&output_bs_, par.mfx.BufferSizeInKB * 1000);
    if (sts < MFX_ERR_NONE) {MLOG_ERROR("ExtendMfxBitstream failed with %d\n", sts); WipeMfxBitstream(&output_bs_); return sts;}

    return sts;
}

mfxStatus H264FEIEncoderEncode::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_encode->GetVideoParam(par);
    return sts;
}

mfxStatus H264FEIEncoderEncode::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
        Stream *out_stream)
{
#if 1 //#if defined SUPPORT_SMTA
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
#endif //MSDK_AVC_FEI
