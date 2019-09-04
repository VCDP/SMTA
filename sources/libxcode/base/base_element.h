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

#ifndef BASEELEMENT_H
#define BASEELEMENT_H

/**
 *\file   BaseElement.h
 *
 *\brief  The element base class declaration.
 *
 */

#include <list>
#include "media_types.h"
#include "os/platform.h"
#if defined(PLATFORM_OS_WINDOWS)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#include "os/XC_Time.h"

class MediaPad;

class BaseElement
{
public:
    BaseElement();
    virtual ~BaseElement();

    virtual bool Init(void *cfg, ElementMode element_mode) = 0;

    int LinkNextElement(BaseElement *element);

    int UnlinkNextElement();

    int UnlinkPrevElement();

    bool Start();

    bool Stop();

    bool Join();

    bool IsAlive();

    ElementMode get_element_mode() {
        return element_mode_;
    };
    ElementType GetCodecType() {
        return element_type_;
    }

    bool IsRunning() {
       return is_running_;
    }
    bool is_running_;
    ElementType element_type_;

    // the input stream name in case decoder or dispatcher.
    const char* input_name;

protected:

    bool WaitSrcMutex();
    bool ReleaseSrcMutex();

    bool WaitSinkMutex();
    bool ReleaseSinkMutex();

    void SetThreadName(const char* name);

    ElementMode element_mode_;

    virtual void NewPadAdded(MediaPad *pad);

    virtual void PadRemoved(MediaPad *pad);

    virtual int HandleProcess();


    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf) = 0;

    virtual int Recycle(MediaBuf &buf) = 0;

    int ReturnBuf(MediaBuf &buf);

    std::list<MediaPad *> sinkpads_;
    std::list<MediaPad *> srcpads_;

private:
    MediaPad* GetFreePad(int type);
    int AddPad(MediaPad *pad);
    int RemovePad(MediaPad *pad);

    static int ProcessChainCallback(MediaBuf &buf, void *arg1, void *arg2);
    static int RecycleCallback(MediaBuf &buf, void *arg);

#if defined(PLATFORM_OS_WINDOWS)
    static void ThreadFunc(void *arg);
#else
    static void *ThreadFunc(void *arg);
#endif

    bool thread_created_;
#if defined(PLATFORM_OS_WINDOWS)
    HANDLE src_pad_mutex_;
    HANDLE sink_pad_mutex_;
    uintptr_t thread_id_;
#else
    pthread_mutex_t src_pad_mutex_;
    pthread_mutex_t sink_pad_mutex_;
    pthread_t thread_id_;
#endif
};

class CodecEventCallback {
public:
    CodecEventCallback(){}
    virtual ~CodecEventCallback(){}

    virtual void DecodeHeaderFailEvent(void *DecHandle = 0) = 0;
};
#endif

