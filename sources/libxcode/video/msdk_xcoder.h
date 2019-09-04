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

#ifndef TEE_TRANSCODER_H_
#define TEE_TRANSCODER_H_

#include <memory>

#include "base/media_common.h"
#include "base/mem_pool.h"
#include "base/stream.h"
#include "base/logger.h"
#include "base/media_types.h"
#include "base/measurement.h"
#if defined (PLATFORM_OS_LINUX)
#include "vaapi_allocator.h"
#elif defined(PLATFORM_OS_WINDOWS)
#include "d3d_allocator.h"
#include "d3d11_allocator.h"
#endif

#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
class CHWDevice;
#endif
class MSDKCodec;
class MFXVideoSession;
class GeneralAllocator;
class Dispatcher;
class CodecEventCallback;
//class IBitstreamReader;

typedef enum {
    CODEC_TYPE_INVALID = 0,
    CODEC_TYPE_VIDEO_FIRST,
    CODEC_TYPE_VIDEO_AVC,
    CODEC_TYPE_VIDEO_VP8,
    CODEC_TYPE_VIDEO_JPEG,
    CODEC_TYPE_VIDEO_STRING,
    CODEC_TYPE_VIDEO_PICTURE,
    CODEC_TYPE_VIDEO_YUV,
    CODEC_TYPE_VIDEO_LAST,
    CODEC_TYPE_VIDEO_MPEG2,
    CODEC_TYPE_VIDEO_HEVC,
    CODEC_TYPE_OTHER
} CodecType;

typedef enum {
    DECODER,
    ENCODER,
    FEI_ENCODER
} CoderType;

/* background color*/
typedef struct {
    unsigned short Y;
    unsigned short U;
    unsigned short V;
} BgColor;

typedef struct DecOptions_ {
    union {
        MemPool *inputMempool;
        Stream *inputStream;
        StringInfo *inputString; //added for string decoder
        PicInfo *inputPicture; //added for picture decoder
    };
    //IBitstreamReader *reader;
    CodecType input_codec_type;
    // if vp8 codec
    // vp8UseHWDec : true  use hardware decode
    // vp8UseHWDec : false use software decode
    // if other codec this field will be ignore
    bool vp8UseHWDec;
    bool have_NoVpp;
    bool isMempool;
    mfxU16 suggestSurfaceNum;
    void *DecHandle;
    Measurement *measurement;
    DecOptions_() {
        inputStream = NULL;
        inputMempool = NULL;
        //reader = NULL;
        input_codec_type = CODEC_TYPE_INVALID;
        vp8UseHWDec = false;
        have_NoVpp = false;
        isMempool = true;
        suggestSurfaceNum = 0;
        DecHandle = NULL;
        measurement = NULL;
    }
} DecOptions;

