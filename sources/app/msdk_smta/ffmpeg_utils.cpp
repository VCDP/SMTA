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

#include "ffmpeg_utils.h"
#include "sample_utils.h"
#include "debug_stream.h"
#include "os/mutex.h"

#ifdef __cplusplus
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#endif

static Mutex s_Mutex;

void av_log_callback(void* stuf, int level, const char* fmt, va_list va)
{
    const int BUF_SZ = 2*1024;
    char avBuf[BUF_SZ];
#if defined(PLATFORM_OS_WINDOWS)
    vsnprintf_s(avBuf, BUF_SZ, _TRUNCATE, fmt, va);
#else
    _vsnprintf(avBuf, BUF_SZ, fmt, va);
#endif
    if (level <= AV_LOG_ERROR) {
        DEBUG_ERROR((_T("libav: %s "), avBuf ));
    } else if (level <= AV_LOG_INFO) {
        DEBUG_MSG(( _T("libav: %s "), avBuf ));
    }
}

void EnsureFFmpegInit()
{
    static bool done = false;
    {   // Just once!
        Locker<Mutex> locker(s_Mutex);

        if (!done) {
#ifndef NDEBUG
            av_log_set_level(AV_LOG_DEBUG);
            av_log_set_callback(av_log_callback);
#else
            av_log_set_level(AV_LOG_INFO);
#endif

            av_log_set_flags(AV_LOG_SKIP_REPEATED);
            av_register_all();
            avformat_network_init();
            done = true;
        }
    }
}
