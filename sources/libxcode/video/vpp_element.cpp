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

#include "vpp_element.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if defined (PLATFORM_OS_LINUX)
#include <sys/time.h>
#include <unistd.h>
#endif

#include "element_common.h"

#ifdef ENABLE_VA
#include "va_plugin.h"
#endif

DEFINE_MLOGINSTANCE_CLASS(VPPElement, "VPPElement");

VPPElement::VPPElement(ElementType type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator, CodecEventCallback *callback):
mfx_session_(session), m_pMFXAllocator(pMFXAllocator)
{
    codec_init_ = false;
    surface_pool_ = NULL;
    num_of_surf_ = 0;
    input_str_ = NULL;
    input_pic_ = NULL;
    input_raw_ = NULL;
    mfx_vpp_ = NULL;
    vppframe_num_ = 0;
    callback_ = callback;
    measurement = NULL;

    combo_type_ = COMBO_BLOCKS;
    combo_master_ = NULL;
    reinit_vpp_flag_ = false;
    res_change_flag_ = false;
    stream_cnt_ = 0;
    string_cnt_ = 0;
    pic_cnt_ = 0;
    raw_cnt_ = 0;
    eos_cnt = 0;
    element_type_ = type;
    comp_stc_ = 0;
#ifdef ENABLE_VA
    extbuf_ = NULL;
#endif
    if (m_pMFXAllocator) {
        m_bUseOpaqueMemory = false;
    } else {
        m_bUseOpaqueMemory = true;
    }

    memset(&ext_opaque_alloc_, 0, sizeof(ext_opaque_alloc_));
    memset(&mfx_video_param_, 0, sizeof(mfx_video_param_));

    comp_stc_ = 0;
    max_comp_framerate_ = 0;
    current_comp_number_ = 0;
    comp_info_change_ = false;
    frame_width_ = 0;
    frame_height_ = 0;
    memset(&bg_color_info, 0, sizeof(bg_color_info));
    keep_ratio_ = false;
    m_bLibType = MFX_IMPL_HARDWARE_ANY;
    enabledFilterCount = 0;
    memset(tabDoUseAlg, 0, ENH_FILTERS_COUNT);
    memset(&extDoUse, 0, sizeof(extDoUse));
    memset(&vpp_denoise, 0, sizeof(vpp_denoise));
}

VPPElement::~VPPElement()
{
    if (mfx_vpp_) {
        mfx_vpp_->Close();
        delete mfx_vpp_;
        mfx_vpp_ = NULL;
    }

    if (surface_pool_) {
        for (unsigned int i = 0; i < num_of_surf_; i++) {
            if (surface_pool_[i]) {
#if ENABLE_VA
                if (element_type_ == ELEMENT_VPP) {
                    mfxExtBuffer **ext_buf = surface_pool_[i]->Data.ExtParam;
                    VAPluginData* plugin_data = (VAPluginData*)(*ext_buf);
                    if (plugin_data) {
                        FrameMeta* out = plugin_data->Out;
                        if (out) {
                            if (out->FeatureType == 1) {
                                if (out->Descriptor) {
                                    delete out->Descriptor;
                                }
                            } else {
                                LIST_ROI::iterator it;
                                for(it = out->ROIList.begin(); it != out->ROIList.end(); it++) {
                                    if (*it)
                                        delete *it;
                                }
                                out->ROIList.clear();
                             }
                            delete out;
                            out = NULL;
                        }
                        delete plugin_data;
                    }
                }
#endif
                //when element is deleted, all pre-allocated surfaces should have already been recycled.
                //dj: workaround here, surface not unlocked by msdk. Try "p d <end> q" to reproduce
                //if (element_type_ != ELEMENT_DECODER && element_type_ != ELEMENT_STRING_DEC &&
                //    element_type_ != ELEMENT_BMP_DEC) {
                //    assert(surface_pool_[i]->Data.Locked == 0);
                //}
                delete surface_pool_[i];
                surface_pool_[i] = NULL;
            }
        }
        delete surface_pool_;
        surface_pool_ = NULL;
    }
#if ENABLE_VA
    if(extbuf_ != NULL) {
        delete[] extbuf_;
        extbuf_ = NULL;
    }
#endif

    if (m_pMFXAllocator && !m_bUseOpaqueMemory) {
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxEncResponse);
        m_pMFXAllocator = NULL;
    }
}

int VPPElement::ConfigVppCombo(ComboType combo_type, void *master_handle)
{
    if (combo_type < COMBO_BLOCKS || combo_type > COMBO_CUSTOM) {
        MLOG_ERROR("Set invalid combo type\n");
        return -1;
    }

    if (COMBO_MASTER == combo_type && NULL == master_handle) {
        MLOG_ERROR("Set invalid combo master handle\n");
        return -1;
    }

    if (combo_type_ != combo_type || (COMBO_MASTER == combo_type && combo_master_ != master_handle)) {
        combo_type_ = combo_type;

        if (COMBO_MASTER == combo_type) {
            combo_master_ = master_handle;
        }

        //For COMBO_CUSTOM, wait for SetCompRegion() to re-init VPP.
        if (COMBO_CUSTOM != combo_type) {
            reinit_vpp_flag_ = true;
        }
        MLOG_INFO("Set VPP combo type=%d, master=%p\n", combo_type, master_handle);
    }

    return 0;
}

bool VPPElement::Init(void *cfg, ElementMode element_mode)
{
    element_mode_ = element_mode;
    ElementCfg *ele_param = static_cast<ElementCfg *>(cfg);

    if (NULL == ele_param) {
        return false;
    }

    measurement = ele_param->measurement;
    mfx_vpp_ = new MFXVideoVPP(*mfx_session_);

    if (ele_param->VppParams.vpp.Out.FourCC == 0) {
        mfx_video_param_.vpp.Out.FourCC        = MFX_FOURCC_NV12;
    } else {
        mfx_video_param_.vpp.Out.FourCC        = ele_param->VppParams.vpp.Out.FourCC;
    }

    //MLOG_INFO("FourCC is %d\n", mfx_video_param_.vpp.Out.FourCC);

    if (mfx_video_param_.vpp.Out.FourCC == MFX_FOURCC_NV12) {
        mfx_video_param_.vpp.Out.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    }

    mfx_video_param_.vpp.Out.CropX         = ele_param->VppParams.vpp.Out.CropX;
    mfx_video_param_.vpp.Out.CropY         = ele_param->VppParams.vpp.Out.CropY;
    mfx_video_param_.vpp.Out.CropW         = ele_param->VppParams.vpp.Out.CropW;
    mfx_video_param_.vpp.Out.CropH         = ele_param->VppParams.vpp.Out.CropH;
    mfx_video_param_.vpp.Out.PicStruct     = ele_param->VppParams.vpp.Out.PicStruct;
    mfx_video_param_.vpp.Out.FrameRateExtN = ele_param->VppParams.vpp.Out.FrameRateExtN;
    mfx_video_param_.vpp.Out.FrameRateExtD = ele_param->VppParams.vpp.Out.FrameRateExtD;
    mfx_video_param_.AsyncDepth = 1;

    // Init VPP Filters
    enabledFilterCount = 0;
    memset(&extDoUse, 0, sizeof(extDoUse));
    extDoUse.Header.BufferId = MFX_EXTBUFF_VPP_DOUSE;
    extDoUse.Header.BufferSz = sizeof(mfxExtVPPDoUse);
    extDoUse.NumAlg  = 0;
    extDoUse.AlgList = NULL;

    memset(&vpp_denoise, 0, sizeof(vpp_denoise));
    if (ele_param->vpp_denoise.enabled) {
        // if denoise level not set, using default value
        tabDoUseAlg[enabledFilterCount++] = MFX_EXTBUFF_VPP_DENOISE;
        if (0xFFFF != ele_param->vpp_denoise.factor) {
            vpp_denoise.Header.BufferId = MFX_EXTBUFF_VPP_DENOISE;
            vpp_denoise.Header.BufferSz = sizeof(vpp_denoise);
            vpp_denoise.DenoiseFactor = ele_param->vpp_denoise.factor;
        }
    }

    bg_color_info.Y = 0;
    bg_color_info.U = 128;
    bg_color_info.V = 128;

    // m_extParams.bLABRC = ele_param->extVppParams.bLABRC;
    // m_extParams.nLADepth = ele_param->extVppParams.nLADepth;
    m_extParams.nSuggestSurface = ele_param->extVppParams.nSuggestSurface;
    m_bLibType = ele_param->libType;

    return true;

}

