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

#include "debug_info.h"

#include <sstream>
#include <stdio.h>
#include <fstream>
#include <errno.h>
#include <stdarg.h>
#include <string>
#include <sstream>

#include "os/XC_Time.h"

#if _MSC_VER
#define snprintf _snprintf_s
#endif

#define MAXLINSIZE 2048
#define MAXFILEPATHLENGTH 512

#define LOG_CONFIG_PATH "/tmp/msdk_log_config.ini"
#define NONE "\033[m"
#define MAXLOGFILESIZE 2*1024*1024

LogFactory* LogFactory::pLogFactory = NULL;

#if defined (PLATFORM_OS_WINDOWS)
HANDLE LogFactory::mMutex = 0;
#elif defined (PLATFORM_OS_LINUX)
pthread_mutex_t LogFactory::mMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

LogFactory* LogFactory::GetLogFactory()
{
    if (pLogFactory == NULL) {
#if defined (PLATFORM_OS_WINDOWS)
            WaitForSingleObject(mMutex, INFINITE);
#elif defined (PLATFORM_OS_LINUX)
            pthread_mutex_lock(&mMutex);
#endif
        if (pLogFactory == NULL) {
            pLogFactory = new LogFactory();
            pLogFactory->Init();
        }
#if defined (PLATFORM_OS_WINDOWS)
        ReleaseMutex(mMutex);
#elif defined (PLATFORM_OS_LINUX)
        pthread_mutex_unlock(&mMutex);
#endif
    }
    return pLogFactory;
}
LogFactory::LogFactory():
    mHasTime(false),
    mHasFileName(false),
    mHasLogFile(false),
    mHasModuleName(false)
{}

// read ini file for config option
void LogFactory::ReadConfigFile(char *fileName)
{
    std::string newSection;
    std::string currentSection;
    /* Open the config file. */
    std::ifstream configFile(fileName);

    if (!configFile.is_open()) {
        exit(1);
    }

    /* Read the config file. */
    while (!configFile.eof()) {
        std::string line;
        getline(configFile, line);
        /* Remove comments and leading and trailing whitespace. */
        TrimConfigComments(line);
        TrimConfigWhitespace(line);

        /* Skip blank lines. */
        if (line.length() == 0) {
            continue;
        }

        /* Match a section header like [stream]. */
        if (line.find('[') == 0 &&
                line.find(']') == line.length() - 1) {
            newSection = line.substr(1, line.length() - 2);

            /* The new section becomes the current section. */
            currentSection = newSection;
        } /* End of match [section]. */

        if (line.find('=') != std::string::npos) {
            std::string key;
            std::string value;
            /* Create a stream string iterator for the line. */
            std::istringstream strStream(line);
            /* Split the line into a key/value pair around "=" */
            getline(strStream, key,   '=');
            getline(strStream, value, '=');
            /* Trim any whitespace from the tokens. */
            TrimConfigWhitespace(key);
            TrimConfigWhitespace(value);

            /* Store the key/value data. */
            if (key.compare("logfile") == 0) {
                mLogFilePath = value;
                mHasLogFile = true;
            } else if (key.compare("hasTime") == 0) {
                mHasTime = atoi(value.c_str()) == 1? true: false;
            } else if (key.compare("hasFilename") == 0) {
                mHasFileName = atoi(value.c_str()) == 1 ? true : false;
            } else if (key.compare("hasModulename") == 0) {
                mHasModuleName = atoi(value.c_str()) == 1 ? true : false;
            }

            std::map<std::string, int>::iterator itrl = mModulesList.begin();
            for( ; itrl != mModulesList.end(); itrl++) {
                if (key.compare(itrl->first.c_str()) == 0) {
                    itrl->second = atoi(value.c_str());
                    break;
                }
            }
        } /* End of match key = value. */
    } /* configFile.eof(). */

#if 0
    for(std::map<std::string, int>::iterator itrl = mModulesList.begin(); itrl != mModulesList.end(); itrl++) {
        printf("========modulename = %s(%d)\n", itrl->first.c_str(), itrl->second);
    }
    printf("=======time = %d, filename= %d, logfile=%s\n", mHasTime, mHasFileName, mLogFilePath.c_str());
#endif
    configFile.close();
}


