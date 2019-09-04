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
 *\file trace_filter.cpp
 *\brief Implementation for TraceFilter class
 */

#include "trace_filter.h"
#include "trace_base.h"
#include "trace_info.h"

void TraceFilter::enable(AppModId mod, TraceLevel level)
{
    filters_[mod].level(level);
    filters_[mod].enable(true);
}

void TraceFilter::enable_all(TraceLevel level)
{
    for (int idx = 0; idx < APP_END_MOD; idx++) {
        filters_[idx].level(level);
        filters_[idx].enable(true);
    }
}

bool TraceFilter::enable(TraceInfo *trc)
{
    if (trc->level() <= level_) {
        return true;
    }

    if (enable()) {
        return true;
    } else {
        return filters_[trc->mod()].enable(trc->level());
    }
}

bool TraceFilter::enable(TraceInfo *trc, TraceBase *obj)
{
    if (enable(trc)) {
        return true;
    } else {
        return obj->enable();
    }
}

bool TraceFilter::enable( AppModId mod, TraceLevel level, int flag)
{
    if (level <= level_) {
        return true;
    }

    if (enable()) {
        return true;
    } else {
        return filters_[mod].enable(level);
    }
}

bool TraceFilter::enable(AppModId mod, TraceLevel level, TraceBase *obj)
{
    if (enable(mod, level, 0)) {
        return true;
    } else {
        return obj->enable();
    }
}

