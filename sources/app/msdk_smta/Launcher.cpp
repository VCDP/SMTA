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

#ifndef _WIN32
#include <signal.h>
#endif

#include "Launcher.h"
#include "FFmpegBitstream.h"
#include "base/trace_file.h"
#include "base/trace_filter.h"
#include "ffmpeg_utils.h"

#include <sstream>

using namespace std;
using namespace TranscodingSample;

Launcher::Launcher() : m_StartTime(0),
    m_pBitstreamSource(new FFmpegReader),
    m_pVideoSink(new SinkWrap),
    m_pPipeline(new MsdkXcoder),
    logPrintThread(NULL),
    start_trace(false), start_log_print(false)
{
    m_mvoutStreamList.clear();
    m_mvinStreamList.clear();
    m_mbcodeoutStreamList.clear();
    m_weightStreamList.clear();
} // Launcher::Launcher()

Launcher::~Launcher()
{
    if (start_trace) {
        Trace::stop();
    }
    if(!m_mvoutStreamList.empty()){
        std::list<Stream*>::iterator it;
        for(it = m_mvoutStreamList.begin(); it != m_mvoutStreamList.end(); it++){
            delete *it;
        }
        m_mvoutStreamList.clear();
    }
    if(!m_mvinStreamList.empty()){
        std::list<Stream*>::iterator it;
        for(it = m_mvinStreamList.begin(); it != m_mvinStreamList.end(); it++){
            delete *it;
        }
        m_mvinStreamList.clear();
    }
    if(!m_mbcodeoutStreamList.empty()){
        std::list<Stream*>::iterator it;
        for(it = m_mbcodeoutStreamList.begin(); it != m_mbcodeoutStreamList.end(); it++){
            delete *it;
        }
        m_mbcodeoutStreamList.clear();
    }
    if(!m_weightStreamList.empty()){
        std::list<Stream*>::iterator it;
        for(it = m_weightStreamList.begin(); it != m_weightStreamList.end(); it++){
            delete *it;
        }
        m_weightStreamList.clear();
    }

} // Launcher::~Launcher()

bool Launcher::CheckNeedVPP(sVppParams &vppInputParams)
{
    int width = m_pBitstreamSource->getInputWidth();
    int height = m_pBitstreamSource->getInputHeight();
    AVRational rate = m_pBitstreamSource->getFrameRate();
    mfxF64 fFrameRate = (rate.den != 0) ? ((mfxF64)((int)(rate.num/(double)rate.den * 100 + 0.5)) / 100) : 0;

    if (fFrameRate == (mfxF64)0 && vppInputParams.dFrameRate == (mfxF64)0) {
        vppInputParams.dFrameRate = 30.0;
    }

    if ((vppInputParams.nDstHeight && vppInputParams.nDstHeight != height)
        || (vppInputParams.nDstWidth && vppInputParams.nDstWidth != width)
        || vppInputParams.bDenoiseEnabled
        || vppInputParams.bComposition
        || (vppInputParams.dFrameRate != (mfxF64)0 && vppInputParams.dFrameRate != fFrameRate)
        || vppInputParams.bEnableDeinterlacing
        || vppInputParams.nPicStruct)
        return true;

    return false;
}
mfxU16 Launcher::CalCuEncRequestSurface(EncParams &encInputParams)
{
    mfxU16 length = encInputParams.nLADepth ? encInputParams.nLADepth : encInputParams.bLABRC ? 40 : 0;
    length += encInputParams.nGopRefDist;
#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    if (encInputParams.bPreEnc) {
        length += encInputParams.nGopRefDist;
    }
    if (encInputParams.fei_ctrl.nDSstrength) {
        length += (encInputParams.nRefNum ? encInputParams.nRefNum :4);
    }
