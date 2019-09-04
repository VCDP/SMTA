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
#ifndef __BITSTREAM_MGR_H__
#define __BITSTREAM_MGR_H__

#include <mfxstructures.h>

#include "base/mem_pool.h"
#include "base/stream.h"
#include "base/logger.h"

class BitstreamMgr {
    public:
        DECLARE_MLOGINSTANCE();
        BitstreamMgr();
        ~BitstreamMgr();

        int open(void);
        void close(void);

        // copy data from AVPacket to memory pool
        // and create a mfxBitstream Node , then set Data, PTS, DTS
        size_t push(void *buffer, size_t size,
                uint16_t type = 0, int64_t pts = 0, int64_t dts = 0);
        void pop(mfxBitstream &stream);
        void setEndOfInput(void);
        bool getEndOfInput(void) { return read_over; }

        MemPool* get_mp(void) { return &mp; }

    private:
        mfxBitstream *new_bs(void);
        size_t store_to_mp(void *buffer, size_t size,
                uint16_t type, int64_t pts, int64_t dts);

        // stash data
        size_t push_stash(void *buffer, size_t size,
                uint16_t type, int64_t pts, int64_t dts);
        // Update data in stash to mp
        size_t flush_stash(void);

    private:
        MemPool mp;
        Stream stash;
        std::list<mfxBitstream*> bs_list;
        bool read_over;
};

#endif //ifndef __BITSTREAM_MGR_H__

