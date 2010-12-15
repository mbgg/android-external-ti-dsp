/*******************************************************************************
 * omx-dsp-aac.h
 *
 * Definition of AAC DSP-accelerated audio decoder. 
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

#ifndef __OMX_DSP_AAC_H
#define __OMX_DSP_AAC_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

/* ...base audio decoder */
#include "omx-dsp-audio.h"

#define OMX_DSP_AAC_INPUT_BUFFER_NUMBER 10
#define OMX_DSP_AAC_INPUT_BUFFER_SIZE 1536
#define OMX_DSP_AAC_OUTPUT_BUFFER_NUMBER 9
#define OMX_DSP_AAC_OUTPUT_BUFFER_SIZE 8192

/*******************************************************************************
 * Component definition
 ******************************************************************************/

class OmxDspAacDecoder : public OmxDspAudioDecoder
{
public:

    /***************************************************************************
     * Constructor / destructor
     **************************************************************************/

    OmxDspAacDecoder ();
    ~OmxDspAacDecoder ();

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

    /* ...add ADIF header for AAC stream stored into the MP4 container */
    void                    Create_ADIF_Header (void);

    /* ...variable to store sample-rate index and channels. Uses in case MPEG-4 container only */
    int                     rate_Idx;
    int                     n_Channels;
};

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...component factory */
extern OMX_ERRORTYPE AacOmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount);

/* ...component destructor */
extern OMX_ERRORTYPE AacOmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount);

#endif

