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

#include "h265_fei_encoder_preenc.h"
#ifdef MSDK_HEVC_FEI
#include "os/atomic.h"
#include "h265_fei_utils.h"

DEFINE_MLOGINSTANCE_CLASS(H265FEIEncoderPreenc, "H265FEIEncoderPreenc");

template<typename T>
T* AcquireResource(std::vector<T> & pool)
{
    T * freeBuffer = NULL;
    for (size_t i = 0; i < pool.size(); i++)
    {
        if (pool[i].m_locked == 0)
        {
            freeBuffer = &pool[i];
            msdk_atomic_inc16((volatile mfxU16*)&pool[i].m_locked);
            break;
        }
    }
    return freeBuffer;
}

H265FEIEncoderPreenc::H265FEIEncoderPreenc(MFXVideoSession *session,
                                   MFXFrameAllocator* allocator,
                                   bool UseOpaqueMemory,
                                   const mfxExtFeiPreEncCtrl& preencCtrl,
                                   EncExtParams* extParams)
    :m_session(session),
     m_frameCtrl(preencCtrl),
     m_DSSurfacePool(allocator),
     m_bMVout(false),
     m_pMvoutStream(NULL),
     m_pMvinStream(NULL),
     m_extParams(extParams),
     m_bUseOpaqueMemory(UseOpaqueMemory)

{
    memset(&m_preencParams, 0, sizeof(mfxVideoParam));
    memset(&m_DSParams, 0, sizeof(mfxVideoParam));
    memset(&m_DSResponse, 0, sizeof(mfxFrameAllocResponse));
    /* Default values for I-frames */
    memset(&m_tmpMVMB, 0x8000, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB));

    m_preenc = NULL;
    m_ds = NULL;
    m_DSstrength = 0;

    /* Default value for I-frames */
    for (size_t i = 0; i < 16; i++)
    {
        for (size_t j = 0; j < 2; j++)
        {
            m_default_MVMB.MV[i][j].x = (mfxI16)0x8000;
            m_default_MVMB.MV[i][j].y = (mfxI16)0x8000;
        }
    }
}

H265FEIEncoderPreenc::~H265FEIEncoderPreenc()
{
    for (size_t i = 0; i < m_mvs.size(); ++i)
    {
        delete[] m_mvs[i].MB;
    }
    for (size_t i = 0; i < m_mbs.size(); ++i)
    {
        delete[] m_mbs[i].MB;
    }
}


mfxStatus H265FEIEncoderPreenc::ResetExtBuffers(const mfxVideoParam& videoParams)
{
    for (size_t i = 0; i < m_mvs.size(); ++i)
    {
        delete[] m_mvs[i].MB;
        m_mvs[i].MB = 0;
        m_mvs[i].NumMBAlloc = 0;
    }
    for (size_t i = 0; i < m_mbs.size(); ++i)
    {
        delete[] m_mbs[i].MB;
        m_mbs[i].MB = 0;
        m_mbs[i].NumMBAlloc = 0;
    }

    MSDK_CHECK_NOT_EQUAL(m_preencParams.AsyncDepth, 1, MFX_ERR_UNSUPPORTED);

    mfxU32 nMB = videoParams.mfx.FrameInfo.Width * videoParams.mfx.FrameInfo.Height >> 8;
    mfxU8 nBuffers = videoParams.mfx.NumRefFrame;

    m_mvs.resize(nBuffers * m_numOfFields);
    m_mbs.resize(nBuffers * m_numOfFields);
    for (size_t i = 0; i < nBuffers * m_numOfFields; ++i)
    {
        MSDK_ZERO_MEMORY(m_mvs[i]);
        m_mvs[i].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MV;
        m_mvs[i].Header.BufferSz = sizeof(mfxExtFeiPreEncMV);
        m_mvs[i].MB = new mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB[nMB];
        m_mvs[i].NumMBAlloc = nMB;

        MSDK_ZERO_MEMORY(m_mbs[i]);
        m_mbs[i].Header.BufferId = MFX_EXTBUFF_FEI_PREENC_MB;
        m_mbs[i].Header.BufferSz = sizeof(mfxExtFeiPreEncMBStat);
        m_mbs[i].MB = new mfxExtFeiPreEncMBStat::mfxExtFeiPreEncMBStatMB[nMB];
        m_mbs[i].NumMBAlloc = nMB;
    }
    return MFX_ERR_NONE;
}

