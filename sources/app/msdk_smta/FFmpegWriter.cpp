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

#include "FFmpegWriter.h"
#include "sample_defs.h"
#include "common_defs.h"
#include "sample_utils.h"
#include <vector>
#include <memory>
#include <stdio.h>

using namespace TranscodingSample;
struct MuxChain {
    AVFormatContext *mpegts_ctx;
    AVFormatContext *rtp_ctx;
};

FFmpegWriter::FFmpegWriter(OutputType type, string url, bool bDump) :
            m_bStart(false),
            m_nBufferSize(4*1024*1024),m_DumpFile(NULL) {
    m_bInited = false;
    m_pInputFormatCtx = NULL;
    m_pOutputFormatCtx = NULL;
    m_nVideoIndexIn = -1;
    m_nVideoIndexOut =-1;
    m_nFrameIndex = 0;
    m_bDump = bDump;
    m_strDstURI = url;
    m_eOutputType = type;
    m_inputStream = NULL;
    m_avioBuffer = NULL;
}

FFmpegWriter::~FFmpegWriter()
{
    Close();
    m_bDump = false;
}

mfxStatus FFmpegWriter::Close()
{
    if (m_bInited) {
        if (m_pInputFormatCtx) {
            avformat_close_input(&m_pInputFormatCtx);
            m_pInputFormatCtx = NULL;
        }

        if (NULL != m_pOutputFormatCtx) {
            av_write_trailer(m_pOutputFormatCtx);
            // Free the streams
            for (unsigned int i = 0; i < m_pOutputFormatCtx->nb_streams; i++) {
                av_freep(&m_pOutputFormatCtx->streams[i]->codec);
                av_freep(&m_pOutputFormatCtx->streams[i]);
            }
            avio_close(m_pOutputFormatCtx->pb);
            av_free(m_pOutputFormatCtx);
        }
        if (m_DumpFile) {
            fclose(m_DumpFile);
            m_DumpFile = NULL;
        }
        // if (m_avioBuffer)
        //     av_free(m_avioBuffer);
    }

    m_bInited = false;
    return MFX_ERR_NONE;
}
/**
 * get xlibcode encoded bitstream to ffmpeg
 */
static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    int len = 0;
    if (!opaque) {
        DEBUG_ERROR((_T("FFmpegWriter reader packet stream is null\n")));
        return -1;
    }

    FFmpegWriter *thiz = (FFmpegWriter*) opaque;
    // get memory handle
    Stream* stream = thiz->m_inputStream;

    do {
        // if input stream EOS and memory data size = 0
        if (stream->GetEndOfStream() && 0 == stream->GetBlockCount(NULL) ) {
            DEBUG_ERROR((_T("%s: Stream EOS !!!\n"), __FUNCTIONW__));
            return AVERROR_EOF;
        }

        len = stream->ReadBlockEx(buf, buf_size, false);
        if (len == 0) {
            usleep(500);
        }
    } while(len == 0);

    if (0 == len || len > buf_size) {
        return 0;
    }
    return len;
}

/**
 * initial muxer parameters by encode video parameters
 */
