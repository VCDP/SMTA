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

#include "h264_fei_encoder_pak.h"
#ifdef MSDK_AVC_FEI
#include "element_common.h"

DEFINE_MLOGINSTANCE_CLASS(H264FEIEncoderPAK, "H264FEIEncoderPAK");

H264FEIEncoderPAK::H264FEIEncoderPAK(MFXVideoSession *session,
                             iTaskPool *task_pool,
                             bufList* ext_buf,
                             bufList* enc_ext_buf,
                             bool UseOpaqueMemory,
                             PipelineCfg* cfg,
                             mfxEncodeCtrl* encodeCtrl,
                             EncExtParams* extParams)
     :m_session(session),
      m_extParams(extParams),
      m_bUseOpaqueMemory(UseOpaqueMemory),
      m_extBufs(ext_buf),
      m_encodeBufs(enc_ext_buf),
      m_pipelineCfg(cfg),
      m_inputTasks(task_pool)
{
    memset(&m_pakParams, 0, sizeof(mfxVideoParam));
    memset(&m_codingOpt2, 0, sizeof(mfxExtCodingOption2));
    memset(&m_feiSPS, 0, sizeof(mfxExtFeiSPS));
    memset(&m_feiPPS, 0, sizeof(mfxExtFeiPPS));

    memset(&m_ExtBufInit, 0, sizeof(mfxExtFeiParam));

    m_PakExtParams.reserve(4);
    m_pak = NULL;
    m_bitstream = NULL;
    m_outputStream = NULL;
    m_RefInfo = NULL;
    m_pMvoutStream = NULL;
    m_pMBcodeoutStream = NULL;
}
H264FEIEncoderPAK::~H264FEIEncoderPAK()
{
    if (m_pak) {
        m_pak->Close();
        delete m_pak;
        m_pak = NULL;
    }
}

bool H264FEIEncoderPAK::Init(ElementCfg *ele_param)
{
    m_outputStream = ele_param->output_stream;

    m_pak = new MFXVideoPAK(*m_session);

    /*PREENC video parameters*/
    m_pakParams.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
    m_pakParams.mfx.TargetUsage = 0;
    m_pakParams.mfx.TargetKbps = 0;
    m_pakParams.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
    m_pakParams.mfx.QPI = ele_param->EncParams.mfx.QPI;
    m_pakParams.mfx.QPP = ele_param->EncParams.mfx.QPP;
    m_pakParams.mfx.QPB = ele_param->EncParams.mfx.QPB;

    // binary flag, 0 signals encoder to take frames in display order. PREENC, ENCPAK, ENC, PAK interfaces works only in encoded order
    m_pakParams.mfx.EncodedOrder = 1;
    m_pakParams.AsyncDepth = 1;
    m_pakParams.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist==0? 4: ele_param->EncParams.mfx.GopRefDist;
    m_pakParams.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize==0? 30: ele_param->EncParams.mfx.GopPicSize;
    m_pakParams.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
    m_pakParams.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
    m_pakParams.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
    m_pakParams.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    //NumRefFrame is used to define the size of iTask_pool, so should be defined by GopRefDist
    m_pakParams.mfx.NumRefFrame = (m_pakParams.mfx.GopRefDist+1) > 4? 4: (m_pakParams.mfx.GopRefDist+1);
    m_pakParams.mfx.GopOptFlag = ele_param->EncParams.mfx.GopOptFlag;

    m_codingOpt2.BRefType = ele_param->extParams.bRefType;

    m_feiSPS.Log2MaxPicOrderCntLsb = 5;//GetDefaultLog2MaxPicOrdCnt(m_pakParams.mfx.GopRefDist, AdjustBrefType(ele_param->extParams.bRefType, m_pakParams));

    m_pMvoutStream = ele_param->extParams.mvoutStream;
    m_pMBcodeoutStream =  ele_param->extParams.mbcodeoutStream;

    return true;
}

mfxStatus H264FEIEncoderPAK::QueryIOSurf(mfxFrameAllocRequest* request)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_pak->QueryIOSurf(&m_pakParams, request);
    MLOG_INFO("FEI PAK: QueryIOSurf suggest number %u\n", request->NumFrameSuggested)
    if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("FEI PAK QueryIOSurf failed. sts = %d\n", sts);
    }
    return sts;
}

