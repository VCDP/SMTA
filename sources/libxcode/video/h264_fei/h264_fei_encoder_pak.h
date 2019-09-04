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

#ifndef FEI_ENCODER_PAK_H
#define FEI_ENCODER_PAK_H

#include "encoding_task_pool.h"
#include "../msdk_codec.h"


class H264FEIEncoderPAK
{
public:
    DECLARE_MLOGINSTANCE();
    H264FEIEncoderPAK(MFXVideoSession *session,
                  iTaskPool *task_pool,
                  bufList* ext_buf,
                  bufList* enc_ext_buf,
                  bool UseOpaqueMemory,
                  PipelineCfg* cfg,
                  mfxEncodeCtrl* encodeCtrl,
                  EncExtParams* extParams);
    ~H264FEIEncoderPAK();
    bool Init(ElementCfg *ele_param);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus InitPak(MediaBuf &buf);
    mfxStatus ProcessOneFrame(iTask* eTask, RefInfo* refInfo);
    mfxStatus GetVideoParam(mfxVideoParam *par);
    mfxVideoParam* GetCommonVideoParams() { return &m_pakParams;}
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
    H264FEIEncoderPAK(const H264FEIEncoderPAK& encode);
    H264FEIEncoderPAK& operator= (const H264FEIEncoderPAK& encode);

    mfxStatus AllocateSufficientBuffer();
    mfxStatus InitFrameParams(iTask* eTask);
    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream,Stream *out_stream);

    MFXVideoSession *m_session;
    MFXVideoPAK *m_pak;
    std::vector<mfxExtBuffer *> m_PakExtParams;
    mfxVideoParam m_pakParams;

    mfxExtCodingOption2 m_codingOpt2;
    mfxExtFeiSPS m_feiSPS;
    mfxExtFeiPPS m_feiPPS;
    mfxExtFeiParam m_ExtBufInit;
    EncExtParams* m_extParams;

    mfxBitstream* m_bitstream;
    Stream* m_outputStream;
    Stream* m_pMvoutStream;
    Stream* m_pMBcodeoutStream;

    bool m_bUseOpaqueMemory;
    bufList *m_extBufs;
    bufList *m_encodeBufs;
    PipelineCfg *m_pipelineCfg;
    iTaskPool *m_inputTasks;
    RefInfo* m_RefInfo;
};
#endif //FEI_ENCODER_PAK_H
#endif //MSDK_AVC_FEI