bool H265FEIEncoderPreenc::Init(ElementCfg *ele_param)
{
    if (ele_param->extParams.fei_ctrl.nDSstrength){
        m_ds = new MFXVideoVPP(*m_session);
        m_DSstrength = ele_param->extParams.fei_ctrl.nDSstrength;
    }

    m_preenc = new MFXVideoENC(*m_session);

    /*PREENC video parameters*/
    m_preencParams.mfx.CodecId = MFX_CODEC_AVC;//ele_param->EncParams.mfx.CodecId; // why not?
    m_preencParams.mfx.TargetUsage = 0;
    m_preencParams.mfx.TargetKbps = 0;
    m_preencParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    m_preencParams.mfx.QPI = ele_param->EncParams.mfx.QPI;
    m_preencParams.mfx.QPP = ele_param->EncParams.mfx.QPP;
    m_preencParams.mfx.QPB = ele_param->EncParams.mfx.QPB;

    // binary flag, 0 signals encoder to take frames in display order. PREENC, ENCPAK, ENC, PAK interfaces works only in encoded order
    m_preencParams.mfx.EncodedOrder = 1;
    m_preencParams.AsyncDepth = 1;
    m_preencParams.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist==0? 4: ele_param->EncParams.mfx.GopRefDist;
    m_preencParams.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize==0? 30: ele_param->EncParams.mfx.GopPicSize;
    m_preencParams.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval==0?0xffff:ele_param->EncParams.mfx.IdrInterval;
    m_preencParams.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
    m_preencParams.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
    m_preencParams.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    //NumRefFrame is used to define the size of iTask_pool, so should be defined by GopRefDist
    if (ele_param->EncParams.mfx.NumRefFrame)
        m_preencParams.mfx.NumRefFrame = ele_param->EncParams.mfx.NumRefFrame;
    else
        m_preencParams.mfx.NumRefFrame = (m_preencParams.mfx.GopRefDist+1) > 4? 4: (m_preencParams.mfx.GopRefDist+1);
    m_preencParams.mfx.GopOptFlag = ele_param->EncParams.mfx.GopOptFlag;

    return true;
}

