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
#ifndef ELEMENT_COMMON_H
#define ELEMENT_COMMON_H
#include "mfxvideo.h"
#include "base/partially_linear_bitrate.h"
#include "os/platform.h"
#if defined(PLATFORM_OS_WINDOWS)
#include <math.h>
#endif
#include "os/atomic.h"

#define MSDK_ALIGN32(X)     (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)     (((value + 15) >> 4) << 4)
#define FRAME_RATE_CTRL_TIMER (1000 * 1000)
#define FRAME_RATE_CTRL_PADDING (1)
#define MACROBLOCK_SIZE (16 * 16)

unsigned int GetSysTimeInUs();

mfxU16 CalDefBitrate(mfxU32 nCodecId, mfxU32 nTargetUsage, mfxU32 nWidth, mfxU32 nHeight, mfxF64 dFrameRate);
mfxU16 GetMinNumRefFrameForPyramid(mfxU16 GopRefDist);
mfxU16 AdjustBrefType(mfxU16 init_bref, mfxVideoParam const & par);

#if defined(PLATFORM_OS_WINDOWS)

inline double round( double d )
{
    return floor( d + 0.5 );
}

#endif

#endif //ELEMENT_COMMON_H
