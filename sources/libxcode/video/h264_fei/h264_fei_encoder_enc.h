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

#ifndef FEI_ENCODER_ENC_H
#define FEI_ENCODER_ENC_H

#include "encoding_task_pool.h"
#include "../msdk_codec.h"
#include "predictors_repacking.h"

class H264FEIEncoderENC
{
public:
    DECLARE_MLOGINSTANCE();
    H264FEIEncoderENC(MFXVideoSession *session,
                  iTaskPool *task_pool,
                  bufList* ext_buf,
                  bufList* enc_ext_buf,
                  bool UseOpaqueMemory,
                  PipelineCfg* cfg,
                  mfxEncodeCtrl* encodeCtrl,
                  EncExtParams* extParams);
    ~H264FEIEncoderENC();
    bool Init(ElementCfg *ele_param);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus InitEnc(MediaBuf &buf);
    mfxStatus ProcessOneFrame(iTask* eTask, RefInfo* refInfo);
    mfxVideoParam* GetCommonVideoParams() { return &m_encParams;}
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

private:
    H264FEIEncoderENC(const H264FEIEncoderENC& encode);
    H264FEIEncoderENC& operator= (const H264FEIEncoderENC& encode);
    //mfxStatus AllocateSufficientBuffer();
    mfxStatus InitFrameParams(iTask* eTask);

    MFXVideoSession *m_session;
    MFXVideoENC *m_enc;
    std::vector<mfxExtBuffer *> m_EncExtParams;
    std::vector<mfxExtBuffer *> m_tmpPakExtParams;
    /* Temporary memory to speed up computations */
    std::vector<mfxI16> m_tmpForMedian;
    std::vector<mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB> m_tmpForReading;
    mfxVideoParam m_encParams;
    mfxVideoParam m_tmpPakParams;
    //mfxExtFeiParam  m_ext_fei_param_;

    mfxExtCodingOption2 m_codingOpt2;
    mfxExtFeiSPS m_feiSPS;
    mfxExtFeiPPS m_feiPPS;
    mfxExtFeiParam m_ExtBufInit;
    mfxExtFeiParam m_tmpExtBufInit;

    bool m_bUseOpaqueMemory;
    bufList *m_extBufs;
    bufList *m_encodeBufs;
    mfxEncodeCtrl* m_encodeCtrl;
    EncExtParams* m_extParams;
    PipelineCfg *m_pipelineCfg;
    iTaskPool *m_inputTasks;
    RefInfo* m_RefInfo;
    Stream* m_pMvoutStream;
    Stream* m_pMBcodeoutStream;
    Stream* m_pMvinStream;

};
#endif //FEI_ENCODER_ENC_H
#endif //MSDK_AVC_FEI
