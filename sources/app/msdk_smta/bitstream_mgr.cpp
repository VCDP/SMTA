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
 * @file: bitstream_mgr.cpp
 * @description:
 *   This class is used to buffer data between ffmpeg reader and libxcode
 *   decoder.
 *   It contains a MemPool and a Stream. libXcode decoder read data from
 *   MemPool and update MemPool read pointer. A reading thread reads data
 *   in FFmpegReader and push data to Stream. Before decoder popping data
 *   from MemPool, BitstreamMgr reads data from Stream, and fills them to
 *   MemePool.
 *
 ****************************************************************************/
#include "bitstream_mgr.hpp"

#include <string.h>

#define BUFFER_SIZE (4 * 1024 *1024)
#define DYN_BUFFER

DEFINE_MLOGINSTANCE_CLASS(BitstreamMgr, "BitstreamMgr");

BitstreamMgr::BitstreamMgr() :
    read_over(false) {
}

BitstreamMgr::~BitstreamMgr() {
}


int BitstreamMgr::open(void) {
    int ret = 0;

    mp.init(BUFFER_SIZE);
    stash.Open();

    return ret;
}

void BitstreamMgr::close(void) {
}

size_t BitstreamMgr::push(void *buffer, size_t size,
        uint16_t type, int64_t pts, int64_t dts) {
#if !defined(DYN_BUFFER)
    size_t buf_size = 0 ;
    // first to flush stash
    flush_stash();

    // get mempool free buffer
    buf_size = mp.GetFreeFlatBufSize();
    if (buf_size < size) {
        //not enouth memory, put data to stash
        return push_stash(buffer, size, type, pts, dts);
    }

    return store_to_mp(buffer, size, type, pts, dts);
#else
    return push_stash(buffer, size, type, pts, dts);
#endif
}

mfxBitstream *BitstreamMgr::new_bs(void) {
    mfxBitstream *bs = new mfxBitstream;
    if (nullptr == bs) {
        printf("%s: Create stream error!\n", __FILE__);
        return bs;
    }

    memset(bs, 0, sizeof(mfxBitstream));
    bs->Data = mp.GetWritePtr();

    return bs;
}

size_t BitstreamMgr::store_to_mp(void *buffer, size_t size,
        uint16_t type, int64_t pts, int64_t dts) {

    mfxBitstream *bs = new_bs();
    if (nullptr == bs) {
        printf("%s: Create stream error!\n", __FILE__);
        return 0;
    }

    // copy input stream data buffer to mempool
    memcpy(bs->Data, buffer, size);
    mp.UpdateWritePtrCopyData(size);

    bs->DataLength = bs->MaxLength = size;
    bs->FrameType = type;
    bs->TimeStamp = pts;
    bs->DecodeTimeStamp = dts;

    bs_list.push_back(bs);

    //printf("%s: Store to memory %p, %d\n", __FILE__, mp.GetWritePtr(), size);
    return size;
}

size_t BitstreamMgr::push_stash(void *buffer, size_t size,
        uint16_t type, int64_t pts, int64_t dts) {
    size_t sz = 0;

    sz = stash.WriteBlockEx(buffer, size, true, type, pts, dts);
    MLOG_INFO("%s: Push stash %d!\n", __FILE__, sz);
    if (0 == sz) {
        printf("%s: Push stash error!\n", __FILE__);
    }

    return sz;
}

size_t BitstreamMgr::flush_stash(void) {
    size_t b_size  = 0, mp_size = 0;
    mfxBitstream *bs = nullptr;

    while (stash.GetBlockCount(nullptr) > 0) {
        b_size = stash.GetTopBlockSize();
        mp_size = mp.GetFreeFlatBufSize();

        MLOG_INFO("mp size is %d, block size is %d\n", mp_size, b_size);
        if ((mp_size > b_size) || (mp.IsOverflowFromPadding((unsigned int)b_size) == MEM_POOL_RETVAL_OVERSTEP)) {
            bs = new_bs();
            if (nullptr == bs) {
                printf("%s: Create stream error!\n", __FILE__);
                return 0;
            }
            size_t copy_size = mp_size > b_size ? b_size : mp_size;

            if (0 == stash.ReadBlockEx(bs->Data, copy_size, false,
                    &bs->FrameType, (uint64_t*)&bs->TimeStamp,
                    (uint64_t*)&bs->DecodeTimeStamp)) {
                delete bs;
                return 0;
            }
            mp.UpdateWritePtrCopyData(copy_size);
            bs->MaxLength = bs->DataLength = copy_size;
            bs_list.push_back(bs);
            return copy_size;
        } else {
            return 0;
        }
    }

    return 0;
}

void BitstreamMgr::pop(mfxBitstream &stream) {
    mfxBitstream *bs = nullptr;

    if (bs_list.size() > 0) {
        bs = bs_list.front();
    }

    if (0 == stash.GetBlockCount(NULL) && read_over) {
        mp.SetDataEof(true);
        return;
    }

    flush_stash();

    if (nullptr != bs) {
        // check if has some not read in stream,
        // do not read from mem pool
        if (bs->Data < stream.Data) {
            return;
        }
        stream.TimeStamp = bs->TimeStamp;
        stream.DecodeTimeStamp = bs->DecodeTimeStamp;
        stream.FrameType = bs->FrameType;
        stream.Data = mp.GetReadPtr();
        stream.DataLength = bs->DataLength;

        delete bs;
        bs_list.pop_front();
    }
}

void BitstreamMgr::setEndOfInput(void) {
    read_over = true;
}

