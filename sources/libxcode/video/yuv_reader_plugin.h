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

#ifndef YUV_READER_PLUGIN_H_
#define YUV_READER_PLUGIN_H_

#if defined ENABLE_RAW_DECODE

#include "mfxvideo++.h"
#include "mfxplugin++.h"
#include "base/media_common.h"
#include "base/logger.h"

//Task structure for plugin
typedef struct {
    RawInfo *rawInfo;    //Pointer to picture info
    mfxFrameSurface1  *surfaceOut; //Pointer to output surface
    bool              bBusy;       //Task is in use or not
} MFXTask;

class YUVReaderPlugin: public MFXGenericPlugin
{
public:
    DECLARE_MLOGINSTANCE();
    YUVReaderPlugin();
    void SetAllocPoint(MFXFrameAllocator *pMFXAllocator);
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out);
    virtual mfxStatus Init(mfxVideoParam *par);
    virtual mfxStatus Close();
    virtual mfxStatus GetVideoParam(mfxVideoParam *par);
    virtual ~YUVReaderPlugin();
    mfxExtBuffer *GetExtBuffer(mfxExtBuffer **ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

    // Interface called by Media SDK framework
    mfxStatus PluginInit(mfxCoreInterface *core);
    mfxStatus PluginClose();
    mfxStatus GetPluginParam(mfxPluginParam *par);
    mfxStatus Submit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task); // called to submit task but not execute it
    mfxStatus Execute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a); // function that called to start task execution and check status of execution
    mfxStatus FreeResources(mfxThreadTask task, mfxStatus sts); // free task and releated resources
    mfxStatus SetAuxParams(void *auxParam, int auxParamSize);
    void      Release();


    // Interface for yuv reader
    virtual mfxStatus LoadNextFrame(mfxFrameSurface1* pSurface);
    void SetMultiView() { m_bIsMultiView = true; }
    virtual mfxStatus DecodeHeader(RawInfo *rawInfo, mfxVideoParam *par);

protected:
    FILE* m_fSource, **m_fSourceMVC;
    bool m_bInited, m_bIsMultiView;
    mfxU32 m_numLoadedFiles;

private:
    mfxStatus Init(const char *strFileName, const mfxU32 ColorFormat, const mfxU32 numViews, std::vector<char*> srcFileBuff);

    mfxU32 m_ColorFormat; // color format of input YUV data, YUV420 or NV12
    mfxPluginParam     m_mfxPluginParam;  //Plug-in parameters
    mfxCoreInterface  *m_pmfxCore;        //Pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxFrameAllocator *m_pAlloc;
    mfxVideoParam     *m_VideoParam;
    mfxExtBuffer      *m_ExtParam;
    MFXTask           *m_pTasks;          //Array of tasks
    mfxU16             m_MaxNumTasks;     //Max number of concurrent tasks
    bool               m_bIsOpaque;
    int                raw_width_;        //width of bitmap
    int                raw_height_;       //height of bitmap
    RawInfo           *raw_info_;

};

#endif // #if defined ENABLE_RAW_DECODE

#endif /* YUV_READER_PLUGIN_H_ */

