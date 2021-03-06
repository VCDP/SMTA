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
 *\file h264e_trace.h
 *\brief Definition of trace MCAROs for H264E module
 */

#ifndef _APP_TRACE_H_
#define _APP_TRACE_H_

#include "trace.h"

#define EX_INFO __FILE__, __LINE__

#define STRM_TRACE_ERROR    (Trace(STRM, SYS_ERROR, EX_INFO))
#define STRM_TRACE_WARNI    (Trace(STRM, SYS_WARNING, EX_INFO))
#define STRM_TRACE_DEBUG    (Trace(STRM, SYS_DEBUG, EX_INFO))
#define STRM_TRACE_INFO     (Trace(STRM, SYS_INFORMATION, EX_INFO))

#define F2F_TRACE_ERROR    (Trace(F2F, SYS_ERROR, EX_INFO))
#define F2F_TRACE_WARNI    (Trace(F2F, SYS_WARNING, EX_INFO))
#define F2F_TRACE_DEBUG    (Trace(F2F, SYS_DEBUG, EX_INFO))
#define F2F_TRACE_INFO     (Trace(F2F, SYS_INFORMATION, EX_INFO))

#define FRMW_TRACE_ERROR    (Trace(FRMW, SYS_ERROR, EX_INFO))
#define FRMW_TRACE_WARNI    (Trace(FRMW, SYS_WARNING, EX_INFO))
#define FRMW_TRACE_DEBUG    (Trace(FRMW, SYS_DEBUG, EX_INFO))
#define FRMW_TRACE_INFO     (Trace(FRMW, SYS_INFORMATION, EX_INFO))

#define H264D_TRACE_ERROR    (Trace(H264D, SYS_ERROR, EX_INFO))
#define H264D_TRACE_WARNI    (Trace(H264D, SYS_WARNING, EX_INFO))
#define H264D_TRACE_DEBUG    (Trace(H264D, SYS_DEBUG, EX_INFO))
#define H264D_TRACE_INFO     (Trace(H264D, SYS_INFORMATION, EX_INFO))

#define H264E_TRACE_ERROR    (Trace(H264E, SYS_ERROR, EX_INFO))
#define H264E_TRACE_WARNI    (Trace(H264E, SYS_WARNING, EX_INFO))
#define H264E_TRACE_DEBUG    (Trace(H264E, SYS_DEBUG, EX_INFO))
#define H264E_TRACE_INFO     (Trace(H264E, SYS_INFORMATION, EX_INFO))

#define MPEG2D_TRACE_ERROR    (Trace(MPEG2D, SYS_ERROR, EX_INFO))
#define MPEG2D_TRACE_WARNI    (Trace(MPEG2D, SYS_WARNING, EX_INFO))
#define MPEG2D_TRACE_DEBUG    (Trace(MPEG2D, SYS_DEBUG, EX_INFO))
#define MPEG2D_TRACE_INFO     (Trace(MPEG2D, SYS_INFORMATION, EX_INFO))

#define MPEG2E_TRACE_ERROR    (Trace(MPEG2E, SYS_ERROR, EX_INFO))
#define MPEG2E_TRACE_WARNI    (Trace(MPEG2E, SYS_WARNING, EX_INFO))
#define MPEG2E_TRACE_DEBUG    (Trace(MPEG2E, SYS_DEBUG, EX_INFO))
#define MPEG2E_TRACE_INFO     (Trace(MPEG2E, SYS_INFORMATION, EX_INFO))

#define VC1D_TRACE_ERROR    (Trace(VC1D, SYS_ERROR, EX_INFO))
#define VC1D_TRACE_WARNI    (Trace(VC1D, SYS_WARNING, EX_INFO))
#define VC1D_TRACE_DEBUG    (Trace(VC1D, SYS_DEBUG, EX_INFO))
#define VC1D_TRACE_INFO     (Trace(VC1D, SYS_INFORMATION, EX_INFO))

#define VC1E_TRACE_ERROR    (Trace(VC1E, SYS_ERROR, EX_INFO))
#define VC1E_TRACE_WARNI    (Trace(VC1E, SYS_WARNING, EX_INFO))
#define VC1E_TRACE_DEBUG    (Trace(VC1E, SYS_DEBUG, EX_INFO))
#define VC1E_TRACE_INFO     (Trace(VC1E, SYS_INFORMATION, EX_INFO))

#define VP8D_TRACE_ERROR    (Trace(VP8D, SYS_ERROR, EX_INFO))
#define VP8D_TRACE_WARNI    (Trace(VP8D, SYS_WARNING, EX_INFO))
#define VP8D_TRACE_DEBUG    (Trace(VP8D, SYS_DEBUG, EX_INFO))
#define VP8D_TRACE_INFO     (Trace(VP8D, SYS_INFORMATION, EX_INFO))

#define VP8E_TRACE_ERROR    (Trace(VP8E, SYS_ERROR, EX_INFO))
#define VP8E_TRACE_WARNI    (Trace(VP8E, SYS_WARNING, EX_INFO))
#define VP8E_TRACE_DEBUG    (Trace(VP8E, SYS_DEBUG, EX_INFO))
#define VP8E_TRACE_INFO     (Trace(VP8E, SYS_INFORMATION, EX_INFO))

#define VPP_TRACE_ERROR    (Trace(VPP, SYS_ERROR, EX_INFO))
#define VPP_TRACE_WARNI    (Trace(VPP, SYS_WARNING, EX_INFO))
#define VPP_TRACE_DEBUG    (Trace(VPP, SYS_DEBUG, EX_INFO))
#define VPP_TRACE_INFO     (Trace(VPP, SYS_INFORMATION, EX_INFO))

#define MSMT_TRACE_ERROR    (Trace(MSMT, SYS_ERROR, EX_INFO))
#define MSMT_TRACE_WARNI    (Trace(MSMT, SYS_WARNING, EX_INFO))
#define MSMT_TRACE_DEBUG    (Trace(MSMT, SYS_DEBUG, EX_INFO))
#define MSMT_TRACE_INFO     (Trace(MSMT, SYS_INFORMATION, EX_INFO))

#define TRC_TRACE_ERROR    (Trace(TRC, SYS_ERROR, EX_INFO))
#define TRC_TRACE_WARNI    (Trace(TRC, SYS_WARNING, EX_INFO))
#define TRC_TRACE_DEBUG    (Trace(TRC, SYS_DEBUG, EX_INFO))
#define TRC_TRACE_INFO     (Trace(TRC, SYS_INFORMATION, EX_INFO))

#define APP_TRACE_ERROR    (Trace(APP, SYS_ERROR, EX_INFO))
#define APP_TRACE_WARNI    (Trace(APP, SYS_WARNING, EX_INFO))
#define APP_TRACE_DEBUG    (Trace(APP, SYS_DEBUG, EX_INFO))
#define APP_TRACE_INFO     (Trace(APP, SYS_INFORMATION, EX_INFO))

#endif

