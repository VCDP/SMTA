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

#include "SinkWrap.h"

#include <algorithm>
#include "common_defs.h"
#include "FFmpegBitstream.h"

#ifdef MSDK_AVC_FEI
#include "video/h264_fei/h264_fei_encoder_element.h"
#endif
#ifdef MSDK_HEVC_FEI
#include "video/h265_fei/h265_fei_encoder_element.h"
#endif

using namespace TranscodingSample;

#define TIME_TO_SLEEP 5
#define MAXSPSPPSBUFFERSIZE 1000

/**
 * output thread function
 */
uint32_t XC_THREAD_CALLCONVENTION ThreadWrap(void *pObj)
{
    SinkWrap::SinkPipeline *sp = (SinkWrap::SinkPipeline*)pObj;
    uint32_t sts = sp->OneSinkOutput();

    DEBUG_MSG((_T("%s: Leaving Sink Thread!!! TERMINATING!!!\n"), __FUNCTIONW__));
    return sts;
}

SinkWrap::SinkWrap() : m_nPipelineCnt(0)
{
    MSDK_ZERO_MEMORY(m_mapThread);
    m_bIsRunning = false;
    m_pCurrSink = NULL;
}

SinkWrap::~SinkWrap(void)
{
    Stop();
}

SinkWrap::SinkPipeline::SinkPipeline()
        : m_bThreadExit(false),
        m_OutPackets(0),
        m_nBufferSize(4*1024*1024)
{
    MSDK_ZERO_MEMORY(m_encInputParams);
    m_pStreamOutput = NULL;
}

/**
 * Get libxcode encoded information
 */
mfxStatus SinkWrap::SinkPipeline::OnFirstPutBS(mfxBitstream *pMfxBitstream)
{
    mfxVideoParam par;
    MSDK_ZERO_MEMORY(par);
    mfxStatus ret = MFX_ERR_NONE;
    std::vector<mfxExtBuffer*> pEncExtParams;

    mfxU8 SPSBuffer[MAXSPSPPSBUFFERSIZE];
    mfxU8 PPSBuffer[MAXSPSPPSBUFFERSIZE];
    mfxExtCodingOptionSPSPPS extSPSPPS;
#ifdef MFX_DISPATCHER_EXPOSED_PREFIX
    mfxU8 VPSBuffer[MAXSPSPPSBUFFERSIZE];
    mfxExtCodingOptionVPS extVPS;
#endif

    MSDK_ZERO_MEMORY(par);
    MSDK_ZERO_MEMORY(extSPSPPS);
    extSPSPPS.Header.BufferId   = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
    extSPSPPS.Header.BufferSz   = sizeof(mfxExtCodingOptionSPSPPS);
    extSPSPPS.PPSBuffer         = PPSBuffer;
    extSPSPPS.SPSBuffer         = SPSBuffer;
    extSPSPPS.PPSBufSize        = MAXSPSPPSBUFFERSIZE;
    extSPSPPS.SPSBufSize        = MAXSPSPPSBUFFERSIZE;
    pEncExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(&extSPSPPS));

#ifdef MFX_DISPATCHER_EXPOSED_PREFIX
    if (m_encInputParams->nEncodeId == MFX_CODEC_HEVC) {
        memset(&extVPS, 0, sizeof(extVPS));
        extVPS.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_VPS;
        extVPS.Header.BufferSz = sizeof(mfxExtCodingOptionVPS);
        extVPS.VPSBuffer = VPSBuffer;
        extVPS.VPSBufSize = MAXSPSPPSBUFFERSIZE;
        pEncExtParams.push_back(reinterpret_cast<mfxExtBuffer*>(&extVPS));

    } else {
        extVPS.VPSBufSize = 0;
    }
#endif

    par.ExtParam = &pEncExtParams.front();
    par.NumExtParam = (mfxU16)pEncExtParams.size();

#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    if (m_encInputParams->bPreEnc || m_encInputParams->bENC || m_encInputParams->bPAK || m_encInputParams->bEncode || m_encInputParams->bENCPAK) {
#ifdef MSDK_AVC_FEI
        if (m_encInputParams->nEncodeId == MFX_CODEC_AVC)
            ret = ((H264FEIEncoderElement *)(m_encInputParams->encHandle))->GetVideoParam(&par);
