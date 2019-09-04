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

#ifndef __GENERAL_ALLOCATOR_H__
#define __GENERAL_ALLOCATOR_H__

#include <vector>
#include <memory>
#include "base_allocator.h"

// #if defined (PLATFORM_OS_WINDOWS)
// #include <map>

// #endif
#include <map>
class SysMemFrameAllocator;

class GeneralAllocator : public BaseFrameAllocator
{
public:
    GeneralAllocator();
    virtual ~GeneralAllocator();

#if defined (PLATFORM_OS_WINDOWS)
    virtual mfxStatus Init(mfxAllocatorParams *pParams);
#elif defined (PLATFORM_OS_LINUX)
    virtual mfxStatus Init(VADisplay *dpy);
    virtual mfxStatus Init(mfxAllocatorParams *pParams);
#endif
    virtual mfxStatus Close();

protected:
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);

    std::auto_ptr<BaseFrameAllocator>       m_D3DAllocator;
#if defined (PLATFORM_OS_WINDOWS)
    void StoreFrameMids(bool isD3DFrames, mfxFrameAllocResponse *response);
    bool isD3DMid(mfxHDL mid);
    std::map<mfxHDL, bool>                  m_Mids;
#endif
    std::auto_ptr<SysMemFrameAllocator>        m_SYSAllocator;

};
#endif //__GENERAL_ALLOCATOR_H__
