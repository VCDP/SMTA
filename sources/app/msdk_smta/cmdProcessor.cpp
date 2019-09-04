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

#include <sys/types.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "cmdProcessor.h"
#include "sample_utils.h"

using namespace TranscodingSample;
using std::string;
using std::vector;

unsigned int s_TranscoderLogFlags;

#define STRFILEVER     "0.9.0.0"

#ifndef UINT16_MAX
#define UINT16_MAX (0xFFFF)
#endif

#define MaxNumActiveRefP      4
#define MaxNumActiveRefBL0    4
#define MaxNumActiveRefBL1    1
#define MaxNumActiveRefBL1_i  2
#define MaxNumActiveRefPAK    16 // PAK supports up to 16 L0 / L1 references

// parsing defines
#define VAL_CHECK(val) {if (val) return MFX_ERR_UNSUPPORTED;}
#define STS_CHECK(sts) {if (sts < MFX_ERR_NONE) return sts; }
// Convert string (char array) to upper case
#define TO_UPPER(str) for(char *p = str; *p != 0; ++p) { *p = toupper(*p); }

//set DecParams default values
DecParams::DecParams() {
    nDecodeId = 0;
    bIsLoopMode = false;
    strSrcURI.clear();
    nLoopNum = UINT16_MAX;
    bInputByFPS = false;
}

//set EncParams default values
EncParams::EncParams() {
    nEncodeId = 0;
    strDstURI.clear();
    outputType = BitstreamSink::E_NONE;
    nBitRate = 0;
    nSlices = 0;
    nMaxSliceSize = 0;
    nQP = UINT16_MAX;
    bLABRC = false;
    bMBBRC = false;
    nLADepth = 0;
    nRateControlMethod = MFX_RATECONTROL_CBR;
    nTargetUsage = MFX_TARGETUSAGE_BALANCED;
    nCodecProfile = MFX_PROFILE_UNKNOWN;
    nCodecLevel = MFX_LEVEL_UNKNOWN;
    nGopOptFlag = 0;
    nGopPicSize = 0;
    nGopRefDist = 0;
    nRefNum = 0;
    nIdrInterval = 0;
    bRefType = MFX_B_REF_UNKNOWN;
    pRefType = MFX_P_REF_SIMPLE;
    nGPB = MFX_CODINGOPTION_ON;
    nTileRows = 0;
    nTileColumns = 0;
    bDump = false;
    bUseHW = true;
    encHandle = NULL;
#if (MFX_VERSION >= 1025)
    nMFEFrames = 0;
    nMFEMode = MFX_MF_DEFAULT;
    nMFETimeout = 0;
#endif
    nRepartitionCheckMode = MFX_CODINGOPTION_UNKNOWN;
#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    bPreEnc = false;
    bEncode = false;
    bPAK = false;
    bENC = false;
    bENCPAK = false;
    bQualityRepack = false;
    mvinFile.clear();
    mvoutFile.clear();
    mbcodeoutFile.clear();
    weightFile.clear();
#endif
}

// set sVppParams default values
sVppParams::sVppParams() {
    nPicStruct = MFX_PICSTRUCT_UNKNOWN;
    bDenoiseEnabled = false;
    nDenoiseLevel = UINT16_MAX;
    bEnableDeinterlacing = false;
    dFrameRate = 0;
    nDstWidth = 0;
    nDstHeight = 0;
    bComposition = false;
    nSuggestSurface = 0;
    // nLADepth = 0;
    // bLABRC = false;
}

// set CompositionParams default values
CompositionParams::CompositionParams() {
    strStreamName.clear();
    nDstX = 0;
    nDstY = 0;
    nDstW = 0;
    nDstH = 0;

    nLumaKeyEnable = 0;
    nLumaKeyMin = 0;
    nLumaKeyMax = 0;

    nGlobalAlphaEnable = 0;
    nGlobalAlpha = 0;

    nPixelAlphaEnable = 0;
    nStreamFourcc = 0;

    nWidth = 0;
    nHeight = 0;

    nCropX = 0;
    nCropY = 0;
    nCropW = 0;
    nCropH = 0;

    nFourCC = 0;
    nPicStruct = 0;
    dFrameRate = 0;
}

// set structure to define values
InputParams::InputParams() {
    bStatistics = false;
    strSessionName.clear();
    strWorkID.clear();
    bIsJoin = false;
    bUseHW = true;
}

CmdProcessor::CmdProcessor() {
    m_nSessionParamId = 0;
    m_strParFileName.clear();
} //CmdProcessor::CmdProcessor()

CmdProcessor::~CmdProcessor() {

} //CmdProcessor::~CmdProcessor()

/**
 * print help information
 */