//int FFmpegWriter::InitMuxerParams(mfxBitstream *pMfxBitstream, MuxerParam &muxerParam) {
int FFmpegWriter::InitMuxerParams(Stream* stream, MuxerParam &muxerParam) {

    //mfxStatus sts = MFX_ERR_NONE;
    Close();

    DEBUG_MSG((_T("output file %s \n"), m_strDstURI.c_str()));
    //m_pMfxBitstream = pMfxBitstream;
    m_inputStream = stream;

    // muxer to rtsp or TS
    string streamFormat, inputVideoFormat;
    AVDictionary *opts = NULL;
    AVCodecID inputCodecID = AV_CODEC_ID_NONE;
    string rtspTransport = "rtsp_transport";
    string tcp = "tcp";
    switch (m_eOutputType) {
        /*
        case E_RTSP:
            streamFormat = "rtsp";
            if (av_dict_set(&opts, rtspTransport.c_str(), tcp.c_str(), 0) < 0) {
                DEBUG_ERROR(("set rtsp use TCP socket failed!\n"));
                return -1;
            }
            DEBUG_MSG((_T("set rtsp use TCP socket \n")));
            break;
            */
        case E_RTP_ES:
            streamFormat = "rtp";
            if (av_dict_set(&opts, "rtpflags", "send_bye", 0) < 0) {
                DEBUG_ERROR(("set rtpflags send_by failed!\n"));
                return -1;
            }
            /*if (av_dict_set(&opts, "rtp", tcp.c_str(), 0) < 0) {
                DEBUG_ERROR(("set rtp use TCP socket failed!\n"));
                return -1;
            }*/
            DEBUG_MSG((_T("set rtsp use TCP socket \n")));
            break;
        case E_RTP_TS:
            streamFormat = "rtp_mpegts";
            DEBUG_MSG((_T("set rtsp use TCP socket \n")));
            break;
        case E_FILE:
        default:
            streamFormat = "mpegts";
            break;
    }

    if (av_dict_set(&opts, "probesize", "20000000", 0) < 0) {
        DEBUG_ERROR((_T("set rtsp buffer size failed!\n")));
        return -1;
    }

    // set video codec
    if (muxerParam.nEncodeId == MFX_CODEC_AVC) {
        inputVideoFormat = "h264";
        inputCodecID = AV_CODEC_ID_H264;
    } else if (muxerParam.nEncodeId == MFX_CODEC_MPEG2) {
        inputVideoFormat = "mpegvideo";
        inputCodecID = AV_CODEC_ID_MPEG2VIDEO;
    } else if (muxerParam.nEncodeId == MFX_CODEC_HEVC) {
        inputVideoFormat = "hevc";
        inputCodecID = AV_CODEC_ID_HEVC;
    } else {
        DEBUG_MSG((_T("set video codec unknown \n")));
        return -1;
    }
    DEBUG_MSG((_T("set video codec: %s\n"), inputVideoFormat.c_str()));

    AVInputFormat *inputFormat = NULL;
    inputFormat = av_find_input_format(inputVideoFormat.c_str());
    if (!inputFormat) {
        DEBUG_ERROR(("Find AVInputFormat based on the short name of the input format failed!\n"));
        return -1;
    }
    // allocate the input media context
    m_pInputFormatCtx = avformat_alloc_context();
    if (!m_pInputFormatCtx) {
        DEBUG_ERROR(("allocate the input media context failed!\n"));
        return -1;
    }
    // Allocate the input media context
    AVIOContext *pb = NULL;

    m_avioBuffer = (unsigned char*) av_malloc(m_nBufferSize);
    if (!m_avioBuffer) {
        DEBUG_ERROR(("avio buffer alloc failed\n"));
        return -1;
    }
    pb = avio_alloc_context(m_avioBuffer, m_nBufferSize, 0, (void*) this, &read_packet, NULL, NULL);
    if (!pb) {
        DEBUG_ERROR(("avio context alloc failed\n"));
        return -1;
    }
    m_pInputFormatCtx->pb = pb;
    m_pInputFormatCtx->flags|=AVFMT_NOFILE|AVFMT_FLAG_IGNIDX;
    m_pInputFormatCtx->flags&=~AVFMT_FLAG_GENPTS;
    if (avformat_open_input(&m_pInputFormatCtx, "", inputFormat, &opts) < 0) {
        DEBUG_ERROR(("muxer open input failed\n"));
        return -1;
    }

    // set codeced video parameters
    AVCodecContext *c = m_pInputFormatCtx->streams[0]->codec;
    c->bit_rate = muxerParam.nBitRate * 1000;
    c->width = muxerParam.nWidth;
    c->height = muxerParam.nHeight;
    c->codec_id = inputCodecID;
    // time base: this is the fundamental unit of time (in seconds) in terms of which frame timestamps are represented. for fixed-fps content,
    //            timebase should be 1/framerate and timestamp increments should be identically 1
    c->time_base.den = muxerParam.nTimebaseNum;
    c->time_base.num = muxerParam.nTimebaseDen;
    // Codec "extradata" conveys the H.264 stream SPS and PPS info (MPEG2: sequence header is housed in SPS buffer, PPS buffer is empty)
    c->extradata = muxerParam.pExtraData;
    c->extradata_size = muxerParam.extraDataSize;
    c->pix_fmt = (enum AVPixelFormat) muxerParam.nPiexlFomat;
    for (unsigned int i = 0; i < m_pInputFormatCtx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        if (m_pInputFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVStream *in_stream = m_pInputFormatCtx->streams[i];
            in_stream->r_frame_rate.num = muxerParam.nTimebaseNum;
            in_stream->r_frame_rate.den = muxerParam.nTimebaseDen;
        }
    }
    avcodec_parameters_from_context(m_pInputFormatCtx->streams[0]->codecpar, m_pInputFormatCtx->streams[0]->codec);

    // dump codecd information
    DEBUG_MSG(("=============FFmpegWriter input stream format===========================\n"));
    av_dump_format(m_pInputFormatCtx, 0, NULL, 0);
    DEBUG_MSG(("========================================================================\n\n"));

    // Allocate the output media context
    avformat_alloc_output_context2(&m_pOutputFormatCtx, NULL, streamFormat.c_str(), m_strDstURI.c_str());
    if (!m_pOutputFormatCtx) {
        DEBUG_ERROR(("output media context failed!\n"));
        return -1;
    }

    // get output format
    AVOutputFormat *outputFormat = m_pOutputFormatCtx->oformat;
    if (!outputFormat) {
        DEBUG_ERROR(("output format %s failed!\n", streamFormat.c_str()));
        return -1;
    }

    for (unsigned int i = 0; i < m_pInputFormatCtx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        if (m_pInputFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVStream *in_stream = m_pInputFormatCtx->streams[i];
            //AVStream *out_stream = avformat_new_stream(m_pOutputFormatCtx, in_stream->codec->codec);
            AVStream *out_stream = avformat_new_stream(m_pOutputFormatCtx, NULL);
            m_nVideoIndexIn = i;
            if (!out_stream) {
                DEBUG_ERROR(("Failed allocating output stream\n"));
                return -1;
            }
            m_nVideoIndexOut = out_stream->index;
            //Copy the settings of AVCodecContext
            if (avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar) < 0) {
                DEBUG_ERROR(("Failed to copy context from input to output stream codec context\n"));
                return -1;;
            }
            out_stream->codecpar->codec_tag = 0;
            if (m_pOutputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
                out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }
            // for interlaced frame, FPS * 2
            if (muxerParam.bInterlaced) {
                in_stream->r_frame_rate.num *= 2;
            }
            break;
        }
    }
    // Output Information
    DEBUG_MSG((_T("=============FFmpegWriter output stream format===========================\n")));
    av_dump_format(m_pOutputFormatCtx, 0, m_strDstURI.c_str(), 1);
    DEBUG_MSG((_T("========================================================================\n\n")));

    // Open the output container file
    if (!(m_pOutputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&m_pOutputFormatCtx->pb, m_strDstURI.c_str(), AVIO_FLAG_WRITE);
        if (0 > ret) {
            DEBUG_ERROR(("could not open out file %s\n", m_strDstURI.c_str()));
            return -1;
        }
    }

    // For rtp_mpegts, need to add send_bye to RTP output mannually and must after
    // Writer container header
    if (0 == streamFormat.compare("rtp_mpegts")) {
        av_dict_set(&opts, "rtpflags", "send_bye", 0);
    }
    // Write container header
    if (avformat_write_header(m_pOutputFormatCtx, &opts) < 0) {
        DEBUG_ERROR(("could not open out file %s\n", m_strDstURI.c_str()));
        return -1;
    }

    if (m_bDump) {
        char m2t_dump[255];
        static int nIndex = 0;
        snprintf(m2t_dump, sizeof(m2t_dump), "output_%d.m2t", nIndex++);
        DEBUG_FORCE((_T("Opening muxed output dump: ")_T(PRIaSTR)_T("\n"), m2t_dump));
        m_DumpFile = fopen(m2t_dump, "wb+");
        if (NULL == m_DumpFile) {
            DEBUG_ERROR((_T("Failed to open M2T dump file [%d]. \n"), nIndex));
        }
    }

    m_bInited = true;
    return 0;
}

