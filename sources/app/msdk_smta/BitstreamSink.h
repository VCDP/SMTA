/****************************************************************************
 *
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
 *
 ****************************************************************************/

#ifndef __BITSTREAMSINK_H__
#define __BITSTREAMSINK_H__

#include "mfxvideo++.h"

namespace TranscodingSample
{
    // Receives xlibcode encoded bitstream at output of encoder parent class
    class BitstreamSink
    {
        public:
            enum OutputType {
                E_NONE,
                E_RTP_ES,
                E_RTP_TS,
                E_FILE
            };

            struct MuxerParam {
                mfxU32              nEncodeId;            // type of output coded video
                mfxU32              nBitRate;
                mfxU16              nWidth;
                mfxU16              nHeight;
                mfxI32              nTimebaseNum;
                mfxI32              nTimebaseDen;
                mfxI32              extraDataSize;
                mfxU8               *pExtraData;
                mfxI32              nGropSize;
                mfxI32              nPiexlFomat;
                bool                bInterlaced;
            };

        public:
            BitstreamSink() {};
            virtual ~BitstreamSink() {};
            // get xlibcode encoded bitstream and handle it
            virtual mfxStatus ProcessOutputBitstream() = 0;
            // To start again transcoding when reached end of one input bitstream (source looping)
            virtual mfxStatus Resume() { return MFX_ERR_MORE_DATA; }
            // set muxer parameters to encoded video parameter
            virtual int InitMuxerParams(Stream* stream, MuxerParam &muxerParam) = 0;
            // Returns whether object should also be deleted after Close() or if deletion will happen
            // as a consequence of calling Close(). True means caller should also delete BitstreamSink object.
            virtual mfxStatus Close() { return MFX_ERR_NONE; };
            virtual mfxI32 GetOutputPackets() = 0;
    };

    class IBitstreamSinkFactory
    {
        public:
            virtual BitstreamSink* getInstance(BitstreamSink::OutputType type,
                    string url, bool isDump) = 0;
    };
} // ns

#endif /* __BITSTREAMSINK_H__ */
