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

#include "sample_defs.h"
#include "common_defs.h"
#include "sample_utils.h"
#include <sstream>
#include <math.h>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#define _getpid getpid
#include <stdarg.h>
#include <stdlib.h>
#include <algorithm>
#endif

#ifdef __cplusplus
extern "C" {
#include "libavutil/pixfmt.h"
}
#endif

mfxI64 CTimer::frequency = 0;

mfxU16 GetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize)
{
    if (pSurfacesPool)
    {
        for (mfxU16 i = 0; i < nPoolSize; i++)
        {
            if (0 == pSurfacesPool[i].Data.Locked)
            {
                return i;
            }
        }
    }

    return MSDK_INVALID_SURF_IDX;
}

mfxU16 GetFreeSurface(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize)
{
    mfxU32 SleepInterval = 10; // milliseconds

    mfxU16 idx = MSDK_INVALID_SURF_IDX;

    CTimer t;
    t.Start();
    //wait if there's no free surface
    do
    {
        idx = GetFreeSurfaceIndex(pSurfacesPool, nPoolSize);

        if (MSDK_INVALID_SURF_IDX != idx)
        {
            break;
        }
        else
        {
            usleep(1000*SleepInterval);
        }
    } while ( t.GetTime() < MSDK_SURFACE_WAIT_INTERVAL / 1000 );

    if(idx==MSDK_INVALID_SURF_IDX)
    {
        printf("ERROR: No free surfaces in pool (during long period)\n");
    }

    return idx;
}

mfxStatus ConvertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD)
{
    MSDK_CHECK_POINTER(pnFrameRateExtN, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pnFrameRateExtD, MFX_ERR_NULL_PTR);

    mfxU32 fr;

    fr = (mfxU32)(dFrameRate + .5);

    if (fabs(fr - dFrameRate) < 0.0001)
    {
        *pnFrameRateExtN = fr;
        *pnFrameRateExtD = 1;
        return MFX_ERR_NONE;
    }

    fr = (mfxU32)(dFrameRate * 1.001 + .5);

    if (fabs(fr * 1000 - dFrameRate * 1001) < 10)
    {
        *pnFrameRateExtN = fr * 1000;
        *pnFrameRateExtD = 1001;
        return MFX_ERR_NONE;
    }

    *pnFrameRateExtN = (mfxU32)(dFrameRate * 10000 + .5);
    *pnFrameRateExtD = 10000;

    return MFX_ERR_NONE;
}

mfxF64 CalculateFrameRate(mfxU32 nFrameRateExtN, mfxU32 nFrameRateExtD)
{
    if (nFrameRateExtN && nFrameRateExtD)
        return (mfxF64)nFrameRateExtN / nFrameRateExtD;
    else
        return 0;
}

mfxStatus InitMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize)
{
    //check input params
    MSDK_CHECK_POINTER(pBitstream, MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(nSize, 0, MFX_ERR_NOT_INITIALIZED);

    //prepare pBitstream
    WipeMfxBitstream(pBitstream);

    //prepare buffer
    pBitstream->Data = new mfxU8[nSize];
    MSDK_CHECK_POINTER(pBitstream->Data, MFX_ERR_MEMORY_ALLOC);

    pBitstream->MaxLength = nSize;

    return MFX_ERR_NONE;
}

mfxStatus ExtendMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize)
{
    MSDK_CHECK_POINTER(pBitstream, MFX_ERR_NULL_PTR);

    MSDK_CHECK_ERROR(nSize <= pBitstream->MaxLength, true, MFX_ERR_UNSUPPORTED);

    mfxU8* pData = new mfxU8[nSize];
    MSDK_CHECK_POINTER(pData, MFX_ERR_MEMORY_ALLOC);

    memmove(pData, pBitstream->Data + pBitstream->DataOffset, pBitstream->DataLength);

    WipeMfxBitstream(pBitstream);

    pBitstream->Data       = pData;
    pBitstream->DataOffset = 0;
    pBitstream->MaxLength  = nSize;

    return MFX_ERR_NONE;
}

void WipeMfxBitstream(mfxBitstream* pBitstream)
{
    MSDK_CHECK_POINTER_NO_RET(pBitstream);

    //free allocated memory
    MSDK_SAFE_DELETE_ARRAY(pBitstream->Data);
}

const TCHAR* CodecIdToStr(mfxU32 nCodecId)
{
    switch(nCodecId)
    {
    case MFX_CODEC_AVC:
        return _T("AVC");
    case MFX_CODEC_HEVC:
        return _T("HEVC");
    case MFX_CODEC_VC1:
        return _T("VC1");
    case MFX_CODEC_MPEG2:
        return _T("MPEG2");
    case MFX_CODEC_NULL:
        return _T("NULL");
    default:
        return _T("unsupported");
    }

}

