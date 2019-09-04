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
#ifndef TEE_H
#define TEE_H

#include <list>
#include <map>

#include "base_element.h"
#include "media_types.h"
#include "media_pad.h"
#include "os/mutex.h"

// An 1 to N class
class Dispatcher: public BaseElement
{
public:
    Dispatcher();
    ~Dispatcher();

    virtual bool Init(void *cfg, ElementMode element_mode);

    virtual int Recycle(MediaBuf &buf);

    virtual int HandleProcess();

    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);

protected:
    virtual void PadRemoved(MediaPad *pad);

private:
    std::map<void*, unsigned int> sur_ref_map_;

    Mutex surMutex;
    // TODO:
    // Dynamic add/remove session, may need mutex
};
#endif

