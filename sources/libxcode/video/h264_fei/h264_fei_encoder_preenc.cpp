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

#include "h264_fei_encoder_preenc.h"
#ifdef MSDK_AVC_FEI

DEFINE_MLOGINSTANCE_CLASS(H264FEIEncoderPreenc, "H264FEIEncoderPreenc");

H264FEIEncoderPreenc::H264FEIEncoderPreenc(MFXVideoSession *session,
                                   iTaskPool *task_pool,
                                   bufList* ext_buf,
                                   bufList* enc_ext_buf,
                                   bool UseOpaqueMemory,
                                   PipelineCfg* cfg,
                                   EncExtParams* extParams)
    :m_session(session),
     m_pipelineCfg(cfg),
     m_inputTasks(task_pool),
     m_extBufs(ext_buf),
     m_encodeBufs(enc_ext_buf),
     m_bMVout(false),
     m_pMvoutStream(NULL),
     m_pMvinStream(NULL),
     m_extParams(extParams),
     m_bUseOpaqueMemory(UseOpaqueMemory)

{
    memset(&m_preencParams, 0, sizeof(mfxVideoParam));
    memset(&m_DSParams, 0, sizeof(mfxVideoParam));
    memset(&m_fullResParams, 0, sizeof(mfxVideoParam));
    //memset(&m_DSResponse, 0, sizeof(mfxFrameAllocResponse));
    memset(&extDontUse,0,sizeof(extDontUse));
    /* Default values for I-frames */
    memset(&m_tmpMVMB, 0x8000, sizeof(mfxExtFeiEncMV::mfxExtFeiEncMVMB));

    m_preenc = NULL;
    m_ds = NULL;
    m_DSstrength = 0;
}

H264FEIEncoderPreenc::~H264FEIEncoderPreenc()
{
    if (m_ds) {
        m_ds->Close();
        delete m_ds;
        m_ds = NULL;
    }
    if (m_preenc) {
        m_preenc->Close();
        delete m_preenc;
        m_preenc = NULL;
    }

    if (!m_PreencExtParams.empty()) {
        m_PreencExtParams.clear();
    }

    if (!m_DSExtParams.empty()) {
        m_DSExtParams.clear();
    }

    if (extDontUse.AlgList)
        delete extDontUse.AlgList;
}

bool H264FEIEncoderPreenc::Init(ElementCfg *ele_param)
{
    if (ele_param->extParams.fei_ctrl.nDSstrength){
        m_ds = new MFXVideoVPP(*m_session);
        m_DSstrength = ele_param->extParams.fei_ctrl.nDSstrength;
    }

    m_preenc = new MFXVideoENC(*m_session);

    /*PREENC video parameters*/
    m_preencParams.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
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
    m_preencParams.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
    m_preencParams.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
    m_preencParams.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
    m_preencParams.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    //NumRefFrame is used to define the size of iTask_pool, so should be defined by GopRefDist
    m_preencParams.mfx.NumRefFrame = (m_preencParams.mfx.GopRefDist+1) > 4? 4: (m_preencParams.mfx.GopRefDist+1);
    m_preencParams.mfx.GopOptFlag = ele_param->EncParams.mfx.GopOptFlag;

    return true;
}

