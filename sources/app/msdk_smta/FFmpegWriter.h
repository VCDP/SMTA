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

#ifndef __BITSTREAMWRITER_H__
#define __BITSTREAMWRITER_H__

#include "sample_defs.h"
#include "mfxstructures.h"
#include "InputParams.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/avstring.h"
}

namespace TranscodingSample {

    class FFmpegWriter : public BitstreamSink
    {
    public :
        FFmpegWriter(OutputType type, string url, bool bDump);
        virtual ~FFmpegWriter();

        // from BitstreamSink
        virtual mfxStatus ProcessOutputBitstream();
        virtual mfxStatus Resume();
        virtual int InitMuxerParams(Stream* stream, MuxerParam &muxerParam);
        virtual mfxStatus Close();
        virtual mfxI32 GetOutputPackets() { return m_nFrameIndex; };

    public:
        bool              m_bStart;
        //mfxBitstream*     m_pMfxBitstream;
        Stream*           m_inputStream;

    private:
        bool              m_bInited;
        mfxI32            m_nVideoIndexIn;
        mfxI32            m_nVideoIndexOut;
        AVFormatContext*  m_pOutputFormatCtx;
        AVFormatContext*  m_pInputFormatCtx;
        string            m_strDstURI;
        mfxI32            m_nFrameIndex;
        const mfxI32      m_nBufferSize;
        FILE *            m_DumpFile;
        bool              m_bDump;
        OutputType        m_eOutputType;
        unsigned char*    m_avioBuffer;
    };

} // ns

#endif // __BITSTREAMWRITER_H__
