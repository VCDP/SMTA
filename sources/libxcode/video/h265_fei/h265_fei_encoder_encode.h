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
//#define MSDK_HEVC_FEI
#ifdef MSDK_HEVC_FEI

#ifndef H265_FEI_ENCODER_ENCODE_H
#define H265_FEI_ENCODER_ENCODE_H

#include "../msdk_codec.h"
#include "ref_list_manager.h"
#include "h265_fei_predictors_repacking.h"
#include "sample_hevc_fei_defs.h"
class H265FeiBufferAllocator;

class H265FEIEncoderEncode
{
public:
    DECLARE_MLOGINSTANCE();
    H265FEIEncoderEncode(MFXVideoSession *session,
                    //  bufList* ext_buf,
                    //  bufList* enc_ext_buf,
                     bool UseOpaqueMemory,
                     //PipelineCfg* cfg,
                     const mfxExtFeiHevcEncFrameCtrl &encodeCtrl,
                     EncExtParams* extParams);
    ~H265FEIEncoderEncode();
    bool Init(ElementCfg *ele_param);
    mfxStatus ProcessChainEncode(HevcTask* task, bool is_eos);
    mfxStatus ProcessChainEncode(mfxFrameSurface1 * surf, bool is_eos);
    mfxStatus InitEncode(MediaBuf &buf, PredictorsRepacking* repacker, bool order);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus GetVideoParam(mfxVideoParam *par);

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
    H265FEIEncoderEncode(const H265FEIEncoderEncode& encode);
    H265FEIEncoderEncode& operator= (const H265FEIEncoderEncode& encode);

    mfxStatus AllocateSufficientBuffer();
    //mfxStatus InitEncodeFrameParams(mfxFrameSurface1 * encodeSurface,PairU8 frameType,iTask * eTask);
    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, Stream *out_stream);
    mfxStatus ResetExtBuffers(const mfxVideoParam & videoParams);
    mfxStatus SetCtrlParams(const HevcTask& task);

    MFXVideoSession* m_session;
    //PipelineCfg *m_pipelineCfg;
    mfxExtFeiHevcEncFrameCtrl m_frameCtrl;
    Stream *m_outputStream;
    Stream* m_pMvoutStream;
    Stream* m_pMvinStream;
    Stream* m_pMBcodeoutStream;
    PredictorsRepacking* m_repacker;

    MFXVideoENCODE *m_encode;
    mfxEncodeCtrlWrap m_encodeCtrl;
    EncExtParams* m_extParams;
    mfxVideoParam m_encodeParams;
    mfxBitstream output_bs_;
    /* Temporary memory to speed up computations */
    std::vector<mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB> m_tmpForReading;
    std::vector<mfxExtBuffer *> m_EncodeExtParams;
    H265FeiBufferAllocator* m_buf_allocator;
    mfxU16 m_numOfFields;

    mfxExtCodingOption3 m_codingOpt3;
    mfxExtCodingOption2 m_codingOpt2;
    mfxExtCodingOption m_codingOpt;
    mfxExtOpaqueSurfaceAlloc m_extOpaqueAlloc;
    mfxExtFeiParam  m_extFEIParam;
    bool m_bUseOpaqueMemory;
    //bool reset_res_flag_;
};
#endif //H265_FEI_ENCODER_ENCODE_H
#endif //MSDK_HEVC_FEI
