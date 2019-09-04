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

/**
 *\file Stream.cpp
 *\brief Implementation for Stream class
 */

#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "stream.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
const int BufferSize = 1024 * 1024;

Stream::Stream():
    mSafeData(false),
    mFile(NULL),
    mTotalDataSize(0),
    mAllocBufferSize(0),
    mTimeout(5),
    mEndOfStream(false),
    mLoopTimes(0),
    alloc_count(0),
    mStreamType(STREAM_TYPE_UNKNOWN)
{
}

Stream::~Stream()
{
    switch (mStreamType) {
    case STREAM_TYPE_FILE:

        if (mFile) {
            fclose(mFile);
            mFile = NULL;
        }

        break;
    case STREAM_TYPE_MEMORY:

        //flush buffer list
        while (!mBufferList.empty()) {
            STREAM_BUFF &block = mBufferList.back();
            free(block.buffer);
            mBufferList.pop_back();
        }
        while (!mFreeBufferList.empty()) {
            void* buf = mFreeBufferList.front();
            if (buf)
                free(buf);
            mFreeBufferList.pop_front();
        }

        break;
    default:
        break;
    }
}

bool Stream::Open(const char *filename, bool openWrite)
{
    mStreamType = STREAM_TYPE_FILE;
    if (mFile) {
        fclose(mFile);
        mFile = NULL;
    }
    mFile = fopen(filename, openWrite ? "wb" : "rb");
    mFileName = filename;

    if (!openWrite) {
        mInFileName = filename;
    }

    return mFile != NULL;
}

bool Stream::Open(string name)
{
    mFileName = name;
    mStreamType = STREAM_TYPE_MEMORY;
    return true;
}

bool Stream::Open(const size_t size)
{
    mStreamType = STREAM_TYPE_MEMORY;
    mAllocBufferSize = size;
    return true;
}

void Stream::SetStreamAttribute(int flag) {
    if(mStreamType == STREAM_TYPE_FILE) {
        switch(flag) {
            case FILE_HEAD:
                fseek(mFile, 0L, SEEK_SET);
                break;
            case FILE_TAIL:
                fseek(mFile, 0L, SEEK_END);
                break;
            case FILE_CUR:
                fseek(mFile, 0L, SEEK_CUR);
                break;
            default:
                break;
        }
    } else {
        printf("Don't work, it's a stream input\n");
    }
}

int Stream::SetStreamAttribute(int flag, long offset) {
    int seek_sts = 0;
    if(mStreamType == STREAM_TYPE_FILE) {
        switch(flag) {
            case FILE_HEAD:
                seek_sts = fseek(mFile, offset, SEEK_SET);
                break;
            case FILE_TAIL:
                seek_sts = fseek(mFile, offset, SEEK_END);
                break;
            case FILE_CUR:
                seek_sts = fseek(mFile, offset, SEEK_CUR);
                break;
            default:
                break;
        }
    } else {
        printf("Don't work, it's a stream input\n");
        return -1;
    }

    return seek_sts;
}
size_t Stream::ReadBlock(void *buffer, size_t size, bool waitData)
{
    switch (mStreamType) {
    case STREAM_TYPE_FILE:

        if (mFile) {
            size_t rd = fread(buffer, 1, size, mFile);

            if (feof(mFile)) {
                mEndOfStream = true;
            }

            return rd;
        } else {
            return 0;
        }

    case STREAM_TYPE_MEMORY: {
        size_t retValue = 0;

        //waits for data to be available
        if (waitData) {
            bool timedOut = false;
            {
                Locker<Mutex> lock(mutex); 
                waitData = !mEndOfStream && (mTotalDataSize < size);
            }

            while (waitData && !timedOut) {
                Locker<Mutex> lock(mutex);
                if (!mEndOfStream && mTotalDataSize < size) {
                    timedOut = mutex.TimedWait(mTimeout);
                }

                if (timedOut && (mTotalDataSize == 0)) {
                    FRMW_TRACE_INFO("Stream %p time out!!!!!!!!!!!!!!!!\n", this);
                    mEndOfStream = true;
                }

                waitData = !mEndOfStream && (mTotalDataSize == 0);
            }

            //throw timeout exception
            if (timedOut) {
                FRMW_TRACE_ERROR("Data timeout");
                return -1;
            }
        }

        {
            Locker<Mutex> lock(mutex);
            if (!mBufferList.empty()) {
                int remainingSize = size;
                char *pDest = (char *)buffer;

                while (remainingSize && !mBufferList.empty()) {
                    STREAM_BUFF &block = mBufferList.front();

                    //read from block if it has data
                    if (block.size) {
                        size_t copySize = MIN((int)block.size, remainingSize);
                        memcpy(pDest, block.indx, copySize);
                        pDest += copySize;
                        remainingSize -= copySize;
                        block.size -= copySize;

                       //update block index if there is more data
                        if (block.size) {
                            block.indx += copySize;
                        }
                    }

                    //if no more data delete the block
                    if (!block.size) {
                        free(block.buffer);
                        mBufferList.pop_front();
                    }
                }

                //return actual size
                retValue = size - remainingSize;
                mTotalDataSize -= retValue;
            } //if (!mBufferList.empty())
        }
        return retValue;
    }
    break;
    default:
        return 0;
    }
}