mfxStatus H265FEIEncoderPreenc::InitPreenc(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameAllocRequest vpp_request[2];
    Zero(&vpp_request[0], 2);
    if (m_DSstrength){
        sts = InitDS(buf, vpp_request);
        if (MFX_ERR_NONE != sts){
            MLOG_ERROR("down sampling create failed %d\n", sts);
            return sts;
        }
    }

    if (m_DSstrength){
        memcpy(&(m_preencParams.mfx.FrameInfo), &(m_DSParams.vpp.Out), sizeof(mfxFrameInfo));
    } else{
        memcpy(&(m_preencParams.mfx.FrameInfo), &(((mfxFrameSurface1 *)buf.msdk_surface)->Info), sizeof(mfxFrameInfo));
    }

    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        m_preencParams.mfx.FrameInfo.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        m_preencParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    m_preencParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;

    /* Create extended buffer to Init FEI PREENC */
    mfxExtFeiParam pExtBufInit;
    memset(&pExtBufInit, 0, sizeof(pExtBufInit));
    pExtBufInit.Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
    pExtBufInit.Header.BufferSz = sizeof(mfxExtFeiParam);
    pExtBufInit.Func            = MFX_FEI_FUNCTION_PREENC;
    pExtBufInit.SingleFieldProcessing = MFX_CODINGOPTION_OFF;
    m_PreencExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&pExtBufInit));

    mfxExtCodingOption pCO;
    memset(&pCO, 0, sizeof(pCO));
    pCO.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    pCO.Header.BufferSz = sizeof(mfxExtCodingOption);
    pCO.PicTimingSEI = MFX_CODINGOPTION_UNKNOWN;
    m_PreencExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&pCO));

    mfxExtCodingOption2 pCO2;
    memset(&pCO2, 0, sizeof(pCO2));
    pCO2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    pCO2.Header.BufferSz = sizeof(mfxExtCodingOption2);
    pCO2.BRefType = m_extParams->bRefType;// set B frame reference
    m_PreencExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&pCO2));

    mfxExtCodingOption3 pCO3;
    memset(&pCO3, 0, sizeof(pCO3));
    pCO3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
    pCO3.Header.BufferSz = sizeof(mfxExtCodingOption3);
    pCO3.PRefType = m_extParams->pRefType; // set P frame Reference
    pCO3.GPB = MFX_CODINGOPTION_UNKNOWN;//m_extParams->GPB;

    m_extParams->fei_ctrl.nRefActiveBL1     = ((m_preencParams.mfx.FrameInfo.PicStruct) & MFX_PICSTRUCT_PROGRESSIVE) ? 1
                                                : ((m_extParams->fei_ctrl.nRefActiveBL1 >2 || m_extParams->fei_ctrl.nRefActiveBL1 == 0)? 2 : m_extParams->fei_ctrl.nRefActiveBL1);

    std::fill(pCO3.NumRefActiveP,   pCO3.NumRefActiveP + 8,   m_extParams->fei_ctrl.nRefActiveP);
    std::fill(pCO3.NumRefActiveBL0, pCO3.NumRefActiveBL0 + 8, m_extParams->fei_ctrl.nRefActiveBL0);
    std::fill(pCO3.NumRefActiveBL1, pCO3.NumRefActiveBL1 + 8, m_extParams->fei_ctrl.nRefActiveBL1);
    m_PreencExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&pCO3));

    if (MFX_PICSTRUCT_FIELD_SINGLE == m_preencParams.mfx.FrameInfo.PicStruct)
        m_preencParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

    m_numOfFields = m_preencParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? 1: 2;
    m_syncp.second.first.SetFields(m_numOfFields);
    for (mfxU16 i = 0; i < m_numOfFields; i++) {
        mfxExtFeiPreEncCtrl* ctrl = m_syncp.second.first.AddExtBuffer<mfxExtFeiPreEncCtrl>(i);
        if (!ctrl) {
            return MFX_ERR_NULL_PTR;
        }

        *ctrl = m_frameCtrl;
        ctrl->Qp = m_preencParams.mfx.QPI;
    }

    m_bMVout = m_extParams->bEncode;
    if (!!m_extParams->mvoutStream && !(m_extParams->bEncode))
    {
        m_bMVout = true;
        m_pMvoutStream = m_extParams->mvoutStream;
    }
    m_pMvinStream = m_extParams->mvinStream;

    if (!m_PreencExtParams.empty())
    {
        m_preencParams.ExtParam    = &m_PreencExtParams[0]; // vector is stored linearly in memory
        m_preencParams.NumExtParam = (mfxU16)m_PreencExtParams.size();
        MLOG_INFO("init preenc, ext param number is %d\n", m_preencParams.NumExtParam);
    }

    m_preencParams.mfx.LowPower = 32;
    m_preencParams.mfx.BRCParamMultiplier = 1;

    sts = m_preenc->Init(&m_preencParams);
    if (sts > MFX_ERR_NONE) {
        MLOG_WARNING("\t\tPreenc init warning %d\n", sts);
    } else if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\tPreenc init return error %d\n", sts);
    }

    ResetExtBuffers(m_preencParams);
    MfxVideoParamsWrapper pars;
    m_preenc->GetVideoParam(&pars);

    {
        if (m_DSstrength) {
            vpp_request[1].Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET  | MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_FROM_ENC;
            vpp_request[1].NumFrameSuggested = vpp_request[1].NumFrameMin = m_preencParams.mfx.NumRefFrame + 5;
            sts = m_DSSurfacePool.AllocSurfaces(vpp_request[1]);
            if (sts < MFX_ERR_NONE) {
                MLOG("Alloc DS surfaces failed.");
                return sts;
            }
        }

    }
    return sts;
}

