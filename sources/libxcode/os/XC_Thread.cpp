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

#include "XC_Thread.h"

#if defined(PLATFORM_OS_LINUX)
#include <unistd.h>
#endif

#include "base/mutex_locker.h"

XC::Thread::Thread(void) : m_Handle(0), m_ExitEvent(true), m_WaitCount(0), m_bIsValid(false), m_ThreadFunc(NULL), m_ThreadFuncArg(NULL) {
}

XC::Thread::~Thread(void) {
}

bool XC::Thread::IsValid(void)
{
    return m_bIsValid;
}

#if defined(PLATFORM_OS_WINDOWS)
unsigned int XC::Thread::ThreadLaunch(void *arg)
#else
void* XC::Thread::ThreadLaunch(void *arg)
#endif
{
    XC::Thread* pThread = (XC::Thread*)arg;

    pThread->m_ThreadFunc(pThread->m_ThreadFuncArg);

    pThread->m_ExitEvent.Signal();

#if defined(PLATFORM_OS_WINDOWS)
    return 0;
#else
    return ((void *) 1);
#endif
}

XC::Status XC::Thread::Create(ThreadProc_t func, void *arg)
{
    if (func == NULL)
        return XC_NULL_ARGUMENT;

    XC::Status sts = XC_OPERATION_FAILED;
#if defined(PLATFORM_OS_LINUX)
    pthread_attr_t attr;
#endif

    {
        Locker<Mutex> guard(m_Mutex);

        m_ThreadFunc = func;
        m_ThreadFuncArg = arg;

#if defined(PLATFORM_OS_WINDOWS)
        m_Handle = (HANDLE)_beginthreadex(0, 0, XC::Thread::ThreadLaunch, (void*)this, 0, 0);
        if (m_Handle != 0L && m_Handle != (HANDLE)-1L) {
            m_bIsValid = true;
            sts = XC_SUCCESS;
        }
    }
#else
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, geteuid() ? SCHED_OTHER : SCHED_RR);

        if (0 == pthread_create(&m_Handle,
                                &attr,
                                XC::Thread::ThreadLaunch,
                                (void*)this)) {
            m_bIsValid = true;
            sts = XC_SUCCESS;
        }
    }
    pthread_attr_destroy(&attr);
#endif

    //SetPriority(XC_THREAD_PRIORITY_LOWEST);

    return sts;
}

void XC::Thread::Wait(unsigned int timeout)
{
    {
        Locker<Mutex> guard(m_Mutex);
        m_WaitCount++;
    }

    m_ExitEvent.Wait(timeout);

    {
        Locker<Mutex> guard(m_Mutex);
        m_WaitCount--;
        if (0 == m_WaitCount) { // We are the last one...
#if defined(PLATFORM_OS_WINDOWS)
            WaitForSingleObject(m_Handle, timeout);
            CloseHandle(m_Handle);
            m_Handle = NULL;
#else
            pthread_join(m_Handle, NULL);
#endif
            m_bIsValid = false;
        }
    }
}

void XC::Thread::Close(void)
{
    if (IsValid()) {
        Wait();
    }
}