int VPPElement::Recycle(MediaBuf &buf)
{
    //when this assert happens, check if you return the buf the second time or to incorrect element.
    assert(surface_pool_);
    if (buf.msdk_surface != NULL) {
        unsigned int i;
        for (i = 0; i < num_of_surf_; i++) {
            if (surface_pool_[i] == buf.msdk_surface) {
                mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
                msdk_atomic_dec16((volatile mfxU16*)(&(msdk_surface->Data.Locked)));
                break;
            }
        }
        //or else the buf is not owned by this element, why did you return it to this element?
        assert(i < num_of_surf_);
    }

    return 0;
}
int VPPElement::SetVppCompParam(mfxExtVPPComposite *vpp_comp)
{
    memset(vpp_comp, 0, sizeof(mfxExtVPPComposite));
    vpp_comp->Header.BufferId = MFX_EXTBUFF_VPP_COMPOSITE;
    vpp_comp->Header.BufferSz = sizeof(mfxExtVPPComposite);
    vpp_comp->Y = bg_color_info.Y;
    vpp_comp->U = bg_color_info.U;
    vpp_comp->V = bg_color_info.V;
    if (combo_type_ == COMBO_CUSTOM) {
        vpp_comp->NumInputStream = (mfxU16)dec_region_list_.size();
    } else {
        vpp_comp->NumInputStream = (mfxU16)vpp_comp_map_.size();
    }
    vpp_comp->InputStream = new mfxVPPCompInputStream[vpp_comp->NumInputStream];
    memset(vpp_comp->InputStream, 0 , sizeof(mfxVPPCompInputStream) * vpp_comp->NumInputStream);
    MLOG_INFO("About to set composition parameter, number of stream is %d\n",
           vpp_comp->NumInputStream);
    std::map<MediaPad *, VPPCompInfo>::iterator it_comp_info;
    int pad_idx = 0;    //indicate the index in sinkpads(or all inputs)
    int vid_idx = 0;    //indicate the index in video streams(subset of inputs)
    int output_width = mfx_video_param_.vpp.Out.Width;
    int output_height = mfx_video_param_.vpp.Out.Height;
    //3 kinds of inputs: stream(video), string and picture
    int stream_cnt = 0; //count of stream inputs
    int string_cnt = 0; //count of string inputs
    int pic_cnt = 0;    //count of picture inputs
    int raw_cnt = 0;    //count of YUV inputs

    for (it_comp_info = vpp_comp_map_.begin();
            it_comp_info != vpp_comp_map_.end();
            ++it_comp_info) {
        if (!it_comp_info->second.str_info.text && !it_comp_info->second.pic_info.bmp
#if defined ENABLE_RAW_DECODE
               && (NULL == it_comp_info->second.raw_info.raw_file)) {
#else
            ) {
#endif
            stream_cnt++;
        } else if (it_comp_info->second.str_info.text && !it_comp_info->second.pic_info.bmp
#if defined ENABLE_RAW_DECODE
               && (NULL == it_comp_info->second.raw_info.raw_file)) {
#else
            ) {
#endif
            string_cnt++;
        } else if (!it_comp_info->second.str_info.text && it_comp_info->second.pic_info.bmp
#if defined ENABLE_RAW_DECODE
               && (NULL == it_comp_info->second.raw_info.raw_file)) {
#else
            ) {
#endif
            pic_cnt++;
        } else if (!it_comp_info->second.str_info.text && !it_comp_info->second.pic_info.bmp
#if defined ENABLE_RAW_DECODE
               && (NULL != it_comp_info->second.raw_info.raw_file)) {
#else
            ) {
#endif
            raw_cnt++;
        } else {
            MLOG_ERROR("Input can't be string and picture at the same time.\n");
            assert(0);
            return -1;
        }
    }
    MLOG_INFO("Input comp count is %d,video streams %d,text strings %d, picture %d\n",
           vpp_comp->NumInputStream, stream_cnt, string_cnt, pic_cnt);

    //there should be at least one stream
    assert(stream_cnt);
    stream_cnt_ = stream_cnt;
    string_cnt_ = string_cnt;
    pic_cnt_ = pic_cnt;
    raw_cnt_ = raw_cnt;
    eos_cnt = 0;

    //stream_idx - to indicate stream's index in vpp_comp->InputStream(as that in sinkpads_).
    int *stream_idx;
    stream_idx = new int[stream_cnt];
    memset(stream_idx, 0, sizeof(int) * stream_cnt);
    std::list<MediaPad *>::iterator it_sinkpad;
    bool is_text;
    bool is_pic;
    bool is_raw;
    VPPCompInfo pad_comp_info;

    //Set the comp info for text&pic inputs, and set stream_idx[] for stream inputs
    for (it_sinkpad = this->sinkpads_.begin(), pad_idx = 0;
            it_sinkpad != this->sinkpads_.end();
            ++it_sinkpad, ++pad_idx) {
        is_text = false;
        is_pic = false;
        is_raw = false;
        pad_comp_info = vpp_comp_map_[*it_sinkpad];

        if (pad_comp_info.str_info.text) {
            is_text = true;
        }
        if (pad_comp_info.pic_info.bmp) {
            is_pic = true;
        }
#if defined ENABLE_RAW_DECODE
        if (pad_comp_info.raw_info.raw_file != NULL) {
            is_raw = true;
        }
#endif
        if ((is_text && is_pic) || (is_text && is_raw)) {
            MLOG_ERROR("Input can't be text and picture at the same time.\n");
            delete [] stream_idx;
            return -1;
        }
        if (!is_text && !is_pic && !is_raw) { //input is video stream
            stream_idx[vid_idx] = pad_idx;
            vid_idx++;
        } else if (is_text) { //input is text string
            vpp_comp->InputStream[pad_idx].DstX = pad_comp_info.str_info.pos_x;
            vpp_comp->InputStream[pad_idx].DstY = pad_comp_info.str_info.pos_y;
            vpp_comp->InputStream[pad_idx].DstW = pad_comp_info.str_info.width;
            vpp_comp->InputStream[pad_idx].DstH = pad_comp_info.str_info.height;
            vpp_comp->InputStream[pad_idx].LumaKeyEnable = 1;
            vpp_comp->InputStream[pad_idx].LumaKeyMin = 0;
            vpp_comp->InputStream[pad_idx].LumaKeyMax = 10;
            vpp_comp->InputStream[pad_idx].GlobalAlphaEnable = 1;
            vpp_comp->InputStream[pad_idx].GlobalAlpha = (mfxU16)(pad_comp_info.str_info.alpha * 255);
        } else if (is_pic) { //input is picture (bitmap)
            vpp_comp->InputStream[pad_idx].DstX = pad_comp_info.pic_info.pos_x;
            vpp_comp->InputStream[pad_idx].DstY = pad_comp_info.pic_info.pos_y;
            vpp_comp->InputStream[pad_idx].DstW = pad_comp_info.pic_info.width;
            vpp_comp->InputStream[pad_idx].DstH = pad_comp_info.pic_info.height;
            vpp_comp->InputStream[pad_idx].LumaKeyEnable = pad_comp_info.pic_info.luma_key_enable;
            vpp_comp->InputStream[pad_idx].LumaKeyMin = pad_comp_info.pic_info.luma_key_min;
            vpp_comp->InputStream[pad_idx].LumaKeyMax = pad_comp_info.pic_info.luma_key_max;
            vpp_comp->InputStream[pad_idx].GlobalAlphaEnable = pad_comp_info.pic_info.global_alpha_enable;
            vpp_comp->InputStream[pad_idx].GlobalAlpha = (mfxU16)(pad_comp_info.pic_info.alpha * 255);
            vpp_comp->InputStream[pad_idx].PixelAlphaEnable = pad_comp_info.pic_info.pixel_alpha_enable;
#if defined ENABLE_RAW_DECODE
        } else if (is_raw) { //input is picture (bitmap)
            vpp_comp->InputStream[pad_idx].DstX = pad_comp_info.raw_info.dstx;
            vpp_comp->InputStream[pad_idx].DstY = pad_comp_info.raw_info.dsty;
            vpp_comp->InputStream[pad_idx].DstW = pad_comp_info.raw_info.dstw;
            vpp_comp->InputStream[pad_idx].DstH = pad_comp_info.raw_info.dsth;
            vpp_comp->InputStream[pad_idx].LumaKeyEnable = pad_comp_info.raw_info.luma_key_enable;
            vpp_comp->InputStream[pad_idx].LumaKeyMin = pad_comp_info.raw_info.luma_key_min;
            vpp_comp->InputStream[pad_idx].LumaKeyMax = pad_comp_info.raw_info.luma_key_max;
            vpp_comp->InputStream[pad_idx].GlobalAlphaEnable = pad_comp_info.raw_info.global_alpha_enable;
            vpp_comp->InputStream[pad_idx].GlobalAlpha = (mfxU16)(pad_comp_info.raw_info.alpha * 255);
            vpp_comp->InputStream[pad_idx].PixelAlphaEnable = pad_comp_info.raw_info.pixel_alpha_enable;
#endif
        }
    }

    if (combo_type_ == COMBO_CUSTOM) {
        CustomLayout::iterator it = dec_region_list_.begin();
        for (vid_idx = 0; it != dec_region_list_.end(); ++it) {
            Region region = it->region;
            MediaPad *pad = (MediaPad *)it->handle;
            ComputeCompPosition((unsigned int)output_width * region.left,
                                (unsigned int)output_height * region.top,
                                (unsigned int)output_width * region.width_ratio,
                                (unsigned int)output_height * region.height_ratio,
                                vpp_comp_map_[pad].org_width,
                                vpp_comp_map_[pad].org_height,
                                vpp_comp->InputStream[vid_idx]);

            vid_idx++;
        }
    } else if (combo_type_ == COMBO_MASTER) {
        int master_vid_idx = -1; //master's index in video streams
        int sequence = 0;
        int line_size = 0;

        if (stream_cnt <= 6) {
            line_size = 3;
        } else if (stream_cnt > 6 && stream_cnt <= 16) {
            line_size = (stream_cnt + 1) / 2;
        }

        //find the index for master stream input
        if (combo_master_) {
            for (it_sinkpad = this->sinkpads_.begin();
                 it_sinkpad != this->sinkpads_.end();
                 it_sinkpad++) {
                pad_comp_info = vpp_comp_map_[*it_sinkpad];
                if (!pad_comp_info.str_info.text && !pad_comp_info.pic_info.bmp
#if defined ENABLE_RAW_DECODE
                        && NULL == pad_comp_info.raw_info.raw_file) {
#else
                    ) {
#endif
                    master_vid_idx++;
                    if ((*it_sinkpad)->get_peer_pad()->get_parent() == combo_master_) {
                        break;
                    }
                }
            }
            assert(it_sinkpad != this->sinkpads_.end());
        } else {
            //happens when master is detached.
            //TODO: how to handle this gently?
            MLOG_WARNING("master doesn't exist, set the first input as master\n");
            master_vid_idx = 0;
        }
        assert(master_vid_idx >= 0 && master_vid_idx < stream_cnt);

        for (vid_idx = 0; vid_idx < stream_cnt; vid_idx++) {
            pad_idx = stream_idx[vid_idx];
            int index = 0;
            for (it_sinkpad = this->sinkpads_.begin();
                 it_sinkpad != this->sinkpads_.end();
                 it_sinkpad++) {
                if (index == pad_idx) {
                    break;
                } else {
                    index++;
                }
            }
            if (vid_idx == master_vid_idx) {
                if (it_sinkpad != this->sinkpads_.end()) {
                    ComputeCompPosition(0,
                                        0,
                                        output_width * (line_size - 1) / line_size,
                                        output_height * (line_size - 1) / line_size,
                                        vpp_comp_map_[(*it_sinkpad)].org_width,
                                        vpp_comp_map_[(*it_sinkpad)].org_height,
                                        vpp_comp->InputStream[stream_idx[vid_idx]]);
                } else {
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstX = 0;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstY = 0;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstW = output_width * (line_size - 1) / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstH = output_height * (line_size - 1) / line_size;
                }
                continue;
            }

            sequence = (vid_idx > master_vid_idx) ? vid_idx : (vid_idx + 1);
            if (sequence <= line_size) {
                if (it_sinkpad != this->sinkpads_.end()) {
                    ComputeCompPosition((sequence - 1) * output_width / line_size,
                                        output_height * (line_size - 1) / line_size,
                                        output_width / line_size,
                                        output_height / line_size,
                                        vpp_comp_map_[(*it_sinkpad)].org_width,
                                        vpp_comp_map_[(*it_sinkpad)].org_height,
                                        vpp_comp->InputStream[stream_idx[vid_idx]]);
                } else {
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstX = (sequence - 1) * output_width / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstY = output_height * (line_size - 1) / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstW = output_width / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstH = output_height / line_size;
                }
            } else if (sequence < line_size * 2) {
                if (it_sinkpad != this->sinkpads_.end()) {
                    ComputeCompPosition(output_width * (line_size - 1) / line_size,
                                        output_height - (sequence - (line_size - 1)) * output_height / line_size,
                                        output_width / line_size,
                                        output_height / line_size,
                                        vpp_comp_map_[(*it_sinkpad)].org_width,
                                        vpp_comp_map_[(*it_sinkpad)].org_height,
                                        vpp_comp->InputStream[stream_idx[vid_idx]]);
                } else {
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstX = output_width * (line_size - 1) / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstY = output_height - (sequence - (line_size - 1)) * output_height / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstW = output_width / line_size;
                    vpp_comp->InputStream[stream_idx[vid_idx]].DstH = output_height / line_size;
                }
            }
        }
    } else if (combo_type_ == COMBO_BLOCKS) {
        //to calculate block count for every line/column
        int block_count = 0;
        int i = 0;

        for (i = 0; ; i++) {
            if (stream_cnt > i * i && stream_cnt <= (i + 1) * (i + 1)) {
                block_count = i + 1;
                break;
            }
        }

        int block_width = output_width / block_count;
        int block_height = output_height / block_count;

        MLOG_INFO("COMBO_BLOCKS mode, %d stream(s), %dx%d, block size %dx%d\n",
               stream_cnt_, block_count, block_count, block_width, block_height);

        for (i = 0; i < stream_cnt; i++) {
            pad_idx = stream_idx[i];
            int index = 0;
            for (it_sinkpad = this->sinkpads_.begin();
                    it_sinkpad != this->sinkpads_.end(); it_sinkpad++) {
                if (index == pad_idx) {
                    break;
                } else {
                    index++;
                }
            }
            if (it_sinkpad != this->sinkpads_.end()) {
                ComputeCompPosition((i % block_count) * block_width,
                                    (i / block_count) * block_height,
                                    block_width,
                                    block_height,
                                    vpp_comp_map_[(*it_sinkpad)].org_width,
                                    vpp_comp_map_[(*it_sinkpad)].org_height,
                                    vpp_comp->InputStream[stream_idx[i]]);
            } else {
                vpp_comp->InputStream[stream_idx[i]].DstX = (i % block_count) * block_width;
                vpp_comp->InputStream[stream_idx[i]].DstY = (i / block_count) * block_height;
                vpp_comp->InputStream[stream_idx[i]].DstW = block_width;
                vpp_comp->InputStream[stream_idx[i]].DstH = block_height;
            }
        }
    } else {
        assert(0);
    }

#ifndef NDEBUG
    for (int i = 0; i < stream_cnt; i++) {
        MLOG_DEBUG("vpp_comp->InputStream[%d].DstX = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstX);
        MLOG_DEBUG("vpp_comp->InputStream[%d].DstY = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstY);
        MLOG_DEBUG("vpp_comp->InputStream[%d].DstW = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstW);
        MLOG_DEBUG("vpp_comp->InputStream[%d].DstH = %d\n", stream_idx[i], vpp_comp->InputStream[stream_idx[i]].DstH);
    }
#endif

    delete [] stream_idx;
    return 0;
}

mfxStatus VPPElement::InitVpp(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(VPP_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    assert(buf.msdk_surface);
    memcpy(&mfx_video_param_.vpp.In, &((mfxFrameSurface1 *)buf.msdk_surface)->Info,
            sizeof(mfx_video_param_.vpp.In));

    //set output frame rate
    if (!mfx_video_param_.vpp.Out.FrameRateExtN) {
        mfx_video_param_.vpp.Out.FrameRateExtN = mfx_video_param_.vpp.In.FrameRateExtN;
    }

    if (!mfx_video_param_.vpp.Out.FrameRateExtD) {
        mfx_video_param_.vpp.Out.FrameRateExtD = mfx_video_param_.vpp.In.FrameRateExtD;
    }

    MLOG_INFO("VPP Out put frame rate:%f\n", \
           1.0 * mfx_video_param_.vpp.Out.FrameRateExtN / \
           mfx_video_param_.vpp.Out.FrameRateExtD);

    //set vpp out ChromaFormat & FourCC
    if (!mfx_video_param_.vpp.Out.ChromaFormat) {
        mfx_video_param_.vpp.Out.ChromaFormat  = mfx_video_param_.vpp.In.ChromaFormat;
    }

    if (!mfx_video_param_.vpp.Out.FourCC) {
        mfx_video_param_.vpp.Out.FourCC  = mfx_video_param_.vpp.In.FourCC;
    }

    //set vpp PicStruct
    if (!mfx_video_param_.vpp.Out.PicStruct) {
        mfx_video_param_.vpp.Out.PicStruct  = mfx_video_param_.vpp.In.PicStruct;
    }

    MLOG_INFO("---Init VPP, in res %d/%d\n", mfx_video_param_.vpp.In.CropW, \
           mfx_video_param_.vpp.In.CropH);

    if (mfx_video_param_.vpp.Out.CropW > 4096 || \
            mfx_video_param_.vpp.Out.CropH > 3112 || \
            mfx_video_param_.vpp.Out.CropW <= 0 || \
            mfx_video_param_.vpp.Out.CropH <= 0) {
        MLOG_WARNING("set out put resolution: %dx%d,use original %dx%d\n", \
               mfx_video_param_.vpp.Out.CropW, \
               mfx_video_param_.vpp.Out.CropH, \
               mfx_video_param_.vpp.In.CropW, \
               mfx_video_param_.vpp.In.CropH);
        mfx_video_param_.vpp.Out.CropW = mfx_video_param_.vpp.In.CropW;
        mfx_video_param_.vpp.Out.CropH = mfx_video_param_.vpp.In.CropH;
    }

    MLOG_INFO("---Init VPP, out res %d/%d\n", mfx_video_param_.vpp.Out.CropW, \
           mfx_video_param_.vpp.Out.CropH);

    // width must be a multiple of 16.
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture.
    mfx_video_param_.vpp.Out.Width  = MSDK_ALIGN16(mfx_video_param_.vpp.Out.CropW);
    mfx_video_param_.vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfx_video_param_.vpp.Out.PicStruct) ?
                                      MSDK_ALIGN16(mfx_video_param_.vpp.Out.CropH) : MSDK_ALIGN32(mfx_video_param_.vpp.Out.CropH);

    if (m_bUseOpaqueMemory) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
    } else if (m_bLibType == MFX_IMPL_SOFTWARE) {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    } else {
        mfx_video_param_.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
    }

    mfxFrameAllocRequest VPPRequest[2];// [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = mfx_vpp_->QueryIOSurf(&mfx_video_param_, VPPRequest);
    MLOG_INFO("VPP suggestion surface is %d\n", VPPRequest[1].NumFrameSuggested);
    //TODO: try to get next element's surfaces;
    if (surface_pool_ == NULL) {
        // for reuse surface, do not reset num_of_surf_, because VPPRequest[1].NumFrameSuggested maybe change
        num_of_surf_ = VPPRequest[1].NumFrameSuggested + m_extParams.nSuggestSurface + 10;
    }
    MLOG_INFO("VPP suggest number of surfaces is in/out %d/%d\n",
           VPPRequest[0].NumFrameSuggested,
           VPPRequest[1].NumFrameSuggested);

#if defined(PLATFORM_OS_WINDOWS)
    VPPRequest[1].Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET |
                            MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_FROM_VPPOUT;
#endif

    if (surface_pool_ == NULL) {
        MLOG_INFO("Creating VPP surface pool, surface num %d\n", num_of_surf_);
        sts = AllocFrames(&VPPRequest[1], false);
        frame_width_ = VPPRequest[1].Info.CropW;
        frame_height_ = VPPRequest[1].Info.CropH;
    } else {
        //if VPP re-init without res change, don't do the following
        if (res_change_flag_) {
            //Re-use pre-allocated frames. For res changes, it decreases or increases
            //resolution up to the size of pre-allocated frames.
            unsigned i;
            for (i = 0; i < num_of_surf_; i++) {
                while (surface_pool_[i]->Data.Locked) {
                    usleep(1000);
                }

                memcpy(&(surface_pool_[i]->Info), &(VPPRequest[1].Info), sizeof(surface_pool_[i]->Info));
            }
        }
    }


    mfxExtVPPComposite vpp_comp;
    SetVppCompParam(&vpp_comp);

    if (m_bUseOpaqueMemory) {
        mfxExtBuffer *vpp_ext[2];
        ext_opaque_alloc_.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        ext_opaque_alloc_.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
        vpp_ext[0] = (mfxExtBuffer *)&ext_opaque_alloc_;
        vpp_ext[1] = (mfxExtBuffer *)&vpp_comp;
        memcpy(&ext_opaque_alloc_.In, \
               & (((mfxExtOpaqueSurfaceAlloc *)buf.ext_opaque_alloc)->Out), \
               sizeof(ext_opaque_alloc_.In));
        ext_opaque_alloc_.Out.Surfaces = surface_pool_;
        ext_opaque_alloc_.Out.NumSurface = num_of_surf_;
        ext_opaque_alloc_.Out.Type = VPPRequest[1].Type;
        MLOG_INFO("vpp surface pool %p, num_of_surf_ %d, type %x\n", surface_pool_, num_of_surf_, VPPRequest[1].Type);
        mfx_video_param_.ExtParam = vpp_ext;

        if (sinkpads_.size() == 1) {
            mfx_video_param_.NumExtParam = 1;
        } else {
            mfx_video_param_.NumExtParam = 2;
        }
    } else {
        mfx_video_param_.NumExtParam = 0;

        if (sinkpads_.size() == 1 && !keep_ratio_) {
            mfx_video_param_.ExtParam = NULL;
        } else {
            mfx_video_param_.ExtParam = (mfxExtBuffer **)pExtBuf;
            mfx_video_param_.ExtParam[mfx_video_param_.NumExtParam++] = (mfxExtBuffer *) & (vpp_comp);
        }
    }

    if(enabledFilterCount > 0) {
        if (NULL == mfx_video_param_.ExtParam) {
            mfx_video_param_.ExtParam = (mfxExtBuffer **)pExtBuf;
        }
        extDoUse.NumAlg  = enabledFilterCount;
        extDoUse.AlgList = tabDoUseAlg;
        mfx_video_param_.ExtParam[mfx_video_param_.NumExtParam++] = (mfxExtBuffer*)&(extDoUse);
    }

    if (0 != vpp_denoise.Header.BufferId) {
        if (NULL == mfx_video_param_.ExtParam) {
            mfx_video_param_.ExtParam = (mfxExtBuffer **)pExtBuf;
        }
        mfx_video_param_.ExtParam[mfx_video_param_.NumExtParam++] = (mfxExtBuffer *)&vpp_denoise;
        MLOG_INFO("vpp de-noise level is %u\n", vpp_denoise.DenoiseFactor);
    }

    sts = mfx_vpp_->Init(&mfx_video_param_);

    if (MFX_WRN_FILTER_SKIPPED == sts) {
        MLOG_INFO("------------------- Got MFX_WRN_FILTER_SKIPPED\n");
        sts = MFX_ERR_NONE;
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(VPP_INIT_TIME_STAMP, this);
        measurement->RelLock();
    }

    delete[] vpp_comp.InputStream;
    return sts;
}

mfxStatus VPPElement::DoingVpp(mfxFrameSurface1* in_surf, mfxFrameSurface1* out_surf)
{
    mfxStatus sts = MFX_ERR_NONE;
    MediaBuf out_buf;
    mfxSyncPoint syncpV;
    MediaPad *srcpad = *(this->srcpads_.begin());

    do {
        for (;;) {
            // Process a frame asychronously (returns immediately).
            sts = mfx_vpp_->RunFrameVPPAsync(in_surf,
                                             out_surf, NULL, &syncpV);
            if (MFX_ERR_NONE < sts && !syncpV) {
                if (MFX_WRN_DEVICE_BUSY == sts) {
                    if (is_running_) {
                        usleep(100);
                    } else {
                        break;
                    }
                }
            } else if (MFX_ERR_NONE < sts && syncpV) {
                // Ignore warnings if output is available.
                sts = MFX_ERR_NONE;
                break;
            } else if (MFX_ERR_MORE_DATA == sts && 1 == this->sinkpads_.size()) {
                // if not Composite case, will do VPP again
                if (is_running_) {
                    continue;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if (MFX_ERR_MORE_DATA == sts) {
            // Composite case, direct return
            break;
        }

        //If vpp connects to render, then sync the surface
        if (mfx_video_param_.vpp.Out.FourCC == MFX_FOURCC_RGB4) {
            mfx_session_->SyncOperation(syncpV, 60000);
        }

        if (MFX_ERR_NONE < sts && syncpV) {
            sts = MFX_ERR_NONE;
        }

        if (MFX_ERR_NONE == sts || MFX_ERR_MORE_SURFACE == sts) {
            out_buf.msdk_surface = out_surf;

            if (out_buf.msdk_surface) {
                // Increase the lock count within the surface
                msdk_atomic_inc16((volatile mfxU16*)(&(out_surf->Data.Locked)));
            }

            out_buf.mWidth = mfx_video_param_.vpp.Out.CropW;
            out_buf.mHeight = mfx_video_param_.vpp.Out.CropH;
            out_buf.is_eos = 0;

            if (m_bUseOpaqueMemory) {
                out_buf.ext_opaque_alloc = &ext_opaque_alloc_;
            }

            if (srcpad) {
                srcpad->PushBufToPeerPad(out_buf);
                vppframe_num_++;
            }
        }

        if (MFX_ERR_MORE_DATA == sts && NULL == syncpV) {
            // Is it last frame decoded? Do not push EOS here.
            out_buf.msdk_surface = NULL;
            out_buf.is_eos = 0;

            if (srcpad) {
                srcpad->PushBufToPeerPad(out_buf);
                vppframe_num_++;
            }
            if (MFX_ERR_MORE_SURFACE == sts && NULL != in_surf) {
                break;
            }
        }
    } while (in_surf == NULL && sts >= MFX_ERR_NONE);

    return sts;
}

int VPPElement::ProcessChainVpp(MediaBuf &buf)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *pmfxInSurface = (mfxFrameSurface1 *)buf.msdk_surface;

    if (!codec_init_) {
        sts = InitVpp(buf);

        if (MFX_ERR_NONE == sts) {
            MLOG_INFO("VPP element init successfully\n");
            codec_init_ = true;
        } else {
            MLOG_ERROR("vpp create failed %d\n", sts);
            return -1;
        }
    }

    if (buf.is_eos) {
        eos_cnt += buf.is_eos;
        //MLOG_INFO("The EOS number is:%d\n", eos_cnt);
    }
    if (stream_cnt_ == eos_cnt) {
        MLOG_INFO("All video streams EOS, can end VPP now. %d, %d\n", stream_cnt_, eos_cnt);
        WaitSrcMutex();
        MediaBuf out_buf;
        MediaPad *srcpad = *(this->srcpads_.begin());
        out_buf.msdk_surface = NULL;
        out_buf.is_eos = 1;
        if (srcpad) {
            srcpad->PushBufToPeerPad(out_buf);
        }
        ReleaseSrcMutex();
        return 1;
    }

    int nIndex = 0;

    while (is_running_) {
        nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

        if (MFX_ERR_NOT_FOUND == nIndex) {
            usleep(100);
            //return MFX_ERR_MEMORY_ALLOC;
        } else {
            break;
        }
    }

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpStart(VPP_FRAME_TIME_STAMP, this);
        measurement->RelLock();
    }

    DoingVpp(pmfxInSurface, surface_pool_[nIndex]);

    if (measurement) {
        measurement->GetLock();
        measurement->TimeStpFinish(VPP_FRAME_TIME_STAMP, this);
        measurement->RelLock();
    }

    return 0;
}

/*
 * return 0 - successful
 *        1 - all eos
 *       -1 - thread is stopped
 *        2 - failed to get buf within 10ms during initialization
 */
int VPPElement::PrepareVppCompFrames()
{
    MediaBuf buf;
    std::list<MediaPad *>::iterator it_sinkpad;
    MediaPad *sinkpad = NULL;
    unsigned int tick;

    if (0 == comp_stc_) {
        // Init stage.
        // Wait for all the frame to generate the 1st composited one.
        bool all_no_data = true;
        for (it_sinkpad = this->sinkpads_.begin();
                it_sinkpad != this->sinkpads_.end();
                ++it_sinkpad) {
            sinkpad = *it_sinkpad;

            tick = 0;
            buf.msdk_surface = NULL;
            buf.is_eos = 0;
            if(vpp_comp_map_[sinkpad].ready_surface.msdk_surface != NULL) {
                // only wait the first frame for each input
                continue;
            }
            while (sinkpad->GetBufData(buf) != 0) {
                // No data, just sleep and wait
                if (!is_running_) {
                    return -1;
                }
                usleep(1000);
                tick++;
                //wait for ~10ms and return, or else it may hold the sink mutex infinitely
                if (tick > 10) {
                    break;
                }
            }

            // Set corresponding framerate in VPPCompInfo
            if (vpp_comp_map_.find(sinkpad) == vpp_comp_map_.end()) {
                assert(0);
            }

            mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
            if (msdk_surface) {
                vpp_comp_map_[sinkpad].ready_surface = buf;
                vpp_comp_map_[sinkpad].org_width = msdk_surface->Info.Width;
                vpp_comp_map_[sinkpad].org_height = msdk_surface->Info.Height;

                if (!vpp_comp_map_[sinkpad].str_info.text &&
                        !vpp_comp_map_[sinkpad].pic_info.bmp &&
                        !vpp_comp_map_[sinkpad].raw_info.raw_file) {
                    // Start composite, when one of the video stream has been coming.
                    all_no_data = false;
                }
                if (msdk_surface->Info.FrameRateExtD > 0) {
                    vpp_comp_map_[sinkpad].frame_rate = (float)msdk_surface->Info.FrameRateExtN;
                    vpp_comp_map_[sinkpad].frame_rate /= (float)msdk_surface->Info.FrameRateExtD;
                    MLOG_INFO("sinkpad %p, framerate is %f, %u,%u\n",
                    sinkpad,
                    vpp_comp_map_[sinkpad].frame_rate,
                    msdk_surface->Info.FrameRateExtN,
                    msdk_surface->Info.FrameRateExtD);
                } else {
                    // Set default frame rate as 30.
                    vpp_comp_map_[sinkpad].frame_rate = 30;
                }

                if (vpp_comp_map_[sinkpad].frame_rate > max_comp_framerate_) {
                    max_comp_framerate_ = vpp_comp_map_[sinkpad].frame_rate;
                    MLOG_INFO("------- Set max_comp_framerate_ to %f\n", max_comp_framerate_);
                }
            } else {
                //if it goes here, means that decoder was stopped before
                //it generated and put the first valid surface to vpp
                if (buf.is_eos) {
                    MLOG_INFO("The first surface from decoder is eos\n");
                }
            }
        }
        if(all_no_data){
            return 2;
        }
    } else {
        // TODO:
        // How to end, what if input surface is NULL.
        bool out_of_time = false;
        int pad_cursor = 1;
        int get_buf_ret = 0;
#if not defined SUPPORT_SMTA
        int pad_size = 0;
        if (combo_type_ == COMBO_CUSTOM)
            pad_size = dec_region_list_.size();
        else
            pad_size = this->sinkpads_.size();
#endif

        if (this->sinkpads_.size() == 0 ) {
            MLOG_ERROR("sinkpads_.size() [%d] dec_region_list_.size() [%d] combo_type_ [%d]\n",sinkpads_.size(), dec_region_list_.size(), combo_type_);
            return -1;
        }
        // for some custom layout
        // first call MsdkXcoder::SetComboType
        // second call MsdkXcoder::SetCustomLayout
        if (dec_region_list_.size() == 0 && combo_type_ == COMBO_CUSTOM) {
            return 2;
        }

#if not defined SUPPORT_SMTA
        unsigned pad_com_start = GetSysTimeInUs();
        int pad_comp_time = (FRAME_RATE_CTRL_TIMER / (max_comp_framerate_ + FRAME_RATE_CTRL_PADDING)) - (pad_com_start - comp_stc_);
        int pad_comp_timer = (pad_comp_time < 0) ? 0 : pad_comp_time / pad_size;
        int pad_max_level = 0;
        bool is_alpha_exist = false;
#endif

        if (combo_type_ == COMBO_CUSTOM) {
            CustomLayout::iterator it = dec_region_list_.begin();
            for (; it != dec_region_list_.end(); ++it, ++pad_cursor) {
                MediaPad *pad = (MediaPad *)it->handle;
#if not defined SUPPORT_SMTA
                if (vpp_comp_map_[pad].str_info.text || vpp_comp_map_[pad].pic_info.bmp) {
                    is_alpha_exist = true;
                }
#endif
                if ((vpp_comp_map_[pad].ready_surface.msdk_surface == NULL && \
                        vpp_comp_map_[pad].ready_surface.is_eos)) {
                    continue;
                }
                get_buf_ret = GetCompBufFromPad(pad, out_of_time
#if not defined SUPPORT_SMTA
                    , pad_max_level
                    , pad_com_start
                    , pad_comp_timer
                    , pad_cursor
#endif
                );
                if (get_buf_ret != 0) {
                    return get_buf_ret;
                }
            }
        } else {
            for (it_sinkpad = this->sinkpads_.begin();
                    it_sinkpad != this->sinkpads_.end();
                    ++it_sinkpad, ++pad_cursor) {
                sinkpad = *it_sinkpad;
#if not defined SUPPORT_SMTA
                if (vpp_comp_map_[sinkpad].str_info.text || vpp_comp_map_[sinkpad].pic_info.bmp) {
                    is_alpha_exist = true;
                }
#endif

                if ((vpp_comp_map_[sinkpad].ready_surface.msdk_surface == NULL && \
                        vpp_comp_map_[sinkpad].ready_surface.is_eos)) {
                    continue;
                }

                get_buf_ret = GetCompBufFromPad(sinkpad, out_of_time
#if not defined SUPPORT_SMTA
                    , pad_max_level
                    , pad_com_start
                    , pad_comp_timer
                    , pad_cursor
#endif
                );
                if (get_buf_ret != 0) {
                    return get_buf_ret;
                }
            }
        }

#if not defined SUPPORT_SMTA
        unsigned pad_comp_end = GetSysTimeInUs();
        int need_sleep = 0;
        need_sleep = pad_size * pad_comp_timer - (pad_comp_end - pad_com_start);
#if defined LIVE_STREAM
        // In living stream, process video data with real time
        if (!is_alpha_exist && pad_max_level >= FRAME_POOL_LOW_WATER_LEVEL && need_sleep > 0) {
            need_sleep = 0;
        }
#endif
        if (need_sleep > 0) {
            usleep(need_sleep);
        }
#endif
    }

    comp_stc_ = GetSysTimeInUs();
    return 0;
}

int VPPElement::HandleProcessVpp()
{
    std::list<MediaPad *>::iterator it_sinkpad;
    mfxStatus sts = MFX_ERR_NONE;
    int nIndex = 0;
    bool all_eos = false;

    SetThreadName("VPP Thread");

    while (is_running_) {
        usleep(100);
        WaitSinkMutex();

        if (0 == current_comp_number_) {
            current_comp_number_ = sinkpads_.size();
        }

        if (0 == current_comp_number_) {
            MLOG_ERROR("---------something wrong here, all sinkpads detached\n");
            ReleaseSinkMutex();
            break;
        }

        int prepare_result = PrepareVppCompFrames();
        if (prepare_result == -1) {
            ReleaseSinkMutex();
            //break, then to return the bufs in in_buf_q
            break;
        } else if (prepare_result == 1) {
            ReleaseSinkMutex();
            all_eos = true;
            is_running_ = false;
            break;
        } else if (prepare_result == 2) {
            //release mutex for now
            ReleaseSinkMutex();
            continue;
        }

        if (!codec_init_) {
            for (it_sinkpad = sinkpads_.begin(); it_sinkpad != sinkpads_.end(); ++it_sinkpad) {
                //find the first valid surface to initialize vpp
                if (vpp_comp_map_[*it_sinkpad].ready_surface.msdk_surface) {
                    break;
                }
            }
            if (it_sinkpad == sinkpads_.end()) {
                //decoders eos before any valid surface arrives.
                //for this case, stop the vpp directly.
                ReleaseSinkMutex();
                all_eos = true;
                is_running_ = false;
                break;
            }

            sts = InitVpp(vpp_comp_map_[*it_sinkpad].ready_surface);

            if (MFX_ERR_NONE == sts) {
                MLOG_INFO("[%p]VPP element init successfully\n", this);
                codec_init_ = true;
                //initialized, so set the following flags as false. (they mey be set before playing)
                comp_info_change_ = false;
                reinit_vpp_flag_ = false;
                res_change_flag_ = false;
            } else {
                MLOG_ERROR("VPP create failed: %d\n", sts);
                ReleaseSinkMutex();
                return -1;
            }
        }

        if ((current_comp_number_ != sinkpads_.size()) || comp_info_change_ ||
            reinit_vpp_flag_ || res_change_flag_) {
            // Re-init VPP.
            unsigned int before_change = GetSysTimeInUs();
            MLOG_INFO("stop/init vpp comp_info_change_[%d] reinit_vpp_flag_[%d] res_change_flag_[%d]\n", comp_info_change_, reinit_vpp_flag_, res_change_flag_);
            mfx_vpp_->Close();
            MLOG_INFO("Re-init VPP...\n");
            bool wait_decoder_reset = false;
            //find valid surface to init vpp
            for (it_sinkpad = sinkpads_.begin(); it_sinkpad != sinkpads_.end(); ++it_sinkpad) {
                // if decoder is resetting, the vpp must not stop.
                if (vpp_comp_map_[*it_sinkpad].is_decoder_reset) {
                    wait_decoder_reset = true;
                }
                if (vpp_comp_map_[*it_sinkpad].ready_surface.msdk_surface)
                    break;
            }
            //assert(it_sinkpad != sinkpads_.end());
            if (it_sinkpad == sinkpads_.end()) {
                if (wait_decoder_reset) {
                    // the the reset decoder deocdes new frame
                    MLOG_INFO("all inputs are null surfaces, wait decode reset\n");
                    ReleaseSinkMutex();
                    continue;
                } else {
                    //happens: p d 1eos u 2eos v
                    //TODO: need to refine the exit logic for VPP
                    MLOG_ERROR("all inputs are null surfaces, exit\n");
                    is_running_ = false;
                    ReleaseSinkMutex();
                    break;
                }
            }

            //initialize vpp
            sts = InitVpp(vpp_comp_map_[*it_sinkpad].ready_surface);
            // find the max frame rate
            max_comp_framerate_ = 0;
            for (it_sinkpad = sinkpads_.begin(); it_sinkpad != sinkpads_.end(); ++it_sinkpad) {
                if (vpp_comp_map_[*it_sinkpad].ready_surface.msdk_surface) {
                    mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)vpp_comp_map_[*it_sinkpad].ready_surface.msdk_surface;
                    if (msdk_surface->Info.FrameRateExtD > 0) {
                        vpp_comp_map_[*it_sinkpad].frame_rate = (float)msdk_surface->Info.FrameRateExtN;
                        vpp_comp_map_[*it_sinkpad].frame_rate /= (float)msdk_surface->Info.FrameRateExtD;
                    } else {
                        // Set default frame rate as 30.
                        vpp_comp_map_[*it_sinkpad].frame_rate = 30;
                    }
                    if (vpp_comp_map_[*it_sinkpad].frame_rate > max_comp_framerate_) {
                        max_comp_framerate_ = vpp_comp_map_[*it_sinkpad].frame_rate;
                    }
                }
            }
            // Set default frame rate
            if (max_comp_framerate_ == 0) {
                max_comp_framerate_ = 30;
            }
            current_comp_number_ = sinkpads_.size();
            comp_info_change_ = false;
            reinit_vpp_flag_ = false;
            res_change_flag_ = false;
            MLOG_INFO("Re-Init VPP takes time %u(us)\n", GetSysTimeInUs() - before_change);
        }

        while (is_running_) {
            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

            if (MFX_ERR_NOT_FOUND == nIndex) {
                usleep(10000);
                //return MFX_ERR_MEMORY_ALLOC;
            } else {
                break;
            }
        }
        if (nIndex == MFX_ERR_NOT_FOUND) {
            //is_running_ is false, so element is stopped when it was waiting for free surface.
            //break then return queued buffers.
            ReleaseSinkMutex();
            break;
        }

        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpStart(VPP_FRAME_TIME_STAMP, this);
            measurement->RelLock();
        }
        // because set COMBO_CUSTOM layout need two steps(call ConfigVppCombo/SetCompRegion function)
        // so when program go there and the second step not excute, layout use the old one.
        // TODO need merge ConfigVppCombo/SetCompRegion in next release
        if (combo_type_ == COMBO_CUSTOM && dec_region_list_.size() == sinkpads_.size()) {
            CustomLayout::iterator it = dec_region_list_.begin();
            for (; it != dec_region_list_.end(); ++it) {
                MediaPad *pad = (MediaPad *)it->handle;
                if (vpp_comp_map_.find(pad) == vpp_comp_map_.end()) {
                    continue;
                }
                do {
                    sts = DoingVpp((mfxFrameSurface1 *)vpp_comp_map_[pad].ready_surface.msdk_surface,
                                   surface_pool_[nIndex]);
                    if (MFX_ERR_MORE_SURFACE == sts) {
                        MLOG_INFO("COMBO_CUSTOM layout need more output surface\n");
                        while (is_running_) {
                            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);
                            if (MFX_ERR_NOT_FOUND == nIndex) {
                                usleep(10000);
                            } else {
                                break;
                            }
                        }
                        if (nIndex == MFX_ERR_NOT_FOUND) {
                            //is_running_ is false, so element is stopped when it was waiting for free surface.
                            //break then return queued buffers.
                            break;
                        }
                    }
                } while (MFX_ERR_MORE_SURFACE == sts);
                if (nIndex == MFX_ERR_NOT_FOUND) {
                    // if output surface invalid, then break loop
                    break;
                }
            }
        } else {
            for (it_sinkpad = this->sinkpads_.begin();
                 it_sinkpad != this->sinkpads_.end();
                 ++it_sinkpad) {
                do {
                    sts = DoingVpp((mfxFrameSurface1 *)vpp_comp_map_[*it_sinkpad].ready_surface.msdk_surface,
                               surface_pool_[nIndex]);
                    if (MFX_ERR_MORE_SURFACE == sts) {
                        MLOG_INFO("COMBO_OTHER layout need more output surface\n");
                        while (is_running_) {
                            nIndex = GetFreeSurfaceIndex(surface_pool_, num_of_surf_);

                            if (MFX_ERR_NOT_FOUND == nIndex) {
                                usleep(10000);
                            } else {
                                break;
                            }
                        }
                        if (nIndex == MFX_ERR_NOT_FOUND) {
                            //is_running_ is false, so element is stopped when it was waiting for free surface.
                            //break then return queued buffers.
                            break;
                        }
                    }
                } while (MFX_ERR_MORE_SURFACE == sts);
                if (nIndex == MFX_ERR_NOT_FOUND) {
                    // if output surface invalid, then break loop
                    break;
                }
            }
        }
        if (measurement) {
            measurement->GetLock();
            measurement->TimeStpFinish(VPP_FRAME_TIME_STAMP, this);
            measurement->RelLock();
        }
        ReleaseSinkMutex();
    }

    if (!is_running_) {
        //handle both two cases: stopped by EOS and before EOS
        //from every sinkpad.
        WaitSrcMutex();
        MediaBuf out_buf;
        MediaPad *srcpad = *(this->srcpads_.begin());
        //sending the null buf might be useless, as encoder would be stopped immediately after vpp
        //and it would return this null buf back without processing it.
        out_buf.msdk_surface = NULL;
        out_buf.is_eos = 1;
        if (srcpad) {
            srcpad->PushBufToPeerPad(out_buf);
            vppframe_num_++;
        }
        ReleaseSrcMutex();

        WaitSinkMutex();
        for (it_sinkpad = this->sinkpads_.begin(); \
            it_sinkpad != this->sinkpads_.end(); \
            ++it_sinkpad) {
            MediaPad *sinkpad = *it_sinkpad;
            if (sinkpad) {
                if (all_eos) {
                    if (vpp_comp_map_[sinkpad].str_info.text || vpp_comp_map_[sinkpad].pic_info.bmp
#if defined ENABLE_RAW_DECODE
                                 || NULL != vpp_comp_map_[sinkpad].raw_info.raw_file //this sinkpad corresponds to video stream
#endif
                        ) {
                        vpp_comp_map_[sinkpad].ready_surface.is_eos = 1;
                    }
                }
                sinkpad->ReturnBufToPeerPad(vpp_comp_map_[sinkpad].ready_surface);
                while (sinkpad->GetBufQueueSize() > 0) {
                    MediaBuf return_buf;
                    sinkpad->GetBufData(return_buf);
                    sinkpad->ReturnBufToPeerPad(return_buf);
                }
            }
        }
        ReleaseSinkMutex();
    } else {
        is_running_ = false;
    }

    return 0;
}