mfxStatus H265FEIEncoderPreenc::InitDS(MediaBuf &buf, mfxFrameAllocRequest *request)
{
    mfxStatus sts = MFX_ERR_NONE;

    assert(buf.msdk_surface);
    memcpy(&m_DSParams.vpp.In, &((mfxFrameSurface1 *)buf.msdk_surface)->Info,
            sizeof(m_DSParams.vpp.In));
    m_DSParams.vpp.In.PicStruct = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    m_DSParams.vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_DSParams.vpp.In.PicStruct) ?
            MSDK_ALIGN16(m_DSParams.vpp.In.Height) : MSDK_ALIGN32(m_DSParams.vpp.In.Height);

    memcpy(&m_DSParams.vpp.Out, &m_DSParams.vpp.In, sizeof(m_DSParams.vpp.Out));

    // only resizing is supported
    m_DSParams.vpp.Out.CropX = m_DSParams.vpp.Out.CropY = 0;
    m_DSParams.vpp.Out.CropW = m_DSParams.vpp.Out.Width  = MSDK_ALIGN16(m_DSParams.vpp.In.Width / m_DSstrength);
    m_DSParams.vpp.Out.CropH = m_DSParams.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE & m_DSParams.vpp.Out.PicStruct) ?
            MSDK_ALIGN16(m_DSParams.vpp.In.Height / m_DSstrength) : MSDK_ALIGN32(m_DSParams.vpp.In.Height / m_DSstrength);

    m_DSParams.AsyncDepth = 1;

    if (!m_bUseOpaqueMemory) {
            m_DSParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    } else {
            m_DSParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    }

    memset(request, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = m_ds->QueryIOSurf(&m_DSParams, request);
    MLOG_INFO("DownSampling suggest number of surfaces is in/out %d/%d\n",
        request[0].NumFrameSuggested,request[1].NumFrameSuggested);

    sts = m_ds->Init(&m_DSParams);

    if (MFX_WRN_FILTER_SKIPPED == sts) {
        sts = MFX_ERR_NONE;
    }

    return sts;
}

mfxStatus H265FEIEncoderPreenc::QueryIOSurf(mfxFrameAllocRequest* request)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_preenc->QueryIOSurf(&m_preencParams, request);
    MLOG_INFO("Preenc suggest surfaces %d\n", request->NumFrameSuggested);
    //assert(sts >= MFX_ERR_NONE);
    return sts;
}

mfxStatus H265FEIEncoderPreenc::ProcessChainPreenc(HevcTask* task)
{
    MSDK_CHECK_POINTER(task, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    /* PreENC DownSampling */
    if (m_ds)
    {
        sts = DownSampleFrame(task);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tPreENC DownsampleInput failed with %d\n", sts);
            return sts;
        }
    }

    sts = ProcessMultiPreenc(task);

    if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("PreEnc: ProcessMultiPreenc failed.");
        return sts;
    }
    sts = DumpResult(task);
    if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("PreEnc: DumpResult failed.");
        return sts;
    }

    return sts;
}

