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

#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

#include "os/XC_Thread.h"
#include "video/msdk_xcoder.h"
#include "FFmpegReader.h"
#include "SinkWrap.h"
#include "LogPrintThread.h"
#include "cmdProcessor.h"

namespace TranscodingSample
{

    class Launcher : public BitstreamSource::ErrListener
    {
    public:
        Launcher();
        virtual ~Launcher();

        virtual mfxStatus Init(InputParams &paramList);
        virtual void      Run();
        virtual mfxStatus ProcessResult(CmdProcessor &cmdParser);
        virtual void      Stop();
    protected:
        mfxU64                              m_StartTime;
    private:
        struct VppEnc {
            VppOptions vpp;
            EncOptions enc;
        };

        typedef struct {
            union {
                StringInfo *stringinfo;
                PicInfo *picinfo;
                RawInfo *rawInfo;
            };
            union {
                void *dec_handle;
                void *vpp_handle;
                void *enc_handle;
            };
            int type;
        } BlendInfo;

        std::unique_ptr<FFmpegReader> m_pBitstreamSource;
        std::unique_ptr<SinkWrap> m_pVideoSink;
        std::unique_ptr<MsdkXcoder> m_pPipeline;
        std::list<Stream*> m_mvoutStreamList;
        std::list<Stream*> m_mvinStreamList;
        std::list<Stream*> m_mbcodeoutStreamList;
        std::list<Stream*> m_weightStreamList;


        // for pipeline config
        Measurement measurement;
        LogPrintThread* logPrintThread;

        mfxStatus InitInputCfg(const DecParams &decInputParams, DecOptions &dec_cfg);
        mfxStatus InitOutputCfg(const EncParams &encInputParam, EncOptions &enc_cfg);
        mfxStatus InitVppCfg(const sVppParams &vppInputParam, VppOptions &vpp_cfg);
        void AddPicBlendInfo(const CompositionParams &compositionInputParams, VppOptions &vppCfg, BlendInfo &blendInfo);
        void AttachPicBlend(MsdkXcoder *xcoder, unsigned long time, void *vppHandle, BlendInfo &blendInfo);
        void DetachPicBlend(MsdkXcoder *xcoder, unsigned long time, BlendInfo &blendInfo);
        void AttachNewTranscoder(MsdkXcoder *xcoder, VppOptions &vppCfg, EncOptions &encCfg, bool haveVPP);
        void DetachTranscoder(MsdkXcoder *xcoder, VppOptions &vpp);
        bool CheckNeedVPP(sVppParams &vppInputParams);
        mfxU16 CalCuEncRequestSurface(EncParams &encInputParams);
        mfxU16 GetDecOuputSurfNum(InputParams &paramList);


        // for BitstreamSource::ErrListener
        virtual void onError(void);

        bool start_trace;
        bool start_log_print;
    };
}

#endif /* __LAUNCHER_H__ */
