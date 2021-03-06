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

#include "measurement.h"

#include <string.h>
#include <stdio.h>

#if _MSC_VER
#define snprintf _snprintf_s
#endif

#define defaultFile "./log.txt"

using namespace std;

unsigned int Measurement::whole_num_ = 0;
unsigned int Measurement::cnt_bench = 0;
unsigned int Measurement::sum = 0;
unsigned long Measurement::init_enc = 0;
unsigned long Measurement::init_vpp = 0;
unsigned long Measurement::init_dec = 0;

Measurement::Measurement()
{
    ttime_.tv_usec = 0;
    ttime_.tv_sec = 0;
    frame_time_ = 0;
    channel_codec_time_ = 0;
    channel_codec_num_ = 0;
    cnt_bench++;
    sum++;
}

Measurement::~Measurement()
{
    mEncGrp.clear();
    mDecGrp.clear();
    mVppGrp.clear();
    --cnt_bench;
}

void Measurement::GetLock()
{
    mMutex.Lock();
}

void Measurement::RelLock()
{
    mMutex.Unlock();
}

void Measurement::StartTime()
{
    gettimeofday (&ttime_, NULL);
}

void Measurement::EndTime()
{
    struct timeval cur_tv, last_tv;
    unsigned long elapsed_time;
    /*calculate the current time*/
    gettimeofday (&cur_tv, NULL);

    last_tv = ttime_;

    if (cur_tv.tv_usec < last_tv.tv_usec) {
        cur_tv.tv_sec--;
        elapsed_time = 1000000 + cur_tv.tv_usec - last_tv.tv_usec;
    } else {
        elapsed_time = cur_tv.tv_usec - last_tv.tv_usec;
    }

    elapsed_time += (cur_tv.tv_sec - last_tv.tv_sec) * 1000000;
    frame_time_ = elapsed_time;
    channel_codec_time_ += elapsed_time;
}

void Measurement::EndTime(unsigned long *time)
{
    struct timeval cur_tv, last_tv;
    unsigned long elapsed_time;
    /*calculate the current time*/
    gettimeofday (&cur_tv, NULL);

    last_tv = ttime_;

    if (cur_tv.tv_usec < last_tv.tv_usec) {
        cur_tv.tv_sec--;
        elapsed_time = 1000000 + cur_tv.tv_usec - last_tv.tv_usec;
    } else {
        elapsed_time = cur_tv.tv_usec - last_tv.tv_usec;
    }

    elapsed_time += (cur_tv.tv_sec - last_tv.tv_sec) * 1000000;
    *time = elapsed_time;
}

void Measurement::GetCurTime(unsigned long *time)
{
    struct timeval ttime;
    gettimeofday (&ttime, NULL);
    *time = ttime.tv_sec * 1000000 + ttime.tv_usec;
}

#if 0
void Measurement::GetFmtTime(char *time, size_t buf_sz)
{
    struct timeb tb;
    struct tm *t;
    char buf[64] = {0};
    ftime(&tb);
    t = localtime(&tb.time);
    if (t) {
        strftime(buf, 64, "%F %T ", t);
    }
    snprintf(time, buf_sz, "%s", buf);
}
#endif

unsigned long Measurement::CalcInterval(timerec *tr)
{
    if(tr->finish_t < tr->start_t)
        return 0;
    else
        return (tr->finish_t - tr->start_t);
}

codecdata* Measurement::GetCodecData(StampType st, void *id)
{
    codecgroup *cgrp;
    switch(st) {
        case PREENC_ENCODE_ENDURATION_TIME_STAMP:
        case PREENC_ENCODE_FRAME_TIME_STAMP:
        case PREENC_ENCODE_INIT_TIME_STAMP:
        case ENC_ENDURATION_TIME_STAMP:
        case ENC_FRAME_TIME_STAMP:
        case ENC_INIT_TIME_STAMP:
            cgrp = &mEncGrp;
            break;
        case VPP_ENDURATION_TIME_STAMP:
        case VPP_FRAME_TIME_STAMP:
        case VPP_INIT_TIME_STAMP:
            cgrp = &mVppGrp;
            break;
        case DEC_ENDURATION_TIME_STAMP:
        case DEC_FRAME_TIME_STAMP:
        case DEC_INIT_TIME_STAMP:
            cgrp = &mDecGrp;
            break;
        default:
            return NULL;
    }
    for(unsigned int i = 0; i < cgrp->size(); i++) {
        if((*cgrp)[i].id == id) {
            return &((*cgrp)[i]);
        }
    }
    //new codec data
    codecdata cdata;
    cdata.id = id;
    cgrp->push_back(cdata);
    return &(cgrp->back());
}

