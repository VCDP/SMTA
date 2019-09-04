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
 *\file trace_info.h
 *\brief Implementation for trace info class
 */

#ifndef _TRACE_INFO_H_
#define _TRACE_INFO_H_

#include <string>
#include <ctime>
#include "app_def.h"

class TraceInfo
{
private:
    AppModId mod_;
    TraceLevel level_;
    time_t time_;
    std::string desc_;

public:
    TraceInfo(AppModId mod, TraceLevel level, std::string desc):
        mod_(mod), level_(level) {
        desc_.erase();
        desc_ = desc;
        time_ = std::time(0);
    }

    AppModId mod() {
        return mod_;
    }
    TraceLevel level() {
        return level_;
    }
    time_t *time() {
        return &time_;
    }
    std::string &desc() {
        return desc_;
    }
};

#endif

