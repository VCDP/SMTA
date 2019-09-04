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

#ifndef __SAMPLE_UTILS_H__
#define __SAMPLE_UTILS_H__

#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <sys/time.h>

#include <mfxvideo++.h>

#include "mfxstructures.h"
#include "mfxplugin.h"
#include "video/msdk_xcoder.h"
#include "common_defs.h"

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
        TypeName(const TypeName&);               \
        void operator=(const TypeName&)


//
// DEBUG Print / Logging
//
#define DEBUG_PRINTF_WITH_TIME
#define DEBUG_PRINTF_WITH_THREAD_ID
#define DEBUG_PRINTF_TO_FILE

void debug_printf(const TCHAR* fmt, ...);

#if !defined(NDEBUG) && !defined(NO_DEBUG_MSG)
// Comment out (or use NO_DEBUG_MSG define) to have a debug build that doesn't print/OutputDebugString too much
#define DO_DEBUG_MSG
#endif

#ifdef DO_DEBUG_MSG
#define DEBUG_MSG(A) debug_printf A
#else
#define DEBUG_MSG(A)
#endif

//#define NO_VERBOSE_LOG
//#define VERBOSE_LOG

#ifdef _DEBUG
#ifndef NO_VERBOSE_LOG
#define VERBOSE_LOG
#endif
#endif

#define RUNTIME_LOG_SELECT

#define DEBUG_FORCE(A) debug_printf A

// Run-time selectable logging:
#ifdef RUNTIME_LOG_SELECT

extern unsigned int s_TranscoderLogFlags;

// Define constants for logging control
enum e_LogFlag {
#define DEF_LOG_FLAG(NAME, VAL) XC_LOG_##NAME = VAL,
#include "log_flags.h"
#undef DEF_LOG_FLAG
    XC_LOG_LAST
};

#define XC_LOG(flags, A) \
    do { \
        if (flags & s_TranscoderLogFlags) { debug_printf A; } \
    } while(0) \

#define DEBUG_ERROR(A) debug_printf A

#define DEBUG_PAYLOAD_DTOR(A) XC_LOG( XC_LOG_PAYLOAD_DTOR, A )
#define DEBUG_VIDEO(A) XC_LOG( XC_LOG_VIDEO, A )
#define DEBUG_VIDEO_DETAIL(A) XC_LOG( XC_LOG_VIDEO_DETAIL, A )
#define DEBUG_DISPATCH(A) XC_LOG( XC_LOG_DISPATCH, A )
#define DEBUG_SYNC_START(A) XC_LOG( XC_LOG_SYNC_START, A )
#define DEBUG_TIME(A) XC_LOG( XC_LOG_TIME, A )
#define DEBUG_LOOP(A) XC_LOG( XC_LOG_LOOP, A )

#else

// Some specific debug areas that can be enabled when needed
#ifdef VERBOSE_LOG
// == Video ==
#define DEBUG_VIDEO(A) debug_printf A
#define DEBUG_VIDEO_DETAIL(A) debug_printf A
// == Common ==
#define DEBUG_ERROR(A) debug_printf A
#define DEBUG_PAYLOAD_DTOR(A) //debug_printf A
#define DEBUG_DISPATCH(A) debug_printf A
#define DEBUG_SYNC_START(A) debug_printf A
#define DEBUG_TIME(A) //debug_printf A
#define DEBUG_LOOP(A) debug_printf A
#else
// == Video ==
#define DEBUG_VIDEO(A) //debug_printf A
#define DEBUG_VIDEO_DETAIL(A) //debug_printf A
// == Common ==
#define DEBUG_ERROR(A) debug_printf A
#define DEBUG_PAYLOAD_DTOR(A) //debug_printf A
#define DEBUG_DISPATCH(A) //debug_printf A
#define DEBUG_SYNC_START(A) //debug_printf A
#define DEBUG_TIME(A) //debug_printf A
#define DEBUG_LOOP(A) debug_printf A
#endif
#endif

/** Helper class to measure execution time of some code. Use this class
 * if you need manual measurements.
 *
 * Usage example:
 * {
 *   CTimer timer;
 *   msdk_tick summary_tick;
 *
 *   timer.Start()
 *   function_to_measure();
 *   summary_tick = timer.GetDelta();
 *   printf("Elapsed time 1: %f\n", timer.GetTime());
 *   ...
 *   if (condition) timer.Start();
     function_to_measure();
 *   if (condition) {
 *     summary_tick += timer.GetDelta();
 *     printf("Elapsed time 2: %f\n", timer.GetTime();
 *   }
 *   printf("Overall time: %f\n", CTimer::ConvertToSeconds(summary_tick);
 * }
 */
class CTimer
{
public:
    CTimer():
        start(0)
    {
    }
    static mfxI64 GetFrequency()
    {
        if (!frequency) frequency = 1000000;
        return frequency;
    }
    static mfxF64 ConvertToSeconds(mfxI64 elapsed)
    {
        return ((mfxF64)(elapsed-0)/(mfxF64)GetFrequency());
    }

    inline void Start()
    {
        start = msdk_time_get_tick();
    }
    inline mfxI64 GetDelta()
    {
        return msdk_time_get_tick() - start;
    }
    inline mfxF64 GetTime()
    {
        return ((mfxF64)(msdk_time_get_tick()-start)/(mfxF64)GetFrequency());
    }
    mfxI64 msdk_time_get_tick(void)
    {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        return (mfxI64)tv.tv_sec * (mfxI64)1000000 + (mfxI64)tv.tv_usec;
    }

protected:
    static mfxI64 frequency;
    mfxI64 start;
private:
    CTimer(const CTimer&);
    void operator=(const CTimer&);
};

void SetSessionName(const std::string& name);
const char* GetSessionName();

mfxU16 GetFreeSurface(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize);
mfxU16 GetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize);

mfxStatus ConvertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD);
mfxF64 CalculateFrameRate(mfxU32 nFrameRateExtN, mfxU32 nFrameRateExtD);
mfxStatus InitMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize);
mfxStatus ExtendMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize);
void WipeMfxBitstream(mfxBitstream* pBitstream);
const TCHAR* CodecIdToStr(mfxU32 nFourCC);
const TCHAR* TargetUsageToStr(mfxU16 tu);
const TCHAR* MfxStatusToStr(mfxStatus sts);
mfxU32 Str2FourCC(const std::string& strInput);
CodecType MfxTypeToMsdkType(mfxU32 mfxType);

// function for getting a pointer to a specific external buffer from the array
mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId);
mfxStatus ConvertStringToGuid(const std::basic_string<TCHAR> & sGuid, mfxPluginUID &mfxGuid);
extern double CurrentTime();

//
std::string FromTString(const TCHAR* value);
std::string FromTString(const tstring& value);
std::string ltrim(std::string s);
std::string rtrim(std::string s);
std::string trim(std::string s);
int ConvertPixelFormat(int type);
void WaitForDeviceToBecomeFree(MFXVideoSession& session, mfxSyncPoint& syncPoint,mfxStatus& currentStatus);


#endif //__SAMPLE_UTILS_H__
