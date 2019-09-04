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

#include "hevc_encoder_element.h"

#ifdef ENABLE_HEVC

#include <assert.h>
#include <math.h>

#include "element_common.h"

DEFINE_MLOGINSTANCE_CLASS(HevcEncoderElement, "HevcEncoderElement");

unsigned int HevcEncoderElement::mNumOfEncChannels = 0;

HevcEncoderElement::HevcEncoderElement(ElementType type,
                               MFXVideoSession *session,
                               MFXFrameAllocator *pMFXAllocator,
                               CodecEventCallback *callback)
{
    initialize(type, session, pMFXAllocator, callback);
    mfx_enc_ = NULL;

    memset(&output_bs_, 0, sizeof(output_bs_));
    memset(&enc_ctrl_, 0, sizeof(enc_ctrl_));

    force_key_frame_ = false;
    reset_bitrate_flag_ = false;
    reset_res_flag_ = false;
    m_nFramesProcessed = 0;
}

HevcEncoderElement::~HevcEncoderElement()
{
    if (mfx_enc_) {
        mfx_enc_->Close();
        delete mfx_enc_;
        mfx_enc_ = NULL;
    }

    mfxPluginUID uid = MFX_PLUGINID_HEVCE_HW;
    MFXVideoUSER_UnLoad(*mfx_session_, &uid);

    if (!m_EncExtParams.empty()) {
        m_EncExtParams.clear();
    }

    if (NULL != output_bs_.Data) {
        delete[] output_bs_.Data;
        output_bs_.Data = NULL;
    }
}

void HevcEncoderElement::SetForceKeyFrame()
{
    force_key_frame_ = true;
}
void HevcEncoderElement::SetResetBitrateFlag()
{
    reset_bitrate_flag_ = true;
}

bool HevcEncoderElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    if (ele_param->EncParams.mfx.CodecId != MFX_CODEC_HEVC) {
        MLOG_ERROR("Encoder type is not HEVC\n");
        return false;
    }

    measurement = ele_param->measurement;
    m_bLibType = ele_param->libType;

    if (m_bLibType == MFX_IMPL_HARDWARE_ANY) {
        mfxPluginUID uid = MFX_PLUGINID_HEVCE_HW;
        mfxStatus sts = MFXVideoUSER_Load(*mfx_session_, &uid, 1/*version*/);
        assert(sts == MFX_ERR_NONE);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("failed to load HEVC encode plugin\n");
            return false;
        }
    }

    mfx_enc_ = new MFXVideoENCODE(*mfx_session_);
    mfx_video_param_.mfx.CodecId = ele_param->EncParams.mfx.CodecId;
    mfx_video_param_.mfx.CodecProfile = ele_param->EncParams.mfx.CodecProfile;
    mfx_video_param_.mfx.CodecLevel = ele_param->EncParams.mfx.CodecLevel;
    /* Encoding Options */
    mfx_video_param_.mfx.TargetUsage = ele_param->EncParams.mfx.TargetUsage;
    mfx_video_param_.mfx.GopPicSize = ele_param->EncParams.mfx.GopPicSize;
    mfx_video_param_.mfx.GopRefDist = ele_param->EncParams.mfx.GopRefDist;
    mfx_video_param_.mfx.IdrInterval = ele_param->EncParams.mfx.IdrInterval;
    mfx_video_param_.mfx.RateControlMethod =
        (ele_param->EncParams.mfx.RateControlMethod > 0) ?
        ele_param->EncParams.mfx.RateControlMethod : MFX_RATECONTROL_VBR;

    if (mfx_video_param_.mfx.RateControlMethod == MFX_RATECONTROL_CQP) {
        mfx_video_param_.mfx.QPI = ele_param->EncParams.mfx.QPI;
        mfx_video_param_.mfx.QPP = ele_param->EncParams.mfx.QPP;
        mfx_video_param_.mfx.QPB = ele_param->EncParams.mfx.QPB;
    } else {
        mfx_video_param_.mfx.TargetKbps = ele_param->EncParams.mfx.TargetKbps;
    }

    mfx_video_param_.mfx.NumSlice = ele_param->EncParams.mfx.NumSlice;
    mfx_video_param_.mfx.NumRefFrame = ele_param->EncParams.mfx.NumRefFrame;
    if (0 == mfx_video_param_.mfx.NumRefFrame) {
        mfx_video_param_.mfx.NumRefFrame = 4;
    }
    mfx_video_param_.mfx.EncodedOrder = ele_param->EncParams.mfx.EncodedOrder;
    mfx_video_param_.AsyncDepth = 1;
    extParams.bMBBRC = ele_param->extParams.bMBBRC;
    extParams.nLADepth = ele_param->extParams.nLADepth;
    extParams.nMbPerSlice = ele_param->extParams.nMbPerSlice;
    extParams.bRefType = ele_param->extParams.bRefType;
    if (mfx_video_param_.mfx.NumSlice && ele_param->extParams.nMaxSliceSize) {
        MLOG_WARNING(" MaxSliceSize is not compatible with NumSlice, discard it\n");
        extParams.nMaxSliceSize = 0;
    } else {
        extParams.nMaxSliceSize = ele_param->extParams.nMaxSliceSize;
    }
    output_stream_ = ele_param->output_stream;

    return true;
}