typedef struct VppOptions_ {
    // The output pic/surface width/height
    unsigned short out_width;
    unsigned short out_height;

    /**
     * \brief fps of encode video.
     * fps = out_fps_n / out_fps_d
     * \note \note FPS Valid range 1 .. 60.<br>
     * Default value: input's fps
     */
    unsigned int out_fps_n;
    unsigned int out_fps_d;

    /**
     * \brief depth of the look ahead bitrate control .
     * \note Valid values are:
     *    MFX_PICSTRUCT_UNKNOWN =0x00
     *    MFX_PICSTRUCT_PROGRESSIVE =0x01
     *    MFX_PICSTRUCT_FIELD_TFF =0x02
     *    MFX_PICSTRUCT_FIELD_BFF =0x04
     * Default value: MFX_PICSTRUCT_PROGRESSIVE
     */
    unsigned int nPicStruct;

    /**
     * \brief color sampling method.
     * \note Valid values are:
     *    MFX_CHROMAFORMAT_MONOCHROME =0,
     *    MFX_CHROMAFORMAT_YUV420 =1,
     *    MFX_CHROMAFORMAT_YUV422 =2,
     *    MFX_CHROMAFORMAT_YUV444 =3,
     *    MFX_CHROMAFORMAT_YUV400 = MFX_CHROMAFORMAT_MONOCHROME,
     *    MFX_CHROMAFORMAT_YUV411 = 4,
     *    MFX_CHROMAFORMAT_YUV422H = MFX_CHROMAFORMAT_YUV422,
     *    MFX_CHROMAFORMAT_YUV422V = 5
     * Default value: MFX_CHROMAFORMAT_YUV420
     */
    unsigned int chromaFormat;

    /**
     * \brief color format.
     * \note Valid values are:
     *    MFX_FOURCC_NV12 = MFX_MAKEFOURCC('N','V','1','2'), //Native Format
     *    MFX_FOURCC_YV12 = MFX_MAKEFOURCC('Y','V','1','2'),
     *    MFX_FOURCC_YUY2 = MFX_MAKEFOURCC('Y','U','Y','2'),
     *    MFX_FOURCC_RGB3 = MFX_MAKEFOURCC('R','G','B','3'), //RGB24
     *    MFX_FOURCC_RGB4 = MFX_MAKEFOURCC('R','G','B','4'), //RGB32
     *    MFX_FOURCC_P8 = 41, //D3DFMT_P8
     *    MFX_FOURCC_P8_TEXTURE = MFX_MAKEFOURCC('P','8','M','B')
     * Default value: MFX_FOURCC_NV12
     */
    unsigned int colorFormat;

    /**
     * \brief Denoise Factor.
     * \note Valid values are: [0,100]
     * Default value: 0xFFFF
     */
    unsigned int denoiseFactor;
    bool denoiseEnabled;
    mfxU16 nLADepth;
    bool bLABRC;
    mfxU16 nSuggestSurface;
    void *VppHandle;
    Measurement *measurement;
} VppOptions;

/**
 * \brief Encoder configuration.
 */
typedef struct EncOptions_ {
    CodecType output_codec_type;

    /**
     * \ brief Codec Flag
     * \ note To indicate if sw code is preferred
     *     If it's true, MsdkXoder will try to use SW codec
     *     or else, it would use HW codec if it exists
     * Default value: false
     */
    bool swCodecPref;

    /**
     * \brief Encoding profile.
     * \note Valid values are:
     *    MFX_PROFILE_AVC_BASELINE = 66,
     *    MFX_PROFILE_AVC_MAIN=77,
     *    MFX_PROFILE_AVC_EXTENDED=88,
     *    MFX_PROFILE_AVC_HIGH=100.
     * Default value: MFX_PROFILE_AVC_MAIN
     */
    unsigned short profile;

    /**
     * \brief Encoding codec level
     * \note Valid values are 10-51.<br>
     * Default value: 41
     */
    unsigned short level;

    /**
     * \brief Bit rate used during encoding.
     * \note Valid range 0 .. \<any positive value\>.<br>
     * Default value: 0
     */
    unsigned short bitrate;

    /**
     * \brief Intra frames period.
     * \note Valid range 0 .. \<any positive value\>.<br>
     * Default value: 30
     */
    unsigned short intraPeriod;

    unsigned short gopRefDist;

    unsigned short gopOptFlag;

    unsigned short idrInterval;

    /**
     * \brief Quantization parameter value.
     * \note Valid range H264[0,51], MPEG2[1,31].<br>
     * Default H264 value: 0
     */
    unsigned short qpValue;

    /**
     * \brief Bitrate control
     * \note Valid values are:
     *     MFX_RATECONTROL_CBR = 1
     *     MFX_RATECONTROL_VBR = 2
     *     MFX_RATECONTROL_CQP = 3
     *     MFX_RATECONTROL_LA = 8
     * Default value: CBR
     */
    unsigned short ratectrl;

    /**
     * \brief Target Usage.
     * \note Valid values are 0~7.<br>
     *    MFX_TARGETUSAGE_1 = 1,
     *    MFX_TARGETUSAGE_2 = 2,
     *    MFX_TARGETUSAGE_3 = 3,
     *    MFX_TARGETUSAGE_4 = 4,
     *    MFX_TARGETUSAGE_5 = 5,
     *    MFX_TARGETUSAGE_6 = 6,
     *    MFX_TARGETUSAGE_7 = 7,
     *    MFX_TARGETUSAGE_UNKNOWN = 0,
     * Default value:MFX_TARGETUSAGE_7
     * 1 is quality mode;7 is speed mode;others is normal mode.
     * *notes: not vaild in CQP mode.
     */
    unsigned short targetUsage;

    /**
     * \brief slice number for each encoded frame.
     * \note Valid range 0 .. 31.<br>
     * Default value: 0
     */
    unsigned short numSlices;

    /**
     * \brief mbbrc enable.
     * \note Valid range 0 .. 1.<br>
     * Default value: 0
     */
    unsigned short mbbrc_enable;

    /**
     * \brief depth of the look ahead bitrate control .
     * \note Valid range 10 .. 100.<br>
     * Default value: 10
     */
    unsigned short la_depth;

    unsigned short numRefFrame;

    Stream *outputStream;

    unsigned short vp8OutFormat;

    unsigned short nMaxSliceSize;
    unsigned short nMbPerSlice;
    unsigned short bRefType;
    unsigned short pRefType;
    unsigned short GPB;

    mfxU16 nRepartitionCheckEnable;
    //HEVC
    unsigned short numTileRows;
    unsigned short numTileColumns;

    void *EncHandle;

    Measurement *measurement;
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    std::map<int, mfxExtEncoderROI> extRoiData;
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#if (MFX_VERSION >= 1025)
    unsigned short nMFEFrames;
    unsigned short nMFEMode;
    unsigned int   nMFETimeout;
#endif

#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    bool              bPreEnc;
    bool              bEncode;
    bool              bENC;
    bool              bPAK;
    bool              bQualityRepack;
    Stream*           mvinStream;
    Stream*           mvoutStream;
    Stream*           mbcodeoutStream;
    Stream*           weightStream;
    FEICtrlParams     fei_ctrl;
#endif

} EncOptions;


