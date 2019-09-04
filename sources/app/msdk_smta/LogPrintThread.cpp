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

#include "LogPrintThread.h"
#ifdef __UNIX__
#include <unistd.h>
#include <dirent.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

const std::string LogPrintThread::LOG_PATH = "/tmp/smta";

LogPrintThread::LogPrintThread(Measurement  *measurement) :
    measurement(measurement), bStop(false), interval(0)
{
}

LogPrintThread::~LogPrintThread()
{
    Stop();
}

XC::Status LogPrintThread::Start(unsigned long interval, std::string name)
{
    this->interval = interval;
    this->name = name;
    return Create(LogPrintThread::Run, (void*)this);
}

void LogPrintThread::Stop(void)
{
    this->bStop = true;
    Close();
}


uint32_t XC_THREAD_CALLCONVENTION LogPrintThread::Run(void* pArguments)
{
    printf("Start log info print thread\n");

    LogPrintThread *obj = (LogPrintThread*) pArguments;
    obj->PrintLog();

    printf("End log info print thread\n");
    return 0;
}

void LogPrintThread::PrintLog(void)
{
    unsigned long ticks = interval / 10;
    unsigned int cnt = 0;
    std::string log;

    if (NULL == measurement) {
        printf("\033[40;41m Params measurement =%p \033[0m \n", measurement);
        return;
    }

    out.open(name.c_str(), std::ios::out);
    while(!this->bStop) {
        if (cnt < 10) {
            XC::Sleep(ticks);
            cnt ++;
            continue;
        }
        cnt = 0;

        log.clear();
        if (out.is_open()) {
            measurement->GetPerformanceInfo(log);
            out.seekp(std::ios::beg);
            out << log << std::endl;
        } else {
            printf("\033[40;41m Open %s error!\033[0m \n", name.c_str());
            this->bStop = true;
        }
    }
    if (out.is_open()) {
        out.close();
    }
}