mfxStatus HevcEncoderElement::InitEncoder(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest EncRequest;
    mfxVideoParam par;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(ENC_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    mfx_video_param_.mfx.FrameInfo.FrameRateExtN = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtN;
    mfx_video_param_.mfx.FrameInfo.FrameRateExtD = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FrameRateExtD;

    if (((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_TFF || \
            ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct == MFX_PICSTRUCT_FIELD_BFF) {
        mfx_video_param_.mfx.FrameInfo.PicStruct     = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.PicStruct;
    } else {
        mfx_video_param_.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
    }
    mfx_video_param_.mfx.FrameInfo.FourCC        = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.FourCC;
    mfx_video_param_.mfx.FrameInfo.ChromaFormat  = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.ChromaFormat;
    mfx_video_param_.mfx.FrameInfo.CropX         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropX;
    mfx_video_param_.mfx.FrameInfo.CropY         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropY;
    mfx_video_param_.mfx.FrameInfo.CropW         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropW;
    mfx_video_param_.mfx.FrameInfo.CropH         = ((mfxFrameSurface1 *)buf.msdk_surface)->Info.CropH;
    // In case of HW HEVC decoder width and height must be aligned to 32 pixels. This limitation is planned to be removed in later versions of plugin
    mfx_video_param_.mfx.FrameInfo.Width = MSDK_ALIGN32(((mfxFrameSurface1 *)buf.msdk_surface)->Info.Width);
    mfx_video_param_.mfx.FrameInfo.Height = MSDK_ALIGN32(((mfxFrameSurface1 *)buf.msdk_surface)->Info.Height);

    if ((mfx_video_param_.mfx.RateControlMethod != MFX_RATECONTROL_CQP) && \
            mfx_video_param_.mfx.RateControlMethod != MFX_RATECONTROL_VBR && \
            !mfx_video_param_.mfx.TargetKbps) {
        mfx_video_param_.mfx.TargetKbps = CalDefBitrate(mfx_video_param_.mfx.CodecId, \
            mfx_video_param_.mfx.TargetUsage, \
            mfx_video_param_.mfx.FrameInfo.Width, \
            mfx_video_param_.mfx.FrameInfo.Height, \
            1.0 * mfx_video_param_.mfx.FrameInfo.FrameRateExtN / \
            mfx_video_param_.mfx.FrameInfo.FrameRateExtD);
        MLOG_WARNING(" invalid setup bitrate,use default %d Kbps\n", \
               mfx_video_param_.mfx.TargetKbps);
    }

    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;
        //opaue alloc
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(ext_opaque_alloc_);
        memcpy(&ext_opaque_alloc_.In, \
               & ((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out, \
               sizeof(ext_opaque_alloc_.In));
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));

    } else {
        MLOG_INFO("------------Encoder using video memory\n");
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
    }

#if 0 // undevelop
    if (NULL != tiles.get()) {
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(tiles.get()));
        MLOG_INFO("init encoder, tile param number is %dx%d\n", tiles->NumTileRows, tiles->NumTileColumns);
    }
    if (NULL != region.get()) {
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(region.get()));
    }
#endif
    // In case of HEVC when height and/or width divided with 8 but not divided with 16
    // add extended parameter to increase performance
    if (( !((mfx_video_param_.mfx.FrameInfo.CropW & 15 ) ^ 8 ) || 
          !((mfx_video_param_.mfx.FrameInfo.CropH & 15 ) ^ 8 ))) {
        SetParam(mfx_video_param_.mfx.FrameInfo.CropW, mfx_video_param_.mfx.FrameInfo.CropH, 0);
        m_EncExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(param.get()));
    }

    //set mfx video ExtParam
    if (!m_EncExtParams.empty()) {
        mfx_video_param_.ExtParam = &m_EncExtParams.front(); // vector is stored linearly in memory
        mfx_video_param_.NumExtParam = (mfxU16)m_EncExtParams.size();
        MLOG_INFO("init encoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
    }

    memset(&EncRequest, 0, sizeof(EncRequest));

    sts = mfx_enc_->QueryIOSurf(&mfx_video_param_, &EncRequest);
    MLOG_INFO("Enc suggest surfaces %d\n", EncRequest.NumFrameSuggested);
    assert(sts >= MFX_ERR_NONE);
    memset(&par, 0, sizeof(par));

    sts = mfx_enc_->Init(&mfx_video_param_);
    if (sts > MFX_ERR_NONE) {
        MLOG_WARNING("\t\t------------enc init warning %d\n", sts);
    } else if (sts < MFX_ERR_NONE) {
        MLOG_ERROR("\t\t------------enc init return %d\n", sts);
    }
    assert(sts >= 0);

    sts = mfx_enc_->GetVideoParam(&par);

    // Prepare Media SDK bit stream buffer for encoder
    memset(&output_bs_, 0, sizeof(output_bs_));
    output_bs_.MaxLength = par.mfx.BufferSizeInKB * 2000;
    output_bs_.Data = new mfxU8[output_bs_.MaxLength];
    MLOG_INFO("output bitstream buffer size %d\n", output_bs_.MaxLength);

    m_EncExtParams.clear();
    m_nFramesProcessed = 0;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(ENC_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    return sts;
}

int HevcEncoderElement::ProcessChainEncode(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;
    mfxSyncPoint syncpE;
    mfxEncodeCtrl *enc_ctrl = NULL;

    SetThreadName("HEVC Encode Thread");

    if (!codec_init_) {
        if (buf.msdk_surface == NULL && buf.is_eos) {
            //The first buf encoder received is eos
            return -1;
        }
        sts = InitEncoder(buf);

        if (MFX_ERR_NONE == sts) {
            codec_init_ = true;

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(ENC_ENDURATION_TIME_STAMP, this);
                pipelineinfo einfo;
                einfo.mElementType = element_type_;
                einfo.mChannelNum = HevcEncoderElement::mNumOfEncChannels;
                HevcEncoderElement::mNumOfEncChannels++;

                switch (mfx_video_param_.mfx.CodecId) {
                    case MFX_CODEC_HEVC:
                        einfo.mCodecName = "HEVC";
                        break;
                    default:
                        einfo.mCodecName = "unkown codec";
                        break;
                }

                measurement->SetElementInfo(ENC_ENDURATION_TIME_STAMP, this, &einfo);
                measurement->RelLock();
            }

            MLOG_INFO("Encoder %p init successfully\n", this);
        } else {
            MLOG_ERROR("encoder create failed %d\n", sts);
            return -1;
        }
    }

    // to check if there is res change in incoming surface
    if (pmfxInSurface) {
        if (mfx_video_param_.mfx.FrameInfo.Width  != pmfxInSurface->Info.Width ||
            mfx_video_param_.mfx.FrameInfo.Height != pmfxInSurface->Info.Height) {
            mfx_video_param_.mfx.FrameInfo.Width   = pmfxInSurface->Info.Width;
            mfx_video_param_.mfx.FrameInfo.Height  = pmfxInSurface->Info.Height;
            mfx_video_param_.mfx.FrameInfo.CropW   = pmfxInSurface->Info.CropW;
            mfx_video_param_.mfx.FrameInfo.CropH   = pmfxInSurface->Info.CropH;

            reset_res_flag_ = true;
            MLOG_INFO(" to reset encoder res as %d x %d\n",
                   mfx_video_param_.mfx.FrameInfo.CropW, mfx_video_param_.mfx.FrameInfo.CropH);
        }

        //mark the frame order for incoming surface.
        pmfxInSurface->Data.FrameOrder = m_nFramesProcessed;
    }

    if (reset_bitrate_flag_ || reset_res_flag_){
        //need to complete encoding with current settings
        for (;;) {
            for (;;) {
                sts = mfx_enc_->EncodeFrameAsync(enc_ctrl, NULL, &output_bs_, &syncpE);

                if (MFX_ERR_NONE < sts && !syncpE) {
                    if (MFX_WRN_DEVICE_BUSY == sts) {
                        usleep(20);
                    }
                } else if (MFX_ERR_NONE < sts && syncpE) {
                    // ignore warnings if output is available
                    sts = MFX_ERR_NONE;
                    break;
                } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                    // Allocate more bitstream buffer memory here if needed...
                    break;
                } else {
                    break;
                }
            }

            if (sts == MFX_ERR_MORE_DATA) {
                break;
            }
        }
    }
    if (reset_bitrate_flag_) {
        mfx_enc_->Reset(&mfx_video_param_);
        reset_bitrate_flag_ = false;
    }

    if (reset_res_flag_) {
        mfxVideoParam par;
        memset(&par, 0, sizeof(par));
        mfx_enc_->GetVideoParam(&par);
        par.mfx.FrameInfo.CropW = mfx_video_param_.mfx.FrameInfo.CropW;
        par.mfx.FrameInfo.CropH = mfx_video_param_.mfx.FrameInfo.CropH;
        par.mfx.FrameInfo.Width = mfx_video_param_.mfx.FrameInfo.Width;
        par.mfx.FrameInfo.Height = mfx_video_param_.mfx.FrameInfo.Height;

        mfx_enc_->Reset(&par);
        reset_res_flag_ = false;
    }

    if (force_key_frame_) {

        memset(&enc_ctrl_, 0, sizeof(enc_ctrl_));
        enc_ctrl_.FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR | MFX_FRAMETYPE_REF;
        //workaround, bug tracked by CQ VCSD100022643
        enc_ctrl = &enc_ctrl_;
        force_key_frame_ = false;
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(ENC_FRAME_TIME_STAMP, this);
        measurement->RelLock();
    }

    do {
        while (is_running_) {

            sts = mfx_enc_->EncodeFrameAsync(enc_ctrl, pmfxInSurface,
                                             &output_bs_, &syncpE);


            if (MFX_ERR_NONE < sts && !syncpE) {
                if (MFX_WRN_DEVICE_BUSY == sts) {
                    usleep(20);
                }
            } else if (MFX_ERR_NONE < sts && syncpE) {
                // ignore warnings if output is available
                sts = MFX_ERR_NONE;
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else {
                break;
            }
        }

        //Klockwork issue #531: if is_running_ == false in this do-while loop, syncpE is uninitialized.
        if (MFX_ERR_NONE == sts && is_running_) {
            sts = mfx_session_->SyncOperation(syncpE, 60000);

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(ENC_FRAME_TIME_STAMP, this);
                measurement->RelLock();
            }
            if (output_stream_) {
                sts = WriteBitStreamFrame(&output_bs_, output_stream_);
            }

            m_nFramesProcessed++;
        }

        if (is_running_ == false) {
            //break the loop
            break;
        }
    } while (pmfxInSurface == NULL && sts >= MFX_ERR_NONE);

    if (NULL == pmfxInSurface && buf.is_eos && output_stream_ != NULL) {
        output_stream_->SetEndOfStream();
        MLOG_INFO("Got EOF in Encoder %p\n", this);
        is_running_ = false;
    }

    //MLOG_INFO("leave ProcessChainEncode\n");
    return 0;
}

