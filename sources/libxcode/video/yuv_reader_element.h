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
#ifdef ENABLE_RAW_DECODE
#ifndef YUV_READER_ELEMENT_H
#define YUV_READER_ELEMENT_H
#include <stdlib.h>
#include "msdk_codec.h"
#include "yuv_reader_plugin.h"

class YUVReaderElement: public BaseDecoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    YUVReaderElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~YUVReaderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual RawInfo *GetRawInfo() {
        return input_raw_;
    }

protected:
    //BaseElement
    virtual int HandleProcess();
    virtual int Recycle(MediaBuf &buf);

private:
    YUVReaderElement(const YUVReaderElement &);
    YUVReaderElement &operator=(const YUVReaderElement &);

    mfxStatus InitDecoder();
    int HandleProcessDecode();
    RawInfo *input_raw_;

    YUVReaderPlugin *user_dec_;
    //mfxFrameAllocResponse m_mfxDecResponse;  // memory allocation response for decoder
    std::vector<mfxExtBuffer *> m_DecExtParams;
    bool is_eos_;

    static unsigned int  mNumOfDecChannels;
};

#endif //YUV_READER_ELEMENT_H
#endif //ENABLE_RAW_DECODE
