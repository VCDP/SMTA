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

#ifndef __BITSTREAMREADER_H__
#define __BITSTREAMREADER_H__

#include <string>
#include <memory>
#include "BitstreamSource.h"
#include "bitstream_mgr.hpp"
#include "os/XC_Thread.h"

#ifdef __cplusplus
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#endif

namespace TranscodingSample {

    class FFmpegReader : public BitstreamSource
    {
        public :
            FFmpegReader();
            ~FFmpegReader();

            AVRational getFrameRate(void) {return m_pVideoStream->avg_frame_rate; }
            AVRational getTimeBase(void) {return m_pVideoStream->r_frame_rate; }
            int getInputWidth(void) {return m_pVideoStream->codecpar->width; }
            int getInputHeight(void) {return m_pVideoStream->codecpar->height; }
            enum AVFieldOrder getInputField(void) {return m_pVideoStream->codecpar->field_order; }
            bool threadIsRunning() { return m_bIsRunning;}

        public: // BitstreamSource interface
            virtual mfxStatus open(const std::string &strFileName,
                                    const int codecId, const bool bIsLoop, const mfxU16 loopNum, const bool bInputByFPS);
            virtual mfxStatus resume(void);
            virtual mfxStatus close(void);
            virtual mfxStatus start(void);
            virtual MemPool* getMemPool(void) {return &m_mempool;}
            virtual Stream *getStreamBuffer(void) {return &m_stream; }
            virtual bool isRTSP(void) { return m_bIsRTSP;}
            virtual mfxStatus stop(void);

       private:
            mfxStatus readNextFrame(void);
            int openInputFile();
            static int interruptCallback(void *ctx);
            bool isTimeOut();
            static uint32_t XC_THREAD_CALLCONVENTION ReadThread(void *pObj);
            int reloadSource();
            mfxStatus PushData(void* buffer, int size);

        private:
            int  m_nVideoIndex;
            int  m_nTimeOut;
            time_t  m_nStartTime;
            enum AVCodecID codec_id;
            AVStream *m_pVideoStream;
            AVFormatContext *m_pAvFormatContext;
            AVBSFContext *m_pVideoBSF;

            std::string m_strInputURI;
            std::unique_ptr<XC::Thread> m_thread;
            //BitstreamMgr stream_buf;
            MemPool m_mempool;
            Stream m_stream;
            bool m_bIsRTSP;
            bool m_bIsLoopMode;
            bool m_bIsRunning;
            mfxU16 m_nLoopNum;
            mfxU16 m_nCurrentLoopNum;
            int m_timediff;
            bool m_bInputByFPS;
            int64_t m_prevTime;
    };

} // ns

#endif /* __BITSTREAMREADER_H__ */