void CmdProcessor::PrintHelp(const TCHAR *strAppName, const TCHAR *strErrorMessage) {
    if (strErrorMessage) {
        _tprintf(_T("Error: %s\n"), strErrorMessage);
    } else {
        _tprintf(_T("\n"));
        _tprintf(_T("Intel(R) Streaming Media Transcoding Application Version %s\n"), _T(STRFILEVER));
        _tprintf(_T("Copyright (c) 2010 - 2013 Intel Corporation. All Rights Reserved.\n"));
        _tprintf(_T("\n"));
        _tprintf(_T("This software is supplied under the terms of a license  agreement or\n"));
        _tprintf(_T("nondisclosure agreement with Intel Corporation and may not be copied\n"));
        _tprintf(_T("or disclosed except in  accordance  with the terms of that agreement.\n"));
        _tprintf(_T("\n"));
        _tprintf(_T("THIRD PARTY SOFTWARE AND LICENSES (refer to the user documentation for details):\n"));
        _tprintf(_T("This software uses libraries from the FFmpeg project under the LGPLv2.1.\n"));
        _tprintf(_T("This software uses libraries from the Live555 project under the LGPLv2.1.\n"));
        _tprintf(_T("This software uses code from the 'msinttypes' project under the New BSD license.\n"));
        _tprintf(_T("'msinttypes' is Copyright 2006-2013 Alexander Chemeris. This software uses\n"));
        _tprintf(_T("code from the RapidXml project under the MIT license. RapidXml is Copyright\n"));
        _tprintf(_T("2006, 2007 Marcin Kalicinski\n"));
    }
    _tprintf(_T("\n"));

    if (strAppName) {
        _tprintf(_T("Usage: %s -params <SessionParams> or %s -par 1toN.par\n"), strAppName, strAppName);
        _tprintf(_T("       or %s -par_str <parameters string>\n"), strAppName);
    } else {
        _tprintf(_T("Command line parameters:\n"));
    }
    _tprintf(_T("\n"));
    _tprintf(_T("<SessionParams> or 1toN options:\n"));
    _tprintf(_T(" For files: -i::h264|mpeg2|h265 InputURI -o::h264|mpeg2|h265 OutputURI -w width -h height\n"));
    _tprintf(_T("\n Options: \n"));
    _tprintf(_T("   [-name <SessionName>] - Session name for logging.\n"));
    _tprintf(_T("   [-work_id <Work ID>] - Work ID of Aync Message Worker.\n"));
    _tprintf(_T("   [-f <frameRate>] - Video frame rate (frames per second)\n"));
    _tprintf(_T("   [-rc <rateControl>] - Rate control method (CBR, VBR, CQP, AVBR; def: CBR), CQP is needed for FEI\n"));
    _tprintf(_T("   [-b <bitRate>] - Encoded bit rate (Kbits per second)\n"));
    _tprintf(_T("                    (mandatory for CBR and VBR)\n"));
    _tprintf(_T("   [-qp <QP>] - QP value to use (mandatory for CQP)\n"));
    _tprintf(_T("   [-cprofile <profile>] - Codec profile to use (def: let encoder decide)\n"));
    _tprintf(_T("   [-clevel <level>] - Codec level to use (def: let encoder decide)\n"));
    _tprintf(_T("   [-u speed|quality|balanced|<value>] - Target usage (name or value)\n"));
    _tprintf(_T("   [-l <numSlices>] - Number of slices for encoder; default value 0 \n"));
    _tprintf(_T("   [-refnum <number of reference frame>] - Video number of referenc frame.\n"));
    _tprintf(_T("   [-gopPicSize <value>] - Number of pictures within the current GOP\n"));
    _tprintf(_T("   [-gopRefDist <value>] - Distance between I- or P- key frames\n"));
    _tprintf(_T("   [-gopOptFlag CLOSED|STRICT] - Extra GOP options. One option per parameter,\n"));
    _tprintf(_T("              repeat parameter for multiple options.\n"));
    _tprintf(_T("   [-idrInterval <value>] - IDR-frame interval in terms of I-frames\n"));
    _tprintf(_T("   [-bref] - arrange B frames in B pyramid reference structure\n"));
    _tprintf(_T("   [-nobref] - do not use B-pyramid (by default the decision is made by library)\n"));
    _tprintf(_T("   [-ppyr] - enables P-pyramid. (disable - by default)\n"));
    _tprintf(_T("   [-gpb <on, off>] - make HEVC encoder use regular P-frames.(on - by default) \n"));
    _tprintf(_T("   [-w <width>] - Destination picture width, invokes VPP resizing\n"));
    _tprintf(_T("   [-h <height>] - destination picture height, invokes VPP resizing\n"));
    _tprintf(_T("   [-deinterlace] - Configure vpp to perform de-interlacing of the input video stream\n"));
    _tprintf(_T("   [-tff|bff] - Output stream is interlaced, top|bottom fielf first, if not specified progressive is expected\n"));
    _tprintf(_T("   [-denoise <level>] - Enable denoise algorithm. Level is optional. in range [0-100] \n"));
    _tprintf(_T("   [-loop <number>] - Enables resuming reading the input stream from the start when. Number is optional,  (>= 0) is needed\n"));
    _tprintf(_T("             reaching the end, while keeping the output stream active.\n"));
    _tprintf(_T("   [-stats] - Enables collection of pipeline statistics (per frame latency,\n"));
    _tprintf(_T("              frame rate).\n"));
    _tprintf(_T("   [-inputbyFPS] - Input local data to decoder with FPS speed. \n"));
    _tprintf(_T("   [-join] - Join session with other session(s), by default sessions are not joined.It is only used in 1-to-N streaming transcoding\n"));
    _tprintf(_T("   [-mbbrc] - Use the macroblock-level bitrate control algorithm for H264 encoder.\n"));
    //_tprintf(_T("   [-la] - Use the look ahead bitrate control algorithm (LA BRC) for H264 encoder. Supported only with -hw option on 4th Generation Intel Core processors.\n"));
    _tprintf(_T("   [-la] - Use the look ahead bitrate control algorithm (LA BRC) for H264 encoder.\n"));
    _tprintf(_T("   [-lad <depth>] - Depth parameter for the LA BRC, the number of frames to be analyzed before encoding. In rang [10, 100].\n"));
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    _tprintf(_T("   [-roi_fil <roi-file-name>] - Set regions of interest for each frame from <roi-file-name>\n"));
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#if (MFX_VERSION >= 1025)
    _tprintf(_T("   [-mfe_frames <N>] - maximum number of frames to be combined in multi-frame encode pipeline"));
    _tprintf(_T("               0 - default for platform will be used\n"));
    _tprintf(_T("   [-mfe_mode 0|1|2|3] - multi-frame encode operation mode - should be the same for all sessions\n"));
    _tprintf(_T("            0, MFE operates as DEFAULT mode, decided by SDK if MFE enabled\n"));
    _tprintf(_T("            1, MFE is disabled\n"));
    _tprintf(_T("            2, MFE operates as AUTO mode\n"));
    _tprintf(_T("            3, MFE operates as MANUAL mode\n"));

    _tprintf(_T("   [-mfe_timeout <N>] - multi-frame encode timeout in milliseconds - set per sessions control\n"));
#endif
    _tprintf(_T("   [-repartitioncheck <mode> ] - RepartitionCheckEnable mode : 0x10: favor quality, 0x20: favor performance.\n"));
    _tprintf(_T("   [-composite <par_file>] - composite one RGB4|YUV files. The syntax of the parameters file is:\n"));
    _tprintf(_T("                                  stream=<video file name>\n"));
    _tprintf(_T("                                  width=<input video width>\n"));
    _tprintf(_T("                                  height=<input video height>\n"));
    _tprintf(_T("                                  cropx=<input cropX (def: 0)>\n"));
    _tprintf(_T("                                  cropy=<input cropY (def: 0)>\n"));
    _tprintf(_T("                                  cropw=<input cropW (def: width)>\n"));
    _tprintf(_T("                                  croph=<input cropH (def: height)>\n"));
    _tprintf(_T("                                  framerate=<input frame rate (def: 30.0)>\n"));
    _tprintf(_T("                                  fourcc=<format (FourCC) of input video (def: rgb4. support nv12|rgb4)>\n"));
    _tprintf(_T("                                  picstruct=<picture structure of input video,\n"));
    _tprintf(_T("                                             0 = interlaced top    field first\n"));
    _tprintf(_T("                                             2 = interlaced bottom field first\n"));
    _tprintf(_T("                                             1 = progressive (default)>\n"));
    _tprintf(_T("                                  dstx=<X coordinate of input video located in the output (def: 0)>\n"));
    _tprintf(_T("                                  dsty=<Y coordinate of input video located in the output (def: 0)>\n"));
    _tprintf(_T("                                  dstw=<width of input video located in the output (def: width)>\n"));
    _tprintf(_T("                                  dsth=<height of input video located in the output (def: height)>\n\n"));
    _tprintf(_T("                                  AlphaEnable=1\n"));
    _tprintf(_T("                                  Alpha=<Alpha value>\n"));
    _tprintf(_T("                                  LumaKeyEnable=1\n"));
    _tprintf(_T("                                  LumaKeyMin=<Luma key min value>\n"));
    _tprintf(_T("                                  LumaKeyMax=<Luma key max value>\n"));
    _tprintf(_T("                                  ...\n"));
    _tprintf(_T("                                  The parameters file may contain one YUV stream or one RGB4 stream.\n\n"));
    _tprintf(_T("                                  For example:\n"));
    _tprintf(_T("                                  stream=over_lay.bgra\n"));
    _tprintf(_T("                                  width=400\n"));
    _tprintf(_T("                                  height=300\n"));
    _tprintf(_T("                                  cropx=0\n"));
    _tprintf(_T("                                  cropy=0\n"));
    _tprintf(_T("                                  cropw=400\n"));
    _tprintf(_T("                                  croph=300\n"));
    _tprintf(_T("                                  dstx=100\n"));
    _tprintf(_T("                                  dsty=100\n"));
    _tprintf(_T("                                  dstw=400\n"));
    _tprintf(_T("                                  dsth=300\n"));
    _tprintf(_T("                                  PixelAlphaEnable=1\n"));
    _tprintf(_T(" AVC FEI encoder options\n"));
    _tprintf(_T("   [-preenc <ds_strength>] - use extended FEI interface PREENC (RC is forced to constant QP)\n"));
    _tprintf(_T("                            if ds_strength parameter is missed or less than 2, PREENC is used on the full resolution\n"));
    _tprintf(_T("                            otherwise PREENC is used on downscaled (by VPP resize in ds_strength times) surfaces\n"));
    _tprintf(_T("   [-encode] - use extended FEI interface ENC+PAK (FEI ENCODE) (RC is forced to constant QP)\n"));
    _tprintf(_T("   [-pak] - use extended FEI interface PAK (only)\n"));
    _tprintf(_T("   [-enc] - use extended FEI interface ENC (only)\n"));
    _tprintf(_T("   [-encpak] - use extended FEI interface ENC only and PAK only (separate calls)\n"));
    _tprintf(_T("   [-search_path <value>] - defines shape of search path. (0 -full, 1- diamond for AVC FEI) (1 - diamond, 2 - full, 0 - default (full) for HEVC FEI)\n"));
    _tprintf(_T("   [-sub_mb_part_mask <mask_hex>] - specifies which partitions should be excluded from search (default is 0x00 - enable all)\n"));
    _tprintf(_T("   [-sub_pel_mode <mode_hex>] - specifies sub pixel precision for motion estimation 0x00-0x01-0x03 integer-half-quarter (default is 0x03)\n"));
    _tprintf(_T("   [-intra_part_mask <mask_hex>] - specifies what blocks and sub-blocks partitions are enabled for intra prediction (default is 0)\n"));
    _tprintf(_T("   [-adaptive_search] - enables adaptive search\n"));
    _tprintf(_T("   [-search_window <value>] - specifies one of the predefined search path and window size. In range [1,8] (5 is default).\n"));
    _tprintf(_T("                            If non-zero value specified: -ref_window_w / _h, -len_sp are ignored\n"));
    _tprintf(_T("   [-ref_window_w <width>] - width of search region (should be multiple of 4), maximum allowed search window w*h is 2048 for\n"));
    _tprintf(_T("                            one direction and 1024 for bidirectional search\n"));
    _tprintf(_T("   [-ref_window_h <height>] - height of search region (should be multiple of 4), maximum allowed is 32\n"));
    _tprintf(_T("   [-len_sp <length>] - defines number of search units in search path. In range [1,63] (default is 57)\n"));
    _tprintf(_T("   [-repartition_check_fei] - enables additional sub pixel and bidirectional refinements (ENC, ENCODE)\n"));
    _tprintf(_T("   [-multi_pred_l0] - MVs from neighbor MBs will be used as predictors for L0 prediction list (ENC, ENCODE)\n"));
    _tprintf(_T("   [-multi_pred_l1] - MVs from neighbor MBs will be used as predictors for L1 prediction list (ENC, ENCODE)\n"));
    _tprintf(_T("   [-adjust_distortion] - if enabled adds a cost adjustment to distortion, default is RAW distortion (ENC, ENCODE)\n"));
    _tprintf(_T("   [-n_mvpredictors_P_l0 <num>] - number of MV predictors for l0 list of P frames, up to 4 is supported (default is 1) (AVC FEI ENC, ENCODE)\n"));
    _tprintf(_T("   [-n_mvpredictors_B_l0 <num>] - number of MV predictors for l0 list of B frames, up to 4 is supported (default is 1) (AVC FEI ENC, ENCODE)\n"));
    _tprintf(_T("   [-n_mvpredictors_B_l1 <num>] - number of MV predictors for l1 list, up to 4 is supported (default is 0) (AVC FEI ENC, ENCODE)\n"));
    _tprintf(_T("   [-num_predictorsL0 <num>] - number of maximum L0 predictors (default - assign depending on the frame type (HEVC FEI ENCODE)\n"));
    _tprintf(_T("   [-num_predictorsL1 <num>] - number of maximum L1 predictors (default - assign depending on the frame type (HEVC FEI ENCODE)\n"));
    _tprintf(_T("   [-mvPBlocksize <size>] - external MV predictor block size (0 - no MVP, 1 - MVP per 16x16, 2 - MVP per 32x32, 7 - use with -mvpin (HEVC FEI ENCODE)\n"));
    _tprintf(_T("   [-forceCtuSplit] - force splitting CTU into CU at least once (HEVC FEI ENCODE)\n"));
    _tprintf(_T("   [-num_framepartitions <num>] - number of partitions in frame that encoder processes concurrently (1, 2, 4, 8 or 16) (HEVC FEI ENCODE)\n"));
    _tprintf(_T("   [-colocated_mb_distortion] - provides the distortion between the current and the co-located MB. It has performance impact (ENC, ENCODE)\n"));
    _tprintf(_T("                                do not use it, unless it is necessary\n"));

    _tprintf(_T("   [-num_active_P <numRefs>] - number of maximum allowed references for P frames (up to 4(default)); for PAK only limit is 16\n"));
    _tprintf(_T("   [-num_active_BL0 <numRefs>] - number of maximum allowed backward references for B frames (up to 4(default)); for PAK only limit is 16\n"));
    _tprintf(_T("   [-num_active_BL1 <numRefs>] - number of maximum allowed forward references for B frames (up to 2(default) for interlaced, 1(default) for progressive); for PAK only limit is 16\n"));
    _tprintf(_T("   [-mbsize] - with this options size control fields will be used from MB control structure (only ENC+PAK)\n"));
    _tprintf(_T("   [-dblk_idc <value>] - value of DisableDeblockingIdc (default is 0), in range [0,2]\n"));
    _tprintf(_T("   [-dblk_alpha <value>] - value of SliceAlphaC0OffsetDiv2 (default is 0), in range [-6,6]\n"));
    _tprintf(_T("   [-dblk_beta <value>] - va lue of SliceBetaOffsetDiv2 (default is 0), in range [-6,6]\n"));
    _tprintf(_T("   [-chroma_qpi_offset <first_offset>] - first offset used for chroma qp in range [-12, 12] (used in PPS)\n"));
    _tprintf(_T("   [-s_chroma_qpi_offset <second_offset>] - second offset used for chroma qp in range [-12, 12] (used in PPS)\n"));
    _tprintf(_T("   [-transform_8x8_mode_flag] - enables 8x8 transform, by default only 4x4 is used (used in PPS)\n"));
    _tprintf(_T("   [-mvout <file>] - use this to output MV predictors\n"));
    _tprintf(_T("   [-mvin <file>] - use this input to set MV predictor for FEI. PREENC and ENC (ENCODE) expect different structures.\n"));
    _tprintf(_T("   [-mbcode <file>] - file to output per MB information (structure mfxExtFeiPakMBCtrl) for each frame\n"));
    _tprintf(_T("   [-repack_preenc_mv] - use this in pair with -mvin to import preenc MVout directly\n"));
    _tprintf(_T("   [-qrep] - quality predictor MV repacking before encode\n"));

    _tprintf(_T("\n"));
    _tprintf(_T("Example for file transcoding: -i::h264 in.h264 -o::mpeg2 out.mpg -w 640 -h 480 -hw\n"));
    _tprintf(_T("Example of 1toN streaming transcode, contents of 1toN.par:\n"));
    _tprintf(_T("    -name DancingGuy -stats -loop -i::h264 in.h264 -o::sink -join\n"));
    _tprintf(_T("    -i::source -o::h264 out.h264 -h 360 -w 640 -b 2000 -f 29.97 -join\n"));
    _tprintf(_T("    -i::source -o::h264 out.h264 -h 360 -w 640 -b 5000 -f 29.97 -join\n"));
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    _tprintf(_T("ROI file format:\n"));
    _tprintf(_T("<start frame>;<end frame>;\n"));
    _tprintf(_T("<number of ROI>;\n"));
    _tprintf(_T("<left>;<top>;<right>;<bottom>;<qp>;\n"));
    _tprintf(_T("<left>;<top>;<right>;<bottom>;<qp>;\n"));
    _tprintf(_T("... ... ...\n"));
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    _tprintf(_T("\n"));
} // CmdProcessor::PrintHelp(const TCHAR *strAppName, const TCHAR *strErrorMessage)

