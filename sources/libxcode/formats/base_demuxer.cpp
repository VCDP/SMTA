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

#include <stdio.h>
#include <unistd.h>

#include "base_demuxer.h"

BaseDemux::BaseDemux():is_running_(false), input_mempool_(NULL)
{
}

BaseDemux::~BaseDemux()
{
}


int BaseDemux::Start()
{
    // Create a thread
    if (is_running_) {
        return 0;
    }
    is_running_ = (pthread_create(&thread_id_, NULL, ThreadFunc, this) == 0);
    return 0;
}

int BaseDemux::Join()
{
    if (is_running_) {
        pthread_join(thread_id_, NULL);
    }
    return 0;
}

int BaseDemux::Stop()
{
    if (is_running_) {
        is_running_ = false;
        pthread_join(thread_id_, NULL);
    }
    return 0;
}

void* BaseDemux::ThreadFunc(void* arg)
{
    BaseDemux *dmx = static_cast<BaseDemux*>(arg);
    while (dmx->is_running_) {
        // Doing demux work
        unsigned char* buffer_start = dmx->input_mempool_->GetReadPtr();
        unsigned buffer_length = dmx->input_mempool_->GetFlatBufFullness();
        int demuxed_data_size = dmx->DemultiplexStream(buffer_start, buffer_length);
        if (demuxed_data_size) {
            dmx->input_mempool_->UpdateReadPtr(demuxed_data_size);
        } else {
            // Lack data and got EOF
            if (dmx->input_mempool_->GetDataEof()) {
                // EOF
                printf("[%p]Demuxer got EOF\n", dmx);
                dmx->SetEOF();
                break;
            }
        }
        usleep(10*1000);
    }
    return NULL;
}

int BaseDemux::RegisterEventCallback(EventType evt_type, void* func, void* arg)
{
    switch(evt_type) {
        case STREAM_ADD_EVENT:
            new_stream_added_func_ = (NewStreamAdded)(func);
            new_stream_added_func_arg_ = arg;
            break;
        case NO_MORE_STREAM_EVENT:
            no_more_stream_func_ = (NoMoreStream)(func);
            no_more_stream_func_arg_ = arg;
            break;
        case STREAM_REMOVED_EVENT:
        default:
            break;
    }
    return 0;
}

int BaseDemux::Init(MemPool* in)
{
    if (NULL == in) {
        return -1;
    }

    input_mempool_ = in;
    return 0;
}

