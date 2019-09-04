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

#ifndef __CMDPROCESSOR_H__
#define __CMDPROCESSOR_H__

#include "InputParams.h"
#include "common_defs.h"

class SysMemFrameAllocator;

namespace TranscodingSample
{
    struct InputParams;

    typedef enum {
        PIPELINE = 0,
        DECODE = 1,
        ENCODE_VPP = 2
    } paramMode;

    class CmdProcessor
    {

    public:
        CmdProcessor();
        virtual ~CmdProcessor();
        mfxStatus ParseCmdLine(int argc, TCHAR *argv[]);
        InputParams &GetInputParams() { return m_inputParams; };
        string GetParFileName() {return m_strParFileName;};
    protected:
        void PrintHelp(const TCHAR *strAppName, const TCHAR *strErrorMessage);
        mfxStatus ParseParFile(FILE* file);
        mfxStatus ParseParString(std::string& string);
        mfxStatus ParseParams(const std::vector<string>& argv, paramMode mode);
        mfxStatus ParseRateControlMethod(const string& arg, EncParams &encInputParams);
        mfxStatus ParseCodecProfile(const string& arg, EncParams &encInputParams);
        mfxStatus ParseCodecLevel(const string& arg, EncParams &encInputParams);
        mfxStatus ParseTargetUsage(const string& strInput, mfxU16& rTargetUsage);
        mfxStatus ParseLogFlags(const string& strInput);
        mfxStatus ParseTiles(const string& arg, EncParams &encInputParams);
        mfxStatus ParseCompositionParfile(const std::string& parFileName, CompositionParams& pParams);
        mfxStatus VerifyAndCorrectInputParams(DecParams &decInputParams, EncParams &encInputParams,
                sVppParams &vppInputParams, paramMode mode);
        void getPair(string line, string &key, string &value);
        mfxStatus VerifyCrossSessionsOptions();
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
        bool ParseRoiFile(std::map<int, mfxExtEncoderROI>& roiData, std::string& roiFile);
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022

    protected:
        mfxU32              m_nSessionParamId;
        InputParams        m_inputParams;
        string              m_strParFileName;
    };
}

#endif //__TRANSCODE_UTILS_H__
