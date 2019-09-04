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

#include "decoder_element.h"

#include <assert.h>
#include "os/platform.h"
#ifdef PLATFORM_OS_LINUX
#include <unistd.h>
#endif

#include "element_common.h"

#define VP8_RAW_FRAME_HDR_SZ (sizeof(uint32_t))
#define VP8_IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define IVF_FILE_HEADER_LENGTH 32
#define VP8_FRAME_MAX_LENGTH (256*1024*1024)
#define VP8_RAW_DATA_MAX_LENGTH (256*1024)

DEFINE_MLOGINSTANCE_CLASS(DecoderElement, "DecoderElement");
unsigned int DecoderElement::mNumOfDecChannels = 0;

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
static unsigned int GetU32FromMem(const void *vmem)
{
    unsigned int  val;
    const unsigned char *mem = (const unsigned char *)vmem;
    val = mem[3] << 24;
    val |= mem[2] << 16;
    val |= mem[1] << 8;
    val |= mem[0] << 0;
    return val;
}
#endif

DecoderElement::DecoderElement(ElementType type,
                               MFXVideoSession *session,
                               MFXFrameAllocator *pMFXAllocator,
                               CodecEventCallback *callback)
{
    initialize(type, session, pMFXAllocator);
    input_mp_ = NULL;
    input_stream_ = NULL;
    mfx_dec_ = NULL;
    vp8_file_type_ = UNKNOWN;
    is_mempool_ = true;

    memset(&input_bs_, 0, sizeof(input_bs_));
}
DecoderElement::~DecoderElement()
{
    if (mfx_dec_) {
        mfx_dec_->Close();
        delete mfx_dec_;
        mfx_dec_ = NULL;
    }

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    //to unload vp8 decode plugin
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8) {
        mfxPluginUID uid = MFX_PLUGINID_VP8D_HW;
        MFXVideoUSER_UnLoad(*mfx_session_, &uid);
    }
#endif

    if (!m_DecExtParams.empty()) {
        m_DecExtParams.clear();
    }

    if (surface_pool_) {
        for (unsigned int i = 0; i < num_of_surf_; i++) {
            if (surface_pool_[i]) {
                delete surface_pool_[i];
                surface_pool_[i] = NULL;
            }
        }
        delete surface_pool_;
        surface_pool_ = NULL;
    }

    if (m_pMFXAllocator && !m_bUseOpaqueMemory ) {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxDecResponse);
        m_pMFXAllocator = NULL;
    }
}

bool DecoderElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        MLOG_ERROR("decoder initialize params is null\n");
        return false;
    }

    measurement = ele_param->measurement;

    m_extParams.suggestSurfaceNum = ele_param->extDecParams.suggestSurfaceNum;
    m_extParams.have_NoVpp = ele_param->extDecParams.have_NoVpp;

    mfx_dec_ = new MFXVideoDECODE(*mfx_session_);
    mfx_video_param_.mfx.CodecId = ele_param->DecParams.mfx.CodecId;

    // to enable decorative flags, has effect with 1.3 libraries only
    // (in case of JPEG decoder - it is not valid to use this field)
    //if (mfx_video_param_.mfx.CodecId != MFX_CODEC_JPEG) {
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_AVC || \
            mfx_video_param_.mfx.CodecId == MFX_CODEC_MPEG2 || \
            mfx_video_param_.mfx.CodecId == MFX_CODEC_VC1) {
        mfx_video_param_.mfx.ExtendedPicStruct = 1;
    }
    m_bLibType = ele_param->libType;
    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    } else if (m_bLibType == MFX_IMPL_SOFTWARE) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    }

    mfx_video_param_.AsyncDepth = 1;
    is_mempool_ = ele_param->isMempool;
    if (!is_mempool_) {
        input_stream_ = ele_param->input_stream;
        input_mp_ = new MemPool();
        input_mp_->init(1024*1024*4);
    } else {
        input_mp_ = ele_param->input_mp;
    }

    input_bs_.MaxLength = input_mp_->GetTotalBufSize();
    input_bs_.Data = input_mp_->GetReadPtr();
    MLOG_INFO("init finished\n");

    return true;
}

