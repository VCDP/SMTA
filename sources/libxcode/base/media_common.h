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
#ifndef MEDIA_COMMON_H
#define MEDIA_COMMON_H
#include <stdio.h>
#include <list>

#define MSDK_ROI_SUPPORT 0
typedef enum {
    STREAM_TYPE_INVALID = 0,
    STREAM_TYPE_AUDIO_FIRST,
    STREAM_TYPE_AUDIO_MPEG1,
    STREAM_TYPE_AUDIO_MPEG2,
    STREAM_TYPE_AUDIO_AC3,
    STREAM_TYPE_AUDIO_DTS,
    STREAM_TYPE_AUDIO_ADTS,
    STREAM_TYPE_AUDIO_AAC,
    STREAM_TYPE_AUDIO_MP3,
    STREAM_TYPE_AUDIO_PCM,
    STREAM_TYPE_AUDIO_ALAW,
    STREAM_TYPE_AUDIO_MULAW,
    STREAM_TYPE_AUDIO_LAST,
//HW CODEC
    STREAM_TYPE_VIDEO_FIRST,
    STREAM_TYPE_VIDEO_MPEG1,
    STREAM_TYPE_VIDEO_MPEG2,
    STREAM_TYPE_VIDEO_MPEG4,
    STREAM_TYPE_VIDEO_AVC,
    STREAM_TYPE_VIDEO_VC1,
    STREAM_TYPE_VIDEO_VP8,
    STREAM_TYPE_VIDEO_TEXT,
    STREAM_TYPE_VIDEO_BMP,
    STREAM_TYPE_VIDEO_RAW,
    STREAM_TYPE_VIDEO_LAST,
//SW CODEC
    STREAM_TYPE_VIDEO_SW_FIRST,
    STREAM_TYPE_VIDEO_SW_H263,
    STREAM_TYPE_VIDEO_SW_H264,
    STREAM_TYPE_VIDEO_SW_JPEG,
    STREAM_TYPE_VIDEO_SW_LAST,
    STREAM_TYPE_OTHER
} StreamType;