mfxStatus H264FEIEncoderPAK::InitPak(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    // initialize PAK
    m_pakParams.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    m_pakParams.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    m_pakParams.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    m_pakParams.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    m_pakParams.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    m_pakParams.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
    m_pakParams.mfx.FrameInfo.Width         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width;
    m_pakParams.mfx.FrameInfo.Height        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height;

    m_pakParams.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    m_pakParams.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;
    m_pakParams.mfx.FrameInfo.BitDepthLuma  = 8;
    m_pakParams.mfx.FrameInfo.BitDepthChroma = 8;

    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        m_pakParams.mfx.FrameInfo.PicStruct = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        m_pakParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }

    mfxU16 num_ref_limit = (m_extParams->bPAK && !m_extParams->bPreEnc && !m_extParams->bEncode && !m_extParams->bENC) ?
                                 16 : ((m_pakParams.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2);

    if(m_extParams->fei_ctrl.nRefActiveBL1 > num_ref_limit)
        m_extParams->fei_ctrl.nRefActiveBL1 = num_ref_limit;

    if (m_bUseOpaqueMemory) {
        MLOG_INFO("opaque memory");
        m_pakParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
    } else {
        MLOG_INFO("video memory");
        m_pakParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }


    m_codingOpt2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    m_codingOpt2.Header.BufferSz = sizeof(mfxExtCodingOption2);

    mfxU16 num_fields = (m_pakParams.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) ? 1 : 2;

    /* Create SPS Ext Buffer for Init */
    m_feiSPS.Header.BufferId = MFX_EXTBUFF_FEI_SPS;
    m_feiSPS.Header.BufferSz = sizeof(mfxExtFeiSPS);
    m_feiSPS.SPSId                 = 0;
    m_feiSPS.PicOrderCntType       = (num_fields == 2 || m_pakParams.mfx.GopRefDist > 1) ? 0 : 2;
    //m_feiSPS.Log2MaxPicOrderCntLsb = GetDefaultLog2MaxPicOrdCnt(m_pakParams.mfx.GopRefDist, AdjustBrefType(m_pAppConfig->bRefType, m_pakParams));

    /* Create PPS Ext Buffer for Init */
    m_feiPPS.Header.BufferId = MFX_EXTBUFF_FEI_PPS;
    m_feiPPS.Header.BufferSz = sizeof(mfxExtFeiPPS);

    m_feiPPS.SPSId                     = 0;
    m_feiPPS.PPSId                     = 0;

    /* PicInitQP should be always 26 !!!
    * Adjusting of QP parameter should be done via Slice header */
    m_feiPPS.PicInitQP                 = 26;
    m_feiPPS.NumRefIdxL0Active         = 1;
    m_feiPPS.NumRefIdxL1Active         = 1;
    m_feiPPS.ChromaQPIndexOffset       = m_extParams->fei_ctrl.nChromaQPIndexOffset;
    m_feiPPS.SecondChromaQPIndexOffset = m_extParams->fei_ctrl.nSecondChromaQPIndexOffset;
    /*
    IntraPartMask description from manual
    This value specifies what block and sub-block partitions are enabled for intra MBs.
    0x01 - 16x16 is disabled
    0x02 - 8x8 is disabled
    0x04 - 4x4 is disabled

    So on, there is additional condition for Transform8x8ModeFlag:
    If partitions below 16x16 present Transform8x8ModeFlag flag should be ON
    * */
    m_feiPPS.Transform8x8ModeFlag      = ((m_extParams->fei_ctrl.nIntraPartMask & 0x06) != 0x06) ?
                                               1 :m_extParams->fei_ctrl.bTransform8x8ModeFlag;

    m_ExtBufInit.Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
    m_ExtBufInit.Header.BufferSz = sizeof(mfxExtFeiParam);
    m_ExtBufInit.Func = MFX_FEI_FUNCTION_PAK;

    m_ExtBufInit.SingleFieldProcessing = MFX_CODINGOPTION_OFF;
    m_PakExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_ExtBufInit));
    m_PakExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_codingOpt2));
    m_PakExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_feiSPS));
    m_PakExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&m_feiPPS));

    m_pakParams.ExtParam    = &m_PakExtParams[0];
    m_pakParams.NumExtParam = (mfxU16)m_PakExtParams.size();

    sts = m_pak->Init(&m_pakParams);
    if (sts > MFX_ERR_NONE) {
        MLOG_WARNING("\t\tPAK init warning %d\n", sts);
    } else if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\tPAK init return %d\n", sts);
    }

    return sts;
}