int HevcEncoderElement::HandleProcessEncode()
{
    MediaBuf buf;
    MediaPad *sinkpad = *(this->sinkpads_.begin());
    assert(0 != this->sinkpads_.size());

    while (is_running_) {
        if (sinkpad->GetBufData(buf) != 0) {
            // No data, just sleep and wait
            usleep(100);
            continue;
        }

        int ret = ProcessChain(sinkpad, buf);
        if (ret == 0) {
            sinkpad->ReturnBufToPeerPad(buf);
        }
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(ENC_ENDURATION_TIME_STAMP, this);
        measurement->RelLock();
    }

    if (!is_running_) {
        while (sinkpad->GetBufData(buf) == 0) {
            sinkpad->ReturnBufToPeerPad(buf);
        }
    } else {
        is_running_ = false;
    }

    return 0;
}



int HevcEncoderElement::HandleProcess()
{
    return HandleProcessEncode();
}

int HevcEncoderElement::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    if (buf.msdk_surface == NULL && buf.is_eos) {
        MLOG_INFO("Got EOF in element %p, type is %d\n", this, element_type_);
    }
    return ProcessChainEncode(buf);
}

void HevcEncoderElement::SetTiles(mfxU16 rows, mfxU16 cols)
{
    tiles.reset(new mfxExtHEVCTiles);
    memset(tiles.get(), 0, sizeof(mfxExtHEVCTiles));
    tiles->Header.BufferId = MFX_EXTBUFF_HEVC_TILES;
    tiles->Header.BufferSz = sizeof(mfxExtHEVCTiles);
    tiles->NumTileRows = rows;
    tiles->NumTileColumns = cols;
}

