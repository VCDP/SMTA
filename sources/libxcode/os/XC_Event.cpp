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

#include "XC_Event.h"

#include <errno.h>

#include "XC_Time.h"

#ifndef XC_EVENT_DEBUG
#define XC_EVENT_DEBUG(A)
#endif

using namespace XC;

#if defined(PLATFORM_OS_WINDOWS)
Event::Event(bool manual) : m_Handle(NULL)
#else
Event::Event(bool manual) : m_State(-1), m_bManual(manual)
#endif
{
#if defined(PLATFORM_OS_WINDOWS)
     m_Handle = CreateEvent(NULL, manual, FALSE, NULL);
#else
    int rc = pthread_cond_init(&m_Cond, 0);
    if (rc == 0) {
        m_State = 0;  // Not signaled
    }
#endif
}

Event::~Event()
{
#if defined(PLATFORM_OS_WINDOWS)
    CloseHandle(m_Handle);
#else
    if (m_State >= 0) {
        pthread_cond_destroy(&m_Cond);
    }
#endif
}

XC::Status Event::Wait(unsigned int msTimeout)
{
#if defined(PLATFORM_OS_WINDOWS)
    DWORD ret = WaitForSingleObject(m_Handle, msTimeout);
    if (ret == WAIT_OBJECT_0)
        return XC::XC_SUCCESS;
    else if (ret == WAIT_TIMEOUT) {
        return XC::XC_TIMEOUT;
    }
    return XC::XC_OPERATION_FAILED;
#else
    XC_EVENT_DEBUG((_T("%s [%p] msTimeout=%u state=%d\n"), __FUNCTIONW__, this, msTimeout, m_State));

    if (0 <= m_State) {
        Locker<Mutex> guard(m_Mutex);

        XC::Status sts = XC::XC_SUCCESS;

        if (!m_State) {
            if (msTimeout == XC::XC_WAIT_INFINITE) {
                pthread_cond_wait(&m_Cond, m_Mutex.InnerMutex());
                XC_EVENT_DEBUG((_T("%s [%p] cond_wait() returned\n"), __FUNCTIONW__, this, rc));
            }
            else {
                struct timespec ts = {0, 0};
                struct timeval now;
                gettimeofday(&now, NULL);
                ts.tv_sec = now.tv_sec + msTimeout / 1000;
                ts.tv_nsec = (now.tv_usec + (msTimeout % 1000) * 1000) * 1000;
                int rc = pthread_cond_timedwait(&m_Cond, m_Mutex.InnerMutex(), &ts);
                XC_EVENT_DEBUG((_T("%s [%p] cond_timedwait()->%d\n"), __FUNCTIONW__, this, rc));
                if (rc == ETIMEDOUT) {
                    sts = XC::XC_TIMEOUT;
                }
            }
        }

        if (!m_bManual)
            m_State = 0;  // Auto-Reset (while locked after being waken up)

        return sts;
    }
    return XC::XC_NOT_INITIALIZED;
#endif
}

XC::Status Event::Signal()
{
#if defined(PLATFORM_OS_WINDOWS)
    return (SetEvent(m_Handle) ? XC::XC_SUCCESS : XC::XC_OPERATION_FAILED);
#else
    XC_EVENT_DEBUG((_T("%s [%p] state=%d\n"), __FUNCTIONW__, this, m_State));

    if (0 <= m_State) {
        Locker<Mutex> guard(m_Mutex);

        if (0 == m_State) {
            m_State = 1;
            if (m_bManual)
                pthread_cond_broadcast(&m_Cond);
            else
                pthread_cond_signal(&m_Cond);
        }
        return XC::XC_SUCCESS;
    }
    return XC::XC_NOT_INITIALIZED;
#endif
}

XC::Status Event::Reset()
{
#if defined(PLATFORM_OS_WINDOWS)
    return (ResetEvent(m_Handle) ? XC::XC_SUCCESS : XC::XC_OPERATION_FAILED);
#else
    XC_EVENT_DEBUG((_T("%s [%p]\n"), __FUNCTIONW__, this));

    if (0 <= m_State) {
        Locker<Mutex> guard(m_Mutex);
        m_State = 0;
        return XC::XC_SUCCESS;
    }
    return XC::XC_NOT_INITIALIZED;
#endif
}
