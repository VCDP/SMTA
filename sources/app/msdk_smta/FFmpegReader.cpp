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

#include "FFmpegReader.h"
#include "sample_defs.h"

using namespace TranscodingSample;
const size_t BUFFER_SIZE = (4 * 1024 *1024);

FFmpegReader::FFmpegReader() :
            codec_id(AV_CODEC_ID_NONE),
            m_bIsRTSP(false),
            m_bIsLoopMode(false),
            m_bIsRunning(false){
    m_nVideoIndex = -1;
    m_nTimeOut = 5;
    m_nStartTime = 0;
    m_pAvFormatContext = nullptr;
    m_pVideoStream = nullptr;
    m_pVideoBSF = nullptr;
    m_strInputURI.clear();

    m_nCurrentLoopNum = 0;
    m_nLoopNum = 0;
    m_timediff = 0;
    m_bInputByFPS = false;
    m_prevTime = 0;
    m_thread.reset(nullptr);
}

FFmpegReader::~FFmpegReader() {
    close();
    if (nullptr != m_pDumpFile) {
        fclose(m_pDumpFile);
        m_pDumpFile = nullptr;
    }
}

/**
 * initial FFmpegReader and open input file
 */
mfxStatus FFmpegReader::open(const string &strFileName, const int codecId, const bool bIsLoop, const mfxU16 loopNum, const bool bInputByFPS) {
    // check input file name
    MSDK_CHECK_ERROR(strFileName.length(), 0, MFX_ERR_NOT_INITIALIZED);

    // set ffmpeg initial demux params
    m_strInputURI = strFileName;
    if (!m_strInputURI.compare(0, 7, "rtsp://")) {
        m_bIsRTSP = true;
    }
    if (m_bIsRTSP)
        m_stream.Open();
    else
        m_mempool.init(BUFFER_SIZE);

    //input data in loop mode
    m_bIsLoopMode = bIsLoop;
    m_nLoopNum = loopNum;
    m_bInputByFPS = bInputByFPS;

    switch(codecId) {
        case MFX_CODEC_AVC:
            codec_id = AV_CODEC_ID_H264;
            break;
        case MFX_CODEC_HEVC:
            codec_id = AV_CODEC_ID_HEVC;
            break;
        case MFX_CODEC_MPEG2:
            codec_id =  AV_CODEC_ID_MPEG2VIDEO;
            break;
        default:
            DEBUG_ERROR((_T("CodecId Invalid: %d !\n"), codecId));
            codec_id =  AV_CODEC_ID_NONE;
            return MFX_ERR_UNKNOWN;
    }

    if (openInputFile() < 0) {
        close();
        m_thread.reset();
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

/**
 * open input file
 */
int FFmpegReader::openInputFile() {
    int ret = 0;

    // get current time, it is used interruptCallback
    m_nStartTime = time(nullptr);

    // set rtsp use TCP socket
    AVDictionary *opts = nullptr;
    /* it seems that no work for vlc VOD server*/
    //if (!m_strInputURI.compare(0, 7, "rtsp://")) {
    if (m_bIsRTSP) {
        /*if (av_dict_set(&opts, "rtsp_transport", "tcp", 0) < 0) {
            DEBUG_ERROR((_T("set rtsp use TCP socket failed!\n")));
            return -1;
        }*/
        if (av_dict_set(&opts, "reorder_queue_size", "2000", 0) < 0) {
            DEBUG_ERROR((_T("set rtsp reorder queue size failed!\n")));
            return -1;
        }
        if (av_dict_set(&opts, "buffer_size", "10000000", 0) < 0) {
            DEBUG_ERROR((_T("set rtsp buffer size failed!\n")));
            return -1;
        }
    }

    if (av_dict_set(&opts, "probesize", "8000000", 0) < 0) {
        DEBUG_ERROR((_T("set rtsp buffer size failed!\n")));
        return -1;
    }

    // Allocate an AVFormatContext
    m_pAvFormatContext = avformat_alloc_context();
    if (!m_pAvFormatContext) {
        DEBUG_ERROR((_T("AVFormat alloc context is nullptr\n")));
        return -1;
    }

    // set interrupt callback function
    m_pAvFormatContext->interrupt_callback.callback = interruptCallback;
    m_pAvFormatContext->interrupt_callback.opaque = this;

    // Open an input stream
    //m_pAvFormatContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
    m_pAvFormatContext->flags|=AVFMT_NOFILE|AVFMT_FLAG_IGNIDX;
    m_pAvFormatContext->flags&=~AVFMT_FLAG_GENPTS;
    DEBUG_MSG((_T("FFmpegReader open input file name = %s\n"), m_strInputURI.c_str()));
    ret = avformat_open_input(&m_pAvFormatContext, m_strInputURI.c_str(), nullptr, &opts);
    if (ret < 0) {
        DEBUG_ERROR((_T("Could not open input file. error id is %d\n"), ret));
        return -1;
    }
    // Read packets of a media file to get stream information
    if (avformat_find_stream_info(m_pAvFormatContext, 0) < 0) {
        DEBUG_ERROR((_T("Failed to retrieve input stream information \n")));
        return -1;
    }

    // find video stream
    AVCodecParameters* avCodecParameters = nullptr;
    for (unsigned int i = 0; i < m_pAvFormatContext->nb_streams; i++) {
        if (m_pAvFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_nVideoIndex = i;
            m_pVideoStream = m_pAvFormatContext->streams[i];
            avCodecParameters = m_pAvFormatContext->streams[i]->codecpar;
        }
    }
    if (nullptr == avCodecParameters) {
        DEBUG_ERROR((_T("Didn't find video stream!\n")));
        return -1;
    }
    if (codec_id != avCodecParameters->codec_id) {
        printf("Video codec is not match the value for decoder!\n");
        DEBUG_ERROR((_T("Video codec is not match the value for decoder!\n")));
        if (nullptr != listener) {
            listener->onError();
        }
        return -1;
    }

    // for h264 m_pVideoBSF
    if (avCodecParameters->extradata && avCodecParameters->extradata_size > 0 && avCodecParameters->extradata[0] == 1) {
        DEBUG_MSG((_T("FFmpegReader open videoBSF\n")));
        const AVBitStreamFilter *filter = NULL;
        if (avCodecParameters->codec_id == AV_CODEC_ID_H264) {
            filter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (avCodecParameters->codec_id == AV_CODEC_ID_HEVC) {
            filter = av_bsf_get_by_name("hevc_mp4toannexb");
        }

        if (!filter)
            return -1;
        ret = av_bsf_alloc(filter, &m_pVideoBSF);
        if (ret < 0) {
            DEBUG_ERROR((_T("FFmpegReader alloc BSF failed.\n")));
            return ret;
        }
        ret = avcodec_parameters_copy(m_pVideoBSF->par_in, avCodecParameters);
        if (ret < 0)
            return ret;
        ret = av_bsf_init(m_pVideoBSF);
        if (ret < 0) {
            DEBUG_ERROR((_T("FFmpegReader init BSF failed.\n")));
            return -1;
        }
    }

    m_pAvFormatContext->max_delay *= 4;

    av_dict_free(&opts);

    DEBUG_ERROR((_T("==========================FFmpegReader input stream format===========================\n")));
    av_dump_format(m_pAvFormatContext, 0, m_strInputURI.c_str(), 0);
    DEBUG_ERROR((_T("=====================================================================================\n\n")));

    if (!m_bIsRTSP && m_bInputByFPS && !m_timediff) {
        AVRational rate = getFrameRate();
        mfxF64 fFrameRate = (rate.den != 0) ? ((mfxF64)((int)(rate.num/(double)rate.den * 100 + 0.5)) / 100) : 0;
        if (fFrameRate == 0) {
            fFrameRate = 30.0;
        }
        m_timediff = (int)(1 * 1000000/fFrameRate);
    }
    return 0;
}
/**
 * reload Source file for loop mode
 */
int FFmpegReader::reloadSource()
{
    int ret = 0;

    if (m_pVideoBSF) {
        av_bsf_free(&m_pVideoBSF);
        m_pVideoBSF = nullptr;
    }

    // close an opened input AVFormatContext. Free it and all its contents
    if (m_pAvFormatContext) {
        avformat_close_input(&m_pAvFormatContext);
        m_pAvFormatContext = nullptr;
    }
    m_pVideoStream = nullptr;
    m_nVideoIndex = -1;
    ret = openInputFile();
    if (ret < 0) {
        DEBUG_ERROR((_T("FFmpegReader open input file failed.\n")));
        return -1;
    }

    return 0;
}
mfxStatus FFmpegReader::PushData(void* buffer, int size)
{
    int pktSize = size;
    int writedSize = 0;

    while(pktSize > 0) {
        if (m_bIsRTSP) {
            m_stream.WriteBlockEx(buffer, size, true, 0, 0, 0);
            pktSize = 0;
            usleep(2000);
        } else {
            int mempool_freeflat = m_mempool.GetFreeFlatBufSize();
            unsigned char *mempool_wrptr = m_mempool.GetWritePtr();
            int copySize = mempool_freeflat > pktSize ? pktSize : mempool_freeflat;

            if (!mempool_freeflat) {
                usleep(100);
                continue;
            }
            memcpy(mempool_wrptr, (void*)(buffer + writedSize), copySize);
            writedSize += copySize;
            pktSize -= copySize;
            m_mempool.UpdateWritePtrCopyData(copySize);
        }
    }
    return MFX_ERR_NONE;
}
/**
 * read input file and copy data to xlibcode
 */
mfxStatus FFmpegReader::readNextFrame(void)
{
    // for h264 or h265
    int nalSize = 0;
    char nal[] = {0,0,0,0};
    AVPacket pkt;
    av_init_packet(&pkt);
    if (m_pVideoStream->codecpar->codec_id == AV_CODEC_ID_H264 || m_pVideoStream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
            nalSize = 4;
            nal[0] = nal[1] = nal[2] = 0;
            nal[3] = 1;
    }

    AVCodecParameters *vCodec = m_pAvFormatContext->streams[m_nVideoIndex]->codecpar;
    static int needHeader = 1;
    if (needHeader && vCodec && vCodec->extradata != nullptr) {
        PushData(vCodec->extradata, vCodec->extradata_size);
        needHeader = 0;
    }

    // get current time, it is used interruptCallback
    m_nStartTime = time(nullptr);

    int ret = av_read_frame(m_pAvFormatContext, &pkt);
    if ( 0 <= ret) {
        if (pkt.stream_index == m_nVideoIndex) {
            if (pkt.size < 0) {
                printf("End of stream \n");
                //return MFX_ERR_MORE_DATA;
            } else {
                if (m_pVideoStream->codecpar->codec_id == AV_CODEC_ID_H264 || m_pVideoStream->codecpar->codec_id == AV_CODEC_ID_HEVC) {
                    if (m_pVideoBSF) {
                        ret = av_bsf_send_packet(m_pVideoBSF, &pkt);
                        if (ret < 0) {
                            av_packet_unref(&pkt);
                            DEBUG_ERROR((_T("BSF send filter failed!\n")));
                            return MFX_ERR_UNKNOWN;
                        }
                        ret = av_bsf_receive_packet(m_pVideoBSF, &pkt);
                        if (ret < 0) {
                            av_packet_unref(&pkt);
                            DEBUG_ERROR((_T("BSF receive filter failed!\n")));
                            return MFX_ERR_UNKNOWN;
                        }
                    } else {
                        if (strncmp((const char *)pkt.data, nal, nalSize) && strncmp((const char*)pkt.data, (nal + 1), (nalSize - 1))) {
                            memcpy(pkt.data, nal, nalSize);
                        }
                    }
                }
                PushData(pkt.data, pkt.size);

                //calculate time that send data.
                if (!m_bIsRTSP && m_bInputByFPS) {
                    struct timeval tv;
                    gettimeofday(&tv, NULL);
                    int64_t curTime = tv.tv_sec * 1000000 + tv.tv_usec;

                    if (m_prevTime && (curTime - m_prevTime < m_timediff)) {
                        int64_t dValue = m_timediff - (curTime - m_prevTime);

                        if (dValue > 0)
                            usleep(dValue);
                    }
                    m_prevTime = curTime;

                }
            }
        }
    } else {

        if (AVERROR_EOF == ret) {
            printf("FFMPEG DEMUX EOS\n");
            av_packet_unref(&pkt);;
            if (m_bIsLoopMode && (m_nCurrentLoopNum < m_nLoopNum) ) {
                if (reloadSource() < 0) {
                    DEBUG_ERROR((_T("FFmpegReader reloads input file failed!\n")));
                    m_stream.SetEndOfStream();
                    m_mempool.SetDataEof();
                    return MFX_ERR_UNKNOWN;
                }
                if (m_nLoopNum != UINT16_MAX)
                    m_nCurrentLoopNum++;

            } else {
                m_stream.SetEndOfStream();
                m_mempool.SetDataEof();
                m_bIsRunning = false;
            }
            return MFX_ERR_NONE;
        } else {
            DEBUG_ERROR((_T("FFmpegReader av_read_frame failed!\n")));
            av_packet_unref(&pkt);
            m_stream.SetEndOfStream();
            m_mempool.SetDataEof();
            m_bIsRunning = false;
            return MFX_ERR_UNKNOWN;
        }
    }
    av_packet_unref(&pkt);
    return MFX_ERR_NONE;
}

/**
 * release FFmpeg resource
 */
mfxStatus FFmpegReader::close(void)
{
    m_stream.SetEndOfStream();
    m_mempool.SetDataEof();
    if (nullptr != m_thread) {
        m_thread->Close();
    }

    if (m_pVideoBSF) {
        //av_bitstream_filter_close(m_pVideoBSF);
        av_bsf_free(&m_pVideoBSF);
        m_pVideoBSF = nullptr;
    }

    // close an opened input AVFormatContext. Free it and all its contents
    if (m_pAvFormatContext) {
        avformat_close_input(&m_pAvFormatContext);
        m_pAvFormatContext = nullptr;
    }
    m_pVideoStream = nullptr;
    m_nVideoIndex = -1;

    printf("FFmpegReader closed.\n");
    return MFX_ERR_NONE;
}

/**
 * Not wait if timeout
 */
int FFmpegReader::interruptCallback(void *ctx) {

    FFmpegReader *demuxer = reinterpret_cast<FFmpegReader*>(ctx);
    if (demuxer->isTimeOut()) {
        // not wait
        return 1;
    }
    // wait
    return 0;
}

/**
 * check timeout
 */
bool FFmpegReader::isTimeOut() {
    time_t now = time(nullptr);
    if ((now - m_nStartTime) > m_nTimeOut) {
        printf(" FFmpegReader call InterruptCallback\n");
        return true;
    }
    return false;
}

/**
 * resume stream
 */
mfxStatus FFmpegReader::resume() {
    close();

    if (openInputFile() < 0) {
        return MFX_ERR_UNKNOWN;
    }

    return MFX_ERR_NONE;
}

uint32_t FFmpegReader::ReadThread(void *pObj)
{
    FFmpegReader *pp = (FFmpegReader*)pObj;

    while (pp->threadIsRunning()) {
        mfxStatus sts = pp->readNextFrame();
        if (MFX_ERR_NONE != sts) {
            printf("Read Frame Error! sts = %d\n", sts);
            return sts;
        }
    }
    printf("Reading Thread Over!\n");
    return 0;
}

mfxStatus FFmpegReader::start()
{
#if 0
    if (m_bIsRTSP && reloadSource() < 0) {
        return MFX_ERR_UNKNOWN;
    }
#endif
    if (nullptr == m_thread) {
        m_bIsRunning = true;
        m_thread.reset(new XC::Thread());
        m_thread->Create(ReadThread, (void*)this);
    }
    return MFX_ERR_NONE;
}

mfxStatus FFmpegReader::stop(void) {
    m_bIsRunning = false;
    close();
    return MFX_ERR_NONE;
}