#endif
    return length;
}
//calculate suggestSurfaceNum for decoder if NoVpp pipeline exists.
mfxU16 Launcher::GetDecOuputSurfNum(InputParams &paramList)
{
    int enc_cnt = paramList.encParamsList.size();
    mfxU16 max = 0;
    for (int i = 0; i < enc_cnt; i++) {
        EncParams &encInputParams = paramList.encParamsList.at(i);
        sVppParams &vppInputParams = paramList.vppParamsList.at(i);
        mfxU16 length = 0;
        if (!CheckNeedVPP(vppInputParams)) {
            length = CalCuEncRequestSurface(encInputParams);
        }
        if (max < length)
            max = length;
    }

    return max;
}
mfxStatus Launcher::Init(InputParams &paramList)
{
    mfxStatus sts = MFX_ERR_NONE;
    int libType = paramList.bUseHW ? MFX_IMPL_HARDWARE_ANY : MFX_IMPL_SOFTWARE;

    // Set session tag for troubleshooting purposes (in case of many parallel sessions on same system)
    SetSessionName(paramList.strSessionName);

    string work_id = paramList.strWorkID;
    if (!work_id.empty()) {
        string log_file = "/tmp/" + work_id + "/smta/" +
                paramList.strSessionName + "_perf.log";

        logPrintThread = new LogPrintThread(&measurement);
        if (NULL != logPrintThread) {
            logPrintThread->Start(1000, log_file);
            start_log_print = true;
        }
    }

    if (paramList.bStatistics) {
          int loglevel = 3;  //default LEVEL is INFO
          TraceFile::instance()->init("trace.log");
          Trace::filter.enable_all((TraceLevel) loglevel);
          Trace::start();
          start_trace = true;
      }

    DEBUG_FORCE((_T("Begin to create session\n")));
#if (MFX_VERSION >= 1025)
    bool allMFEModesEqual=true;
    bool allMFEFramesEqual=true;
    mfxU16 usedMFEMaxFrames = 0;
    mfxU16 usedMFEMode = 0;

    for (mfxU32 i = 0; i < paramList.encParamsList.size(); i++)
    {
        // loop over all sessions and check mfe-specific params
        // for mfe is required to have sessions joined, HW impl
        if(paramList.encParamsList[i].nMFEFrames > 1)
        {
            usedMFEMaxFrames = paramList.encParamsList[i].nMFEFrames;
            for (mfxU32 j = 0; j < paramList.encParamsList.size(); j++)
            {
                if(paramList.encParamsList[j].nMFEFrames &&
                   paramList.encParamsList[j].nMFEFrames != usedMFEMaxFrames)
                {
                    paramList.encParamsList[j].nMFEFrames = usedMFEMaxFrames;
                    allMFEFramesEqual = false;
                    paramList.encParamsList[j].nMFEMode = paramList.encParamsList[j].nMFEMode < MFX_MF_AUTO
                      ? MFX_MF_AUTO : paramList.encParamsList[j].nMFEMode;
                }
            }
        }
        if(paramList.encParamsList[i].nMFEMode >= MFX_MF_AUTO)
        {
            usedMFEMode = paramList.encParamsList[i].nMFEMode;
            for (mfxU32 j = 0; j < paramList.encParamsList.size(); j++)
            {
                if(paramList.encParamsList[j].nMFEMode &&
                   paramList.encParamsList[j].nMFEMode != usedMFEMode)
                {
                    paramList.encParamsList[j].nMFEMode = usedMFEMode;
                    allMFEModesEqual = false;
                }
            }
        }
    }
    if(!allMFEFramesEqual)
        DEBUG_ERROR((_T("WARNING: All sessions for MFE should have the same number of MFE frames!\n used ammount of frame for MFE: %d\n"),  (int)usedMFEMaxFrames));
    if(!allMFEModesEqual)
        DEBUG_ERROR((_T("WARNING: All sessions for MFE should have the same mode!\n, used mode: %d\n"),  (int)usedMFEMode));
#endif

    // Set allocator type
    m_pPipeline->SetLibraryType(libType);
    // create session
    //int vpp_enc_cnt = paramList.vppParamsList.size();
    int vpp_enc_cnt = paramList.encParamsList.size();
    printf("%s: vpp and enc pair count is %d \n", __FUNCTIONW__, vpp_enc_cnt);

    for (int i = 0; i < vpp_enc_cnt; i++) {
        DecOptions dec_cfg;
        VppOptions vpp_cfg;
        EncOptions enc_cfg;
        bool have_vpp = false;

        DecParams &decInputParams = paramList.decParamsList.at(0);
        EncParams &encInputParams = paramList.encParamsList.at(i);
        sVppParams &vppInputParams = paramList.vppParamsList.at(i);

        // Configure output thread before VCSSA
         if(m_pVideoSink.get() != NULL) {
             sts = m_pVideoSink->AddNewSink(encInputParams);
             MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
         }

        if (0 == i) {
            sts = InitInputCfg(decInputParams, dec_cfg);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            mfxU16 length = GetDecOuputSurfNum(paramList);
            if (length > 0) {
                dec_cfg.have_NoVpp = true;
                dec_cfg.suggestSurfaceNum = length;
            }
        }
        // check Whether need VPP element
        have_vpp = CheckNeedVPP(vppInputParams);
        if (have_vpp) {
            sts = InitVppCfg(vppInputParams, vpp_cfg);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            mfxU16 length = CalCuEncRequestSurface(encInputParams);
            if (vpp_cfg.nSuggestSurface < length)
                vpp_cfg.nSuggestSurface = length;
        }
        sts = InitOutputCfg(encInputParams, enc_cfg);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        if (0 == i) {
            if (have_vpp) {
                if (0 != m_pPipeline->Init(&dec_cfg, &vpp_cfg, &enc_cfg)) {
                    DEBUG_ERROR((_T("MsdkXcoder initialize failed\n")));
                    return MFX_ERR_UNKNOWN;
                }
            } else {
                if (0 != m_pPipeline->Init(&dec_cfg, nullptr, &enc_cfg)) {
                    DEBUG_ERROR((_T("MsdkXcoder initialize failed\n")));
                    return MFX_ERR_UNKNOWN;
                }
            }

        } else {
            AttachNewTranscoder(m_pPipeline.get(), vpp_cfg, enc_cfg, have_vpp);
        }

        encInputParams.encHandle = enc_cfg.EncHandle;
         // generate blend pic config
         if (have_vpp && vppInputParams.bComposition) {
             BlendInfo blendInfo;
             AddPicBlendInfo(vppInputParams.compositionParams, vpp_cfg, blendInfo);
             AttachPicBlend(m_pPipeline.get(), 0, vpp_cfg.VppHandle, blendInfo);
         }

    }
    _tprintf(_T("\n"));

    return sts;

} // mfxStatus Launcher::Init()