const TCHAR* TargetUsageToStr(mfxU16 tu)
{
    switch(tu)
    {
    case MFX_TARGETUSAGE_BALANCED:
        return _T("balanced");
    case MFX_TARGETUSAGE_BEST_QUALITY:
        return _T("quality");
    case MFX_TARGETUSAGE_BEST_SPEED:
        return _T("speed");
    case MFX_TARGETUSAGE_UNKNOWN:
        return _T("unknown");
    default:
        return _T("unsupported");
    }
}

// function for getting a pointer to a specific external buffer from the array
mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId)
{
    if (!ebuffers) return 0;
    for(mfxU32 i=0; i<nbuffers; i++) {
        if (!ebuffers[i]) continue;
        if (ebuffers[i]->BufferId == BufferId) {
            return ebuffers[i];
        }
    }
    return 0;
}

mfxStatus ConvertStringToGuid(const std::basic_string<TCHAR> & string_guid, mfxPluginUID & mfx_guid)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 i   = 0;
    mfxU32 hex = 0;
    for(i = 0; i != 16; i++)
    {
        hex = 0;

#if defined(_WIN32) || defined(_WIN64)
        if (1 != _stscanf_s(string_guid.c_str() + 2*i, _T("%2x"), &hex))
#else
        if (1 != sscanf(string_guid.c_str() + 2*i, _T("%2x"), &hex))
#endif
        {
            sts = MFX_ERR_UNKNOWN;
            break;
        }
        if (hex == 0 && string_guid.c_str() + 2*i != _tcsstr((const TCHAR*)string_guid.c_str() + 2*i,  _T("00")))
        {
            sts = MFX_ERR_UNKNOWN;
            break;
        }
        mfx_guid.Data[i] = (mfxU8)hex;
    }
    return sts;
}

std::string s_SessionName;

void SetSessionName(const std::string& name)
{
    s_SessionName = name;
    if (!s_SessionName.empty()) {
#ifdef _WIN32
        TCHAR title[512];
        _sntprintf(title, sizeof(title)/sizeof(title[0]), _T("Streaming Media Transcoder - ") _T(PRIaSTR), name.c_str());
        SetConsoleTitle(title);
#endif
    }
}

const char* GetSessionName()
{
    if (!s_SessionName.empty()) {
        return s_SessionName.c_str();
    }
    return "";
}

inline uint64_t CalculateMicroseconds(uint64_t perfCounter, uint64_t perfFreq)
{
    uint64_t f = perfFreq;
    uint64_t a = perfCounter;
    uint64_t b = a/f;
    uint64_t c = a%f; // a = b*f+c => (a*1000000)/f = b*1000000+(c*1000000)/f

    return (b * (uint64_t)1000000L) + (c * (uint64_t)1000000L) / f;
}

mfxU32 Str2FourCC(const std::string& strInput)
{
    mfxU32 fourcc = MFX_FOURCC_YV12;//default

    std::string tu = strInput;

    if (0 == tu.compare("yv12"))
    {
        fourcc = MFX_FOURCC_YV12;
    }
    else if (0 == tu.compare("rgb3"))
    {
        fourcc = MFX_FOURCC_RGB3;
    }
    else if (0 == tu.compare("rgb4"))
    {
        fourcc = MFX_FOURCC_RGB4;
    }
    else if (0 == tu.compare("yuy2"))
    {
        fourcc = MFX_FOURCC_YUY2;
    }
    else if (0 == tu.compare("nv12"))
    {
        fourcc = MFX_FOURCC_NV12;
    }

    return fourcc;
}


CodecType MfxTypeToMsdkType(mfxU32 mfxType)
{
    CodecType type;
    switch(mfxType) {
        case MFX_CODEC_AVC:
            type = CODEC_TYPE_VIDEO_AVC;
            break;
        case MFX_CODEC_MPEG2:
            type = CODEC_TYPE_VIDEO_MPEG2;
            break;
        case MFX_CODEC_HEVC:
            type = CODEC_TYPE_VIDEO_HEVC;
            break;
        default:
            type = CODEC_TYPE_VIDEO_VP8;
            break;
    }

    return type;
}

double CurrentTime()
{
#ifdef _WIN32
    static bool first = true;
    static unsigned __int64 microseconds, init_time_ms;
    static LARGE_INTEGER perfFreq;

    LARGE_INTEGER perfCounter;

    QueryPerformanceCounter(&perfCounter);

    if (first) {
        first = false;
        QueryPerformanceFrequency(&perfFreq);

        SYSTEMTIME sysTime;
        FILETIME fileTime;
        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        microseconds = ( ((uint64_t)(fileTime.dwHighDateTime) << 32) + (uint64_t)(fileTime.dwLowDateTime) ) / (uint64_t)10;
        microseconds -= (uint64_t)11644473600000000L; // EPOCH
        init_time_ms = CalculateMicroseconds(perfCounter.QuadPart, perfFreq.QuadPart);
    }

    unsigned __int64 emulate_time_ms, ms_diff;
    emulate_time_ms = CalculateMicroseconds(perfCounter.QuadPart, perfFreq.QuadPart);

    ms_diff = emulate_time_ms - init_time_ms;

    return (double)(microseconds + ms_diff) / 1000000.0f;
#else
    static XC::XC_TIME_TICK startTime = 0L;
    if (startTime == 0L)
        startTime = XC::GetTick();
    return (double)XC::GetTime(startTime);
#endif
}

