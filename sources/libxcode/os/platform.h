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

#ifndef __PLATFORM_H__
#define __PLATFORM_H__


/** OS **/
#if defined(__WINDOWS__) || defined(_WIN32) || defined(WIN32) || \
    defined(_WIN64) || defined(WIN64) || \
    defined(__WIN32__) || defined(__TOS_WIN__)
#define PLATFORM_OS_NAME "Windows"
#define PLATFORM_OS_WINDOWS
#elif defined(__linux__) || defined(linux) || defined(__linux) || \
    defined(__LINUX__) || defined(LINUX) || defined(_LINUX)
#define PLATFORM_OS_NAME "Linux"
#define PLATFORM_OS_LINUX
#else
#define PLATFORM_OS_NAME "Unknown"
#error Unknown OS
#endif // OS


/** Bits **/
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
    defined(__amd64) || defined(__LP64__) || defined(_LP64) || \
    defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || \
    defined(__ia64__) || defined(_IA64) || defined(__IA64__) || \
    defined(__ia64) || defined(_M_IA64)
#define PLATFORM_BITS_NAME "64"
#define PLATFORM_BITS_64
#elif defined(_WIN32) || defined(WIN32) || defined(__32BIT__) || \
    defined(__ILP32__) || defined(_ILP32) || defined(i386) || \
    defined(__i386__) || defined(__i486__) || defined(__i586__) || \
    defined(__i686__) || defined(__i386) || defined(_M_IX86) || \
    defined(__X86__) || defined(_X86_) || defined(__I86__)
#define PLATFORM_BITS_NAME "32"
#define PLATFORM_BITS_32
#else
#define PLATFORM_BITS_NAME "Unknown"
#error Unknown Bits
#endif // Bits


/** Compiler **/
#if defined(_MSC_VER)
#define PLATFORM_CC_NAME "VC"
#define PLATFORM_CC_VC
#elif defined(__GNUG__) || defined(__GNUC__)
#define PLATFORM_CC_NAME "GCC"
#define PLATFORM_CC_GCC
#else
#define PLATFORM_CC_NAME "Unknown"
#error Unknown Compiler
#endif // Compiler


/*Module API definitions*/
#if defined(PLATFORM_OS_WINDOWS)
#define DLL_API extern "C" __declspec(dllexport)
#else
#define DLL_API extern "C"
#endif // PLATFORM_OS_WINDOWS


/*String*/
#define PLATFORM_STR "OS: " PLATFORM_OS_NAME \
                     ", Bits: " PLATFORM_BITS_NAME \
                     ", Compiler: " PLATFORM_CC_NAME

namespace XC {

// add this for compflict define in Xlib.h
#ifdef Status
#undef Status
#endif
    enum Status {
        XC_SUCCESS = 0,
        XC_OPERATION_FAILED,
        XC_TIMEOUT,
        XC_NOT_INITIALIZED,
        XC_NULL_ARGUMENT
    };
} // ns

/*Module API definitions*/
#if defined(PLATFORM_OS_WINDOWS)
#include <wtypes.h>

#define PRIaSTR "%S"    // Unicode on Windows => ASCII strings need %S
#define PRIwSTR "%s"

#else
#endif // include

#endif // __PLATFORM_H__