enum errors_t {
    ERR_NONE = 0,
    ERR_X11_DISPLAY_OPEN_FAIL,
    ERR_X11_DISPLAY_CLOSE_FAIL,
    VAAPI_ERR_PIPELINE_INITIALIZE_FAIL,
    VAAPI_ERR_DEC_INITIALIZE,
    VAAPI_ERR_DEC_INVALID_PROFILE,                      /**< Error 4: Could not find H.264 decoder profile */
    VAAPI_ERR_DEC_NO_VLD_ENTRYPOINT,                    /**< Error 5: Could not find H.264 Variable Length Decoding entry point in current profile */
    VAAPI_ERR_DEC_GET_CFG_ATTRIB,                       /**< Error 6: Could not get configuration attribute in current profile */
    VAAPI_ERR_DEC_NO_YUV420_FORMAT_FOUND,               /**< Error 7: YUV 4.2.0 format not supported for current entry point */
    VAAPI_DEC_ERR_VA_QUERY_IMAGE_FORMATS,               /**< Error 8: Fail to query supported VA image formats. */
    VAAPI_DEC_ERR_I420_IMAGE_FORMAT_NOT_FOUND,          /**< Error 9: Could not find I420 image format. */
    VAAPI_ERR_DEC_VA_CREATE_IMAGE,                      /**< Error 10: Could not create VA image to export YUV data */
    VAAPI_ERR_DEC_VA_GET_IMAGE,                         /**< Error 11: Could not fill VA image with surface data. */
    VAAPI_ERR_DEC_VA_CREATE_CONFIG,                     /**< Error 12: Could not create configuration for H.264 decoder */
    VAAPI_ERR_DEC_VA_CREATE_SURFACE,                    /**< Error 13: Could not create all the required VA surfaces for H.264 decoder */
    VAAPI_ERR_DEC_VA_CREATE_CONTEXT,                    /**< Error 14: Could not create context for H.264 decoder context */
    VAAPI_ERR_DEC_VA_CREATE_PIC_PARAM_BUFFER,           /**< Error 15: Could not create buffer for picture parameters */
    VAAPI_ERR_DEC_VA_CREATE_IQ_MATRIX_BUFFER,           /**< Error 16: Could not create buffer for IQ matrices */
    VAAPI_ERR_DEC_VA_CREATE_SLICE_PARAM_BUFFER,         /**< Error 17: Could not create buffer for slice parameters */
    VAAPI_ERR_DEC_VA_CREATE_SLICE_DATA_BUFFER,          /**< Error 18: Could not create buffer for slice data */
    VAAPI_ERR_DEC_VA_MAP_BUFFER,                        /**< Error 19: Could not map VA Buffer */
    VAAPI_ERR_DEC_VA_BEGIN_PICTURE,                     /**< Error 20: Could not begin rendering */
    VAAPI_ERR_DEC_VA_RENDER_PICTURE,                    /**< Error 21: Could not perform rendering */
    VAAPI_ERR_DEC_VA_END_PICTURE,                       /**< Error 22: Could not terminate rendering */
    VAAPI_ERR_DEC_VA_UNMAP_BUFFER,                      /**< Error 23: Could not unmap VA Buffer */
    VAAPI_ERR_DEC_VA_DESTROY_BUFFER,                    /**< Error 24: Could not destroy a VA Buffer */
    VAAPI_ERR_DEC_VA_DESTROY_SURFACE,                   /**< Error 25: Could not destroy VA surfaces, surface still being rendered. */
    VAAPI_ERR_DEC_VA_DESTROY_CONTEXT,                   /**< Error 26: Could not destroy H.264 decoder context */
    VAAPI_ERR_DEC_VA_DESTROY_CONFIG,                    /**< Error 27: Could not destroy H.264 decoder configuration */
    VAAPI_ERR_DEC_MEMORY_FAULT,                         /**< Error 28: Could not allocate system memory for decoder */
    VAAPI_ERR_DEC_CREATE_SLICE_LIST_BUFFER,             /**< Error 29: Could not allocate system memory for new slice */
    VAAPI_ERR_DEC_IMG_MEMORY_FAULT,                     /**< Error 30: Could not allocate system memory for image format query */
    VAAPI_ERR_DEC_SURFACE_RESOURCES_FAULT,              /**< Error 31: Could not allocate surface for current decoding */
    VAAPI_ERR_DEC_VA_TERMINATE,                         /**< Error 32: Error closing the VA API */
    VAAPI_ERR_DEC_LUMA_TABLE_SIZE,                      /**< Error 33: Invalid luma table size in slice header */
    VAAPI_ERR_DEC_CHROMA_TABLE_SIZE,                    /**< Error 34: Invalid chroma table size in slice header */
    VAAPI_ERR_DEC_NUM_FRAMES_IN_DPB_FOR_I,
    VAAPI_ERR_DEC_NUM_FRAMES_IN_DPB_FOR_P,
    ERR_H264_PARSER_MEMORY_FAULT,                       /**< Error 37: Could not allocate memory for H.264 parser object */
    ERR_FRAME_BUFFER_NOT_ALLOCATED,                     /**< Error 38: No memory allocated for output frame buffer */
    ERR_PARAMS_INVALID_INPUT_FILE_PATH,                 /**< Error 39: Invalid file path for H.264 input file */
    ERR_PARAMS_INVALID_OUTPUT_FILE_PATH,                /**< Error 40: Invalid file path for YUV output file */
    ERR_PARAMS_INVALID_STREAM_INPUT_PROFILE,            /**< Error 41: Profile found in H.264 input stream not supported by VA API */
    ERR_PARAMS_INVALID_STREAM_INPUT_LEVEL,              /**< Error 42: Level found in H.264 input stream not supported by VA API */
    ERR_PARAMS_INVALID_STREAM_INPUT_FRAME_WIDTH,        /**< Error 43: Invalid frame width in H.264 input stream */
    ERR_PARAMS_INVALID_STREAM_INPUT_FRAME_HEIGHT,       /**< Error 44: Invalid frame height in H.264 input stream */
    ERR_PARAMS_INVALID_STREAM_INPUT_FRAME_RATE,         /**< Error 45: Invalid frame rate */
    ERR_FAIL_TO_READ_INPUT_FILE,                        /**< Error 46: Could not read H.264 input file */
    ERR_INVALID_PPS_PARAMS,                             /**< Error 47: Invalid PPS found on bit stream, check number of slice data per picture */
    ERR_REFERENCE_INITALISATION,                        /**< Error 48: Could not initialise correctly references */
    ERR_PARAMS_INVALID_PICTURE_TYPE,
    ERR_INVALID_B_FRAME,
    ERR_INVALID_P_FRAME,
    ERR_FRAMEWORK_INITIALIZATION_FAIL,
    ERR_INVALID_INPUT_PARAMETER,
    ERR_ALLOCATION_FAIL,
    ERR_DECODER_INITIALIZATION_FAIL,
    ERR_ENCODER_INITIALIZATION_FAIL,
    ERR_ENCODER_TYPE,
    ERR_VPP_INITIALIZATION_FAIL,
    ERR_OPEN_FILE_FAIL,
    ERR_INVALID_SURFACE,
    ERR_ENCODE_FAIL,
    ERR_MPEG2_CODEC,
    ERR_MPEG2_ALLOC_CONTEXT,
    ERR_MPEG2_OPEN_CODEC,
    ERR_MPEG2_ALLOC_FRAME,
    ERR_MPEG2_ALLOC_IMAGE,
    ERR_MPEG2_ENCODING,
    ERR_MPEG2_WRITE,
    ERR_MPEG2_WRITE_SUBSTREAM,
    ERR_MPEG2_ALLOC_YUV_BUFFER,
    ERR_MPEG2_DERIVE_IMAGE,
    ERR_MPEG2_MAP_BUFFER,
    ERR_MPEG2_UNMAP_BUFFER,
    ERR_MPEG2_DESTROY_IMAGE,
    ERR_MPEG2_FRAME_RATE,
    ERR_MPEG2_SOURCE_WIDTH,
    ERR_MPEG2_SOURCE_HEIGHT,
    ERR_NOT_MATCH_ELEMENT,
    ERR_UNKNOWN
};

