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
 *\file trace_pool.cpp
 *\brief Implementation for trace pool
 */

#include "trace_pool.h"
#include <sstream>
#include <iostream>
#include "app_util.h"
#include "trace_info.h"

using namespace std;

void TracePool::add(TraceInfo *trc)
{
    if (NULL == trc) {
        TRC_DEBUG("Input parameter is NULL\n");
        return;
    }

    if (vect_.size() < MAX_SIZE) {
        vect_.push_back(trc);
        return;
    }

    delete vect_[0];
    vect_[0] = NULL;
    vect_.erase(vect_.begin(), vect_.begin() + 1);
    vect_.push_back(trc);
}

TraceInfo *TracePool::get(unsigned int index) const
{
    if (index < vect_.size()) {
        return vect_[index];
    }

    return NULL;
}

void TracePool::clear()
{
    for (unsigned int idx = 0; idx < vect_.size(); idx++) {
        delete vect_[idx];
        vect_[idx] = NULL;
    }

    vect_.clear();
}

void TracePool::dump()
{
    ostringstream buff;
    buff << "Mod\tLevel\tTime\tDesc\n";
    char timestr[40] = {0};
    tm *timeinfo  = NULL;

    for (unsigned int i = 0; i < vect_.size(); i++) {
        timeinfo = localtime(vect_[i]->time());
        if (timeinfo) {
            strftime (timestr, 40, "%X", timeinfo);
        }
        buff << "<" << Utility::mod_name(vect_[i]->mod()) << "><"
             << Utility::level_name(vect_[i]->level()) << "><"
             << timestr << ">:" << vect_[i]->desc() << "\n";
    }

    cout << buff.str();
}