mfxStatus H264FEIEncoderPreenc::InitPreenc(MediaBuf &buf, mfxFrameAllocRequest *request)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_DSstrength){
        sts = InitDS(buf, request);
        if (MFX_ERR_NONE != sts){
            MLOG_ERROR("down sampling create failed %d\n", sts);
            return sts;
        }
    }

    if (m_DSstrength){
        memcpy(&(m_preencParams.mfx.FrameInfo), &(m_DSParams.vpp.Out), sizeof(mfxFrameInfo));
    } else{
        m_preencParams.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
        m_preencParams.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
        m_preencParams.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
        m_preencParams.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
        m_preencParams.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
        m_preencParams.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
        m_preencParams.mfx.FrameInfo.Width         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width;
        m_preencParams.mfx.FrameInfo.Height        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height;
    }

    m_preencParams.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    m_preencParams.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;
    m_preencParams.mfx.FrameInfo.BitDepthLuma  = 8;
    m_preencParams.mfx.FrameInfo.BitDepthChroma = 8;

    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        m_preencParams.mfx.FrameInfo.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        m_preencParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    memcpy(&m_fullResParams, &m_preencParams, sizeof(mfxVideoParam));
    if (m_ds){
        memcpy(&m_fullResParams.mfx.FrameInfo, &m_DSParams.vpp.In, sizeof(mfxFrameInfo));
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

    m_extParams->fei_ctrl.nRefActiveBL1     = ((m_preencParams.mfx.FrameInfo.PicStruct) & MFX_PICSTRUCT_PROGRESSIVE) ? 1
                                                : ((m_extParams->fei_ctrl.nRefActiveBL1 >2 || m_extParams->fei_ctrl.nRefActiveBL1 == 0)? 2 : m_extParams->fei_ctrl.nRefActiveBL1);

    m_bMVout = m_extParams->bEncode || m_extParams->bENC ;

    if (!!m_extParams->mvoutStream && !(m_extParams->bEncode || m_extParams->bENC))
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

    sts = m_preenc->Init(&m_preencParams);
    if (sts > MFX_ERR_NONE) {
        MLOG_WARNING("\t\tPreenc init warning %d\n", sts);
    } else if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\tPreenc init return error %d\n", sts);
    }

    return sts;
}

mfxStatus H264FEIEncoderPreenc::InitDS(MediaBuf &buf, mfxFrameAllocRequest *request)
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

    // configure and attach external parameters
    extDontUse.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    extDontUse.Header.BufferSz = sizeof(mfxExtVPPDoNotUse);
    extDontUse.NumAlg = 4;

    extDontUse.AlgList = new mfxU32[extDontUse.NumAlg];
    // turn off denoising (on by default)
    extDontUse.AlgList[0] = MFX_EXTBUFF_VPP_DENOISE;
    // turn off scene analysis (on by default)
    extDontUse.AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS;
    // turn off detail enhancement (on by default)
    extDontUse.AlgList[2] = MFX_EXTBUFF_VPP_DETAIL;
    // turn off processing amplified (on by default)
    extDontUse.AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP;
    m_DSExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&extDontUse));

    m_DSParams.ExtParam    = &m_DSExtParams[0];
    m_DSParams.NumExtParam = (mfxU16)m_DSExtParams.size();

    memset(request, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = m_ds->QueryIOSurf(&m_DSParams, request);
    MLOG_INFO("DownSampling suggest number of surfaces is in/out %d/%d\n",
        request[0].NumFrameSuggested,request[1].NumFrameSuggested);

    sts = m_ds->Init(&m_DSParams);

    if (MFX_WRN_FILTER_SKIPPED == sts) {
        MLOG_INFO("------------------- Got MFX_WRN_FILTER_SKIPPED\n");
        sts = MFX_ERR_NONE;
    }

    return sts;
}

mfxStatus H264FEIEncoderPreenc::QueryIOSurf(mfxFrameAllocRequest* request)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_preenc->QueryIOSurf(&m_preencParams, request);
    MLOG_INFO("Preenc suggest surfaces %d\n", request->NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    return sts;
}

mfxStatus H264FEIEncoderPreenc::ProcessChainPreenc(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    /* PreENC DownSampling */
    if (m_ds)
    {
        sts = DownSampleInput(eTask);
        if (sts < MFX_ERR_NONE) {MLOG_ERROR("\t\tPreENC DownsampleInput failed with %d\n", sts); return sts;}
    }

    sts = ProcessMultiPreenc(eTask);

    if (sts < MFX_ERR_NONE) {MLOG_ERROR("\t\tProcessMultiPreenc failed with %d\n", sts); return sts;}

    if (m_pipelineCfg->bEncode || m_pipelineCfg->bEnc) {
        // Pass MVP to encode
        if (eTask->m_fieldPicFlag || !(ExtractFrameType(*eTask) & MFX_FRAMETYPE_I))
        {
            //repack MV predictors
            sts = RepackPredictors(eTask);
            if (sts < MFX_ERR_NONE) {MLOG_ERROR("\t\tPredictors Repacking failed with %d\n", sts); return sts;}
        }
    }

    return sts;
}

