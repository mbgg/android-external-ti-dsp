/*******************************************************************************
 * omx-dsp-h264.cpp
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

#define MODULE_TAG                      H264

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "omx-dsp.h"
#include "omx-dsp-h264.h"

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

static const OMX_STRING     avc_decoder_name = (OMX_STRING) "h264dec";

/*******************************************************************************
 * Plugin system infrastructure
 ******************************************************************************/

/*******************************************************************************
 * AvcOmxDspComponentFactory
 *
 * This function is called by OMX_GetHandle and it creates an instance of the 
 * avc component AO
 ******************************************************************************/

OMX_ERRORTYPE AvcOmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspAvcDecoder   *d;
    OMX_ERRORTYPE       Status;

    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /***************************************************************************
     * Create decoder component object
     **************************************************************************/

    if ((d = (OmxDspAvcDecoder*)OSCL_NEW(OmxDspAvcDecoder, ())) == NULL)
    {
        return TRACE (ERROR, _X("Failed to allocate resources")), OMX_ErrorInsufficientResources;
    }

    /***************************************************************************
     * Call the construct component to initialize OMX types
     **************************************************************************/

    /* ...construct component */
    Status = d->ConstructComponent (pAppData, pProxy, avc_decoder_name);

    /* ...and return the handle and status */
    return *pHandle = d->GetOmxHandle(), TRACE (DEBUG, _b("Component[%p] created: %d"), (void*)(*pHandle), Status), Status;
}

/*******************************************************************************
 * AvcOmxDspComponentDestructor
 *
 * This function is called by OMX_FreeHandle when component AO needs to be 
 * destroyed
 ******************************************************************************/

OMX_ERRORTYPE AvcOmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspAvcDecoder   *d = (OmxDspAvcDecoder*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;
    
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /* ...clean up decoder, OMX component stuff */
    d->DestroyComponent();

    /* ...destroy the AO class */
    OSCL_DELETE(d);

    /* ...and return success result */
    return TRACE (DEBUG, _b("Component[%p] destroyed"), (void*)pHandle), OMX_ErrorNone;
}

/*******************************************************************************
 * Class implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspVideoDecoder::OmxDspVideoDecoder
 *
 * Component object constructor
 ******************************************************************************/

OmxDspAvcDecoder::OmxDspAvcDecoder()
{
    TRACE (DEBUG, _b("Component[%p] constructed"), this);
    
    /* ...add component to scheduler */
    (!IsAdded() ? AddToScheduler(), 1 : 0);
}

/*******************************************************************************
 * OmxDspAvcDecoder::~OmxDspAvcDecoder
 *
 * Component object destructor
 ******************************************************************************/

OmxDspAvcDecoder::~OmxDspAvcDecoder()
{
    /* ...remove from scheduler as required */
    (IsAdded() ? RemoveFromScheduler(), 1 : 0);

    TRACE (DEBUG, _b("Component[%p] destructed"), this);
}

/*******************************************************************************
 * Component interface implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspAvcDecoder::ConstructComponent
 *
 * Construct component
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAvcDecoder::ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName)
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

    /***************************************************************************
     * Initialize ports
     **************************************************************************/

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /***************************************************************************
     * Input port configuration - encoded AVC video stream
     **************************************************************************/

    /* ...input video format */
    pInPort->PortParam.format.video.cMIMEType = (OMX_STRING)"video/Avc";
    pInPort->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

    /* ...default values for Avc video param port */
    pInPort->VideoAvc.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->VideoAvc.eProfile = OMX_VIDEO_AVCProfileBaseline;
    pInPort->VideoAvc.eLevel = OMX_VIDEO_AVCLevel1;

    /* ...profile configuration */
    pInPort->ProfileLevel.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->ProfileLevel.nProfileIndex = 0;
    pInPort->ProfileLevel.eProfile = OMX_VIDEO_AVCProfileBaseline;
    pInPort->ProfileLevel.eLevel = OMX_VIDEO_AVCLevel1;

    /* ...video format */
    pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingAVC;

    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("AVC decoder component constructed")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspAvcDecoder::DestroyComponent
 *
 * This function is called by the omx core when the component is disposed by 
 * the IL client with a call to FreeHandle().
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAvcDecoder::DestroyComponent (void)
{
    OMX_ERRORTYPE   Status;
    
    /***************************************************************************
     * Pass control to base version (no specific actions)
     **************************************************************************/

    return Status = OmxDspVideoDecoder::DestroyComponent(), TRACE (INIT, _b("AVC decoder component destructed")), Status;
}
/*******************************************************************************
 * DSP engine support
 ******************************************************************************/

/*******************************************************************************
 * OmxDspAvcDecoder::DSPDecoderInit
 *
 * Initialize DSP decoder 
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAvcDecoder::DSPDecoderInit (void)
{
    OMX_ERRORTYPE   Status;

    /***************************************************************************
     * Initialize base video decoder
     **************************************************************************/

    if ((Status = OmxDspVideoDecoder::DSPDecoderInit()) != OMX_ErrorNone)
    {
        return Status;
    }

    /***************************************************************************
     * Put NAL start code into DSP buffer head (and do not touch it anymore)
     **************************************************************************/
    
    /* ...put NAL start code in the head of buffer (shall never be changed) */
    oscl_memset (pDSPBuffer, 0x00, 3), pDSPBuffer[3] = 0x01, iDSPWriteIndex = 4;

    /***************************************************************************
     * Decoder engine successfully created
     **************************************************************************/

    return TRACE (INIT, _b("AVC video decoder initialized")), OMX_ErrorNone;
}
    
/*******************************************************************************
 * OmxDspAvcDecoder::DSPPut
 *
 * Copy data from input buffer to DSP buffer (use NAL start codes)
 ******************************************************************************/

OMX_BOOL OmxDspAvcDecoder::DSPPut (OMX_BUFFERHEADERTYPE* b)
{
    OMX_U32     n = b->nFilledLen;
    OMX_U32     e;
    
    TRACE (DATA, _b("copy %u bytes: [%lu, %lu)"), (unsigned)n, iDSPWriteIndex, iDSPWriteIndex + n);
    
    /* ...check out there is enough room to store data (TBD) */
    if ((e = iDSPWriteIndex + n) > iDSPBufferSize) return TRACE (DATA, _b("Input buffer to large")), OMX_FALSE;
    
    /* ...copy input buffer content into DSP-owned buffer */
    oscl_memcpy (pDSPBuffer + iDSPWriteIndex, b->pBuffer + b->nOffset, n);
    
    /* ...set input buffer length and reset writing pointer (keep NAL start code) for a final chunk */
    (iEndOfFrameFlag == OMX_TRUE ? TRACE (DATA, _b("Received frame: %lu bytes"), e - 4), Buffer_setNumBytesUsed (hDSPBuffer, e), e = 4 : 0);

    /* ...advance writing index and return success result */
    return iDSPWriteIndex = e, OMX_TRUE;
}
