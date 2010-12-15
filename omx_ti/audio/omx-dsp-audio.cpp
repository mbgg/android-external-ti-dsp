/*******************************************************************************
 * omx-dsp-audio.cpp
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

#define MODULE_TAG                      AUDIO

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "omx-dsp.h"
#include "omx-dsp-audio.h"
#include "omx-dsp-aac.h"
#include "omx-dsp-mp3.h"

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
        
        if (PV_OMX_AACDEC_UUID == aOmxTypeId)
        {
            if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
            {
                /* ...query for component factory */
                return ((OsclAny*)(&AacOmxDspComponentFactory));
            }
            else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
            {
                /* ...query for component destructor */
                return ((OsclAny*)(&AacOmxDspComponentDestructor));
            }
        } else
        if (PV_OMX_MP3DEC_UUID == aOmxTypeId)
        {
            if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
            {
                /* ...query for component factory */
                return ((OsclAny*)(&Mp3OmxDspComponentFactory));
            }
            else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
            {
                /* ...query for component destructor */
                return ((OsclAny*)(&Mp3OmxDspComponentDestructor));
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
 * Base audio decoder component interface implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspAudioDecoder::ConstructComponent
 *
 * Base implementation of component constructor
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName)
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

#if 0 /* OpenCORE 2.0.7 */
    iPVCapabilityFlags.iOMXComponentUsesInterleaved2BNALSizes = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentUsesInterleaved4BNALSizes = OMX_FALSE;
#endif

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
     * Generic audio decoder ports initialization
     **************************************************************************/

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /* ...totally we have two ports */
    iPortTypesParam.nPorts = 2, iPortTypesParam.nStartPortNumber = 0;

    /***************************************************************************
     * Input port (partial) configuration - to-be-adjusted in derived class
     **************************************************************************/

    pInPort->PortParam.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->PortParam.eDomain = OMX_PortDomainAudio;
    pInPort->PortParam.format.audio.cMIMEType = (OMX_STRING) "audio/mpeg";
    pInPort->PortParam.format.audio.pNativeRender = 0;
    pInPort->PortParam.format.audio.bFlagErrorConcealment = OMX_TRUE;
    pInPort->PortParam.eDir = OMX_DirInput;
    pInPort->PortParam.nBufferCountMin = 1;
    pInPort->PortParam.bEnabled = OMX_TRUE;
    pInPort->PortParam.bPopulated = OMX_FALSE;

    /***************************************************************************
     * Output port configuartion
     **************************************************************************/

    pOutPort->PortParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->PortParam.eDomain = OMX_PortDomainAudio;
    pOutPort->PortParam.format.audio.cMIMEType = (OMX_STRING)"raw";
    pOutPort->PortParam.format.audio.pNativeRender = 0;
    pOutPort->PortParam.format.audio.bFlagErrorConcealment = OMX_TRUE;
    pOutPort->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    pOutPort->PortParam.eDir = OMX_DirOutput;
    pOutPort->PortParam.nBufferCountMin = 1;
    pOutPort->PortParam.bEnabled = OMX_TRUE;
    pOutPort->PortParam.bPopulated = OMX_FALSE;

    /***************************************************************************
     * Output port PCM configuartion
     **************************************************************************/

    pOutPort->AudioPcmMode.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    pOutPort->AudioPcmMode.nChannels = 2;
    pOutPort->AudioPcmMode.eNumData = OMX_NumericalDataSigned;
    pOutPort->AudioPcmMode.bInterleaved = OMX_TRUE;
    pOutPort->AudioPcmMode.nBitPerSample = 16;
    pOutPort->AudioPcmMode.nSamplingRate = 44100;
    pOutPort->AudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
    pOutPort->AudioPcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    pOutPort->AudioPcmMode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    SetHeader(&pInPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));

    pInPort->AudioParam.nPortIndex = 0;
    pInPort->AudioParam.nIndex = 0;

    SetHeader(&pOutPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));

    pOutPort->AudioParam.nPortIndex = 1;
    pOutPort->AudioParam.nIndex = 0;
    pOutPort->AudioParam.eEncoding = OMX_AUDIO_CodingPCM;

    /***************************************************************************
     * Decoder low-level initialization
     **************************************************************************/

    /* ...mark there is no result from decoder yet (tbd) */
    iDecodeReturn = OMX_FALSE;

    /* ...reset DSP handles */
    hEngine = NULL, hAd = NULL, hDSPBuffer = NULL, hOutBufTab = NULL;

    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("Component constructed successfully")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspAudioDecoder::DestroyComponent
 *
 * This function is called by the framework in response to call to FreeHandle
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::DestroyComponent (void)
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
 * OmxDspAudioDecoder::BufferMgmtFunction
 *
 * Buffer management function
 ******************************************************************************/

