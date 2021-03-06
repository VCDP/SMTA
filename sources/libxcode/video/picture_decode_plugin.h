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

#ifdef ENABLE_PICTURE_CODEC
#ifndef PICTURE_DECODER_PLUGIN_H_
#define PICTURE_DECODER_PLUGIN_H_

#include <stdlib.h>
//#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "base/media_common.h"
#include "base_allocator.h"
#include "base/logger.h"

//Task structure for plugin
typedef struct {
    PicInfo           *picInfo;    //Pointer to picture info
    mfxFrameSurface1  *surfaceOut; //Pointer to output surface
    bool              bBusy;       //Task is in use or not
} MFXTask3;

//
// This class uses the Media SDK plug-in interfaces to
// realize an asynchronous picture processing user plug-in
// which can be used in a transcode pipeline.
//
class PicDecPlugin : public MFXGenericPlugin
{
public:
    DECLARE_MLOGINSTANCE();
    PicDecPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus Init(mfxVideoParam *param);
    virtual mfxStatus DecodeHeader(PicInfo *picinfo, mfxVideoParam *par);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Close();
    virtual ~PicDecPlugin();
    mfxExtBuffer *GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

    // Interface called by Media SDK framework
    mfxStatus PluginInit(mfxCoreInterface *core);
    mfxStatus PluginClose();
    mfxStatus GetPluginParam(mfxPluginParam *par);
    mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task); //called to submit task but not execute it
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a); //function that called to start task execution and check status of execution
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts); //free task and releated resources
    mfxStatus SetAuxParams(void *auxParam, int auxParamSize);
    void      Release();

private:
    // Interface for picture decoding
    mfxStatus Bmp2Yuv(char *bmp_data, int bmp_width, int bmp_height, PicInfo *picinfo);
    mfxStatus GetRGB16(char *bmp_data, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b);
    mfxStatus GetRGB24(char *bmp_data, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b);
    mfxStatus GetRGB32(char *bmp_data, unsigned char *rgb_r, unsigned char *rgb_g, unsigned char *rgb_b);
    mfxStatus UploadToSurface(unsigned char *yuv_buf, mfxFrameSurface1 *surface_out);

protected:
    mfxCoreInterface  *m_pmfxCore;        //Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam     m_mfxPluginParam;  //Plug-in parameters
    mfxU16             m_MaxNumTasks;     //Max number of concurrent tasks
    MFXTask3          *m_pTasks;          //Array of tasks
    mfxVideoParam      m_VideoParam;
    mfxFrameAllocator *m_pAlloc;
    mfxExtBuffer      *m_ExtParam;
    bool               m_bIsOpaque;

    // Picture Decoder
    unsigned long      offset_bytes_;     //offset from bitmap file header to pixel data
    int                bmp_width_;        //width of bitmap
    int                bmp_height_;       //height of bitmap
    int                bit_cnt_;          //bits per pixel
    int                bytes_per_line_;   //bytes per line in bitmap data
    char              *bmp_data_;
    unsigned char     *yuv_buf_;
    PicInfo           *pic_info_;
};
#endif /* PICTURE_DECODER_PLUGIN_H_ */
#endif