int VPPElement::GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool,
                                   mfxU16 nPoolSize)
{
    if (pSurfacesPool) {
        for (mfxU16 i = 0; i < nPoolSize; i++) {
            if (0 == pSurfacesPool[i]->Data.Locked) {
                return i;
            }
        }
    }

    return MFX_ERR_NOT_FOUND;
}

void VPPElement::PadRemoved(MediaPad *pad)
{
    if (ELEMENT_VPP != element_type_) {
        if(MEDIA_PAD_SINK == pad->get_pad_direction()) {
            //if this assert hits, check if the element is stopped before it's unlinked
            assert(pad->GetBufQueueSize() == 0);
        }
        return;
    }

    if (MEDIA_PAD_SINK == pad->get_pad_direction()) {
        // VPP remove an input, may be N:1 --> N-1:1
        MLOG_INFO("--------------vpp element %p, Pad %p removed----\n", this, pad);
        // vpp is unlinked with its prev element when it's still running, to return the queued
        // buffers below
        if (is_running_) {
            //Return the ready_surface
            pad->ReturnBufToPeerPad(vpp_comp_map_[pad].ready_surface);
            //Return the bufs in in_buf_q
            MediaBuf return_buf;
            while (pad->GetBufQueueSize() > 0) {
                pad->GetBufData(return_buf);
                pad->ReturnBufToPeerPad(return_buf);
            }
        }

        vpp_comp_map_.erase(pad);

        //remove this pad from dec_region_list_
        if (combo_type_ == COMBO_CUSTOM) {
            CustomLayout::iterator it = dec_region_list_.begin();
            for (; it != dec_region_list_.end(); it++) {
                if (it->handle == (void *)pad) {
                    dec_region_list_.erase(it);
                    break;
                }
            }
            assert(it != dec_region_list_.end());
        }

        //if master is removed
        if (combo_type_ == COMBO_MASTER) {
            assert(pad->get_pad_status() == MEDIA_PAD_LINKED);
            if (pad->get_peer_pad()->get_parent() == combo_master_) {
                combo_master_ = NULL;
            }
        }
    }

    return;
}