mfxStatus H264FEIEncoderPAK::InitFrameParams(iTask* eTask)
{
    mfxStatus sts = MFX_ERR_NONE;

    bool is_I_frame = ExtractFrameType(*eTask, eTask->m_fieldPicFlag) & MFX_FRAMETYPE_I;

    // Initialize controller for Extension buffers
    sts = eTask->ExtBuffersController.InitializeController(eTask->bufs, bufSetController::PAK, is_I_frame, !eTask->m_fieldPicFlag);
    if (sts < MFX_ERR_NONE)
        MLOG_ERROR("eTask->ExtBuffersController.InitializeController failed");

    eTask->PAK_in.NumFrameL0 = (mfxU16)m_RefInfo->reference_frames.size();
    eTask->PAK_in.NumFrameL1 = 0;
    eTask->PAK_in.L0Surface = eTask->PAK_in.NumFrameL0 ? &m_RefInfo->reference_frames[0] : NULL;
    eTask->PAK_in.L1Surface = NULL;

    // Update extension buffers

    // check whole ENCPAK buffers set, to update buffers in PAK only case
    std::vector<mfxExtBuffer*> active_encpak_buffers = eTask->bufs->PB_bufs.in.buffers;
    active_encpak_buffers.insert(active_encpak_buffers.end(),
                                 eTask->bufs->PB_bufs.out.buffers.begin(), eTask->bufs->PB_bufs.out.buffers.end());

    mfxExtFeiPPS*         feiPPS         = NULL;
    mfxExtFeiSliceHeader* feiSliceHeader = NULL;

    for (std::vector<mfxExtBuffer*>::iterator it = active_encpak_buffers.begin(); it != active_encpak_buffers.end(); ++it)
    {
        switch ((*it)->BufferId)
        {
        case MFX_EXTBUFF_FEI_PPS:
            if (!feiPPS){ feiPPS = reinterpret_cast<mfxExtFeiPPS*>(*it); }
            break;

        case MFX_EXTBUFF_FEI_SLICE:
            if (!feiSliceHeader){ feiSliceHeader = reinterpret_cast<mfxExtFeiSliceHeader*>(*it); }
            break;


        case MFX_EXTBUFF_FEI_ENC_MV:
            // In case of PAK only we have to read required buffers from disk
            if (!m_extParams->bENC && m_pMvoutStream)
            {

                mfxExtFeiEncMV* mvBuf = reinterpret_cast<mfxExtFeiEncMV*>(*it);
                if(m_pMvoutStream->ReadBlock(mvBuf->MB, sizeof(mvBuf->MB[0])*mvBuf->NumMBAlloc ) != sizeof(mvBuf->MB[0])*mvBuf->NumMBAlloc){
                     MLOG_ERROR("m_pMvoutStream: ReadBlock failed.\n");
                     return MFX_ERR_MORE_DATA;
                }
            }
            break;
        case MFX_EXTBUFF_FEI_PAK_CTRL:
            // In case of PAK only we have to read required buffers from disk
            if (!m_extParams->bENC && m_pMBcodeoutStream)
            {
                mfxExtFeiPakMBCtrl* mbcodeBuf = reinterpret_cast<mfxExtFeiPakMBCtrl*>(*it);
                if(m_pMBcodeoutStream->ReadBlock(mbcodeBuf->MB, sizeof(mbcodeBuf->MB[0])*mbcodeBuf->NumMBAlloc ) != sizeof(mbcodeBuf->MB[0])*mbcodeBuf->NumMBAlloc){
                     MLOG_ERROR("m_pMBcodeoutStream: ReadBlock failed.\n");
                     return MFX_ERR_MORE_DATA;
                }
            }
            break;

        } // switch ((*it)->BufferId)
    } // for (iterator it = active_encpak_buffers.begin(); it != active_encpak_buffers.end(); ++it

    // These buffers required for ENC & PAK
    MSDK_CHECK_POINTER(feiPPS && feiSliceHeader, MFX_ERR_NULL_PTR);

    /* PPS, SliceHeader processing */
#if MFX_VERSION >= 1023
    for (mfxU32 fieldId = 0; fieldId < mfxU32(eTask->m_fieldPicFlag ? 2 : 1); fieldId++)
    {

        feiPPS[fieldId].FrameType   = ExtractFrameType(*eTask, fieldId);
        feiPPS[fieldId].PictureType = mfxU16(!eTask->m_fieldPicFlag ? MFX_PICTYPE_FRAME :
                    (eTask->m_fid[fieldId] ? MFX_PICTYPE_BOTTOMFIELD : MFX_PICTYPE_TOPFIELD));

        memset(feiPPS[fieldId].DpbBefore, 0xffff, sizeof(feiPPS[fieldId].DpbBefore));

        std::vector<mfxExtFeiPPS::mfxExtFeiPpsDPB> & DPB_before_to_use = fieldId ? m_RefInfo->DPB_after : m_RefInfo->DPB_before;
        if (!DPB_before_to_use.empty())
        {
            memcpy(feiPPS[fieldId].DpbBefore, &DPB_before_to_use[0], DPB_before_to_use.size() * sizeof(DPB_before_to_use[0]));
        }

        memset(feiPPS[fieldId].DpbAfter, 0xffff, sizeof(feiPPS[fieldId].DpbAfter));
        if (!m_RefInfo->DPB_after.empty())
        {
            memcpy(feiPPS[fieldId].DpbAfter, &m_RefInfo->DPB_after[0], m_RefInfo->DPB_after.size() * sizeof(m_RefInfo->DPB_after[0]));
        }

        MSDK_CHECK_POINTER(feiSliceHeader[fieldId].Slice, MFX_ERR_NULL_PTR);

        for (mfxU32 i = 0; i < feiSliceHeader[fieldId].NumSlice; ++i)
        {
            MSDK_ZERO_ARRAY(feiSliceHeader[fieldId].Slice[i].RefL0, 32);
            MSDK_ZERO_ARRAY(feiSliceHeader[fieldId].Slice[i].RefL1, 32);

            feiSliceHeader[fieldId].Slice[i].SliceType         = FrameTypeToSliceType(ExtractFrameType(*eTask, fieldId));
            feiSliceHeader[fieldId].Slice[i].NumRefIdxL0Active = mfxU16(m_RefInfo->L0[fieldId].size());
            feiSliceHeader[fieldId].Slice[i].NumRefIdxL1Active = mfxU16(m_RefInfo->L1[fieldId].size());
            feiSliceHeader[fieldId].Slice[i].IdrPicId          = eTask->m_frameIdrCounter;

            if (feiSliceHeader[fieldId].Slice[i].NumRefIdxL0Active)
                memcpy(feiSliceHeader[fieldId].Slice[i].RefL0, &m_RefInfo->L0[fieldId][0], m_RefInfo->L0[fieldId].size() * sizeof(m_RefInfo->L0[fieldId][0]));

            if (feiSliceHeader[fieldId].Slice[i].NumRefIdxL1Active)
                memcpy(feiSliceHeader[fieldId].Slice[i].RefL1, &m_RefInfo->L1[fieldId][0], m_RefInfo->L1[fieldId].size() * sizeof(m_RefInfo->L1[fieldId][0]));
        }
    }
#else
    for (mfxU32 fieldId = 0; fieldId < mfxU32(eTask->m_fieldPicFlag ? 2 : 1); fieldId++)
    {
        feiPPS[fieldId].PictureType = ExtractFrameType(*eTask, fieldId);

        memset(&feiPPS[fieldId].ReferenceFrames, 0xffff, 16 * sizeof(mfxU16));
        memcpy(&feiPPS[fieldId].ReferenceFrames, &m_RefInfo->state[fieldId].dpb_idx[0], sizeof(mfxU16)*m_RefInfo->state[fieldId].dpb_idx.size());


        MSDK_CHECK_POINTER(feiSliceHeader[fieldId].Slice, MFX_ERR_NULL_PTR);

        for (mfxU32 i = 0; i < feiSliceHeader[fieldId].NumSlice; i++)
        {
            MSDK_ZERO_ARRAY(feiSliceHeader[fieldId].Slice[i].RefL0, 32);
            MSDK_ZERO_ARRAY(feiSliceHeader[fieldId].Slice[i].RefL1, 32);

            feiSliceHeader[fieldId].Slice[i].SliceType         = FrameTypeToSliceType(ExtractFrameType(*eTask, fieldId));
            feiSliceHeader[fieldId].Slice[i].NumRefIdxL0Active = (mfxU16)m_RefInfo->state[fieldId].l0_idx.size();
            feiSliceHeader[fieldId].Slice[i].NumRefIdxL1Active = (mfxU16)m_RefInfo->state[fieldId].l1_idx.size();
            feiSliceHeader[fieldId].Slice[i].IdrPicId          = eTask->m_frameIdrCounter;

            for (mfxU32 k = 0; k < m_RefInfo->state[fieldId].l0_idx.size(); k++)
            {
                feiSliceHeader[fieldId].Slice[i].RefL0[k].Index       = m_RefInfo->state[fieldId].l0_idx[k];
                feiSliceHeader[fieldId].Slice[i].RefL0[k].PictureType = (mfxU16)(!eTask->m_fieldPicFlag ? MFX_PICTYPE_FRAME :
                    (m_RefInfo->state[fieldId].l0_parity[k] ? MFX_PICTYPE_BOTTOMFIELD : MFX_PICTYPE_TOPFIELD));
            }
            for (mfxU32 k = 0; k < m_RefInfo->state[fieldId].l1_idx.size(); k++)
            {
                feiSliceHeader[fieldId].Slice[i].RefL1[k].Index       = m_RefInfo->state[fieldId].l1_idx[k];
                feiSliceHeader[fieldId].Slice[i].RefL1[k].PictureType = (mfxU16)(!eTask->m_fieldPicFlag ? MFX_PICTYPE_FRAME :
                    (m_RefInfo->state[fieldId].l1_parity[k] ? MFX_PICTYPE_BOTTOMFIELD : MFX_PICTYPE_TOPFIELD));
            }
        }
    }
#endif // MSDK_API >= 1023

    return sts;
}

