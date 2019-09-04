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

#include "h265_fei_encoder_encode.h"
#ifdef MSDK_HEVC_FEI

#include "h265_fei_buffer_allocator.h"

DEFINE_MLOGINSTANCE_CLASS(H265FEIEncoderEncode, "H265FEIEncoderEncode");

H265FEIEncoderEncode::H265FEIEncoderEncode(MFXVideoSession *session,
                                   bool UseOpaqueMemory,
                                   const mfxExtFeiHevcEncFrameCtrl &encodeCtrl,
                                   EncExtParams* extParams)
    :m_session(session),
     m_frameCtrl(encodeCtrl),
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

    m_outputStream = NULL;
    m_pMvoutStream = NULL,
    m_pMvinStream = NULL,
    m_pMBcodeoutStream = NULL;
    m_buf_allocator = NULL;

    m_encode = NULL;
    m_repacker = NULL;
    m_numOfFields = 0;
}
H265FEIEncoderEncode::~H265FEIEncoderEncode()
{
    for (mfxU16 i = 0; i < m_numOfFields; i++) {
        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>(i);
        if (pMVP)
        {
            m_buf_allocator->Free(pMVP);
        }

        mfxExtFeiHevcEncQP* pQP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncQP>(i);
        if (pQP)
        {
            m_buf_allocator->Free(pQP);
        }
    }

    if (m_encode) {
        m_encode->Close();
        delete m_encode;
        m_encode = NULL;
    }

    if (NULL != output_bs_.Data) {
        delete[] output_bs_.Data;
        output_bs_.Data = NULL;
    }
}

bool H265FEIEncoderEncode::Init(ElementCfg *ele_param)
{
    m_outputStream = ele_param->output_stream;
    m_encode = new MFXVideoENCODE(*m_session);
    /* Encoding Options */
    m_encodeParams.mfx.LowPower = 32;
    m_encodeParams.mfx.BRCParamMultiplier = 1;
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
    if (ele_param->EncParams.mfx.NumRefFrame)
        m_encodeParams.mfx.NumRefFrame = ele_param->EncParams.mfx.NumRefFrame;
    else
        m_encodeParams.mfx.NumRefFrame =  (m_encodeParams.mfx.GopRefDist+1) > 4? 4: (m_encodeParams.mfx.GopRefDist+1);
    m_encodeParams.AsyncDepth = 1;

    m_pMvoutStream = ele_param->extParams.mvoutStream;
    m_pMvinStream = ele_param->extParams.mvinStream;
    m_pMBcodeoutStream = ele_param->extParams.mbcodeoutStream;

    return true;
}