void VPPElement::NewPadAdded(MediaPad *pad)
{
    // Only cares about VPP input streams, which means ELEMENT_VPP's sink pads.
    if (ELEMENT_VPP != element_type_) {
        return;
    }

    if (MEDIA_PAD_SINK == pad->get_pad_direction()) {
        // VPP get an input, may be N:1 --> N+1:1
        MLOG_INFO("--------------vpp element %p, New Pad %p added ----\n", this, pad);
        VPPCompInfo pad_info;
        pad_info.vpp_sinkpad = pad;
        //pad_info.dst_rect is used in COMBO_CUSTOM mode only.
        //When pad is newly added, don't display it.
        pad_info.dst_rect.x = 0;
        pad_info.dst_rect.y = 0;
        pad_info.dst_rect.w = 1;
        pad_info.dst_rect.h = 1;
        pad_info.ready_surface.msdk_surface = NULL;
        pad_info.ready_surface.is_eos = 0;
        pad_info.org_height = 0;
        pad_info.org_width = 0;
        pad_info.is_decoder_reset = false;

        if (input_str_) {
            pad_info.str_info = *input_str_;
        }
        if (input_pic_) {
            pad_info.pic_info = *input_pic_;
        }
#if defined ENABLE_RAW_DECODE
        if (input_raw_ != NULL) {
            pad_info.raw_info = *input_raw_;
        }
#endif

        vpp_comp_map_[pad] = pad_info;
    }

    return;
}

