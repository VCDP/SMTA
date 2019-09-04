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
#ifndef __SAMPLE_FEI_PRED_REPACKING_H__
#define __SAMPLE_FEI_PRED_REPACKING_H__

#include "encoding_task.h"
#ifdef MSDK_AVC_FEI

/*  PreEnc outputs 16 MVs per-MB, one of the way to construct predictor from this array
    is to extract median for x and y component

    preencMB - MB of motion vectors buffer
    tmpBuf   - temporary array, for inplace sorting
    xy       - indicates coordinate being processed [0-1] (0 - x coordinate, 1 - y coordinate)
    L0L1     - indicates reference list being processed [0-1] (0 - L0-list, 1 - L1-list)

    returned value - median of 16 element array
*/
inline mfxI16 get16Median(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB* preencMB, mfxI16* tmpBuf, int xy, int L0L1)
{
    switch (xy){
    case 0:
        for (int k = 0; k < 16; k++){
            tmpBuf[k] = preencMB->MV[k][L0L1].x;
        }
        break;
    case 1:
        for (int k = 0; k < 16; k++){
            tmpBuf[k] = preencMB->MV[k][L0L1].y;
        }
        break;
    default:
        return 0;
    }

    std::sort(tmpBuf, tmpBuf + 16);
    return (tmpBuf[7] + tmpBuf[8]) / 2;
}

/* repackPreenc2Enc passes only one predictor (median of provided preenc MVs) because we dont have distortions to choose 4 best possible */

static mfxStatus repackPreenc2Enc(mfxExtFeiPreEncMV::mfxExtFeiPreEncMVMB *preencMVoutMB, mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB *EncMVPredMB, mfxU32 NumMB, mfxI16 *tmpBuf)
{
    MSDK_ZERO_ARRAY(EncMVPredMB, NumMB);
    for (mfxU32 i = 0; i<NumMB; i++)
    {
        //only one ref is used for now
        for (int j = 0; j < 4; j++){
            EncMVPredMB[i].RefIdx[j].RefL0 = 0;
            EncMVPredMB[i].RefIdx[j].RefL1 = 0;
        }

        //use only first subblock component of MV
        for (int j = 0; j < 2; j++){
            EncMVPredMB[i].MV[0][j].x = get16Median(preencMVoutMB + i, tmpBuf, 0, j);
            EncMVPredMB[i].MV[0][j].y = get16Median(preencMVoutMB + i, tmpBuf, 1, j);
        }
    } // for(mfxU32 i=0; i<NumMBAlloc; i++)

    return MFX_ERR_NONE;
}
#endif // MSDK_AVC_FEI
#endif // __SAMPLE_FEI_PRED_REPACKING_H__
