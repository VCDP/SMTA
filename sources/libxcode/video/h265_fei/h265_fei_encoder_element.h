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

#ifndef H265_FEI_ENCODER_ELEMENT_H
#define H265_FEI_ENCODER_ELEMENT_H

#include <map>
#include <stdio.h>
#include <vector>
#include "../msdk_codec.h"

#include "mfxfeihevc.h"

//#include "../../app/msdk_smta/sample_fei_defs.h"
#include "h265_fei_encoder_preenc.h"
#include "h265_fei_encoder_encode.h"
#include "ref_list_manager.h"


class H265FEIEncoderElement: public BaseEncoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    H265FEIEncoderElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~H265FEIEncoderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    //virtual mfxStatus FillSurfacePool(mfxFrameSurface1* & surfacesPool, mfxFrameAllocResponse* allocResponse, mfxFrameInfo* FrameInfo);

protected:
    //BaseElement
    virtual int HandleProcess();

private:
    H265FEIEncoderElement(const H265FEIEncoderElement &);
    H265FEIEncoderElement &operator=(const H265FEIEncoderElement &);

    mfxStatus InitEncoder(MediaBuf&);
    //int ProcessChainEncode(MediaBuf&);
    int HandleProcessEncode();
    mfxStatus SetSequenceParameters();
    mfxStatus InitExtBuffers(ElementCfg *ele_param);
    MfxVideoParamsWrapper GetVideoParams(MediaBuf&);

    H265FEIEncoderPreenc *m_preenc;
    H265FEIEncoderEncode *m_encode;
    EncodeOrderControl *m_pOrderCtrl;

    MFXVideoSession *m_pPreencSession;

    mfxExtFeiPreEncCtrl m_preencCtrl;
    mfxExtFeiHevcEncFrameCtrl m_encodeCtrl;
    mfxU32 m_nLastForceKeyFrame;
    mfxVideoParam m_videoParams;

    //DownSampling list
    mfxU8 m_DSstrength;

    mfxU32 m_frameCount;
    mfxU32 m_frameOrderIdrInDisplayOrder;
    EncExtParams m_extParams;
    std::auto_ptr<HEVCEncodeParamsChecker> m_pParamChecker;
    static unsigned int  mNumOfEncChannels;
};
#endif //H265_FEI_ENCODER_ELEMENT_H
#endif //#ifdef MSDK_HEVC_FEI