void LogFactory::TrimConfigWhitespace(std::string &str)
{
    std::string delimiters = " \t\r\n";
    str.erase(0, str.find_first_not_of(delimiters)     );
    str.erase(   str.find_last_not_of (delimiters) + 1 );
}

/**
 *  @brief Trim comments starting with # from a string. In-place.
 */
void LogFactory::TrimConfigComments(std::string &str)
{
    size_t pos = str.find('#');

    if (pos != std::string::npos) {
        str.erase(pos);
    }
}

#if 0 
// read json file for config option
bool LogFactory::ReadConfigFile(char* fileName)
{
    Json::Reader reader;
    Json::Value root;

    std::ifstream is;
    is.open(fileName, std::ios::binary);
    if (reader.parse(is, root)) {
        std::string code;
        if (root["modules"].isNull())
            return false;

        mModulesList["null"] = root["modules"]["null"].asInt();
        mModulesList["MSDKCodec"] = root["modules"]["MSDKCodec"].asInt();
        mModulesList["VP8DecPlugin"] = root["modules"]["VP8DecPlugin"].asInt();
        mModulesList["MsdkXcoder"] = root["modules"]["MsdkXcoder"].asInt();
        mHasTime = root["time"].asInt();
        mHasFileName = root["filename"].asInt();
        if (root["logfile"].isNull()) {
            mHasLogFile = false;
        }else {
            mHasLogFile = true;
            mLogFilePath = root["logfile"].asString();
        }
    } else {
        printf("========parse json failed %s\n", strerror(errno));
    }

    is.close();
    return true;
}
#endif

