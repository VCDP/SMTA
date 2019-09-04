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

#ifndef FEI_ENCODER_ENCODE_H
#define FEI_ENCODER_ENCODE_H

#include "encoding_task_pool.h"
#include "../msdk_codec.h"
#include "predictors_repacking.h"

class H264FEIEncoderEncode
{
public:
    DECLARE_MLOGINSTANCE();
    H264FEIEncoderEncode(MFXVideoSession *session,
                     bufList* ext_buf,
                     bufList* enc_ext_buf,
                     bool UseOpaqueMemory,
                     PipelineCfg* cfg,
                     mfxEncodeCtrl* encodeCtrl,
                     EncExtParams* extParams);
    ~H264FEIEncoderEncode();
    bool Init(ElementCfg *ele_param);
    mfxStatus ProcessChainEncode(iTask* eTask, bool is_eos);
    mfxStatus InitEncode(MediaBuf &buf);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus GetVideoParam(mfxVideoParam *par);
    mfxVideoParam* GetCommonVideoParams() { return &m_encodeParams;}
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
    H264FEIEncoderEncode(const H264FEIEncoderEncode& encode);
    H264FEIEncoderEncode& operator= (const H264FEIEncoderEncode& encode);

    mfxStatus AllocateSufficientBuffer();
    mfxStatus InitEncodeFrameParams(mfxFrameSurface1 * encodeSurface,PairU8 frameType,iTask * eTask);
    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, Stream *out_stream);

    MFXVideoSession* m_session;
    PipelineCfg *m_pipelineCfg;
    Stream *m_outputStream;
    Stream* m_pMvoutStream;
    Stream* m_pMvinStream;
    Stream* m_pMBcodeoutStream;
    Stream* m_pWeightStream;
    // sets of extended buffers for PreENC and ENCODE(ENC+PAK)
    bufList *m_extBufs;
    bufList *m_encodeBufs;
    MFXVideoENCODE *m_encode;
    mfxEncodeCtrl* m_encodeCtrl;
    EncExtParams* m_extParams;
    mfxVideoParam m_encodeParams;
    mfxBitstream output_bs_;
    /* Temporary memory to speed up computations */
    std::vector<mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB> m_tmpForReading;
    std::vector<mfxExtBuffer *> m_EncodeExtParams;

    mfxExtCodingOption3 m_codingOpt3;
    mfxExtCodingOption2 m_codingOpt2;
    mfxExtOpaqueSurfaceAlloc m_extOpaqueAlloc;
    mfxExtFeiParam  m_extFEIParam;
#if (MFX_VERSION >= 1025)
    mfxExtMultiFrameParam m_multiFrame;
    mfxExtMultiFrameControl m_multiFrameControl;
    mfxU16 m_nMFEFrames;
    mfxU16 m_nMFEMode;
    mfxU32 m_nMFETimeout;
#endif
    bool m_bUseOpaqueMemory;
    bool reset_res_flag_;
};
#endif //FEI_ENCODER_ENCODE_H
#endif //MSDK_AVC_FEI
