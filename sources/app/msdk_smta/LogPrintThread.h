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

#ifndef __LOG_PRINT_THREAD_H__
#define __LOG_PRINT_THREAD_H__

#include <memory>
#include <fstream>

#include "os/XC_Thread.h"
#include "base/measurement.h"

class LogPrintThread : private XC::Thread
{
    public:
        LogPrintThread(Measurement  *measurement);
        virtual ~LogPrintThread();

    private:   // Prevent copy and assignment
        LogPrintThread& operator = (const LogPrintThread& rhs);
        LogPrintThread(const LogPrintThread& other);

        static uint32_t XC_THREAD_CALLCONVENTION Run(void* pArguments);

        static const std::string LOG_PATH;
        Measurement  *measurement;
        bool bStop;
        unsigned long interval;
        std::ofstream out;
        std::string name;

    public:
        XC::Status Start(unsigned long interval, std::string name); //interal is msec
        void Stop();
        void PrintLog(void);
};

#endif // __LOG_PRINT_THREAD_H__