mfxStatus H264FEIEncoderPreenc::DownSampleInput(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask,                 MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(eTask->PREENC_in.InSurface,   MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(eTask->ENC_in.InSurface, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;
    mfxSyncPoint     m_SyncPoint = 0;

    // VPP will use this surface as output, so it should be unlocked
    mfxU16 locker = eTask->PREENC_in.InSurface->Data.Locked;
    eTask->PREENC_in.InSurface->Data.Locked = 0;

    for (;;)
    {
        sts = m_ds->RunFrameVPPAsync(eTask->ENC_in.InSurface, eTask->PREENC_in.InSurface, NULL, &m_SyncPoint);

        if (!m_SyncPoint)
        {
            if (MFX_WRN_DEVICE_BUSY == sts){
                WaitForDeviceToBecomeFree(*m_session, m_SyncPoint, sts);
            }
            else
                return sts;
        }
        else if (MFX_ERR_NONE <= sts) {
            // ignore warnings if output is available
            sts = MFX_ERR_NONE;
            break;
        }
        else
            MSDK_BREAK_ON_ERROR(sts);
    }
    if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\tPreENC DownsampleInput failed with %d\n", sts);
        return sts;
    }

    for (;;)
    {
        sts = m_session->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);

        if (!m_SyncPoint)
        {
            if (MFX_WRN_DEVICE_BUSY == sts){
                WaitForDeviceToBecomeFree(*m_session, m_SyncPoint, sts);
            }
            else
                return sts;
        }
        else if (MFX_ERR_NONE <= sts) {
            // ignore warnings if output is available
            sts = MFX_ERR_NONE;
            break;
        }
        else
            MSDK_BREAK_ON_ERROR(sts);
    }

    // restore locker
    eTask->PREENC_in.InSurface->Data.Locked = locker;

    return sts;
}

