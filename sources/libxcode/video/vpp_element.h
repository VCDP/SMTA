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
#ifndef VPP_ELEMENT_H
#define VPP_ELEMENT_H
#include <stdlib.h>
#include <map>
#include "msdk_codec.h"

typedef struct VPPCompInfo {
    MediaPad *vpp_sinkpad;
    VppRect dst_rect;
    StringInfo str_info;
    PicInfo pic_info;
    RawInfo raw_info;

    MediaBuf ready_surface;
    //mfxFrameSurface1* ready_surface;
    float frame_rate;
    unsigned int drop_frame_num; // Number of frames need to be dropped.
    unsigned int total_dropped_frames; // Totally dropped frames.
    unsigned int org_width;
    unsigned int org_height;
    bool is_decoder_reset;

    VPPCompInfo():
    vpp_sinkpad(NULL),
    frame_rate(0),
    drop_frame_num(0),
    total_dropped_frames(0),
    org_width(0),
    org_height(0),
    is_decoder_reset(false) {
        dst_rect.x = dst_rect.y = dst_rect.w = dst_rect.h = 0;
    };
} VPPCompInfo;

class VPPElement: public MSDKCodec
{
public:
    DECLARE_MLOGINSTANCE();
    VPPElement(ElementType type,MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL, CodecEventCallback *callback = NULL);
    ~VPPElement();
    // BaseElement
    virtual bool Init(void *cfg, ElementMode element_mode);


    // IMSDKCodec
    //virtual void SetForceKeyFrame() = 0;

    //virtual void SetResetBitrateFlag() = 0;
    virtual int SetRes(unsigned int width, unsigned int height);
    virtual int SetVPPCompRect(BaseElement *element, VppRect *rect);
    virtual int ConfigVppCombo(ComboType combo_type, void *master_handle);
    virtual int SetCompRegion(const CustomLayout *layout);

    virtual int SetBgColor(BgColorInfo *bgColorInfo);
    virtual int SetKeepRatio(bool isKeepRatio);
    virtual void SetStrInfo(StringInfo *strinfo) {
        input_str_ = strinfo;
    }
    virtual void SetCompInfoChe(bool ifchange) {
        comp_info_change_ = ifchange;
    }
    virtual unsigned long GetNumOfVppFrm() {
        return vppframe_num_;
    }
    virtual unsigned int GetVppOutFpsN() {
        return mfx_video_param_.vpp.Out.FrameRateExtN;
    }
    virtual unsigned int GetVppOutFpsD() {
        return mfx_video_param_.vpp.Out.FrameRateExtD;
    }

    void SetPicInfo(PicInfo *picinfo) {
        input_pic_ = picinfo;
    }

    virtual void SetRawInfo(RawInfo *rawinfo) {
        input_raw_ = rawinfo;
    }
    virtual int QueryStreamCnt() {
        return stream_cnt_;
    }

protected:
    //BaseElement
    virtual void NewPadAdded(MediaPad *pad);
    virtual void PadRemoved(MediaPad *pad);
    virtual int HandleProcess();

    virtual int Recycle(MediaBuf &buf);
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf);
private:
    VPPElement(const VPPElement &);
    VPPElement &operator=(const VPPElement &);

    int SetVppCompParam(mfxExtVPPComposite *vpp_comp);
    int PrepareVppCompFrames();
    mfxStatus InitVpp(MediaBuf &buf);
    int ProcessChainVpp(MediaBuf &buf);
    mfxStatus DoingVpp(mfxFrameSurface1* in_surf, mfxFrameSurface1* out_surf);
    //int PrepareVppCompFrames();
    int HandleProcessVpp();
    int GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool,
                                       mfxU16 nPoolSize);
    mfxStatus AllocFrames(mfxFrameAllocRequest*, bool);
    void ComputeCompPosition(unsigned int x,
                             unsigned int y,
                             unsigned int width,
                             unsigned int height,
                             unsigned int org_width,
                             unsigned int org_height,
                             mfxVPPCompInputStream &inputStream);
    int GetCompBufFromPad(MediaPad *pad, bool &out_of_time
#if not defined SUPPORT_SMTA
            , int &pad_max_level
            , unsigned pad_com_start
            , int pad_comp_timer
            , int pad_cursor
#endif
            );

    //ElementType element_type_;
    MFXVideoSession *mfx_session_;
    unsigned num_of_surf_;
    bool codec_init_;
    //MemPool *input_mp_;
    //Stream *output_stream_;
    mfxFrameSurface1 **surface_pool_;
    mfxVideoParam mfx_video_param_;
    //mfxVideoParam mfx_va_param_;
    MFXFrameAllocator *m_pMFXAllocator;
    mfxExtBuffer *pExtBuf[1 + ENH_FILTERS_COUNT];
    mfxExtOpaqueSurfaceAlloc ext_opaque_alloc_;
    bool m_bUseOpaqueMemory;
    StringInfo *input_str_;
    PicInfo *input_pic_;
    RawInfo *input_raw_;

    MFXVideoVPP *mfx_vpp_;
    std::map<MediaPad *, VPPCompInfo> vpp_comp_map_;
    mfxFrameAllocResponse m_mfxEncResponse;
    unsigned int comp_stc_;
    float max_comp_framerate_; // Largest frame rate of composition streams.
    unsigned current_comp_number_;
    bool comp_info_change_;    //denote whether info for composition changed
    unsigned long vppframe_num_;
    ComboType combo_type_;
    CustomLayout dec_region_list_; //for COMBO_CUSTOM mode only.
    void *combo_master_;
    bool reinit_vpp_flag_;
    bool res_change_flag_;
#ifdef ENABLE_VA
    // VA plugin
    //MFXGenericPlugin *user_va_;
    mfxExtBuffer **extbuf_;
    //VA_OUTPUT_FORMAT output_format_;
    //FrameMeta* in_;
#endif

    int stream_cnt_;
    int string_cnt_;
    int pic_cnt_;
    int raw_cnt_;
    int eos_cnt;
    //res of pre-allocated surfaces, following res change can't exceed it.
    unsigned int frame_width_;
    unsigned int frame_height_;
    BgColorInfo bg_color_info;
    bool keep_ratio_;
    int m_bLibType;
    Measurement  *measurement;
    CodecEventCallback *callback_;

    /* VPP extension */
    mfxExtVPPDoUse      extDoUse;
    mfxU32              tabDoUseAlg[ENH_FILTERS_COUNT];
    mfxU32              enabledFilterCount;
    mfxExtVPPDenoise    vpp_denoise;

    VppExtParams        m_extParams;
};

#endif //VPP_ELEMENT_H
