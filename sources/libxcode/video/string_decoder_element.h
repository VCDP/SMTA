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
#ifdef ENABLE_STRING_CODEC
#ifndef STRING_DECODER_ELEMENT_H
#define STRING_DECODER_ELEMENT_H
#include <stdlib.h>
#include "msdk_codec.h"
#include "string_decode_plugin.h"

class StringDecoderElement: public BaseDecoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    StringDecoderElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~StringDecoderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);
    virtual StringInfo *GetStrInfo() {
        return input_str_;
    }

protected:
    //BaseElement
    virtual int HandleProcess();
    virtual int Recycle(MediaBuf &buf);
    //virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);
private:
    StringDecoderElement(const StringDecoderElement &);
    StringDecoderElement &operator=(const StringDecoderElement &);

    mfxStatus InitDecoder();
    int HandleProcessDecode();
    StringInfo *input_str_;

    StringDecPlugin *user_dec_;
    //mfxFrameAllocResponse m_mfxDecResponse;  // memory allocation response for decoder
    std::vector<mfxExtBuffer *> m_DecExtParams;
    bool is_eos_;

    static unsigned int  mNumOfDecChannels;

};

#endif //STRING_DECODER_ELEMENT_H
#endif //ENABLE_STRING_CODEC