/**
 * parse command line
 */
mfxStatus CmdProcessor::ParseCmdLine(int argc, TCHAR *argv[]) {

    mfxStatus sts = MFX_ERR_UNSUPPORTED;

    if (1 == argc) {
        PrintHelp(argv[0], NULL);
        return MFX_ERR_UNSUPPORTED;
    }

    // parse command line
    for (int i = 1; i < argc; i++) {
        if (0 == _tcscmp(argv[i], _T("-par"))) {
            // input par file (1ToN)
            i++;
            FILE *pParFile = NULL;
            m_strParFileName = FromTString(argv[i]);
            pParFile = fopen(m_strParFileName.c_str(), "r");
            if (NULL == pParFile) {
                PrintHelp(argv[0], _T("Par File not found"));
                return MFX_ERR_UNSUPPORTED;
            }
            _tprintf(_T("Par file is: ") _T(PRIaSTR) _T("\n\n"), m_strParFileName.c_str());
            // parse par file
            sts = ParseParFile(pParFile);
            // close par file
            fclose(pParFile);
            pParFile = NULL;
            if (MFX_ERR_NONE != sts) {
                PrintHelp(argv[0], _T("Command line is invalid"));
            }
            return sts;
        } else if (0 == _tcscmp(argv[i], _T("-par_str"))) {
            // input par string (1ToN)
            i++;
            string parString = argv[i];
            _tprintf(_T("Par String is: ") _T(PRIaSTR) _T("\n\n"), parString.c_str());
            // parse par String
            sts = ParseParString(parString);
            if (MFX_ERR_NONE != sts) {
                PrintHelp(argv[0], _T("Command line is invalid"));
            }
            return sts;
        } else if (0 == _tcscmp(argv[i], _T("-params"))) {
            // parse command line (1To1)
            i++;
            vector<string> session_args;
            for (int j = i; j < argc; j++) {
                session_args.push_back(FromTString(argv[j]));
            }
            // parse each parameter
            return ParseParams(session_args, PIPELINE);
        } else if (0 == _tcscmp(argv[i], _T("-?")) || 0 == _tcscmp(argv[i], _T("-h"))) {
            // display help info
            PrintHelp(argv[0], NULL);
            return MFX_ERR_UNSUPPORTED;
        } else {
            PrintHelp(argv[0], _T("Invalid input CMD line"));
            return MFX_ERR_UNSUPPORTED;
        }
    }
    // check correctness of input parameters
    sts = VerifyCrossSessionsOptions();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    return sts;

} //mfxStatus CmdProcessor::ParseCmdLine(int argc, TCHAR *argv[])

mfxStatus CmdProcessor::ParseParFile(FILE *parFile) {
    mfxStatus sts = MFX_ERR_UNSUPPORTED;
    if (!parFile)
        return MFX_ERR_UNSUPPORTED;

    // calculate file size
    fseek(parFile, 0, SEEK_END);
    mfxU32 fileSize = ftell(parFile) + 1;
    fseek(parFile, 0, SEEK_SET);

    // allocate buffer for parsing
    std::vector<char> parBuf;
    parBuf.resize(fileSize);
    int lineIndex = 0;
    std::vector<std::string> args;

    std::vector<char>::iterator currPos, prevPos;

    while (true) {
        if (NULL == fgets(&parBuf.at(0), fileSize, parFile))
            break;  // Finished

        args.clear();
        for (currPos = prevPos = parBuf.begin(); true; currPos++) {
            if ((currPos != parBuf.begin()) && (*currPos == ' ')) {
                if (prevPos != currPos) {
                    args.push_back(std::string(prevPos, currPos));
                }
                prevPos = currPos;
                prevPos++;
            }
            if ((currPos == parBuf.end()) || (*currPos == '\n') || (*currPos == '\0')) {
                if (prevPos != currPos) {
                    args.push_back(std::string(prevPos, currPos));
                }
                break;
            }
        }
        // get par a line information
        if (!args.empty()) {

            for (string::size_type i = 0; i < args.size(); i++) {
                // encode and VPP parameter
                if (0 == args[i].compare("-i::source")) {
                    sts = ParseParams(args, ENCODE_VPP);
                    break;
                }
                // decode parameter
                if (0 == args[i].compare("-o::sink")) {
                    sts = ParseParams(args, DECODE);
                    break;
                }
            }

            if (MFX_ERR_NONE != sts) {
                _tprintf(_T("Invalid parameters in par file at line %d\n"), lineIndex);
                PrintHelp(NULL, _T("Error in par file parameters"));
            }
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
        }
        lineIndex++;
    }

    return MFX_ERR_NONE;

} //mfxStatus CmdProcessor::ParseParFile(FILE *parFile)

mfxStatus CmdProcessor::ParseParString(std::string& str) {
    mfxStatus sts = MFX_ERR_UNSUPPORTED;

    std::vector<std::string> args;
    size_t startPos = 0;

    while (true) {
        //int currPos = str.find(' ', startPos);
        size_t currPos = startPos;
        size_t endPos = str.find(';', startPos);
        if (endPos == std::string::npos)
            break;
        args.clear();
        for (; currPos < endPos;) {
            currPos = str.find(' ', startPos);
            if (startPos == currPos) {
                startPos++;
                continue;
            }
            if (currPos > endPos || currPos == std::string::npos) {
                currPos = endPos;
            }

            args.push_back(str.substr(startPos, (currPos - startPos)));
            startPos = currPos + 1;
        }


        // get par a line information
        if (!args.empty()) {

            for (string::size_type i = 0; i < args.size(); i++) {
                // encode and VPP parameter
                if (0 == args[i].compare("-i::source")) {
                    sts = ParseParams(args, ENCODE_VPP);
                    break;
                }
                // decode parameter
                if (0 == args[i].compare("-o::sink")) {
                    sts = ParseParams(args, DECODE);
                    break;
                }
            }

            if (MFX_ERR_NONE != sts) {
                _tprintf(_T("Invalid parameters in par string %s\n"), str.c_str());
                PrintHelp(NULL, _T("Error in par file parameters"));
            }
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
        }
    }

    return MFX_ERR_NONE;

} //mfxStatus CmdProcessor::ParseParString(std::string& string)

#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
bool isspace(char a) { return (std::isspace(a) != 0); }

bool is_not_allowed_char(char a) {
    return (std::isdigit(a) != 0) && (std::isspace(a) != 0) && (a != ';') && (a != '-');
}

