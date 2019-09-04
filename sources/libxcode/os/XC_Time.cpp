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

#include "XC_Time.h"

#include <time.h>


void XC::Sleep(uint32_t millis)
{
#if defined(PLATFORM_OS_WINDOWS)
    if (millis) {
        ::Sleep(millis);
    }
#else
    if (millis) {
        usleep(1000 * millis);
    }
    else {
        sched_yield();
    }
#endif
}

XC::XC_TIME_TICK XC::GetTick()
{ // Ticks in microseconds
#if defined(PLATFORM_OS_WINDOWS)
    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);
    return tick.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // CLOCK_PROCESS_CPUTIME_ID
    return (XC::XC_TIME_TICK)ts.tv_sec * 1000000L + (ts.tv_nsec / 1000L);
#endif
}

double XC::GetTime(XC::XC_TIME_TICK start)
{ // Time elapse since start, in seconds
#if defined(PLATFORM_OS_WINDOWS)
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);

    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);
    return (double)((tick.QuadPart - start)/(double)freq.QuadPart);
#else
    XC::XC_TIME_TICK tick = XC::GetTick();
    return ((double)(tick - start))/1000000.0;
#endif
}

double XC::GetTime(XC::XC_TIME_TICK end, XC::XC_TIME_TICK start)
{ // Time elapse since start, in seconds
#if defined(PLATFORM_OS_WINDOWS)
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);

    return ((double)(end - start))/(double)freq.QuadPart;
#else
    return ((double)(end - start))/1000000.0;
#endif
}

#if defined(PLATFORM_OS_WINDOWS)
void usleep(uint32_t microsecond)
{
    uint32_t ms = microsecond / 1000;
    if (0 == ms) {
        for (int i = 0; i < 5; i++) {
            ::Sleep(0);
        }
    }
    else {
        ::Sleep(ms);
    }
}

int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME systime;

    GetLocalTime(&systime);

    tm.tm_year     = systime.wYear - 1900;
    tm.tm_mon     = systime.wMonth - 1;
    tm.tm_mday     = systime.wDay;
    tm.tm_hour     = systime.wHour;
    tm.tm_min     = systime.wMinute;
    tm.tm_sec     = systime.wSecond;
    tm. tm_isdst    = -1;
    clock = mktime(&tm);

    tp->tv_sec = (long)clock;
    tp->tv_usec = systime.wMilliseconds * 1000;

    return 0;
}
#endif