void HevcEncoderElement::SetRegion(mfxU32 id, mfxU16 type, mfxU16 encoding)
{
    region.reset(new mfxExtHEVCRegion);
    memset(region.get(), 0, sizeof(mfxExtHEVCRegion));
    region->Header.BufferId = MFX_EXTBUFF_HEVC_REGION;
    region->Header.BufferSz = sizeof(mfxExtHEVCRegion);
    region->RegionId = id;
    region->RegionType = type;
    region->RegionEncoding = encoding;
}

void HevcEncoderElement::SetParam(mfxU16 pic_w_luma, mfxU16 pic_h_luma, mfxU64 flags)
{
    param.reset(new mfxExtHEVCParam);
    memset(param.get(), 0, sizeof(mfxExtHEVCParam));
    param->Header.BufferId = MFX_EXTBUFF_HEVC_PARAM;
    param->Header.BufferSz = sizeof(mfxExtHEVCParam);
    param->PicWidthInLumaSamples = pic_w_luma;
    param->PicHeightInLumaSamples = pic_h_luma;
    param->GeneralConstraintFlags = flags;

    MLOG_INFO("Width = %u, Height = %u, flags = %u\n", pic_w_luma, pic_h_luma, flags);
}

mfxStatus HevcEncoderElement::GetVideoParam(mfxVideoParam *par)
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = mfx_enc_->GetVideoParam(par);
    return sts;
}

#endif //#ifdef ENABLE_HEVC