void debug_printf(const TCHAR* fmt, ...)
{
    const int BUF_SZ = 512;
    TCHAR msg[BUF_SZ];

#ifdef DEBUG_PRINTF_WITH_TIME
#ifdef _WIN32
    const TCHAR* time_fmt = _T("%010.4f | ");
    const int extra_sz = 0;
#else
    const TCHAR* time_fmt = _T("%015.4f | ");  // On Linux, the field size for floats includes the decimal point and following decimal part digits
    const int extra_sz = 1;  // max size is also interpreted different on Win and Unix
#endif
    const int time_sz = 10+1+4 + 3;

    double now = CurrentTime();
    _sntprintf(msg, time_sz + extra_sz, time_fmt, now);
#else
    const int time_sz = 0;
#endif

#ifdef DEBUG_PRINTF_WITH_THREAD_ID
#ifdef _WIN32
    const TCHAR* threadid_fmt = _T("%04x | ");
    const int threadid_sz = 4 + 3;
#else
    const TCHAR* threadid_fmt = _T("%08x | ");
    const int threadid_sz = 8 + 3;
#endif

    _sntprintf(msg + time_sz, threadid_sz + extra_sz, threadid_fmt, GetCurrentThreadId());
#else
    const int threadid_sz = 0;
#endif
    va_list ap;
    va_start(ap, fmt);
    (void)_vsntprintf(msg + time_sz + threadid_sz, BUF_SZ-time_sz-threadid_sz-extra_sz, fmt, ap);
    va_end(ap);

#ifdef DEBUG_PRINTF_TO_FILE
    static FILE* fp = NULL;
    if (fp == NULL) {
        std::ostringstream os;
        os << "xcoder.";
        if (s_SessionName.empty()) {
            os << _getpid();
        }
        else {
            os << s_SessionName;
#ifdef PID_LOG_FILE
            os << "." << getpid();
#endif
        }
        os << ".log";
        fp = fopen(os.str().c_str(), "w");
        if (fp == NULL) {
            fprintf(stderr, "ERROR: Failed to open log file\n");
            exit(SMTA_EXIT_FAILURE);
        }
    }
    _ftprintf(fp, "%s", msg);
    fflush(fp);
#else
    OutputDebugString(msg);
#endif
}

std::string FromTString(const TCHAR* value) {
#ifdef UNICODE
    char mbcs[256] = {0};
    wcstombs(mbcs, value, sizeof(mbcs));
    return std::string(mbcs);
#else
    return value;
#endif
}

std::string FromTString(const tstring& value) {
#ifdef UNICODE
    char mbcs[256] = {0};
    wcstombs(mbcs, value.c_str(), sizeof(mbcs));
    return std::string(mbcs);
#else
    return value;
#endif
}
#ifdef _WIN32
std::string ltrim(std::string s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                    std::not1(std::ptr_fun<int, int>(isspace))));
    return s;
}

// trim from end
std::string rtrim(std::string s) {
    s.erase(
            std::find_if(s.rbegin(), s.rend(),
                    std::not1(std::ptr_fun<int, int>(isspace))).base(),
            s.end());
    return s;
}
#else
std::string ltrim(std::string s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                    std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string rtrim(std::string s) {
    s.erase(
            std::find_if(s.rbegin(), s.rend(),
                    std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
            s.end());
    return s;
}
#endif


// trim from both ends
std::string trim(std::string s) {
    return ltrim(rtrim(s));
}

int ConvertPixelFormat(int type)
{
    switch (type){
        case MFX_FOURCC_NV12:
            return AV_PIX_FMT_YUV420P;
        case MFX_FOURCC_RGB4:
            return AV_PIX_FMT_RGB4;
        default:
            return -1;
    }
}

// 1 ms provides better result in range [0..5] ms
#define DEVICE_WAIT_TIME 1

// This function either performs synchronization using provided syncpoint, or just waits for predefined time if syncpoint is already 0 (this usually happens if syncpoint was already processed)
void WaitForDeviceToBecomeFree(MFXVideoSession& session, mfxSyncPoint& syncPoint,mfxStatus& currentStatus)
{
    // Wait 1ms will be probably enough to device release
    if (syncPoint) {
        mfxStatus stsSync = session.SyncOperation(syncPoint, DEVICE_WAIT_TIME);
        if (MFX_ERR_NONE == stsSync) {
            // retire completed sync point (otherwise we may start active polling)
            syncPoint = NULL;
        } else if (stsSync < 0) {
            currentStatus = stsSync;
        }
    } else {
        usleep(1000*DEVICE_WAIT_TIME);
    }
}
