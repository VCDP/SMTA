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

#ifndef __TIME_H__
#define __TIME_H__

#include "platform.h"

#include <stdint.h>
#if defined (PLATFORM_OS_LINUX)
#include <unistd.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <pthread.h>
#elif defined (PLATFORM_OS_WINDOWS)
#include <time.h>
#endif

namespace XC {

#if defined(PLATFORM_OS_WINDOWS)
    const int XC_WAIT_INFINITE = INFINITE;
#else
    const int XC_WAIT_INFINITE = -1L;
#endif

    typedef uint64_t XC_TIME_TICK;

    void Sleep(uint32_t millis);
    XC::XC_TIME_TICK GetTick();
    double GetTime(XC::XC_TIME_TICK start);
    double GetTime(XC::XC_TIME_TICK end, XC::XC_TIME_TICK start);
} // ns

#if defined(PLATFORM_OS_WINDOWS)
void usleep(uint32_t micros);
int gettimeofday(struct timeval *tp, void *tzp);
#endif

#endif //__TIME_H__