/**
 * write output file or stream
 */
mfxStatus FFmpegWriter::ProcessOutputBitstream() {
    int ret = 0;
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);

    m_bStart = true;
    int stream_index = m_nVideoIndexOut;
    AVStream *in_stream, *out_stream;
    AVPacket pkt;

    // read memory data to ffmpeg
    do {
        av_init_packet(&pkt);
        ret = av_read_frame(m_pInputFormatCtx, &pkt);
        if (AVERROR_EOF == ret) {
            av_packet_unref(&pkt);
            return MFX_ERR_NONE;
        }
        if (ret < 0 && AVERROR_EOF != ret) {
            DEBUG_ERROR((_T("FFmpegWriter av_read_frame failed!\n")));
            av_packet_unref(&pkt);
            return MFX_ERR_UNKNOWN;
        }

        if (m_DumpFile) {
            if (1 != fwrite(pkt.data, pkt.size, 1, m_DumpFile)) {
                DEBUG_ERROR((_T("Failed to write to M2T dump file \n")));
            }
            fflush(m_DumpFile);
        }

        in_stream = m_pInputFormatCtx->streams[pkt.stream_index];
        out_stream = m_pOutputFormatCtx->streams[stream_index];

        if (pkt.stream_index == m_nVideoIndexIn) {
            // recalculate pts, dts ,duration
            AVRational time_base1 = in_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            //Parameters
            pkt.pts = (double) (m_nFrameIndex * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
            m_nFrameIndex++;
        }
        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index = stream_index;

        //Write package using ffmpeg
        if ((ret = av_interleaved_write_frame(m_pOutputFormatCtx, &pkt)) < 0) {
            DEBUG_ERROR((_T("Error muxing packet\n")));
            av_packet_unref(&pkt);
            return MFX_ERR_UNKNOWN;
        }
        av_packet_unref(&pkt);
    } while (ret >= 0);

    return MFX_ERR_NONE;
}

mfxStatus FFmpegWriter::Resume(void)
{
    return MFX_ERR_MORE_DATA;
}

