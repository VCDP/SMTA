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

#ifndef __COMMON_DEFS_H__
#define __COMMON_DEFS_H__

#include <string>
#include <sstream>

#include "os/platform.h"
#if defined(PLATFORM_OS_LINUX)
#include "common_unix.h"
#endif

// Application exit codes
enum SMTA_EXIT_CODES {
    SMTA_EXIT_SUCCESS = 0,

    SMTA_EXIT_INVALID_PARAM,      // Invalid parameter value specified
    SMTA_EXIT_NO_DATA,            // No data in input bitstream
    SMTA_EXIT_INPUT_TIMEOUT,      // Timeout waiting for incoming data
    SMTA_EXIT_HEARTBEAT,          // Heartbeat missed (no data produced)

    SMTA_EXIT_FAILURE = 100,      // Internal failure
};

// RTP timestamp frequencies
#define RTP_TIMESTAMP_FREQ_VIDEO 90000.0
#define RTP_TIMESTAMP_FREQ_AUDIO 1000.0

#define DEFAULT_MTU 1300  // Default max size of an outgoing packet

// RTP Payload Types
#define DYNAMIC_RTP_AUDIO_PAYLOAD_TYPE 96
#define DYNAMIC_RTP_VIDEO_PAYLOAD_TYPE 97
#define RTP_MPEG2_PAYLOAD_TYPE 32

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstring;
typedef std::basic_ostringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tostringstream;

template<class T>
tstring to_tstring(const T& source)
{
  tostringstream oss;
  oss << source;
  return oss.str();
};

#endif  // __COMMON_DEFS_H__
