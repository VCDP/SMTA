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

#ifndef MSDK_CODEC_H
#define MSDK_CODEC_H
#include <map>
#include <stdio.h>
#include <mfxvideo++.h>
#include <mfxplugin++.h>
#include <mfxjpeg.h>
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX) || defined(MFX_LIBRARY_EXPOSED)
#include <mfxvp8.h>
#endif

#include "base/base_element.h"
#include "base/media_pad.h"
#include "base/media_types.h"
#include "base/media_common.h"
#include "base/mem_pool.h"
#include "base/stream.h"
#include "base/measurement.h"
//#include "base/trace.h"
#include "general_allocator.h"
#include "base/logger.h"
#include "os/platform.h"
#include "io_adapter.hpp"

#ifdef ENABLE_VA
#include "base/frame_meta.h"
#endif

#if defined(ENABLE_VA) || defined(LIBVA_X11_SUPPORT) || defined(LIBVA_DRM_SUPPORT)
#include "vaapi_device.h"
#endif

#if defined (PLATFORM_OS_WINDOWS)
#define usleep(A) Sleep(A/1000)
#endif

#if !defined(MFX_DISPATCHER_EXPOSED_PREFIX) && !defined(MFX_LIBRARY_EXPOSED)
#define MFX_CODEC_VP8 101
#endif
#define MFX_CODEC_STRING 102
#define MFX_CODEC_PICTURE 103
#ifdef ENABLE_RAW_DECODE
#define MFX_CODEC_YUV 104
#endif
// number of video enhancement filters (denoise, procamp, detail, video_analysis, image stab)
#define ENH_FILTERS_COUNT 5


#ifdef ENABLE_VA
typedef enum {
    FORMAT_ES,
    FORMAT_TXT,
    FORMAT_VECTOR
} VA_OUTPUT_FORMAT;
#endif

typedef struct _EncExtParams {
    bool bMBBRC;
    mfxU16 nLADepth; // depth of the look ahead bitrate control  algorithm
    mfxU16 nMbPerSlice; // slice size in number of macroblocks
    mfxU16 bRefType; // B frame as reference
    mfxU16 pRefType; // P frame as reference
    mfxU16 GPB;
    mfxU32 nMaxSliceSize; //Max Slice Size, for AVC only
    mfxU16 nRepartitionCheckEnable; //Controls AVC encoder attempts to predict from small partitions
#if (MFX_VERSION >= 25)
    mfxU16 nMFEFrames;  // maximum number of frames to be combined in MFE
    mfxU16 nMFEMode;    // MFE encode operation mode
    mfxU32 nMFETimeout; // MFE timeout in milliseconds
#endif
#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
    bool              bPreEnc;
    bool              bEncode;
    bool              bENC;
    bool              bPAK;
    bool              bQualityRepack;
    MFXVideoSession   *pPreencSession;
    Stream*           mvinStream;
    Stream*           mvoutStream;
    Stream*           mbcodeoutStream;
    Stream*           weightStream;
    FEICtrlParams     fei_ctrl;
#endif
    _EncExtParams() {
        bMBBRC = false;
        nLADepth = 0;
        nMbPerSlice = 0;
        bRefType = 0;
        pRefType = 0;
        GPB = MFX_CODINGOPTION_ON;
        nMaxSliceSize = 0;
        nRepartitionCheckEnable = MFX_CODINGOPTION_UNKNOWN;
#if (MFX_VERSION >= 25)
        nMFEFrames = 0;
        nMFEMode = 0;
        nMFETimeout = 0;
#endif
#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
        bPreEnc = false;
        bEncode = false;
        bENC = false;
        bPAK = false;
        bQualityRepack = false;
        pPreencSession = NULL;
        mvinStream = NULL;
        mvoutStream = NULL;
        mbcodeoutStream = NULL;
        weightStream = NULL;
#endif
    }
} EncExtParams;

typedef struct _VppExtParams {
    bool    bLABRC;
    mfxU16  nLADepth; // depth of the look ahead bitrate control  algorithm
    mfxU16  nSuggestSurface;
    _VppExtParams() {
        bLABRC = false;
        nLADepth = 0;
        nSuggestSurface = 0;
    }
} VppExtParams;