int DecoderElement::Recycle(MediaBuf &buf)
{
    recycle(buf);

    return 0;
}

int DecoderElement::ProcessChainDecode()
{
#ifdef ENABLE_VA
    bool bFlushed = true;
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf buf;
    MediaPad *srcpad = *(this->srcpads_.begin());
    mfxSyncPoint syncpD;
    mfxFrameSurface1 *pmfxOutSurface = NULL;

    input_bs_.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
    // Preprae the bit stream for frame.
    // check if the decoder initialized.
    while (1) {
        if (!codec_init_) {
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(DEC_INIT_TIME_STAMP, this);
                measurement->RelLock();
            }

            sts = InitDecoder();

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(DEC_INIT_TIME_STAMP, this);
                measurement->RelLock();
            }

            if (MFX_ERR_NONE == sts) {
                codec_init_ = true;
                if (measurement) {
                    measurement->GetLock();
                    pipelineinfo einfo;
                    einfo.mElementType = element_type_;
                    einfo.mChannelNum = DecoderElement::mNumOfDecChannels;
                    DecoderElement::mNumOfDecChannels++;
                    if (input_mp_) {
                        einfo.mInputName.append(input_mp_->GetInputName());
                    }
                    measurement->SetElementInfo(DEC_ENDURATION_TIME_STAMP, this, &einfo);
                    measurement->RelLock();
                }
            } else {
                MLOG_ERROR("Decode create failed: %d\n", sts);
                return -1;
            }
        }

        UpdateBitStream();
        // prepare the surface for the decoder.
        int nIndex = 0;
        nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);
        if (MFX_ERR_NOT_FOUND == nIndex) {
            MLOG_ERROR("Failed to get the output surface\n");
            return -1;
        }

        if (bFlushed == false) {
            sts = mfx_dec_->DecodeFrameAsync(NULL, surface_pool_[nIndex],
                                                        &pmfxOutSurface, &syncpD);
            if (!syncpD && MFX_ERR_MORE_DATA == sts) {
                // Last frame decoded, push EOS
                buf.msdk_surface = NULL;
                buf.is_eos = 1;
                ReturnBuf(buf);
                break;
            }
        } else {
            sts = mfx_dec_->DecodeFrameAsync(&input_bs_, surface_pool_[nIndex],
                                                   &pmfxOutSurface, &syncpD);
        }

        UpdateMemPool();
        if (MFX_ERR_NONE < sts && syncpD) {
            sts = MFX_ERR_NONE;
        }

        if (MFX_ERR_NONE == sts) {
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(DEC_FRAME_TIME_STAMP, this);
                measurement->RelLock();
            }
            buf.msdk_surface = pmfxOutSurface;

            if (buf.msdk_surface) {
                mfxFrameSurface1 *msdk_surface =
                       (mfxFrameSurface1 *)buf.msdk_surface;
                msdk_atomic_inc16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
            }

            buf.mWidth = mfx_video_param_.mfx.FrameInfo.CropW;
            buf.mHeight = mfx_video_param_.mfx.FrameInfo.CropH;
            buf.is_eos = 0;

            if (m_bUseOpaqueMemory) {
                buf.ext_opaque_alloc = &ext_opaque_alloc_;
            }

            sts = mfx_session_->SyncOperation(syncpD, 60000);

            srcpad->PushBufToPeerPad(buf);
            if (measurement) {
                measurement->GetLock();
                measurement->UpdateCodecNum();
                measurement->RelLock();
            }
            break;
        } else if (sts < MFX_ERR_NONE){
            if ((sts == MFX_ERR_MORE_SURFACE) || (sts == MFX_ERR_MORE_DATA)) {
                bFlushed = false;
            } else {
                break;
            }
        }
    }
    return 0;
#else
    return -1;
#endif
}

