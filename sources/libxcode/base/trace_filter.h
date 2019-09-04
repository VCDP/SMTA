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
 *\file trace_filter.h
 *\brief Definition for ModFilter & TraceFilter
 */

#ifndef _TRACE_FILTER_H_
#define _TRACE_FILTER_H_

#include "app_def.h"

class TraceInfo;
class TraceBase;

class ModFilter
{
public:
    ModFilter(): enable_(false), level_(SYS_INFORMATION) {}

    void enable(bool flag) {
        enable_ = flag;
    }
    const bool enable() {
        return enable_;
    }

    const bool enable(TraceLevel level) {
        if (enable_ && level <= level_) {
            return true;
        } else {
            return false;
        }
    }

    void level(TraceLevel level) {
        level_ = level;
    }
    const TraceLevel level() {
        return level_;
    }

private:
    bool enable_;
    TraceLevel level_;
};

class TraceFilter
{
private:
    bool enable_;
    TraceLevel level_;
    ModFilter filters_[APP_END_MOD];

public:
    TraceFilter(): enable_(false), level_(SYS_ERROR) {}

    TraceLevel level(AppModId mod) {
        return filters_[mod].level();
    }
    TraceLevel level() {
        return level_;
    }
    void level(TraceLevel level) {
        level_ = level;
    }

    void enable(bool flag) {
        enable_ = flag;
    }
    bool enable() {
        return enable_;
    }

    void enable(AppModId, TraceLevel);
    void enable_all(TraceLevel);
    bool enable(AppModId mod) {
        return filters_[mod].enable();
    }
    void disable(AppModId mod) {
        filters_[mod].enable(false);
    }

    bool enable(TraceInfo *);
    bool enable(TraceInfo *, TraceBase *);
    bool enable(AppModId, TraceLevel, int);
    bool enable(AppModId, TraceLevel, TraceBase *);
};

#endif