int Measurement::SetElementInfo(StampType st, void *hdl, pipelineinfo *pif)
{
    codecdata *codec_d = GetCodecData(st, hdl);
    if(codec_d == NULL) {
        MSMT_TRACE_ERROR("codec time stamp data allocate error.");
        return MEASUREMNT_ERROR_CODECDATA;
    }

    codec_d->pinfo = *pif;
    return MEASUREMNT_ERROR_NONE;
}

int Measurement::TimeStpStart(StampType st, void *hdl)
{
    codecdata *codec_d = GetCodecData(st, hdl);
    if(codec_d == NULL) {
        MSMT_TRACE_ERROR("codec time stamp data allocate error.");
        return MEASUREMNT_ERROR_CODECDATA;
    }

    timerec tm;
    timestamp *ptmstp = NULL;
    switch(st) {
        case PREENC_ENCODE_ENDURATION_TIME_STAMP:
        case ENC_ENDURATION_TIME_STAMP:
        case DEC_ENDURATION_TIME_STAMP:
        case VPP_ENDURATION_TIME_STAMP:
            ptmstp = &(codec_d->Enduration);
            break;
        case PREENC_ENCODE_FRAME_TIME_STAMP:
        case ENC_FRAME_TIME_STAMP:
        case VPP_FRAME_TIME_STAMP:
        case DEC_FRAME_TIME_STAMP:
            ptmstp = &(codec_d->FrameTimeStp);
            break;
        case PREENC_ENCODE_INIT_TIME_STAMP:
        case ENC_INIT_TIME_STAMP:
        case DEC_INIT_TIME_STAMP:
        case VPP_INIT_TIME_STAMP:
            ptmstp = &(codec_d->InitStp);
            break;
        default: //other type of stamp may be added
            break;
    }
    if(ptmstp->size() > 0 && ptmstp->back().finish_t == 0) { //it lost setting the finish time
        return MEASUREMNT_FINISHSTP_NOT_SET;
    }
    GetCurTime(&tm.start_t);
    tm.finish_t = 0;
    ptmstp->push_back(tm);

    return MEASUREMNT_ERROR_NONE;
}

int Measurement::TimeStpFinish(StampType st, void *hdl)
{
    codecdata *codec_d = GetCodecData(st, hdl);
    if(codec_d == NULL) {
        MSMT_TRACE_ERROR("codec time stamp data allocate error.");
        return MEASUREMNT_ERROR_CODECDATA;
    }
    timestamp *ptmstp = NULL;
    switch(st) {
        case PREENC_ENCODE_ENDURATION_TIME_STAMP:
        case ENC_ENDURATION_TIME_STAMP:
        case DEC_ENDURATION_TIME_STAMP:
        case VPP_ENDURATION_TIME_STAMP:
            ptmstp = &(codec_d->Enduration);
            break;
        case PREENC_ENCODE_FRAME_TIME_STAMP:
        case ENC_FRAME_TIME_STAMP:
        case VPP_FRAME_TIME_STAMP:
        case DEC_FRAME_TIME_STAMP:
            ptmstp = &(codec_d->FrameTimeStp);
            break;
        case PREENC_ENCODE_INIT_TIME_STAMP:
        case ENC_INIT_TIME_STAMP:
        case DEC_INIT_TIME_STAMP:
        case VPP_INIT_TIME_STAMP:
            ptmstp = &(codec_d->InitStp);
            break;
        default: //other type of stamp may be added
            break;
    }
    GetCurTime(&(ptmstp->back().finish_t));

    return MEASUREMNT_ERROR_NONE;
}