void OmxDspAudioDecoder::BufferMgmtFunction (void)
{
    OMX_COMPONENTTYPE  *pHandle = &iOmxComponent;
    QueueType          *pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType          *pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    ComponentPortType  *pInPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ComponentPortType  *pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_BOOL            ResizeNeeded = OMX_FALSE;

    TRACE (DEBUG, _b(">> Buffer management: IBE:%d EOF:%d EOS:%d (in:%u out:%u)"), (iIsInputBufferEnded == OMX_TRUE ? 1 : 0), (iEndOfFrameFlag == OMX_TRUE ? 1 : 0), (iEndofStream == OMX_TRUE ? 1 : 0), (unsigned)GetQueueNumElem(pInputQueue), (unsigned)GetQueueNumElem(pOutputQueue));

    /***************************************************************************
     * Checking for resize flag
     **************************************************************************/

    /* ...we are still pending */
    if (iResizePending == OMX_TRUE) goto exit;

    /***************************************************************************
     * Input data processing
     **************************************************************************/

    if (iEndofStream == OMX_FALSE && iEndOfFrameFlag == OMX_FALSE && iSendOutBufferAfterPortReconfigFlag == OMX_FALSE)
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
                (ipInputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME ? TRACE (DATA, _b("End-of-frame flag received")), iEndOfFrameFlag = OMX_TRUE : 0);
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

    if (iEndOfFrameFlag == OMX_TRUE && iSendOutBufferAfterPortReconfigFlag == OMX_FALSE)
    {
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
            DSPResize (hOutBuf);
        }
    }

    /* ...resize completed */
    if (iSendOutBufferAfterPortReconfigFlag == OMX_TRUE)
    {
        TRACE (INFO, _b("Buffer reconfiguration is completed"));

        iSendOutBufferAfterPortReconfigFlag = OMX_FALSE;
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

            if ((ipOutputBuffer = (OMX_BUFFERHEADERTYPE*)DeQueue(pOutputQueue)) == NULL)
            {
                /***************************************************************
                 * Failed to get output buffer from pool
                 **************************************************************/

                TRACE (ERROR, _b("Failed to get the output pool; error"));

                goto error;
            }
            else if (ipOutputBuffer->nAllocLen < (OMX_U32)(8096))
            {
                /***************************************************************
                 * Do not proceed if the output buffer can't fit the raw data 
                 **************************************************************/

                TRACE (DATA, _b("Output buffer cannot accomodate raw frame; skip processing"));

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
        if (ResizeNeeded == OMX_FALSE)
        {
            DSPGet (ipOutputBuffer);
        }

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

        if ((ipOutputBuffer->nFilledLen > 0) || (ResizeNeeded == OMX_TRUE))
        {
            TRACE (DATA, _b("Pass output buffer: %p - %u bytes, ts: %u"), ipOutputBuffer->pBuffer, (unsigned)ipOutputBuffer->nFilledLen, (unsigned)ipOutputBuffer->nTimeStamp);
            
            ReturnOutputBuffer (ipOutputBuffer, pOutPort), ipOutputBuffer = NULL;
        }
    }

exit:

    /***************************************************************************
     * Processing completed
     **************************************************************************/

    if (((GetQueueNumElem(pInputQueue) > 0 && iEndOfFrameFlag == OMX_FALSE) || (iEndOfFrameFlag == OMX_TRUE && GetQueueNumElem(pOutputQueue) > 0)) && (ResizeNeeded == OMX_FALSE))
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
 * OmxDspAudioDecoder::ProcessData
 *
 * Data processing stub (it is pure virtual function)
 ******************************************************************************/

void OmxDspAudioDecoder::ProcessData (void)
{
    /* ...dead code */
}

/*******************************************************************************
 * OmxDspAudioDecoder::CalculateBufferParameters
 *
 * Calculate output buffer parameters
 ******************************************************************************/

void OmxDspAudioDecoder::CalculateBufferParameters (OMX_U32 PortIndex)
{
    /* to-be-understood */
}

/*******************************************************************************
 * OmxDspAudioDecoder::BaseComponentAllocateBuffer
 *
 * Static function used by framework to allocate output buffer
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::BaseComponentAllocateBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** pBuffer, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, OMX_U32 nSizeBytes)
{
    OmxDspAudioDecoder     *d = (OmxDspAudioDecoder*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
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
 * OmxDspAudioDecoder::BaseComponentFillThisBuffer
 *
 * Add output buffer to the output queue
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::BaseComponentFillThisBuffer (OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE* pBuffer)
{
    OmxDspAudioDecoder     *d = (OmxDspAudioDecoder*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    QueueType              *queue;
    Int8                   *p;
    OMX_U32                 idx;
    OMX_ERRORTYPE           result;

    TRACE (INFO, _b(">> Fill this buffer"));
    
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
 * OmxDspAudioDecoder::BaseComponentFreeBuffer
 *
 * Static function used by framework to free output buffer
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::BaseComponentFreeBuffer (OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer)
{
    OmxDspAudioDecoder     *d = (OmxDspAudioDecoder*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
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
 * OmxDspAudioDecoder::ComponentInit
 *
 * Component initialization function (called by framework)
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::ComponentInit (void)
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
 * OmxDspAudioDecoder::ComponentDeInit
 *
 * This function is called upon a transition to the idle or invalid state 
 * (and by component destructor)
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::ComponentDeInit()
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
 * OmxDspAudioDecoder::GetConfig
 *
 * Get current configuration
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAudioDecoder::GetConfig (OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_INDEXTYPE nIndex, OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    OSCL_UNUSED_ARG(hComponent);
    OSCL_UNUSED_ARG(nIndex);
    OSCL_UNUSED_ARG(pComponentConfigStructure);

    return TRACE (DEBUG, _b("GetConfig - not implemented function")), OMX_ErrorNotImplemented;
}

/*******************************************************************************
 * OmxDspAudioDecoder::ResetComponent
 *
 * This routine will reset the decoder library and some of the associated flags
 ******************************************************************************/

void OmxDspAudioDecoder::ResetComponent (void)
{
    TRACE (INFO, _b("Component reset - not implemented"));
}

/*******************************************************************************
 * OmxDspAudioDecoder::OmxDspAudioDecoder
 *
 * Component object constructor
 ******************************************************************************/

OmxDspAudioDecoder::OmxDspAudioDecoder()
{
    TRACE (DEBUG, _b("Component[%p] constructed"), this);
    
    /* ...add component to scheduler */
    (!IsAdded() ? AddToScheduler(), 1 : 0);
}

/*******************************************************************************
 * OmxDspAudioDecoder::~OmxDspAudioDecoder
 *
 * Component object destructor
 ******************************************************************************/

OmxDspAudioDecoder::~OmxDspAudioDecoder()
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
 * OmxDspAudioDecoder::DSPDecoderInit
 *
 * Initialize DSP decoder 
 ******************************************************************************/
extern "C" {
extern Engine_Handle global_engine_handle;
}

OMX_ERRORTYPE OmxDspAudioDecoder::DSPDecoderInit (void)
{
    Buffer_Attrs                    bAttrs = Buffer_Attrs_DEFAULT;
    Engine_Error                    ec;

    AUDDEC1_Params                  params = Adec1_Params_DEFAULT;
    AUDDEC1_DynamicParams           dynParams = Adec1_DynamicParams_DEFAULT;
    
    /***************************************************************************
     * Open DSP engine
     **************************************************************************/

    hEngine = global_engine_handle;

    /***************************************************************************
     * Open audio decoder interface
     **************************************************************************/

    /* ...open audio decoder interface */
    if ((hAd = Adec1_create (hEngine, iDecoderName, &params, &dynParams)) == NULL)
    {
        TRACE (ERROR, _b("Failed to open decoder '%s'"), (char*)iDecoderName);

        goto error;
    }

    /***************************************************************************
     * Create input buffer
     **************************************************************************/

    if ((hDSPBuffer = Buffer_create ((iDSPBufferSize = Adec1_getInBufSize(hAd) * 10), &bAttrs)) == NULL)
    {
        TRACE (ERROR, _b("Failed to create input buffer"));
        
        goto error;
    }
    else
    {
        /* ...set buffer pointer and writing index */
        pDSPBuffer = (OMX_U8*) Buffer_getUserPtr (hDSPBuffer), iDSPWriteIndex = 0, iDSPBufferOffset = 0;

        TRACE (INIT, _b("Allocated input buffer: %u bytes"), (unsigned) iDSPBufferSize);
    }    
        
    /***************************************************************************
     * Create output buffers table (to-be-removed)
     **************************************************************************/

    /* ...create buffers table (16 buffers) */
    if ((hOutBufTab = BufTab_create (24, Dmai_roundUp(Adec1_getOutBufSize(hAd), 128), &bAttrs)) == NULL)
    {
        TRACE (ERROR, _b("Failed to create output buffers table"));

        goto error;
    }
    else
    {
        /* ...and bind it to decoder display buffers
           Vdec2_setBufTab (hVd, hOutBufTab);
         */

        /* ...mark we do not have output buffer yet */
        hOutBuf = hDispBuffer = NULL;
    }

    /***************************************************************************
     * Codec initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("DSP AUDDEC1 codec initialized")), OMX_ErrorNone;

error:

    /***************************************************************************
     * Initialization failed; perform cleanup
     **************************************************************************/

    DSPDecoderClean ();
    
    return TRACE (INIT, _b("Failed to initialize DSP AUDDEC1 codec")), OMX_ErrorInsufficientResources;
}

/*******************************************************************************
 * OmxDspAudioDecoder::DSPDecoderClean
 *
 * Cleanup DSP decoder 
 ******************************************************************************/

void OmxDspAudioDecoder::DSPDecoderClean (void)
{
    /***************************************************************************
     * Release all allocated resources
     **************************************************************************/

    /* ...mark all buffers as free, destroy any resizing data and destroy codec engine */
    (hAd ? BufTab_freeAll (hOutBufTab), BufTab_collapse (hOutBufTab), Adec1_delete(hAd), hAd = NULL : 0);

    /* ...delete output buffer table if needed (TBD) */
    (hOutBufTab ? BufTab_delete(hOutBufTab), hOutBufTab = NULL : 0);

    /* ...delete input buffer */
    if (iDSPBufferOffset > 0)
    {
        hDSPBuffer->userPtr -= iDSPBufferOffset;
        hDSPBuffer->physPtr -= iDSPBufferOffset;
        pDSPBuffer = (OMX_U8*) Buffer_getUserPtr (hDSPBuffer);
    }
    (hDSPBuffer ? Buffer_delete (hDSPBuffer), hDSPBuffer = NULL : 0);

    /***************************************************************************
     * Close all capturing stuff (impossible to leave open file handles)
     **************************************************************************/

    PC_CLOSE(PROCESS), PC_CLOSE(HOLD);

    /***************************************************************************
     * Cleanup sequence completed
     **************************************************************************/

    TRACE (INIT, _b("DSP AUDDEC1 decoder cleanup completed"));
}

/*******************************************************************************
 * OmxDspAudioDecoder::DSPPut
 *
 * Copy data from input buffer to DSP buffer
 ******************************************************************************/

OMX_BOOL OmxDspAudioDecoder::DSPPut (OMX_BUFFERHEADERTYPE* b)
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
 * OmxDspAudioDecoder::DSPGet
 *
 * Retrieve decoded data from DSP engine
 ******************************************************************************/

OMX_BOOL OmxDspAudioDecoder::DSPGet (OMX_BUFFERHEADERTYPE *b)
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
        
        /* ...no need more hDispBuffer pointer */
        hDispBuffer = NULL;
        
        return OMX_TRUE;
    }
    else
    {
        return TRACE (DEBUG, _b("Nothing to display")), OMX_FALSE;
    }
}