// pre_element is the element linked to VPP
int VPPElement::SetVPPCompRect(BaseElement *pre_element, VppRect *rect)
{
    if (!pre_element || !rect) {
        return -1;
    }

    if (0 == rect->w || 0 == rect->h) {
        MLOG_ERROR("invalid dst rect\n");
        return -1;
    }

    std::map<MediaPad *, VPPCompInfo>::iterator it_comp_info;

    for (it_comp_info = vpp_comp_map_.begin();
            it_comp_info != vpp_comp_map_.end();
            ++it_comp_info) {
        MediaPad *vpp_sink_pad = it_comp_info->first;

        if (MEDIA_PAD_UNLINKED == vpp_sink_pad->get_pad_status()) {
            // should not happen
            assert(0);
            continue;
        }

        if (pre_element == vpp_sink_pad->get_peer_pad()->get_parent()) {
            MLOG_INFO("Set element %p's vpp rect, %d/%d/%d/%d\n",
                   pre_element, rect->x, rect->y, rect->w, rect->h);
            it_comp_info->second.dst_rect = *rect;
            return 0;
        }
    }

    return -1;
}

mfxStatus VPPElement::AllocFrames(mfxFrameAllocRequest *pRequest, bool isDecAlloc)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU16 i;
    pRequest->NumFrameMin = num_of_surf_;
    pRequest->NumFrameSuggested = num_of_surf_;
    //mfxFrameAllocResponse *pResponse = isDecAlloc ? &m_mfxDecResponse : &m_mfxEncResponse;
    mfxFrameAllocResponse *pResponse = &m_mfxEncResponse;

    if (!m_bUseOpaqueMemory) {
        sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, pRequest, pResponse);

        if (sts != MFX_ERR_NONE) {
            MLOG_ERROR("AllocFrame failed %d\n", sts);
            return sts;
        }
    }
    surface_pool_ = new mfxFrameSurface1*[num_of_surf_];
