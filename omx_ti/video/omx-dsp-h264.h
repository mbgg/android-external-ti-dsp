/*******************************************************************************
 * omx-dsp-h264.h
 *
 * Definition of H.264 DSP-accelerated video decoder. 
 *
 * Copyright (C) 2010 Konstantin Kozhevnikov <konstantin.kozhevnikov@gmail.com>
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

#ifndef __OMX_DSP_H264_H
#define __OMX_DSP_H264_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

/* ...base video decoder */
#include "omx-dsp-video.h"

/*******************************************************************************
 * Component definition
 ******************************************************************************/

class OmxDspAvcDecoder : public OmxDspVideoDecoder
{
public:

    /***************************************************************************
     * Constructor / destructor
     **************************************************************************/

    OmxDspAvcDecoder ();
    ~OmxDspAvcDecoder ();

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
};

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...component factory */
extern OMX_ERRORTYPE AvcOmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount);

/* ...component destructor */
extern OMX_ERRORTYPE AvcOmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount);

#endif  /* __OMX_DSP_H264_H */