mfxStatus H265FEIEncoderPreenc::DownSampleFrame(HevcTask* task)
{
    mfxFrameSurface1* pOutSurf = NULL;
    mfxFrameSurface1* pInSurf = task->m_surf;
    mfxStatus sts = MFX_ERR_NONE;

    for (;;)
    {
        mfxSyncPoint syncp;
        MSDK_ZERO_MEMORY(syncp);

        pOutSurf = m_DSSurfacePool.GetFreeSurface();
        MSDK_CHECK_POINTER(pOutSurf, MFX_ERR_MEMORY_ALLOC);

        sts = m_ds->RunFrameVPPAsync(pInSurf, pOutSurf, NULL, &syncp);
        //MSDK_CHECK_WRN(sts, "VPP RunFrameVPPAsync");

        if (MFX_ERR_NONE < sts && !syncp) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts)
            {
                WaitForDeviceToBecomeFree(*m_session, syncp, sts);
            }
            continue;
        }

        if (MFX_ERR_NONE <= sts && syncp) // ignore warnings if output is available
        {
            sts = m_session->SyncOperation(syncp, MSDK_WAIT_INTERVAL);
            break;
        }

        MSDK_BREAK_ON_ERROR(sts);
    }

    //fprintf(stderr, "downsample handle sts = %d, surface = %p\n", sts, pOutSurf);
    // when pInSurf==NULL, MFX_ERR_MORE_DATA indicates all cached frames within VPP were drained,
    // return status as is
    // otherwise fetch more input for VPP, ignore status
    if (MFX_ERR_MORE_DATA == sts && pInSurf)
    {
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    }

    *task->m_ds_surf = pOutSurf;
    // HACK: lock ds surf here, but unlock in reorder
    msdk_atomic_inc16((volatile mfxU16*)&pOutSurf->Data.Locked);

    return MFX_ERR_NONE;
}

