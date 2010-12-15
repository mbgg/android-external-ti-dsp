/*******************************************************************************
 * omx-dsp-mp3.cpp
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

#define MODULE_TAG                      MP3

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "omx-dsp.h"
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

/*******************************************************************************
 * Local constants definitions
 ******************************************************************************/

static const OMX_STRING     mp3_decoder_name = (OMX_STRING) "mp3dec";

/*******************************************************************************
 * Plugin system infrastructure
 ******************************************************************************/

/*******************************************************************************
 * Mp3OmxDspComponentFactory
 *
 * This function is called by OMX_GetHandle and it creates an instance of the 
 * mp3 component AO
 ******************************************************************************/

OMX_ERRORTYPE Mp3OmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspMp3Decoder   *d;
    OMX_ERRORTYPE       Status;

    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /***************************************************************************
     * Create decoder component object
     **************************************************************************/

    if ((d = (OmxDspMp3Decoder*)OSCL_NEW(OmxDspMp3Decoder, ())) == NULL)
    {
        return TRACE (ERROR, _X("Failed to allocate resources")), OMX_ErrorInsufficientResources;
    }

    /***************************************************************************
     * Call the construct component to initialize OMX types
     **************************************************************************/

    /* ...construct component */
    Status = d->ConstructComponent (pAppData, pProxy, mp3_decoder_name);

    /* ...and return the handle and status */
    return *pHandle = d->GetOmxHandle(), TRACE (DEBUG, _b("Component[%p] created: %d"), (void*)(*pHandle), Status), Status;
}

/*******************************************************************************
 * Mp3OmxDspComponentDestructor
 *
 * This function is called by OMX_FreeHandle when component AO needs to be 
 * destroyed
 ******************************************************************************/

OMX_ERRORTYPE Mp3OmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspMp3Decoder   *d = (OmxDspMp3Decoder*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;
    
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
 * OmxDspMp3Decoder::OmxDspMp3Decoder
 *
 * Component object constructor
 ******************************************************************************/

OmxDspMp3Decoder::OmxDspMp3Decoder()
{
    TRACE (DEBUG, _b("Component[%p] constructed"), this);
    
    /* ...add component to scheduler */
    (!IsAdded() ? AddToScheduler(), 1 : 0);
}

/*******************************************************************************
 * OmxDspMp3Decoder::~OmxDspMp3Decoder
 *
 * Component object destructor
 ******************************************************************************/

OmxDspMp3Decoder::~OmxDspMp3Decoder()
{
    /* ...remove from scheduler as required */
    (IsAdded() ? RemoveFromScheduler(), 1 : 0);

    TRACE (DEBUG, _b("Component[%p] destructed"), this);
}

/*******************************************************************************
 * Component interface implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspMp3Decoder::ConstructComponent
 *
 * Construct component
 ******************************************************************************/

OMX_ERRORTYPE OmxDspMp3Decoder::ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName)
{
    ComponentPortType  *pInPort, *pOutPort;
    OMX_ERRORTYPE       Status;

    /***************************************************************************
     * Call base class construction function first
     **************************************************************************/

    if ((Status = OmxDspAudioDecoder::ConstructComponent (pAppData, pProxy, cDecoderName)) != OMX_ErrorNone)
    {
        /* ...base component creation failed; resign */
        return Status;
    }

    /***************************************************************************
     * Initialize ports
     **************************************************************************/

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /***************************************************************************
     * Input port configuration - encoded MP3 audio stream
     **************************************************************************/

    /* ...input audio format */
    pInPort->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

    /* ...input/output ports dimentions */
    pInPort->PortParam.nBufferCountActual = OMX_DSP_MP3_INPUT_BUFFER_NUMBER;
    pInPort->PortParam.nBufferSize = OMX_DSP_MP3_INPUT_BUFFER_SIZE;
    pOutPort->PortParam.nBufferCountActual = OMX_DSP_MP3_OUTPUT_BUFFER_NUMBER;
    pOutPort->PortParam.nBufferSize = OMX_DSP_MP3_OUTPUT_BUFFER_SIZE;

    /* ...default values for MP3 audio param port */
    pInPort->AudioMp3Param.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->AudioMp3Param.nChannels = 2;
    pInPort->AudioMp3Param.nBitRate = 0;
    pInPort->AudioMp3Param.nSampleRate = 44100;
    pInPort->AudioMp3Param.nAudioBandWidth = 0;
    pInPort->AudioMp3Param.eChannelMode = OMX_AUDIO_ChannelModeStereo;
    pInPort->AudioMp3Param.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

    pInPort->AudioEqualizerType.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->AudioEqualizerType.sBandIndex.nMin = 0;
    pInPort->AudioEqualizerType.sBandIndex.nValue = 0; //(e_equalization) flat;
    pInPort->AudioEqualizerType.sBandIndex.nMax = 7; //(e_equalization) flat_;

    pInPort->AudioParam.eEncoding = OMX_AUDIO_CodingMP3;

    oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"audio_decoder.mp3", OMX_MAX_STRINGNAME_SIZE);

    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("MP3 decoder component constructed")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspMp3Decoder::DestroyComponent
 *
 * This function is called by the omx core when the component is disposed by 
 * the IL client with a call to FreeHandle().
 ******************************************************************************/