#ifdef ENABLE_SW_CODEC
typedef enum {
    SW_CODEC_ID_H264,
    SW_CODEC_ID_H263,
    SW_CODEC_ID_JPEG
}SWCodecID;
#endif

#ifdef ENABLE_VA
typedef enum {
    VA_PEOPLE,
    VA_FACE,
    VA_ROI,
    VA_CAR,
    VA_PLATE
} VAType;
#endif

enum VP8FILETYPE {
    RAW_FILE,
    IVF_FILE,
    WEBM_FILE,
    UNKNOWN
};

//String Info
typedef struct StringInfo {
    char          *text;  //the string
    unsigned int  pos_x; //starting position of the string on surface from primary stream in horizontal direction
    unsigned int  pos_y; //starting position of the string on surface from primary stream in vertical direction
    unsigned int  width; //width of string image on composite output
    unsigned int  height; //height of string image on composite output
    char          *font; //font file
    unsigned int  plsize; //character pixel size. Character size will be plsize*plsize
    float         alpha; //alpha value for blending, in the range of [0, 1.0]
    unsigned char rgb_r; //R value for character color
    unsigned char rgb_g; //G value for character color
    unsigned char rgb_b; //B value for character color

    StringInfo():
    text(NULL),
    pos_x(0),
    pos_y(0),
    width(0),
    height(0),
    font(NULL),
    plsize(0),
    alpha(0),
    rgb_r(255),
    rgb_g(255),
    rgb_b(255) {
    };
} StringInfo;