mfxStatus H264FEIEncoderPreenc::RepackPredictors(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtFeiEncMVPredictors* mvp = NULL;
    std::vector<mfxExtFeiPreEncMV*> mvs_v;
    std::vector<mfxU8*>             refIdx_v;
    bufSet* set = m_encodeBufs->GetFreeSet();
    MSDK_CHECK_POINTER(set, MFX_ERR_NULL_PTR);

    mvs_v.reserve(MaxFeiEncMVPNum);
    refIdx_v.reserve(MaxFeiEncMVPNum);

    mfxU32 i, j, preencMBidx, MVrow, MVcolumn, preencMBMVidx;

    mfxU32 nOfPredPairs = 0, numOfFields = eTask->m_fieldPicFlag ? 2 : 1;


    for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
    {
        nOfPredPairs = (std::min)(MaxFeiEncMVPNum , (std::max)(m_pipelineCfg->numOfPredictors[fieldId][0],m_pipelineCfg->numOfPredictors[fieldId][1]));

        if (nOfPredPairs == 0 || (ExtractFrameType(*eTask, fieldId) & MFX_FRAMETYPE_I)) {
            // I-field
            continue;
        }

        mvs_v.clear();
        refIdx_v.clear();

        mvp = reinterpret_cast<mfxExtFeiEncMVPredictors*>(set->PB_bufs.in.getBufById(MFX_EXTBUFF_FEI_ENC_MV_PRED, fieldId));
        if (mvp == NULL){
            sts = MFX_ERR_NULL_PTR;
            break;
        }

        j = 0;
        for (std::list<PreEncOutput>::iterator it = eTask->preenc_output.begin(); j < nOfPredPairs && it != eTask->preenc_output.end(); ++it, ++j)
        {
            mvs_v.push_back(reinterpret_cast<mfxExtFeiPreEncMV*>((*it).output_bufs->PB_bufs.out.getBufById(MFX_EXTBUFF_FEI_PREENC_MV, fieldId)));
            refIdx_v.push_back((*it).refIdx[fieldId]);
        }

        // get nPred_actual L0/L1 predictors for each MB
        for (i = 0; i < m_pipelineCfg->numMB_refPic; ++i)
        {
            for (j = 0; j < nOfPredPairs; ++j)
            {
                mvp->MB[i].RefIdx[j].RefL0 = refIdx_v[j][0];
                mvp->MB[i].RefIdx[j].RefL1 = refIdx_v[j][1];

                if (!m_DSstrength)
                {
                    memcpy(mvp->MB[i].MV[j], mvs_v[j]->MB[i].MV[0], 2 * sizeof(mfxI16Pair));
                }
                else
                {
                    static mfxI16 MVZigzagOrder[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };

                    static mfxU16 widthMB_ds = ((m_ds? m_DSParams.vpp.Out.Width : m_DSParams.mfx.FrameInfo.Width) + 15) >> 4,
                                widthMB_full = ((m_ds? m_DSParams.vpp.In.Width  : m_DSParams.mfx.FrameInfo.Width) + 15) >> 4;

                    preencMBidx = i / widthMB_full / m_DSstrength * widthMB_ds + i % widthMB_full / m_DSstrength;

                    if (preencMBidx >= m_pipelineCfg->numMB_preenc_refPic)
                        continue;

                    MVrow    = i / widthMB_full % m_DSstrength;
                    MVcolumn = i % widthMB_full % m_DSstrength;

                    switch (m_DSstrength)
                    {
                    case 2:
                        preencMBMVidx = MVrow * 8 + MVcolumn * 2;
                        break;
                    case 4:
                        preencMBMVidx = MVrow * 4 + MVcolumn;
                        break;
                    case 8:
                        preencMBMVidx = MVrow / 2 * 4 + MVcolumn / 2;
                        break;
                    default:
                        preencMBMVidx = 0;
                        break;
                    }

                    memcpy(mvp->MB[i].MV[j], mvs_v[j]->MB[preencMBidx].MV[MVZigzagOrder[preencMBMVidx]], 2 * sizeof(mfxI16Pair));

                    mvp->MB[i].MV[j][0].x *= m_DSstrength;
                    mvp->MB[i].MV[j][0].y *= m_DSstrength;
                    mvp->MB[i].MV[j][1].x *= m_DSstrength;
                    mvp->MB[i].MV[j][1].y *= m_DSstrength;
                }
            }
        }

    }

    SAFE_RELEASE_EXT_BUFSET(set);

    eTask->ReleasePreEncOutput();

    return sts;

}

