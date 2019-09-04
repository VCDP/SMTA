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
#ifndef MPEGTS_H
#define MPEGTS_H

#include <map>

#include "base_demuxer.h"

typedef enum {
    RESERVED = 0,
    MPEG1_VIDEO = 1,
    MPEG2_VIDEO = 2,
    MPEG1_AUDIO = 3,
    MPEG2_AUDIO = 4,
    ADTS_AUDIO = 0x0f,
    AVC_VIDEO = 0x1b,
    VC1_VIDEO = 0xea
} TransportStreamType;


// First implement a simple demux.
// In future, will implement a demuxer which can dynamically output stream.
class MpegTsDemux: public BaseDemux
{
public:
    MpegTsDemux();
    ~MpegTsDemux();

private:
    unsigned int pmt_pid_;
    bool pmt_parsed_;
    std::map<unsigned int, void *> pid_mempool_map_;

    void AnalyzePAT(unsigned char *pkt, unsigned size);
    void AnalyzePMT(unsigned char *pkt, unsigned size);
    int DemultiplexStream(unsigned char *buf, unsigned buf_size);
    int SetEOF();
    StreamType GetStreamType(unsigned ts_stream_type);
};
#endif

