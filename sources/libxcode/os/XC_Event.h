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

#ifndef __XC_EVENT_H__
#define __XC_EVENT_H__

#include "platform.h"
#if defined(PLATFORM_OS_LINUX)
#include <pthread.h>
#endif

#include "mutex.h"

namespace XC {

class Event {
public:
    Event(bool manual = false);
    ~Event();

    XC::Status Wait(unsigned int msTimeout);
    XC::Status Signal();
    XC::Status Reset();

private:
#if defined(PLATFORM_OS_LINUX)
    pthread_cond_t  m_Cond;
    Mutex           m_Mutex;
    int             m_State;
    bool            m_bManual;
#else
    HANDLE m_Handle;
#endif
};

} // ns

#endif // __XC_EVENT_H__
