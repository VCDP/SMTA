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

#ifndef _BITSTREAMSOURCE_H__
#define _BITSTREAMSOURCE_H__

#include <stdio.h>

#include "video/io_adapter.hpp"
#include "base/stream.h"

namespace TranscodingSample
{
    // Provides decoder with encoded bitstream parent class
    class BitstreamSource //: public IBitstreamReader
    {
        public:
            class ErrListener {
                public:
                    virtual void onError(void) = 0;
            };

            BitstreamSource() : m_pDumpFile(NULL), listener(NULL) {};
            virtual ~BitstreamSource() {};

            virtual mfxStatus open(const std::string &strFileName, const int codecId, const bool bIsLoop, const mfxU16 nLoopNum, const bool bInputByFPS) = 0;
            // To start again transcoding when reached end of one input bitstream (source looping)
            virtual mfxStatus resume() { return MFX_ERR_MORE_DATA; }
            // Returns whether object should also be deleted after Close() or if deletion will happen
            // as a consequence of calling Close(). True means caller should also delete BitstreamSource object.
            virtual mfxStatus close() = 0;
            virtual mfxStatus start() = 0;
            virtual mfxStatus stop() = 0;
            virtual MemPool* getMemPool(void) = 0;
            virtual Stream *getStreamBuffer(void) = 0;
            virtual bool isRTSP(void) = 0;
            void setErrListener(ErrListener *listener) { this->listener = listener; }

        protected:
            FILE* m_pDumpFile;
            ErrListener *listener;
    };

} // ns

#endif /* _BITSTREAMSOURCE_H__ */
