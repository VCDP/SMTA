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

#ifndef _WIN32
#include <signal.h>
#endif

#include "Launcher.h"
#include "ffmpeg_utils.h"
#include "cmdProcessor.h"
#include "sample_utils.h"
#include <sstream>

using namespace TranscodingSample;

static Launcher* s_pLauncher = NULL;  // To be used in CtrlHandler

/*
 * registered SIGINT/SIGTERM signal handler
 */
#ifdef _WIN32
BOOL WINAPI CtrlHandlerRoutine(DWORD sig)
#else
static void CtrlHandlerRoutine(int sig)
#endif
{
    DEBUG_ERROR((_T("%s(CtrlType=%d)\n"), __FUNCTIONW__, sig ));

    if (NULL != s_pLauncher) {
        s_pLauncher->Stop();
    }

    // Give some time to cleanup (leave Transcode loop due to above flag, send RTSP TEARDOWN, etc.)
    XC::Sleep(2000);

#ifdef _WIN32
    return TRUE;
#endif
}

/*
 * main function
 */
#ifdef _WIN32
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    mfxStatus sts;
    // parse command line class
    CmdProcessor cmdParser;
    // new launcher
    s_pLauncher = new Launcher();
    if (NULL == s_pLauncher) {
        printf("Create Launcher error!\n");
        return -1;
    }

    // initialize FFMpeg
    EnsureFFmpegInit();

#ifdef _WIN32
    WSADATA dat;
    WSAStartup(MAKEWORD(2,2),&dat);
#endif //WIN32

    // parse command line
    sts = cmdParser.ParseCmdLine(argc, argv);
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, sts);

    // initialize launcher by input parameters
    sts = s_pLauncher->Init(cmdParser.GetInputParams());
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

    // register SIGINT/SIGTERM signal handler
#ifdef _WIN32
    SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
#else
    signal(SIGINT, CtrlHandlerRoutine);
    signal(SIGTERM, CtrlHandlerRoutine);
#endif

    // run launcher
    s_pLauncher->Run();

    // get decode/vpp/encode result
    sts = s_pLauncher->ProcessResult(cmdParser);

#ifdef _WIN32
    WSACleanup();
#endif

    // delete s_pLauncher
    if (s_pLauncher) {
        // Give some time to cleanup (leave Transcode loop due to above flag, send RTSP TEARDOWN, etc.)
        delete s_pLauncher;
        s_pLauncher = NULL;
    }

    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, 1);

    return 0;
}