mfxStatus H264FEIEncoderPAK::AllocateSufficientBuffer()
{
    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);

    // find out the required buffer size
    MSDK_CHECK_POINTER(m_pak, MFX_ERR_NULL_PTR);
    mfxStatus sts = m_pak->GetVideoParam(&par);
    if (sts < MFX_ERR_NONE)
        MLOG_ERROR("m_pak->GetVideoParam failed");

    // reallocate bigger buffer for output
    sts = ExtendMfxBitstream(m_bitstream, par.mfx.BufferSizeInKB * 1000);
    if (sts < MFX_ERR_NONE) {
        WipeMfxBitstream(m_bitstream);
        MLOG_ERROR("ExtendMfxBitstream failed.");
    }

    return sts;
}

mfxStatus H264FEIEncoderPAK::ProcessOneFrame(iTask* eTask, RefInfo* refInfo)
{
    MLOG_DEBUG("H264FEIEncoderPAK::ProcessOneFrame");
    MSDK_CHECK_POINTER(eTask, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(refInfo, MFX_ERR_NULL_PTR);
    m_RefInfo = refInfo;

    mfxSyncPoint     m_SyncPoint = 0;
    mfxStatus sts = InitFrameParams(eTask);
    if (sts < MFX_ERR_NONE)
        MLOG_ERROR("FEI PAK: InitFrameParams failed sts = %d", sts);

    for (;;)
    {
        MLOG_DEBUG("frame: %d  t:%d %d : submit ", eTask->m_frameOrder, eTask->m_type[eTask->m_fid[0]], eTask->m_type[eTask->m_fid[1]]);

        for (int i = 0; i < 1; ++i)
        {
            // Attach extension buffers for current field
            std::vector<mfxExtBuffer *> * in_buffers = eTask->ExtBuffersController.GetBuffers(bufSetController::PAK, i, true);
            MSDK_CHECK_POINTER(in_buffers, MFX_ERR_NULL_PTR);

#if MFX_VERSION < 1023
            std::vector<mfxExtBuffer *> * out_buffers = eTask->ExtBuffersController.GetBuffers(bufSetController::PAK, i, false);
            MSDK_CHECK_POINTER(out_buffers, MFX_ERR_NULL_PTR);
#endif // MFX_VERSION < 1023

            eTask->PAK_in.NumExtParam  = mfxU16(in_buffers->size());
            eTask->PAK_in.ExtParam     = !in_buffers->empty() ? in_buffers->data() : NULL;

#if MFX_VERSION < 1023
            eTask->PAK_out.NumExtParam = mfxU16(out_buffers->size());
            eTask->PAK_out.ExtParam    = !out_buffers->empty() ? out_buffers->data() : NULL;;
#endif // MFX_VERSION < 1023

            sts = m_pak->ProcessFrameAsync(&eTask->PAK_in, &eTask->PAK_out, &m_SyncPoint);
            if (sts == MFX_ERR_GPU_HANG)
            {
                return MFX_ERR_GPU_HANG;
            }
            MSDK_BREAK_ON_ERROR(sts);

            sts = m_session->SyncOperation(m_SyncPoint, 60000);
            if (sts == MFX_ERR_GPU_HANG)
            {
                return MFX_ERR_GPU_HANG;
            }
            MSDK_BREAK_ON_ERROR(sts);
            mdprintf(stderr, "synced : %d\n", sts);
        }

        if (MFX_ERR_NONE < sts && !m_SyncPoint) // repeat the call if warning and no output
        {
            if (MFX_WRN_DEVICE_BUSY == sts){
                WaitForDeviceToBecomeFree(*m_session, m_SyncPoint, sts);
            }
        }
        else if (MFX_ERR_NONE < sts && m_SyncPoint)
        {
            sts = MFX_ERR_NONE; // ignore warnings if output is available
            break;
        }
        else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
        {
            sts = AllocateSufficientBuffer();
            if (sts < MFX_ERR_NONE)
                MLOG_ERROR("AllocateSufficientBuffer failed");
        }
        else
        {
            break;
        }
    }

    if (sts < MFX_ERR_NONE)
        MLOG_ERROR("FEI PAK failed to encode frame. sts=%d", sts);

    if (m_outputStream) {
        sts = WriteBitStreamFrame(eTask->PAK_out.Bs, m_outputStream);
    }

    if (sts < MFX_ERR_NONE)
        MLOG_ERROR("FEI ENCODE: WriteNextFrame failed");

    return sts;
}

void H264FEIEncoderPAK::GetRefInfo(
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
    std::vector<mfxExtBuffer*> & InitExtParams = m_PakExtParams;

    for (mfxU32 i = 0; i < InitExtParams.size(); ++i)
    {
        switch (InitExtParams[i]->BufferId)
        {
        case MFX_EXTBUFF_CODING_OPTION2:
        {
            mfxExtCodingOption2* ptr = reinterpret_cast<mfxExtCodingOption2*>(InitExtParams[i]);
            bRefType = ptr->BRefType;
        }
        break;

        case MFX_EXTBUFF_CODING_OPTION3:
        {
            mfxExtCodingOption3* ptr = reinterpret_cast<mfxExtCodingOption3*>(InitExtParams[i]);
            numRefActiveP   = ptr->NumRefActiveP[0];
            numRefActiveBL0 = ptr->NumRefActiveBL0[0];
            numRefActiveBL1 = ptr->NumRefActiveBL1[0];
        }
        break;

        case MFX_EXTBUFF_FEI_PARAM:
        default:
            break;
        }
    }

    picStruct   = m_pakParams.mfx.FrameInfo.PicStruct;
    refDist     = m_pakParams.mfx.GopRefDist;
    numRefFrame = m_pakParams.mfx.NumRefFrame;
    gopSize     = m_pakParams.mfx.GopPicSize;
    gopOptFlag  = m_pakParams.mfx.GopOptFlag;
    idrInterval = m_pakParams.mfx.IdrInterval;
}

mfxStatus H264FEIEncoderPAK::WriteBitStreamFrame(mfxBitstream *pMfxBitstream,
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

mfxStatus H264FEIEncoderPAK::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_pak->GetVideoParam(par);
    return sts;
}
#endif // MSDK_AVC_FEI