mfxStatus H264FEIEncoderPreenc::InitPreencFrameParams(iTask* eTask, iTask* refTask[2][2], mfxU8 ref_fid[2][2], bool isDownsamplingNeeded)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxFrameSurface1* refSurf0[2] = { NULL, NULL }; // L0 ref surfaces
    mfxFrameSurface1* refSurf1[2] = { NULL, NULL }; // L1 ref surfaces

    eTask->preenc_bufs = m_extBufs->GetFreeSet();
    MSDK_CHECK_POINTER(eTask->preenc_bufs, MFX_ERR_NULL_PTR);

    for (mfxU32 fieldId = 0; fieldId < mfxU32(eTask->m_fieldPicFlag ? 2 : 1); fieldId++)
    {
        bool is_I_frame = ExtractFrameType(*eTask, fieldId) & MFX_FRAMETYPE_I;

        refSurf0[fieldId] = refTask[fieldId][0] ? refTask[fieldId][0]->PREENC_in.InSurface : NULL;
        refSurf1[fieldId] = refTask[fieldId][1] ? refTask[fieldId][1]->PREENC_in.InSurface : NULL;

        eTask->PREENC_in.NumFrameL0 = !!refSurf0[fieldId];
        eTask->PREENC_in.NumFrameL1 = !!refSurf1[fieldId];

        // Initialize controller for Extension buffers
        mfxStatus sts = eTask->ExtBuffersController.InitializeController(eTask->preenc_bufs, bufSetController::PREENC, is_I_frame, !eTask->m_fieldPicFlag);
        //MSDK_CHECK_STATUS(sts, "eTask->ExtBuffersController.InitializeController failed");
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("eTask->ExtBuffersController.InitializeController failed.\n");
            return sts;
        }
    }

    mfxU32 preENCCtrId = 0, pMvPredId = 0;
    mfxU8 type = MFX_FRAMETYPE_UNKNOWN;
    bool ref0_isFrame = !eTask->m_fieldPicFlag, ref1_isFrame = !eTask->m_fieldPicFlag;


    for (std::vector<mfxExtBuffer*>::iterator it = eTask->preenc_bufs->PB_bufs.in.buffers.begin();
        it != eTask->preenc_bufs->PB_bufs.in.buffers.end(); ++it)
    {
        switch ((*it)->BufferId)
        {
        case MFX_EXTBUFF_FEI_PREENC_CTRL:
        {
            mfxExtFeiPreEncCtrl* preENCCtr = reinterpret_cast<mfxExtFeiPreEncCtrl*>(*it);

            /* Get type of current field */
            type = ExtractFrameType(*eTask, preENCCtrId);
            /* Disable MV output for I frames / if no reference frames provided / if no mv output requested */
            preENCCtr->DisableMVOutput = (type & MFX_FRAMETYPE_I)
                                || (!refSurf0[preENCCtrId] && !refSurf1[preENCCtrId]) || !m_bMVout;

            //preENCCtr->DisableStatisticsOutput = true;

            /* Section below sets up reference frames and their types */
            ref0_isFrame = refSurf0[preENCCtrId] ? (refSurf0[preENCCtrId]->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) :
                !eTask->m_fieldPicFlag;
            ref1_isFrame = refSurf1[preENCCtrId] ? (refSurf1[preENCCtrId]->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) :
                !eTask->m_fieldPicFlag;

            preENCCtr->RefPictureType[0] = mfxU16(ref0_isFrame ? MFX_PICTYPE_FRAME :
                ref_fid[preENCCtrId][0] == 0 ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);

            preENCCtr->RefPictureType[1] = mfxU16(ref1_isFrame ? MFX_PICTYPE_FRAME :
                ref_fid[preENCCtrId][1] == 0 ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);

            preENCCtr->RefFrame[0] = refSurf0[preENCCtrId];
            preENCCtr->RefFrame[1] = refSurf1[preENCCtrId];

            /* Default value is ON */
            preENCCtr->DownsampleInput = mfxU16(isDownsamplingNeeded ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF);

            /* Configure MV predictors */
            preENCCtr->MVPredictor = 0;

            preENCCtrId++;
        }
        break;

        case MFX_EXTBUFF_FEI_PREENC_MV_PRED:

            if (m_pMvinStream)
            {
                /* Skip MV predictor data for I-fields/frames */
                if (!(ExtractFrameType(*eTask, pMvPredId) & MFX_FRAMETYPE_I))
                {
                    mfxExtFeiPreEncMVPredictors* pMvPredBuf = reinterpret_cast<mfxExtFeiPreEncMVPredictors*>(*it);

                    if(m_pMvinStream->ReadBlock(pMvPredBuf->MB, sizeof(pMvPredBuf->MB[0])*pMvPredBuf->NumMBAlloc ) != sizeof(pMvPredBuf->MB[0])*pMvPredBuf->NumMBAlloc){
                         MLOG_ERROR("m_pMvinStream: ReadBlock failed.\n");
                         return MFX_ERR_MORE_DATA;
                    }
                }
                else{
                    if(!!( m_pMvinStream->SetStreamAttribute(FILE_CUR, sizeof(mfxExtFeiPreEncMVPredictors::mfxExtFeiPreEncMVPredictorsMB)*m_pipelineCfg->numMB_preenc_refPic))){
                         MLOG_ERROR("m_pMvinStream: SetStreamAttribute failed.\n");
                         return MFX_ERR_MORE_DATA;
                    }
                }
                pMvPredId++;
            }
            break;

        }
    } // for (int i = 0; i<eTask->in.NumExtParam; i++)

    mdprintf(stderr, "enc: %d t: %d\n", eTask->m_frameOrder, (eTask->m_type[0] & MFX_FRAMETYPE_IPB));
    return MFX_ERR_NONE;
}