size_t Stream::ReadBlockEx(void *buffer, size_t size, bool waitData,
                            uint16_t* type, uint64_t* pts, uint64_t* dts)
{
    switch (mStreamType) {
    case STREAM_TYPE_FILE:

        if (mFile) {
            size_t rd = fread(buffer, 1, size, mFile);

            if (feof(mFile)) {
                mEndOfStream = true;
            }

            return rd;
        } else {
            return 0;
        }

    case STREAM_TYPE_MEMORY: {
        size_t retValue = 0;

        //waits for data to be available
        if (waitData) {
            bool timedOut = false;
            {
                Locker<Mutex> lock(mutex);
                waitData = !mEndOfStream && (mTotalDataSize < size);
            }

            while (waitData && !timedOut) {
                Locker<Mutex> lock(mutex);
                if (!mEndOfStream && mTotalDataSize < size) {
                    timedOut = mutex.TimedWait(mTimeout);
                }

                if (timedOut && (mTotalDataSize == 0)) {
                    FRMW_TRACE_INFO("Stream %p time out!!!!!!!!!!!!!!!!\n", this);
                    mEndOfStream = true;
                }

                waitData = !mEndOfStream && (mTotalDataSize == 0);
            }

            //throw timeout exception
            if (timedOut) {
                FRMW_TRACE_ERROR("Data timeout");
                return -1;
            }
        }

        {
            Locker<Mutex> lock(mutex);
            if (!mBufferList.empty()) {
                int remainingSize = size;
                char *pDest = (char *)buffer;

                STREAM_BUFF &block = mBufferList.front();

                //read from block if it has data
                if (block.size) {
                    retValue = MIN((int)block.size, remainingSize);
                    memcpy(pDest, block.indx, retValue);

                    block.size -= retValue;
                    mTotalDataSize -= retValue;
                    if (block.size) {
                        block.indx += retValue;
                    }
                }

                //if no more data delete the block
                if (!block.size) {
                    mFreeBufferList.push_back(block.buffer);
                    mBufferList.pop_front();
                }
            } //if (!mBufferList.empty())
        }
        return retValue;
    }
    break;
    default:
        return 0;
    }
}

size_t Stream::GetTopBlockSize(void)
{
    size_t retValue = 0;

    if (STREAM_TYPE_MEMORY == mStreamType) {
        //waits for data to be available
        Locker<Mutex> lock(mutex);
        if (!mBufferList.empty()) {
            STREAM_BUFF &block = mBufferList.front();
            retValue = block.size;
        } //if (!mBufferList.empty())
    }

    return retValue;
}

// size_t Stream::ReadBlocks(void *buffer, int count)
// {
//     int retValue = 0;

//     {
//         Locker<Mutex> lock(mutex);
//         if (!mBufferList.empty()) {
//             int remainingCount = count;
//             char *pDest = (char *)buffer;

//             while (remainingCount-- && !mBufferList.empty()) {
//                 STREAM_BUFF &block = mBufferList.front();

//                 //read from block and delete it
//                 if (block.size) {
//                     memcpy(pDest, block.indx, block.size;
//                     retValue += block.size;
//                 }

//                 free(block.buffer);
//                 mBufferList.pop_front();
//             }

//             mTotalDataSize -= retValue;
//         }
//     }

//     return retValue;
// }

size_t Stream::WriteBlock(void *buffer, size_t size, bool copy)
{
    switch (mStreamType) {
    case STREAM_TYPE_FILE:

        if (mFile) {
            return fwrite(buffer, 1, size, mFile);
        } else {
            return 0;
        }

    case STREAM_TYPE_MEMORY:

        if (size && buffer) {
            void *inBuff;

            if (copy) {
                //copy buffer, leave input buffer untouched
                inBuff = malloc(size);

                if (inBuff == NULL) {
                    FRMW_TRACE_ERROR("Memory allocation failed");
                    return -1;
                }

                memcpy(inBuff, buffer, size);
            } else { //push the input buffer, it will be free'd when no longer needed
                inBuff = buffer;
            }

            STREAM_BUFF block = {inBuff, (char *)inBuff, size, 0};
            {
                Locker<Mutex> lock(mutex);
                mBufferList.push_back(block);
                mTotalDataSize += size;
                mutex.CondSignal();
            }
        }

        return size;
    default:
        return 0;
    }
}