bool LogFactory::Init()
{
    std::fstream file;

    mHasTime = false;
    mHasFileName = false;
    mHasLogFile = false;
    mHasModuleName = false;
    mModulesList["null"] = LogInstance::M_ERROR;
    mModulesList["MSDKCodec"] = LogInstance::M_ERROR;
    mModulesList["VP8DecPlugin"] = LogInstance::M_ERROR;
    mModulesList["MsdkXcoder"] = LogInstance::M_ERROR;
    mModulesList["OpenCLFilterBase"] = LogInstance::M_ERROR;
    mModulesList["OpenCLFilterVA"] = LogInstance::M_ERROR;
    mModulesList["PicDecPlugin"] = LogInstance::M_ERROR;
    mModulesList["StringDecPlugin"] = LogInstance::M_ERROR;
    mModulesList["SWDecPlugin"] = LogInstance::M_ERROR;
    mModulesList["SWEncPlugin"] = LogInstance::M_ERROR;
    mModulesList["vaapiFrameAllocator"] = LogInstance::M_ERROR;
    mModulesList["VAplugin"] = LogInstance::M_ERROR;
    mModulesList["VAXcoder"] = LogInstance::M_ERROR;
    mModulesList["OpencvVideoAnalyzer"] = LogInstance::M_ERROR;
    mModulesList["VP8DecPlugin"] = LogInstance::M_ERROR;
    mModulesList["VP8EncPlugin"] = LogInstance::M_ERROR;
    mModulesList["YUVWriterPlugin"] = LogInstance::M_ERROR;
    mModulesList["YUVReaderPlugin"] = LogInstance::M_ERROR;
    mModulesList["AudioPostProcessing"] = LogInstance::M_ERROR;
    mModulesList["AudioDecoder"] = LogInstance::M_ERROR;
    mModulesList["AudioEncoder"] = LogInstance::M_ERROR;
    mModulesList["AudioPCMReader"] = LogInstance::M_ERROR;
    mModulesList["AudioPCMWriter"] = LogInstance::M_ERROR;
    mModulesList["AudioTranscoder"] = LogInstance::M_ERROR;
    mModulesList["VCSA"] = LogInstance::M_ERROR;
    mModulesList["SNVA"] = LogInstance::M_ERROR;
    mModulesList["FEI"] = LogInstance::M_ERROR;
    mModulesList["AUDIO_FILE"] = LogInstance::M_ERROR;
    mModulesList["AUDIO_MIXER"] = LogInstance::M_ERROR;
    mModulesList["VIOA"] = LogInstance::M_ERROR;
    mModulesList["ASTA"] = LogInstance::M_ERROR;
    mModulesList["MediaPad"] = LogInstance::M_ERROR;
    mModulesList["EncoderElement"] = LogInstance::M_ERROR;
    mModulesList["VPPElement"] = LogInstance::M_ERROR;
    mModulesList["DecoderElement"] = LogInstance::M_ERROR;
    mModulesList["ElementFactory"] = LogInstance::M_ERROR;
    mModulesList["VP8DecoderElement"] = LogInstance::M_ERROR;
    mModulesList["VP8EncoderElement"] = LogInstance::M_ERROR;
    mModulesList["VAElement"] = LogInstance::M_ERROR;
    mModulesList["StringDecoderElement"] = LogInstance::M_ERROR;
    mModulesList["PictureDecoderElement"] = LogInstance::M_ERROR;
    mModulesList["SWEncoderElement"] = LogInstance::M_ERROR;
    mModulesList["SWDecoderElement"] = LogInstance::M_ERROR;
    mModulesList["RenderElement"] = LogInstance::M_ERROR;
    mModulesList["YUVWriterElement"] = LogInstance::M_ERROR;
    mModulesList["YUVReaderElement"] = LogInstance::M_ERROR;
    mModulesList["HevcEncoderElement"] = LogInstance::M_ERROR;
    mModulesList["H264FEIEncoderElement"] = LogInstance::M_ERROR;
    mModulesList["H264FEIEncoderPreenc"] = LogInstance::M_ERROR;
    mModulesList["H264FEIEncoderEncode"] = LogInstance::M_ERROR;
    mModulesList["H264FEIEncoderPAK"] = LogInstance::M_ERROR;
    mModulesList["H264FEIEncoderENC"] = LogInstance::M_ERROR;
    mModulesList["H265FEIEncoderElement"] = LogInstance::M_ERROR;
    mModulesList["H265FEIEncoderPreenc"] = LogInstance::M_ERROR;
    mModulesList["H265FEIEncoderEncode"] = LogInstance::M_ERROR;

    file.open(LOG_CONFIG_PATH, std::ios::in);
    if (file) {
        ReadConfigFile((char*)LOG_CONFIG_PATH);
    }

    return true;
}

LogInstance* LogFactory::CreateLogInstance(const char* moduleName)
{
    LogInstance* pLogInstance = new LogInstance(moduleName,
                                                mModulesList[moduleName],
                                                mHasTime,
                                                mHasFileName,
                                                mHasModuleName,
                                                mHasLogFile,
                                                mLogFilePath.c_str(),
                                                &mMutex);


    return pLogInstance;
}


int LogInstance::mLogFileCount = 0;

LogInstance::LogInstance()
    :mLevel(0),
     mLogFilePath(NULL),
     mModuleName(NULL),
     mHasTime(false),
     mHasFileName(false),
     mHasLogFile(false),
     mHasModuleName(false),
     mLocalMutex(NULL)
{}
LogInstance::LogInstance(const char* moduleName,
                        int level,
                        bool hasTime,
                        bool hasFileName,
                        bool hasModuleName,
                        bool hasLogFile,
                        const char* logFilePath,
#if defined (PLATFORM_OS_WINDOWS)
                        HANDLE mutex)
#elif defined (PLATFORM_OS_LINUX)
                        pthread_mutex_t* mutex)