typedef struct PicInfo {
    char         *bmp;   //the bitmap
    unsigned int pos_x;  //starting position of bitmap on composited output in horizontal direction
    unsigned int pos_y;  //starting position of bitmap on composited output in vertical direction
    unsigned int width;  //width of bitmap on composited output
    unsigned int height; //height of bitmap on composited output
    float        alpha;  //alpha value for blending, in the range of [0, 1.0]

    // Add for pixel alpha
    unsigned short luma_key_enable;
    unsigned short luma_key_min;
    unsigned short luma_key_max;
    unsigned short global_alpha_enable;
    unsigned short pixel_alpha_enable;

    PicInfo():
    bmp(NULL),
    pos_x(0),
    pos_y(0),
    width(0),
    height(0),
    alpha(0),
#if defined(SUPPORT_SMTA)
    luma_key_enable(0),
    luma_key_min(0),
    luma_key_max(10),
    global_alpha_enable(0),
    pixel_alpha_enable(0) {
#else
    luma_key_enable(0),
    luma_key_min(0),
    luma_key_max(0),
    global_alpha_enable(1),
    pixel_alpha_enable(0) {
#endif
    };
} PicInfo;

//#if defined ENABLE_RAW_DECODE
typedef struct RawInfo {
    char         *raw_file;
    unsigned int fourcc;
    unsigned int width;
    unsigned int height;
    unsigned int cropx;
    unsigned int cropy;
    unsigned int cropw;
    unsigned int croph;
    unsigned int dstx;
    unsigned int dsty;
    unsigned int dstw;
    unsigned int dsth;

    float        alpha;  //alpha value for blending, in the range of [0, 1.0]

    // Add for pixel alpha
    unsigned short luma_key_enable;
    unsigned short luma_key_min;
    unsigned short luma_key_max;
    unsigned short global_alpha_enable;
    unsigned short pixel_alpha_enable;
    RawInfo():
        raw_file(NULL),
        fourcc(0),
        width(0),
        height(0),
        cropx(0),
        cropy(0),
        cropw(0),
        croph(0),
        dstx(0),
        dsty(0),
        dstw(0),
        dsth(0),
        alpha(0),
        luma_key_enable(0),
        luma_key_min(0),
        luma_key_max(10),
        global_alpha_enable(0),
        pixel_alpha_enable(0) {
    };
} RawInfo;
//#endif

//COMBO_CUSTOM mode is for upper level to configure/control the composition
//1. Inputs to render maybe a subset of inputs attached to VPP.
//2. VPP only composite inputs set by SetCompRegion();
//3. VPP renders the inputs according to their sequence in the list
//3. String/Pic inputs are not supported in this mode.
typedef enum {
    COMBO_BLOCKS, //default type
    COMBO_MASTER,
    COMBO_CUSTOM
} ComboType;

typedef struct {
    float left;            //"x / WIDTH"
    float top;             //"y / HEIGHT"
    float width_ratio;     //"width / WIDTH"
    float height_ratio;    //"height / HEIGHT"
} Region;

typedef struct {
    void *handle;          //pointer, may point to dec handle, dis handle, or pad handle
    Region region;
} CompRegion;

typedef std::list<CompRegion> CustomLayout; //covers the inputs to composite, including z-order


typedef struct {
    unsigned left;
    unsigned top;
    unsigned right;
    unsigned bottom;
} EncoderROI;
typedef std::list<EncoderROI*> ROI_LIST;

#if defined(MSDK_AVC_FEI) || defined(MSDK_HEVC_FEI)
typedef struct _FEICtrlParams {
    unsigned char nDSstrength;
    unsigned short nSearchPath;
    unsigned short nLenSP;
    unsigned short nSubMBPartMask;
    unsigned short nIntraPartMask;
    unsigned short nSubPelMode;
    unsigned short nAdaptiveSearch;
    unsigned short nSearchWindow;
    unsigned short nRefWidth;
    unsigned short nRefHeight;
    unsigned short nQP;
    unsigned short nMVPredictors_Pl0;
    unsigned short nMVPredictors_Bl0;
    unsigned short nMVPredictors_Bl1;
    unsigned short nRefActiveP;
    unsigned short nRefActiveBL0;
    unsigned short nRefActiveBL1;
    unsigned short nNumMvPredictors[2];
    unsigned short nMVPredictor;
    unsigned short nForceCtuSplit;
    unsigned short nNumFramePartitions;
    unsigned short nDisableDeblockingIdc;
    short          nSliceAlphaC0OffsetDiv2;
    short          nSliceBetaOffsetDiv2;
    short          nChromaQPIndexOffset;
    short          nSecondChromaQPIndexOffset;
    bool           bRepartitionCheckEnable;
    bool           bMultiPredL0;
    bool           bMultiPredL1;
    bool           bDistortionType;
    bool           bColocatedMbDistortion;
    bool           bNPredSpecified_Pl0;
    bool           bNPredSpecified_Bl0;
    bool           bNPredSpecified_Bl1;
    bool           bMBSize;
    bool           bTransform8x8ModeFlag;
    bool           bRepackPreencMV;

    _FEICtrlParams() {
        nDSstrength = 0;
        nSearchPath = 0;
        nLenSP = 57;
        nSubMBPartMask = 0x00;
        nIntraPartMask = 0x00;
        nSubPelMode = 0x03;
        nAdaptiveSearch = 0;
        nSearchWindow = 5;
        nRefWidth = 32;
        nRefHeight = 32;
        nQP = 0;
        nMVPredictors_Pl0        = 1;
        nMVPredictors_Bl0        = 1;
        nMVPredictors_Bl1        = 0;
        nRefActiveP            = 0;
        nRefActiveBL0          = 0;
        nRefActiveBL1          = 0;
        nNumMvPredictors[0]    = 3;
        nNumMvPredictors[1]    = 1;
        nMVPredictor           = 0;
        nForceCtuSplit         = 0;
        nNumFramePartitions    = 1;
        nDisableDeblockingIdc     = 0;
        nSliceAlphaC0OffsetDiv2   = 0;
        nSliceBetaOffsetDiv2      = 0;
        nChromaQPIndexOffset      = 0;
        nSecondChromaQPIndexOffset= 0;
        bRepartitionCheckEnable   = false;
        bMultiPredL0              = false;
        bMultiPredL1              = false;
        bDistortionType           = false;
        bColocatedMbDistortion    = false;
        bNPredSpecified_Pl0      = false;
        bNPredSpecified_Bl0      = false;
        bNPredSpecified_Bl1      = false;
        bMBSize                  = false;
        bTransform8x8ModeFlag    = false;
        bRepackPreencMV          = false;

    }
} FEICtrlParams;
#endif
#endif
