/****************************************************************************
 *
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
 *
 ****************************************************************************/

DEF_LOG_FLAG(VIDEO,                 0x00000001)
DEF_LOG_FLAG(VIDEO_DETAIL,          0x00000002)

DEF_LOG_FLAG(LOOP,                  0x00000010)
DEF_LOG_FLAG(SYNC_START,            0x00000020)
DEF_LOG_FLAG(DISPATCH,              0x00000040)
DEF_LOG_FLAG(PAYLOAD_DTOR,          0x00000080)
DEF_LOG_FLAG(DEBUG_TIME,            0x00000100)
DEF_LOG_FLAG(TIME,                  0x00000200)

DEF_LOG_FLAG(AUDIO,                 0x00001000)
DEF_LOG_FLAG(AUDIO_DEC,             0x00002000)
DEF_LOG_FLAG(AUDIO_DEC_DETAILS,     0x00004000)
DEF_LOG_FLAG(AUDIO_ENC,             0x00008000)
DEF_LOG_FLAG(AUDIO_ENC_DETAILS,     0x00010000)
DEF_LOG_FLAG(AUDIO_ENC_TIMESTAMP,   0x00020000)
DEF_LOG_FLAG(AUDIO_DETAIL,          0x00001001)

DEF_LOG_FLAG(STREAM_OUTPUT,         0x00100000)
DEF_LOG_FLAG(STREAM_INPUT,          0x00200000)
DEF_LOG_FLAG(SPLITTER,              0x00400000)
DEF_LOG_FLAG(SPLITTER_BUFFER,       0x00800000)
DEF_LOG_FLAG(SPLITTER_BUFFER_CYCLE, 0x01000000)
DEF_LOG_FLAG(STREAM_OUTPUT_MUXING,  0x02000000)
DEF_LOG_FLAG(FFMPEG,                0x04000000)
