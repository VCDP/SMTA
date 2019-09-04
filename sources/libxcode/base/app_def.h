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
 *\file app_def.h
 *\brief system definition header */

#ifndef _APP_DEF_H_
#define _APP_DEF_H_

typedef enum {
    APP_START_MOD = 0,
    STRM = 0x01,  /* streaming transcoder */
    F2F = 0x02,  /* File2File transcoder */
    FRMW = 0x03,  /* transcoder framework */
    H264D = 0x04,  /* H264 decoder */
    H264E = 0x05,  /* H264 encoder */
    MPEG2D = 0x06,  /* MPEG2 decoder */
    MPEG2E = 0x07,  /* MPEG2 eccoder */
    VC1D = 0x08,  /* VC1 decoder */
    VC1E = 0x09,  /* VC1 eccoder */
    VP8D = 0x0a,  /* VP8 decoder */
    VP8E = 0x0b,  /* VP8 encoder */
    VPP = 0x0c,  /* Video Post Processing */
    MSMT = 0x0d,  /* Measurement */
    TRC = 0x0e,  /* Trace */
    APP = 0x0f,  /* Audio Post Processing */
    APP_MAX_MOD_NUM,
    APP_END_MOD = APP_MAX_MOD_NUM
} AppModId;

typedef enum {
    SYS_ERROR = 1,
    SYS_WARNING,
    SYS_INFORMATION,
    SYS_DEBUG
} TraceLevel;


#ifdef DEBUG
#include <stdio.h>

#define TRC_DEBUG(fmt, ...) \
    do { fprintf(stderr, fmt); } while (0)
#else
#define TRC_DEBUG(fmt, ...)
#endif


#endif

