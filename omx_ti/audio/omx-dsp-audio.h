/*******************************************************************************
 * omx-dsp-audio.h
 *
 * Definition of base class for DSP-accelerated audio decoder. 
 *
 * Copyright (C) 2010 Alexander Smirnov <asmirnov.bluesman@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#ifndef __OMX_DSP_AUDIO_H
#define __OMX_DSP_AUDIO_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

/* ...opencore headers */
#include "oscl_base.h"
#include "OMX_Types.h"
#include "pv_omxdefs.h"
#include "omx_proxy_interface.h"
#include "oscl_dll.h"
#include "pv_omxcomponent.h"

/* ...DMAI headers */
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/dmai/Dmai.h>
#include <ti/sdo/dmai/ce/Adec1.h>
#include <ti/sdo/dmai/Buffer.h>
#include <ti/sdo/dmai/BufTab.h>

/* ...maximum frame size, must be equals max(aac, mp3) */
#define OMX_MAX_AUDIO_FRAME_SIZE 4096

/*******************************************************************************
 * Component definition
 ******************************************************************************/

class OmxDspAudioDecoder : public OmxComponentAudio
{
public:

    /***************************************************************************
     * Constructor / destructor
     **************************************************************************/

    OmxDspAudioDecoder ();
    ~OmxDspAudioDecoder ();

    /***************************************************************************
     * Framework API
     **************************************************************************/

    /* ...base component constructor */
    OMX_ERRORTYPE           ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING);

    /* ...base component destructor */
    OMX_ERRORTYPE           DestroyComponent (void);

    /* ...component initialization routine */
    OMX_ERRORTYPE           ComponentInit (void);

    /* ...component deinitialization routine */
    OMX_ERRORTYPE           ComponentDeInit (void);

    /* ...get configuration (do we need that?) */
    OMX_ERRORTYPE           GetConfig (OMX_HANDLETYPE hComponent, OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure);

    /* ...calculate output buffer parameters */
    void                    CalculateBufferParameters (OMX_U32 PortIndex);

    /* ...main processing function */
    void                    BufferMgmtFunction (void);

    /* ...specific processing function (stub) */
    void                    ProcessData (void);

    /* ...component reset routine */
    void                    ResetComponent (void);

    /* ...buffer allocation routine */
    static OMX_ERRORTYPE    BaseComponentAllocateBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** pBuffer, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes);

    /* ...buffer freeing routine */
    static OMX_ERRORTYPE    BaseComponentFillThisBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer);

    /* ...buffer freeing routine */
    static OMX_ERRORTYPE    BaseComponentFreeBuffer (OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer);

protected:

    /***************************************************************************
     * Internal data
     **************************************************************************/

    /* ...codec server engine */
	Engine_Handle           hEngine;

    /* ...DMAI IVIDDEC2 interface handle */
	Adec1_Handle            hAd;

    /* ...output buffer */
	BufTab_Handle           hOutBufTab;

    /* ...reference to display buffer */
    Buffer_Handle           hDispBuffer;

    /* ...decoder engine name */
    Char                   *iDecoderName;

    /* ...decoding status? (to-be-understood) */
    OMX_BOOL                iDecodeReturn;
    
    /* ...first data frame flag */
    OMX_BOOL                iFirstDataFrame;
    /***************************************************************************
     * DSP input/output buffers data
     **************************************************************************/

    /* ...reference to active DSP output buffer */
    Buffer_Handle           hOutBuf;    

    /* ...buffer handle */
    Buffer_Handle           hDSPBuffer;

    /* ...pointer to the DSP buffer storage */
    OMX_U8                 *pDSPBuffer;

    /* ...current writing index */
    OMX_U32                 iDSPWriteIndex;

    /* ...DSP buffer storage size */
    OMX_U32                 iDSPBufferSize;

    /* ...encoded data offset from the head in DSP buffer */
    OMX_U32                 iDSPBufferOffset;

    /***************************************************************************
     * Private functions
     **************************************************************************/

    /* ...initialize DSP decoder */
    virtual OMX_ERRORTYPE   DSPDecoderInit (void);
    
    /* ...cleanup DSP decoder */
    virtual void            DSPDecoderClean (void);

    /* ...data-collection function */
    virtual OMX_BOOL        DSPPut (OMX_BUFFERHEADERTYPE *b);

    /* ...resize buffer table */
    virtual int             DSPResize (Buffer_Handle hBuf);

    /***************************************************************************
     * Non-virtual DSP supporting functions
     **************************************************************************/

    /* ...data-retrieval function */
    OMX_BOOL                DSPGet (OMX_BUFFERHEADERTYPE *b);
    
    /* ...audio decoding procedure */
    OMX_BOOL                DSPProcess (OMX_PARAM_PORTDEFINITIONTYPE* aPortParam, OMX_BOOL *aResizeFlag);

    /* ...release output buffer back to pool */
    void                    DSPRelease (OMX_BUFFERHEADERTYPE *b);
    
    /* ...flush audio decoder */
    void                    DSPFlush (void);
};

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...register DLL */
extern "C" OSCL_EXPORT_REF OsclAny* PVGetInterface (void);

/* ...unregister DLL */
extern "C" OSCL_EXPORT_REF void     PVReleaseInterface (OsclSharedLibraryInterface *aInstance);


#endif

