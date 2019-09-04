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

#ifndef BASE_DEMUXER_H
#define BASE_DEMUXER_H

#include <pthread.h>

#include "base/media_common.h"
#include "base/mem_pool.h"


typedef enum {
    STREAM_ADD_EVENT = 0,
    NO_MORE_STREAM_EVENT,
    STREAM_REMOVED_EVENT,
    EVENT_LAST
} EventType;

typedef void* (*NewStreamAdded)(StreamType stream_type, void* arg);
typedef void (*NoMoreStream)(void* arg);
typedef void (*StreamRemoved)(void* arg);

class BaseDemux {
public:
    BaseDemux();
    virtual ~BaseDemux();
    int Init(MemPool* in);
    int RegisterEventCallback(EventType evt_type, void* func, void* arg);
    int Start();
    int Stop();
    int Join();

    bool is_running_;
    MemPool* input_mempool_;

    NewStreamAdded new_stream_added_func_;
    void* new_stream_added_func_arg_;

    NoMoreStream no_more_stream_func_;
    void* no_more_stream_func_arg_;
protected:

    // Subclass shall override this function to do the demux work.
    // Return value is the data size which has been demuxed.
    virtual int DemultiplexStream(unsigned char* buf, unsigned buf_size) = 0;

    virtual int SetEOF() = 0;
private:

    pthread_t thread_id_;
    static void* ThreadFunc(void* arg);
};
#endif

