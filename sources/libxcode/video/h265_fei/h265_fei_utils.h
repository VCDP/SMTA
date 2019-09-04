/******************************************************************************\
Copyright (c) 2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifdef MSDK_HEVC_FEI
#ifndef __FEI_UTILS_H__
#define __FEI_UTILS_H__
#include "sample_hevc_fei_defs.h"

class SurfacesPool
{
public:
    SurfacesPool(MFXFrameAllocator* allocator = NULL);
    ~SurfacesPool();

    mfxStatus AllocSurfaces(mfxFrameAllocRequest& request);
    void SetAllocator(MFXFrameAllocator* allocator);
    MFXFrameAllocator * GetAllocator(void)
    {
        return m_pAllocator;
    }
    mfxFrameSurface1* GetFreeSurface();
    mfxStatus LockSurface(mfxFrameSurface1* pSurf);
    mfxStatus UnlockSurface(mfxFrameSurface1* pSurf);

private:
    MFXFrameAllocator* m_pAllocator;
    std::vector<mfxFrameSurface1> m_pool;
    mfxFrameAllocResponse m_response;

private:
    DISALLOW_COPY_AND_ASSIGN(SurfacesPool);
};


class HEVCEncodeParamsChecker
{
public:
    HEVCEncodeParamsChecker(MFXVideoSession*  session):
    m_session(session)
    {
        mfxStatus sts;

        mfxPluginUID pluginGuid = MFX_PLUGINID_HEVC_FEI_ENCODE;

        sts = MFXVideoUSER_Load(*m_session, &pluginGuid, 1);
        if (sts != MFX_ERR_NONE) {
            fprintf(stderr, "Failed to load plugin from GUID, sts=%d", sts);
        }

        m_encode.reset(new MFXVideoENCODE(*m_session));
    }

    ~HEVCEncodeParamsChecker()
    {
        mfxStatus sts;
        mfxPluginUID pluginGuid = MFX_PLUGINID_HEVC_FEI_ENCODE;
        if (m_session) {
            sts = MFXVideoUSER_UnLoad(*m_session, &pluginGuid);
            if (sts != MFX_ERR_NONE) {
                fprintf(stderr, "Failed to load plugin from GUID, sts=%d", sts);
            }
        }
    }

    mfxStatus Query(MfxVideoParamsWrapper & pars) // in/out
    {
        mfxStatus sts = m_encode->Query(&pars, &pars);
        if (sts != MFX_ERR_NONE && sts != MFX_WRN_INCOMPATIBLE_VIDEO_PARAM) {
            fprintf(stderr, "HEVCEncodeParamsChecker Query failed sts = %d\n", sts);
        }

        return sts;
    }

private:
    MFXVideoSession*  m_session;
    std::auto_ptr<MFXVideoENCODE>   m_encode;

private:
    DISALLOW_COPY_AND_ASSIGN(HEVCEncodeParamsChecker);
};
#endif // #define __FEI_UTILS_H__
#endif // MSDK_HEVC_FEI