inline mfxU16 H265FEIEncoderPreenc::GetCurPicType(mfxU32 fieldId)
{

    switch (m_preencParams.mfx.FrameInfo.PicStruct & 0xf)
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

mfxStatus H265FEIEncoderPreenc::ProcessMultiPreenc(HevcTask* pTask)
{
    mfxStatus sts = MFX_ERR_NONE;
    HevcTask & task = *pTask;
    mfxU8 const (&RPL)[2][MAX_DPB_SIZE] = task.m_refPicList;

    bool bDownsampleInput = true;
    for (size_t idxL0 = 0, idxL1 = 0;
         idxL0 < task.m_numRefActive[0] || idxL1 < task.m_numRefActive[1] // Iterate thru L0/L1 frames
         || idxL0 < !!(task.m_frameType & MFX_FRAMETYPE_I); // tricky: use idxL0 for 1 iteration for I-frame
         ++idxL0, ++idxL1)
    {
        RefIdxPair dpbRefIdxPair    = {IDX_INVALID, IDX_INVALID};
        RefIdxPair activeRefIdxPair = {IDX_INVALID, IDX_INVALID};

        if (RPL[0][idxL0] < MAX_DPB_SIZE)
        {
            dpbRefIdxPair.RefL0    = RPL[0][idxL0];
            activeRefIdxPair.RefL0 = idxL0;
        }

        if (RPL[1][idxL1] < MAX_DPB_SIZE)
        {
            dpbRefIdxPair.RefL1    = RPL[1][idxL1];
            activeRefIdxPair.RefL1 = idxL1;
        }

        // mfxStatus sts;
        const HevcDpbArray & DPB = task.m_dpb[TASK_DPB_ACTIVE];

        mfxENCInputWrap & in = m_syncp.second.first;
        mfxENCOutputWrap & out = m_syncp.second.second;

        for (mfxU16 i = 0; i < m_numOfFields; i++) {
            in.InSurface  = *task.m_ds_surf ? *task.m_ds_surf : task.m_surf;
            in.NumFrameL0 = (dpbRefIdxPair.RefL0 != IDX_INVALID);
            in.NumFrameL1 = (dpbRefIdxPair.RefL1 != IDX_INVALID);

            mfxExtFeiPreEncCtrl* ctrl = in.GetExtBuffer<mfxExtFeiPreEncCtrl>(i);
            MSDK_CHECK_POINTER(ctrl, MFX_ERR_NULL_PTR);

            ctrl->PictureType     = GetCurPicType(i);
            ctrl->DownsampleInput = bDownsampleInput ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF;
            ctrl->RefPictureType[0] = ctrl->RefPictureType[1] = ctrl->PictureType;
            ctrl->DownsampleReference[0] = ctrl->DownsampleReference[1] = MFX_CODINGOPTION_OFF;

            ctrl->RefFrame[0] = NULL;
            ctrl->RefFrame[1] = NULL;
            if (dpbRefIdxPair.RefL0 < MAX_DPB_SIZE)
            {
                if (*task.m_ds_surf)
                    ctrl->RefFrame[0] = *DPB[dpbRefIdxPair.RefL0].m_ds_surf;
                else
                    ctrl->RefFrame[0] = DPB[dpbRefIdxPair.RefL0].m_surf;
            }
            if (dpbRefIdxPair.RefL1 < MAX_DPB_SIZE)
            {
                if (*task.m_ds_surf)
                    ctrl->RefFrame[1] = *DPB[dpbRefIdxPair.RefL1].m_ds_surf;
                else
                    ctrl->RefFrame[1] = DPB[dpbRefIdxPair.RefL1].m_surf;
            }

            // disable MV output for I frames / if no reference frames provided
            ctrl->DisableMVOutput = (task.m_frameType & MFX_FRAMETYPE_I) || (IDX_INVALID == dpbRefIdxPair.RefL0 && IDX_INVALID == dpbRefIdxPair.RefL1);

            PreENCOutput stat;
            MSDK_ZERO_MEMORY(stat);
            if (!ctrl->DisableMVOutput)
            {
                mfxExtFeiPreEncMV * mv = out.AddExtBuffer<mfxExtFeiPreEncMV>(i);
                MSDK_CHECK_POINTER(mv, MFX_ERR_NULL_PTR);

                mfxExtFeiPreEncMVExtended * ext_mv = AcquireResource(m_mvs);

                if (!ext_mv) {
                    MLOG_ERROR("ERROR: No free buffer in mfxExtFeiPreEncMVExtended.");
                    return MFX_ERR_MEMORY_ALLOC;
                }
                mfxExtFeiPreEncMV & free_mv = *ext_mv;
                Copy(*mv, free_mv);

                stat.m_mv = ext_mv;
                stat.m_activeRefIdxPair = activeRefIdxPair;
            }
            ctrl->DisableStatisticsOutput = false;

            if (!ctrl->DisableStatisticsOutput)
            {
                mfxExtFeiPreEncMBStat* mb = out.AddExtBuffer<mfxExtFeiPreEncMBStat>(i);
                MSDK_CHECK_POINTER(mb, MFX_ERR_NULL_PTR);

                mfxExtFeiPreEncMBStatExtended * ext_mb = AcquireResource(m_mbs);

                if (!ext_mb) {
                    MLOG_ERROR("ERROR: No free buffer in mfxExtFeiPreEncMBStatExtended.");
                    return MFX_ERR_MEMORY_ALLOC;
                }
                mfxExtFeiPreEncMBStat & free_mb = *ext_mb;
                Copy(*mb, free_mb);

                stat.m_mb = ext_mb;
            }

            if (stat.m_mb || stat.m_mv)
                task.m_preEncOutput.push_back(stat);
        }

        mfxSyncPoint & syncp = m_syncp.first;
        sts = m_preenc->ProcessFrameAsync(&in, &out, &syncp);
        if (sts < MFX_ERR_NONE || !syncp) {
            MLOG_ERROR("FEI PreEnc ProcessFrameAsync failed. sts = %d\n", sts);
            return sts;
        }

        sts = m_session->SyncOperation(syncp, MSDK_WAIT_INTERVAL);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("FEI PreEnc SyncOperation failed. sts = %d\n", sts);
            return sts;
        }

        // If input surface is not changed between PreENC calls
        // an application can avoid redundant downsampling on driver side.
        bDownsampleInput = false;
    }

    return sts;
}

