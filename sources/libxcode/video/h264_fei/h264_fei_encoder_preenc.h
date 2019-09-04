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

#ifdef MSDK_AVC_FEI

#ifndef FEI_ENCODER_PREENC_H
#define FEI_ENCODER_PREENC_H

#include "encoding_task_pool.h"
#include "../msdk_codec.h"


class H264FEIEncoderPreenc
{
public:
    DECLARE_MLOGINSTANCE();
    H264FEIEncoderPreenc(MFXVideoSession *session,
                     iTaskPool *task_pool,
                     bufList* ext_buf,
                     bufList* enc_ext_buf,
                     bool UseOpaqueMemory,
                     PipelineCfg* cfg,
                     EncExtParams* extParams);
    ~H264FEIEncoderPreenc();
    bool Init(ElementCfg *ele_param);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus InitPreenc(MediaBuf &buf, mfxFrameAllocRequest *request);
    mfxVideoParam* GetCommonVideoParams() { return &m_fullResParams;}
    mfxStatus ProcessChainPreenc(iTask* eTask);
    mfxStatus GetVideoParam(mfxVideoParam *par);
    mfxStatus FlushOutput(iTask* eTask);

    void GetRefInfo(mfxU16 & picStruct,
                    mfxU16 & refDist,
                    mfxU16 & numRefFrame,
                    mfxU16 & gopSize,
                    mfxU16 & gopOptFlag,
                    mfxU16 & idrInterval,
                    mfxU16 & numRefActiveP,
                    mfxU16 & numRefActiveBL0,
                    mfxU16 & numRefActiveBL1,
                    mfxU16 & bRefType,
                    bool   & bSigleFieldProcessing);

    mfxVideoParam m_DSParams;
private:
    H264FEIEncoderPreenc(const H264FEIEncoderPreenc& encode);
    H264FEIEncoderPreenc& operator= (const H264FEIEncoderPreenc& encode);

    mfxStatus InitDS(MediaBuf &buf, mfxFrameAllocRequest *request);
    mfxStatus DownSampleInput(iTask* eTask);
    mfxStatus RepackPredictors(iTask* eTask);
    mfxStatus InitPreencFrameParams(iTask* eTask, iTask* refTask[2][2], mfxU8 ref_fid[2][2], bool isDownsamplingNeeded);
    mfxStatus ProcessMultiPreenc(iTask* eTask);
    mfxStatus GetRefTaskEx(iTask *eTask, mfxU8 l0_idx, mfxU8 l1_idx, mfxU8 refIdx[2][2], mfxU8 ref_fid[2][2], iTask *outRefTask[2][2]);


    MFXVideoSession *m_session;
    PipelineCfg *m_pipelineCfg;
    MFXVideoENC *m_preenc;
    MFXVideoVPP *m_ds;
    iTaskPool *m_inputTasks;
    // sets of extended buffers for PreENC and ENCODE(ENC+PAK)
    bufList *m_extBufs;
    bufList *m_encodeBufs;
    mfxVideoParam m_preencParams;
    mfxVideoParam m_fullResParams;
    std::vector<mfxExtBuffer *> m_PreencExtParams;
    std::vector<mfxExtBuffer *> m_DSExtParams;
    // configure and attach external parameters for DS
    mfxExtVPPDoNotUse extDontUse;
    bool m_bMVout;
    Stream* m_pMvoutStream;
    Stream* m_pMvinStream;
    mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB m_tmpMVMB;
    EncExtParams* m_extParams;

    //DownSampling list
    mfxU8 m_DSstrength;
    // memory allocation response for VPP input
    mfxFrameAllocResponse m_DSResponse;
    bool m_bUseOpaqueMemory;
};
#endif //FEI_ENCODER_PREENC_H
#endif //MSDK_AVC_FEI
