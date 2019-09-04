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

#ifndef STREAM_H_
#define STREAM_H_

#include <list>
#include <stdio.h>
#include <string>
#include <vector>

#include "os/mutex.h"

using namespace std;

enum Attributes {
    FILE_HEAD = 0,
    FILE_TAIL,
    FILE_CUR
};
/**
 *\brief Transparent Stream class.
 *\details Provides a transparent interface for codec objects,
 * hiding the data channel type.
 */
class Stream
{
public:
    Stream();
    virtual ~Stream();

    /**
     *\brief Opens a file.
     *\param filename File name to be open.
     *\param openWrite true if file is write-only, false for read-only.
     *\returns true on successful opening.
     */
    bool Open(const char *filename, bool openWrite = false);

    /**
     *\brief Opens a memory based stream.
     *\param openWrite true if file is write-only, false for read-only.
     *\returns true on successful opening.
     */
    bool Open(const size_t size = (1024 * 1024));

    /**
     *\brief Opens a memory based stream.
     *\param openWrite true if file is write-only, false for read-only.
     *\returns true on successful opening.
     */
    bool Open(string name);

    /**
     *\brief Reads a block of data from the input stream.
     *\param buffer Preallocated buffer to hold the data.
     *\param size Size in bytes.
     *\param waitData blocks if not enough data is available until either a
     *\packet of data is available or until a time out occurs. Setting this flag
     *\does not guarantee that the data size read will be the data size requested.
     *\returns Byte count actually read.
     */
    size_t ReadBlock(void *buffer, size_t size, bool waitData = true);

    /**
     *\brief Reads a block of data from the input stream Extra.
     *\param buffer Preallocated buffer to hold the data.
     *\param size Size in bytes.
     *\param waitData blocks if not enough data is available until either a
     *\packet of data is available or until a time out occurs. Setting this flag
     *\does not guarantee that the data size read will be the data size requested.
     *\param type Frame type.
     *\returns Byte count actually read.
     */
    size_t ReadBlockEx(void *buffer, size_t size, bool waitData = true,
                        uint16_t* type = NULL, uint64_t* pts = NULL,
                        uint64_t* dts = NULL);

    /**
     *\brief Reads the first block size.
     *\returns size of the first block in list
     */
    size_t GetTopBlockSize(void);

    /**
     *\brief Writes a block of data to the output stream.
     *\param buffer Preallocated buffer that holds the data.
     *\param size Size in bytes.
     *\param copy For memory streams only: If true the data will be copied and
     *the input buffer will not be modified; if false then the buffer will be used as it is,
     *and free() will be called when no longer needed, so it must be
     *allocated with malloc() rather than new().
     *\returns Byte count actually wrote.
     */
    size_t WriteBlock(void *buffer, size_t size, bool copy = true);

    /**
     *\brief Writes a block of data to the output stream.
     *\param buffer Preallocated buffer that holds the data.
     *\param size Size in bytes.
     *\param copy For memory streams only: If true the data will be copied and
     *the input buffer will not be modified; if false then the buffer will be used as it is,
     *and free() will be called when no longer needed, so it must be
     *allocated with malloc() rather than new().
     *\param type Frame type.
     *\returns Byte count actually wrote.
     */
    size_t WriteBlockEx(void *buffer, size_t size, bool loop = true,
                        uint16_t type = 0, int64_t pts = 0, int64_t dts = 0);
    /**
     *\brief returns total number of memory blocks.
     *\param dataSize optional pointer to a variable to hold total available data,
     *as calling GetDataSize() before or after can give false results.
     */
    size_t GetBlockCount(size_t *dataSize = NULL);


    /**
     *\brief Reads a number of blocks of data from the input stream.
     *\param buffer Preallocated buffer to hold the data.
     *\param count Number of memory blocks to be read.
     *\returns Byte count actually read.
     */
    // size_t ReadBlocks(void *buffer, int count);

    /**
     *\brief Sets the stream attributes(based on file).
     */
    void SetStreamAttribute(int flag);
    /**
     *\brief Sets the stream attributes(based on file).
     */
    int SetStreamAttribute(int flag, long offset);
    /**
     *\brief Sets the end of stream flag.
     */
    void SetEndOfStream(bool endFlag);
    /**
     *\brief Sets the end of stream flag (for memory streams only).
     */
    void SetEndOfStream();
    /**
     *\brief Gets the end of stream flag (for memory streams only).
     */
    bool GetEndOfStream();
    /**
     *\brief Sets the loop tiems.
     */
    void SetLoopTimesOfStream(int loop);
    /**
     *\brief Gets the loop times.
     */
    int GetLoopTimesOfStream();
    /**
     *\brief returns stream name.
     */
    string &GetFilename() {
        return mFileName;
    };

    /**
     *\brief returns total available data (for memory streams only).
     */
    size_t GetDataSize() {
        return mTotalDataSize;
    };

    bool mSafeData;          /**< \brief safe data to the next module*/
protected:
    string &GetInFilename() {
        return mInFileName;
    };

    /**\brief Memory buffer list element datatype*/
    typedef struct {
        void    *buffer;    /**<\brief buffer address.*/
        char    *indx;      /**<\brief current address.*/
        size_t  size;       /**<\brief current data size.*/
        char*   wIndex;     /**<\brief current writed data address.*/
        // unsigned short frame_type; /**<\brief current frame type.*/
        // int64_t pts;
        // int64_t dts;
    } STREAM_BUFF;

    enum STREAM_TYPE {
        STREAM_TYPE_UNKNOWN,
        STREAM_TYPE_FILE,
        STREAM_TYPE_MEMORY
    };


    /**\brief FILE object for file based streams*/
    FILE               *mFile;
    string              mFileName;
    string              mInFileName;

    /**\brief buffer list for memory based streams*/
    list<STREAM_BUFF>   mBufferList;
    /**\brief total available data size (in bytes)*/
    size_t              mTotalDataSize;
    list<void*>       mFreeBufferList;
    size_t              mAllocBufferSize;

    int                 mTimeout;           /**< \brief Default time out*/
    bool                mEndOfStream;
    int                 mLoopTimes;
    int                 alloc_count;
    /**\brief Type of stream*/
    STREAM_TYPE     mStreamType;
    Mutex           mutex;
};

#endif /* STREAM_H_ */
