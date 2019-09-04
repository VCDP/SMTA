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

#ifndef __XC_THREAD_H__
#define __XC_THREAD_H__

#include <stdint.h>

#include "platform.h"

#if defined(PLATFORM_OS_WINDOWS)
#include <process.h>
#endif

#include "XC_Time.h"
#include "mutex.h"
#include "XC_Event.h"

#if defined(PLATFORM_OS_WINDOWS)
#define XC_THREAD_CALLCONVENTION __stdcall
#else
#define XC_THREAD_CALLCONVENTION
#endif

namespace XC {

class Thread
{
public:

    typedef uint32_t (XC_THREAD_CALLCONVENTION * ThreadProc_t)(void *);

public:
    Thread(void);

    virtual ~Thread(void);

    // Check thread status
    bool IsValid(void);

    // Create new thread
    XC::Status Create(ThreadProc_t func, void *arg);

    // Wait until thread does exit
    void Wait(unsigned int timeout = XC::XC_WAIT_INFINITE);

    // Close thread object
    void Close(void);

protected:
#if defined(PLATFORM_OS_WINDOWS)
    static unsigned int ThreadLaunch(void *arg);
#else
    static void* ThreadLaunch(void *arg);
#endif


protected:
#if defined(PLATFORM_OS_WINDOWS)
    HANDLE m_Handle;   // handle to system thread
#else
    pthread_t       m_Handle;
#endif
    Mutex           m_Mutex;
    XC::Event       m_ExitEvent;
    int             m_WaitCount;
    bool            m_bIsValid;
    ThreadProc_t    m_ThreadFunc;
    void*           m_ThreadFuncArg;
};

} // ns

#endif // __XC_THREAD_H__
