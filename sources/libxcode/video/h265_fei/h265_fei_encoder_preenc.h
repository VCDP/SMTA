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
#ifdef MSDK_HEVC_FEI

#ifndef H265_FEI_ENCODER_PREENC_H
#define H265_FEI_ENCODER_PREENC_H

#include "../msdk_codec.h"
#include "sample_hevc_fei_defs.h"
#include "h265_fei_utils.h"


class H265FEIEncoderPreenc
{
public:
    DECLARE_MLOGINSTANCE();
    H265FEIEncoderPreenc(MFXVideoSession *session,
                    //  iTaskPool *task_pool,
                    //  bufList* ext_buf,
                    //  bufList* enc_ext_buf,
                    MFXFrameAllocator* allocator,
                     bool UseOpaqueMemory,
                     //PipelineCfg* cfg,
                     const mfxExtFeiPreEncCtrl& preencCtrl,
                     EncExtParams* extParams);
    ~H265FEIEncoderPreenc();
    bool Init(ElementCfg *ele_param);
    mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    mfxStatus InitPreenc(MediaBuf &buf);
    mfxStatus ProcessChainPreenc(HevcTask* task);
    mfxStatus GetVideoParam(mfxVideoParam *par);
    MfxVideoParamsWrapper GetVideoParams();

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

    //mfxVideoParam m_DSParams;
private:
    H265FEIEncoderPreenc(const H265FEIEncoderPreenc& encode);
    H265FEIEncoderPreenc& operator= (const H265FEIEncoderPreenc& encode);

    mfxStatus InitDS(MediaBuf &buf, mfxFrameAllocRequest *request);
    mfxStatus ProcessMultiPreenc(HevcTask* task);
    mfxStatus ResetExtBuffers(const mfxVideoParam & videoParams);
    mfxStatus DownSampleFrame(HevcTask* task);
    mfxStatus DumpResult(HevcTask* task);
    mfxU16 GetCurPicType(mfxU32 fieldId);


    MFXVideoSession *m_session;
    //PipelineCfg *m_pipelineCfg;
    mfxExtFeiPreEncCtrl m_frameCtrl;
    MFXVideoENC *m_preenc;
    MFXVideoVPP *m_ds;
    //iTaskPool *m_inputTasks;
    SurfacesPool m_DSSurfacePool;
    mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB m_default_MVMB;

    std::vector<mfxExtFeiPreEncMVExtended>     m_mvs;
    std::vector<mfxExtFeiPreEncMBStatExtended> m_mbs;
    mfxVideoParam m_preencParams;
    mfxVideoParam m_DSParams;
    std::vector<mfxExtBuffer *> m_PreencExtParams;
    std::vector<mfxExtBuffer *> m_DSExtParams;

    bool m_bMVout;
    Stream* m_pMvoutStream;
    Stream* m_pMvinStream;
    mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB m_tmpMVMB;
    EncExtParams* m_extParams;
    typedef std::pair<mfxSyncPoint, std::pair<mfxENCInputWrap, mfxENCOutputWrap> > SyncPair;
    SyncPair m_syncp;
    mfxU16 m_numOfFields;


    //DownSampling list
    mfxU8 m_DSstrength;
    // memory allocation response for VPP input
    mfxFrameAllocResponse m_DSResponse;
    bool m_bUseOpaqueMemory;
    bool m_isMVoutFormatted;
};
#endif //H265_FEI_ENCODER_PREENC_H
#endif //MSDK_HEVC_FEI