mfxStatus H265FEIEncoderPreenc::DumpResult(HevcTask* task)
{
    MSDK_CHECK_POINTER(task, MFX_ERR_NULL_PTR);

    m_isMVoutFormatted = false;

    if (m_pMvoutStream)
    {
        // count number of MB 16x16, as PreENC works as AVC
        mfxU32 numMB = (MSDK_ALIGN16(task->m_surf->Info.Width) * MSDK_ALIGN16(task->m_surf->Info.Height)) >> 8;

        {
            if (task->m_frameType & MFX_FRAMETYPE_I)
            {
                if (*task->m_ds_surf)
                {
                    numMB = (MSDK_ALIGN16((*task->m_ds_surf)->Info.Width) * MSDK_ALIGN16((*task->m_ds_surf)->Info.Height)) >> 8;
                }
                for (mfxU32 k = 0; k < numMB; k++)
                {
                    if (m_pMvoutStream->WriteBlock(&m_default_MVMB, sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB), false) != sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB)) {
                        //MSDK_CHECK_STATUS(sts, "Write MV output to file failed in DumpResult");
                        MLOG_ERROR("Write MV output to file failed in DumpResult.");
                        return MFX_ERR_MORE_DATA;
                    }
                }
            }
            else
            {
                for (std::list<PreENCOutput>::iterator it = task->m_preEncOutput.begin(); it != task->m_preEncOutput.end(); ++it)
                {
                    if (m_pMvoutStream->WriteBlock(it->m_mv->MB, sizeof(it->m_mv->MB[0]) * it->m_mv->NumMBAlloc, false) != sizeof(it->m_mv->MB[0]) * it->m_mv->NumMBAlloc) {
                        //MSDK_CHECK_STATUS(sts, "Write MV output to file failed in DumpResult");
                        MLOG_ERROR("Write MV output to file failed in DumpResult.");
                        return MFX_ERR_MORE_DATA;
                    }
                }
            }
        }
    }

    return MFX_ERR_NONE;
}

void H265FEIEncoderPreenc::GetRefInfo(
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

    numRefActiveP   = m_extParams->fei_ctrl.nRefActiveP;
    numRefActiveBL0 = m_extParams->fei_ctrl.nRefActiveBL0;
    numRefActiveBL1 = m_extParams->fei_ctrl.nRefActiveBL1;

    picStruct = m_preencParams.mfx.FrameInfo.PicStruct;
    refDist = m_preencParams.mfx.GopRefDist;
    numRefFrame = m_preencParams.mfx.NumRefFrame;
    gopSize = m_preencParams.mfx.GopPicSize;
    gopOptFlag = m_preencParams.mfx.GopOptFlag;
    idrInterval = m_preencParams.mfx.IdrInterval;
}

mfxStatus H265FEIEncoderPreenc::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_preenc->GetVideoParam(par);
    return sts;
}
MfxVideoParamsWrapper H265FEIEncoderPreenc::GetVideoParams()
{
    MfxVideoParamsWrapper pars;

    memcpy(&(pars.mfx), &(m_preencParams.mfx), sizeof(mfxInfoMFX));
    pars.AsyncDepth            = 1; // inherited limitation from AVC FEI
    pars.IOPattern             = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    mfxExtCodingOption* pCO = pars.AddExtBuffer<mfxExtCodingOption>();
    if (!pCO) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption");

    pCO->PicTimingSEI = MFX_CODINGOPTION_OFF;

    mfxExtCodingOption2* pCO2 = pars.AddExtBuffer<mfxExtCodingOption2>();
    if (!pCO2) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption2");

    // configure B-pyramid settings
    pCO2->BRefType = m_extParams->bRefType;

    mfxExtCodingOption3* pCO3 = pars.AddExtBuffer<mfxExtCodingOption3>();
    if (!pCO3) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Failed to attach mfxExtCodingOption3");

    pCO3->PRefType             = m_extParams->pRefType;

    std::fill(pCO3->NumRefActiveP,   pCO3->NumRefActiveP + 8,   m_extParams->fei_ctrl.nRefActiveP);
    std::fill(pCO3->NumRefActiveBL0, pCO3->NumRefActiveBL0 + 8, m_extParams->fei_ctrl.nRefActiveBL0);
    std::fill(pCO3->NumRefActiveBL1, pCO3->NumRefActiveBL1 + 8, m_extParams->fei_ctrl.nRefActiveBL1);


    pCO3->GPB = m_extParams->GPB;

    // qp offset per pyramid layer, default is library behavior
    pCO3->EnableQPOffset = MFX_CODINGOPTION_UNKNOWN;

    return pars;
}
#endif //MSDK_HEVC_FEI