typedef struct _DecExtParams {
    bool have_NoVpp;
    mfxU16  suggestSurfaceNum; // enlarge dec surface as depth of the look ahead bitrate control  algorithm is set
    _DecExtParams() {
        have_NoVpp = false;
        suggestSurfaceNum = 0;
    }
} DecExtParams;

typedef struct _VppDenoiseParams {
    bool enabled;
    mfxU16 factor;
    _VppDenoiseParams() {
        enabled = false;
        factor = 0;
    }
} VppDenoiseParams;

typedef struct _ElementCfg {
    //IBitstreamReader *reader;
    union {
        union {
            MemPool *input_mp;
            Stream  *input_stream;
            StringInfo *input_str; //added for string decoder
            PicInfo *input_pic; //added for picture decoder
            RawInfo *input_raw; //added for picture decoder
        };
        Stream *output_stream;
    };
    union {
        mfxVideoParam DecParams;
        mfxVideoParam EncParams;
        mfxVideoParam VppParams;
        mfxVideoParam PlgParams;
    };
    union {
        void* DecHandle;
        void* VppHandle;
        void* EncHandle;
    };
    bool isMempool;
    Measurement  *measurement;
    EncExtParams extParams;
    VppExtParams extVppParams;
    DecExtParams extDecParams;
#if MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
    std::map<int, mfxExtEncoderROI> extRoiData;
#endif // MSDK_ROI_SUPPORT && MFX_VERSION >= 1022
#ifdef ENABLE_VA
    bool bOpenCLEnable;
    char* KernelPath;
    bool bDump;
    bool PerfTrace;
    int  va_interval;
    VAType va_type;
    VA_OUTPUT_FORMAT output_format;
#endif
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
    CHWDevice *hwdev;
#endif
    VppDenoiseParams vpp_denoise;
    int libType;

    _ElementCfg() {
        //reader = NULL;
        output_stream = NULL;
        memset(&DecParams, 0, sizeof(DecParams));
        DecHandle = NULL;
        isMempool = true;
        measurement = NULL;
#ifdef ENABLE_VA
        bOpenCLEnable = false;
        KernelPath = NULL;
        bDump = false;
        PerfTrace = false;
        va_interval = 0;
        va_type = VA_ROI;
        output_format = FORMAT_TXT;
#endif
#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_X11_SUPPORT)
        hwdev = NULL;
#endif
        libType = MFX_IMPL_HARDWARE_ANY;
    }
} ElementCfg;

typedef struct {
    unsigned x;
    unsigned y;
    unsigned w;
    unsigned h;
} VppRect;

/* background color*/
typedef struct {
    unsigned short Y;
    unsigned short U;
    unsigned short V;
} BgColorInfo;

class MSDKCodec: public BaseElement
{
public:
    /*!
     *  \brief Initialize a MSDK element.
     *  @param[in]: element_-+type, specify the type of element, see @ElementType
     *  @param[in]: session, session of this element.
     */
    //MSDKCodec(ElementType element_type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator = NULL);

    virtual ~MSDKCodec(){};

    /*!
     *  \brief Initialize a MSDK element.
     *  @param[in]: ele_param, specify the configuration of element.
     *  @param[in]: element_mode, active or passive.
     */
    virtual bool Init(void *cfg, ElementMode element_mode) = 0;
#ifdef ENABLE_VA
    virtual int SetVARegion(FrameMeta* frame){return 0;}
    virtual int Step(FrameMeta* input){return 0;}
#endif

    virtual int SetVPPCompRect(BaseElement *element, VppRect *rect){return -1;}

    virtual int MarkLTR(){return 1;}

    virtual void SetForceKeyFrame(){return;}

    virtual void SetResetBitrateFlag(){}

    virtual void SetBitrate(unsigned short bitrate){}

    virtual int SetRes(unsigned int width, unsigned int height){return 1;}

    virtual int ConfigVppCombo(ComboType combo_type, void *master_handle){return 1;}

    virtual int SetCompRegion(const CustomLayout *layout){return 1;}

    virtual int SetBgColor(BgColorInfo *bgColorInfo){return 1;}
    virtual int SetKeepRatio(bool isKeepRation){return 0;}

