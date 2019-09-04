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
 *\file trace_pool.h
 *\brief Definition for Trace pool
 */


#ifndef _TRACE_MEM_H_
#define _TRACE_MEM_H_

#include <vector>

class TraceInfo;

class TracePool
{
public:
    void add(TraceInfo *);
    void clear();
    TraceInfo *get(unsigned int index) const;
    void dump();

private:
    static const unsigned int MAX_SIZE = 1000;
    typedef std::vector<TraceInfo *> TracePoolVec;
    TracePoolVec vect_;

};

#endif

