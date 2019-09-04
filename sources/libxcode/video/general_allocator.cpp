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

#include "general_allocator.h"

#include <stdarg.h>
#if defined (PLATFORM_OS_WINDOWS)
#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#include "d3d_device.h"
#elif defined (PLATFORM_OS_LINUX)
#include "vaapi_allocator.h"
#endif
#include "sysmem_allocator.h"

// Wrapper on standard allocator for concurrent allocation of
// D3D and system surfaces
GeneralAllocator::GeneralAllocator()
{
};
GeneralAllocator::~GeneralAllocator()
{
};

#if defined (PLATFORM_OS_WINDOWS)
mfxStatus GeneralAllocator::Init(mfxAllocatorParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    D3DAllocatorParams *d3dAllocParams = dynamic_cast<D3DAllocatorParams*>(pParams);
if (d3dAllocParams)
        m_D3DAllocator.reset(new D3DFrameAllocator);
#if MFX_D3D11_SUPPORT
    D3D11AllocatorParams *d3d11AllocParams = dynamic_cast<D3D11AllocatorParams*>(pParams);
if (d3d11AllocParams)
        m_D3DAllocator.reset(new D3D11FrameAllocator);
#endif
    if (m_D3DAllocator.get())
    {
        sts = m_D3DAllocator.get()->Init(pParams);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    m_SYSAllocator.reset(new SysMemFrameAllocator);
    sts = m_SYSAllocator.get()->Init(0);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    return sts;
}
#elif defined (PLATFORM_OS_LINUX)
// Create video memory allocator
mfxStatus GeneralAllocator::Init(VADisplay *dpy)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_D3DAllocator.reset(new vaapiFrameAllocator);
    sts = m_D3DAllocator.get()->Init(dpy);

    return sts;
}

// Create system memory allocator
mfxStatus GeneralAllocator::Init(mfxAllocatorParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_SYSAllocator.reset(new SysMemFrameAllocator);
    sts = m_SYSAllocator.get()->Init(pParams);

    return sts;
}
#endif

mfxStatus GeneralAllocator::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_D3DAllocator.get())
        sts = m_D3DAllocator.get()->Close();

    if (m_SYSAllocator.get())
        sts = m_SYSAllocator.get()->Close();

   return sts;
}

mfxStatus GeneralAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    if (m_D3DAllocator.get())
        return m_D3DAllocator.get()->Lock(m_D3DAllocator.get(), mid, ptr);
    else
        return m_SYSAllocator.get()->Lock(m_SYSAllocator.get(),mid, ptr);

}
mfxStatus GeneralAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    if (m_D3DAllocator.get())
        return m_D3DAllocator.get()->Unlock(m_D3DAllocator.get(), mid, ptr);
    else
        return m_SYSAllocator.get()->Unlock(m_SYSAllocator.get(),mid, ptr);

}

mfxStatus GeneralAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle)
{
    if (m_D3DAllocator.get())
        return m_D3DAllocator.get()->GetHDL(m_D3DAllocator.get(), mid, handle);
    else
        return m_SYSAllocator.get()->GetHDL(m_SYSAllocator.get(), mid, handle);

}

mfxStatus GeneralAllocator::ReleaseResponse(mfxFrameAllocResponse *response)
{
    if (m_D3DAllocator.get())
        return m_D3DAllocator.get()->Free(m_D3DAllocator.get(),response);
    else
        return m_SYSAllocator.get()->Free(m_SYSAllocator.get(), response);

}
mfxStatus GeneralAllocator::AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    mfxStatus sts = MFX_ERR_NONE;
    if ((request->Type & MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET || request->Type & MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET) && m_D3DAllocator.get())
    {
        // Allocate frame buffer with video memory.
        sts = m_D3DAllocator.get()->Alloc(m_D3DAllocator.get(), request, response);
    } else {
        // Allocate frame buffer with system memory.
        sts = m_SYSAllocator.get()->Alloc(m_SYSAllocator.get(), request, response);
    }

    return sts;
}

