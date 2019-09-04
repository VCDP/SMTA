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

#include "h265_fei_utils.h"

#ifdef MSDK_HEVC_FEI
SurfacesPool::SurfacesPool(MFXFrameAllocator* allocator)
    : m_pAllocator(allocator)
{
    MSDK_ZERO_MEMORY(m_response);
}

SurfacesPool::~SurfacesPool()
{
    if (m_pAllocator)
    {
        m_pAllocator->Free(m_pAllocator->pthis, &m_response);
        m_pAllocator = NULL;
    }
}

void SurfacesPool::SetAllocator(MFXFrameAllocator* allocator)
{
    m_pAllocator = allocator;
}

mfxStatus SurfacesPool::AllocSurfaces(mfxFrameAllocRequest& request)
{
    MSDK_CHECK_POINTER(m_pAllocator, MFX_ERR_NULL_PTR);

    // alloc frames
    mfxStatus sts = m_pAllocator->Alloc(m_pAllocator->pthis, &request, &m_response);
    if (m_pAllocator->pthis == 0) {
        fprintf(stderr, "alloc pthis is 0\n");
    }
    if (sts < MFX_ERR_NONE  ) {
        fprintf(stderr, "m_pAllocator->Alloc failed");
        return sts;
    }

    mfxFrameSurface1 surf;
    MSDK_ZERO_MEMORY(surf);

    memcpy(&surf.Info, &request.Info, sizeof(mfxFrameInfo));

    m_pool.reserve(m_response.NumFrameActual);
    for (mfxU16 i = 0; i < m_response.NumFrameActual; i++)
    {
        if (m_response.mids)
        {
            surf.Data.MemId = m_response.mids[i];
        }
        m_pool.push_back(surf);
    }

    return MFX_ERR_NONE;
}

mfxFrameSurface1* SurfacesPool::GetFreeSurface()
{
    if (m_pool.empty()) // seems AllocSurfaces wasn't called
        return NULL;

    mfxU16 idx = GetFreeSurfaceIndex(m_pool.data(), m_pool.size());

    return (idx != MSDK_INVALID_SURF_IDX) ? &m_pool[idx] : NULL;
}

mfxStatus SurfacesPool::LockSurface(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    if (m_pAllocator)
    {
        sts = m_pAllocator->Lock(m_pAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        //MSDK_CHECK_STATUS(sts, "m_pAllocator->Lock failed");
        if (sts < 0)
            fprintf(stderr, "m_pAllocator->Lock failed");
    }

    return sts;
}

mfxStatus SurfacesPool::UnlockSurface(mfxFrameSurface1* pSurf)
{
    mfxStatus sts = MFX_ERR_NOT_INITIALIZED;

    if (m_pAllocator)
    {
        sts = m_pAllocator->Unlock(m_pAllocator->pthis, pSurf->Data.MemId, &(pSurf->Data));
        //MSDK_CHECK_STATUS(sts, "m_pAllocator->Unlock failed");
                if (sts < 0)
            fprintf(stderr, "m_pAllocator->Unlock failed");
    }

    return sts;
}

#endif // MSDK_HEVC_FEI