    virtual StringInfo *GetStrInfo(){return NULL;}

    virtual void SetStrInfo(StringInfo *strinfo){}

    virtual void SetCompInfoChe(bool ifchange){}

    virtual unsigned long GetNumOfVppFrm(){return 0;}

    virtual unsigned int GetVppOutFpsN(){return 0;}

    virtual unsigned int GetVppOutFpsD(){return 0;}

    virtual PicInfo *GetPicInfo(){return NULL;}

    virtual void SetPicInfo(PicInfo *picinfo){}

    virtual RawInfo *GetRawInfo(){return NULL;}

    virtual void SetRawInfo(RawInfo *rawinfo){}

    virtual int QueryStreamCnt(){return -1;}

    virtual int MarkRL(unsigned int refNum) {return 1;}

    virtual void AppendEncoderROI(std::vector<EncoderROI> *roi_list) {}

protected:
    virtual void NewPadAdded(MediaPad *pad){return;}
    virtual void PadRemoved(MediaPad *pad){return;}
    virtual int HandleProcess(){return 0;};

    virtual int Recycle(MediaBuf &buf) = 0;
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf) = 0;

};

class BaseDecoderElement: public MSDKCodec
{
public:
    BaseDecoderElement();
    ~BaseDecoderElement(){};

    virtual bool Init(void *cfg, ElementMode element_mode) = 0;
#ifdef ENABLE_VA
    virtual int Step(FrameMeta* input){return 0;}
#endif
    virtual PicInfo *GetPicInfo(){return NULL;}
    virtual RawInfo *GetRawInfo(){return NULL;}
    virtual StringInfo *GetStrInfo(){return NULL;}

protected:
    virtual int HandleProcess(){return 0;};
    virtual int Recycle(MediaBuf &buf) = 0;
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf){return -1;}
    mfxStatus AllocFrames(mfxFrameAllocRequest *pRequest, bool isDecAlloc);
    int GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool, mfxU16 nPoolSize);
    void recycle(MediaBuf &buf);
    void initialize(ElementType type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator, CodecEventCallback *callback = NULL);

    MFXVideoSession *mfx_session_;
    unsigned num_of_surf_;
    bool codec_init_;
    mfxFrameSurface1 **surface_pool_;
    mfxVideoParam mfx_video_param_;
    MFXFrameAllocator *m_pMFXAllocator;
    mfxExtOpaqueSurfaceAlloc ext_opaque_alloc_;
    bool m_bUseOpaqueMemory;
    mfxFrameAllocResponse m_mfxDecResponse;
    int m_bLibType;

    /**
     * \brief benchmark.
     */
    Measurement  *measurement;
    CodecEventCallback *callback_;

};

class BaseEncoderElement: public MSDKCodec
{
public:
    BaseEncoderElement();
    ~BaseEncoderElement(){}
    virtual bool Init(void *cfg, ElementMode element_mode) = 0;

    virtual int MarkLTR(){return 1;}
    virtual void SetForceKeyFrame(){return;}
    virtual void SetResetBitrateFlag(){}
    virtual void SetBitrate(unsigned short bitrate);
    virtual int MarkRL(unsigned int refNum) {return 1;}

    virtual void AppendEncoderROI(std::vector<EncoderROI> *roi_list) {}

protected:
    //BaseElement
    virtual int HandleProcess(){return 0;}
    virtual int Recycle(MediaBuf &buf){return 0;}
    virtual int ProcessChain(MediaPad *pad, MediaBuf &buf){return -1;}

protected:
    void initialize(ElementType type, MFXVideoSession *session, MFXFrameAllocator *pMFXAllocator, CodecEventCallback *callback = NULL);
    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, FILE *fSink);
    mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, Stream *out_stream);

    MFXVideoSession *mfx_session_;
    bool codec_init_;
    Stream *output_stream_;
    mfxVideoParam mfx_video_param_;
    MFXFrameAllocator *m_pMFXAllocator;
    mfxExtOpaqueSurfaceAlloc ext_opaque_alloc_;
    bool m_bUseOpaqueMemory;
    int m_bLibType;
    Measurement  *measurement;
    CodecEventCallback *callback_;

};
#endif //MSDK_CODEC_H