#ifdef ENABLE_VA
    extbuf_ = new mfxExtBuffer*[num_of_surf_];
#endif
    for (i = 0; i < num_of_surf_; i++) {
        surface_pool_[i] = new mfxFrameSurface1;
        memset(surface_pool_[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(surface_pool_[i]->Info), &(pRequest->Info), sizeof(surface_pool_[i]->Info));

        if (!m_bUseOpaqueMemory) {
            surface_pool_[i]->Data.MemId = pResponse->mids[i];
        }
#ifdef ENABLE_VA
        VAPluginData* plugData = new VAPluginData;
        extbuf_[i] = (mfxExtBuffer*)(plugData);
        plugData->Out = new FrameMeta;
        plugData->In = NULL;
        plugData->Header.BufferId = MFX_EXTBUFF_VA_PLUGIN_PARAM;
        plugData->Header.BufferSz = sizeof(VAPluginData);
        surface_pool_[i]->Data.NumExtParam = 1;
        surface_pool_[i]->Data.ExtParam = (mfxExtBuffer**)(&(extbuf_[i]));
#endif
    }
    return MFX_ERR_NONE;
}

/*
 * Set/re-set resolution, it applys to VPP only, encoder will be updated when new frame arrives
 * Returns: 0 - Successful
 *          1 - Warning, it doesn't apply to non-VPP element
 */
