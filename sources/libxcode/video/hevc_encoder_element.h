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

#ifndef HEVC_ENCODER_ELEMENT_H
#define HEVC_ENCODER_ELEMENT_H

#ifdef ENABLE_HEVC

#include <map>
#include <stdio.h>
#include <vector>
#include "msdk_codec.h"

class HevcEncoderElement : public BaseEncoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    HevcEncoderElement(ElementType type,MFXVideoSession *session,
            MFXFrameAllocator *pMFXAllocator = NULL,
            CodecEventCallback *callback = NULL);
    ~HevcEncoderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual void SetForceKeyFrame();
    virtual void SetResetBitrateFlag();

    void SetTiles(mfxU16 rows, mfxU16 cols);
    void SetRegion(mfxU32 id, mfxU16 type, mfxU16 encoding);
    void SetParam(mfxU16 pic_w_luma, mfxU16 pic_h_luma, mfxU64 flags);
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
protected:
    //BaseElement
    virtual int HandleProcess();
    //virtual int Recycle(MediaBuf &buf);
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

private:
    HevcEncoderElement(const HevcEncoderElement &);
    HevcEncoderElement &operator=(const HevcEncoderElement &);

    mfxStatus InitEncoder(MediaBuf&);
    int ProcessChainEncode(MediaBuf&);
    int HandleProcessEncode();
    void InitEncoderROI(mfxExtEncoderROI *pROI);

    MFXVideoENCODE *mfx_enc_;
    mfxBitstream output_bs_;
    std::vector<mfxExtBuffer *> m_EncExtParams;
    bool force_key_frame_;
    bool reset_bitrate_flag_;
    bool reset_res_flag_;
    mfxEncodeCtrl enc_ctrl_;
    mfxU32 m_nFramesProcessed;

    EncExtParams extParams;
    //Measurement  *measurement;
    static unsigned int  mNumOfEncChannels;

    //Extern params
    std::auto_ptr<mfxExtHEVCTiles>    tiles;
    std::auto_ptr<mfxExtHEVCRegion>   region;
    std::auto_ptr<mfxExtHEVCParam>    param;
};
#endif //#ifdef ENABLE_HEVC
#endif //HEVC_ENCODER_ELEMENT_H

