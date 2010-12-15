/*******************************************************************************
 * omx-dsp-mpeg4.cpp
 *
 * Definition of MPEG4 / H.263 DSP-accelerated video decoder. 
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

#define MODULE_TAG                      MPEG4

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "omx-dsp.h"
#include "omx-dsp-mpeg4.h"

/*******************************************************************************
 * Debugging configuration
 ******************************************************************************/

/* ...tracing configuration */
TRACE_TAG (INIT,    1);
TRACE_TAG (DATA,    0);
TRACE_TAG (PROCESS, 0);
TRACE_TAG (DEBUG,   0);
TRACE_TAG (ERROR,   1);
TRACE_TAG (INFO,    0);

/*******************************************************************************
 * Local constants definitions
 ******************************************************************************/

static const OMX_STRING     mpeg4_decoder_name = (OMX_STRING) "mpeg4dec";
static const OMX_STRING     h263_decoder_name = (OMX_STRING) "h263dec";

/*******************************************************************************
 * Plugin system infrastructure
 ******************************************************************************/

/*******************************************************************************
 * Mpeg4OmxDspComponentFactory
 *
 * This function is called by OMX_GetHandle and it creates an instance of the 
 * MPEG4 component AO
 ******************************************************************************/

OMX_ERRORTYPE Mpeg4OmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspMpeg4Decoder     *d;
    OMX_ERRORTYPE           Status;

    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /***************************************************************************
     * Create decoder component object
     **************************************************************************/

    if ((d = (OmxDspMpeg4Decoder*)OSCL_NEW(OmxDspMpeg4Decoder, ())) == NULL)
    {
        return TRACE (ERROR, _X("Failed to allocate resources")), OMX_ErrorInsufficientResources;
    }

    /***************************************************************************
     * Call the construct component to initialize OMX types
     **************************************************************************/

    /* ...construct component */
    Status = d->ConstructComponent (pAppData, pProxy, mpeg4_decoder_name);

    /* ...and return the handle and status */
    return *pHandle = d->GetOmxHandle(), TRACE (DEBUG, _b("MPEG-4 decoder component[%p] created: %d"), (void*)(*pHandle), Status), Status;
}

/*******************************************************************************
 * Mpeg4OmxDspComponentDestructor
 *
 * This function is called by OMX_FreeHandle when component AO needs to be 
 * destroyed
 ******************************************************************************/

OMX_ERRORTYPE Mpeg4OmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspMpeg4Decoder     *d = (OmxDspMpeg4Decoder*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;
    
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /* ...clean up decoder, OMX component stuff */
    d->DestroyComponent();

    /* ...destroy the AO class */
    OSCL_DELETE(d);

    /* ...and return success result */
    return TRACE (DEBUG, _b("MPEG-4 decoder component[%p] destroyed"), (void*)pHandle), OMX_ErrorNone;
}


/*******************************************************************************
 * H263OmxDspComponentFactory
 *
 * This function is called by OMX_GetHandle and it creates an instance of the 
 * H263 component AO
 ******************************************************************************/

OMX_ERRORTYPE H263OmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspMpeg4Decoder     *d;
    OMX_ERRORTYPE           Status;

    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /***************************************************************************
     * Create decoder component object
     **************************************************************************/

    if ((d = (OmxDspMpeg4Decoder*)OSCL_NEW(OmxDspMpeg4Decoder, ())) == NULL)
    {
        return TRACE (ERROR, _X("Failed to allocate resources")), OMX_ErrorInsufficientResources;
    }

    /***************************************************************************
     * Call the construct component to initialize OMX types
     **************************************************************************/

    /* ...construct component */
    Status = d->ConstructComponent (pAppData, pProxy, "mpeg4dec");

    /* ...and return the handle and status */
    return *pHandle = d->GetOmxHandle(), TRACE (DEBUG, _b("H263 decoder component[%p] created: %d"), (void*)(*pHandle), Status), Status;
}

/*******************************************************************************
 * H263OmxDspComponentDestructor
 *
 * This function is called by OMX_FreeHandle when component AO needs to be 
 * destroyed
 ******************************************************************************/

OMX_ERRORTYPE H263OmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspMpeg4Decoder   *d = (OmxDspMpeg4Decoder*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;
    
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /* ...clean up decoder, OMX component stuff */
    d->DestroyComponent();

    /* ...destroy the AO class */
    OSCL_DELETE(d);

    /* ...and return success result */
    return TRACE (DEBUG, _b("H263 decoder component[%p] destroyed"), (void*)pHandle), OMX_ErrorNone;
}

/*******************************************************************************
 * Class implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspVideoDecoder::OmxDspVideoDecoder
 *
 * Component object constructor
 ******************************************************************************/