#ifdef ENABLE_VA
int DecoderElement::Step(FrameMeta* input)
{
    int sts;

    sts = ProcessChainDecode();
    return sts;
}
#endif
void DecoderElement::UpdateBitStream()
{
    if (!is_mempool_) {
        int mempool_freeflat = input_mp_->GetFreeFlatBufSize();
        unsigned char *mempool_wrptr = input_mp_->GetWritePtr();
        int remainSize = mempool_freeflat;
        int writedSize = 0;

        while(remainSize && input_stream_->GetBlockCount() > 0) {
            int copySize = 0;
            copySize = input_stream_->ReadBlockEx(mempool_wrptr + writedSize, remainSize, false, NULL, NULL, NULL);
            writedSize += copySize;
            remainSize -= copySize;
        }
        input_mp_->UpdateWritePtrCopyData(writedSize);
        if (input_stream_->GetBlockCount() == 0 && input_stream_->GetEndOfStream())
            input_mp_->SetDataEof();
    }
    input_bs_.Data = input_mp_->GetReadPtr();
    input_bs_.DataOffset = 0;
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8 && ELEMENT_DECODER == element_type_) {
        // if VP8 format, get beginning pointer of raw data, and get raw data length
        mfxU32 dataLen = input_mp_->GetFlatBufFullness();
        if (dataLen > VP8_RAW_FRAME_HDR_SZ) {
            mfxU32 frameLength = GetU32FromMem(input_bs_.Data);
            mfxU32 frameHeaderLength = vp8_file_type_ == RAW_FILE ? VP8_RAW_FRAME_HDR_SZ : VP8_IVF_FRAME_HDR_SZ;
            if (dataLen >= (frameLength + frameHeaderLength)) {
                // the beginning pointer of raw data is the first byte after frame header
                input_bs_.DataOffset = frameHeaderLength;
                input_bs_.DataLength = frameLength;
            } else {
                // if buffer data is not enough, do not decode and wait data.
                input_bs_.DataLength = 0;
            }
        } else {
            input_bs_.DataLength = 0;
        }
    } else {
        input_bs_.DataLength = input_mp_->GetFlatBufFullness();
    }
#else
    input_bs_.DataLength = input_mp_->GetFlatBufFullness();
#endif
}

void DecoderElement::UpdateMemPool()
{
    input_mp_->UpdateReadPtr(input_bs_.DataOffset);
}

mfxStatus DecoderElement::InitDecoder()
{
    mfxStatus sts = MFX_ERR_NONE;

    UpdateBitStream();
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    // if vp8 raw data length is invalid return unknown error.
    if ((input_bs_.DataLength > VP8_FRAME_MAX_LENGTH || \
            (vp8_file_type_ == RAW_FILE && input_bs_.DataLength > VP8_RAW_DATA_MAX_LENGTH))) {
        return MFX_ERR_UNKNOWN;
    }
    mfxU32 frameTotalLen =  input_bs_.DataOffset + input_bs_.DataLength;
#endif

    sts = mfx_dec_->DecodeHeader(&input_bs_, &mfx_video_param_);

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8) {
        if (MFX_ERR_NONE == sts) {
            // do not discard key frame, decode function need the key frame.
            input_bs_.DataOffset = 0;
        } else if (MFX_ERR_MORE_DATA == sts) {
            // if not find key frame, discard this frame.
            MLOG_WARNING("invald key frame, skip this frame bytes [%u]\n", frameTotalLen);
            if (input_mp_->GetFlatBufFullness() >= frameTotalLen) {
                input_bs_.DataOffset = frameTotalLen;
            } else {
                input_bs_.DataOffset = 0;
            }
        }
    }