// Parse ROI setting frome ROI file
bool CmdProcessor::ParseRoiFile(std::map<int, mfxExtEncoderROI>& roiData, std::string& roiFile)
{
    FILE *roi_file = NULL;

    if ((roi_file = fopen(roiFile.c_str(), "rb")) == NULL) {
        fprintf(stderr, "Open ROI file failed.\n");
        return false;
    }

    roiData.clear();

    if (roi_file)
    {
        // read file to buffer
        fseek(roi_file, 0, SEEK_END);
        long file_size = ftell(roi_file);
        rewind (roi_file);
        std::vector<char> buffer(file_size);
        char *roi_data = &buffer[0];
        if (file_size<0 || (size_t)file_size != fread(roi_data, 1, file_size, roi_file))
        {
            fclose(roi_file);
            return false;
        }
        fclose(roi_file);

        // search for not allowed characters
        char *not_allowed_char = std::find_if(roi_data, roi_data + file_size,
            is_not_allowed_char);
        if (not_allowed_char != (roi_data + file_size))
        {
            return false;
        }

        // get unformatted roi data
        std::string unformatted_roi_data;
        unformatted_roi_data.clear();
        std::remove_copy_if(roi_data, roi_data + file_size,
            std::inserter(unformatted_roi_data, unformatted_roi_data.end()),
            [](char c){return isspace(c);});

        // split data to items
        std::stringstream unformatted_roi_data_ss(unformatted_roi_data);
        std::vector<std::string> items;
        items.clear();
        std::string item;
        while (std::getline(unformatted_roi_data_ss, item, ';'))
        {
            items.push_back(item);
        }

        // parse data and store roi data for each frame
        unsigned int item_ind = 0;
        while (1)
        {
            if (item_ind >= items.size()) break;

            mfxExtEncoderROI frame_roi;
            memset(&frame_roi, 0, sizeof(frame_roi));
            frame_roi.Header.BufferId = MFX_EXTBUFF_ENCODER_ROI;
            frame_roi.ROIMode = MFX_ROI_MODE_QP_DELTA;

            //unsigned int item_ind = 0;
            int startFrame = std::atoi(items[item_ind++].c_str());
            int endFrame = std::atoi(items[item_ind++].c_str());
            int roi_num = std::atoi(items[item_ind].c_str());
            if (roi_num < 0 || roi_num > (int)(sizeof(frame_roi.ROI) / sizeof(frame_roi.ROI[0])))
            {
                roiData.clear();
                return false;
            }
            if ((item_ind + 5 * roi_num) >= items.size())
            {
                roiData.clear();
                return false;
            }

            for (int i = 0; i < roi_num; i++)
            {
                // do not handle out of range integer errors
                frame_roi.ROI[i].Left = std::atoi(items[item_ind + i * 5 + 1].c_str());
                frame_roi.ROI[i].Top = std::atoi(items[item_ind + i * 5 + 2].c_str());
                frame_roi.ROI[i].Right = std::atoi(items[item_ind + i * 5 + 3].c_str());
                frame_roi.ROI[i].Bottom = std::atoi(items[item_ind + i * 5 + 4].c_str());
                frame_roi.ROI[i].DeltaQP = (mfxI16) std::atoi(items[item_ind +i * 5 + 5].c_str());
                fprintf(stderr, "roi num = %d, frame num = %d:%d, %u:%u:%u:%u:%u\n", i, startFrame,endFrame, frame_roi.ROI[i].Left, frame_roi.ROI[i].Top, frame_roi.ROI[i].Right, frame_roi.ROI[i].Bottom, frame_roi.ROI[i].DeltaQP);
            }
            frame_roi.NumROI = (mfxU16) roi_num;
            for (int i = startFrame; i <= endFrame; i++) {

                roiData[i] = frame_roi;
            }
            item_ind = item_ind + roi_num * 5 + 1;
        }
    }
    else {
        return false;
    }
    return true;
}
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
mfxStatus CmdProcessor::ParseParams(const std::vector<string>& argv, paramMode mode) {
    mfxStatus sts;
    int argc = argv.size();
    int i;

    DecParams decInputParams;
    EncParams encInputParams;
    sVppParams vppInputParams;
    CompositionParams compositionParams;

    for (i = 0; i < argc; i++) {
        if (0 == argv[i].compare(0, 4, "-i::")) { // Source spec -i::<codec>[::<transport>]
            std::string codec, tmp;
            size_t idx = argv[i].find_first_of("::");
            tmp = argv[i].substr(idx + 2);
            size_t idx2 = tmp.find("::");
            if (string::npos == idx2) {
                codec = tmp;
            } else {
                codec = tmp.substr(0, idx2);
            }

            // Which input codec?
            if (0 == codec.compare("mpeg2")) {
                decInputParams.nDecodeId = MFX_CODEC_MPEG2;
            } else if (0 == codec.compare("h264")) {
                decInputParams.nDecodeId = MFX_CODEC_AVC;
            } else if (0 == codec.compare("h265")) {
                decInputParams.nDecodeId = MFX_CODEC_HEVC;
            } else if (0 == codec.compare("source")) {
                continue;
            } else {
                std::ostringstream os;
                os << "Invalid input codec: " << codec << "\n";
                _tprintf(_T("Error: ") _T(PRIaSTR) _T("\n"), os.str().c_str());
                DEBUG_ERROR((_T(PRIaSTR), os.str().c_str() ));
                PrintHelp(NULL, _T("Command line is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }

            VAL_CHECK((i + 1) >= argc);
            i++;
            decInputParams.strSrcURI = argv[i];
        } else if (0 == argv[i].compare(0, 4, "-o::")) {
            std::string codec, transport;
            const int codec_start = 4;
            size_t idx = argv[i].find("::", codec_start);
            if (idx == std::string::npos) {
                codec = argv[i].substr(codec_start);
            } else {
                codec = argv[i].substr(codec_start, idx - codec_start);
                transport = argv[i].substr(idx + 2);  // Skip the "::"
            }

            // Which output codec?
            if (0 == codec.compare("mpeg2")) {
                encInputParams.nEncodeId = MFX_CODEC_MPEG2;
            } else if (0 == codec.compare("h264")) {
                encInputParams.nEncodeId = MFX_CODEC_AVC;
            } else if (0 == codec.compare("h265")) {
                encInputParams.nEncodeId = MFX_CODEC_HEVC;
            } else if (0 == codec.compare("sink")) {
                continue;
            } else if (0 == codec.compare("null")) {
                // No output
                encInputParams.nEncodeId = MFX_CODEC_NULL;
            } else {
                std::ostringstream os;
                os << "Invalid output codec: " << codec << "\n";
                _tprintf(_T("Error: ") _T(PRIaSTR) _T("\n"), os.str().c_str());
                DEBUG_ERROR((_T(PRIaSTR), os.str().c_str()));
                PrintHelp(NULL, _T("Command line is invalid"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (encInputParams.nEncodeId != MFX_CODEC_NULL) {
                // Which output transport?
                if (!transport.empty()) {
                    if (0 == transport.compare("rtp")) {
                        encInputParams.outputType = BitstreamSink::E_RTP_ES;
                    } else if (0 == transport.compare("rtp::ts")) {
                        encInputParams.outputType = BitstreamSink::E_RTP_TS;
                    }

                    VAL_CHECK((i + 2) >= argc);
                    encInputParams.strDstURI = "rtp://" + argv[i+1] + ":" + argv[i+2];
                    i += 2;
                } else { // No transport means local file
                    VAL_CHECK((i + 1) >= argc);
                    i++;
                    encInputParams.outputType = BitstreamSink::E_FILE;
                    encInputParams.strDstURI = argv[i];
                }
            }
        } else if (0 == argv[i].compare("-name")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            m_inputParams.strSessionName = argv[i];
        } else if (0 == argv[i].compare("-work_id")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            m_inputParams.strWorkID = argv[i];
        } else if (0 == argv[i].compare("-f")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%lf", &vppInputParams.dFrameRate);
        } else if (0 == argv[i].compare("-rc")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            mfxStatus sts = ParseRateControlMethod(argv[i], encInputParams);
            STS_CHECK(sts)
        } else if (0 == argv[i].compare("-b")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            // if not set, xcode will compute it automatic
            sscanf_s(argv[i].c_str(), "%u", &encInputParams.nBitRate);
        } else if (0 == argv[i].compare("-la")) {
            encInputParams.bLABRC = true;
        } else if (0 == argv[i].compare("-mbbrc")) {
            encInputParams.bMBBRC = true;
        } else if (0 == argv[i].compare("-lad")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nLADepth);
            VAL_CHECK(encInputParams.nLADepth < 10 || encInputParams.nLADepth > 100);
        } else if (0 == argv[i].compare("-repartitioncheck")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "0x%hx", &encInputParams.nRepartitionCheckMode);
            printf("encInputParams.nRepartitionCheckMode: %d %hx \n", encInputParams.nRepartitionCheckMode, encInputParams.nRepartitionCheckMode);
            VAL_CHECK( (encInputParams.nRepartitionCheckMode!= MFX_CODINGOPTION_OFF) && (encInputParams.nRepartitionCheckMode != MFX_CODINGOPTION_ON) );
        } else if (0 == argv[i].compare("-tff")) {
            vppInputParams.nPicStruct = MFX_PICSTRUCT_FIELD_TFF;
        } else if (0 == argv[i].compare("-bff")) {
            vppInputParams.nPicStruct = MFX_PICSTRUCT_FIELD_BFF;
        } else if (0 == argv[i].compare("-qp")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nQP) != 1);
        } else if (0 == argv[i].compare("-cprofile")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            mfxStatus sts = ParseCodecProfile(argv[i].c_str(), encInputParams);
            STS_CHECK(sts)
        } else if (0 == argv[i].compare("-clevel")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            mfxStatus sts = ParseCodecLevel(argv[i].c_str(), encInputParams);
            STS_CHECK(sts)
        } else if (0 == argv[i].compare("-u")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            mfxStatus sts = ParseTargetUsage(argv[i], encInputParams.nTargetUsage);
            STS_CHECK(sts)
        } else if (0 == argv[i].compare("-w")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &vppInputParams.nDstWidth);
        } else if (0 == argv[i].compare("-h")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &vppInputParams.nDstHeight);
        } else if (0 == argv[i].compare("-l")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nSlices);
        } else if (0 == argv[i].compare("-gopPicSize")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nGopPicSize) == EOF);
        } else if (0 == argv[i].compare("-gopRefDist")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nGopRefDist) == EOF);
        } else if (0 == argv[i].compare("-refnum")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nRefNum) == EOF);
        } else if (0 == argv[i].compare("-gopOptFlag")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            string gopOption = argv[i];
            transform(gopOption.begin(), gopOption.end(), gopOption.begin(), ::toupper);
#define GOP_OPTION(opt) if (0 == gopOption.compare(#opt)) { encInputParams.nGopOptFlag |= MFX_GOP_##opt; }
            GOP_OPTION(CLOSED) else GOP_OPTION(STRICT) else {
                _tprintf(_T("Error: unknown GOP option; valid options are CLOSED, STRICT\n"));
                VAL_CHECK(true);
            }
#undef GOP_OPTION
        } else if (0 == argv[i].compare("-idrInterval")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nIdrInterval) == EOF);
        } else if (0 == argv[i].compare("-bref")) {
            encInputParams.bRefType = MFX_B_REF_PYRAMID;
        } else if (0 == argv[i].compare("-nobref")) {
            encInputParams.bRefType = MFX_B_REF_OFF;
        } else if (0 == argv[i].compare("-ppyr")) {
            encInputParams.pRefType = MFX_P_REF_PYRAMID;
        } else if (0 == argv[i].compare("-gpb")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            if (!argv[i].compare("on"))
                encInputParams.nGPB = MFX_CODINGOPTION_ON;
            else
                encInputParams.nGPB = MFX_CODINGOPTION_OFF;
        } else if (0 == argv[i].compare("-stats")) {
            m_inputParams.bStatistics = true;
        } else if (0 == argv[i].compare("-inputbyFPS")) {
            decInputParams.bInputByFPS = true;
        } else if (0 == argv[i].compare("-join")) {
            m_inputParams.bIsJoin = true;
        } else if (0 == argv[i].compare("-sw")) {
            m_inputParams.bUseHW = false;
            encInputParams.bUseHW = false;
        } else if (0 == argv[i].compare("-d")) {
            encInputParams.bDump = true;
        } else if (0 == argv[i].compare("-log")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            mfxStatus sts = ParseLogFlags(argv[i]);
            STS_CHECK(sts)
        } else if (0 == argv[i].compare("-loop")) {
            decInputParams.bIsLoopMode = true;
            VAL_CHECK((i + 1) >= argc);
            //sscanf_s(argv[i + 1].c_str(), "%hu", &decInputParams.nLoopNum);
            decInputParams.nLoopNum = (mfxU16)atoi(argv[i + 1].c_str());
            i++;
            if (0 == decInputParams.nLoopNum) {
                decInputParams.nLoopNum = 1;
            }
        } else if (0 == argv[i].compare("-denoise")) {
            vppInputParams.bDenoiseEnabled = true;
            if ((i + 1) < argc) {
                sscanf_s(argv[i + 1].c_str(), "%hu", &vppInputParams.nDenoiseLevel);
                if (UINT16_MAX != vppInputParams.nDenoiseLevel) {
                    i++;
                    if (vppInputParams.nDenoiseLevel > 100) {
                        PrintHelp(NULL, _T("de-noise level is out of range [0, 100] !\n"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                }
            }
        } else if (0 == argv[i].compare("-tiles")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            mfxStatus sts = ParseTiles(argv[i], encInputParams);
            STS_CHECK(sts)
        } else if (0 == argv[i].compare("-deinterlace")) {
            vppInputParams.bEnableDeinterlacing = true;
        } else if (0 == argv[i].compare("-composite")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            if (ParseCompositionParfile(argv[i], compositionParams) != MFX_ERR_NONE) {
                PrintHelp(NULL, _T("Incorrect parfile for -composite"));
                return MFX_ERR_UNSUPPORTED;
            }
            vppInputParams.bComposition = true;
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
        }else if(0 == argv[i].compare("-roi_file")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            std::string fileName(argv[i]);
            if (!ParseRoiFile(encInputParams.extRoiData, fileName)) {
                PrintHelp(NULL, _T("Incorrect ROI file format."));
                return MFX_ERR_UNSUPPORTED;
            }
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#if (MFX_VERSION >= 1025)
        } else if(0 == argv[i].compare("-mfe_frames"))
        {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nMFEFrames) == EOF);
        }
        else if(0 == argv[i].compare("-mfe_mode"))
        {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%hu", &encInputParams.nMFEMode) == EOF);
        }
        else if(0 == argv[i].compare("-mfe_timeout"))
        {
            VAL_CHECK((i + 1) >= argc);
            i++;
            VAL_CHECK(sscanf_s(argv[i].c_str(), "%u", &encInputParams.nMFETimeout) == EOF);
#endif
#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
        } else if (0 == argv[i].compare("-preenc")) {
            encInputParams.bPreEnc = true;
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hhu", &encInputParams.fei_ctrl.nDSstrength);
            VAL_CHECK(!(encInputParams.fei_ctrl.nDSstrength < 3
                     || encInputParams.fei_ctrl.nDSstrength == 4
                     || encInputParams.fei_ctrl.nDSstrength == 8));
        } else if (0 == argv[i].compare("-encode")) {
            encInputParams.bEncode = true;
        } else if (0 == argv[i].compare("-pak")) {
            encInputParams.bPAK = true;
        } else if (0 == argv[i].compare("-enc")) {
            encInputParams.bENC = true;
        } else if (0 == argv[i].compare("-encpak")) {
            encInputParams.bENCPAK = true;
        }else if (0 == argv[i].compare("-repartition_check_fei")) {
            encInputParams.fei_ctrl.bRepartitionCheckEnable = true;
        }else if (0 == argv[i].compare("-multi_pred_l0")){
            encInputParams.fei_ctrl.bMultiPredL0 = true;
        }else if (0 == argv[i].compare("-multi_pred_l1")){
            encInputParams.fei_ctrl.bMultiPredL1 = true;
        }else if (0 == argv[i].compare("-adjust_distortion")){
            encInputParams.fei_ctrl.bDistortionType = true;
        }else if (0 == argv[i].compare("-colocated_mb_distortion")){
            encInputParams.fei_ctrl.bColocatedMbDistortion = true;
        }else if (0 == argv[i].compare("-n_mvpredictors_P_l0")){
            encInputParams.fei_ctrl.bNPredSpecified_Pl0 = true;
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nMVPredictors_Pl0);
        }else if (0 == argv[i].compare("-n_mvpredictors_B_l0")){
            encInputParams.fei_ctrl.bNPredSpecified_Bl0 = true;
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nMVPredictors_Bl0);
        }else if (0 == argv[i].compare("-n_mvpredictors_B_l1")){
            encInputParams.fei_ctrl.bNPredSpecified_Bl1 = true;
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nMVPredictors_Bl1);
        //20170809 add:
        }else if (0 == argv[i].compare("-num_active_P")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nRefActiveP);
        }else if (0 == argv[i].compare("-num_active_BL0")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nRefActiveBL0);
        }else if (0 == argv[i].compare("-num_active_BL1")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nRefActiveBL1);
        }else if (0 == argv[i].compare("-num_predictorsL0")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nNumMvPredictors[0]);
        }else if (0 == argv[i].compare("-num_predictorsL1")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nNumMvPredictors[1]);
        }else if (0 == argv[i].compare("-mvPBlocksize")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nMVPredictor);
        }else if (0 == argv[i].compare("-num_framepartitions")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nNumFramePartitions);
        }else if (0 == argv[i].compare("-forceCtuSplit")){
            encInputParams.fei_ctrl.nForceCtuSplit = 1;
        }else if (0 == argv[i].compare("-mbsize")){
            encInputParams.fei_ctrl.bMBSize = true;
        }else if (0 == argv[i].compare("-dblk_idc")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nDisableDeblockingIdc);

            VAL_CHECK(encInputParams.fei_ctrl.nDisableDeblockingIdc > 2);
        }else if (0 == argv[i].compare("-dblk_alpha")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hd", &encInputParams.fei_ctrl.nSliceAlphaC0OffsetDiv2);

            VAL_CHECK((encInputParams.fei_ctrl.nSliceAlphaC0OffsetDiv2 > 6)||(encInputParams.fei_ctrl.nSliceAlphaC0OffsetDiv2 < -6));
        }else if (0 == argv[i].compare("-dblk_beta")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hd", &encInputParams.fei_ctrl.nSliceBetaOffsetDiv2);

            VAL_CHECK((encInputParams.fei_ctrl.nSliceBetaOffsetDiv2 > 6)||(encInputParams.fei_ctrl.nSliceBetaOffsetDiv2 < -6));
        }else if (0 == argv[i].compare("-chroma_qpi_offset")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hd", &encInputParams.fei_ctrl.nChromaQPIndexOffset);

            VAL_CHECK((encInputParams.fei_ctrl.nChromaQPIndexOffset > 12)||(encInputParams.fei_ctrl.nChromaQPIndexOffset < -12));
        }else if (0 == argv[i].compare("-s_chroma_qpi_offset")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hd", &encInputParams.fei_ctrl.nSecondChromaQPIndexOffset);

            VAL_CHECK((encInputParams.fei_ctrl.nSecondChromaQPIndexOffset > 12)||(encInputParams.fei_ctrl.nSecondChromaQPIndexOffset < -12));
        }else if (0 == argv[i].compare("-transform_8x8_mode_flag")){
            encInputParams.fei_ctrl.bTransform8x8ModeFlag = true;
        }else if(0 == argv[i].compare("-mvout")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            encInputParams.mvoutFile = argv[i];
        }else if(0 == argv[i].compare("-mbcode")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            encInputParams.mbcodeoutFile = argv[i];
        }else if(0 == argv[i].compare("-mvin")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            encInputParams.mvinFile = argv[i];
        }else if(0 == argv[i].compare("-weights")){
            VAL_CHECK((i + 1) >= argc);
            i++;
            encInputParams.weightFile = argv[i];
        }else if (0 == argv[i].compare("-repack_preenc_mv")){
            encInputParams.fei_ctrl.bRepackPreencMV = true;
        }else if (0 == argv[i].compare("-search_path")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nSearchPath);
            VAL_CHECK(!(encInputParams.fei_ctrl.nSearchPath == 0 || encInputParams.fei_ctrl.nSearchPath == 1 || encInputParams.fei_ctrl.nSearchPath == 2));
        } else if (0 == argv[i].compare("-len_sp")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nLenSP);

            //In range [1,63] (default is 57)
            VAL_CHECK(encInputParams.fei_ctrl.nLenSP < 1 || encInputParams.fei_ctrl.nLenSP > 63);
        } else if (0 == argv[i].compare("-sub_mb_part_mask")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hx", &encInputParams.fei_ctrl.nSubMBPartMask);

            //Specifies which partitions should be excluded from search (default is 0x00 - enable all)
            VAL_CHECK(encInputParams.fei_ctrl.nSubMBPartMask >= 0x7f);
        } else if (0 == argv[i].compare("-intra_part_mask")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hx", &encInputParams.fei_ctrl.nIntraPartMask);

            //Specifies what blocks and sub-blocks partitions are enabled for intra prediction (default is 0)
            VAL_CHECK(encInputParams.fei_ctrl.nIntraPartMask >= 0x07);
        } else if (0 == argv[i].compare("-sub_pel_mode")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hx", &encInputParams.fei_ctrl.nSubPelMode);

            //Specifies sub pixel precision for motion estimation 0x00-0x01-0x03 integer-half-quarter (default is 0x03)
            VAL_CHECK(encInputParams.fei_ctrl.nSubPelMode != 0x00
                   && encInputParams.fei_ctrl.nSubPelMode != 0x01
                   && encInputParams.fei_ctrl.nSubPelMode != 0x03);
        } else if (0 == argv[i].compare("-adaptive_search")) {
            encInputParams.fei_ctrl.nAdaptiveSearch = 1;
        } else if (0 == argv[i].compare("-search_window")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nSearchWindow);

            //specifies one of the predefined search path and window size.
            //In range [1,8] (5 is default).If non-zero value specified: -ref_window_w / _h, -len_sp are ignored
            VAL_CHECK(encInputParams.fei_ctrl.nSearchWindow > 8);
        } else if (0 == argv[i].compare("-ref_window_w")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nRefWidth);

            //width of search region (should be multiple of 4),
            //maximum allowed search window w*h is 2048 for one direction and 1024 for bidirectional search
            VAL_CHECK(encInputParams.fei_ctrl.nRefWidth > 64);
        } else if (0 == argv[i].compare("-ref_window_h")) {
            VAL_CHECK((i + 1) >= argc);
            i++;
            sscanf_s(argv[i].c_str(), "%hu", &encInputParams.fei_ctrl.nRefHeight);

            //height of search region (should be multiple of 4), maximum allowed is 32
            VAL_CHECK(encInputParams.fei_ctrl.nRefHeight > 64);
        } else if (0 == argv[i].compare("-qrep")) {
            encInputParams.bQualityRepack = true;
#endif
        } else {
            std::ostringstream os;
            os << "Invalid argument: " << argv[i] << "\n";
            _tprintf(_T("Warning: ") _T(PRIaSTR) _T("\n"), os.str().c_str());
            //PrintHelp(NULL, _T("Command line is invalid"));
            i++;
        }
    }
    sts = VerifyAndCorrectInputParams(decInputParams, encInputParams, vppInputParams, mode);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if (mode == PIPELINE) {
        m_inputParams.decParamsList.push_back(decInputParams);
        m_inputParams.encParamsList.push_back(encInputParams);
        if (vppInputParams.bComposition) {
            vppInputParams.compositionParams = compositionParams;
        }
        m_inputParams.vppParamsList.push_back(vppInputParams);
    } else if (mode == DECODE) {
        m_inputParams.decParamsList.push_back(decInputParams);
    } else if (mode == ENCODE_VPP) {
        m_inputParams.encParamsList.push_back(encInputParams);
        if (vppInputParams.bComposition) {
            vppInputParams.compositionParams = compositionParams;
        }
        m_inputParams.vppParamsList.push_back(vppInputParams);
    } else {
        std::ostringstream os;
        os << __LINE__ << ": Invalid argument: " << argv[i] << "\n";
        _tprintf(_T("Error: ") _T(PRIaSTR) _T("\n"), os.str().c_str());
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;

} //mfxStatus CmdProcessor::ParseParamsForOneSession()

