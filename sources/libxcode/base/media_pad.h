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

#ifndef MEDIAPAD_H
#define MEDIAPAD_H

#include "media_types.h"
#include "ring_buffer.h"
#include "logger.h"

#define FRAME_POOL_Q_SIZE 16
#define FRAME_POOL_LOW_WATER_LEVEL (2)
typedef int (*ElementChainFunc)(MediaBuf &buf, void *arg1, void *arg2);
typedef int (*ElementRecycleFunc)(MediaBuf &buf, void *arg);

typedef enum {
    MEDIA_PAD_UNKNOWN,
    MEDIA_PAD_SRC,
    MEDIA_PAD_SINK
} MediaPadDirection;

typedef enum {
    MEDIA_PAD_UNLINKED,
    MEDIA_PAD_LINKED
} MediaPadLinkStatus;


class BaseElement;

class MediaPad
{
public:
    DECLARE_MLOGINSTANCE();
    MediaPad(MediaPadDirection direction);
    virtual ~MediaPad();

    void set_parent(BaseElement *parent) {
        parent_ = parent;
    }

    void set_parent_mode(ElementMode mode) {
        parent_element_mode_ = mode;
    }

    BaseElement *get_parent() {
        return parent_;
    }

    // Element push data to its peer, SRC => SINK
    int PushBufToPeerPad(MediaBuf &buf);

    // Return used SID back to its peer, SINK => SRC
    int ReturnBufToPeerPad(MediaBuf &buf);

    int SetPeerPad(MediaPad *peer);

    MediaPad *get_peer_pad() {
        return peer_pad_;
    }

    int SetChainFunction(ElementChainFunc func, void *arg1);

    int SetRecycleFunction(ElementRecycleFunc func, void *arg);

    int PushBufData(MediaBuf &buf);

    int GetBufData(MediaBuf &buf);

    int GetBufQueueSize();

    int PickBufData(MediaBuf &buf);

    void set_pad_status(MediaPadLinkStatus status) {
        pad_status_ = status;
    }
    MediaPadLinkStatus get_pad_status() {
        return pad_status_;
    }

    void set_pad_direction(MediaPadDirection direction) {
        pad_direction_ = direction;
    }

    MediaPadDirection get_pad_direction() {
        return pad_direction_;
    }

    // Processing data
    int ProcessBuf(MediaBuf &buf);

    // Peer recycle data
    int RecycleBuf(MediaBuf &buf);

private:
    MediaPad();

    ElementChainFunc chain_func_;
    void *chain_func_arg_;
    RingBuffer < MediaBuf, FRAME_POOL_Q_SIZE + 1 > in_buf_q_;
    MediaPadDirection pad_direction_;
    MediaPadLinkStatus pad_status_;
    BaseElement *parent_;
    ElementMode parent_element_mode_;
    MediaPad *peer_pad_;
    ElementRecycleFunc recycle_func_;
    void *recycle_func_arg_;
};

#endif