#endif
#ifdef MSDK_HEVC_FEI
        if (m_encInputParams->nEncodeId == MFX_CODEC_HEVC)
            ret = ((H265FEIEncoderElement *)(m_encInputParams->encHandle))->GetVideoParam(&par);
#endif
    } else
#endif
    {
        ret = ((EncoderElement *)(m_encInputParams->encHandle))->GetVideoParam(&par);
    }
    if (ret == 0) {
	//mfxU8 * pExtraData = NULL;
        BitstreamSink::MuxerParam muxerParam ;
        MSDK_ZERO_MEMORY(muxerParam);

#ifdef MFX_DISPATCHER_EXPOSED_PREFIX
        muxerParam.pExtraData = (mfxU8 *)new char[extSPSPPS.SPSBufSize + extSPSPPS.PPSBufSize + extVPS.VPSBufSize + 1]();
        muxerParam.extraDataSize = extSPSPPS.SPSBufSize + extSPSPPS.PPSBufSize + extVPS.VPSBufSize;
        memcpy(muxerParam.pExtraData, extVPS.VPSBuffer, extVPS.VPSBufSize);
        memcpy(muxerParam.pExtraData + extVPS.VPSBufSize, extSPSPPS.SPSBuffer, extSPSPPS.SPSBufSize);
        memcpy(muxerParam.pExtraData + extVPS.VPSBufSize + extSPSPPS.SPSBufSize, extSPSPPS.PPSBuffer, extSPSPPS.PPSBufSize);
#else
        muxerParam.pExtraData = (mfxU8 *)new char[extSPSPPS.SPSBufSize + extSPSPPS.PPSBufSize + 1]();
        muxerParam.extraDataSize = extSPSPPS.SPSBufSize + extSPSPPS.PPSBufSize;
        //memcpy(pExtraData, extVPS.VPSBuffer, extVPS.VPSBufSize);
        memcpy(muxerParam.pExtraData, extSPSPPS.SPSBuffer, extSPSPPS.SPSBufSize);
        memcpy(muxerParam.pExtraData + extSPSPPS.SPSBufSize, extSPSPPS.PPSBuffer, extSPSPPS.PPSBufSize);
#endif

        muxerParam.nBitRate = par.mfx.TargetUsage;
        muxerParam.nGropSize = par.mfx.GopPicSize;
        muxerParam.nWidth = par.mfx.FrameInfo.CropW;
        muxerParam.nHeight = par.mfx.FrameInfo.CropH;
        muxerParam.nTimebaseNum = par.mfx.FrameInfo.FrameRateExtN;
        muxerParam.nTimebaseDen = par.mfx.FrameInfo.FrameRateExtD;
        muxerParam.nPiexlFomat = ConvertPixelFormat(par.mfx.FrameInfo.FourCC);
        muxerParam.nEncodeId = par.mfx.CodecId;
        muxerParam.bInterlaced = !(MFX_PICSTRUCT_PROGRESSIVE == par.mfx.FrameInfo.PicStruct);

        if (m_pBSSink->InitMuxerParams(m_pStreamOutput, muxerParam) < 0) {
            return MFX_ERR_UNSUPPORTED;
        }
    }
    return ret;
}

/**
 * output thread
 */