void Launcher::Run()
{
    _tprintf(_T("Transcode: Transcoding started\n"));

    // mark start time
    m_StartTime = XC::GetTick();

    // Start MsdkXcoder pipeline
    if (!m_pPipeline->Start()) {
        DEBUG_VIDEO((_T("%s: pipeline.Run() failed\n"), __FUNCTIONW__));
        return;
    }

    //Start output thread
    if (m_pVideoSink.get() != NULL) {
        m_pVideoSink->Start();
    }

    if (m_pBitstreamSource.get())
        m_pBitstreamSource->start();

    m_pPipeline->Join();
    printf("Pipeline stopped!\n");

    // Wait for output catch EOS flag and run over
    m_pVideoSink->Wait();
    printf("Video Sink stopped!\n");

    if (start_log_print) {
        logPrintThread->Stop();
        start_log_print = false;
        delete logPrintThread;
    }

    DEBUG_FORCE((_T("%s: Transcoding finished\n"), __FUNCTIONW__));
    _tprintf(_T("\nTranscoding finished\n"));
} // mfxStatus Launcher::Init()

void Launcher::Stop() {
    // Stop this ,all will stop
    m_pBitstreamSource->stop();
    m_pPipeline->Stop();
    m_pVideoSink->Stop();

    printf("Stop by User %s\n", __FUNCTIONW__);
}

mfxStatus Launcher::ProcessResult(CmdProcessor &cmdParser)
{
    _tprintf(_T("\nCommon transcoding time is  %.2f sec \n"), XC::GetTime(m_StartTime));
    string strName = cmdParser.GetParFileName();
    if (!strName.empty()) {
        _tprintf(_T("Input par file: %s\n\n"), strName.c_str());
    }

    measurement.ShowPerformanceInfo();
    _tprintf(_T("\nThe test PASSED\n"));
    return MFX_ERR_NONE;
} // mfxStatus Launcher::ProcessResult()

