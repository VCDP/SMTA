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

#include "element_common.h"
#include "os/XC_Time.h"
#include <algorithm>

unsigned int GetSysTimeInUs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

mfxU16 CalDefBitrate(mfxU32 nCodecId, mfxU32 nTargetUsage, mfxU32 nWidth, mfxU32 nHeight, mfxF64 dFrameRate)
{
    PartiallyLinearBitrate fnc;
    mfxF64 bitrate = 0;

    switch (nCodecId)
    {
        case MFX_CODEC_AVC :
        case MFX_CODEC_HEVC :
            fnc.AddPair(0, 0);
            fnc.AddPair(25344, 225);
            fnc.AddPair(101376, 1000);
            fnc.AddPair(414720, 4000);
            fnc.AddPair(2058240, 5000);
            break;
        case MFX_CODEC_MPEG2:
            fnc.AddPair(0, 0);
            fnc.AddPair(414720, 12000);
            break;
        default:
            fnc.AddPair(0, 0);
            fnc.AddPair(414720, 12000);
            break;
    }

    mfxF64 at = nWidth * nHeight * dFrameRate / 30.0;

    if (!at)
    {
        return 0;
    }

    switch (nTargetUsage)
    {
        case MFX_TARGETUSAGE_BEST_QUALITY :
            bitrate = (&fnc)->at(at);
            break;
        case MFX_TARGETUSAGE_BEST_SPEED :
            bitrate = (&fnc)->at(at) * 0.5;
            break;
        case MFX_TARGETUSAGE_BALANCED :
        default:
            bitrate = (&fnc)->at(at) * 0.75;
            break;
    }

    return (mfxU16)bitrate;
}

static mfxU16 GetMaxNumRefFrame(mfxU16 level, mfxU16 width, mfxU16 height)
{
    mfxU32 maxDpbSize = 0;
    if (level == MFX_LEVEL_UNKNOWN)
        level = MFX_LEVEL_AVC_52;

    switch (level)
    {
    case MFX_LEVEL_AVC_1:  maxDpbSize = 152064;   break;
    case MFX_LEVEL_AVC_1b: maxDpbSize = 152064;   break;
    case MFX_LEVEL_AVC_11: maxDpbSize = 345600;   break;
    case MFX_LEVEL_AVC_12: maxDpbSize = 912384;   break;
    case MFX_LEVEL_AVC_13: maxDpbSize = 912384;   break;
    case MFX_LEVEL_AVC_2:  maxDpbSize = 912384;   break;
    case MFX_LEVEL_AVC_21: maxDpbSize = 1824768;  break;
    case MFX_LEVEL_AVC_22: maxDpbSize = 3110400;  break;
    case MFX_LEVEL_AVC_3:  maxDpbSize = 3110400;  break;
    case MFX_LEVEL_AVC_31: maxDpbSize = 6912000;  break;
    case MFX_LEVEL_AVC_32: maxDpbSize = 7864320;  break;
    case MFX_LEVEL_AVC_4:  maxDpbSize = 12582912; break;
    case MFX_LEVEL_AVC_41: maxDpbSize = 12582912; break;
    case MFX_LEVEL_AVC_42: maxDpbSize = 13369344; break;
    case MFX_LEVEL_AVC_5:  maxDpbSize = 42393600; break;
    case MFX_LEVEL_AVC_51: maxDpbSize = 70778880; break;
    case MFX_LEVEL_AVC_52: maxDpbSize = 70778880; break;
    }

    mfxU32 frameSize = width * height * 3 / 2;
    return mfxU16((std::max)(mfxU32(1), (std::min)(maxDpbSize / frameSize, mfxU32(16))));
}

mfxU16 GetMinNumRefFrameForPyramid(mfxU16 GopRefDist)
{
    mfxU16 refIP = (GopRefDist > 1 ? 2 : 1);
    mfxU16 refB = GopRefDist ? (GopRefDist - 1) / 2 : 0;

    for (mfxU16 x = refB; x > 2;)
    {
        x = (x - 1) / 2;
        refB -= x;
    }

    return refIP + refB;
}

bool IsPowerOf2(mfxU32 n)
{
    return (n & (n - 1)) == 0;
}

mfxU16 AdjustBrefType(mfxU16 init_bref, mfxVideoParam const & par)
{
    const mfxU16 nfxMaxByLevel    = GetMaxNumRefFrame(par.mfx.CodecLevel, par.mfx.FrameInfo.Width, par.mfx.FrameInfo.Height);
    const mfxU16 nfxMinForPyramid = GetMinNumRefFrameForPyramid(par.mfx.GopRefDist);

    if (nfxMinForPyramid > nfxMaxByLevel)
    {
        // max dpb size is not enougn for pyramid
        return MFX_B_REF_OFF;
    }

    if (init_bref == MFX_B_REF_PYRAMID && par.mfx.GopRefDist < 3)
    {
        return MFX_B_REF_OFF;
    }

    if (init_bref == MFX_B_REF_UNKNOWN)
    {
        if (par.mfx.GopRefDist >= 4 &&
            IsPowerOf2(par.mfx.GopRefDist) &&
            par.mfx.GopPicSize % par.mfx.GopRefDist == 0 &&
            par.mfx.NumRefFrame >= nfxMinForPyramid)
        {
            return par.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? MFX_B_REF_PYRAMID : MFX_B_REF_OFF;
        }
        else
        {
            return MFX_B_REF_OFF;
        }
    }

    return init_bref;
}