OmxDspMpeg4Decoder::OmxDspMpeg4Decoder()
{
    TRACE (DEBUG, _b("Component[%p] constructed"), this);
    
    /* ...add component to scheduler */
    (!IsAdded() ? AddToScheduler(), 1 : 0);
}

/*******************************************************************************
 * OmxDspMpeg4Decoder::~OmxDspMpeg4Decoder
 *
 * Component object destructor
 ******************************************************************************/

OmxDspMpeg4Decoder::~OmxDspMpeg4Decoder()
{
    /* ...remove from scheduler as required */
    (IsAdded() ? RemoveFromScheduler(), 1 : 0);

    TRACE (DEBUG, _b("Component[%p] destructed"), this);
}

/*******************************************************************************
 * Component interface implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspMpeg4Decoder::ConstructComponent
 *
 * Construct component
 ******************************************************************************/

OMX_ERRORTYPE OmxDspMpeg4Decoder::ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName)
{
    ComponentPortType  *pInPort, *pOutPort;
    OMX_ERRORTYPE       Status;

    /***************************************************************************
     * Call base class construction function first
     **************************************************************************/

    if ((Status = OmxDspVideoDecoder::ConstructComponent (pAppData, pProxy, cDecoderName)) != OMX_ErrorNone)
    {
        /* ...base component creation failed; resign */
        return Status;
    }
    
    /***************************************************************************
     * Adjust PV capabilities of the component
     **************************************************************************/

    /* ...do not pass NAL start codes (default setting) */
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;

    /* ...use NAL mode of framework (default setting) */
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_FALSE;

#if 0 /* OpenCORE 2.0.7 */
    /* ...to-be-understood (presumably, this stuff is unused) */
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#endif

    /***************************************************************************
     * Initialize ports
     **************************************************************************/

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /***************************************************************************
     * Input port configuration - encoded MPEG4 video stream
     **************************************************************************/

    if (cDecoderName == mpeg4_decoder_name)
    {
        /***********************************************************************
         * Configuration for MPEG-4
         **********************************************************************/

        /* ...input port configuration */
        pInPort->PortParam.format.video.cMIMEType = (OMX_STRING)"video/mpeg4";
        pInPort->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;

        /* ...default values for MPEG-4 video param port */
        pInPort->VideoMpeg4.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        pInPort->VideoMpeg4.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
        pInPort->VideoMpeg4.eLevel = OMX_VIDEO_MPEG4Level4a;
        
        /* ...profile configuration */
        pInPort->ProfileLevel.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        pInPort->ProfileLevel.nProfileIndex = 0;
        pInPort->ProfileLevel.eProfile = OMX_VIDEO_MPEG4ProfileSimple;
        pInPort->ProfileLevel.eLevel = OMX_VIDEO_MPEG4Level4a;

        /* ...video format */
        pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingMPEG4;

    }
    else
    {
        /***********************************************************************
         * Configuration for H.263
         **********************************************************************/

        /* ...input port configuration */
        pInPort->PortParam.format.video.cMIMEType = (OMX_STRING)"video/h263";
        pInPort->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;

        /* ...default values for H.263 video param port */
        pInPort->VideoH263.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        pInPort->VideoH263.eProfile = OMX_VIDEO_H263ProfileBaseline;
        pInPort->VideoH263.eLevel = OMX_VIDEO_H263Level70;

        /* ...profile configuration (well, we do support profile 3 - tbd) */
        pInPort->ProfileLevel.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
        pInPort->ProfileLevel.nProfileIndex = 0;
        pInPort->ProfileLevel.eProfile = OMX_VIDEO_H263ProfileBaseline;
        pInPort->ProfileLevel.eLevel = OMX_VIDEO_H263Level70;

        /* ...video format */
        pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingH263;
    }
    
    /***************************************************************************
     * Common configuration of output port
     **************************************************************************/

#if 0 /* OpenCORE 2.0.7 */
    /* ...deblocking setting */
    SetHeader(&pOutPort->VideoDeBlocking, sizeof(OMX_PARAM_DEBLOCKINGTYPE));
    
    pOutPort->VideoDeBlocking.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->VideoDeBlocking.bDeblocking = OMX_FALSE;
#endif

    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("%s decoder component constructed"), (iDecoderName == mpeg4_decoder_name ? "MPEG-4" : "H.263")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspMpeg4Decoder::DestroyComponent
 *
 * This function is called by the omx core when the component is disposed by 
 * the IL client with a call to FreeHandle().
 ******************************************************************************/

OMX_ERRORTYPE OmxDspMpeg4Decoder::DestroyComponent (void)
{
    OMX_ERRORTYPE   Status;
    
    /***************************************************************************
     * Pass control to base version (no specific actions)
     **************************************************************************/

    return Status = OmxDspVideoDecoder::DestroyComponent(), TRACE (INIT, _b("%s decoder component destructed"), (iDecoderName == mpeg4_decoder_name ? "MPEG-4" : "H.263")), Status;
}
