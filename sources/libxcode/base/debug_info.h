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

#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H

#include <stdlib.h>
#include <map>
#include <iostream>
#include <string.h>

#include "os/platform.h"

class LogInstance
{
public:
//#define NONE "\033[m"
//#define MAXLOGFILESIZE 2*1024*1024

    typedef enum{
        M_FATAL=2,
        M_ERROR=4,
        M_WARNING=8,
        M_INFO=16,
        M_TRACE=24,
        M_DEBUG=32
    }LOGLEVEL;

    static int mLogFileCount;
#if defined (PLATFORM_OS_WINDOWS)
    LogInstance(const char* moduleName,
                int level,
                bool hasTime,
                bool hasFileName,
                bool hasModuleName,
                bool hasLogFile,
                const char* logFilePath,
                HANDLE mutex);
#elif defined (PLATFORM_OS_LINUX)
    LogInstance(const char* moduleName,
                int level,
                bool hasTime,
                bool hasFileName,
                bool hasModuleName,
                bool hasLogFile,
                const char* logFilePath,
                pthread_mutex_t* mutex);
#endif
    LogInstance();
    ~LogInstance();

    //char* GetModuleName();
    bool isErrorEnabled() { return M_ERROR <= mLevel ? true:false; };
    bool isFatalEnabled() { return M_FATAL <= mLevel ? true:false; };
    bool isInfoEnabled() { return M_INFO <= mLevel ? true:false; };
    bool isWarningEnabled() { return M_WARNING <= mLevel ? true:false; };
    bool isDebugEnabled() { return M_DEBUG <= mLevel ? true:false; };
    bool isTraceEnabled() { return M_TRACE <= mLevel ? true:false; };
    bool hasFileName() { return mHasFileName;}

    //bool LogInfo(const char *strFile,const int &iLineNum,const int &iPriority,const char *strMsg,const char *strCatName);
    void Init(void);
    bool LogFunction(const char *strFun, const int &iLineNum, const int &iPriority, const char* color, const char* format, ...);
    bool LogFile(const char* strFile, const char *strFun, const int &iLineNum, const int &iPriority, const char* color, const char* format, ...);
    bool LogPrintf(const char* format, ...);
private:
    bool SaveLogFileForOther(FILE* fp);
    bool WriteLog(const char * szFileName, const char * strMsg);

    int mLevel;
    std::map<int, std::string> mLevelList;
    const char* mLogFilePath;
    const char* mModuleName;
    char mModuleStr[64];
    bool mHasTime;
    bool mHasFileName;
    bool mHasLogFile;
    bool mHasModuleName;
#if defined(PLATFORM_OS_WINDOWS)
    HANDLE mLocalMutex;
#elif defined (PLATFORM_OS_LINUX)
    pthread_mutex_t* mLocalMutex;
#endif
};


class LogFactory
{
private:
    LogFactory();
    LogFactory(const LogFactory &);
    LogFactory& operator = (const LogFactory &);
    bool Init();
    void TrimConfigWhitespace(std::string &str);
    void TrimConfigComments(std::string &str);
    void ReadConfigFile(char *fileName);


    std::map<std::string, int> mModulesList;
    std::string mLogFilePath;
    bool mHasTime;
    bool mHasFileName;
    bool mHasLogFile;
    bool mHasModuleName;

public:
    static void SetLogFactoryParams(char* fileName);
    static LogFactory *GetLogFactory();
    static LogFactory *pLogFactory;
#if defined(PLATFORM_OS_WINDOWS)
    static HANDLE mMutex;
#elif defined (PLATFORM_OS_LINUX)
    static pthread_mutex_t mMutex;
#endif

    LogInstance* CreateLogInstance(const char* moduleName);
};


#endif //DEBUG_INFO_H
