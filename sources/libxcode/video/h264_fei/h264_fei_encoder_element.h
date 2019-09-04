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

#ifndef FEI_ENCODER_ELEMENT_H
#define FEI_ENCODER_ELEMENT_H

#include <map>
#include <stdio.h>
#include <vector>
#include "../msdk_codec.h"

#include "mfxfei.h"
#include "encoding_task.h"
#include "encoding_task_pool.h"
#include "../../app/msdk_smta/sample_fei_defs.h"
#include "h264_fei_encoder_preenc.h"
#include "h264_fei_encoder_encode.h"
#include "h264_fei_encoder_enc.h"
#include "h264_fei_encoder_pak.h"

class H264FEIEncoderElement: public BaseEncoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    H264FEIEncoderElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~H264FEIEncoderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    virtual mfxStatus FillSurfacePool(mfxFrameSurface1* & surfacesPool, mfxFrameAllocResponse* allocResponse, mfxFrameInfo* FrameInfo);

protected:
    //BaseElement
    virtual int HandleProcess();
    virtual PairU8 GetFrameType(mfxU32 frameOrder);
    mfxU16 GetCurPicType(mfxU32 fieldId);

    bufList m_preencBufs, m_encodeBufs; // sets of extended buffers for PreENC and ENCODE(ENC+PAK)

private:
    H264FEIEncoderElement(const H264FEIEncoderElement &);
    H264FEIEncoderElement &operator=(const H264FEIEncoderElement &);

#if MFX_VERSION < 1023
    mfxStatus FillRefInfo(iTask* eTask);
#endif

    mfxStatus InitEncoder(MediaBuf&);
    //int ProcessChainEncode(MediaBuf&);
    int HandleProcessEncode();
    mfxStatus SetSequenceParameters();
    mfxStatus AllocExtBuffers();

    H264FEIEncoderPreenc *m_preenc;
    H264FEIEncoderEncode *m_encode;
    H264FEIEncoderENC *m_enc;
    H264FEIEncoderPAK *m_pak;
    MFXVideoSession *m_pPreencSession;

    bool reset_res_flag_;
    mfxEncodeCtrl m_encCtrl;
    mfxU32 m_nLastForceKeyFrame;

    mfxU16 m_picStruct;
    mfxU16 m_refDist;
    mfxU16 m_gopOptFlag;
    mfxU16 m_gopPicSize;
    mfxU16 m_idrInterval;
    mfxU16 m_numRefFrame;
    mfxU16 m_numSlice;
    mfxU16 m_width;
    mfxU16 m_height;

    mfxU16 m_numOfFields;
    mfxU16 m_maxQueueLength;
    mfxU16 m_widthMB;
    mfxU16 m_heightMB;

    PipelineCfg m_pipelineCfg;
    mfxBitstream m_bitstream;

    //DownSampling list
    mfxU8 m_DSstrength;
    mfxFrameAllocResponse m_dsResponse;
    // [0] - in, [1] - out
    mfxFrameAllocRequest m_DSRequest[2];

    // frames array for downscaled surfaces for PREENC
    ExtSurfPool m_DSSurfaces;

    // setting for Encode
    mfxFrameInfo m_commonFrameInfo;

    //for encoder
    mfxFrameAllocResponse m_reconResponse;
    ExtSurfPool m_reconSurfaces;
    //for ENC and PAK reference frames information
    RefInfo m_RefInfo;

    // holds all necessery data for task initializing
    iTaskParams m_taskInitializationParams;
    iTaskPool  m_inputTasks;
    mfxU32 m_frameCount;
    mfxU32 m_frameOrderIdrInDisplayOrder;
    PairU8 m_frameType;
    EncExtParams m_extParams;
    static unsigned int  mNumOfEncChannels;
};
PairU8 ExtendFrameType(mfxU32 type);
#endif //FEI_ENCODER_ELEMENT_H
#endif //#ifdef MSDK_AVC_FEI
