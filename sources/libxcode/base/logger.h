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

#ifndef _MLOG_H_
#define _MLOG_H_

#include <stdio.h>
#include <stdlib.h>

#include "debug_info.h"

#define NODE "\033[m"
#define RED "\033[0;32;31m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[0;32;32m"
#define BLUE "\033[0;32;34m"
#define BLACK "\033[0;32;30"

#define DECLARE_MLOGINSTANCE() \
    static LogInstance* logInstance

#define DEFINE_MLOGINSTANCE(moduleName) \
    static LogInstance* logInstance = LogFactory::GetLogFactory()->CreateLogInstance(moduleName)

#define DEFINE_MLOGINSTANCE_CLASS(namespace, moduleName) \
    LogInstance*  namespace::logInstance = LogFactory::GetLogFactory()->CreateLogInstance(moduleName)

#if defined (PLATFORM_OS_LINUX)
#define MLOG_FATAL(format, args...) \
    if (logInstance->isFatalEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_FATAL, RED, format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_FATAL, RED, format, ##args); \
    }

#define MLOG_ERROR(format, args...) \
    if (logInstance->isErrorEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_ERROR, RED,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_ERROR, RED, format, ##args); \
    }

#define MLOG_WARNING(format, args...) \
    if (logInstance->isWarningEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_WARNING, YELLOW,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_WARNING, YELLOW, format, ##args); \
    }

#define MLOG_INFO(format, args...) \
    if (logInstance->isInfoEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_INFO, GREEN,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_INFO, GREEN, format, ##args); \
    }

#define MLOG_TRACE(format, args...) \
    if (logInstance->isTraceEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_TRACE, "",  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_TRACE, "", format, ##args); \
    }

#define MLOG_DEBUG(format, args...) \
    if (logInstance->isDebugEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_DEBUG,BLUE ,  format, ##args); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_DEBUG, BLUE, format, ##args); \
    }

#define MLOG(format, args...) \
    logInstance->LogPrintf(format, ##args);
#elif defined (PLATFORM_OS_WINDOWS)
#define MLOG_FATAL(format, ...) \
    if (logInstance->isFatalEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_FATAL, RED, format, ##__VA_ARGS__); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_FATAL, RED, format, ##__VA_ARGS__); \
    }
#define MLOG_ERROR(format, ...) \
    if (logInstance->isErrorEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_ERROR, RED,  format, ##__VA_ARGS__); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_ERROR, RED, format, ##__VA_ARGS__); \
    }
#define MLOG_WARNING(format, ...) \
    if (logInstance->isWarningEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_WARNING, YELLOW,  format, ##__VA_ARGS__); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_WARNING, YELLOW, format, ##__VA_ARGS__); \
    }

#define MLOG_INFO(format, ...) \
    if (logInstance->isInfoEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_INFO, GREEN,  format, ##__VA_ARGS__); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_INFO, GREEN, format, ##__VA_ARGS__); \
    }

#define MLOG_TRACE(format, ...) \
    if (logInstance->isTraceEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_TRACE, "",  format, ##__VA_ARGS__); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_TRACE, "", format, ##__VA_ARGS__); \
    }

#define MLOG_DEBUG(format, ...) \
    if (logInstance->isDebugEnabled()) { \
        if (logInstance->hasFileName()) \
            logInstance->LogFile(__FILE__, __FUNCTION__, __LINE__, LogInstance::M_DEBUG,BLUE ,  format, ##__VA_ARGS__); \
        else \
            logInstance->LogFunction(__FUNCTION__, __LINE__, LogInstance::M_DEBUG, BLUE, format, ##__VA_ARGS__); \
    }

#define MLOG(format, ...) \
    logInstance->LogPrintf(format, ##__VA_ARGS__);

#endif
#endif //_MLOG_H_