/*******************************************************************************
 * OmxDspAudioDecoder::DSPResize
 *
 * Reconfigure port params
 ******************************************************************************/

int OmxDspAudioDecoder::DSPResize (Buffer_Handle hBuf)
{
    /***************************************************************************
    * Overloaded in child classes
    **************************************************************************/

    return 0;
}

/*******************************************************************************
 * OmxDspAudioDecoder::DSPProcess
 *
 * Audio decoding process
 ******************************************************************************/
OMX_BOOL OmxDspAudioDecoder::DSPProcess (OMX_PARAM_PORTDEFINITIONTYPE* aPortParam, OMX_BOOL *aResizeFlag)
{
    Buffer_Handle   hBuf;
    Int             result;
    Int             iBites_Before_Proc, iBites_After_Proc, iBites_Diff;

    /***************************************************************************
     * Make sure the input buffer contains data
     **************************************************************************/

    if ((iBites_Before_Proc = Buffer_getNumBytesUsed(hDSPBuffer)) == 0)    goto reset;
    
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
    }
    
    TRACE (DEBUG, _b("Use DSP buffer: %p"), hOutBuf);

    /***************************************************************************
     * Perform actual decoding
     **************************************************************************/

    if ((result = Adec1_process (hAd, hDSPBuffer, hOutBuf)) < 0)
    {
        /***********************************************************************
         * Decoding error
         **********************************************************************/

        return TRACE (PROCESS, _b("Fatal processing error: %d"), result), OMX_FALSE;
    }
    else
    {
        /***********************************************************************
         * Mark the output buffer needs to be taken from pool next time
         **********************************************************************/

        /* ...take display buffer if possible */
//        if (result == 0)
        {
            iFrameTimestamp = ipInputBuffer->nTimeStamp;

            /* ...audio frame decoded */
            TRACE (PROCESS, _b("Audio frame decoded"));
            
            /* ...update display buffer if needed */
            (hDispBuffer == NULL ? hDispBuffer = hOutBuf : 0);

            /* ...getting processed bites */
            iBites_After_Proc = Buffer_getNumBytesUsed(hDSPBuffer);

            /* ...how many encoded bites remain in buffer */
            iBites_Diff = iBites_Before_Proc - iBites_After_Proc;

            /* ...update read/write params */
            iDSPWriteIndex -= iBites_After_Proc;

            if (iBites_Diff > 0)
            {
                iDSPBufferOffset += iBites_After_Proc;

                /* ...change DSP buffer geometry and update user pointer */
                hDSPBuffer->userPtr += iBites_After_Proc;
                hDSPBuffer->physPtr += iBites_After_Proc;
                pDSPBuffer = (OMX_U8*) Buffer_getUserPtr (hDSPBuffer);
            }

            TRACE (DEBUG, _b(" Bites before: %d; after: %d; DSPWriteIndex: %u; iDSPBufferOffset: %u"),
                            iBites_Before_Proc, iBites_After_Proc, iDSPWriteIndex, iDSPBufferOffset);

            Buffer_setNumBytesUsed(hDSPBuffer, iBites_Diff);

            /* ...check if the end of DSP-buffer is nearly reached, copy encoded data to the beginning */
            if ((iDSPBufferOffset + iBites_Diff) > (iDSPBufferSize - OMX_MAX_AUDIO_FRAME_SIZE))
            {
                /* ...reset DSP-buffer geometry and update user pointer */
                hDSPBuffer->userPtr -= iDSPBufferOffset;
                hDSPBuffer->physPtr -= iDSPBufferOffset;
                pDSPBuffer = (OMX_U8*) Buffer_getUserPtr (hDSPBuffer);

                /* ...copy encoded data to the beginning of the DSP-buffer */
                memcpy(pDSPBuffer, pDSPBuffer + iDSPBufferOffset, iBites_Diff);
                Buffer_setNumBytesUsed(hDSPBuffer, iBites_Diff);

                /* ...reset reading offset */
                iDSPBufferOffset = 0;
            }

            /* ...keep at least one default ADEC1 buffer size filled */
            if ((iBites_Diff < (iDSPBufferSize / 10)) || (iEndofStream == OMX_TRUE))
            {
                iDSPWriteIndex = iBites_Diff;
                iEndOfFrameFlag = OMX_FALSE;
            }

            /* ...resize audio if possible */
            (iFrameCount == 0 ? *aResizeFlag = OMX_TRUE, 1 : 0);
        } //else
        if (result == Dmai_EBITERROR)
        {
            TRACE (PROCESS, _b("Corrupted stream %d"), result);
            BufTab_freeBuf (hOutBuf);
        }

        /* ...request new buffer */
        hOutBuf = NULL;
    }
 
