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

#ifndef __SAMPLE_DEFS_H__
#define __SAMPLE_DEFS_H__

#include <memory.h>

#include "mfxdefs.h"
#include "sample_utils.h"

#define MSDK_DEC_WAIT_INTERVAL 60000
#define MSDK_ENC_WAIT_INTERVAL 10000
#define MSDK_VPP_WAIT_INTERVAL 60000
#define MSDK_SURFACE_WAIT_INTERVAL 20000

// an estimate for the longest pipeline we have in samples
#define MSDK_WAIT_INTERVAL \
    MSDK_DEC_WAIT_INTERVAL+3*MSDK_VPP_WAIT_INTERVAL+MSDK_ENC_WAIT_INTERVAL

// This FOURCC doesn't actually exist, we use it as a dummy for no encoding;
#define MFX_CODEC_NULL MFX_MAKEFOURCC('N','U','L','L')

#define MSDK_INVALID_SURF_IDX 0xFFFF

#define MSDK_MAX_FILENAME_LEN 1024

#define MSDK_PRINT_RET_MSG(ERR) { \
    _tprintf(_T("\nReturn on error: error code %d,\t%s\t%d\n\n"), ERR, _T(__FILE__), __LINE__); \
    debug_printf(_T("\nReturn on error: error code %d,\t%s\t%d\n\n"), ERR, _T(__FILE__), __LINE__); \
}

#define MSDK_CHECK_ERROR(P, X, ERR)              {if ((X) == (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_NOT_EQUAL(P, X, ERR)          {if ((X) != (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_RESULT(P, X, ERR)             {if ((X) > (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_PARSE_RESULT(P, X, ERR)       {if ((X) > (P)) {return ERR;}}
#define MSDK_CHECK_RESULT_SAFE(P, X, ERR, ADD)   {if ((X) > (P)) {ADD; MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_IGNORE_MFX_STS(P, X)                {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_CHECK_POINTER(P, ERR)               {if (!(P)) {return ERR;}}
#define MSDK_CHECK_POINTER_NO_RET(P)             {if (!(P)) {return;}}
#define MSDK_CHECK_POINTER_SAFE(P, ERR, ADD)     {if (!(P)) {ADD; return ERR;}}
#define MSDK_BREAK_ON_ERROR(P)                   {if (MFX_ERR_NONE != (P)) break;}
#define MSDK_SAFE_DELETE_ARRAY(P)                {if (P) {delete[] P; P = NULL;}}
#define MSDK_SAFE_RELEASE(X)                     {if (X) { X->Release(); X = NULL; }}
#define MSDK_SAFE_FREE(X)                        {if (X) { free(X); X = NULL; }}

#ifndef MSDK_SAFE_DELETE
#define MSDK_SAFE_DELETE(P)                      {if (P) {delete P; P = NULL;}}
#endif // MSDK_SAFE_DELETE

#define MSDK_ZERO_MEMORY(VAR)                    {memset(&VAR, 0, sizeof(VAR));}
#define MSDK_MAX(A, B)                           (((A) > (B)) ? (A) : (B))
#define MSDK_MIN(A, B)                           (((A) < (B)) ? (A) : (B))
#define MSDK_ALIGN16(value)                      (((value + 15) >> 4) << 4) // round up to a multiple of 16
#define MSDK_ALIGN32(value)                      (((value + 31) >> 5) << 5) // round up to a multiple of 32
#define SHIFT_RIGHT(VALUE, SHIFT_AMOUNT)         (VALUE >> SHIFT_AMOUNT)
#define SHIFT_LEFT(VALUE, SHIFT_AMOUNT)          (VALUE << SHIFT_AMOUNT)
#define BITMASK_AND(A, B)                        (A & B)
#define BITMASK_OR(A, B)                         (A | B)
#if 0 //def _DEBUG
#define MSDK_TRACE(_MESSAGE)                     OutputDebugStringA(_MESSAGE)
#else
#define MSDK_TRACE(_MESSAGE)
#endif

#endif //__SAMPLE_DEFS_H__
