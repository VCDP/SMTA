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
#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

#include <stdio.h>
#include "trace.h"
#include <vector>
#include <string>
#include "os/mutex.h"
#include "os/XC_Time.h"

typedef struct
{
    unsigned long start_t;
    unsigned long finish_t;
} timerec;

typedef struct
{
    unsigned int mElementType;
    std::string mInputName;
    std::string mCodecName;
    unsigned int mChannelNum;
} pipelineinfo;

typedef std::vector<timerec> timestamp;
typedef struct
{
    pipelineinfo pinfo;
    void *id;
    timestamp FrameTimeStp;
    timestamp Enduration;
    timestamp InitStp;
} codecdata;

typedef std::vector<codecdata> codecgroup;

enum StampType {
    ENC_FRAME_TIME_STAMP = 1, //record time cost of each frame
    VPP_FRAME_TIME_STAMP,
    DEC_FRAME_TIME_STAMP,
    VA_FRAME_TIME_STAMP,
    PREENC_ENCODE_FRAME_TIME_STAMP,
    ENC_ENDURATION_TIME_STAMP, //the enduration of the element
    VPP_ENDURATION_TIME_STAMP, //the enduration of the element
    DEC_ENDURATION_TIME_STAMP, //the enduration of the element
    VA_ENDURATION_TIME_STAMP,
    PREENC_ENCODE_ENDURATION_TIME_STAMP, //the enduration of the element
    PREENC_ENCODE_INIT_TIME_STAMP,
    ENC_INIT_TIME_STAMP,
    VPP_INIT_TIME_STAMP,
    DEC_INIT_TIME_STAMP
};

enum MeasurementError {
    MEASUREMNT_ERROR_NONE = 0,
    MEASUREMNT_ERROR_CODECDATA,
    MEASUREMNT_FINISHSTP_NOT_SET
};

class Measurement
{
public:
    Measurement();
    virtual ~Measurement();

    void StartTime();
    void EndTime();
    void EndTime(unsigned long *time);
    void GetCurTime(unsigned long *time);
    void GetLock();
    void RelLock();
    //void    Log(unsigned char level, const char* fmt, ...);

    unsigned int FrameTime()         {
        return frame_time_;
    };
    unsigned int ChannelCodecTime()  {
        return channel_codec_time_;
    };

    void UpdateCodecNum()    {
        ++channel_codec_num_;
    };
    void UpdateWholeNum()    {
        ++whole_num_;
    };
    unsigned int ChannelCodecNum()   {
        return channel_codec_num_;
    };
    unsigned int WholeNum()          {
        return whole_num_;
    };

    unsigned long CalcInterval(timerec *tr);
    codecdata *GetCodecData(StampType st, void *id);

    int TimeStpStart(StampType st, void *hdl);

    int TimeStpFinish(StampType st, void *hdl);

    int SetElementInfo(StampType st, void *hdl, pipelineinfo *pif);

    //show the performance data, or do something user wanted with the data
    void ShowPerformanceInfo();
    void GetPerformanceInfo(std::string &res);

protected:
private :
    //void GetFmtTime(char *time, size_t buf_sz);
    struct timeval ttime_;
    /*codec time*/
    unsigned long frame_time_; /*codec time for one frame*/
    unsigned long channel_codec_time_;/*codec time for one stream*/
    /*codec num*/
    unsigned int channel_codec_num_;/*codec num for one stream*/
    static unsigned int whole_num_;/*a total num for all stream*/
    /*performance data*/
    codecgroup mEncGrp;
    codecgroup mDecGrp;
    codecgroup mVppGrp;
    Mutex mMutex;

    static unsigned int cnt_bench, sum;
    static unsigned long init_enc, init_vpp, init_dec;
};
#endif /* _MEASUREMENT_H_ */
