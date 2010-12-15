/*******************************************************************************
 * omx-dsp-mp3.h
 *
 * Definition of MP3 DSP-accelerated audio decoder. 
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

#ifndef __OMX_DSP_MP3_H
#define __OMX_DSP_MP3_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

/* ...DMAI Buffer_Object definition */
#include <ti/sdo/dmai/priv/_Buffer.h>

/* ...base audio decoder */
#include "omx-dsp-audio.h"

#define OMX_DSP_MP3_INPUT_BUFFER_NUMBER 10
#define OMX_DSP_MP3_INPUT_BUFFER_SIZE 8192
#define OMX_DSP_MP3_OUTPUT_BUFFER_NUMBER 9
#define OMX_DSP_MP3_OUTPUT_BUFFER_SIZE 4608

/* ...must be the same as in MP3 OpenCORE node */
#define PVMP3FF_DEFAULT_MAX_FRAMESIZE       4096

/*******************************************************************************
 * Component definition
 ******************************************************************************/

class OmxDspMp3Decoder : public OmxDspAudioDecoder
{
public:

    /***************************************************************************
     * Constructor / destructor
     **************************************************************************/

    OmxDspMp3Decoder ();
    ~OmxDspMp3Decoder ();

    /***************************************************************************
     * Framework API
     **************************************************************************/

    /* ...component factory */
    OMX_ERRORTYPE           ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName);

    /* ...component destructor */
    OMX_ERRORTYPE           DestroyComponent (void);

private:

    /***************************************************************************
     * Overloaded functions
     **************************************************************************/

    /* ...decoder initialization */
    OMX_ERRORTYPE           DSPDecoderInit (void);

    /* ...input data processing function */
    OMX_BOOL                DSPPut (OMX_BUFFERHEADERTYPE *b);

    /* ...resize buffer table */
    int                     DSPResize (Buffer_Handle hBuf);
};

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...component factory */
extern OMX_ERRORTYPE Mp3OmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount);

/* ...component destructor */
extern OMX_ERRORTYPE Mp3OmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount);

#endif