// initialize DecOptions parameter
mfxStatus Launcher::InitInputCfg(const DecParams &decInputParams, DecOptions &dec_cfg) {
    mfxStatus sts = MFX_ERR_NONE;
    DEBUG_FORCE((_T("%s: InitInputCfg begin\n"), __FUNCTIONW__));

    sts = m_pBitstreamSource->open(decInputParams.strSrcURI, decInputParams.nDecodeId, decInputParams.bIsLoopMode, decInputParams.nLoopNum, decInputParams.bInputByFPS);
    if (sts != MFX_ERR_NONE) {
        cerr << "\nERROR: Failed to get SDP" << endl;
        return sts;
    }
    m_pBitstreamSource->setErrListener(this);

    //generate DecOptions
    memset(&dec_cfg, 0, sizeof(DecOptions));
    dec_cfg.measurement = &measurement;

    dec_cfg.isMempool = !m_pBitstreamSource->isRTSP();
    if (m_pBitstreamSource->isRTSP()) {
        dec_cfg.inputStream = m_pBitstreamSource->getStreamBuffer();
    } else {
        dec_cfg.inputMempool = m_pBitstreamSource->getMemPool();
    }
    dec_cfg.input_codec_type = MfxTypeToMsdkType(decInputParams.nDecodeId);

    DEBUG_FORCE((_T("%s: InitInputCfg end\n"), __FUNCTIONW__));
    return sts;
}