void Measurement::ShowPerformanceInfo()
{
    unsigned long codecnum = 0;

    MSMT_TRACE_INFO("<TRANSCODER> total %zu decoder(s), %zu vpp(s), %zu encoder(s) within the pipeline.\n\
            _____________________________________________________________________________\n",
            mDecGrp.size(), mVppGrp.size(), mEncGrp.size());

    for(unsigned int n = 0; n < mDecGrp.size(); n++) {
        codecdata *pd = &mDecGrp[n];
        unsigned long dec_l, dec_avg, initialization;
        dec_l = dec_avg = initialization = 0;
        if(pd->FrameTimeStp.size() == 0 ) {
            MSMT_TRACE_INFO("<DEC> nothing to do. Maybe bad clips.\n");
            continue;
        }
        if(pd->FrameTimeStp.back().finish_t == 0) {
            codecnum = pd->FrameTimeStp.size() - 1;
        }
        else {
            codecnum = pd->FrameTimeStp.size();
        }
        for(unsigned long i = 0; i< codecnum; i++)
        {
            dec_l += CalcInterval(&pd->FrameTimeStp[i]);
        }
        if (codecnum != 0) {
            dec_avg = dec_l * 1.0 / codecnum;
        }
        for(unsigned int i = 0; i< pd->InitStp.size(); i++) {
            initialization += CalcInterval(&(pd->InitStp)[i]);
        }
        if (dec_l == 0)
            dec_l = 1;
        MSMT_TRACE_INFO("<DEC> %d\n\
                ----------------------------\n\
                Input               = %s\n\
                DecodedNum          = %ld\n\
                InitLatency         = %2ld.%2ldms\n\
                AvgDecodeLatency    = %2ld.%2ldms\n\
                DecoderFps          = %.2f\n",
                pd->pinfo.mChannelNum,
                pd->pinfo.mInputName.c_str(),
                codecnum,
                initialization / 1000, initialization % 1000,
                dec_avg / 1000, dec_avg % 1000,
                1000000.0 * codecnum / dec_l);
        init_dec += initialization;
    }

    for(unsigned int n = 0; n < mVppGrp.size(); n++) {
        codecdata *pd = &mVppGrp[n];
        unsigned long initialization = 0;
        unsigned long vpp_l, vpp_avg;
        vpp_l = vpp_avg = 0;
        codecnum = pd->FrameTimeStp.size();
        for(unsigned long i = 0; i < codecnum; i++) {
            vpp_l += CalcInterval(&pd->FrameTimeStp[i]);
        }
        if (codecnum != 0) {
            vpp_avg = vpp_l * 1.0 / codecnum;
        }

        for(unsigned int i = 0; i< pd->InitStp.size(); i++) {
            initialization += CalcInterval(&(pd->InitStp)[i]);
        }
        if (vpp_l == 0)
            vpp_l = 1;
        MSMT_TRACE_INFO("<VPP>   \n\
                            ---------------------------\n\
                            InitLatency         = %2ld.%3ldms\n\
                            VppedNum            = %ld\n\
                            AvgVppLatency       = %2ld.%2ldms\n\
                            VppFps              = %.2f\n",
                initialization /1000, initialization % 1000,
                codecnum,
                vpp_avg / 1000, vpp_avg % 1000,
                1000000.0 * codecnum / vpp_l);
        init_vpp += initialization;
    }

    for(unsigned int n = 0; n < mEncGrp.size(); n++) {
        codecdata *pd = &mEncGrp[n];
        //here is benchmark for encoder
        unsigned long enc_l, enc_avg, max_l, min_l, first_frame_latency, enduration, initialization, pipe_l;
        enc_l = enc_avg = max_l = first_frame_latency = enduration = initialization = pipe_l = 0;
        min_l = 0xffffffff;
        if(pd->FrameTimeStp.size() == 0 ) {
            MSMT_TRACE_INFO("<ENC> nothing to do. Maybe bad clips.\n");
            continue;
        }
        if(pd->FrameTimeStp.back().finish_t == 0) {
            codecnum = pd->FrameTimeStp.size() - 1;
        }
        else {
            codecnum = pd->FrameTimeStp.size();
        }
        for(unsigned long i = 0; i< codecnum; i++)
        {
            unsigned long tmp = CalcInterval(&pd->FrameTimeStp[i]);
            enc_l += tmp;
            if(tmp > max_l) {
                max_l = tmp;
            }
            if(tmp < min_l) {
                min_l = tmp;
            }
        }
        if (codecnum != 0)
            enc_avg = enc_l * 1.0 / codecnum;
        for(unsigned int i = 0; i< pd->Enduration.size(); i++) {
            enduration += CalcInterval(&(pd->Enduration)[i]);
        }
        for(unsigned int i = 0; i< pd->InitStp.size(); i++) {
            initialization += CalcInterval(&(pd->InitStp)[i]);
        }
        first_frame_latency = CalcInterval(&(pd->FrameTimeStp[0]));
        if (!enc_l)
            enc_l = 1;
        MSMT_TRACE_INFO("<ENC> %d\n\
                ---------------------------\n\
                Codec               = %s\n\
                EncodedNum          = %ld\n\
                InitLatency         = %2ld.%3ldms\n\
                AvgEncLatency       = %2ld.%3ldms\n\
                EncoderFPS          = %.2f\n",
                pd->pinfo.mChannelNum,
                pd->pinfo.mCodecName.c_str(),
                codecnum,
                initialization / 1000, initialization % 1000,
                enc_avg / 1000, enc_avg % 1000,
                1000000.0 * codecnum / enc_l);
        init_enc += initialization;

        codecdata* pd_dec = NULL;
        if(mDecGrp.size() == 1) { //1:N use case
            pd_dec = &mDecGrp[0];
        }
        else if(mDecGrp.size() >= mEncGrp.size()) {
            pd_dec = &mDecGrp[n];
        }
        else {
            MSMT_TRACE_ERROR("Number of decoder is not match with encoder.");
            break;
        }

        if( mDecGrp.size() > 1 && mEncGrp.size() == 1) { //N:1 use case
            unsigned long earliest_fst = mDecGrp[0].InitStp[0].start_t;
            for(unsigned int n = 1; n < mDecGrp.size(); n++) {
                codecdata *pdec = &mDecGrp[n];
                if(earliest_fst > pdec->InitStp[0].start_t)
                    earliest_fst = pdec->InitStp[0].start_t;
            }
            first_frame_latency = pd->FrameTimeStp[0].finish_t - earliest_fst;
            max_l = min_l = first_frame_latency;
            pipe_l += first_frame_latency; //init latency included
            for(unsigned long i = 1; i< codecnum; i++) {
                unsigned long earliest = 0xffffffffffffffff;
                for(unsigned int n = 0; n < mDecGrp.size(); n++) { //find the earliest input frame in this group
                    codecdata *pdec = &mDecGrp[n];
                    if(pdec->FrameTimeStp.size() >= i+1) { //this decoder frame is not EOF
                        if(earliest > pdec->FrameTimeStp[i].start_t)
                            earliest = pdec->FrameTimeStp[i].start_t;
                    }
                }
                unsigned long tmp = pd->FrameTimeStp[i].finish_t - earliest;
                pipe_l += tmp;
                if( tmp > max_l ) {
                    max_l = tmp;
                }
                if(tmp < min_l) {
                    min_l = tmp;
                }
            }
            pipe_l = pipe_l / codecnum;
        }
        else { //1:N or N:N
            first_frame_latency = pd->FrameTimeStp[0].finish_t - pd_dec->InitStp[0].start_t;
            codecnum = (codecnum < pd_dec->FrameTimeStp.size())?codecnum:pd_dec->FrameTimeStp.size();
            max_l = min_l = first_frame_latency;
            pipe_l += first_frame_latency; //init latency included
            for(unsigned long i = 1; i< codecnum; i++)
            {
                unsigned long tmp = pd->FrameTimeStp[i].finish_t - pd_dec->FrameTimeStp[i].start_t;
                pipe_l += tmp;
                if( tmp > max_l ) {
                    max_l = tmp;
                }
                if(tmp < min_l) {
                    min_l = tmp;
                }
            }
            pipe_l = pipe_l / codecnum;
        }
        if (!enduration)
            enduration = 1;
        MSMT_TRACE_INFO("<Pipeline> %d\n\
                ---------------------------\n\
                FPS                 = %.2f\n\
                FirstFrameLatency   = %2ld.%3ldms\n\
                MaxFrameLatency     = %2ld.%3ldms\n\
                MinFrameLatency     = %2ld.%3ldms\n\
                AvgFrameLatency     = %2ld.%3ldms\n",
                pd->pinfo.mChannelNum,
                1000000.0 * codecnum / enduration,
                first_frame_latency / 1000, first_frame_latency % 1000,
                max_l / 1000, max_l % 1000,
                min_l / 1000, min_l % 1000,
                pipe_l / 1000, pipe_l % 1000);
    }
#if 0
    //for multi-session avg. init latency
    if(cnt_bench == 1 && sum !=0) {
        MSMT_TRACE_INFO("==============Init latency============\n");
        MSMT_TRACE_INFO("Over all %d sessions, \n\
                dec avg init lantency: %2ld.%3ldms\n\
                vpp avg init lantency: %2ld.%3ldms\n\
                enc avg init lantency: %2ld.%3ldms\n",
                sum,
                (init_dec/sum) / 1000, (init_dec/sum) % 1000,
                (init_vpp/sum) / 1000, (init_vpp/sum) % 1000,
                (init_enc/sum) / 1000, (init_enc/sum) % 1000);
    }
#endif
}

