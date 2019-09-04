/****************************************************************************
 *
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
 *
 ****************************************************************************/

#ifndef __COMMON_UNIX_H__
#define __COMMON_UNIX_H__

#include <assert.h>

#define _STDCALL_
#define _CDECL_
#define HANDLE void *
#define _tprintf printf
#define _tcslen strlen
#define _tclen strlen
#define _sntprintf snprintf
#define _vsntprintf vsnprintf
#define _ftprintf fprintf
#define _tcscmp strcmp
#define _tcsncmp strncmp
#define _tcsstr strstr
#define _stscanf_s sscanf
#define sscanf_s sscanf
#define _tcscpy_s strcpy
#define _vsnprintf vsnprintf
#define _fgetts fgets
#define GetCurrentThreadId pthread_self
#define _ASSERT assert
#define HMODULE pid_t

#define INFINITE XC::XC_WAIT_INFINITE

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(*a))
#endif

#define PRIaSTR "%s"    // On Linux, we normally use ascii strings
#define PRIwSTR "%S"

#ifdef __LP64__

#ifndef PRIu64
#define PRIu64 "%lu"
#endif

#ifndef PRId64
#define PRId64 "%ld"
#endif

#ifndef PRIu32
#define PRIu32 "%lu"
#endif

#ifndef PRId32
#define PRId32 "%ld"
#endif

#else /* __LP64__ */

#ifndef PRIu64
#define PRIu64 "%llu"
#endif

#ifndef PRId64
#define PRId64 "%lld"
#endif

#ifndef PRIu32
#define PRIu32 "%llu"
#endif

#ifndef PRId32
#define PRId32 "%lld"
#endif

#endif /* __LP64__ */

#ifndef ZeroMemory
#define ZeroMemory(buf, sz) memset(buf, 0, sz)
#endif

#ifndef __FUNCTIONW__
//#define __FUNCTIONW__ __func__
#define __FUNCTIONW__ __PRETTY_FUNCTION__
#endif

#ifndef _T
#define _T(A) A
#endif

#define TCHAR char

#endif // __COMMON_UNIX_H__