// initialize EncOptions parameter
mfxStatus Launcher::InitOutputCfg(const EncParams &encInputParam, EncOptions &enc_cfg)
{
    mfxStatus sts = MFX_ERR_NONE;

    //generate EncOptions
    memset(&enc_cfg, 0, sizeof(EncOptions));
    enc_cfg.measurement = &measurement;
    if(m_pVideoSink.get() != NULL) {
        enc_cfg.outputStream = m_pVideoSink->GetStream();
    }

    enc_cfg.output_codec_type = MfxTypeToMsdkType(encInputParam.nEncodeId);
    enc_cfg.bitrate = encInputParam.nBitRate;
    enc_cfg.numSlices = encInputParam.nSlices;
    enc_cfg.level = encInputParam.nCodecLevel;

    enc_cfg.mbbrc_enable = encInputParam.bMBBRC;
    enc_cfg.nMaxSliceSize = encInputParam.nMaxSliceSize;

    enc_cfg.numRefFrame = encInputParam.nRefNum;
    enc_cfg.swCodecPref = false;
    enc_cfg.qpValue = encInputParam.nQP;
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    enc_cfg.extRoiData = encInputParam.extRoiData;
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#if (MFX_VERSION >= 1025)
    enc_cfg.nMFEFrames = encInputParam.nMFEFrames;
    enc_cfg.nMFEMode = encInputParam.nMFEMode;
    enc_cfg.nMFETimeout = encInputParam.nMFETimeout;
#endif

    enc_cfg.nRepartitionCheckEnable = encInputParam.nRepartitionCheckMode;
    enc_cfg.ratectrl =
            (encInputParam.bLABRC || 0 != encInputParam.nLADepth) ?
            (mfxU16)MFX_RATECONTROL_LA : encInputParam.nRateControlMethod;

    if (CODEC_TYPE_VIDEO_AVC == enc_cfg.output_codec_type) {
        enc_cfg.la_depth = encInputParam.nLADepth;
        enc_cfg.bRefType = encInputParam.bRefType;
    }

    if (CODEC_TYPE_VIDEO_AVC == enc_cfg.output_codec_type ||
            CODEC_TYPE_VIDEO_HEVC == enc_cfg.output_codec_type ||
            CODEC_TYPE_VIDEO_MPEG2 == enc_cfg.output_codec_type) {
        enc_cfg.profile = encInputParam.nCodecProfile;
        enc_cfg.targetUsage = encInputParam.nTargetUsage;
        enc_cfg.gopRefDist = encInputParam.nGopRefDist;
        enc_cfg.gopOptFlag = encInputParam.nGopOptFlag;
        enc_cfg.intraPeriod = encInputParam.nGopPicSize;
        enc_cfg.idrInterval = encInputParam.nIdrInterval;
    }
    if (enc_cfg.output_codec_type == CODEC_TYPE_VIDEO_VP8) {
        enc_cfg.vp8OutFormat = IVF_FILE;
    }

    if (CODEC_TYPE_VIDEO_HEVC == enc_cfg.output_codec_type) {
        enc_cfg.numTileRows = encInputParam.nTileRows;
        enc_cfg.numTileColumns = encInputParam.nTileColumns;
    }

#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    enc_cfg.bPreEnc = encInputParam.bPreEnc;
    enc_cfg.bEncode = encInputParam.bEncode;
    if (encInputParam.bENC || encInputParam.bENCPAK)
        enc_cfg.bENC = true;
    if (encInputParam.bPAK || encInputParam.bENCPAK)
        enc_cfg.bPAK = true;

    if(!encInputParam.mvoutFile.empty()){
        Stream* mvout_stream = new Stream();
        if(mvout_stream){
            bool openWrite = (enc_cfg.bPAK && !(enc_cfg.bPreEnc || enc_cfg.bENC || enc_cfg.bEncode))? false : true;
            if(!mvout_stream->Open(encInputParam.mvoutFile.c_str(), openWrite)){
                DEBUG_ERROR((_T("open mvoutFile failed.\n")));
                delete mvout_stream;
                return MFX_ERR_UNKNOWN;
            }
            enc_cfg.mvoutStream = mvout_stream;
            m_mvoutStreamList.push_back(mvout_stream);
        }else{
            DEBUG_ERROR((_T("create object Stream failed.\n")));
            return MFX_ERR_UNKNOWN;
        }
    }

    if(!encInputParam.mbcodeoutFile.empty()){
        Stream* mbcodeout_stream = new Stream();
        if(mbcodeout_stream){
            bool openWrite = (enc_cfg.bPAK && !(enc_cfg.bENC || enc_cfg.bEncode))? false : true;
            if(!mbcodeout_stream->Open(encInputParam.mbcodeoutFile.c_str(), openWrite)){
                DEBUG_ERROR((_T("open mbcodeout_streamFile failed.\n")));
                delete mbcodeout_stream;
                return MFX_ERR_UNKNOWN;
            }
            enc_cfg.mbcodeoutStream = mbcodeout_stream;
            m_mbcodeoutStreamList.push_back(mbcodeout_stream);
        }else{
            DEBUG_ERROR((_T("create object Stream failed.\n")));
            return MFX_ERR_UNKNOWN;
        }
    }

    if(!encInputParam.mvinFile.empty()){
        Stream* mvin_stream = new Stream();
        if(mvin_stream){
            bool openWrite = false;
            if(!mvin_stream->Open(encInputParam.mvinFile.c_str(), openWrite)){
                DEBUG_ERROR((_T("open mbcodeout_streamFile failed.\n")));
                delete mvin_stream;
                return MFX_ERR_UNKNOWN;
            }
            enc_cfg.mvinStream = mvin_stream;
            m_mvinStreamList.push_back(mvin_stream);
        }else{
            DEBUG_ERROR((_T("create object Stream failed.\n")));
            return MFX_ERR_UNKNOWN;
        }
    }

    // Open weights file
    if(!encInputParam.weightFile.empty()){
        Stream* weight_stream = new Stream();
        if(weight_stream){
            bool openWrite = false;
            if(!weight_stream->Open(encInputParam.weightFile.c_str(), openWrite)){
                DEBUG_ERROR((_T("open weights file failed.\n")));
                delete weight_stream;
                return MFX_ERR_UNKNOWN;
            }
            enc_cfg.weightStream = weight_stream;
            m_weightStreamList.push_back(weight_stream);
        }else{
            DEBUG_ERROR((_T("create object Stream failed.\n")));
            return MFX_ERR_UNKNOWN;
        }
    }

    enc_cfg.fei_ctrl = encInputParam.fei_ctrl;
#endif
    return sts;
}

mfxStatus Launcher::InitVppCfg(const sVppParams &vppInputParam, VppOptions &vpp_cfg)
{
    mfxStatus sts = MFX_ERR_NONE;

    memset(&vpp_cfg, 0, sizeof(VppOptions));
    vpp_cfg.measurement = &measurement;
    //will set to vpp.Out.CropW and CropH, not need to align
    vpp_cfg.out_width = vppInputParam.nDstWidth;
    vpp_cfg.out_height = vppInputParam.nDstHeight;
    // vpp_cfg.bLABRC = vppInputParam.bLABRC;
    // vpp_cfg.nLADepth = vppInputParam.nLADepth;
    vpp_cfg.nSuggestSurface = vppInputParam.nSuggestSurface;

    // Add for deinterlace
    if (vppInputParam.bEnableDeinterlacing) {
        vpp_cfg.nPicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        } else {
        vpp_cfg.nPicStruct = vppInputParam.nPicStruct;
    }

    ConvertFrameRate(vppInputParam.dFrameRate,
          &vpp_cfg.out_fps_n, &vpp_cfg.out_fps_d);

    vpp_cfg.denoiseEnabled = vppInputParam.bDenoiseEnabled ;
    vpp_cfg.denoiseFactor = vppInputParam.nDenoiseLevel;

    return sts;
}