int VPPElement::SetRes(unsigned int width, unsigned int height)
{
    if (!codec_init_) {
        mfx_video_param_.vpp.Out.CropW = width;
        mfx_video_param_.vpp.Out.CropH = height;
        return 0;
    }

    if (width > frame_width_) {
        MLOG_WARNING(" specified width %d exceeds max supported width %d.\n", width, frame_width_);
        width = frame_width_;
    }
    if (height > frame_height_) {
        MLOG_WARNING(" specified height %d exceeds max supported height %d.\n", height, frame_height_);
        height = frame_height_;
    }

    if (width != mfx_video_param_.vpp.Out.CropW ||
        height != mfx_video_param_.vpp.Out.CropH) {
        MLOG_INFO(" reset vpp res to %dx%d\n", width, height);
        mfx_video_param_.vpp.Out.CropW = width;
        mfx_video_param_.vpp.Out.CropH = height;

        res_change_flag_ = true;
    } else {
        MLOG_INFO(" specified res is the same as current one, ignored\n");
    }

    return 0;
}

/*
 * This function sets the information of the rendering objects, including: which inputs
 * to render(so maybe not all attached inputs are rendered); rendering area; z-order to render, etc
 * z-order to render is based on the sequence in input list "layout".
 * Returns: 0 - Successful
 *          1 - Warning, it does't apply to non-vpp element
 *          2 - Warning, VPP is not in COMBO_CUSTOM mode
 *         -1 - No valid inputs to composite in VPP
 */
int VPPElement::SetCompRegion(const CustomLayout *layout)
{
    if (ELEMENT_VPP != element_type_) {
        MLOG_WARNING("VPPElement::SetCompRegion is for ELEMENT_VPP only\n");
        return 1;
    }

    if (COMBO_CUSTOM != combo_type_) {
        MLOG_WARNING(" VPP is not in COMBO_CUSTOM mode, this call is invalid\n");
        return 2;
    }

    WaitSinkMutex();
    //first, clear dec_region_list_
    dec_region_list_.clear();
    std::list<MediaPad *>::iterator it_sinkpad;
    void *handle = NULL;
    //map dec_dis to sink_pad
    for (CustomLayout::const_iterator it = layout->begin(); it != layout->end(); ++it) {
        //check if the input dispatcher handle is valid and attached to vpp
        for (it_sinkpad = this->sinkpads_.begin(); it_sinkpad != this->sinkpads_.end(); ++it_sinkpad) {
            handle = (*it_sinkpad)->get_peer_pad()->get_parent();
            if (it->handle == handle) break;
        }
        assert(it_sinkpad != this->sinkpads_.end());

        //no need to check if the region info is valid, it was done in MsdkXcoder

        //add one instance in dec_region_list_
        if (it_sinkpad != this->sinkpads_.end()) {
            CompRegion input = {*it_sinkpad, it->region};
            dec_region_list_.push_back(input);
        } else {
            MLOG_ERROR(" input handle %p is invalid\n", it->handle);
        }
    }

    if (dec_region_list_.size() == 0) {
        MLOG_ERROR(" no valid inputs to composite in VPP\n");
        ReleaseSinkMutex();
        return -1;
    } else {
        if (codec_init_)
            reinit_vpp_flag_ = true;

        ReleaseSinkMutex();
        return 0;
    }
}

