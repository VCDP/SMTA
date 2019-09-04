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

#ifndef __DEBUG_STREAM_H__
#define __DEBUG_STREAM_H__

#include "sample_utils.h"

#ifdef RUNTIME_LOG_SELECT

#define DEBUG_STREAM_OUTPUT(A) XC_LOG( XC_LOG_STREAM_OUTPUT, A )
#define DEBUG_STREAM_INPUT(A) XC_LOG( XC_LOG_STREAM_INPUT, A )
#define DEBUG_SPLITTER(A) XC_LOG( XC_LOG_SPLITTER, A )
#define DEBUG_SPLITTER_BUFFER(A) XC_LOG( XC_LOG_SPLITTER_BUFFER, A )
#define DEBUG_SPLITTER_BUFFER_CYCLE(A) XC_LOG( XC_LOG_SPLITTER_BUFFER_CYCLE, A )
#define DEBUG_STREAM_OUTPUT_MUXING(A) XC_LOG( XC_LOG_STREAM_OUTPUT_MUXING, A )
#define DEBUG_FFMPEG(A) XC_LOG( XC_LOG_FFMPEG, A)

#else

#ifdef VERBOSE_LOG
#define DEBUG_STREAM_OUTPUT(A) debug_printf A
#define DEBUG_STREAM_INPUT(A) debug_printf A
#define DEBUG_SPLITTER(A) debug_printf A
#define DEBUG_SPLITTER_BUFFER(A) //debug_printf A
#define DEBUG_SPLITTER_BUFFER_CYCLE(A) //debug_printf A
#define DEBUG_STREAM_OUTPUT_MUXING(A) //debug_printf A
#define DEBUG_FFMPEG(A) debug_printf A
#else
#define DEBUG_STREAM_OUTPUT(A) //debug_printf A
#define DEBUG_STREAM_INPUT(A) //debug_printf A
#define DEBUG_SPLITTER(A) //debug_printf A
#define DEBUG_SPLITTER_BUFFER(A) //debug_printf A
#define DEBUG_SPLITTER_BUFFER_CYCLE(A) //debug_printf A
#define DEBUG_STREAM_OUTPUT_MUXING(A) //debug_printf A
#define DEBUG_FFMPEG(A) //debug_printf A
#endif

#endif

#endif // __DEBUG_STREAM_H__
