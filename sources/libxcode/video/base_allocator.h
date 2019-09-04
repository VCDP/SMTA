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

#ifndef __BASE_ALLOCATOR_H__
#define __BASE_ALLOCATOR_H__

#include <list>
#include <string.h>
#include <functional>
#include "os/platform.h"
#ifdef PLATFORM_OS_LINUX
#include <va/va.h>
#endif
#include "mfxvideo.h"
#include <stdio.h>

struct mfxAllocatorParams
{
    virtual ~mfxAllocatorParams(){};
};

// this class implements methods declared in mfxFrameAllocator structure
// simply redirecting them to virtual methods which should be overridden in derived classes
class MFXFrameAllocator : public mfxFrameAllocator
{
public:
    MFXFrameAllocator();
    virtual ~MFXFrameAllocator();

     // optional method, override if need to pass some parameters to allocator from application
#if defined (PLATFORM_OS_WINDOWS)
    virtual mfxStatus Init(mfxAllocatorParams *pParams) = 0;
#elif defined (PLATFORM_OS_LINUX)
    virtual mfxStatus Init(VADisplay *dpy) = 0;
#endif
    virtual mfxStatus Close() = 0;

    virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response) = 0;
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle) = 0;
    virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response) = 0;

private:
    static mfxStatus MFX_CDECL  Alloc_(mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    static mfxStatus MFX_CDECL  Lock_(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    static mfxStatus MFX_CDECL  Unlock_(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
    static mfxStatus MFX_CDECL  GetHDL_(mfxHDL pthis, mfxMemId mid, mfxHDL *handle);
    static mfxStatus MFX_CDECL  Free_(mfxHDL pthis, mfxFrameAllocResponse *response);
};

// This class implements basic logic of memory allocator
// Manages responses for different components according to allocation request type
// External frames of a particular component-related type are allocated in one call
// Further calls return previously allocated response.
// Ex. Preallocated frame chain with type=FROM_ENCODE | FROM_VPPIN will be returned when
// request type contains either FROM_ENCODE or FROM_VPPIN

// This class does not allocate any actual memory
class BaseFrameAllocator: public MFXFrameAllocator
{
public:
    BaseFrameAllocator();
    virtual ~BaseFrameAllocator();

#if defined (PLATFORM_OS_WINDOWS)
    virtual mfxStatus Init(mfxAllocatorParams *pParams) = 0;
#elif defined (PLATFORM_OS_LINUX)
    virtual mfxStatus Init(VADisplay *dpy) = 0;
#endif
    virtual mfxStatus Close();
    virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response);

protected:
    typedef std::list<mfxFrameAllocResponse>::iterator Iter;
    static const mfxU32 MEMTYPE_FROM_MASK = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_FROM_DECODE | \
                                            MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_FROM_VPPOUT | \
                                            MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK;

    static const mfxU32 MEMTYPE_FROM_MASK_INT_EXT = MEMTYPE_FROM_MASK | MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_EXTERNAL_FRAME;

    struct UniqueResponse
        : mfxFrameAllocResponse
    {
        mfxU16 m_width;
        mfxU16 m_height;
        mfxU32 m_refCount;
        mfxU16 m_type;

        UniqueResponse()
            : m_width(0)
            , m_height(0)
            , m_refCount(0)
            , m_type(0)
        {
            //memset(static_cast<mfxFrameAllocResponse*>(this), 0, sizeof(mfxFrameAllocResponse));
        }

        // compare responses by actual frame size, alignment (w and h) is up to application
        UniqueResponse(const mfxFrameAllocResponse & response, mfxU16 width, mfxU16 height, mfxU16 type)
            : mfxFrameAllocResponse(response)
            , m_width(width)
            , m_height(height)
            , m_refCount(1)
            , m_type(type)
        {
        }

        //compare by resolution (and memory type for FEI ENC / PAK)
        bool operator () (const UniqueResponse &response)const
        {
            if (m_width <= response.m_width && m_height <= response.m_height)
            {
                // For FEI ENC and PAK we need to distinguish between INTERNAL and EXTERNAL frames

                if (m_type & response.m_type & (MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK))
                {
                    //return !!((m_type & response.m_type) & 0x000f);
                    return m_width == response.m_width && m_height == response.m_height;
                }
                else
                {
                    //return !!(m_type & response.m_type & MFX_MEMTYPE_FROM_DECODE);
                    return m_width == response.m_width && m_height == response.m_height;
                }
            }
            else
            {
                return false;
            }
        }

        static mfxU16 CropMemoryTypeToStore(mfxU16 type)
        {
            // Remain INTERNAL / EXTERNAL flag for FEI ENC / PAK
            switch (type & 0xf000)
            {
            case MFX_MEMTYPE_FROM_ENC:
            case MFX_MEMTYPE_FROM_PAK:
            case (MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK):
                return type & MEMTYPE_FROM_MASK_INT_EXT;
                break;
            default:
                return type & MEMTYPE_FROM_MASK;
                break;
            }
        }
    };

    std::list<mfxFrameAllocResponse> m_responses;
    std::list<UniqueResponse> m_ExtResponses;

    struct IsSame
        : public std::binary_function<mfxFrameAllocResponse, mfxFrameAllocResponse, bool>
    {
        bool operator () (const mfxFrameAllocResponse & l, const mfxFrameAllocResponse &r)const
        {
            return r.mids != 0 && l.mids != 0 &&
                r.mids[0] == l.mids[0] &&
                r.NumFrameActual == l.NumFrameActual;
        }
    };

    // checks if request is supported
    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);

    // frees memory attached to response
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response) = 0;
    // allocates memory
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response) = 0;

    template <class T>
    class safe_array
    {
    public:
        safe_array(T *ptr = 0):m_ptr(ptr)
        { // construct from object pointer
        };
        ~safe_array()
        {
            reset(0);
        }
        T* get()
        { // return wrapped pointer
            return m_ptr;
        }
        T* release()
        { // return wrapped pointer and give up ownership
            T* ptr = m_ptr;
            m_ptr = 0;
            return ptr;
        }
        void reset(T* ptr)
        { // destroy designated object and store new pointer
            if (m_ptr)
            {
                delete[] m_ptr;
            }
            m_ptr = ptr;
        }
    protected:
        T* m_ptr; // the wrapped object pointer
    private:
        safe_array(const safe_array&);
        safe_array& operator=(const safe_array&);
    };
};

class MFXBufferAllocator : public mfxBufferAllocator
{
public:
    MFXBufferAllocator();
    virtual ~MFXBufferAllocator();

    virtual mfxStatus AllocBuffer(mfxU32 nbytes, mfxU16 type, mfxMemId *mid) = 0;
    virtual mfxStatus LockBuffer(mfxMemId mid, mfxU8 **ptr) = 0;
    virtual mfxStatus UnlockBuffer(mfxMemId mid) = 0;
    virtual mfxStatus FreeBuffer(mfxMemId mid) = 0;

private:
    static mfxStatus MFX_CDECL  Alloc_(mfxHDL pthis, mfxU32 nbytes, mfxU16 type, mfxMemId *mid);
    static mfxStatus MFX_CDECL  Lock_(mfxHDL pthis, mfxMemId mid, mfxU8 **ptr);
    static mfxStatus MFX_CDECL  Unlock_(mfxHDL pthis, mfxMemId mid);
    static mfxStatus MFX_CDECL  Free_(mfxHDL pthis, mfxMemId mid);
};


#endif // __BASE_ALLOCATOR_H__
