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
 *\file trace_file.h
 *\brief Definition of trace file
 */

#ifndef _TRACE_FILE_H_
#define _TRACE_FILE_H_

#include <fstream>
#include <string>

#define MAX_PATH_NAME_LEN 256
#if _MSC_VER
#define snprintf _snprintf_s
#endif

class TraceInfo;

class TraceFile
{
public:
    TraceFile();
    ~TraceFile();

    void add_trace(TraceInfo *);
    static TraceFile *instance(void);
    void init(const char *filename = NULL);

private:
    void add_log(const char *msg);
    void check(void);
    void backup(void);
    void open_file(void);

private:
    bool no_file_;
    std::ofstream file_;
    long size_;

    static TraceFile *instance_;

    static std::string LOG_FILE;

private:
    TraceFile(const TraceFile &);
    TraceFile operator=(const TraceFile &);
};

#endif