OMX_ERRORTYPE OmxDspMp3Decoder::DestroyComponent (void)
{
    OMX_ERRORTYPE   Status;
    
    /***************************************************************************
     * Pass control to base version (no specific actions)
     **************************************************************************/

    return Status = OmxDspAudioDecoder::DestroyComponent(), TRACE (INIT, _b("MP3 decoder component destructed")), Status;
}
/*******************************************************************************
 * DSP engine support
 ******************************************************************************/

/*******************************************************************************
 * OmxDspMP3Decoder::DSPDecoderInit
 *
 * Initialize DSP decoder 
 ******************************************************************************/

OMX_ERRORTYPE OmxDspMp3Decoder::DSPDecoderInit (void)
{
    OMX_ERRORTYPE   Status;

    /***************************************************************************
     * Initialize base audio decoder
     **************************************************************************/

    if ((Status = OmxDspAudioDecoder::DSPDecoderInit()) != OMX_ErrorNone)
    {
        return Status;
    }

    iDSPWriteIndex = 0;
    iDSPBufferOffset = 0;

    /***************************************************************************
     * Decoder engine successfully created
     **************************************************************************/

    return TRACE (INIT, _b("MP3 audio decoder initialized")), OMX_ErrorNone;
}
    
/*******************************************************************************
 * OmxDspMp3Decoder::DSPPut
 *
 * Copy data from input buffer to DSP buffer
 ******************************************************************************/

OMX_BOOL OmxDspMp3Decoder::DSPPut (OMX_BUFFERHEADERTYPE* b)
{
    OMX_U32     n = b->nFilledLen;
    OMX_U32     e;

    TRACE (DATA, _b("copy %u bytes: [%lu, %lu)"), (unsigned)n, iDSPWriteIndex, iDSPWriteIndex + n);
    
    /* ...check out there is enough room to store data (TBD) */
    if ((e = iDSPWriteIndex + n) > iDSPBufferSize) return TRACE (DATA, _b("Input buffer to large")), OMX_FALSE;
    
    /* ...copy input buffer content into DSP-owned buffer */
    oscl_memcpy (pDSPBuffer + iDSPWriteIndex, b->pBuffer + b->nOffset, n);
    
    /* ...set input buffer length and reset writing pointer for a final chunk */
    (iEndOfFrameFlag == OMX_TRUE ? TRACE (DATA, _b("Received frame: %lu bytes"), e), Buffer_setNumBytesUsed (hDSPBuffer, e), e = 0 : 0);

    /* ...advance writing index and return success result */
    return iDSPWriteIndex += n, OMX_TRUE;
}

int OmxDspMp3Decoder::DSPResize (Buffer_Handle hBuf)
{
    ComponentPortType  *pInPort, *pOutPort;
    Int                 SampleRate;
    OMX_U32             OutputLength;

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /***************************************************************************
    * Setting rate for output port. Call once after first frame received.
    **************************************************************************/

    SampleRate = Adec1_getSampleRate(hAd);

    pInPort->AudioMp3Param.nSampleRate = SampleRate;
    pInPort->AudioMp3Param.nChannels = 2;
    pOutPort->AudioPcmMode.nSamplingRate = SampleRate;
    pOutPort->AudioPcmMode.nChannels = 2;

    iFrameCount++;
    iResizePending = OMX_TRUE;

    /* ...send port settings changed event */
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->CompHandle;
    (*(ipCallbacks->EventHandler))
      (pHandle,
       iCallbackData,
       OMX_EventPortSettingsChanged,
       OMX_PORT_OUTPUTPORT_INDEX,
       0,
       NULL);

    return 0;
}

