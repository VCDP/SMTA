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

/**
 *\file trace.cpp
 *\brief Implementation for Trace
 */

#include "trace.h"

#include <stdarg.h>
#include <string.h>

#include "trace_file.h"
#include "trace_filter.h"
#include "trace_info.h"
#include "os/mutex.h"

using namespace std;

TracePool Trace::pool_;
TraceFilter Trace::filter;
Trace *Trace::instance = NULL;
std::queue<TraceInfo *> Trace::queue_;
static Mutex mutex;

uint32_t XC_THREAD_CALLCONVENTION RunThread(void* pArguments)
{
    Trace *tc = (Trace*) pArguments;
    return tc->Run();
}

Trace::Trace(void) :
    mod_(APP_START_MOD),
    level_(SYS_ERROR),
    file_(NULL),
    line_(0),
    running_(false)
{
}

Trace::~Trace(void)
{
    running_ = false;
}

void Trace::operator()(const char *fmt, ...)
{
    if (!running_) {
        return;
    }

    if (!filter.enable(mod_, level_, 0)) {
        return;
    }

    char temp[MAX_DESC_LEN] = {0};
    char buffer[MAX_DESC_LEN] = {0};
    const char *format = fmt;
    int real_len = 0;

#ifdef DEBUG
    if ((real_len = snprintf(temp, MAX_DESC_LEN - 1, "%s,%d: %s", file_, line_, fmt)) < 0)
#else
    if ((real_len = snprintf(temp, MAX_DESC_LEN - 1, "%s", fmt)) < 0)
#endif
    {
        temp[MAX_DESC_LEN - 1] = 0;
    }
    if (real_len > MAX_DESC_LEN - 1) {
        memset(temp, 0, MAX_DESC_LEN);
        snprintf(temp, MAX_DESC_LEN - 1, "Input String is longer than limited size %d\n", MAX_DESC_LEN);
        format = temp;
    }
    va_list args;
    va_start(args, format);

    if (vsnprintf(buffer, MAX_DESC_LEN - 1, temp, args) < 0) {
        buffer[MAX_DESC_LEN - 1] = 0;
    }

    va_end(args);
    TraceInfo *info = new TraceInfo(mod_, level_, buffer);

    if (info == NULL) {
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        queue_.push(info);
    }
}

void Trace::operator()(TraceBase *obj, const char *fmt, ...)
{
    if (!running_) {
        return;
    }

    if (!filter.enable(mod_, level_, obj)) {
        return;
    }

    char temp[MAX_DESC_LEN] = {0};
    char buffer[MAX_DESC_LEN] = {0};
    const char *format = fmt;
    int real_len = 0;

#ifdef DEBUG
    if ((real_len = snprintf(temp, MAX_DESC_LEN - 1, "%s,%d: %s", file_, line_, fmt)) < 0)
#else
    if ((real_len = snprintf(temp, MAX_DESC_LEN - 1, "%s", fmt)) < 0)
#endif
    {
        temp[MAX_DESC_LEN - 1] = 0;
    }

    if (real_len > MAX_DESC_LEN - 1) {
        memset(temp, 0, MAX_DESC_LEN);
        snprintf(temp, MAX_DESC_LEN - 1, "Input String is longer than limited size %d\n", MAX_DESC_LEN);
        format = temp;
    }
    va_list args;
    va_start(args, format);
    if (vsnprintf(buffer, MAX_DESC_LEN - 1, temp, args) < 0) {
        buffer[MAX_DESC_LEN - 1] = 0;
    }


    va_end(args);
    TraceInfo *info = new TraceInfo(mod_, level_, buffer);

    if (info == NULL) {
        return;
    }

    {
        Locker<Mutex> lock(mutex);
        queue_.push(info);
    }
}

Trace* Trace::Instance(void)
{
    if (NULL == instance) {
        instance = new Trace();
    }

    return instance;
}

Trace& Trace::Instance(AppModId mod, TraceLevel level, const char *file, int line)
{
    Trace* in = Instance();

    //file is a relative path, but only need to display the filename
    const char *pos = strrchr(file, '/');

    if (pos) {
        in->SetFile(pos + 1);
    }
    in->SetModLevel(mod, level);

    return *in;
}

void Trace::start()
{
    Instance()->running_ = true;
    Instance()->Create(RunThread, (void*)instance);

    TRC_DEBUG("Succeed to create trace thread");
}

void Trace::stop()
{
    Instance()->running_ = false;
    Instance()->Close();

    TRC_DEBUG("Stop trace thread successfully.");
}

uint32_t Trace::Run(void)
{
    TraceInfo *info = NULL;

    while (running_) {
        XC::Sleep(200);

        while (!queue_.empty()) {
            Locker<Mutex> lock(mutex);
            info = queue_.front();
            pool_.add(info);
            TraceFile::instance()->add_trace(info);
            queue_.pop();
            info = NULL;
        }
    }

    pool_.clear();

    return 0;
}

void Trace::SetModLevel(AppModId mod, TraceLevel level)
{
    mod_ = mod;
    level_ = level;
}
