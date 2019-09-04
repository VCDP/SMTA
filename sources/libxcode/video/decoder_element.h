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

#ifndef DECODER_ELEMENT_H
#define DECODER_ELEMENT_H
#include <stdlib.h>
#include "msdk_codec.h"

class DecoderElement: public BaseDecoderElement
{
public:
    DECLARE_MLOGINSTANCE();
    DecoderElement(ElementType type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~DecoderElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);
    virtual void SetRequestSurface(int surfNum){}

#ifdef ENABLE_VA
    virtual int Step(FrameMeta* input);
#endif

protected:

    virtual int HandleProcess();
    virtual int Recycle(MediaBuf &buf);
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

private:
    DecoderElement(const DecoderElement &);
    DecoderElement &operator=(const DecoderElement &);

    void UpdateMemPool();
    void UpdateBitStream();

    mfxStatus InitDecoder();
    int HandleProcessDecode();
    int ProcessChainDecode();

    MemPool* input_mp_;
    Stream* input_stream_;
    bool    is_mempool_;
    // IBitstreamReader* reader;
    // Decoder input
    mfxBitstream input_bs_;

    MFXVideoDECODE *mfx_dec_;
    //mfxFrameAllocResponse m_mfxDecResponse;  // memory allocation response for decoder
    std::vector<mfxExtBuffer *> m_DecExtParams;
    enum VP8FILETYPE vp8_file_type_;
    DecExtParams m_extParams;

    /**
     * \brief benchmark.
     */
    //Measurement  *measurement;
    static unsigned int  mNumOfDecChannels;

};

#endif //DECODER_ELEMENT_H