#endif
    :mLevel(level),
     mLogFilePath(logFilePath),
     mModuleName(moduleName),
     mHasTime(hasTime),
     mHasFileName(hasFileName),
     mHasLogFile(hasLogFile),
     mHasModuleName(hasModuleName),
     mLocalMutex(mutex)
{
    Init();

    if (!strcmp(mModuleName, "null"))
        mHasModuleName = false;

    if (mHasModuleName)
        snprintf(mModuleStr, sizeof(mModuleStr), "%s:", mModuleName);

}

#if 0
void LogInstance::SetHasTimeFlag()
{
    mHasTime = true;
}

void LogInstance::SetModuleLevel(int level)
{
    mLevel = level;
}
char* LogInstance::GetModuleName()
{
    return mModuleName;
}
void LogInstance::SetLogFilePath(std::string logFilePath)
{
    if (!logFilePath.empty()) {
        mLogFilePath = logFilePath.c_str();
        mHasLogFile = true;
    } else {

        mHasLogFile = false;
    }
}

void LogInstance::SetHasFileNameFlag()
{
    mHasFileName = true;
}

void LogInstance::SetHasModuleNameFlag()
{
    mHasModuleName = true;

    sprintf(mModuleStr, "%s:", mModuleName);
}
#endif

bool LogInstance::SaveLogFileForOther(FILE* fp)
{
    int fileLen = 0;
    int ret = 0;

    fseek(fp, 0, SEEK_END);
    fileLen = ftell(fp);
    if (fileLen > MAXLOGFILESIZE) {
        std::string path = mLogFilePath;
        size_t suffixPos = path.find_last_of('.');
        size_t separatorPos = path.find_last_of('/');
        std::stringstream strCnt;
        //fclose(fp);
        strCnt<< mLogFileCount;
        if (suffixPos && suffixPos > separatorPos) {
            path.insert(suffixPos, strCnt.str().c_str());
        } else {
            path.insert(path.length(), strCnt.str().c_str());
        }
        mLogFileCount++;
        ret = rename(mLogFilePath,  path.c_str());
        return (ret == 0);
    }
    return false;
}
bool LogInstance::WriteLog(const char * szFileName, const char * strMsg)
{

    FILE * fp= NULL;

#if defined (PLATFORM_OS_WINDOWS)
    WaitForSingleObject(mLocalMutex, INFINITE);
#elif defined (PLATFORM_OS_LINUX)
    pthread_mutex_lock(mLocalMutex);
#endif
    fp = fopen(szFileName,"at+");

    if (NULL==fp) {
#if defined (PLATFORM_OS_WINDOWS)
        ReleaseMutex(mLocalMutex);
#elif defined (PLATFORM_OS_LINUX)
        pthread_mutex_unlock(mLocalMutex);
#endif
        return false;
    }

    if (SaveLogFileForOther(fp)) {
        fclose(fp);
        fp= fopen(szFileName,"at+");

        if (NULL==fp) {
#if defined (PLATFORM_OS_WINDOWS)
            ReleaseMutex(mLocalMutex);
#elif defined (PLATFORM_OS_LINUX)
            pthread_mutex_unlock(mLocalMutex);
#endif
            return false;
        }
    }
    fprintf(fp,"%s\r\n",strMsg);
    fflush(fp);
    fclose(fp);
#if defined (PLATFORM_OS_WINDOWS)
    ReleaseMutex(mLocalMutex);
#elif defined (PLATFORM_OS_LINUX)
    pthread_mutex_unlock(mLocalMutex);
#endif
    return true;
}

