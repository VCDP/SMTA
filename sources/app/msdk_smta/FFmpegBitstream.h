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

#ifndef __FILE_BITSTREAM_H__
#define __FILE_BITSTREAM_H__

#include <memory>
#include "sample_utils.h"
#include "InputParams.h"
#include "BitstreamSink.h"
#include "FFmpegReader.h"
#include "FFmpegWriter.h"

namespace TranscodingSample {

    class BitstreamSinkFactory : public IBitstreamSinkFactory
    {
    public:
        virtual BitstreamSink* getInstance(BitstreamSink::OutputType type,
                    string url, bool isDump);
    };

} // ns

#endif // __FILE_BITSTREAM_H__