reset:
   
    /***************************************************************************
     * Enable input path and return success decoding status
     **************************************************************************/

    return OMX_TRUE;
}

/*******************************************************************************
 * OmxDspAudioDecoder::DSPRelease
 *
 * Release output buffer back to the pool
 ******************************************************************************/

void OmxDspAudioDecoder::DSPRelease (OMX_BUFFERHEADERTYPE *b)
{
    Buffer_Handle   hBuf;
    
    /* ...check out if platform private data is set */
    if ((hBuf = (Buffer_Handle)b->pPlatformPrivate) != NULL)
    {
        /* ...mark the buffer is not owned by renderer anymore */
        BufTab_freeBuf (hBuf);

        /* ...capture stop time of buffer holding time */
        (OMX_DSP_PC && hBuf == BufTab_getBuf(hOutBufTab, 0) ? PC_STOP(HOLD), 1 : 0);

        TRACE (DEBUG, _b("Release DSP buffer: %p"), hBuf);
    }
}

/*******************************************************************************
 * OmxDspAudioDecoder::DSPFlush
 *
 * Flush audio decoder 
 ******************************************************************************/

void OmxDspAudioDecoder::DSPFlush (void)
{
    /*
     * Not implemented in Adec1 API, dead code...
     */   
 
    TRACE (PROCESS, _b("Audio decoder flushed"));
}
