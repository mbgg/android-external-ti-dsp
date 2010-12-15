/*******************************************************************************
 * omx-dsp-video.cpp
 *
 * Definition of base class for DSP-accelerated video decoder. 
 * Based on omx_h264 sw codec by PacketVideo.
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

#define MODULE_TAG                      VIDEO

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "omx-dsp.h"
#include "omx-dsp-video.h"
#include "omx-dsp-h264.h"
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

/* ...performance counters */
PC_TAG (PROCESS, 1);
PC_TAG (HOLD, 1);

/*******************************************************************************
 * Plugin system infrastructure
 ******************************************************************************/

OSCL_DLL_ENTRY_POINT_DEFAULT()

/*******************************************************************************
 * Local macros definitions
 ******************************************************************************/

/*******************************************************************************
 * UUID_FMT, UUID_ARG
 *
 * Format string and arguments list for UUID printing
 ******************************************************************************/

/* ...UUID printing format string */
#define UUID_FMT                        "%08X:%04hX:%04hX:%02X%02X%02X%02X%02X%02X%02X%02X"

/* ...UUID printing arguments */
#define UUID_ARG(uuid)                  uuid.data1, uuid.data2, uuid.data3, uuid.data4[0], uuid.data4[1], uuid.data4[2], uuid.data4[3], uuid.data4[4], uuid.data4[5], uuid.data4[6], uuid.data4[7]

/*******************************************************************************
 * OMX_PROXY_BIND
 *
 * Bind framework hook to the component
 ******************************************************************************/

#if PROXY_INTERFACE  
#define OMX_PROXY_BIND(c, p, hook)      (void)((c).hook = BaseComponentProxy##hook, ((ProxyApplication_OMX*)(p))->Component##hook = BaseComponent##hook)
#else
#define OMX_PROXY_BIND(c, p, hook)      (void)((c).hook = BaseComponent##hook)
#endif

/*******************************************************************************
 * Local constants definitions
 ******************************************************************************/

/* ...desired input buffer size */
#define OMX_DSP_INPUT_BUFFER_SIZE       65536

/* ...desired amount of input buffers */
#define OMX_DSP_INPUT_BUFFER_NUMBER     8

/* ...required amount of output buffers */
#define OMX_DSP_OUTPUT_BUFFER_NUMBER    2

/*******************************************************************************
 * OmxDspSharedLibraryInterface
 *
 * Interface binding helper
 ******************************************************************************/

class OmxDspSharedLibraryInterface : public OsclSharedLibraryInterface, public OmxSharedLibraryInterface
{
public:
    
    /***************************************************************************
     * QueryOmxComponentInterface
     *
     * Select proper entry point basing on provided type ID
     **************************************************************************/

    OsclAny *QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId)
    {
        TRACE (INFO, _b("Interface queried: type = " UUID_FMT ", itfc = " UUID_FMT), UUID_ARG(aOmxTypeId), UUID_ARG(aInterfaceId));
        
        if (PV_OMX_AVCDEC_UUID == aOmxTypeId)
        {
            if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
            {
                /* ...query for component factory */
                return ((OsclAny*)(&AvcOmxDspComponentFactory));
            }
            else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
            {
                /* ...query for component destructor */
                return ((OsclAny*)(&AvcOmxDspComponentDestructor));
            }
        }
        else if (PV_OMX_M4VDEC_UUID == aOmxTypeId)
        {
            if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
            {
                /* ...query for component factory */
                return ((OsclAny*)(&Mpeg4OmxDspComponentFactory));
            }
            else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
            {
                /* ...query for component destructor */
                return ((OsclAny*)(&Mpeg4OmxDspComponentDestructor));
            }
        }
        else if (PV_OMX_H263DEC_UUID == aOmxTypeId)
        {
            if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
            {
                /* ...query for component factory */
                return ((OsclAny*)(&H263OmxDspComponentFactory));
            }
            else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
            {
                /* ...query for component destructor */
                return ((OsclAny*)(&H263OmxDspComponentDestructor));
            }
        }
        
        /* ...no valid entry point found */
        return NULL;
    }

    /***************************************************************************
     * SharedLibraryLookup
     *
     * Register the library in plugin framework
     **************************************************************************/

    OsclAny *SharedLibraryLookup(const OsclUuid& aInterfaceId)
    {
        TRACE (INFO, _b("Library lookup: itfc = " UUID_FMT), UUID_ARG(aInterfaceId));

        if (aInterfaceId == PV_OMX_SHARED_INTERFACE)
        {
            return OSCL_STATIC_CAST(OmxSharedLibraryInterface*, this);
        }

        return NULL;
    }
};

/*******************************************************************************
 * Plugin entry points
 ******************************************************************************/

extern "C" OSCL_EXPORT_REF OsclAny* PVGetInterface (void)
{
    TRACE (INIT, _b("Register DLL within Opencore"));
    
    return (OsclAny*) OSCL_NEW(OmxDspSharedLibraryInterface, ());
}

extern "C" OSCL_EXPORT_REF void PVReleaseInterface (OsclSharedLibraryInterface *aInstance)
{
    OmxDspSharedLibraryInterface   *module = (OmxDspSharedLibraryInterface*)aInstance;

    OSCL_DELETE (module);

    TRACE (INIT, _b("DLL unregistered"));
}

