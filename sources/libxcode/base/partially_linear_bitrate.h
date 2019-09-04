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

#ifndef _PARTIALLY_LINEAR_BITRATE_H_
#define _PARTIALLY_LINEAR_BITRATE_H_

//piecewise linear function for bitrate approximation
class PartiallyLinearBitrate {
    double *m_pX;
    double *m_pY;
    unsigned int  m_nPoints;
    unsigned int  m_nAllocated;

public:
    PartiallyLinearBitrate();
    ~PartiallyLinearBitrate();

    void AddPair(double x, double y);
    double at(double);

private:
    PartiallyLinearBitrate(const PartiallyLinearBitrate&);
    void operator=(const PartiallyLinearBitrate&);
};

#endif

