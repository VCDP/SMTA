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
 *\file trace_base.h
 *\brief Implementation for Trace base class
 */

#ifndef _TRACE_BASE_H_
#define _TRACE_BASE_H_

/**
 * This class will use for control each object trace, like decoder or encoeer;
 * use derived class from it, and pass this object to trace, trace module
 * can filter the trace by this object
 */

class TraceBase
{
private:
    bool enable_;

protected:
    TraceBase(): enable_(false) {}

public:
    const bool enable() {
        return enable_;
    } const
    void enable(const bool flag) {
        enable_ = flag;
    }
};

#endif