void Measurement::GetPerformanceInfo(string &res)
{
    unsigned long codecnum = 0;
    char tmp_r[1024 * 4];

    snprintf(tmp_r, sizeof(tmp_r), "<TRANSCODER> total %zu decoder(s), %zu vpp(s), %zu encoder(s) within the pipeline.\n\
            _____________________________________________________________________________\n",
            mDecGrp.size(), mVppGrp.size(), mEncGrp.size());
    res += tmp_r;

    for(unsigned int n = 0; n < mDecGrp.size(); n++) {
        codecdata *pd = &mDecGrp[n];
        unsigned long dec_l, dec_avg, initialization;
        dec_l = dec_avg = initialization = 0;
        if(pd->FrameTimeStp.size() == 0 ) {
            MSMT_TRACE_INFO("<DEC> nothing to do. Maybe bad clips.\n");
            continue;
        }
        if(pd->FrameTimeStp.back().finish_t == 0) {
            codecnum = pd->FrameTimeStp.size() - 1;
        }
        else {
            codecnum = pd->FrameTimeStp.size();
        }
        for(unsigned long i = 0; i< codecnum; i++)
        {
            dec_l += CalcInterval(&pd->FrameTimeStp[i]);
        }
        if (codecnum != 0) {
            dec_avg = dec_l * 1.0 / codecnum;
        }
        if (!dec_l)
            dec_l = 1;
        for(unsigned int i = 0; i< pd->InitStp.size(); i++) {
            initialization += CalcInterval(&(pd->InitStp)[i]);
        }
        snprintf(tmp_r, sizeof(tmp_r), "<DEC> %d\n\
                ----------------------------\n\
                Input               = %s\n\
                DecodedNum          = %ld\n\
                InitLatency         = %2ld.%2ldms\n\
                AvgDecodeLatency    = %2ld.%2ldms\n\
                DecoderFps          = %.2f\n",
                pd->pinfo.mChannelNum,
                pd->pinfo.mInputName.c_str(),
                codecnum,
                initialization / 1000, initialization % 1000,
                dec_avg / 1000, dec_avg % 1000,
                1000000.0 * codecnum / dec_l);
        res += tmp_r;
        init_dec += initialization;
    }

    for(unsigned int n = 0; n < mVppGrp.size(); n++) {
        codecdata *pd = &mVppGrp[n];
        unsigned long initialization = 0;
        for(unsigned int i = 0; i< pd->InitStp.size(); i++) {
            initialization += CalcInterval(&(pd->InitStp)[i]);
        }
        snprintf(tmp_r, sizeof(tmp_r), "<VPP> %d\n\
                ---------------------------\n\
                InitLatency         = %2ld.%3ldms\n",
                n,
                initialization /1000, initialization % 1000);
        res += tmp_r;
        init_vpp += initialization;
    }

    for(unsigned int n = 0; n < mEncGrp.size(); n++) {
        codecdata *pd = &mEncGrp[n];
        //here is benchmark for encoder
        unsigned long enc_l, enc_avg, max_l, min_l, first_frame_latency, enduration, initialization, pipe_l;
        enc_l = enc_avg = max_l = first_frame_latency = enduration = initialization = pipe_l = 0;
        min_l = 0xffffffff;
        if(pd->FrameTimeStp.size() == 0 ) {
            snprintf(tmp_r, sizeof(tmp_r), "<ENC> nothing to do. Maybe bad clips.\n");
            res += tmp_r;
            continue;
        }
        if(pd->FrameTimeStp.back().finish_t == 0) {
            codecnum = pd->FrameTimeStp.size() - 1;
        }
        else {
            codecnum = pd->FrameTimeStp.size();
        }
        for(unsigned long i = 0; i< codecnum; i++)
        {
            unsigned long tmp = CalcInterval(&pd->FrameTimeStp[i]);
            enc_l += tmp;
            if(tmp > max_l) {
                max_l = tmp;
            }
            if(tmp < min_l) {
                min_l = tmp;
            }
        }
        if (codecnum != 0)
            enc_avg = enc_l * 1.0 / codecnum;
        if (enc_l == 0)
            enc_l = 1;

        for(unsigned int i = 0; i< pd->Enduration.size(); i++) {
            enduration += CalcInterval(&(pd->Enduration)[i]);
        }
        for(unsigned int i = 0; i< pd->InitStp.size(); i++) {
            initialization += CalcInterval(&(pd->InitStp)[i]);
        }
        first_frame_latency = CalcInterval(&(pd->FrameTimeStp[0]));
        snprintf(tmp_r, sizeof(tmp_r), "<ENC> %d\n\
                ---------------------------\n\
                Codec               = %s\n\
                EncodedNum          = %ld\n\
                InitLatency         = %2ld.%3ldms\n\
                AvgEncLatency       = %2ld.%3ldms\n\
                EncoderFPS          = %.2f\n",
                pd->pinfo.mChannelNum,
                pd->pinfo.mCodecName.c_str(),
                codecnum,
                initialization / 1000, initialization % 1000,
                enc_avg / 1000, enc_avg % 1000,
                1000000.0 * codecnum / enc_l);
        res += tmp_r;
        init_enc += initialization;

        codecdata* pd_dec = NULL;
        if(mDecGrp.size() == 1) { //1:N use case
            pd_dec = &mDecGrp[0];
        }
        else if(mDecGrp.size() >= mEncGrp.size()) {
            pd_dec = &mDecGrp[n];
        }
        else {
            snprintf(tmp_r, sizeof(tmp_r), "Number of decoder is not match with encoder.");
            res += tmp_r;
            break;
        }

        if( mDecGrp.size() > 1 && mEncGrp.size() == 1) { //N:1 use case
            unsigned long earliest_fst = mDecGrp[0].InitStp[0].start_t;
            for(unsigned int n = 1; n < mDecGrp.size(); n++) {
                codecdata *pdec = &mDecGrp[n];
                if(earliest_fst > pdec->InitStp[0].start_t)
                    earliest_fst = pdec->InitStp[0].start_t;
            }
            first_frame_latency = pd->FrameTimeStp[0].finish_t - earliest_fst;
            max_l = min_l = first_frame_latency;
            pipe_l += first_frame_latency; //init latency included
            for(unsigned long i = 1; i< codecnum; i++) {
                unsigned long earliest = 0xffffffffffffffff;
                for(unsigned int n = 0; n < mDecGrp.size(); n++) { //find the earliest input frame in this group
                    codecdata *pdec = &mDecGrp[n];
                    if(pdec->FrameTimeStp.size() >= i+1) { //this decoder frame is not EOF
                        if(earliest > pdec->FrameTimeStp[i].start_t)
                            earliest = pdec->FrameTimeStp[i].start_t;
                    }
                }
                unsigned long tmp = pd->FrameTimeStp[i].finish_t - earliest;
                pipe_l += tmp;
                if( tmp > max_l ) {
                    max_l = tmp;
                }
                if(tmp < min_l) {
                    min_l = tmp;
                }
            }
            pipe_l = pipe_l / codecnum;
        }
        else { //1:N or N:N
            first_frame_latency = pd->FrameTimeStp[0].finish_t - pd_dec->InitStp[0].start_t;
            codecnum = (codecnum < pd_dec->FrameTimeStp.size())?codecnum:pd_dec->FrameTimeStp.size();
            max_l = min_l = first_frame_latency;
            pipe_l += first_frame_latency; //init latency included
            for(unsigned long i = 1; i< codecnum; i++)
            {
                unsigned long tmp = pd->FrameTimeStp[i].finish_t - pd_dec->FrameTimeStp[i].start_t;
                pipe_l += tmp;
                if( tmp > max_l ) {
                    max_l = tmp;
                }
                if(tmp < min_l) {
                    min_l = tmp;
                }
            }
            pipe_l = pipe_l / codecnum;
        }
        if (!enduration)
            enduration = 1;
        snprintf(tmp_r, sizeof(tmp_r), "<Pipeline> %d\n\
                ---------------------------\n\
                FPS                 = %.2f\n\
                FirstFrameLatency   = %2ld.%3ldms\n\
                MaxFrameLatency     = %2ld.%3ldms\n\
                MinFrameLatency     = %2ld.%3ldms\n\
                AvgFrameLatency     = %2ld.%3ldms\n",
                pd->pinfo.mChannelNum,
                1000000.0 * codecnum / enduration,
                first_frame_latency / 1000, first_frame_latency % 1000,
                max_l / 1000, max_l % 1000,
                min_l / 1000, min_l % 1000,
                pipe_l / 1000, pipe_l % 1000);
        res += tmp_r;
    }
}

