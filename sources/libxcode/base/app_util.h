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
 *\file app_util.h
 *\brief Definition for utility */

#ifndef _APP_UTIL_H_
#define _APP_UTIL_H_

#include <string>
#include "app_def.h"

class Utility
{
private:
    Utility() {};
    ~Utility() {};

    static const std::string mod_name_[APP_END_MOD];
    static const std::string level_name_[SYS_DEBUG + 1];

public:
    static const std::string &mod_name(AppModId);
    static AppModId mod_id(const char *);
    static const std::string &level_name(TraceLevel);
};

#endif