/*******************************************************************************
 * Base vide decoder component interface implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspVideoDecoder::ConstructComponent
 *
 * Base implementation of component constructor
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName)
{
    ComponentPortType  *pInPort, *pOutPort;
    OMX_ERRORTYPE       Status;

    /***************************************************************************
     * Local data initialization
     **************************************************************************/

    /* ...component has two ports */
    iNumPorts = 2;

    /* ...set proxy pointer */
    ipComponentProxy = pProxy;

    /* ...set component size */
    iOmxComponent.nSize = sizeof(OMX_COMPONENTTYPE);

    /* ...component private data points to this AO */
    iOmxComponent.pComponentPrivate = (OMX_PTR) this;

    /* ...save opaque appplication data pointer */
    iOmxComponent.pApplicationPrivate = pAppData;

    /* ...save decoder name */
    iDecoderName = (Char*) cDecoderName;
    
    /***************************************************************************
     * Set up PV capabilities of the component
     **************************************************************************/

    /* ...use dedicated thread for the component (default setting) */
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;

    /* ...use externally-allocated input buffers (default setting) */
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;

    /* ...use internally-allocated output buffers (overriden) */    
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;

    /* ...the input buffers are movable (default setting) */
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;

    /* ...partial frames are allowed (default setting) */
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE;

    /* ...do not pass NAL start codes (default setting) */
    iPVCapabilityFlags.iOMXComponentUsesNALStartCodes = OMX_FALSE;

    /* ...codec provides packet loss concealment (default seting) */
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;

    /* ...use NAL mode of framework (default setting) */
    iPVCapabilityFlags.iOMXComponentUsesFullAVCFrames = OMX_FALSE;

    /***************************************************************************
     * Bind component interface (both overriden and not)
     **************************************************************************/

    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, SendCommand);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, GetParameter);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, SetParameter);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, GetConfig);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, SetConfig);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, GetExtensionIndex);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, GetState);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, UseBuffer);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, AllocateBuffer);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, FreeBuffer);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, EmptyThisBuffer);
    OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, FillThisBuffer);
    //OMX_PROXY_BIND (iOmxComponent, ipComponentProxy, SetCallbacks);
    iOmxComponent.SetCallbacks = BaseComponentSetCallbacks;

    /* ...specify component version */
    iOmxComponent.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    iOmxComponent.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    iOmxComponent.nVersion.s.nRevision = SPECREVISION;
    iOmxComponent.nVersion.s.nStep = SPECSTEP;

    /***************************************************************************
     * Private data initialization
     **************************************************************************/

    /* ...release private application data if set */
    (ipAppPriv ? oscl_free(ipAppPriv), 1 : 0);
    
    /* ...allocate private data */
    if ((ipAppPriv = (ComponentPrivateType*)oscl_malloc(sizeof(ComponentPrivateType))) == NULL)
    {
        return TRACE (ERROR, _x("Insufficient resources")), OMX_ErrorInsufficientResources;
    }

    /***************************************************************************
     * Construct base class
     **************************************************************************/

    if ((Status = ConstructBaseComponent(pAppData)) != OMX_ErrorNone)
    {
        /* ...failed to construct base class; retreat */
        return TRACE (ERROR, _x("ConstructBaseComponent failed with status: %d"), Status), Status;
    }

    /***************************************************************************
     * Generic video decoder ports initialization
     **************************************************************************/

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /* ...totally we have two ports */
    iPortTypesParam.nPorts = 2, iPortTypesParam.nStartPortNumber = 0;

    /***************************************************************************
     * Input port (partial) configuration - to-be-adjusted in derived class
     **************************************************************************/

    pInPort->PortParam.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->PortParam.eDomain = OMX_PortDomainVideo;
    pInPort->PortParam.format.video.cMIMEType = (OMX_STRING) NULL;
    pInPort->PortParam.format.video.pNativeRender = 0;
    pInPort->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    pInPort->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    pInPort->PortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pInPort->PortParam.format.video.nFrameWidth = VideoStd_D1_WIDTH;
    pInPort->PortParam.format.video.nFrameHeight = VideoStd_D1_PAL_HEIGHT;
    pInPort->PortParam.format.video.nBitrate = 64000;
    pInPort->PortParam.format.video.xFramerate = (15 << 16);
    pInPort->PortParam.eDir = OMX_DirInput;
    pInPort->PortParam.nBufferCountActual = OMX_DSP_INPUT_BUFFER_NUMBER;
    pInPort->PortParam.nBufferCountMin = 1;
    pInPort->PortParam.nBufferSize = OMX_DSP_INPUT_BUFFER_SIZE;
    pInPort->PortParam.bEnabled = OMX_TRUE;
    pInPort->PortParam.bPopulated = OMX_FALSE;

    /***************************************************************************
     * Output port configuartion is fixed (raw video in YUV422ILE format)
     **************************************************************************/

    pOutPort->PortParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->PortParam.eDomain = OMX_PortDomainVideo;
    pOutPort->PortParam.format.video.cMIMEType = (OMX_STRING)"raw";
    pOutPort->PortParam.format.video.pNativeRender = 0;
    pOutPort->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    pOutPort->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pOutPort->PortParam.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
    //pOutPort->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pOutPort->PortParam.format.video.nFrameWidth = VideoStd_D1_WIDTH;
    pOutPort->PortParam.format.video.nFrameHeight = VideoStd_D1_PAL_HEIGHT;
    pOutPort->PortParam.format.video.nBitrate = 64000;
    pOutPort->PortParam.format.video.xFramerate = (15 << 16);
    pOutPort->PortParam.eDir = OMX_DirOutput;
    pOutPort->PortParam.nBufferCountActual = OMX_DSP_OUTPUT_BUFFER_NUMBER;
    pOutPort->PortParam.nBufferCountMin = 1;
    pOutPort->PortParam.nBufferSize = (VideoStd_D1_WIDTH * VideoStd_D1_PAL_HEIGHT * 2);
    //pOutPort->PortParam.nBufferSize = (VideoStd_D1_WIDTH * VideoStd_D1_PAL_HEIGHT * 3) / 2;
    pOutPort->PortParam.bEnabled = OMX_TRUE;
    pOutPort->PortParam.bPopulated = OMX_FALSE;

    /***************************************************************************
     * Input port format(s) supported (to-be-adjusted by derived class)
     **************************************************************************/

    SetHeader(&pInPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));

    /* ...unspecified compressed video */
    pInPort->VideoParam[0].nPortIndex = 0;
    pInPort->VideoParam[0].nIndex = 0;
    pInPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    pInPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatUnused;

    /* ...total amount of video formats */
    pInPort->ActualNumPortFormatsSupported = 1;

    /***************************************************************************
     * Output port format(s) supported 
     **************************************************************************/

    SetHeader(&pOutPort->VideoParam[0], sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));

    /* ...raw YUV422 video */
    pOutPort->VideoParam[0].nPortIndex = 1;
    pOutPort->VideoParam[0].nIndex = 0;
    pOutPort->VideoParam[0].eCompressionFormat = OMX_VIDEO_CodingUnused;
    pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatCbYCrY;
    //pOutPort->VideoParam[0].eColorFormat = OMX_COLOR_FormatYUV420Planar;

    /* ...total amount of video formats */
    pOutPort->ActualNumPortFormatsSupported = 1;

    /***************************************************************************
     * Decoder low-level initialization
     **************************************************************************/

    /* ...mark there is no result from decoder yet (tbd) */
    iDecodeReturn = OMX_FALSE;

    /* ...reset DSP handles */
    hEngine = NULL, hVd = NULL, hDSPBuffer = NULL, hOutBufTab = NULL;
    
    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("Component constructed successfully")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspVideoDecoder::DestroyComponent
 *
 * This function is called by the framework in response to call to FreeHandle
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::DestroyComponent (void)
{
    /***************************************************************************
     * Perform component deinitialization as appropriate
     **************************************************************************/

    (iIsInit != OMX_FALSE ? ComponentDeInit(), 1 : 0);    

    /***************************************************************************
     * Destroy the base class now
     **************************************************************************/

    DestroyBaseComponent();

    /***************************************************************************
     * Release private application data
     **************************************************************************/

    (ipAppPriv ? ipAppPriv->CompHandle = NULL, oscl_free(ipAppPriv), ipAppPriv = NULL : 0);

    /***************************************************************************
     * Component destroyed
     **************************************************************************/

    return TRACE (INIT, _b("Component destroyed")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspVideoDecoder::BufferMgmtFunction
 *
 * Buffer management function
 ******************************************************************************/

void OmxDspVideoDecoder::BufferMgmtFunction (void)
{
    OMX_COMPONENTTYPE  *pHandle = &iOmxComponent;
    QueueType          *pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType          *pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    ComponentPortType  *pInPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ComponentPortType  *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    TRACE (DEBUG, _b(">> Buffer management: IBE:%d EOF:%d EOS:%d (in:%u out:%u)"), (iIsInputBufferEnded == OMX_TRUE ? 1 : 0), (iEndOfFrameFlag == OMX_TRUE ? 1 : 0), (iEndofStream == OMX_TRUE ? 1 : 0), (unsigned)GetQueueNumElem(pInputQueue), (unsigned)GetQueueNumElem(pOutputQueue));

    /***************************************************************************
     * Input data processing
     **************************************************************************/

    if (iEndofStream == OMX_FALSE && iEndOfFrameFlag == OMX_FALSE)
    {
        /***********************************************************************
         * Check out if there is a pending input buffer and decoding is done
         **********************************************************************/

        if (GetQueueNumElem(pInputQueue) > 0)
        {
            /*******************************************************************
             * Retrieve new input buffer from the queue
             ******************************************************************/

            if ((ipInputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue)) == NULL)
            {
                /* ...error is fatal? */
                TRACE (ERROR, _b("Failed to dequeue input buffer"));

                goto error;
            }
            else
            {
                TRACE (DEBUG, _b("New input buffer[%p] received: offset=%u, filled=%u, flags=%X, ts=%u"), ipInputBuffer, (unsigned)ipInputBuffer->nOffset, (unsigned)ipInputBuffer->nFilledLen, (unsigned)ipInputBuffer->nFlags, (unsigned)ipInputBuffer->nTimeStamp);
                
                /* ...check for end-of-stream flag */
                (ipInputBuffer->nFlags & OMX_BUFFERFLAG_EOS ? TRACE (DATA, _b("End-of-stream flag received")), iEndofStream = OMX_TRUE : 0);

                /* ...check for end-of-frame flag */
                (ipInputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME ? TRACE (DATA, _b("End-of-frame flag received")), iEndOfFrameFlag = OMX_TRUE, iFrameTimestamp = ipInputBuffer->nTimeStamp : 0);
            }
            
            /*******************************************************************
             * Process the buffer retrieved
             ******************************************************************/

            if (DSPPut(ipInputBuffer) == OMX_FALSE)
            {
                TRACE (ERROR, _b("Frame is too long; fatal error"));
           
                goto error;
            }

            /*******************************************************************
             * Process mark data (to-be-understood)
             ******************************************************************/
            
            /* ...save marking data associated with incoming buffer */
            if (iTargetMarkData = ipInputBuffer->pMarkData, (ipTargetComponent = (OMX_COMPONENTTYPE*) ipInputBuffer->hMarkTargetComponent) == pHandle)
            {
                TRACE (INFO, _b("Issue mark notification"));

                /* ...notify framework about marking data processing (and pass it down the stack?) */
                (*(ipCallbacks->EventHandler)) (pHandle, iCallbackData, OMX_EventMark, 1, 0, iTargetMarkData);
            }

            /*******************************************************************
             * Release input buffer to the framework
             ******************************************************************/

            ReturnInputBuffer (ipInputBuffer, pInPort);
        }
    }

    /***************************************************************************
     * Decoding task
     **************************************************************************/

    if (iEndOfFrameFlag == OMX_TRUE)
    {
        OMX_BOOL    ResizeNeeded = OMX_FALSE;
        
        /***********************************************************************
         * We have filled input buffer and available output buffer
         **********************************************************************/

        /* ...call DSP engine, check for decoding error (input path may become enabled) */
        PC_START (PROCESS), iDecodeReturn = DSPProcess (&ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam, &ResizeNeeded), PC_STOP (PROCESS);
        
        /***********************************************************************
         * Process decoding error 
         **********************************************************************/

        if (!iDecodeReturn && OMX_FALSE == iEndofStream)
        {
            TRACE (INFO, _b("Issue ErrorStreamCorrupt event"));
                
            /* ...issue a callback to framework */
            (*(ipCallbacks->EventHandler)) (pHandle, iCallbackData, OMX_EventError, OMX_ErrorStreamCorrupt, 0, NULL);
        }

        /***********************************************************************
         * Process resize event (tbd) 
         **********************************************************************/

        if (ResizeNeeded == OMX_TRUE)
        {
            TRACE (INFO, _b("Issue resize event"));
            
            /* ...assert resize flag */
            iResizePending = OMX_TRUE;

            /* ...pass notification to the framework (to application) */
            (*(ipCallbacks->EventHandler)) ((OMX_COMPONENTTYPE*) ipAppPriv->CompHandle, iCallbackData, OMX_EventPortSettingsChanged, OMX_PORT_OUTPUTPORT_INDEX, 0, NULL);
        }
    }
    
    /***************************************************************************
     * Output data processing
     **************************************************************************/

    if (ipOutputBuffer || GetQueueNumElem(pOutputQueue) != 0)
    {
        /***********************************************************************
         * Output buffer is available; get it from the pool as necessary
         **********************************************************************/

        if (ipOutputBuffer == NULL)
        {
            /*******************************************************************
             * Take new buffer from the pool
             ******************************************************************/

            OMX_U32     CurrWidth =  pOutPort->PortParam.format.video.nFrameWidth;
            OMX_U32     CurrHeight = pOutPort->PortParam.format.video.nFrameHeight;

            if ((ipOutputBuffer = (OMX_BUFFERHEADERTYPE*)DeQueue(pOutputQueue)) == NULL)
            {
                /***************************************************************
                 * Failed to get output buffer from pool
                 **************************************************************/

                TRACE (ERROR, _b("Failed to get the output pool; error"));

                goto error;
            }
            else if (ipOutputBuffer->nAllocLen < (OMX_U32)(((CurrWidth + 15) & (~15)) * ((CurrHeight + 15) & (~15)) * 3 / 2))
            {
                /***************************************************************
                 * Do not proceed if the output buffer can't fit the YUV data 
                 **************************************************************/

                TRACE (DATA, _b("Output buffer cannot accomodate YUV image; skip processing"));

                /* ...put output buffer back to the pool */
                ipOutputBuffer->nFilledLen = 0, ReturnOutputBuffer(ipOutputBuffer, pOutPort), ipOutputBuffer = NULL;

                goto exit;
            }
            else
            {
                /***************************************************************
                 * Prepare to use the output buffer
                 **************************************************************/

                ipOutputBuffer->nFilledLen = 0, ipOutputBuffer->nOffset = 0;
            }
        }
    
        TRACE (DATA, _b("Use output buffer: %p"), ipOutputBuffer);
        
        /***********************************************************************
         * Pass mark command down the stack as needed
         **********************************************************************/

        if (ipMark != NULL)
        {
            TRACE (INFO, _b("Marking data is set: (%p,%p)"), ipMark->hMarkTargetComponent, ipMark->pMarkData);
            
            /* ...put marking data to output buffer */
            ipOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent, ipOutputBuffer->pMarkData = ipMark->pMarkData;

            /* ...and clear marking flag */
            ipMark = NULL;
        }

        /***********************************************************************
         * Process derived mark data (to-be-understood)
         **********************************************************************/

        if (ipTargetComponent != NULL)
        {
            TRACE (INFO, _b("Marking data is derived: (%p,%p)"), ipMark->hMarkTargetComponent, ipMark->pMarkData);

            /* ...pass marking data down the stack */
            ipOutputBuffer->hMarkTargetComponent = ipTargetComponent, ipOutputBuffer->pMarkData = iTargetMarkData;

            /* ...and clear derived marking data request */
            ipTargetComponent = NULL;
        }

        /***********************************************************************
         * Retrieve the data from decoder if possible
         **********************************************************************/

        /* ...do not check actual result code */
        DSPGet (ipOutputBuffer);
        
        /* ...set the timestamp equal to the latest input buffer timestamp (is it valid?) */
        ipOutputBuffer->nTimeStamp = iFrameTimestamp;
        
        /***********************************************************************
         * If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         **********************************************************************/

        if (OMX_TRUE == iEndofStream)
        {
            /* ...place end-of-stream flag onto output buffer */
            ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;

            if (!iDecodeReturn)
            {
                TRACE (INFO, _b("Issue EOS notification"));
                
                /* ...pass EOS-notification to the framework */
                (*(ipCallbacks->EventHandler)) (pHandle, iCallbackData, OMX_EventBufferFlag, 1, OMX_BUFFERFLAG_EOS, NULL);

                /* ...reset end-of-stream flag and request new input buffer */
                iEndofStream = OMX_FALSE;

                /* ...put output buffer back to the pool */
                ReturnOutputBuffer (ipOutputBuffer, pOutPort), ipOutputBuffer = NULL;

                return;
            }
            else
            {
                TRACE (PROCESS, _b("Stream processed"));
            
                /* ...pass EOS-notification to the framework */
                (*(ipCallbacks->EventHandler)) (pHandle, iCallbackData, OMX_EventBufferFlag, 1, OMX_BUFFERFLAG_EOS, NULL);
    
                /* ...and pass it down the graph */
                ReturnOutputBuffer (ipOutputBuffer, pOutPort), ipOutputBuffer = NULL;

                /* ...and reschedule AO execution */
                TRACE (PROCESS, _b("Decoding completed; reschedule AO")), RunIfNotReady();
                
                return;
            }
        }

        /***********************************************************************
         * Pass output buffer down the stack if it contains valid data
         **********************************************************************/

        if (ipOutputBuffer->nFilledLen > 0)
        {
            TRACE (DATA, _b("Pass output buffer: %p - %u bytes, ts: %u"), ipOutputBuffer->pBuffer, (unsigned)ipOutputBuffer->nFilledLen, (unsigned)ipOutputBuffer->nTimeStamp);
            
            ReturnOutputBuffer (ipOutputBuffer, pOutPort), ipOutputBuffer = NULL;
        }
    }

exit:

    /***************************************************************************
     * Processing completed
     **************************************************************************/

    if ((GetQueueNumElem(pInputQueue) > 0 && iEndOfFrameFlag == OMX_FALSE) || (iEndOfFrameFlag == OMX_TRUE && GetQueueNumElem(pOutputQueue) > 0))
    {
        TRACE (PROCESS, _b("Reschedule AO execution")), RunIfNotReady();
    }

    TRACE (DEBUG, _b("<< Buffer management: IBE:%d EOF:%d EOS:%d"), (iIsInputBufferEnded == OMX_TRUE ? 1 : 0), (iEndOfFrameFlag == OMX_TRUE ? 1 : 0), (iEndofStream == OMX_TRUE ? 1 : 0));

    return;

error:

    /***************************************************************************
     * Processing completed with error (to-be-defined)
     **************************************************************************/
    
    BUG (1, _b("Buffer management function completed with error"));
}

/*******************************************************************************
 * OmxDspVideoDecoder::ProcessData
 *
 * Data processing stub (it is pure virtual function)
 ******************************************************************************/

void OmxDspVideoDecoder::ProcessData (void)
{
    /* ...dead code */
}

/*******************************************************************************
 * OmxDspVideoDecoder::CalculateBufferParameters
 *
 * Calculate output buffer parameters
 ******************************************************************************/

#if 0 /* OpenCORE 2.0.7 */
void OmxDspVideoDecoder::CalculateBufferParameters (OMX_U32 PortIndex)
{
    OMX_VIDEO_PORTDEFINITIONTYPE   *videoformat = &(ipPorts[PortIndex]->PortParam.format.video);

    TRACE (DATA, _b("Calculate buffer parameters: port #%u, width = %u, height = %u"), (unsigned)PortIndex, (unsigned)videoformat->nFrameWidth, (unsigned)videoformat->nFrameHeight);

    if (PortIndex != OMX_PORT_OUTPUTPORT_INDEX)
    {
        OmxComponentVideo::CalculateBufferParameters (PortIndex);
    }
    else
    {
        /* ...minimal stride is always equal to original PAL parameters */
        videoformat->nStride = (videoformat->nFrameWidth + 0xF) & ~0xF, videoformat->nSliceHeight = (videoformat->nFrameHeight + 0xF) & ~0xF;

        /* ...and buffer size is suitable for YUV422 frame */
        ipPorts[PortIndex]->PortParam.nBufferSize = (videoformat->nSliceHeight * videoformat->nStride * 2);
    }
}
#endif

/*******************************************************************************
 * OmxDspVideoDecoder::BaseComponentAllocateBuffer
 *
 * Static function used by framework to allocate output buffer
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::BaseComponentAllocateBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** pBuffer, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes)
{
    OmxDspVideoDecoder     *d = (OmxDspVideoDecoder*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    ComponentPortType      *p = d->ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_BUFFERHEADERTYPE   *b;
    OMX_U32                 i;
    
    /***************************************************************************
     * Parameters validation
     **************************************************************************/

    /* ...check the pointer is non-null */
    if (d == NULL)      return TRACE (ERROR, _x("Null-pointer passed")), OMX_ErrorBadParameter;

    /* ...check the port is output */
    if (nPortIndex != OMX_PORT_OUTPUTPORT_INDEX)    return TRACE (ERROR, _b("Invalid port number: %d"), (int)nPortIndex), OMX_ErrorBadPortIndex;
    
    /* ...check port state */
    if (p->TransientState != OMX_StateIdle)     return TRACE (ERROR, _b("Incorrect port state: %d"), (int)p->TransientState), OMX_ErrorIncorrectStateTransition;

    /* ...check requested buffer size */
    if (nSizeBytes < p->PortParam.nBufferSize)  return TRACE (ERROR, _b("Requested buffer size is less then minimum size: %d < %d"), (int)nSizeBytes, (int)p->PortParam.nBufferSize), OMX_ErrorBadParameter;

    /* ...check out if amount of allocated buffer is too high */
    if ((i = p->NumAssignedBuffers) >= p->PortParam.nBufferCountActual)   return TRACE (ERROR, _b("Attempt to allocate too much buffers")), OMX_ErrorInsufficientResources;
    
    /***************************************************************************
     * Check out if this is first allocation
     **************************************************************************/
    
    if (p->pBuffer == NULL)
    {
        /* ...create headers pool */
        if ((p->pBuffer = (OMX_BUFFERHEADERTYPE**) oscl_calloc(p->PortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*))) == NULL)
        {
            /* ...failed to allocate memory */
            return TRACE (ERROR, _x("Failed to allocate buffer pool")), OMX_ErrorInsufficientResources;
        }

        /* ...allocate buffer states pool */
        if ((p->BufferState = (OMX_U32*) oscl_calloc(p->PortParam.nBufferCountActual, sizeof(OMX_U32))) == NULL)
        {
            return TRACE (ERROR, _x("Failed to allocate buffer pool")), OMX_ErrorInsufficientResources;
        }
    }

    /***************************************************************************
     * Allocate buffer header
     **************************************************************************/

    if ((b = (OMX_BUFFERHEADERTYPE*) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE))) == NULL)
    {
        return TRACE (ERROR, _x("Failed to allocate buffer header")), OMX_ErrorInsufficientResources;
    }
    else
    {
        /* ...initialize buffer header */
        d->SetHeader (b, sizeof(OMX_BUFFERHEADERTYPE));
            
        /* ...memory for buffer is NOT allocated but will be taken from DMAI */
        b->nAllocLen = nSizeBytes, b->nFlags = 0, b->pPlatformPrivate = NULL, b->pAppPrivate = pAppPrivate, b->pBuffer = NULL;
        
        /* ...assign proper port indices */
        b->nInputPortIndex = d->iNumPorts, b->nOutputPortIndex = nPortIndex;
    }
    
    /***************************************************************************
     * Process port direction (better comment is welcomed)
     **************************************************************************/