uint32_t SinkWrap::SinkPipeline::OneSinkOutput()
{
    mfxStatus sts = MFX_ERR_NONE;

    if (NULL == m_pStreamOutput) {
        return MFX_ERR_UNKNOWN;
    }

    fprintf(stderr, "into  SinkWrap::SinkPipeline::OneSinkOutput\n");
    while (!m_bThreadExit) {
        if (m_OutPackets || (!m_OutPackets && m_pStreamOutput->GetBlockCount(NULL) > 5)) {
            // get FFmpeg Bitstream
            if (NULL == m_pBSSink.get()) {
                DEBUG_ERROR((_T("%s: Sink thread error!!!\n"), __FUNCTIONW__));
                break;
            }
            if (m_OutPackets == 0) { // First time only
                sts = OnFirstPutBS(NULL);
                if (sts < MFX_ERR_NONE) {
                    fprintf(stderr, "Get Video parameters failed\n");
                    m_pStreamOutput->SetEndOfStream();
                    break;
                }
            }
            sts = m_pBSSink->ProcessOutputBitstream();
            if (sts < MFX_ERR_NONE) {
                m_pStreamOutput->SetEndOfStream();
                fprintf(stderr, "Failed to handle output bitStream. sts = %d\n", sts);
                break;
            }
            m_OutPackets ++;
        }

        if (m_pStreamOutput->GetEndOfStream() && 0 == m_pStreamOutput->GetBlockCount(NULL)) {
            break;
        }
        XC::Sleep(TIME_TO_SLEEP);
    }
    if (m_pBSSink.get())
        printf("Encode stream EOS, output packets is %d\n", m_pBSSink->GetOutputPackets());

    // delete resources
    delete m_pStreamOutput;
    m_pStreamOutput = NULL;
    //WipeMfxBitstream(pMfxBitstream.get());
    //close output
    if (m_pBSSink.get())
        m_pBSSink->Close();
    printf("Stream Output Close\n");
    return sts;
}

/**
 * new FFmpegBitstreamSink and add sink pool
 */
mfxStatus SinkWrap::AddNewSink(EncParams &encInputParams) {
    mfxStatus sts = MFX_ERR_NONE;

    // new a sink pipeline
    m_pCurrSink = new SinkPipeline();
    BitstreamSink *pBSSink = bitstreamSinkFactory.getInstance(encInputParams.outputType,
                            encInputParams.strDstURI, encInputParams.bDump);
    if (pBSSink == NULL) {
        return MFX_ERR_NULL_PTR;
    }
    m_pCurrSink->m_pBSSink.reset(pBSSink);
    // set sink pipeline parameters
    m_pCurrSink->m_encInputParams = &encInputParams;
    m_pCurrSink->m_pStreamOutput = new Stream();
    if (NULL == m_pCurrSink->m_pStreamOutput) {
        return MFX_ERR_UNKNOWN;
    }
    m_pCurrSink->m_pStreamOutput->Open(4*1024*1024);
    //m_pCurrSink->m_pStreamOutput->Open("test.h265", true);
    // set now sink pipeline to pool
    m_pSink.push_back(m_pCurrSink);

    return sts;
}

/**
 * start output thread
 */
mfxStatus SinkWrap::Start(void) {
    mfxStatus sts = MFX_ERR_NONE;

    if (m_bIsRunning) {
        return sts;
    }
    // for each output sink pipeline
    for (std::vector<SinkPipeline*>::iterator it = m_pSink.begin(); it != m_pSink.end(); ++it) {
        if (m_nPipelineCnt >= MAX_PIPELINE) {
            DEBUG_ERROR((_T("Out of max output pipeline\n")));
            break;
        }
        SinkPipeline * sp = *it;
        XC::Thread *thread = new XC::Thread();
        if (NULL != thread) {
            thread->Create(ThreadWrap, (void*) sp);
            m_mapThread[m_nPipelineCnt].pipeline = sp;
            m_mapThread[m_nPipelineCnt].thread = thread;
            m_nPipelineCnt++;
        }
    }
    m_bIsRunning = true;
    return sts;
}

void SinkWrap::Stop(void)
{
    if (!m_bIsRunning) {
        return;
    }

    for (int i = 0; i < m_nPipelineCnt; i++) {
        SinkPipeline* sp = m_mapThread[i].pipeline;
        XC::Thread *thread = m_mapThread[i].thread;
        if (NULL != sp) {
            sp->m_bThreadExit = true;
        }
        if (NULL != thread) {
            thread->Close();
            delete thread;
            m_mapThread[i].thread = NULL;
        }
        delete sp;
        sp = NULL;
    }

    m_pSink.clear();
    m_bIsRunning = false;
}

void SinkWrap::Wait(void)
{
    for (int i = 0; i < m_nPipelineCnt; i++) {
        XC::Thread *thread = m_mapThread[i].thread;
        if (NULL != thread) {
            thread->Wait();
        }
    }
}

Stream* SinkWrap::SinkPipeline::GetStream(void) {
    return m_pStreamOutput;
}

Stream *SinkWrap::GetStream(void)
{
    if (NULL != m_pCurrSink) {
        return m_pCurrSink->GetStream();
    }
    return NULL;
}