#endif
    UpdateMemPool();

    if (MFX_ERR_MORE_DATA == sts) {
        if (callback_ && is_running_) {
            callback_->DecodeHeaderFailEvent(this);
        }
        usleep(10000);
        return sts;
    } else if (MFX_ERR_NONE == sts) {
        mfxFrameAllocRequest DecRequest;
        memset(&DecRequest, 0, sizeof(mfxFrameAllocRequest));
        sts = mfx_dec_->QueryIOSurf(&mfx_video_param_, &DecRequest);

        if ((MFX_ERR_NONE != sts) && (MFX_WRN_PARTIAL_ACCELERATION != sts)) {
            MLOG_ERROR("Decoder QueryIOSurf return %d, failed\n", sts);
            return sts;
        }

        num_of_surf_ = DecRequest.NumFrameSuggested + (m_extParams.have_NoVpp ? m_extParams.suggestSurfaceNum : 0) + 10;
        MLOG_INFO("Decoder the number of surface is %d %d:%d\n", num_of_surf_, m_extParams.have_NoVpp, m_extParams.suggestSurfaceNum);
        sts = AllocFrames(&DecRequest, true);

        if (m_bUseOpaqueMemory) {
            ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
            ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
            ext_opaque_alloc_.Out.Surfaces = surface_pool_;
            ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
            ext_opaque_alloc_.Out.Type = DecRequest.Type;
            m_DecExtParams.push_back(reinterpret_cast<mfxExtBuffer *>(&ext_opaque_alloc_));
        }

        if (!m_DecExtParams.empty()) {
            mfx_video_param_.ExtParam = &m_DecExtParams.front(); // vector is stored linearly in memory
            mfx_video_param_.NumExtParam = (mfxU16)m_DecExtParams.size();
            MLOG_INFO("init decoder, ext param number is %d\n", mfx_video_param_.NumExtParam);
        }

        // Initialize the Media SDK decoder
        sts = mfx_dec_->Init(&mfx_video_param_);

        if ((sts != MFX_ERR_NONE) && (MFX_WRN_PARTIAL_ACCELERATION != sts)) {
            return sts;
        }
    } else {
        MLOG_ERROR("DecodeHeader returns %d\n", sts);
        return sts;
    }

    return MFX_ERR_NONE;
}

int DecoderElement::HandleProcessDecode()
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf buf;
    MediaPad *srcpad = *(this->srcpads_.begin());
    mfxSyncPoint syncpD;
    mfxFrameSurface1 *pmfxOutSurface = NULL;
    bool bEndOfDecoder = false;

    SetThreadName("Decode Thread");

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
    //load vp8 decode plugin
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8) {
        mfxPluginUID uid = MFX_PLUGINID_VP8D_HW;
        mfxStatus sts = MFXVideoUSER_Load(*mfx_session_, &uid, 1/*version*/);
        assert(sts == MFX_ERR_NONE);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("failed to load vp8 hw decode plugin\n");
            return sts;
        } else {
            MLOG_INFO("success to load vp8 hw decode plugin\n");
        }
    }
#endif

#if defined(ENABLE_HEVC)
    /* Add HEVC support */
    if (mfx_video_param_.mfx.CodecId == MFX_CODEC_HEVC) {
        mfxVersion version;     // real API version with which library is initialized
        sts = MFXQueryVersion(*mfx_session_, &version); // get real API version of the loaded library
        assert(sts == MFX_ERR_NONE);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("failed to get MSDK version\n");
            return -1;
        }
        if ((version.Major != 1) || (version.Minor < 8)) {
            MLOG_ERROR("current MSDK version does not support HEVC HW decoder\n");
            return -1;
        }
        mfxPluginUID uid = MFX_PLUGINID_HEVCD_HW;
        sts = MFXVideoUSER_Load(*mfx_session_, &uid, 1/*version*/);
        assert(sts == MFX_ERR_NONE);
        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("failed to load HEVC Decode plugin:%d\n", sts);
            return -1;
        }
    }
