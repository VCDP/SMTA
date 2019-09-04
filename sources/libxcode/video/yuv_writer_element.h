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
#ifdef ENABLE_RAW_CODEC
#ifndef YUV_WRITER_ELEMENT_H
#define YUV_WRITER_ELEMENT_H

#include <map>
#include <stdio.h>
#include <vector>
#include "msdk_codec.h"
#include "yuv_writer_plugin.h"

class YUVWriterElement: public BaseEncoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    YUVWriterElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~YUVWriterElement();

    virtual bool Init(void *cfg, ElementMode element_mode);
    //virtual void SetBitrate(unsigned short bitrate);

protected:
    //BaseElement
    virtual int HandleProcess();
    //virtual int Recycle(MediaBuf &buf){return 0;};
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

private:
    YUVWriterElement(const YUVWriterElement &);
    YUVWriterElement &operator=(const YUVWriterElement &);

    mfxStatus InitEncoder(MediaBuf&);
    int ProcessChainEncode(MediaBuf&);
    int HandleProcessEncode();

    YUVWriterPlugin *user_enc_;
    mfxBitstream output_bs_;
    //mfxFrameAllocResponse m_mfxEncResponse;  // memory allocation response for encode
    std::vector<mfxExtBuffer *> m_EncExtParams;
    mfxU32 m_nFramesProcessed;
    static unsigned int  mNumOfEncChannels;
};
#endif //YUV_WRITER_ELEMENT_H
#endif //ENABLE_RAW_CODEC
