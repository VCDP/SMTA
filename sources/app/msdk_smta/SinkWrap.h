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

#ifndef __SINKWRAP_H__
#define __SINKWRAP_H__

#include <memory>
#include <mfxdefs.h>
#include <stdio.h>
#include <vector>

#include "InputParams.h"
#include "BitstreamSink.h"
#include "FFmpegBitstream.h"
#include "os/XC_Thread.h"
#include "base/stream.h"
#include "video/msdk_xcoder.h"
#include "video/encoder_element.h"

namespace TranscodingSample
{
    class SinkWrap
    {
        public:
            class SinkPipeline
            {
                public:
                    SinkPipeline();
                    Stream* GetStream(void);
                    uint32_t OneSinkOutput();
                    mfxStatus OnFirstPutBS(mfxBitstream *pMfxBitstream);
                public:
                    bool m_bThreadExit;
                    Stream* m_pStreamOutput;
                    std::auto_ptr<BitstreamSink> m_pBSSink;
                    EncParams * m_encInputParams;
                private:
                    mfxU64 m_OutPackets;
                    const mfxI32 m_nBufferSize;
             };

            struct ThreadSink {
                SinkPipeline* pipeline;
                XC::Thread* thread;
            };

            SinkWrap(void);
            ~SinkWrap(void);
            mfxStatus AddNewSink(EncParams &encInputParams);
            mfxStatus Start(void);
            void Stop(void);
            void Wait(void);
            Stream* GetStream(void);
        private:
            bool m_bIsRunning;
            std::vector<SinkPipeline*> m_pSink;
            struct ThreadSink m_mapThread[MAX_PIPELINE];
            int m_nPipelineCnt;
            SinkPipeline *m_pCurrSink;
            BitstreamSinkFactory bitstreamSinkFactory;
    };
}
#endif //__SINKWRAP_H__

