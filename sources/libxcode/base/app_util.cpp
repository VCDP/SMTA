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
 *\file app_util.cpp
 *\brief Implementation of utility */

#include <string.h>
#include "app_util.h"
using namespace std;

const string Utility::mod_name_[APP_END_MOD] = {
    "",
    "STRM",  /* 0x01 */
    "F2F",  /* 0x02 */
    "FRMW",  /* 0x03 */
    "H264D",  /* 0x04 */
    "H264E",  /* 0x05 */
    "MPEG2D",  /* 0x06 */
    "MPEG2E",  /* 0x07 */
    "VC1D",  /* 0x08 */
    "VC1E",  /* 0x09 */
    "VP8D",  /* 0x0a */
    "V8E",  /* 0x0b */
    "VPP",  /* 0x0c */
    "MSMT",  /* 0x0d */
    "TRC"  /* 0x0e */
};

const string Utility::level_name_[SYS_DEBUG + 1] = {
    "",
    "ERROR",
    "WARNI",
    "INFO",
    "DEBUG"
};

const string &Utility::mod_name(AppModId id)
{
    if (id < APP_END_MOD) {
        return mod_name_[id];
    }

    return mod_name_[0];
}

AppModId Utility::mod_id(const char *modName)
{
    for (int idx = 0; idx < APP_END_MOD; idx++) {
        if (0 == strcmp(mod_name_[idx].c_str(), modName)) {
            return (AppModId)idx;
        }
    }

    return APP_END_MOD;
}

const string &Utility::level_name(TraceLevel level)
{
    if (level >= SYS_ERROR && level <= SYS_DEBUG) {
        return level_name_[level];
    }

    return level_name_[0];
}