#endif

    while (is_running_) {
        usleep(100);
        // when decoder returns MFX_ERR_INCOMPATIBLE_VIDEO_PARAM error
        // decoder will be closed and re-open, and reallocate the frame buffer
        if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts) {
            MLOG_INFO("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM begin\n");
            codec_init_ = false;
            mfx_dec_->Close();
            // wait all surface is not used.
            if (surface_pool_) {
                int count = 0;
                while (is_running_) {
                    bool allrelease = true;
                    // wait VPP release these surface.
                    for (unsigned int i = 0; i < num_of_surf_; i++) {
                        if (surface_pool_[i] && surface_pool_[i]->Data.Locked) {
                            allrelease = false;
                            //MLOG_INFO("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM [%p][%u] \n", this, i);
                        }
                    }
                    // if wait exceed 0.5 second, break this loop
                    if (allrelease || count > 5000) {
                        MLOG_INFO("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM exit loop [%d] count [%d]\n", srcpad->GetBufQueueSize(), count);
                        break;
                    } else {
                        usleep(100);
                    }
                    // if the srcpad is empty, start counting.
                    if (srcpad->GetBufQueueSize() == 0) {
                        count++;
                    }
                }
                if (!is_running_) {
                    break;
                }
                // release all surfaces.
                for (unsigned int i = 0; i < num_of_surf_; i++) {
                    if (surface_pool_[i]) {
                        delete surface_pool_[i];
                        surface_pool_[i] = NULL;
                    }
                }
                delete surface_pool_;
                surface_pool_ = NULL;
            }

            if (m_pMFXAllocator && !m_bUseOpaqueMemory ) {
                m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxDecResponse);
            }
            MLOG_INFO("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM finish\n");
        }

        if (!codec_init_) {
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
            // check input VP8 format
            if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8 && vp8_file_type_ == UNKNOWN) {
                unsigned char * dataPtr = input_mp_->GetReadPtr();
                mfxU32 dataLen = input_mp_->GetFlatBufFullness();
                if (dataLen > 4) {
                    // check IVF format
                    if (dataPtr[0] == 'D' && dataPtr[1] == 'K'
                        && dataPtr[2] == 'I' && dataPtr[3] == 'F') {
                        if (dataLen >= IVF_FILE_HEADER_LENGTH) {
                            vp8_file_type_ = IVF_FILE;
                            MLOG_INFO("input is IVF format\n");
                            // discard the IVF file header(32 bytes).
                            input_mp_->UpdateReadPtr(IVF_FILE_HEADER_LENGTH);
                        } else {
                            continue;
                        }
                    } else {
                        // check raw format
                        if (GetU32FromMem(dataPtr) < VP8_RAW_DATA_MAX_LENGTH) {
                            vp8_file_type_ = RAW_FILE;
                            MLOG_INFO("input is vp8 raw data format\n");
                        } else {
                            MLOG_ERROR("The vp8 format is invalid and MSDK does not support this kind of format\n");
                            break;
                        }
                    }
                }
                continue;
            }
#endif
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpStart(DEC_INIT_TIME_STAMP, this);
                measurement->RelLock();
            }

            sts = InitDecoder();

            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(DEC_INIT_TIME_STAMP, this);
                measurement->RelLock();
            }

            if (MFX_ERR_MORE_DATA == sts) {
                continue;
            } else if (MFX_ERR_NONE == sts) {
                codec_init_ = true;

                if (measurement) {
                    measurement->GetLock();
                    pipelineinfo einfo;
                    einfo.mElementType = element_type_;
                    einfo.mChannelNum = DecoderElement::mNumOfDecChannels;
                    DecoderElement::mNumOfDecChannels++;
                    if (input_mp_) {
                        einfo.mInputName.append(input_mp_->GetInputName());
                    }
                    measurement->SetElementInfo(DEC_ENDURATION_TIME_STAMP, this, &einfo);
                    measurement->RelLock();
                }
            } else {
                MLOG_ERROR("Decode create failed: %d\n", sts);
                return -1;
            }
        }

        // Read es stream into bit stream
        int nIndex = 0;

        while (is_running_) {
            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

            if (MFX_ERR_NOT_FOUND == nIndex) {
                usleep(10000);
            } else {
                break;
            }
        }

        // Check again in case for a invalid surface.
        if (!is_running_) {
            break;
        }

        UpdateBitStream();
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
        // if vp8 raw data length is invalid, break the decode thread.
        if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8) {
            if ((input_bs_.DataLength > VP8_FRAME_MAX_LENGTH || \
                    (vp8_file_type_ == RAW_FILE && input_bs_.DataLength > VP8_RAW_DATA_MAX_LENGTH))) {
                MLOG_ERROR("VP8 frame length is invalid.\n");
                sts = MFX_ERR_UNKNOWN;
                break;
            }
        }
        mfxU32 frameTotalLen =  input_bs_.DataOffset + input_bs_.DataLength;
