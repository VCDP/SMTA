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

#ifndef ENCODER_ELEMENT_H
#define ENCODER_ELEMENT_H

#include <map>
#include <stdio.h>
#include <vector>
#include "msdk_codec.h"

class EncoderElement: public BaseEncoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    EncoderElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~EncoderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual int MarkLTR();
    // set ref list
    virtual int MarkRL(unsigned int refNum = 0);
    virtual void SetForceKeyFrame();
    virtual void SetResetBitrateFlag();
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);

protected:
    //BaseElement
    virtual int HandleProcess();
    //virtual int Recycle(MediaBuf &buf);
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

private:
    EncoderElement(const EncoderElement &);
    EncoderElement &operator=(const EncoderElement &);

    mfxStatus InitEncoder(MediaBuf&);
    int ProcessChainEncode(MediaBuf&);
    int HandleProcessEncode();
    void OnFrameEncoded(mfxBitstream *pBs);
    void OnMarkLTR();
    void OnMarkRL();
    void InitEncoderROI(mfxExtEncoderROI *pROI);
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    mfxStatus AddExtRoiBufferToCtrl();
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022

    MFXVideoENCODE *mfx_enc_;
    mfxBitstream output_bs_;
    //mfxFrameAllocResponse m_mfxEncResponse;  // memory allocation response for encode
    std::vector<mfxExtBuffer *> m_EncExtParams; //Used during init
    std::vector<mfxExtBuffer *> m_CtrlExtParams; //Used during EncodeFrameAsync

    mfxExtCodingOption coding_opt;
    mfxExtCodingOption2 coding_opt2;
    mfxExtCodingOption3 coding_opt3;
    bool force_key_frame_;
    bool reset_bitrate_flag_;
    bool reset_res_flag_;
    mfxEncodeCtrl enc_ctrl_;
    mfxU32 m_nFramesProcessed;
    mfxU32 m_nLastForceKeyFrame;
    mfxExtAVCRefListCtrl m_avcRefList;
    bool m_bMarkLTR;
    mfxExtAVCRefLists m_avcrefLists;
    mfxU16 m_refNum;

    // encoder ROI list
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    std::map<int, mfxExtEncoderROI> m_roiData;
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#if (MFX_VERSION >= 1025)
        // MFE mode and number of frames
        mfxExtMultiFrameParam    m_ExtMFEParam;
        // here we pass general timeout per session.
        mfxExtMultiFrameControl  m_ExtMFEControl;
#endif

    EncExtParams extParams;
    //Measurement  *measurement;
    static unsigned int  mNumOfEncChannels;
};
#endif //ENCODER_ELEMENT_H