mfxStatus H264FEIEncoderPreenc::ProcessMultiPreenc(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(eTask->PREENC_in.InSurface,   MFX_ERR_NULL_PTR);

    mfxStatus sts = MFX_ERR_NONE;
    MSDK_ZERO_ARRAY(m_pipelineCfg->numOfPredictors[0], 2);
    MSDK_ZERO_ARRAY(m_pipelineCfg->numOfPredictors[1], 2);

    mfxU32 numOfFields = eTask->m_fieldPicFlag ? 2 : 1;

    // max possible candidates to L0 / L1
    mfxU32 total_l0 = (ExtractFrameType(*eTask, eTask->m_fieldPicFlag) & MFX_FRAMETYPE_P) ? ((ExtractFrameType(*eTask) & MFX_FRAMETYPE_IDR) ? 1 : eTask->NumRefActiveP) : ((ExtractFrameType(*eTask) & MFX_FRAMETYPE_I) ? 1 : eTask->NumRefActiveBL0);
    mfxU32 total_l1 = (ExtractFrameType(*eTask) & MFX_FRAMETYPE_B) ? eTask->NumRefActiveBL1 : 1; // just one iteration here for non-B

    // adjust to maximal number of references
    total_l0 = (std::min)(total_l0, numOfFields*m_preencParams.mfx.NumRefFrame);
    total_l1 = (std::min)(total_l1, numOfFields*m_preencParams.mfx.NumRefFrame);

    // indexes means [fieldId][L0L1]
    mfxU8 preenc_ref_idx[2][2];
    mfxU8 ref_fid[2][2];
    iTask* refTask[2][2];
    bool isDownsamplingNeeded = true;
    mfxSyncPoint     m_SyncPoint = 0;

    for (mfxU8 l0_idx = 0, l1_idx = 0; l0_idx < total_l0 || l1_idx < total_l1; l0_idx++, l1_idx++)
    {
        sts = GetRefTaskEx(eTask, l0_idx, l1_idx, preenc_ref_idx, ref_fid, refTask);

        if (ExtractFrameType(*eTask) & MFX_FRAMETYPE_I)
            MSDK_IGNORE_MFX_STS(sts, MFX_WRN_OUT_OF_RANGE);

        if (sts == MFX_WRN_OUT_OF_RANGE){ // some of the indexes exceeds DPB
            sts = MFX_ERR_NONE;
            continue;
        }
        MSDK_BREAK_ON_ERROR(sts);

        for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
        {
            if (refTask[fieldId][0]) { m_pipelineCfg->numOfPredictors[fieldId][0]++; }
            if (refTask[fieldId][1]) { m_pipelineCfg->numOfPredictors[fieldId][1]++; }
        }

        sts = InitPreencFrameParams(eTask, refTask, ref_fid, isDownsamplingNeeded);
        if (sts < MFX_ERR_NONE)
            {MLOG_ERROR("\t\tPreENC: InitPreencFrameParams failed with %d\n", sts); return sts;}

        // If input surface is not being changed between PreENC calls
        // (including calls for different fields of the same field pair in Single Field processing mode),
        // an application can avoid redundant downsampling on driver side.
        isDownsamplingNeeded = false;

        // Doing PreEnc
        for (;;)
        {
            for (int i = 0; i < 1 ; ++i)
            {
                // Attach extension buffers for current field
                // (in double-field mode both calls will return equal sets, holding buffers for both fields)

                std::vector<mfxExtBuffer *> * in_buffers = eTask->ExtBuffersController.GetBuffers(bufSetController::PREENC, i, true);
                MSDK_CHECK_POINTER(in_buffers, MFX_ERR_NULL_PTR);

                std::vector<mfxExtBuffer *> * out_buffers = eTask->ExtBuffersController.GetBuffers(bufSetController::PREENC, i, false);
                MSDK_CHECK_POINTER(out_buffers, MFX_ERR_NULL_PTR);

                // Input buffers
                eTask->PREENC_in.NumExtParam  = mfxU16(in_buffers->size());
                eTask->PREENC_in.ExtParam     = in_buffers->data();

                // Output buffers
                eTask->PREENC_out.NumExtParam = mfxU16(out_buffers->size());
                eTask->PREENC_out.ExtParam    = out_buffers->data();

                sts = m_preenc->ProcessFrameAsync(&eTask->PREENC_in, &eTask->PREENC_out, &m_SyncPoint);
                MSDK_BREAK_ON_ERROR(sts);

                /*PRE-ENC is running in separate session */
                sts = m_session->SyncOperation(m_SyncPoint, MSDK_WAIT_INTERVAL);

                MSDK_BREAK_ON_ERROR(sts);
                mdprintf(stderr, "preenc synced : %d\n", sts);
            }

            if (MFX_ERR_NONE < sts && !m_SyncPoint) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts)
                {
                    WaitForDeviceToBecomeFree(*m_session, m_SyncPoint, sts);
                }
            }
            else if (MFX_ERR_NONE < sts && m_SyncPoint)
            {
                sts = MFX_ERR_NONE; // ignore warnings if output is available
                break;
            }
            else
            {
                break;
            }
        }
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tFEI PreENC failed to process frame with %d\n", sts);
            return sts;
        }

        // Store PreEnc output
        eTask->preenc_output.push_back(PreEncOutput(eTask->preenc_bufs, preenc_ref_idx));

        //drop output data to output file
        sts = FlushOutput(eTask);
        if (sts < MFX_ERR_NONE) {
            MLOG_ERROR("\t\tFPreENC: FlushOutput failed. sts: %d\n", sts);
            return sts;
        }
    } // for (mfxU32 l0_idx = 0, l1_idx = 0; l0_idx < total_l0 || l1_idx < total_l1; l0_idx++, l1_idx++)

    return sts;
}
mfxStatus H264FEIEncoderPreenc::FlushOutput(iTask* eTask)
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    int mvsId = 0;

    for (int i = 0; i < eTask->PREENC_out.NumExtParam; i++)
    {
        switch (eTask->PREENC_out.ExtParam[i]->BufferId){

        case MFX_EXTBUFF_FEI_PREENC_MV:

            if (m_pMvoutStream)
            {
                mfxExtFeiPreEncMV* mvs = reinterpret_cast<mfxExtFeiPreEncMV*>(eTask->PREENC_out.ExtParam[i]);

                if (ExtractFrameType(*eTask, mvsId) & MFX_FRAMETYPE_I)
                {
                    for (mfxU32 k = 0; k < mvs->NumMBAlloc; k++){

                        if(m_pMvoutStream->WriteBlock(&m_tmpMVMB,sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB),false ) != sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB)){
                            MLOG_ERROR("\t\t WriteBlock return Error \n");
                            return MFX_ERR_MORE_DATA;
                        }
                    }
                }
                else
                {
                    if(m_pMvoutStream->WriteBlock(mvs->MB, sizeof(mvs->MB[0]) * mvs->NumMBAlloc, false ) != sizeof(mvs->MB[0]) * mvs->NumMBAlloc){
                        MLOG_ERROR("\t\t WriteBlock return Error \n");
                        return MFX_ERR_MORE_DATA;
                    }
                }
            }
            mvsId++;
            break;
        default:
            break;
        } // switch (eTask->PREENC_out.ExtParam[i]->BufferId)
    } //for (int i = 0; i < eTask->PREENC_out.NumExtParam; i++)

    if (!eTask->m_fieldPicFlag && m_pMvoutStream && (ExtractFrameType(*eTask) & MFX_FRAMETYPE_I)){ // drop 0x8000 for progressive I-frames
        for (mfxU32 k = 0; k < m_pipelineCfg->numMB_preenc_refPic; k++){
            if(m_pMvoutStream->WriteBlock(&m_tmpMVMB, sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB), false ) != sizeof(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB)){
                MLOG_ERROR("\t\t WriteBlock return Error \n");
                return MFX_ERR_MORE_DATA;
            }
        }
    }

    return MFX_ERR_NONE;
}