#endif
        if (input_bs_.DataLength == 0 && !(input_mp_->GetDataEof())) continue;

        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpStart(DEC_FRAME_TIME_STAMP, this);
            measurement->RelLock();
        }
        buf.is_decoder_reset = false;
        if (input_mp_->GetDataEof() && bEndOfDecoder) {

            sts = mfx_dec_->DecodeFrameAsync(NULL, surface_pool_[nIndex],
                                             &pmfxOutSurface, &syncpD);

            if (!syncpD && MFX_ERR_MORE_DATA == sts) {
                // Last frame decoded, push EOS
                // MLOG_INFO("Decoder flushed, sending EOS to next element\n");
                buf.msdk_surface = NULL;
                buf.is_eos = 1;
                MLOG_INFO("This video stream EOS.\n");
                srcpad->PushBufToPeerPad(buf);
                break;
            }
        } else {
            sts = mfx_dec_->DecodeFrameAsync(&input_bs_, surface_pool_[nIndex],
                                             &pmfxOutSurface, &syncpD);

            if (!syncpD && input_mp_->GetDataEof()) {
                if (MFX_ERR_NONE > sts && MFX_ERR_MORE_SURFACE != sts && MFX_ERR_INCOMPATIBLE_VIDEO_PARAM != sts) {
                    if (MFX_ERR_MORE_DATA != sts) {
                        MLOG_ERROR("Decoder return error code:%d\n", sts);
                    }
                    bEndOfDecoder = true;
                }
            }
        }

        if (MFX_ERR_NONE < sts && syncpD) {
            sts = MFX_ERR_NONE;
        }

        if (MFX_ERR_NONE == sts) {

            buf.msdk_surface = pmfxOutSurface;
            if (buf.msdk_surface) {
                mfxFrameSurface1 *msdk_surface =
                    (mfxFrameSurface1 *)buf.msdk_surface;
                msdk_atomic_inc16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
            }

            buf.mWidth = mfx_video_param_.mfx.FrameInfo.CropW;
            buf.mHeight = mfx_video_param_.mfx.FrameInfo.CropH;
            buf.is_eos = 0;

            if (m_bUseOpaqueMemory) {
                buf.ext_opaque_alloc = &ext_opaque_alloc_;
            }

            sts = mfx_session_->SyncOperation(syncpD, 60000);

            if (MFX_ERR_NONE == sts) {
            if (measurement) {
                measurement->GetLock();
                measurement->TimeStpFinish(DEC_FRAME_TIME_STAMP, this);
                measurement->RelLock();
            }
                srcpad->PushBufToPeerPad(buf);
                if (measurement) {
                    measurement->GetLock();
                    measurement->UpdateCodecNum();
                    measurement->RelLock();
                }
            } else {
                if (buf.msdk_surface) {
                    mfxFrameSurface1 *msdk_surface =
                        (mfxFrameSurface1 *)buf.msdk_surface;
                    msdk_atomic_dec16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
                }
            }
        } else if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts) {
            MLOG_INFO("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM happen input_bs_.DataOffset[%u][%p]\n", input_bs_.DataOffset, this);
            // if the decoder parsed a new sequence header in the bitstream
            // and decoding cannot continue without reallocating frame buffers.
            // so pipeline need close and re-open decoder.
            mfxVideoParam video_param;
            video_param.mfx.CodecId = mfx_video_param_.mfx.CodecId;
            video_param.mfx.ExtendedPicStruct = mfx_video_param_.mfx.ExtendedPicStruct;
            video_param.IOPattern = mfx_video_param_.IOPattern;
            video_param.AsyncDepth = mfx_video_param_.AsyncDepth;
            // at first use DecodeHeader function check this frame is key frame or not.
            mfxU32  orgDataOffset = input_bs_.DataOffset;
            input_bs_.DataOffset = 0;
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
            if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8) {
                if (vp8_file_type_ == IVF_FILE) {
                    input_bs_.DataOffset = VP8_IVF_FRAME_HDR_SZ;
                } else {
                    input_bs_.DataOffset = VP8_RAW_FRAME_HDR_SZ;
                }
            }