mfxStatus H265FEIEncoderEncode::ResetExtBuffers(const mfxVideoParam& videoParams)
{
    BufferAllocRequest request;
    MSDK_ZERO_MEMORY(request);

    request.Width = videoParams.mfx.FrameInfo.CropW;
    request.Height = videoParams.mfx.FrameInfo.CropH;
    for (mfxU16 i = 0; i < m_numOfFields; i++) {

        mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>(i);
        if (pMVP)
        {
            m_buf_allocator->Free(pMVP);

            m_buf_allocator->Alloc(pMVP, request);
        }

        mfxExtFeiHevcEncQP* pQP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncQP>(i);
        if (pQP)
        {
            m_buf_allocator->Free(pQP);

            m_buf_allocator->Alloc(pQP, request);
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus H265FEIEncoderEncode::InitEncode(MediaBuf &buf, PredictorsRepacking* repacker, bool Order)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxVideoParam par;

    m_repacker = repacker;
    m_encodeParams.mfx.EncodedOrder = Order ? 1:0;
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

    //extcoding option
    m_codingOpt.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    m_codingOpt.Header.BufferSz = sizeof(m_codingOpt);
    m_codingOpt.PicTimingSEI = MFX_CODINGOPTION_OFF;
    m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_codingOpt));

    //extcoding option2
    m_codingOpt2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    m_codingOpt2.Header.BufferSz = sizeof(m_codingOpt2);
    //m_codingOpt2.UseRawRef         = MFX_CODINGOPTION_OFF;

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


    //extcoding option3
    m_codingOpt3.Header.BufferId    = MFX_EXTBUFF_CODING_OPTION3;
    m_codingOpt3.Header.BufferSz    = sizeof(m_codingOpt3);

    std::fill(m_codingOpt3.NumRefActiveP,   m_codingOpt3.NumRefActiveP + 8,   m_extParams->fei_ctrl.nRefActiveP);
    std::fill(m_codingOpt3.NumRefActiveBL0, m_codingOpt3.NumRefActiveBL0 + 8, m_extParams->fei_ctrl.nRefActiveBL0);
    std::fill(m_codingOpt3.NumRefActiveBL1, m_codingOpt3.NumRefActiveBL1 + 8, m_extParams->fei_ctrl.nRefActiveBL1);

    m_codingOpt3.PRefType = m_extParams->pRefType;
    m_codingOpt3.GPB = m_extParams->GPB;
    m_EncodeExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_codingOpt3));

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
        m_encodeParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

    //set mfx video ExtParam
    if (!m_EncodeExtParams.empty()) {
        // vector is stored linearly in memory
        m_encodeParams.ExtParam = &m_EncodeExtParams[0];
        m_encodeParams.NumExtParam = (mfxU16)m_EncodeExtParams.size();
        MLOG_INFO("init encoder, ext param number is %d\n", m_encodeParams.NumExtParam);
    }

    m_encodeParams.mfx.FrameInfo.AspectRatioH = 1;
    m_encodeParams.mfx.FrameInfo.AspectRatioW = 1;
    m_encodeParams.mfx.IdrInterval = 0xffff;

    m_numOfFields = m_encodeParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? 1 : 2;
    m_encodeCtrl.SetFields(m_numOfFields);

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

    // add FEI frame ctrl with default values
    for (mfxU16 i = 0; i < m_numOfFields; i++) {
        mfxExtFeiHevcEncFrameCtrl* ctrl = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcEncFrameCtrl>(i);
        MSDK_CHECK_POINTER(ctrl, MFX_ERR_NOT_INITIALIZED);
        *ctrl = m_frameCtrl;

        mfxExtHEVCRefLists* pRefLists = m_encodeCtrl.AddExtBuffer<mfxExtHEVCRefLists>(i);
        MSDK_CHECK_POINTER(pRefLists, MFX_ERR_NOT_INITIALIZED);

        // allocate ext buffer for input MV predictors required for Encode.
        if (m_repacker|| m_pMvinStream)
        {
            mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.AddExtBuffer<mfxExtFeiHevcEncMVPredictors>(i);
            MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);
            pMVP->VaBufferID = VA_INVALID_ID;
        }
    }
    mfxHDL hdl;
    sts = m_session->GetHandle((mfxHandleType)MFX_HANDLE_VA_DISPLAY, &hdl);
    if (sts != MFX_ERR_NONE) {
        MLOG_ERROR("HEVC FEI Encode get display handle is failed. sts = %d", sts);
        return sts;
    }

    m_buf_allocator = new H265FeiBufferAllocator(hdl, m_encodeParams.mfx.FrameInfo.Width, m_encodeParams.mfx.FrameInfo.Height);

    // TODO: add condition when buffer is required

    sts = ResetExtBuffers(m_encodeParams);
    if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("FEI Encode ResetExtBuffers failed");
    }

    return sts;
}

mfxStatus H265FEIEncoderEncode::QueryIOSurf(mfxFrameAllocRequest* request)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_encode->QueryIOSurf(&m_encodeParams, request);
    MLOG_INFO("Encode suggest surfaces %d\n", request->NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    return sts;
}

void H265FEIEncoderEncode::GetRefInfo(
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
    picStruct   = m_encodeParams.mfx.FrameInfo.PicStruct;
    refDist     = m_encodeParams.mfx.GopRefDist;
    numRefFrame = m_encodeParams.mfx.NumRefFrame;
    gopSize     = m_encodeParams.mfx.GopPicSize;
    gopOptFlag  = m_encodeParams.mfx.GopOptFlag;
    idrInterval = m_encodeParams.mfx.IdrInterval;
}

mfxStatus H265FEIEncoderEncode::ProcessChainEncode(HevcTask* task, bool is_eos)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1* pSurf = NULL;

    if (task)
    {
        sts = SetCtrlParams(*task);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("FEI Encode::SetCtrlParams failed sts = %d", sts);
            return sts;
        }
        pSurf = task->m_surf;
    }
    ProcessChainEncode(pSurf, is_eos);
}

mfxStatus H265FEIEncoderEncode::ProcessChainEncode(mfxFrameSurface1 * surf, bool is_eos)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1* pSurf = surf;
    mfxSyncPoint syncpE;

    for (;;)
    {
        sts = m_encode->EncodeFrameAsync(&m_encodeCtrl, pSurf, &output_bs_, &syncpE);

        if (MFX_ERR_NONE < sts && !syncpE) {
            if (MFX_WRN_DEVICE_BUSY == sts) {
                usleep(20);
                continue;
            }
        } else if (MFX_ERR_NONE < sts && syncpE) {
            // ignore warnings if output is available
            sts = MFX_ERR_NONE;
            //break;
        } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
            // Allocate more bitstream buffer memory here
            sts = AllocateSufficientBuffer();
            //break;
        }else if (sts == MFX_ERR_MORE_DATA){
            break;
        }

        if (MFX_ERR_NONE <= sts && syncpE) // ignore warnings if output is available
        {
            sts = m_session->SyncOperation(syncpE, MSDK_WAIT_INTERVAL);
            if (sts < MFX_ERR_NONE) {
                MLOG_ERROR("FEI Encode: SyncOperation failed");
                return sts;
            }
            if (m_outputStream) {
                sts = WriteBitStreamFrame(&output_bs_, m_outputStream);
            }
            break;
        }

    } // for (;;)

    // when pSurf==NULL, MFX_ERR_MORE_DATA indicates all cached frames within encoder were drained, return as is
    // otherwise encoder need more input surfaces, ignore status
    if (MFX_ERR_MORE_DATA == sts && pSurf)
    {
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    }

    return sts;
}