/** Add Input Picture parameters
 *
 */
void Launcher::AddPicBlendInfo(const CompositionParams &compositionInputParams, VppOptions &vppCfg, BlendInfo &blendInfo)
{
    blendInfo.type = compositionInputParams.nFourCC;

    const char * name = compositionInputParams.strStreamName.c_str();
    if (MFX_FOURCC_NV12 == blendInfo.type || MFX_FOURCC_RGB4 == blendInfo.type) {
        blendInfo.rawInfo = new RawInfo();
        blendInfo.rawInfo->raw_file = (char*)name;
        blendInfo.rawInfo->fourcc = blendInfo.type;

        blendInfo.rawInfo->cropx = compositionInputParams.nCropX;
        blendInfo.rawInfo->cropy = compositionInputParams.nCropY;
        blendInfo.rawInfo->cropw = compositionInputParams.nCropW;
        blendInfo.rawInfo->croph = compositionInputParams.nCropH;

        blendInfo.rawInfo->width = compositionInputParams.nWidth;
        blendInfo.rawInfo->height = compositionInputParams.nHeight;

        blendInfo.rawInfo->dstx = compositionInputParams.nDstX;
        blendInfo.rawInfo->dsty = compositionInputParams.nDstY;
        blendInfo.rawInfo->dstw = compositionInputParams.nDstW;
        blendInfo.rawInfo->dsth = compositionInputParams.nDstH;

        if (compositionInputParams.nGlobalAlphaEnable != 0) {
            blendInfo.rawInfo->global_alpha_enable = compositionInputParams.nGlobalAlphaEnable;
            blendInfo.rawInfo->alpha = compositionInputParams.nGlobalAlpha;
        }
        if (compositionInputParams.nLumaKeyEnable != 0) {
            blendInfo.rawInfo->luma_key_enable = compositionInputParams.nLumaKeyEnable;
            blendInfo.rawInfo->luma_key_min = compositionInputParams.nLumaKeyMin;
            blendInfo.rawInfo->luma_key_max = compositionInputParams.nLumaKeyMax;
        }
        if (compositionInputParams.nPixelAlphaEnable != 0) {
            blendInfo.rawInfo->pixel_alpha_enable = compositionInputParams.nPixelAlphaEnable;
        }
    }
}

void Launcher::AttachPicBlend(MsdkXcoder *xcoder, unsigned long time, void *vppHandle, BlendInfo &blendInfo)
{

        DecOptions dec_cfg;
        dec_cfg.measurement = &measurement;
        dec_cfg.inputPicture = blendInfo.picinfo;
        switch(blendInfo.type) {
            case MFX_FOURCC_NV12:
            case MFX_FOURCC_RGB4:
                dec_cfg.input_codec_type = CODEC_TYPE_VIDEO_YUV;
            break;
            default:
                dec_cfg.input_codec_type = CODEC_TYPE_VIDEO_PICTURE;
            break;
        }
        xcoder->PicOperateAttach(&dec_cfg, vppHandle, time);
        blendInfo.dec_handle = dec_cfg.DecHandle;
        printf("start picture vpp = %p, dec = %p \n", vppHandle, dec_cfg.DecHandle);

}

void Launcher::AttachNewTranscoder(MsdkXcoder *xcoder, VppOptions &vppCfg, EncOptions &encCfg, bool haveVPP)
{
    if (haveVPP)
        xcoder->AttachVpp(&vppCfg, &encCfg);
    else
        xcoder->AttachVpp(nullptr, &encCfg);
}

void Launcher::DetachTranscoder(MsdkXcoder *xcoder, VppOptions &vpp)
{
    xcoder->DetachVpp(vpp.VppHandle);
}

void Launcher::onError(void)
{
    Stop();
}

