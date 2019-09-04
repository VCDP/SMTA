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

#include "atomic.h"

#if defined(PLATFORM_OS_WINDOWS)


#define _interlockedbittestandset      fake_set
#define _interlockedbittestandreset    fake_reset
#define _interlockedbittestandset64    fake_set64
#define _interlockedbittestandreset64  fake_reset64
#include <intrin.h>
#undef _interlockedbittestandset
#undef _interlockedbittestandreset
#undef _interlockedbittestandset64
#undef _interlockedbittestandreset64
#pragma intrinsic (_InterlockedIncrement16)
#pragma intrinsic (_InterlockedDecrement16)

mfxU16 msdk_atomic_inc16(volatile mfxU16 *pVariable)
{
    return _InterlockedIncrement16((volatile short*)pVariable);
}

/* Thread-safe 16-bit variable decrementing */
mfxU16 msdk_atomic_dec16(volatile mfxU16 *pVariable)
{
    return _InterlockedDecrement16((volatile short*)pVariable);
}

#else // #if defined(PLATFORM_OS_WINDOWS)
static mfxU16 msdk_atomic_add16(volatile mfxU16 *mem, mfxU16 val)
{
    asm volatile ("lock; xaddw %0,%1"
                  : "=r" (val), "=m" (*mem)
                  : "0" (val), "m" (*mem)
                  : "memory", "cc");
    return val;
}

mfxU16 msdk_atomic_inc16(volatile mfxU16 *pVariable)
{
    return msdk_atomic_add16(pVariable, 1) + 1;
}

/* Thread-safe 16-bit variable decrementing */
mfxU16 msdk_atomic_dec16(volatile mfxU16 *pVariable)
{
    return msdk_atomic_add16(pVariable, (mfxU16)-1) + 1;
}

#endif //Linux