void FillHEVCRefLists(const HevcTask& task, mfxExtHEVCRefLists & refLists)
{
    const mfxU8 (&RPL)[2][MAX_DPB_SIZE] = task.m_refPicList;
    const HevcDpbArray & DPB = task.m_dpb[TASK_DPB_ACTIVE];

    refLists.NumRefIdxL0Active = task.m_numRefActive[0];
    refLists.NumRefIdxL1Active = task.m_numRefActive[1];

    for (mfxU32 direction = 0; direction < 2; ++direction)
    {
        mfxExtHEVCRefLists::mfxRefPic (&refPicList)[32] = (direction == 0) ? refLists.RefPicList0 : refLists.RefPicList1;
        for (mfxU32 i = 0; i < MAX_DPB_SIZE; ++i)
        {
            const mfxU8 & idx = RPL[direction][i];
            if (idx < MAX_DPB_SIZE)
            {
                refPicList[i].FrameOrder = DPB[idx].m_fo;
                refPicList[i].PicStruct  = MFX_PICSTRUCT_UNKNOWN;
            }
        }
    }
}


mfxStatus H265FEIEncoderEncode::SetCtrlParams(const HevcTask& task)
{
    mfxStatus sts = MFX_ERR_NONE;
    m_encodeCtrl.FrameType = task.m_frameType;

    for (mfxU16 i = 0; i < m_numOfFields; i++) {

        mfxExtHEVCRefLists* pRefLists = m_encodeCtrl.GetExtBuffer<mfxExtHEVCRefLists>(i);
        MSDK_CHECK_POINTER(pRefLists, MFX_ERR_NOT_INITIALIZED);

        FillHEVCRefLists(task, *pRefLists);

        // Get Frame Control buffer
        mfxExtFeiHevcEncFrameCtrl* ctrl = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncFrameCtrl>(i);
        MSDK_CHECK_POINTER(ctrl, MFX_ERR_NOT_INITIALIZED);

        if (m_repacker || m_pMvinStream)
        {
            // 0 - no predictors (for I-frames),
            // 1 - enabled per 16x16 block,
            // 2 - enabled per 32x32 block (default for repacker),
            // 7 - inherit size of block from buffers CTU setting (default for external file with predictors)
            ctrl->MVPredictor = (m_encodeCtrl.FrameType & MFX_FRAMETYPE_I) ? 0 : m_frameCtrl.MVPredictor;

            mfxExtFeiHevcEncMVPredictors* pMVP = m_encodeCtrl.GetExtBuffer<mfxExtFeiHevcEncMVPredictors>(i);
            MSDK_CHECK_POINTER(pMVP, MFX_ERR_NOT_INITIALIZED);
            H265FeiBufferAllocator allocator = *m_buf_allocator;
            AutoBufferLocker<mfxExtFeiHevcEncMVPredictors> lock(allocator, *pMVP);

            ctrl->NumMvPredictors[0] = 0;
            ctrl->NumMvPredictors[1] = 0;

            if (m_repacker)
            {
                ctrl->NumFramePartitions = 1;
                sts = m_repacker->RepackPredictors(task, *pMVP, ctrl->NumMvPredictors);
                if (sts < MFX_ERR_NONE) {
                    MLOG_ERROR("FEI Encode::RepackPredictors failed. sts = %d", sts);
                    return sts;
                }
            }
            else
            {
                if (m_pMvinStream->ReadBlock(pMVP->Data, pMVP->Pitch * pMVP->Height * sizeof(pMVP->Data[0]))) {
                    MLOG_ERROR("FEI Encode. Read MV predictors failed. sts = %d", sts);
                    return MFX_ERR_MORE_DATA;
                }

                if (m_encodeCtrl.FrameType & MFX_FRAMETYPE_P)
                {
                    ctrl->NumMvPredictors[0] = m_frameCtrl.NumMvPredictors[0];
                }
                else if (m_encodeCtrl.FrameType & MFX_FRAMETYPE_B)
                {
                    ctrl->NumMvPredictors[0] = m_frameCtrl.NumMvPredictors[0];
                    ctrl->NumMvPredictors[1] = m_frameCtrl.NumMvPredictors[1];
                }
            }
        }
    }

    // TODO: add condition when buffer is required

    return MFX_ERR_NONE;
}

mfxStatus H265FEIEncoderEncode::AllocateSufficientBuffer()
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

mfxStatus H265FEIEncoderEncode::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_encode->GetVideoParam(par);
    return sts;
}

mfxStatus H265FEIEncoderEncode::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
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
#endif //MSDK_HEVC_FEI
