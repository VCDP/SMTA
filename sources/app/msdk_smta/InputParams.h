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

#ifndef __INPUTPARAMS_H__
#define __INPUTPARAMS_H__

#include <string>
#include <vector>

#include "sample_defs.h"
#include "BitstreamSink.h"
#include "mfxvideo++.h"
#include "video/msdk_xcoder.h"

#define MAX_PIPELINE (100)

namespace TranscodingSample
{

    // specific decode parameters
    struct DecParams {
        DecParams();
        mfxU32          nDecodeId;      // type of input coded video
        std::string     strSrcURI;      // source bitstream URI
        bool            bIsLoopMode;    // If enabled, we loop on source data ... and keep transcoding "forever"
        mfxU16          nLoopNum;       // loop numbers
        bool            bInputByFPS;    // input local data by FPS
    };

    // specific encode parameters
    struct EncParams {
        EncParams();
        mfxU32            nEncodeId;            // type of output coded video
        std::string       strDstURI;            // destination bitstream URI
        BitstreamSink::OutputType   outputType;
        mfxU32            nBitRate;
        mfxU16            nSlices;              // number of slices for encoder initialization
        bool              bMBBRC;               // use macroblock-level bitrate control algorithm
        mfxU32            nMaxSliceSize;        //Max Slice Size, for AVC only
        mfxU16            nQP;
        bool              bLABRC;               // use look ahead bitrate control algorithm
        mfxU16            nLADepth;             // depth of the look ahead bitrate control algorithm
        mfxU16            nRateControlMethod;
        mfxU16            nTargetUsage;
        mfxU16            nCodecProfile;
        mfxU16            nCodecLevel;
        mfxU16            bRefType;
        mfxU16            pRefType;
        mfxU16            nGPB;
        // GOP parameters
        mfxU16            nGopOptFlag;          // Additional GOP options (CLOSED, STRICT)
        mfxU16            nGopPicSize;          // # of pictures within the current GOP
        mfxU16            nGopRefDist;          // Distance between I- or P- key frames
        mfxU16            nRefNum;
        mfxU16            nIdrInterval;         // IDR-frame interval in terms of I-frames
        // HEVC Encoder
        mfxU16            nTileRows;
        mfxU16            nTileColumns;
        // get libxcode encode handle
        void*             encHandle;
        bool              bDump;   // If enabled, dump in/out data as configured at compile time
        bool              bUseHW;
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
        // Store ROI list for some frames
        std::map<int, mfxExtEncoderROI> extRoiData;
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#if (MFX_VERSION >= 1025)
        mfxU16 nMFEFrames;
        mfxU16 nMFEMode;
        mfxU32 nMFETimeout;
#endif
        mfxU16            nRepartitionCheckMode; //Controls AVC encoder attempts to predict from small partitions
#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
        bool              bPreEnc;
        bool              bEncode;
        bool              bPAK;
        bool              bENC;
        bool              bENCPAK;
        bool              bQualityRepack;
        std::string       mvinFile;
        std::string       mvoutFile;
        std::string       mbcodeoutFile;
        std::string       weightFile;
        FEICtrlParams     fei_ctrl;
#endif
    };

    // specific composition parameters
    struct CompositionParams {
        CompositionParams();
        std::string      strStreamName;
        mfxU32      nDstX;
        mfxU32      nDstY;
        mfxU32      nDstW;
        mfxU32      nDstH;

        mfxU16      nLumaKeyEnable;
        mfxU16      nLumaKeyMin;
        mfxU16      nLumaKeyMax;

        mfxU16      nGlobalAlphaEnable;
        mfxU16      nGlobalAlpha;

        mfxU16      nPixelAlphaEnable;
        mfxU32      nStreamFourcc;

        mfxU16      nWidth;
        mfxU16      nHeight;

        mfxU16      nCropX;
        mfxU16      nCropY;
        mfxU16      nCropW;
        mfxU16      nCropH;

        mfxU32      nFourCC;
        mfxU8       nPicStruct;
        mfxF64      dFrameRate;
    };

    // specific VPP parameters
    struct sVppParams {
        sVppParams();
        mfxF64              dFrameRate;
        bool                bDenoiseEnabled;
        mfxU16              nDenoiseLevel;          // De-noise level
        bool                bEnableDeinterlacing;
        mfxU16              nDstWidth;              // destination picture nWidth, specified if resizing required
        mfxU16              nDstHeight;             // destination picture nHeight, specified if resizing required
        mfxU16              nPicStruct;
        bool                bComposition;
        CompositionParams   compositionParams;
        mfxU16              nSuggestSurface;
        // mfxU16              nLADepth;
        // bool                bLABRC;
    };

    // input parameters struct
    struct InputParams
    {
        InputParams();
        bool                        bStatistics; // Measure FPS, transcode latency and write to file
        bool                        bIsJoin;
        bool                        bUseHW;
        std::string                 strSessionName;  // Tag for the session, just used for easier troubleshooting
        std::string                 strWorkID;  // For Reply performance info to server
        vector<DecParams>           decParamsList;
        vector<EncParams>           encParamsList;
        vector<sVppParams>          vppParamsList;
    };


}
#endif /* __INPUTPARAMS_H__ */