#if 0 /* OpenCORE 2.0.7 */
    if ((b->pOutputPortPrivate = oscl_malloc(sizeof(BufferCtrlStruct))) == NULL)
    {
        /* ...destroy buffer itself */
        oscl_free (b);
            
        /* ...and return failure */
        return TRACE (ERROR, _b("Failed to allocate buffer control structure")), OMX_ErrorInsufficientResources;
    }
    else
    {
        BufferCtrlStruct   *pBCTRL = (BufferCtrlStruct*) b->pOutputPortPrivate;

        /* ...initialize reference counter to 1 (since buffers are initially with the IL client) */
        pBCTRL->iRefCount = 1, pBCTRL->iIsBufferInComponentQueue = OMX_FALSE;
    }
#endif
    
    /***************************************************************************
     * Mark the buffer is allocated
     **************************************************************************/

    /* ...the header is allocated and the buffer is assigned */
    p->BufferState[i] = BUFFER_ASSIGNED | HEADER_ALLOCATED, *pBuffer = (p->pBuffer[i] = b);
     
    /***************************************************************************
     * Populate port as appropriate
     **************************************************************************/

    if ((p->NumAssignedBuffers = i + 1) == p->PortParam.nBufferCountActual)
    {
        /* ...set "populated" flag */
        p->PortParam.bPopulated = OMX_TRUE;

        /* ...reschedule the AO for a state change (Loaded->Idle) if its pending on buffer allocation */
        (OMX_TRUE == d->iStateTransitionFlag ? d->RunIfNotReady(), d->iStateTransitionFlag = OMX_FALSE : 0);
    }

    /***************************************************************************
     * Return success result code
     **************************************************************************/
    
    return TRACE (INIT, _b("Output buffer #%d allocated: %p"), (int)i, *pBuffer), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspVideoDecoder::BaseComponentFillThisBuffer
 *
 * Add output buffer to the output queue
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::BaseComponentFillThisBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer)
{
    OmxDspVideoDecoder     *d = (OmxDspVideoDecoder*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    QueueType              *queue;
    Int8                   *p;
    OMX_U32                 idx;
    OMX_ERRORTYPE           result;
    
    /***************************************************************************
     * Parameters validation
     **************************************************************************/

    /* ...check the pointer is non-null */
    if (d == NULL)      return TRACE (ERROR, _x("Null-pointer passed")), OMX_ErrorBadParameter;

    /* ...validate port index (should be queued to output port only) */
    if ((idx = pBuffer->nOutputPortIndex) >= d->iNumPorts || d->ipPorts[idx]->PortParam.eDir != OMX_DirOutput)  return TRACE (ERROR, _x("Bad port index: %u"), (unsigned)idx), OMX_ErrorBadPortIndex;

    /* ...check port state */
    if (d->iState != OMX_StateExecuting && d->iState != OMX_StatePause && d->iState != OMX_StateIdle) return TRACE (ERROR, _x("Invalid state: %d"), (int)d->iState), OMX_ErrorIncorrectStateOperation;

    /* ...port should be in enabled state before accepting buffers */
    if (!PORT_IS_ENABLED(d->ipPorts[idx]))  return TRACE (ERROR, _x("Port is disabled")), OMX_ErrorIncorrectStateOperation;

    /* ...check the header is valid */
    if ((result = d->CheckHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone) return TRACE (ERROR, _x("Wrong header: %d"), (int)result), result;

    /***************************************************************************
     * Release buffer back to the DSP pool as required
     **************************************************************************/

    d->DSPRelease (pBuffer);
    
    /***************************************************************************
     * Add buffer to the output queue
     **************************************************************************/

    if (Queue (d->ipPorts[idx]->pBufferQueue, pBuffer) != OMX_ErrorNone)
    {
        return TRACE (ERROR, _x("Failed to queue buffer %p"), pBuffer), OMX_ErrorInsufficientResources;
    }
#if 0 /* OpenCORE 2.0.7 */
    else
    {
        BufferCtrlStruct *pBCTRL = (BufferCtrlStruct *)(pBuffer->pOutputPortPrivate);

        /* ...mark the buffer is enqueued */
        pBCTRL->iIsBufferInComponentQueue = OMX_TRUE;

        /* ...increase amount of available buffers (owned exclusively by AO) */
        (--pBCTRL->iRefCount == 0 ? d->iNumAvailableOutputBuffers++ : 0);
    }
#endif

    /***************************************************************************
     * Increment amount of available buffers and reschedule AO as appropriate
     **************************************************************************/

    (d->iOutBufferCount++ == 0 ? TRACE (DATA, _b("Reschedule AO")), d->RunIfNotReady(), 1 : 0);

    /***************************************************************************
     * Return final result code
     **************************************************************************/

    return TRACE (DATA, _b("Output buffer %p available (%d)"), pBuffer, (int)d->iOutBufferCount), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspVideoDecoder::BaseComponentFreeBuffer
 *
 * Static function used by framework to free output buffer
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::BaseComponentFreeBuffer (OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer)
{
    OmxDspVideoDecoder     *d = (OmxDspVideoDecoder*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    ComponentPortType      *p = d->ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    int                     i;

    /***************************************************************************
     * Parameters validation
     **************************************************************************/

    /* ...check the pointer is non-null */
    if (d == NULL)      return TRACE (ERROR, _x("Null-pointer passed")), OMX_ErrorBadParameter;

    /* ...check the port is output (redirect to base function if not) */
    if (nPortIndex != OMX_PORT_OUTPUTPORT_INDEX)    return OmxComponentBase::BaseComponentFreeBuffer (hComponent, nPortIndex, pBuffer);

    /* ...check out if we do have buffer to free (still return success) */
    if ((i = p->NumAssignedBuffers - 1) < 0)    return TRACE (ERROR, _b("No buffers to free")), OMX_ErrorNone;
    
    /* ...mek sure buffer state is valid */
    if ((p->BufferState[i] & (BUFFER_ASSIGNED | HEADER_ALLOCATED)) != (BUFFER_ASSIGNED | HEADER_ALLOCATED)) return TRACE (ERROR, _x("Invalid buffer state: %lX"), p->BufferState[i]), OMX_ErrorBadParameter;

    /***************************************************************************
     * Check the port state 
     ***************************************************************************/

    if (p->TransientState != OMX_StateLoaded && p->TransientState != OMX_StateInvalid)
    {
        TRACE (DATA, _b("Issue unpopulate event for output port"));
        
        /* ...issue proper notification to the framework (OMX_CommandStateSet completes with error) */
        (*(d->ipCallbacks->EventHandler)) (hComponent, d->iCallbackData, OMX_EventError, OMX_ErrorPortUnpopulated, nPortIndex, NULL);
    }

    /***************************************************************************
     * Release the buffer
     **************************************************************************/

#if 0 /* OpenCORE 2.0.7 */
    /* ...deallocate control structure (it is an output port) */
    oscl_free (pBuffer->pOutputPortPrivate), pBuffer->pOutputPortPrivate = NULL;
#endif

    /* ...deallocate header itself */
    oscl_free (pBuffer);

    /* ...mark the buffer is free */
    p->BufferState[i] = BUFFER_FREE;

    /***************************************************************************
     * Check out if this was the last allocated buffer
     **************************************************************************/

    if ((p->NumAssignedBuffers = (OMX_U32)i) == 0)
    {
        /* ...no more buffers; unpopulate the port */
        p->PortParam.bPopulated = OMX_FALSE;

        /* ...reschedule the AO for a state change (Idle->Loaded) if its pending on buffer de-allocation */
        (OMX_TRUE == d->iStateTransitionFlag ? d->RunIfNotReady(), d->iStateTransitionFlag = OMX_FALSE, d->iNewOutBufRequired = OMX_TRUE : 0);

        /* ...destroy buffer pool */
        oscl_free (p->pBuffer), p->pBuffer = NULL;
        
        /* ...and states pool */
        oscl_free (p->BufferState), p->BufferState = NULL;
    }

    /***************************************************************************
     * Return final success result
     **************************************************************************/

    return TRACE (INIT, _b("Buffer %p freed"), pBuffer), OMX_ErrorNone;
}


/*******************************************************************************
 * OmxDspVideoDecoder::ComponentInit
 *
 * Component initialization function (called by framework)
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::ComponentInit (void)
{
    OMX_ERRORTYPE   Status;

    /***************************************************************************
     * Check out sequence validness
     **************************************************************************/

    if (OMX_TRUE == iIsInit)    return TRACE (ERROR, _b("Component is already initialized")), OMX_ErrorIncorrectStateOperation;
    
    /***************************************************************************
     * Codec initialization
     **************************************************************************/

    if (!iCodecReady)
    {
        /* ...initialize DSP codec */
        if ((Status = DSPDecoderInit()) != OMX_ErrorNone)   return Status;
        
        /* ...mark the codec is ready */
        iCodecReady = OMX_TRUE;
    }

    /***************************************************************************
     * Reset local variables
     **************************************************************************/

    /* ...current length of input buffer */
    iInputCurrLength = 0;

    /* ...reset frame count (used in dynamic port reconfiguration) */
    iFrameCount = 0;

    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return iIsInit = OMX_TRUE, TRACE (INIT, _b("Component initialization completed")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspVideoDecoder::ComponentDeInit
 *
 * This function is called upon a transition to the idle or invalid state 
 * (and by component destructor)
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::ComponentDeInit()
{
    /***************************************************************************
     * Perform component deinitialization
     **************************************************************************/

    /* ...deinitialize codec if required */
    (iCodecReady ? DSPDecoderClean(), iCodecReady = OMX_FALSE : 0);
    
    /***************************************************************************
     * Deinitialization completed; report result code to caller
     **************************************************************************/

    return iIsInit = OMX_FALSE, TRACE (INIT, _b("Component deinitialization completed")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspVideoDecoder::GetConfig
 *
 * Get current configuration
 ******************************************************************************/

OMX_ERRORTYPE OmxDspVideoDecoder::GetConfig (OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_INDEXTYPE nIndex, OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    OSCL_UNUSED_ARG(hComponent);
    OSCL_UNUSED_ARG(nIndex);
    OSCL_UNUSED_ARG(pComponentConfigStructure);

    return TRACE (DEBUG, _b("GetConfig - not implemented function")), OMX_ErrorNotImplemented;
}

/*******************************************************************************
 * OmxDspVideoDecoder::ResetComponent
 *
 * This routine will reset the decoder library and some of the associated flags
 ******************************************************************************/

void OmxDspVideoDecoder::ResetComponent (void)
{
    TRACE (INFO, _b("Component reset - not implemented"));
}

/*******************************************************************************
 * OmxDspVideoDecoder::OmxDspVideoDecoder
 *
 * Component object constructor
 ******************************************************************************/

OmxDspVideoDecoder::OmxDspVideoDecoder()
{
    TRACE (DEBUG, _b("Component[%p] constructed"), this);
    
    /* ...add component to scheduler */
    (!IsAdded() ? AddToScheduler(), 1 : 0);
}

/*******************************************************************************
 * OmxDspVideoDecoder::~OmxDspVideoDecoder
 *
 * Component object destructor
 ******************************************************************************/

OmxDspVideoDecoder::~OmxDspVideoDecoder()
{
    /* ...remove from scheduler as required */
    (IsAdded() ? RemoveFromScheduler(), 1 : 0);

    TRACE (DEBUG, _b("Component[%p] destructed"), this);
}

/*******************************************************************************
 * DSP engine support
 ******************************************************************************/

/* ...buffer used by codec */
#define OMX_DSP_CODEC_MASK              (1 << 0)

/* ...buffer used by display */
#define OMX_DSP_DISPLAY_MASK            (1 << 1)

/*******************************************************************************
 * OmxDspVideoDecoder::DSPDecoderInit
 *
 * Initialize DSP decoder 
 ******************************************************************************/

extern "C" {
extern Engine_Handle global_engine_handle;
}

OMX_ERRORTYPE OmxDspVideoDecoder::DSPDecoderInit (void)
{
    OMX_VIDEO_PORTDEFINITIONTYPE   *videoformat = &ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video;
    VIDDEC2_Params                  params = Vdec2_Params_DEFAULT;
    VIDDEC2_DynamicParams           dynParams = Vdec2_DynamicParams_DEFAULT;
    BufferGfx_Attrs                 gfxAttrs = BufferGfx_Attrs_DEFAULT;
    Buffer_Attrs                    bAttrs = Buffer_Attrs_DEFAULT;
    Engine_Error                    ec;
    
    /***************************************************************************
     * Open DSP engine
     **************************************************************************/
    hEngine = global_engine_handle;

    /***************************************************************************
     * Open video decoder interface
     **************************************************************************/

    /* ...force color space to YUV 422 interlaced little-endian */
    params.forceChromaFormat = XDM_YUV_422ILE;
    //params.forceChromaFormat = XDM_YUV_420P;

    /* ...specify actual frame size */
    //params.maxWidth = VideoStd_D1_WIDTH, params.maxHeight = VideoStd_D1_PAL_HEIGHT;
    params.maxWidth = videoformat->nStride, params.maxHeight = videoformat->nSliceHeight;

    /* ...open video decoder interface */
    if ((hVd = Vdec2_create (hEngine, iDecoderName, &params, &dynParams)) == NULL)
    {
        TRACE (ERROR, _b("Failed to open decoder '%s'"), (char*)iDecoderName);

        goto error;
    }

    /***************************************************************************
     * Create input buffer
     **************************************************************************/

    if ((hDSPBuffer = Buffer_create ((iDSPBufferSize = Vdec2_getInBufSize(hVd)), &bAttrs)) == NULL)
    {
        TRACE (ERROR, _b("Failed to create input buffer"));
        
        goto error;
    }
    else
    {
        /* ...set buffer pointer and writing index */
        pDSPBuffer = (OMX_U8*) Buffer_getUserPtr (hDSPBuffer), iDSPWriteIndex = 0;

        TRACE (INIT, _b("Allocated input buffer: %u bytes"), (unsigned) iDSPBufferSize);
    }    
        
    /***************************************************************************
     * Create output buffers table (to-be-removed)
     **************************************************************************/

    /* ...set color space */
    gfxAttrs.colorSpace = ColorSpace_UYVY;
    //gfxAttrs.colorSpace = ColorSpace_YUV420P;

    /* ...and frame dimensions */
    gfxAttrs.dim.width = params.maxWidth, gfxAttrs.dim.height = params.maxHeight;

    /* ...calculate line length from parameters */
    gfxAttrs.dim.lineLength = BufferGfx_calcLineLength (gfxAttrs.dim.width, gfxAttrs.colorSpace);

    /* ...mark the buffer is used jointly by codec and display */
    gfxAttrs.bAttrs.useMask = OMX_DSP_CODEC_MASK | OMX_DSP_DISPLAY_MASK;
    
    /* ...create buffers table (5 buffers) */
    if ((hOutBufTab = BufTab_create (5, Dmai_roundUp(Vdec2_getOutBufSize(hVd), 128), BufferGfx_getBufferAttrs(&gfxAttrs))) == NULL)
    {
        TRACE (ERROR, _b("Failed to create output buffers table"));

        goto error;
    }
    else
    {
        /* ...and bind it to decoder display buffers */
        Vdec2_setBufTab (hVd, hOutBufTab);

        /* ...mark we do not have output buffer nor display buffer yet */
        hOutBuf = hDispBuffer = NULL;
    }

    /***************************************************************************
     * Codec initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("DSP VIDDEC2 codec initialized")), OMX_ErrorNone;

error:

    /***************************************************************************
     * Initialization failed; perform cleanup
     **************************************************************************/

    DSPDecoderClean ();
    
    return TRACE (INIT, _b("Failed to initialize DSP VIDDEC2 codec")), OMX_ErrorInsufficientResources;
}

/*******************************************************************************
 * OmxDspVideoDecoder::DSPDecoderClean
 *
 * Cleanup DSP decoder 
 ******************************************************************************/

void OmxDspVideoDecoder::DSPDecoderClean (void)
{
    /***************************************************************************
     * Release all allocated resources
     **************************************************************************/

    /* ...mark all buffers as free, destroy any resizing data and destroy codec engine */
    (hVd ? BufTab_freeAll (hOutBufTab), BufTab_collapse (hOutBufTab), Vdec2_delete(hVd), hVd = NULL : 0);

    /* ...delete output buffer table if needed (TBD) */
    (hOutBufTab ? BufTab_delete(hOutBufTab), hOutBufTab = NULL : 0);

    /* ...delete input buffer */
    (hDSPBuffer ? Buffer_delete (hDSPBuffer), hDSPBuffer = NULL : 0);

    /***************************************************************************
     * Close all capturing stuff (impossible to leave open file handles)
     **************************************************************************/

    PC_CLOSE(PROCESS), PC_CLOSE(HOLD);

    /***************************************************************************
     * Cleanup sequence completed
     **************************************************************************/

    TRACE (INIT, _b("DSP VIDDEC2 decoder cleanup completed"));
}

/*******************************************************************************
 * OmxDspVideoDecoder::DSPPut
 *
 * Copy data from input buffer to DSP buffer
 ******************************************************************************/

OMX_BOOL OmxDspVideoDecoder::DSPPut (OMX_BUFFERHEADERTYPE* b)
{
    OMX_U32     n = b->nFilledLen;
    OMX_U32     e;
    
    TRACE (DATA, _b("copy %u bytes: [%lu, %lu)"), (unsigned)n, iDSPWriteIndex, iDSPWriteIndex + n);
    
    /* ...check out there is enough room to store data (TBD) */
    if ((e = iDSPWriteIndex + n) > iDSPBufferSize) return TRACE (DATA, _b("Input buffer to large")), OMX_FALSE;
    
    /* ...copy input buffer content into DSP-owned buffer */
    oscl_memcpy (pDSPBuffer + iDSPWriteIndex, b->pBuffer + b->nOffset, n);
    
    /* ...set input buffer length and reset writing pointer (keep NAL start code) for a final chunk */
    (iEndOfFrameFlag == OMX_TRUE ? TRACE (DATA, _b("Received frame: %lu bytes"), e), Buffer_setNumBytesUsed (hDSPBuffer, e), e = 0 : 0);

    /* ...advance writing index and return success result */
    return iDSPWriteIndex = e, OMX_TRUE;
}

/*******************************************************************************
 * OmxDspVideoDecoder::DSPGet
 *
 * Retrieve decoded data from DSP engine
 ******************************************************************************/

OMX_BOOL OmxDspVideoDecoder::DSPGet (OMX_BUFFERHEADERTYPE *b)
{
    /***************************************************************************
     * Check out if there is available buffer to display
     **************************************************************************/

    if (hDispBuffer != NULL)
    {
        TRACE (DEBUG, _b("Use display buffer: %p"), hDispBuffer), Buffer_print (hDispBuffer);
            
        /* ...set private platform data */
        b->pPlatformPrivate = (OMX_PTR*) hDispBuffer;
            
        /* ...set pointer to buffer memory and actual length */
        b->pBuffer = (OMX_U8*) Buffer_getUserPtr(hDispBuffer), b->nFilledLen = Buffer_getNumBytesUsed(hDispBuffer);
            
        /* ...increment frame count and resize buffer table if this is the very first frame */
        (iFrameCount++ == 0 ? /*DSPResize (hOutBuf)*/ 1 /*, *aResizeFlag = OMX_TRUE */: 0);

        /* ...capture start time of buffer holding time */
        (OMX_DSP_PC && hDispBuffer == BufTab_getBuf(hOutBufTab, 0) ?  PC_START(HOLD), 1 : 0);
        
        /* ...try to get next display buffer */
        hDispBuffer = Vdec2_getDisplayBuf(hVd);
        
        return OMX_TRUE;
    }
    else
    {
        return TRACE (DEBUG, _b("Nothing to display")), OMX_FALSE;
    }
}

/*******************************************************************************
 * OmxDspVideoDecoder::DSPResize
 *
 * Resize buffer table with respect to actual resolution of video stream
 ******************************************************************************/

int OmxDspVideoDecoder::DSPResize (Buffer_Handle hBuf)
{
    int     numCodecBuffers, numExpBufs;
    Int32   frameSize;
    
    /***************************************************************************
     * Check out how mane buffers the codec can keep at one time
     **************************************************************************/

    if ((numCodecBuffers = Vdec2_getMinOutBufs(hVd)) < 0)
    {
        return TRACE (ERROR, _b("Failed to get buffer requirements: %d"), numCodecBuffers), -1;
    }
    else
    {
        /* ...get required frame size and add two buffers for display pipeline */
        frameSize = Vdec2_getOutBufSize(hVd), numCodecBuffers += 3;
        
        TRACE (INFO, _b("Required buffer parameters: %d * %ld"), numCodecBuffers, frameSize);
    }
    
    /***************************************************************************
     * Check out if buffer table needs to be changed
     **************************************************************************/

    if (frameSize < Buffer_getSize(hBuf) || numCodecBuffers > BufTab_getNumBufs(hOutBufTab))
    {
        if (frameSize < Buffer_getSize(hBuf))
        {
            /*******************************************************************
             * Break the current buffers into smaller ones
             ******************************************************************/

            if ((numExpBufs = BufTab_chunk (hOutBufTab, numCodecBuffers, frameSize)) < 0)
            {
                return TRACE (ERROR, _b("Failed to chunk %d buffer (%ld) to %d buffers (%ld)"), BufTab_getNumBufs(hOutBufTab), Buffer_getSize(hBuf), numCodecBuffers, frameSize), -1;
            }
            else if (BufTab_expand(hOutBufTab, numExpBufs) < 0)
            {
                return TRACE (ERROR, _b("Failed to expand BufTab with %d buffers"), numExpBufs), -1;
            }
            else
            {
                TRACE (INFO, _b("Buffer table converted successfully: %d buffers (%ld)"), BufTab_getNumBufs(hOutBufTab), frameSize);
            }
        }
        else
        {
            /*******************************************************************
             * Just expand the buffer table with more buffers
             ******************************************************************/

            if (BufTab_expand(hOutBufTab, numCodecBuffers) < 0)
            {
                return TRACE (ERROR, _b("Failed to expand BufTab with %d buffers"), numCodecBuffers), -1;
            }
            else
            {
                TRACE (INFO, _b("Buffer table converted successfully: %d buffers (%ld)"), BufTab_getNumBufs(hOutBufTab), Buffer_getSize(hBuf));
            }
        }
    }

    /***************************************************************************
     * Return new amount of buffers
     **************************************************************************/

    return numCodecBuffers;
}

/*******************************************************************************
 * OmxDspVideoDecoder::DSPProcess
 *
 * Video decoding process
 ******************************************************************************/

OMX_BOOL OmxDspVideoDecoder::DSPProcess (OMX_PARAM_PORTDEFINITIONTYPE* aPortParam, OMX_BOOL *aResizeFlag)
{
    Buffer_Handle   hBuf;
    Int             result;

    /***************************************************************************
     * Make sure the input buffer contains data
     **************************************************************************/

    if (Buffer_getNumBytesUsed(hDSPBuffer) == 0)    goto reset;
    
    /***************************************************************************
     * Get output buffer from pool if needed
     **************************************************************************/

    if (hOutBuf == NULL)
    {
        if ((hOutBuf = BufTab_getFreeBuf(hOutBufTab)) == NULL)
        {
            /* ...keep input path disabled */
            return TRACE (PROCESS, _b("No free DSP buffers")), OMX_TRUE;
        }
        else
        {
            /* ...set frame dimensions for new buffer - use entire buffer */
            BufferGfx_resetDimensions (hOutBuf);
        }
    }
    
    TRACE (DEBUG, _b("Use DSP buffer: %p"), hOutBuf);

    /***************************************************************************
     * Perform actual decoding
     **************************************************************************/

    if ((result = Vdec2_process (hVd, hDSPBuffer, hOutBuf)) < 0)
    {
        /***********************************************************************
         * Decoding error
         **********************************************************************/

        return TRACE (PROCESS, _b("Fatal processing error: %d"), result), OMX_FALSE;
    }
    else if (result != Dmai_EFIRSTFIELD)
    {
        /***********************************************************************
         * Mark the output buffer needs to be taken from pool next time
         **********************************************************************/

        /* ...take display buffer if possible */
        if (result == 0)
        {
            /* ...video frame decoded */
            TRACE (PROCESS, _b("Video frame decoded"));
            
            /* ...update display buffer if needed */
            (hDispBuffer == NULL ? hDispBuffer = Vdec2_getDisplayBuf (hVd) : 0);

            /* ...resize video if possible */
            (iFrameCount == 0 ? DSPResize (hOutBuf), 1 : 0);
        }

        /* ...request new buffer */
        hOutBuf = NULL;
    }
    else
    {
        /***********************************************************************
         * First field of interlaced frame decoded; keep using the same buffer
         **********************************************************************/

        TRACE (PROCESS, _b("First field decoded"));
    }
 
reset:
   
    /***************************************************************************
     * Free all buffers not used by codec
     **************************************************************************/

    while ((hBuf = Vdec2_getFreeBuf(hVd)) != NULL)
    {
        TRACE (DEBUG, _b("Free DSP buffer: %p"), hBuf), Buffer_freeUseMask (hBuf, OMX_DSP_CODEC_MASK);
    }
    
    /***************************************************************************
     * Enable input path and return success decoding status
     **************************************************************************/

    return iEndOfFrameFlag = OMX_FALSE, OMX_TRUE;
}

/*******************************************************************************
 * OmxDspVideoDecoder::DSPRelease
 *
 * Release output buffer back to the pool
 ******************************************************************************/

void OmxDspVideoDecoder::DSPRelease (OMX_BUFFERHEADERTYPE *b)
{
    Buffer_Handle   hBuf;
    
    /* ...check out if platform private data is set */
    if ((hBuf = (Buffer_Handle)b->pPlatformPrivate) != NULL)
    {
        /* ...mark the buffer is not owned by renderer anymore */
        Buffer_freeUseMask (hBuf, OMX_DSP_DISPLAY_MASK);

        /* ...capture stop time of buffer holding time */
        (OMX_DSP_PC && hBuf == BufTab_getBuf(hOutBufTab, 0) ? PC_STOP(HOLD), 1 : 0);

        TRACE (DEBUG, _b("Release DSP buffer: %p"), hBuf);
    }
}

/*******************************************************************************
 * OpenmaxH264hwAO::DSPFlush
 *
 * Flush video decoder 
 ******************************************************************************/

void OmxDspVideoDecoder::DSPFlush (void)
{
    /* ...flush DSP codec (well, that's not quite correct, so it is a dead code) */
    Vdec2_flush (hVd);
    
    TRACE (PROCESS, _b("Video decoder flushed"));
}
