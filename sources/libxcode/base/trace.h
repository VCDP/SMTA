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
 *\file trace.h
 *\brief Definition for Trace
 */

#ifndef _TRACE_H_
#define _TRACE_H_

#include <queue>

#include "app_def.h"
#include "os/XC_Thread.h"
#include "trace_pool.h"

#define EX_INFO __FILE__, __LINE__

#define STRM_TRACE_ERROR    (Trace::Instance(STRM, SYS_ERROR, EX_INFO))
#define STRM_TRACE_WARNI    (Trace::Instance(STRM, SYS_WARNING, EX_INFO))
#define STRM_TRACE_DEBUG    (Trace::Instance(STRM, SYS_DEBUG, EX_INFO))
#define STRM_TRACE_INFO     (Trace::Instance(STRM, SYS_INFORMATION, EX_INFO))

#define F2F_TRACE_ERROR    (Trace::Instance(F2F, SYS_ERROR, EX_INFO))
#define F2F_TRACE_WARNI    (Trace::Instance(F2F, SYS_WARNING, EX_INFO))
#define F2F_TRACE_DEBUG    (Trace::Instance(F2F, SYS_DEBUG, EX_INFO))
#define F2F_TRACE_INFO     (Trace::Instance(F2F, SYS_INFORMATION, EX_INFO))

#define FRMW_TRACE_ERROR    (Trace::Instance(FRMW, SYS_ERROR, EX_INFO))
#define FRMW_TRACE_WARNI    (Trace::Instance(FRMW, SYS_WARNING, EX_INFO))
#define FRMW_TRACE_DEBUG    (Trace::Instance(FRMW, SYS_DEBUG, EX_INFO))
#define FRMW_TRACE_INFO     (Trace::Instance(FRMW, SYS_INFORMATION, EX_INFO))

#define H264D_TRACE_ERROR    (Trace::Instance(H264D, SYS_ERROR, EX_INFO))
#define H264D_TRACE_WARNI    (Trace::Instance(H264D, SYS_WARNING, EX_INFO))
#define H264D_TRACE_DEBUG    (Trace::Instance(H264D, SYS_DEBUG, EX_INFO))
#define H264D_TRACE_INFO     (Trace::Instance(H264D, SYS_INFORMATION, EX_INFO))

#define H264E_TRACE_ERROR    (Trace::Instance(H264E, SYS_ERROR, EX_INFO))
#define H264E_TRACE_WARNI    (Trace::Instance(H264E, SYS_WARNING, EX_INFO))
#define H264E_TRACE_DEBUG    (Trace::Instance(H264E, SYS_DEBUG, EX_INFO))
#define H264E_TRACE_INFO     (Trace::Instance(H264E, SYS_INFORMATION, EX_INFO))

#define MPEG2D_TRACE_ERROR    (Trace::Instance(MPEG2D, SYS_ERROR, EX_INFO))
#define MPEG2D_TRACE_WARNI    (Trace::Instance(MPEG2D, SYS_WARNING, EX_INFO))
#define MPEG2D_TRACE_DEBUG    (Trace::Instance(MPEG2D, SYS_DEBUG, EX_INFO))
#define MPEG2D_TRACE_INFO     (Trace::Instance(MPEG2D, SYS_INFORMATION, EX_INFO))

#define MPEG2E_TRACE_ERROR    (Trace::Instance(MPEG2E, SYS_ERROR, EX_INFO))
#define MPEG2E_TRACE_WARNI    (Trace::Instance(MPEG2E, SYS_WARNING, EX_INFO))
#define MPEG2E_TRACE_DEBUG    (Trace::Instance(MPEG2E, SYS_DEBUG, EX_INFO))
#define MPEG2E_TRACE_INFO     (Trace::Instance(MPEG2E, SYS_INFORMATION, EX_INFO))

#define VC1D_TRACE_ERROR    (Trace::Instance(VC1D, SYS_ERROR, EX_INFO))
#define VC1D_TRACE_WARNI    (Trace::Instance(VC1D, SYS_WARNING, EX_INFO))
#define VC1D_TRACE_DEBUG    (Trace::Instance(VC1D, SYS_DEBUG, EX_INFO))
#define VC1D_TRACE_INFO     (Trace::Instance(VC1D, SYS_INFORMATION, EX_INFO))