/**
 * \brief MsdkXcoder class.
 * \details Manages decoder and encoder objects.
 */
class MsdkXcoder
{
public:
    DECLARE_MLOGINSTANCE();
    MsdkXcoder(CodecEventCallback *callback = NULL);
    virtual ~MsdkXcoder();

    int Init(DecOptions *dec_cfg, VppOptions *vpp_cfg, EncOptions *enc_cfg, CodecEventCallback *callback = NULL);

    int ForceKeyFrame(void *ouput_handle);

    int SetBitrate(void *output_handle, unsigned short bitrate);
    int AppendEncoderROI(void *output_handle, std::vector<EncoderROI> *roi_list);

    int MarkLTR(void *encHandle);
    // set ref list
    int MarkRL(void *encHandle, unsigned int refNum);

    int SetResolution(void *vppHandle, unsigned int width, unsigned int height);

    int SetComboType(ComboType type, void *vpp, void* master);

    int AttachInput(DecOptions *dec_cfg, void *vppHandle);

    int DetachInput(void* input_handle);

    int SetCustomLayout(void *vppHandle, const CustomLayout *layout);

    int SetBackgroundColor(void *vppHandle, BgColor *bgColor);

    int SetKeepRatio(void *vppHandle, bool isKeepRatio);

    int AttachVpp(DecOptions *dec_cfg, VppOptions *vpp_cfg, EncOptions *enc_cfg);

    void AttachVpp(VppOptions *vpp_cfg, EncOptions *enc_cfg);

    int DetachVpp(void* vpp_handle);

    int AttachOutput(EncOptions *enc_cfg, void *vppHandle);

    int DetachOutput(void* output_handle);