mfxStatus H264FEIEncoderPreenc::GetRefTaskEx(iTask *eTask, mfxU8 l0_idx, mfxU8 l1_idx, mfxU8 refIdx[2][2], mfxU8 ref_fid[2][2], iTask *outRefTask[2][2])
{
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);

    mfxStatus stsL0 = MFX_WRN_OUT_OF_RANGE, stsL1 = MFX_WRN_OUT_OF_RANGE;

    for (int i = 0; i < 2; i++){
        MSDK_ZERO_ARRAY(refIdx[i], 2);
        MSDK_ZERO_ARRAY(outRefTask[i], 2);
        MSDK_ZERO_ARRAY(ref_fid[i], 2);
    }

    mfxU32 numOfFields = eTask->m_fieldPicFlag ? 2 : 1;
    for (mfxU32 fieldId = 0; fieldId < numOfFields; fieldId++)
    {
        mfxU8 type = ExtractFrameType(*eTask, fieldId);
        mfxU32 l0_ref_count = eTask->GetNBackward(fieldId),
               l1_ref_count = eTask->GetNForward(fieldId),
               fid = eTask->m_fid[fieldId];

        /* adjustment for case of equal l0 and l1 lists*/
        if (!l0_ref_count && eTask->m_list0[fid].Size() && !(eTask->m_type[fid] & MFX_FRAMETYPE_I))
        {
            l0_ref_count = eTask->m_list0[fid].Size();
        }

        if (l0_idx < l0_ref_count && eTask->m_list0[fid].Size())
        {
            mfxU8 const * l0_instance = eTask->m_list0[fid].Begin() + l0_idx;
            iTask *L0_ref_task = m_inputTasks->GetTaskByFrameOrder(eTask->m_dpb[fid][*l0_instance & 127].m_frameOrder);
            MSDK_CHECK_POINTER(L0_ref_task, MFX_ERR_NULL_PTR);

            refIdx[fieldId][0] = l0_idx;
            outRefTask[fieldId][0] = L0_ref_task;
            ref_fid[fieldId][0] = (*l0_instance > 127) ? 1 : 0; // 1 - bottom field, 0 - top field

            stsL0 = MFX_ERR_NONE;
        }
        else {
            if (eTask->m_list0[fid].Size())
                stsL0 = MFX_WRN_OUT_OF_RANGE;
        }

        if (l1_idx < l1_ref_count && eTask->m_list1[fid].Size())
        {
            if (type & MFX_FRAMETYPE_IP)
            {
                //refIdx[fieldId][1] = 0;        // already zero
                //outRefTask[fieldId][1] = NULL; // No forward ref for P
            }
            else
            {
                mfxU8 const *l1_instance = eTask->m_list1[fid].Begin() + l1_idx;
                iTask *L1_ref_task = m_inputTasks->GetTaskByFrameOrder(eTask->m_dpb[fid][*l1_instance & 127].m_frameOrder);
                MSDK_CHECK_POINTER(L1_ref_task, MFX_ERR_NULL_PTR);

                refIdx[fieldId][1] = l1_idx;
                outRefTask[fieldId][1] = L1_ref_task;
                ref_fid[fieldId][1] = (*l1_instance > 127) ? 1 : 0; // for bff ? 0 : 1; (but now preenc supports only tff)

                stsL1 = MFX_ERR_NONE;
            }
        }
        else {
            if (eTask->m_list1[fid].Size())
                stsL1 = MFX_WRN_OUT_OF_RANGE;
        }
    }

    return stsL1 != stsL0 ? MFX_ERR_NONE : stsL0;
}

void H264FEIEncoderPreenc::GetRefInfo(
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
#if 0
    for (mfxU32 i = 0; i < m_InitExtParams.size(); ++i)
    {
        switch (m_InitExtParams[i]->BufferId)
        {
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
}

mfxStatus H264FEIEncoderPreenc::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_preenc->GetVideoParam(par);
    return sts;
}
#endif //MSDK_AVC_FEI