mfxStatus CmdProcessor::ParseRateControlMethod(const std::string& arg, EncParams &encInputParams) {
    string rc_method = arg;
    transform(rc_method.begin(), rc_method.end(), rc_method.begin(), ::toupper);

#define RC_METHOD(method) (0 == rc_method.compare(#method)) ? MFX_RATECONTROL_##method
    encInputParams.nRateControlMethod = RC_METHOD(CBR) : RC_METHOD(VBR) : RC_METHOD(AVBR) : RC_METHOD(CQP) : 0;
#undef RC_METHOD
    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseCodecProfile(const std::string& arg, EncParams &encInputParams) {
    string profile = arg;
    transform(profile.begin(), profile.end(), profile.begin(), ::toupper);

    switch (encInputParams.nEncodeId) {
    case MFX_CODEC_AVC: {
#define CODEC_PROFILE(prof) (0 == profile.compare(#prof)) ? MFX_PROFILE_AVC_##prof
        encInputParams.nCodecProfile = CODEC_PROFILE(BASELINE) : CODEC_PROFILE(MAIN) : CODEC_PROFILE(EXTENDED) : CODEC_PROFILE(HIGH) :
                                       CODEC_PROFILE(CONSTRAINED_BASELINE) : CODEC_PROFILE(CONSTRAINED_HIGH) :
                                       CODEC_PROFILE(PROGRESSIVE_HIGH) : MFX_PROFILE_UNKNOWN;
#undef CODEC_PROFILE
        break;
    }
    case MFX_CODEC_MPEG2: {
#define CODEC_PROFILE(prof) (0 == profile.compare(#prof)) ? MFX_PROFILE_MPEG2_##prof
        encInputParams.nCodecProfile = CODEC_PROFILE(SIMPLE) : CODEC_PROFILE(MAIN) : CODEC_PROFILE(HIGH) : MFX_PROFILE_UNKNOWN;
#undef CODEC_PROFILE
        break;
    }
    case MFX_CODEC_HEVC: {
#define CODEC_PROFILE(prof) (0 == profile.compare(#prof)) ? MFX_PROFILE_HEVC_##prof
        encInputParams.nCodecProfile = CODEC_PROFILE(MAIN) : CODEC_PROFILE(MAIN10) : CODEC_PROFILE(MAINSP) :
                                       CODEC_PROFILE(REXT) : MFX_PROFILE_UNKNOWN;
#undef CODEC_PROFILE
        break;
    }
    default:
        _tprintf(_T("Error: Output codec must be specified before codec profile (-cprofile)\n"));
        return MFX_ERR_UNSUPPORTED;
        break;
    }

    if (encInputParams.nCodecProfile == MFX_PROFILE_UNKNOWN) {
        _tprintf(_T("Error: Unknown profile '") _T(PRIaSTR) _T("' specified\n"), profile.c_str());
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseCodecLevel(const std::string& arg, EncParams &encInputParams) {
    string level = arg;
    transform(level.begin(), level.end(), level.begin(), ::toupper);

    switch (encInputParams.nEncodeId) {
    case MFX_CODEC_AVC: {
#define CODEC_LEVEL(lvl) (0 == level.compare(#lvl)) ? MFX_LEVEL_AVC_##lvl
        encInputParams.nCodecLevel = CODEC_LEVEL(1) : CODEC_LEVEL(1b) : CODEC_LEVEL(11) : CODEC_LEVEL(12) : CODEC_LEVEL(13) : CODEC_LEVEL(2) :
                                     CODEC_LEVEL(21) : CODEC_LEVEL(22) : CODEC_LEVEL(3) : CODEC_LEVEL(31) : CODEC_LEVEL(32) : CODEC_LEVEL(4) :
                                     CODEC_LEVEL(41) : CODEC_LEVEL(42) : CODEC_LEVEL(5) : CODEC_LEVEL(51) : CODEC_LEVEL(52) : MFX_LEVEL_UNKNOWN;
#undef CODEC_LEVEL
        break;
    }
    case MFX_CODEC_MPEG2: {
#define CODEC_LEVEL(lvl) (0 == level.compare(#lvl)) ? MFX_LEVEL_MPEG2_##lvl
        encInputParams.nCodecLevel = CODEC_LEVEL(LOW) : CODEC_LEVEL(MAIN) : CODEC_LEVEL(HIGH) : CODEC_LEVEL(HIGH1440) : MFX_LEVEL_UNKNOWN;
#undef CODEC_LEVEL
        break;
    }
    case MFX_CODEC_HEVC: {
#define CODEC_LEVEL(lvl) (0 == level.compare(#lvl)) ? MFX_LEVEL_HEVC_##lvl
        encInputParams.nCodecLevel = CODEC_LEVEL(1) : CODEC_LEVEL(2) : CODEC_LEVEL(21) : CODEC_LEVEL(3) : CODEC_LEVEL(31) : CODEC_LEVEL(4) :
                                     CODEC_LEVEL(41) : CODEC_LEVEL(5) : CODEC_LEVEL(51) : CODEC_LEVEL(52) : CODEC_LEVEL(6) : CODEC_LEVEL(61) :
                                     CODEC_LEVEL(62) : MFX_LEVEL_UNKNOWN;
#undef CODEC_LEVEL
        if (MFX_LEVEL_UNKNOWN == encInputParams.nCodecLevel) {
#define TIER_LEVEL(lvl) (0 == level.compare(#lvl)) ? MFX_TIER_HEVC_##lvl
            encInputParams.nCodecLevel = TIER_LEVEL(MAIN) : TIER_LEVEL(HIGH) : MFX_LEVEL_UNKNOWN;
#undef TIER_LEVEL
        }
        break;
    }
    default:
        _tprintf(_T("Error: Output codec must be specified before codec level (-clevel)\n"));
        return MFX_ERR_UNSUPPORTED;
        break;
    }

    if (encInputParams.nCodecLevel == MFX_LEVEL_UNKNOWN) {
        _tprintf(_T("Error: Unknown codec level '") _T(PRIaSTR) _T("' specified\n"), level.c_str());
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseTargetUsage(const std::string& strInput, mfxU16& rTargetUsage) {
    unsigned int tu_val;

    string tu = strInput;
    transform(tu.begin(), tu.end(), tu.begin(), ::toupper);

    if (0 == tu.compare("QUALITY")) {
        rTargetUsage = MFX_TARGETUSAGE_BEST_QUALITY;
    } else if (0 == tu.compare("SPEED")) {
        rTargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
    } else if (0 == tu.compare("BALANCED")) {
        rTargetUsage = MFX_TARGETUSAGE_BALANCED;
    } else if (sscanf_s(strInput.c_str(), "%u", &tu_val) == 1) {
        rTargetUsage = tu_val;
    } else {
        _tprintf(_T("Error: Invalid targetUsage value '") _T(PRIaSTR) _T("' specified\n"), strInput.c_str());
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseLogFlags(const std::string& strInput) {
    string log = strInput;
    transform(log.begin(), log.end(), log.begin(), ::toupper);

    int lastIdx = 0;
    size_t idx = std::string::npos;
    do {
        idx = log.find(':', lastIdx);
        std::string flag = (idx == std::string::npos) ? log.substr(lastIdx) : log.substr(lastIdx, idx - lastIdx);
        if (0 == flag.compare("ALL")) {
            s_TranscoderLogFlags |= 0xFFFFFFFF;
        }
#define DEF_LOG_FLAG(NAME, VAL) else if (0 == flag.compare(#NAME)) { s_TranscoderLogFlags |= XC_LOG_##NAME; }
#include "log_flags.h"
#undef DEF_LOG_FLAG
        else {
            _tprintf(_T("Error: Unknown log flag '") _T(PRIaSTR) _T("' specified\n"), flag.c_str());
            return MFX_ERR_UNSUPPORTED;
        }
        lastIdx = idx + 1;
    } while (idx != std::string::npos);

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseTiles(const std::string& arg, EncParams &encInputParams) {
    string str = arg;
    transform(str.begin(), str.end(), str.begin(), ::toupper);

    if (str.empty()) {
        _tprintf(_T("Error: Please set value of -tiles.\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (MFX_CODEC_HEVC != encInputParams.nEncodeId) {
        _tprintf(_T("Error: Only HEVC Encoder supports Tiles[-tiles].\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    std::string::size_type pos = str.find('X');
    if (std::string::npos == pos) {
        _tprintf(_T("Format error: Please use like 4x4(%s)\n"), str.c_str());
        return MFX_ERR_UNSUPPORTED;
    }

    std::string row, col;
    row = str.substr(0, pos);
    col = str.substr(pos + 1, str.length());
    encInputParams.nTileRows = atoi(row.c_str());
    encInputParams.nTileColumns = atoi(col.c_str());
    if (0 == encInputParams.nTileRows || 0 == encInputParams.nTileColumns) {
        _tprintf(_T("Format error: Please use non-zero(%s)\n"), str.c_str());
        /*_tprintf(_T("Format error: Please use non-zero(%d:%d)\n"), InputParams.numTileRows,
         InputParams.numTileColumns);*/

        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus CmdProcessor::ParseCompositionParfile(const std::string& parFileName, CompositionParams& pParams) {
    mfxStatus sts = MFX_ERR_NONE;
    if (parFileName.empty())
        return MFX_ERR_UNKNOWN;

    string line;
    string key, value;
    mfxU8 nStreamInd = 0;
    mfxU8 firstStreamFound = 0;
    ifstream stream(parFileName);
    if (stream.fail()) {
        return MFX_ERR_UNKNOWN;
    }
    while (getline(stream, line)) {
        getPair(line, key, value);
        if (key.compare("width") == 0) {
            pParams.nWidth = (mfxU16) MSDK_ALIGN16(strtol(value.c_str(), NULL, 0));
        } else if (key.compare("height") == 0) {
            pParams.nHeight =
                    (MFX_PICSTRUCT_PROGRESSIVE == pParams.nPicStruct) ?
                            (mfxU16) MSDK_ALIGN16(strtol(value.c_str(), NULL, 0)) : (mfxU16) MSDK_ALIGN32(strtol(value.c_str(), NULL, 0));
        } else if (key.compare("cropx") == 0) {
            pParams.nCropX = (mfxU16) atoi(value.c_str());
        } else if (key.compare("cropy") == 0) {
            pParams.nCropY = (mfxU16) atoi(value.c_str());
        } else if (key.compare("cropw") == 0) {
            pParams.nCropW = (mfxU16) atoi(value.c_str());
        } else if (key.compare("croph") == 0) {
            pParams.nCropH = (mfxU16) atoi(value.c_str());
        } else if (key.compare("framerate") == 0) {
            pParams.dFrameRate = (mfxF64) atof(value.c_str());
        } else if (key.compare("fourcc") == 0) {
            const mfxU16 len_size = 5;
            TCHAR fourcc[len_size];
            for (mfxU16 i = 0; i < (value.size() > len_size ? len_size : value.size()); i++)
                fourcc[i] = value.at(i);
            fourcc[len_size - 1] = 0;
            std::string str(fourcc);
            pParams.nFourCC = Str2FourCC(str);
        } else if (key.compare("picstruct") == 0) {
            pParams.nPicStruct = (mfxU8) atoi(value.c_str());
        } else if (key.compare("dstx") == 0) {
            pParams.nDstX = (mfxU16) atoi(value.c_str());
        } else if (key.compare("dsty") == 0) {
            pParams.nDstY = (mfxU16) atoi(value.c_str());
        } else if (key.compare("dstw") == 0) {
            pParams.nDstW = (mfxU16) atoi(value.c_str());
        } else if (key.compare("dsth") == 0) {
            pParams.nDstH = (mfxU16) atoi(value.c_str());
        } else if (key.compare("AlphaEnable") == 0) {
            pParams.nGlobalAlphaEnable = (mfxU16) atoi(value.c_str());
        } else if ((key.compare("GlobalAlpha") == 0) && (pParams.nGlobalAlphaEnable != 0)) {
            pParams.nGlobalAlpha = (mfxU16) atoi(value.c_str());
        } else if (key.compare("PixelAlphaEnable") == 0) {
            pParams.nPixelAlphaEnable = (mfxU16) atoi(value.c_str());
        } else if (key.compare("LumaKeyEnable") == 0) {
            pParams.nLumaKeyEnable = (mfxU16) atoi(value.c_str());
        } else if ((key.compare("LumaKeyMin") == 0) && (pParams.nLumaKeyEnable != 0)) {
            pParams.nLumaKeyMin = (mfxU16) atoi(value.c_str());
        } else if ((key.compare("LumaKeyMax") == 0) && (pParams.nLumaKeyEnable != 0)) {
            pParams.nLumaKeyMax = (mfxU16) atoi(value.c_str());
        } else if ((key.compare("stream") == 0) && nStreamInd < 1) {
            if (firstStreamFound == 1) {
                nStreamInd++;
            } else {
                nStreamInd = 0;
                firstStreamFound = 1;
            }
            pParams.strStreamName = value;
        }
    }

    return sts;
}

mfxStatus CmdProcessor::VerifyAndCorrectInputParams(DecParams &decInputParams,
        EncParams &encInputParams, sVppParams &vppInputParams,
        paramMode mode) {
    if (0 == decInputParams.strSrcURI.length() && (mode == PIPELINE || mode == DECODE)) {
        PrintHelp(NULL, _T("Source not specified"));
        return MFX_ERR_UNSUPPORTED;
    }

    if ((BitstreamSink::E_NONE == encInputParams.outputType &&
                (mode == PIPELINE || mode == ENCODE_VPP)) &&
                (encInputParams.nEncodeId != MFX_CODEC_NULL)) {
        PrintHelp(NULL, _T("Destination not specified"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (BitstreamSink::E_NONE != encInputParams.outputType) {
        if (0 == encInputParams.strDstURI.length()) {
            PrintHelp(NULL, _T("Destination IP or Port not specified"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (vppInputParams.dFrameRate <= 0) {
        ///vppInputParams.dFrameRate = 30;
    }

#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    encInputParams.fei_ctrl.nQP = encInputParams.nQP;
#endif

    if (mode != DECODE) {  // Rate control parameters not relevant to Sink mode (input only session)
        if (encInputParams.nRateControlMethod == MFX_RATECONTROL_CQP) {
            if (encInputParams.nQP == UINT16_MAX) {
                PrintHelp(NULL, _T("QP must be specified for CQP (use -qp <value>)"));
                return MFX_ERR_UNSUPPORTED;
            } else if (encInputParams.nQP > 51) {
                PrintHelp(NULL, _T("Target QP is out of range (0 <= QP <= 51)"));
                return MFX_ERR_UNSUPPORTED;
            }
        }
    }

    // -mss check
    //fprintf(stderr, "------------hw = %d, id %d:%d\n", m_inputParams.bUseHW, encInputParams.nEncodeId, decInputParams.nDecodeId);
    if (m_inputParams.bUseHW == false &&
        (!(MFX_CODEC_AVC == encInputParams.nEncodeId ||
        MFX_CODEC_MPEG2 == encInputParams.nEncodeId) ||
        !(decInputParams.nDecodeId == MFX_CODEC_AVC ||
        decInputParams.nDecodeId == MFX_CODEC_MPEG2))) {
        PrintHelp(NULL, _T("Software codec supports only AVC and MPEG2!"));
        return MFX_ERR_UNSUPPORTED;
    }
    if (m_inputParams.bUseHW == false && (0 != encInputParams.nMaxSliceSize)) {
        PrintHelp(NULL, _T("nMaxSliceSize is supported only with hardware codec!\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    if (m_inputParams.bUseHW == false && encInputParams.bLABRC) {
        PrintHelp(NULL, _T("bLABRC is supported only with hardware codec!\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    if ((0 != encInputParams.nMaxSliceSize) && (0 != encInputParams.nSlices)) {
        PrintHelp(NULL, _T("-mss and -num_slice options are not compatible!\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    if ((0 != encInputParams.nMaxSliceSize) && (MFX_CODEC_AVC != encInputParams.nEncodeId)) {
        PrintHelp(NULL, _T("MaxSliceSize option is supported only with H.264 encoder!\n"));
        return MFX_ERR_UNSUPPORTED;
    }
    if ((0 != encInputParams.nLADepth) && (encInputParams.nLADepth < 10 || encInputParams.nLADepth > 100)) {
        if (1 != encInputParams.nLADepth || 0 == encInputParams.nMaxSliceSize) {
            PrintHelp(NULL, _T("Unsupported value of -lad parameter, must be in range [10, 100] or 1 in case of -mss option!\n"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

    // GopPicSize GopRefDist idrInterval
    if (MFX_CODEC_AVC == encInputParams.nEncodeId) {
        if (0 != encInputParams.nIdrInterval && 1 != encInputParams.nIdrInterval) {
            PrintHelp(NULL, _T("idrInterval with H.264 encoder, only 0 and 1 avalid!\n"));
            return MFX_ERR_UNSUPPORTED;
        }
    }
    if (0 != encInputParams.nIdrInterval) {
        if (0 == encInputParams.nGopPicSize || 0 == encInputParams.nGopRefDist) {
            PrintHelp(NULL, _T("idrInterval invalid, gopPicSize or gopRefDist is zero!\n"));
            return MFX_ERR_UNSUPPORTED;
        }
    }

#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    fprintf(stderr, "FEI elements: %d, enc: %d, pak: %d, encpak: %d, encode: %d\n", encInputParams.bPreEnc, encInputParams.bENC, encInputParams.bPAK, encInputParams.bENCPAK, encInputParams.bEncode);
    bool allowedFEIpipeline = ((encInputParams.bPreEnc && !encInputParams.bENC && !encInputParams.bPAK && !encInputParams.bENCPAK  && !encInputParams.bEncode) ||
                              (!encInputParams.bPreEnc &&  encInputParams.bENC && !encInputParams.bPAK && !encInputParams.bENCPAK  && !encInputParams.bEncode) ||
                              (!encInputParams.bPreEnc && !encInputParams.bENC &&  encInputParams.bPAK && !encInputParams.bENCPAK  && !encInputParams.bEncode) ||
                              (!encInputParams.bPreEnc && !encInputParams.bENC && !encInputParams.bPAK && !encInputParams.bENCPAK  &&  encInputParams.bEncode) ||
                              (!encInputParams.bPreEnc && !encInputParams.bENC && !encInputParams.bPAK &&  encInputParams.bENCPAK  && !encInputParams.bEncode) ||
                              (!encInputParams.bPreEnc && encInputParams.bENC && encInputParams.bPAK &&  !encInputParams.bENCPAK  && !encInputParams.bEncode) ||
                              (!encInputParams.bPreEnc &&  !encInputParams.bENC && !encInputParams.bPAK && !encInputParams.bENCPAK  && !encInputParams.bEncode) ||
                              (encInputParams.bPreEnc && !encInputParams.bENC && !encInputParams.bPAK && !encInputParams.bENCPAK  &&  encInputParams.bEncode) ||
                              (encInputParams.bPreEnc && encInputParams.bENC && encInputParams.bPAK && !encInputParams.bENCPAK  &&  !encInputParams.bEncode) ||
                              (encInputParams.bPreEnc && !encInputParams.bENC && !encInputParams.bPAK &&  encInputParams.bENCPAK  && !encInputParams.bEncode));

    if (!allowedFEIpipeline) {
        PrintHelp(NULL, _T("ERROR: Unsupported pipeline!\n"));
        printf("Supported pipelines are:\n");
        printf("PREENC\n");
        printf("ENC\n");
        printf("PAK\n");
        printf("ENCODE\n");
        printf("ENC + PAK (ENCPAK)\n");
        //msdk_printf(MSDK_STRING("PREENC + ENC\n"));
        printf("PREENC + ENCODE\n");
        printf("PREENC + ENC + PAK (PREENC + ENCPAK)\n");
        return MFX_ERR_UNSUPPORTED;
    }

    if(encInputParams.fei_ctrl.nMVPredictors_Pl0 > 4 || encInputParams.fei_ctrl.nMVPredictors_Bl0 > 4 || encInputParams.fei_ctrl.nMVPredictors_Bl1 > 4){
        PrintHelp(NULL, _T("ERROR: Unsupported value number of MV predictors (4 is maximum)!\n"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (encInputParams.bPreEnc || encInputParams.bEncode) {
        if (encInputParams.nRateControlMethod != MFX_RATECONTROL_CQP) {
            PrintHelp(NULL, _T("FEI ENCODE can only work with CQP mode\n"));
            return MFX_ERR_UNSUPPORTED;
        }

        if (MFX_CODEC_AVC != encInputParams.nEncodeId && MFX_CODEC_HEVC != encInputParams.nEncodeId) {
            PrintHelp(NULL, _T("FEI option is supported only with H.264 encoder!\n"));
            return MFX_ERR_UNSUPPORTED;
        }

        if (encInputParams.fei_ctrl.nDSstrength == 1) {
            encInputParams.fei_ctrl.nDSstrength = 0;
        }
        if ((encInputParams.fei_ctrl.nSearchWindow == 0) && (encInputParams.fei_ctrl.nRefHeight == 0
                    || encInputParams.fei_ctrl.nRefWidth == 0 || encInputParams.fei_ctrl.nRefHeight % 4 != 0
                    || encInputParams.fei_ctrl.nRefWidth % 4 != 0
                    || encInputParams.fei_ctrl.nRefHeight * encInputParams.fei_ctrl.nRefWidth > 2048))
        {
            PrintHelp(NULL, _T("ERROR: Unsupported value of search window size. -ref_window_h, -ref_window_w parameters, must be multiple of 4!\n"
                        "For bi-prediction window w*h must less than 1024 and less 2048 otherwise."));
            return MFX_ERR_UNSUPPORTED;
        }

        if (encInputParams.fei_ctrl.nSearchWindow && (encInputParams.fei_ctrl.nAdaptiveSearch || encInputParams.fei_ctrl.nSearchPath
            || encInputParams.fei_ctrl.nLenSP || encInputParams.fei_ctrl.nRefWidth || encInputParams.fei_ctrl.nRefHeight))
        {
            encInputParams.fei_ctrl.nLenSP = encInputParams.fei_ctrl.nSearchPath = encInputParams.fei_ctrl.nRefWidth =
                encInputParams.fei_ctrl.nRefHeight = encInputParams.fei_ctrl.nAdaptiveSearch = 0;
        }
        else if (!encInputParams.fei_ctrl.nSearchWindow && (!encInputParams.fei_ctrl.nLenSP || !encInputParams.fei_ctrl.nRefWidth || !encInputParams.fei_ctrl.nRefHeight))
        {
            if (!encInputParams.fei_ctrl.nLenSP)
                encInputParams.fei_ctrl.nLenSP = 57;
            if (!encInputParams.fei_ctrl.nRefWidth)
                encInputParams.fei_ctrl.nRefWidth = 32;
            if (!encInputParams.fei_ctrl.nRefHeight)
                encInputParams.fei_ctrl.nRefHeight = 32;
        }
    }

    // Check references lists limits
    if (encInputParams.nEncodeId == MFX_CODEC_HEVC) {
        if (encInputParams.fei_ctrl.nSearchPath > 2) {
            PrintHelp(NULL, _T("Invalid SearchPath value"));
            return MFX_ERR_UNSUPPORTED;
        }
        if (encInputParams.bPreEnc && !encInputParams.mvinFile.empty()) {
            PrintHelp(NULL, _T("MV predictors from both PreENC and input file (mvpin) are not supported in one pipeline"));
            return MFX_ERR_UNSUPPORTED;
        }
        if (encInputParams.fei_ctrl.nNumMvPredictors[0] > 4) {
            PrintHelp(NULL, _T("Unsupported NumMvPredictorsL0 value (must be in range [0,4])"));
            return MFX_ERR_UNSUPPORTED;
        }
        if (encInputParams.fei_ctrl.nNumMvPredictors[1] > 4) {
            PrintHelp(NULL, _T("Unsupported NumMvPredictorsL1 value (must be in range [0,4])"));
            return MFX_ERR_UNSUPPORTED;
        }
        if (!(encInputParams.fei_ctrl.nNumFramePartitions == 1 ||
            encInputParams.fei_ctrl.nNumFramePartitions == 2 ||
            encInputParams.fei_ctrl.nNumFramePartitions == 4 ||
            encInputParams.fei_ctrl.nNumFramePartitions == 8 ||
            encInputParams.fei_ctrl.nNumFramePartitions == 16  )) {
            PrintHelp(NULL, _T("Unsupported NumFramePartitions value (must be 1, 2, 4, 8 or 16)"));
            return MFX_ERR_UNSUPPORTED;
        }
        if (!encInputParams.fei_ctrl.nRefActiveP)
            encInputParams.fei_ctrl.nRefActiveP = 3;
        if (!encInputParams.fei_ctrl.nRefActiveBL0)
            encInputParams.fei_ctrl.nRefActiveBL0 = 3;
        if (!encInputParams.fei_ctrl.nRefActiveBL1)
            encInputParams.fei_ctrl.nRefActiveBL1 = 1;

        if (encInputParams.fei_ctrl.nRefActiveP > 3
            || encInputParams.fei_ctrl.nRefActiveBL0 > 3
            || encInputParams.fei_ctrl.nRefActiveBL1 > 1) {
            PrintHelp(NULL, _T("HEVC FEI: Unsupported NumRefActiveP/BL0/BL1 value!(must be in range P/BL0(0,3), BL1(0,1))\n"));
            return MFX_ERR_UNSUPPORTED;
        }

        if (encInputParams.nGopRefDist < 2) {
            encInputParams.fei_ctrl.nRefActiveBL0 = 0;
            encInputParams.fei_ctrl.nRefActiveBL1 = 0;
        }

        // if (!encInputParams.bPreEnc && encInputParams.mvinFile.empty()) {
        //     encInputParams.fei_ctrl.nNumMvPredictors[0] = 0;
        //     encInputParams.fei_ctrl.nNumMvPredictors[1] = 0;
        //     encInputParams.fei_ctrl.nMVPredictor = 0;
        // }
    } else {
        mfxU16 num_ref_limit = encInputParams.bPAK ? MaxNumActiveRefPAK : MaxNumActiveRefP;

        if (encInputParams.fei_ctrl.nRefActiveP == 0)
        {
            encInputParams.fei_ctrl.nRefActiveP = num_ref_limit;
        }
        else if (encInputParams.fei_ctrl.nRefActiveP > num_ref_limit)
        {
            encInputParams.fei_ctrl.nRefActiveP = num_ref_limit;
            _tprintf(_T("\nWARNING: Unsupported number of P frame references, adjusted to maximum (%d)\n"), num_ref_limit);
        }

        num_ref_limit = encInputParams.bPAK ? MaxNumActiveRefPAK : MaxNumActiveRefBL0;

        if (encInputParams.fei_ctrl.nRefActiveBL0 == 0)
        {
            encInputParams.fei_ctrl.nRefActiveBL0 = num_ref_limit;
        }
        else if (encInputParams.fei_ctrl.nRefActiveBL0 > num_ref_limit)
        {
            encInputParams.fei_ctrl.nRefActiveBL0 = num_ref_limit;
            _tprintf(_T("\nWARNING: Unsupported number of B frame backward references, adjusted to maximum (%d)\n"), num_ref_limit);
        }
    }


     /* The following three settings affects partitions: Transform8x8ModeFlag, IntraPartMask, CodecProfile.
       Code below adjusts these parameters to avoid contradictions.

       If Transform8x8ModeFlag is set to true it has priority, else Profile settings have top priority.
    */

    bool is_8x8part_present_profile = !((encInputParams.nCodecProfile & 0xff) == MFX_PROFILE_AVC_MAIN
                                     || (encInputParams.nCodecProfile & 0xff) == MFX_PROFILE_AVC_BASELINE);
    bool is_8x8part_present_custom  = !(encInputParams.fei_ctrl.nIntraPartMask & 0x06);

    if (is_8x8part_present_profile != is_8x8part_present_custom
                || is_8x8part_present_custom != encInputParams.fei_ctrl.bTransform8x8ModeFlag)
    {
        bool part_to_set = encInputParams.fei_ctrl.bTransform8x8ModeFlag || is_8x8part_present_profile;

        /*
        IntraPartMask description from manual
            This value specifies what block and sub - block partitions are enabled for intra MBs.
            0x01 - 16x16 is disabled
            0x02 - 8x8   is disabled
            0x04 - 4x4   is disabled
        */
        if (!part_to_set)
            encInputParams.fei_ctrl.nIntraPartMask |= 0x06;
        else
            encInputParams.fei_ctrl.nIntraPartMask ^= encInputParams.fei_ctrl.nIntraPartMask & 0x06;

        encInputParams.fei_ctrl.bTransform8x8ModeFlag = part_to_set;

        if (part_to_set && !is_8x8part_present_profile)
        {
            encInputParams.nCodecProfile = MFX_PROFILE_AVC_HIGH;
        }
    }
    if(!encInputParams.fei_ctrl.bTransform8x8ModeFlag &&
            ((encInputParams.fei_ctrl.nIntraPartMask & 0x04)
            ||(encInputParams.fei_ctrl.nIntraPartMask & 0x05)
            || (encInputParams.fei_ctrl.nIntraPartMask & 0x07)))
        encInputParams.fei_ctrl.nIntraPartMask = 0x06;

#endif

    return MFX_ERR_NONE;

}

mfxStatus CmdProcessor::VerifyCrossSessionsOptions() {
//    bool bIsSinkPresence = false;
//    bool bIsSourcePresence = false;
//    bool bIsHeterSessionJoin = false;
//    bool bIsFirstInTopology = true;
//
//    for (InputParamsList::iterator it = m_inputParamsList.begin(); it != m_inputParamsList.end(); ++it)
//    {
//        if (Source == it->eMode) {
//            // topology definition
//            if (!bIsSinkPresence) {
//                PrintHelp(NULL, _T("Error in par file. Decode source session MUST be declared before encode sinks \n"));
//                return MFX_ERR_UNSUPPORTED;
//            }
//            bIsSourcePresence = true;
//
//            if (bIsFirstInTopology) {
//                if (it->bIsJoin)
//                    bIsHeterSessionJoin = true;
//                else
//                    bIsHeterSessionJoin = false;
//            }
//            else {
//                if (it->bIsJoin && !bIsHeterSessionJoin) {
//                    PrintHelp(NULL, _T("Error in par file. ALL heterogeneous sessions MUST be joined \n"));
//                    return MFX_ERR_UNSUPPORTED;
//                }
//                if (!it->bIsJoin && bIsHeterSessionJoin) {
//                    PrintHelp(NULL, _T("Error in par file. ALL heterogeneous sessions MUST be not joined \n"));
//                    return MFX_ERR_UNSUPPORTED;
//                }
//            }
//
//            if (bIsFirstInTopology)
//                bIsFirstInTopology = false;
//
//        }
//        else if (Sink == it->eMode) {
//            if (bIsSinkPresence) {
//                PrintHelp(NULL, _T("Error in par file. Only one source can be used"));
//                return MFX_ERR_UNSUPPORTED;
//            }
//            bIsSinkPresence = true;
//
//            if (bIsFirstInTopology) {
//                if (it->bIsJoin)
//                    bIsHeterSessionJoin = true;
//                else
//                    bIsHeterSessionJoin = false;
//            }
//            else {
//                if (it->bIsJoin && !bIsHeterSessionJoin) {
//                    PrintHelp(NULL, _T("Error in par file. ALL heterogeneous sessions MUST be joined \n"));
//                    return MFX_ERR_UNSUPPORTED;
//                }
//                if (!it->bIsJoin && bIsHeterSessionJoin) {
//                    PrintHelp(NULL, _T("Error in par file. ALL heterogeneous sessions MUST be not joined \n"));
//                    return MFX_ERR_UNSUPPORTED;
//                }
//            }
//
//            if (bIsFirstInTopology)
//                bIsFirstInTopology = false;
//        }
//    }
//
//    if (bIsSinkPresence && !bIsSourcePresence) {
//        PrintHelp(NULL, _T("Error in par file. Sink should be defined"));
//        return MFX_ERR_UNSUPPORTED;
//    }
    return MFX_ERR_NONE;

} // mfxStatus CmdProcessor::VerifyCrossSessionsOptions()

void CmdProcessor::getPair(std::string line, std::string &key, std::string &value) {
    std::istringstream iss(line);
    getline(iss, key, '=');
    getline(iss, value, '=');

    trim(key);
    trim(value);
}