    void StringOperateAttach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void StringOperateDetach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void StringOperateChange(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void PicOperateAttach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void PicOperateDetach(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    void PicOperateChange(DecOptions *dec_cfg, void *vppHandle, unsigned long time = 0);

    bool Start();

    bool Stop();

    bool Join();

    void SetLibraryType(int type);

private:
    MsdkXcoder(const MsdkXcoder&);
    MsdkXcoder& operator=(const MsdkXcoder&);

    int InitVaDisp();
    void DestroyVaDisp();
    MFXVideoSession* GenMsdkSession();
    void CloseMsdkSession(MFXVideoSession *session);
    void CreateSessionAllocator(MFXVideoSession **session, GeneralAllocator **allocator);
    // for Encoder can not using GeneralAllocator
#if defined (PLATFORM_OS_WINDOWS)
    void CreateEncoderSessionAllocator(MFXVideoSession **session, MFXFrameAllocator **allocator);
#elif defined (PLATFORM_OS_LINUX)
    void CreateEncoderSessionAllocator(MFXVideoSession **session, MFXFrameAllocator **allocator);
#endif
    void CleanUpResource();
    ElementType GetElementType(CoderType type, unsigned int file_type, bool swCodecPref);

    void ReadDecConfig(DecOptions *dec_cfg, void *decCfg);
    void ReadVppConfig(VppOptions *vpp_cfg, void *vppCfg);
    void ReadEncConfig(EncOptions *enc_cfg, void *encCfg);

    std::list<MSDKCodec*> del_dec_list_;

    std::list<MFXVideoSession*> session_list_;
    std::vector<BaseFrameAllocator*> m_pAllocArray;
    MFXVideoSession* main_session_;
    int dri_fd_;
#if defined(PLATFORM_OS_WINDOWS)
    HANDLE va_mutex_;
    mfxAllocatorParams* m_pD3DAllocatorParams;
#elif defined (PLATFORM_OS_LINUX)
    pthread_mutex_t va_mutex_;
#endif
    void *va_dpy_;       /**< \brief va diaplay handle*/
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT) || defined(PLATFORM_OS_WINDOWS)
    CHWDevice *hwdev_;
#endif
    mfxAllocatorParams* m_pAllocatorParams;
    bool is_running_;               /**< \brief xcoder running status*/
    bool done_init_;
    int libType_;
    CodecEventCallback *callback_;
    Mutex mutex;
    Mutex sess_mutex;

    MSDKCodec *dec_;
    std::list<MSDKCodec*> dec_list_;

    Dispatcher * dec_dis_;
    std::map<MSDKCodec*, Dispatcher*> dec_dis_map_;

    MSDKCodec* vpp_;
    std::list<MSDKCodec*> vpp_list_;

    Dispatcher* vpp_dis_;
    std::map<MSDKCodec*, Dispatcher*> vpp_dis_map_;

    MSDKCodec *enc_;
    std::multimap<MSDKCodec*, MSDKCodec*> enc_multimap_;

    MSDKCodec *render_;
    bool m_bRender_;

#if defined (PLATFORM_OS_WINDOWS)
private:
    struct MSDKAdapter {
    // returns the number of adapter associated with MSDK session, 0 for SW session
    static mfxU32 GetNumber(mfxSession session = 0) {
        mfxU32 adapterNum = 0; // default
        mfxIMPL impl = MFX_IMPL_SOFTWARE; // default in case no HW IMPL is found

        // we don't care for error codes in further code; if something goes wrong we fall back to the default adapter
        if (session)
        {
            MFXQueryIMPL(session, &impl);
        }
        else
        {
            // an auxiliary session, internal for this function
            mfxSession auxSession;
            memset(&auxSession, 0, sizeof(auxSession));

            mfxVersion ver = { {1, 1 }}; // minimum API version which supports multiple devices
            MFXInit(MFX_IMPL_HARDWARE_ANY, &ver, &auxSession);
            MFXQueryIMPL(auxSession, &impl);
            MFXClose(auxSession);
        }

        // extract the base implementation type
        mfxIMPL baseImpl = MFX_IMPL_BASETYPE(impl);

        const struct
        {
            // actual implementation
            mfxIMPL impl;
            // adapter's number
            mfxU32 adapterID;

        } implTypes[] = {
            {MFX_IMPL_HARDWARE, 0},
            {MFX_IMPL_SOFTWARE, 0},
            {MFX_IMPL_HARDWARE2, 1},
            {MFX_IMPL_HARDWARE3, 2},
            {MFX_IMPL_HARDWARE4, 3}
        };


        // get corresponding adapter number
        for (mfxU8 i = 0; i < sizeof(implTypes)/sizeof(*implTypes); i++)
        {
            if (implTypes[i].impl == baseImpl)
            {
                adapterNum = implTypes[i].adapterID;
                break;
            }
        }

        return adapterNum;
    }
};
#endif
};

#endif /* TEE_TRANSCODER_H_ */