size_t Stream::WriteBlockEx(void *buffer, size_t size, bool loop,
                            unsigned short type, int64_t pts, int64_t dts)
{
    switch (mStreamType) {
    case STREAM_TYPE_FILE:

        if (mFile) {
            return fwrite(buffer, 1, size, mFile);
        } else {
            return 0;
        }

    case STREAM_TYPE_MEMORY:
    {
        size_t copySize = 0;
        if (size && buffer) {
            void *inBuff = NULL;
            size_t remainSize = size;

            Locker<Mutex> lock(mutex);
            if (loop) {
                while (copySize < size) {
                    size_t wSize = 0;
                    STREAM_BUFF &block = mBufferList.back();
                    size_t spaceSize = mBufferList.empty() ? 0: (mAllocBufferSize - (block.wIndex - (char*)(block.buffer)));
                    if (!mBufferList.empty() && spaceSize) {
                        inBuff = block.wIndex;
                        wSize = MIN(remainSize, spaceSize);
                        memcpy(inBuff, (char*)buffer + copySize, wSize);
                        copySize += wSize;
                        remainSize -= wSize;
                        block.size += wSize;
                        block.wIndex += wSize;
                        mTotalDataSize += wSize;
                        continue;
                    } else if (!mFreeBufferList.empty()){
                        inBuff = mFreeBufferList.front();
                        mFreeBufferList.pop_front();
                    } else {
                        inBuff = malloc(mAllocBufferSize);
                        if (inBuff == NULL) {
                            FRMW_TRACE_ERROR("Memory allocation failed");
                            return -1;
                        }
                    }
                    memset(inBuff, 0, mAllocBufferSize);

                    wSize = MIN(remainSize, mAllocBufferSize);
                    memcpy(inBuff, buffer + copySize, wSize);
                    copySize += wSize;
                    remainSize -= wSize;
                    STREAM_BUFF newBlock = {inBuff, (char *)inBuff, wSize, ((char *)inBuff + wSize)};
                    {
                        mBufferList.push_back(newBlock);
                        mTotalDataSize += wSize;
                        //mutex.CondSignal();
                    }
                }
            } else {
                if (!mFreeBufferList.empty()){
                    inBuff = mFreeBufferList.front();
                    mFreeBufferList.pop_front();
                } else {
                    alloc_count++;
                    inBuff = malloc(mAllocBufferSize);
                    if (inBuff == NULL) {
                        FRMW_TRACE_ERROR("Memory allocation failed");
                        return -1;
                    }
                }
                copySize = MIN(remainSize, mAllocBufferSize);
                memcpy(inBuff, buffer, copySize);
                STREAM_BUFF newBlock = {inBuff, (char *)inBuff, copySize, ((char *)inBuff + copySize)};
                {
                    mBufferList.push_back(newBlock);
                    mTotalDataSize += copySize;
                    //mutex.CondSignal();
                }
            }
            mutex.CondSignal();
        }

        return copySize;
    }
    default:
        return 0;
    }
}

size_t Stream::GetBlockCount(size_t *dataSize)
{
    size_t count = 0;
    {
        Locker<Mutex> lock(mutex);
        count = mBufferList.size();

        if (dataSize) {
            *dataSize = mTotalDataSize;
        }
    }
    return count;
}

void Stream::SetEndOfStream(bool endFlag)
{
    Locker<Mutex> lock(mutex);
    if (endFlag) {
        mEndOfStream = true;
    } else {
        mEndOfStream = false;
    }
    mutex.CondSignal();
}

void Stream::SetEndOfStream()
{
    Locker<Mutex> lock(mutex);
    mEndOfStream = true;
    mutex.CondSignal();
}

bool Stream::GetEndOfStream()
{
    bool eos = false;
    {
        Locker<Mutex> lock(mutex);
        eos = mEndOfStream;
    }
    return eos;
}

void Stream::SetLoopTimesOfStream(int loop) {
    Locker<Mutex> lock(mutex);
    if (loop > 0) {
        mLoopTimes = loop;
    } else {
        mLoopTimes = 0;
    }
}

int  Stream::GetLoopTimesOfStream() {
    int loop = 0;
    {
        Locker<Mutex> lock(mutex); 
        if (mLoopTimes > 0) {
            loop = mLoopTimes--;
        }
    }
    return loop;
}