void LogInstance::Init(void)
{
    mLevelList[M_FATAL] = "FATAL";
    mLevelList[M_ERROR] = "M_ERROR";
    mLevelList[M_WARNING] = "WARNING";
    mLevelList[M_INFO] = "INFO";
    mLevelList[M_DEBUG] = "DEBUG";
    mLevelList[M_TRACE] = "TRACE";

    memset(mModuleStr, 0, sizeof(mModuleStr));
}
bool LogInstance::LogFile(
                            const char *strFile,
                            const char* strFun,
                            const int &iLineNum,
                            const int &iPriority,
                            const char* color,
                            const char* format, ...)
{
    char logstr[MAXLINSIZE] = {0};
    std::stringstream logMsg;
    va_list argp;
    time_t now;
    struct tm* tnow;
    std::stringstream tstr;

    va_start(argp,format);
    if (NULL==format||0==format[0])
        return false;
    vsnprintf(logstr,MAXLINSIZE-1,format,argp);
    va_end(argp);
    if (mHasTime) {
        time(&now);
        tnow = localtime(&now);
        if (tnow) {
            tstr << (1900+tnow->tm_year) << "/" << (1+tnow->tm_mon) << "/" << tnow->tm_mday << "_" << tnow->tm_hour << ":" << tnow->tm_min << ":" << tnow->tm_sec;
        }
    }
    if (mHasLogFile) {
        if (mHasTime) {
            logMsg << color
                   << "["
                   << tstr.str().c_str()
                   << "]:"
                   << strFile
                   << ":"
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        } else {
            logMsg << color
                   << strFile
                   << ":"
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        }
        WriteLog(mLogFilePath,logMsg.str().c_str());
    } else {
        if (mHasTime)
            printf("%s[%s]:%s:%s%s(%d):%s:%s%s\n",color, tstr.str().c_str(), strFile, mModuleStr, strFun, iLineNum,mLevelList[iPriority].c_str(),logstr, NONE);
        else
            printf("%s%s:%s%s(%d):%s:%s%s\n", color, strFile, mModuleStr, strFun, iLineNum,mLevelList[iPriority].c_str(),logstr, NONE);
    }
    return true;
}
bool LogInstance::LogFunction(const char *strFun,
                            const int &iLineNum,
                            const int &iPriority,
                            const char* color,
                            const char* format, ...)
{
    char logstr[MAXLINSIZE] = {0};
    std::stringstream logMsg;
    va_list argp;
    time_t now;
    struct tm* tnow;
    std::stringstream tstr;

    va_start(argp,format);
    if (NULL==format||0==format[0])
        return false;

    vsnprintf(logstr,MAXLINSIZE-1,format,argp);
    va_end(argp);
    if (mHasTime) {
        time(&now);
        tnow = localtime(&now);
        if (tnow) {
           tstr << (1900+tnow->tm_year) << "/" << (1+tnow->tm_mon) << "/" << tnow->tm_mday << " " << tnow->tm_hour << ":" << tnow->tm_min << ":" << tnow->tm_sec;
        }
    }
    if (mHasLogFile) {
        if (mHasTime) {
            logMsg << color
                   << "["
                   << tstr.str().c_str()
                   << "]:"
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        } else {
            logMsg << color
                   << mModuleStr
                   << strFun
                   << "("
                   << iLineNum
                   << "):"
                   << mLevelList[iPriority].c_str()
                   << ":"
                   << logstr
                   << NONE;
        }
        WriteLog(mLogFilePath,logMsg.str().c_str());
    } else {
        if (mHasTime) {
            printf("%s [%s]:%s%s(%d):%s: %s%s\n", color, tstr.str().c_str(), mModuleStr, strFun,iLineNum, mLevelList[iPriority].c_str(),logstr, NONE);
        }else {
            printf("%s %s%s(%d):%s: %s%s\n", color, mModuleStr, strFun,iLineNum, mLevelList[iPriority].c_str(),logstr, NONE);
        }
    }
    return true;
}

bool LogInstance::LogPrintf(const char* format, ...)
{
    char logstr[MAXLINSIZE+1];
    va_list argp;
    va_start(argp,format);
    if (NULL==format||0==format[0])
        return false;
    vsnprintf(logstr,MAXLINSIZE,format,argp);
    va_end(argp);
    if (mHasLogFile) {
        WriteLog(mLogFilePath,logstr);
    } else {
        printf("%s\n", logstr);
    }
    return true;
}