#define VC1E_TRACE_ERROR    (Trace::Instance(VC1E, SYS_ERROR, EX_INFO))
#define VC1E_TRACE_WARNI    (Trace::Instance(VC1E, SYS_WARNING, EX_INFO))
#define VC1E_TRACE_DEBUG    (Trace::Instance(VC1E, SYS_DEBUG, EX_INFO))
#define VC1E_TRACE_INFO     (Trace::Instance(VC1E, SYS_INFORMATION, EX_INFO))

#define VP8D_TRACE_ERROR    (Trace::Instance(VP8D, SYS_ERROR, EX_INFO))
#define VP8D_TRACE_WARNI    (Trace::Instance(VP8D, SYS_WARNING, EX_INFO))
#define VP8D_TRACE_DEBUG    (Trace::Instance(VP8D, SYS_DEBUG, EX_INFO))
#define VP8D_TRACE_INFO     (Trace::Instance(VP8D, SYS_INFORMATION, EX_INFO))

#define VP8E_TRACE_ERROR    (Trace::Instance(VP8E, SYS_ERROR, EX_INFO))
#define VP8E_TRACE_WARNI    (Trace::Instance(VP8E, SYS_WARNING, EX_INFO))
#define VP8E_TRACE_DEBUG    (Trace::Instance(VP8E, SYS_DEBUG, EX_INFO))
#define VP8E_TRACE_INFO     (Trace::Instance(VP8E, SYS_INFORMATION, EX_INFO))

#define VPP_TRACE_ERROR    (Trace::Instance(VPP, SYS_ERROR, EX_INFO))
#define VPP_TRACE_WARNI    (Trace::Instance(VPP, SYS_WARNING, EX_INFO))
#define VPP_TRACE_DEBUG    (Trace::Instance(VPP, SYS_DEBUG, EX_INFO))
#define VPP_TRACE_INFO     (Trace::Instance(VPP, SYS_INFORMATION, EX_INFO))

#define MSMT_TRACE_ERROR    (Trace::Instance(MSMT, SYS_ERROR, EX_INFO))
#define MSMT_TRACE_WARNI    (Trace::Instance(MSMT, SYS_WARNING, EX_INFO))
#define MSMT_TRACE_DEBUG    (Trace::Instance(MSMT, SYS_DEBUG, EX_INFO))
#define MSMT_TRACE_INFO     (Trace::Instance(MSMT, SYS_INFORMATION, EX_INFO))

#define TRC_TRACE_ERROR    (Trace::Instance(TRC, SYS_ERROR, EX_INFO))
#define TRC_TRACE_WARNI    (Trace::Instance(TRC, SYS_WARNING, EX_INFO))
#define TRC_TRACE_DEBUG    (Trace::Instance(TRC, SYS_DEBUG, EX_INFO))
#define TRC_TRACE_INFO     (Trace::Instance(TRC, SYS_INFORMATION, EX_INFO))

#define APP_TRACE_ERROR    (Trace::Instance(APP, SYS_ERROR, EX_INFO))
#define APP_TRACE_WARNI    (Trace::Instance(APP, SYS_WARNING, EX_INFO))
#define APP_TRACE_DEBUG    (Trace::Instance(APP, SYS_DEBUG, EX_INFO))
#define APP_TRACE_INFO     (Trace::Instance(APP, SYS_INFORMATION, EX_INFO))

class TraceFilter;
class TracePool;
class TraceInfo;
class TraceBase;

class Trace : public XC::Thread
{
private:
    AppModId mod_;
    TraceLevel level_;
    const char *file_;
    int line_;
    bool running_;

    static const int MAX_DESC_LEN = 2048;
    static TracePool pool_;
    static Trace *instance;

public:
    static TraceFilter filter;
    static std::queue<TraceInfo *> queue_;

public:
    Trace(void);
    ~Trace(void);
    void SetFile(const char *file) { file_ = file; }
    void SetModLevel(AppModId mod, TraceLevel level);

    void operator()(const char *fmt, ...);
    void operator()(TraceBase *obj, const char *fmt, ...);

    static void start();
    static void stop();
    static Trace* Instance(void);
    static Trace& Instance(AppModId, TraceLevel, const char *file = NULL, int line = 0);

    TracePool &pool() {
        return pool_;
    }

    uint32_t Run(void);
};

#endif