#endif
            // if the new sequence header is invalid, the DecodeHeader will not return MFX_ERR_NONE
            // skip the invalid frame
            mfxStatus reset_sts = mfx_dec_->DecodeHeader(&input_bs_, &video_param);
            MLOG_INFO("DecodeHeader sts[%d] input_bs_.DataOffset[%u]\n", reset_sts, input_bs_.DataOffset);
            if (MFX_ERR_NONE == reset_sts) {
                // Retrieve any remaining frames by calling DecodeFrameAsync with a
                // NULL input bitstream pointer
                reset_sts = mfx_dec_->DecodeFrameAsync(NULL, surface_pool_[nIndex],
                                                 &pmfxOutSurface, &syncpD);
                if (MFX_ERR_MORE_DATA == reset_sts) {
                    MLOG_INFO("decode must be reset.\n");
                }

                // send NULL buffer for VPP, VPP release the msdk surface.
                buf.msdk_surface = NULL;
                buf.is_eos = 0;
                buf.is_decoder_reset = true;
                buf.mWidth = video_param.mfx.FrameInfo.Width;
                buf.mHeight = video_param.mfx.FrameInfo.Height;
                MLOG_INFO("This video stream reset vpp. [%u] bEndOfDecoder[%d] srcpad->GetBufQueueSize()[%d]\n", input_bs_.DataOffset, bEndOfDecoder, srcpad->GetBufQueueSize());
                srcpad->PushBufToPeerPad(buf);
                input_bs_.DataOffset = 0;
            } else {
                // if this frame is not key frame, ignore this frame, and decode the next frame.
                input_bs_.DataOffset = orgDataOffset;
                sts = MFX_ERR_MORE_DATA;
                MLOG_INFO("This frame is not a new sequence header. [%u] bEndOfDecoder[%d]\n", input_bs_.DataOffset, bEndOfDecoder);
            }
        }

#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
        if (mfx_video_param_.mfx.CodecId == MFX_CODEC_VP8 && \
                (MFX_ERR_UNKNOWN == sts || MFX_ERR_MORE_DATA == sts)) {
            // if decode is not success, skip this frame.
            input_bs_.DataOffset = frameTotalLen;
            // if remain video data, do not stop decode.
            if (bEndOfDecoder && input_mp_->GetFlatBufFullness() > frameTotalLen) {
                bEndOfDecoder = false;
            }
            MLOG_INFO("this vp8 frame size [%u] is skipped. bEndOfDecoder[%d]\n", frameTotalLen, bEndOfDecoder);
        }
#endif
        UpdateMemPool();
    }

    if (!is_running_) {
        buf.msdk_surface = NULL;
        buf.is_eos = 1;
        MLOG_INFO("This video stream will be stoped.\n");
        srcpad->PushBufToPeerPad(buf);
    } else {
        is_running_ = false;
    }
    return 0;
}

int DecoderElement::HandleProcess()
{
    return HandleProcessDecode();
}

int DecoderElement::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    if (buf.msdk_surface == NULL && buf.is_eos) {
        MLOG_INFO("Got EOF in element %p, type is %d\n", this, element_type_);
    }
    return ProcessChainDecode();
}