int VPPElement::SetBgColor(BgColorInfo *bgColorInfo)
{
    if (bgColorInfo) {
        bg_color_info.Y = bgColorInfo->Y;
        bg_color_info.U = bgColorInfo->U;
        bg_color_info.V = bgColorInfo->V;
        reinit_vpp_flag_ = true;
    } else {
        MLOG_ERROR("Invalid input parameters\n");
        return -1;
    }
    return 0;
}

/**
 * This function sets the flag of the keep width/height ratio
 * isKeepRatio : true keep the origin video width/height ratio.
 */
int VPPElement::SetKeepRatio(bool isKeepRatio)
{
    if (keep_ratio_ != isKeepRatio) {
        keep_ratio_ = isKeepRatio;
        reinit_vpp_flag_ = true;
    }
    return 0;
}

void VPPElement::ComputeCompPosition(unsigned int x,
                                     unsigned int y,
                                     unsigned int width,
                                     unsigned int height,
                                     unsigned int org_width,
                                     unsigned int org_height,
                                     mfxVPPCompInputStream &inputStream)
{
    MLOG_INFO("x[%u], y[%u], width[%u], height[%u], org_width[%u], org_height[%u]\n", x, y, width, height, org_width, org_height);
    if (!keep_ratio_ || org_width == 0 || org_height == 0 || width == 0 || height == 0 || (float)org_width/org_height == (float)width/height) {
        inputStream.DstX = x;
        inputStream.DstY = y;
        inputStream.DstW = width;
        inputStream.DstH = height;
        return;
    }
    float width_ratio = (float)org_width/width;
    float height_ratio = (float)org_height/height;
    unsigned int dst_width = 0;
    unsigned int dst_height = 0;
    unsigned int dst_x = x;
    unsigned int dst_y = y;
    if (width_ratio > height_ratio) {
        dst_width = width;
        dst_height = (unsigned int)round(((float)org_height / width_ratio));
        if (height - dst_height > 0) {
            dst_y += (height - dst_height) / 2;
        }
    } else {
        dst_width = (unsigned int)round(((float)org_width / height_ratio));
        dst_height = height;
        if (width - dst_width > 0) {
            dst_x += (width - dst_width) / 2;
        }
    }
    inputStream.DstX = dst_x;
    inputStream.DstY = dst_y;
    inputStream.DstW = dst_width;
    inputStream.DstH = dst_height;
}

int VPPElement::GetCompBufFromPad(MediaPad *pad, bool &out_of_time
#if not defined SUPPORT_SMTA
    , int &pad_max_level
    , unsigned pad_com_start
    , int pad_comp_timer
    , int pad_cursor
#endif
)
{
    MediaBuf buf;
#if not defined SUPPORT_SMTA
    int pad_size = 0;
    if (combo_type_ == COMBO_CUSTOM)
        pad_size = dec_region_list_.size();
    else
        pad_size = this->sinkpads_.size();
#endif
    if (this->sinkpads_.size() == 0 ) {
        MLOG_ERROR("GetCompBufFromPad sinkpads_.size() [%d] dec_region_list_.size() [%d] combo_type_ [%d]\n",sinkpads_.size(), dec_region_list_.size(), combo_type_);
        return -1;
    }

    // for some custom layout
    // first call MsdkXcoder::SetComboType
    // second call MsdkXcoder::SetCustomLayout
    if (dec_region_list_.size() == 0 && combo_type_ == COMBO_CUSTOM) {
        return 2;
    }
    buf.is_eos = 0;
    buf.msdk_surface = NULL;
#if not defined SUPPORT_SMTA
    unsigned int tick = 0;
#endif

    while (pad->GetBufData(buf) != 0) {
        // No data, just sleep and wait
        if (!is_running_) {
            //return -1 and then it would return the queued buffers in HandleProcessVpp()
            return -1;
        }
#if not defined SUPPORT_SMTA
        unsigned tmp_cur_time = GetSysTimeInUs();

        if (tmp_cur_time - pad_com_start > pad_cursor * pad_comp_timer) {
            if (vpp_comp_map_[pad].ready_surface.msdk_surface == NULL &&
                vpp_comp_map_[pad].ready_surface.is_eos == 0) {
                // Newly attached stream doesn't have decoded frame yet, wait...
                //MLOG_INFO("Newly attached stream doesn't have decoded frame yet, wait...\n");
                tick++;
                //wait for ~10ms and return, or else it "may" hold the sink mutex infinitely
                //if when decoder is attached, but never send any frame (even EOS)
                if (tick > 50) break;
            } else {
                out_of_time = true;
                MLOG_WARNING("[%p]-frame comes late from pad %p, diff %u, framerate %f\n",
                       this, pad, tmp_cur_time - comp_stc_, max_comp_framerate_);
                break;
            }
        }
#endif
        usleep(200);
    }
    // In live streaming mode, dynamic attach input stream, will get stream info at here.
    mfxFrameSurface1 *msdk_surface = (mfxFrameSurface1 *)buf.msdk_surface;
    if (msdk_surface) {
        if (vpp_comp_map_[pad].is_decoder_reset &&
                msdk_surface->Info.Width != 0 && msdk_surface->Info.Height != 0) {
            MLOG_INFO("decoder reset sinkpad[%p] org width/height msdk_surface->Info.Width[%u] msdk_surface->Info.Height[%u]\n",pad, msdk_surface->Info.Width, msdk_surface->Info.Height);
            vpp_comp_map_[pad].org_width = msdk_surface->Info.Width;
            vpp_comp_map_[pad].org_height = msdk_surface->Info.Height;
            vpp_comp_map_[pad].is_decoder_reset = false;
        }
        if (vpp_comp_map_[pad].org_width == 0 && vpp_comp_map_[pad].org_height == 0 &&
                msdk_surface->Info.Width != 0 && msdk_surface->Info.Height != 0) {
            MLOG_INFO("set sinkpad[%p] org width/height msdk_surface->Info.Width[%u] msdk_surface->Info.Height[%u]\n",pad, msdk_surface->Info.Width, msdk_surface->Info.Height);
            vpp_comp_map_[pad].org_width = msdk_surface->Info.Width;
            vpp_comp_map_[pad].org_height = msdk_surface->Info.Height;
            reinit_vpp_flag_ = true;
        }
    } else if (buf.is_decoder_reset) {
        // receive the buffer from decoder,
        // is_decoder_reset flag is true, meaning the decode is reset
        // vpp need be reset for release decoder surfaces
        vpp_comp_map_[pad].is_decoder_reset = true;
        vpp_comp_map_[pad].org_width = buf.mWidth;
        vpp_comp_map_[pad].org_height = buf.mHeight;
        // reinit vpp for release decoder surfaces.
        reinit_vpp_flag_ = true;
        MLOG_INFO("decoder send reset buffer out_of_time[%d]\n", out_of_time);
    }
    if (!vpp_comp_map_[pad].str_info.text && !vpp_comp_map_[pad].pic_info.bmp) { //this sinkpad corresponds to video stream
        if (buf.is_eos) {
            eos_cnt += buf.is_eos;
        }
        if (stream_cnt_ == eos_cnt) {
            MLOG_INFO("All video streams EOS, can end VPP now. stream_cnt: %d, eos_cnt: %d\n", stream_cnt_, eos_cnt);
            stream_cnt_ = 0;
            return 1;
        }
    }
    if (out_of_time) {
        // Frames comes late.
        // Use last surface, drop 1 frame in future.
        vpp_comp_map_[pad].drop_frame_num++;
        out_of_time = false;
    } else {
        // Update ready surface and release last surface.
        pad->ReturnBufToPeerPad(vpp_comp_map_[pad].ready_surface);
        vpp_comp_map_[pad].ready_surface = buf;

        // Check if need to drop frame.
        while (vpp_comp_map_[pad].drop_frame_num > 0) {
            if (pad->GetBufQueueSize() > 1) {
                // Drop frames.
                // Only drop frame if it has more than 1, don't drop all of them
                MediaBuf drop_buf;
                pad->GetBufData(drop_buf);
                pad->ReturnBufToPeerPad(drop_buf);
                vpp_comp_map_[pad].total_dropped_frames++;
                MLOG_INFO("[%p]-sinkpad %p drop 1 frame\n", this, pad);
                vpp_comp_map_[pad].drop_frame_num--;
            } else {
                break;
            }
        }
    }
#if not defined SUPPORT_SMTA
    int bufCount = pad->GetBufQueueSize();
    if (bufCount > pad_max_level) {
        pad_max_level = bufCount;
    }
#endif

    return 0;
}

int VPPElement::ProcessChain(MediaPad *pad, MediaBuf &buf)
{
    return ProcessChainVpp(buf);
}

int VPPElement::HandleProcess()
{
    return HandleProcessVpp();
}
