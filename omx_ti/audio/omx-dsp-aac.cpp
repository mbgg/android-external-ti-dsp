/*******************************************************************************
 * omx-dsp-aac.cpp
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

#define MODULE_TAG                      AAC

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "omx-dsp.h"
#include "omx-dsp-aac.h"

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

static const OMX_STRING     aac_decoder_name = (OMX_STRING) "aachedec";

/*******************************************************************************
 * Plugin system infrastructure
 ******************************************************************************/

/*******************************************************************************
 * AacOmxDspComponentFactory
 *
 * This function is called by OMX_GetHandle and it creates an instance of the 
 * aac component AO
 ******************************************************************************/

OMX_ERRORTYPE AacOmxDspComponentFactory (OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_IN OMX_PTR pProxy , OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspAacDecoder   *d;
    OMX_ERRORTYPE       Status;

    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    /***************************************************************************
     * Create decoder component object
     **************************************************************************/

    if ((d = (OmxDspAacDecoder*)OSCL_NEW(OmxDspAacDecoder, ())) == NULL)
    {
        return TRACE (ERROR, _X("Failed to allocate resources")), OMX_ErrorInsufficientResources;
    }

    /***************************************************************************
     * Call the construct component to initialize OMX types
     **************************************************************************/

    /* ...construct component */
    Status = d->ConstructComponent (pAppData, pProxy, aac_decoder_name);

    /* ...and return the handle and status */
    return *pHandle = d->GetOmxHandle(), TRACE (DEBUG, _b("Component[%p] created: %d"), (void*)(*pHandle), Status), Status;
}

/*******************************************************************************
 * AacOmxDspComponentDestructor
 *
 * This function is called by OMX_FreeHandle when component AO needs to be 
 * destroyed
 ******************************************************************************/

OMX_ERRORTYPE AacOmxDspComponentDestructor (OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OmxDspAacDecoder   *d = (OmxDspAacDecoder*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;
    
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
 * OmxDspAacDecoder::OmxDspAacDecoder
 *
 * Component object constructor
 ******************************************************************************/

OmxDspAacDecoder::OmxDspAacDecoder()
{
    TRACE (DEBUG, _b("Component[%p] constructed"), this);
    
    /* ...add component to scheduler */
    (!IsAdded() ? AddToScheduler(), 1 : 0);
}

/*******************************************************************************
 * OmxDspAacDecoder::~OmxDspAacDecoder
 *
 * Component object destructor
 ******************************************************************************/

OmxDspAacDecoder::~OmxDspAacDecoder()
{
    /* ...remove from scheduler as required */
    (IsAdded() ? RemoveFromScheduler(), 1 : 0);

    TRACE (DEBUG, _b("Component[%p] destructed"), this);
}

/*******************************************************************************
 * Component interface implementation
 ******************************************************************************/

/*******************************************************************************
 * OmxDspAacDecoder::ConstructComponent
 *
 * Construct component
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAacDecoder::ConstructComponent (OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING cDecoderName)
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
     * Input port configuration - encoded AAC audio stream
     **************************************************************************/

    /* ...input audio format */
    pInPort->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingAAC;

    /* ...input/output ports dimentions */
    pInPort->PortParam.nBufferCountActual = OMX_DSP_AAC_INPUT_BUFFER_NUMBER;
    pInPort->PortParam.nBufferSize = OMX_DSP_AAC_INPUT_BUFFER_SIZE;
    pOutPort->PortParam.nBufferCountActual = OMX_DSP_AAC_OUTPUT_BUFFER_NUMBER;
    pOutPort->PortParam.nBufferSize = OMX_DSP_AAC_OUTPUT_BUFFER_SIZE * 6;

    /* ...default values for Aac audio param port */
    pInPort->AudioAacParam.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    pInPort->AudioAacParam.nChannels = 2;
    pInPort->AudioAacParam.nBitRate = 0;
    pInPort->AudioAacParam.nSampleRate = 44100;
    pInPort->AudioAacParam.nAudioBandWidth = 0;
    pInPort->AudioAacParam.nFrameLength = 2048; // use HE_PS frame size as default
    pInPort->AudioAacParam.eChannelMode = OMX_AUDIO_ChannelModeStereo;
    pInPort->AudioAacParam.eAACProfile = OMX_AUDIO_AACObjectMain;    //OMX_AUDIO_AACObjectLC;
    pInPort->AudioAacParam.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;

    pInPort->AudioParam.eEncoding = OMX_AUDIO_CodingAAC;

    oscl_strncpy((OMX_STRING)iComponentRole, (OMX_STRING)"audio_decoder.aac", OMX_MAX_STRINGNAME_SIZE);

    /***************************************************************************
     * Initialization completed successfully
     **************************************************************************/

    return TRACE (INIT, _b("AAC decoder component constructed")), OMX_ErrorNone;
}

/*******************************************************************************
 * OmxDspAacDecoder::DestroyComponent
 *
 * This function is called by the omx core when the component is disposed by 
 * the IL client with a call to FreeHandle().
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAacDecoder::DestroyComponent (void)
{
    OMX_ERRORTYPE   Status;
    
    /***************************************************************************
     * Pass control to base version (no specific actions)
     **************************************************************************/

    return Status = OmxDspAudioDecoder::DestroyComponent(), TRACE (INIT, _b("AAC decoder component destructed")), Status;
}
/*******************************************************************************
 * DSP engine support
 ******************************************************************************/

/*******************************************************************************
 * OmxDspAacDecoder::DSPDecoderInit
 *
 * Initialize DSP decoder 
 ******************************************************************************/

OMX_ERRORTYPE OmxDspAacDecoder::DSPDecoderInit (void)
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
    rate_Idx = 0;
    n_Channels = 0;

    /***************************************************************************
     * Decoder engine successfully created
     **************************************************************************/

    return TRACE (INIT, _b("AAC audio decoder initialized")), OMX_ErrorNone;
}
    
/*******************************************************************************
 * OmxDspAacDecoder::CreateADIFHeader
 *
 * Create ADIF header for mp4 stream
 ******************************************************************************/
#define MAX_AAC_HEADER_LENGTH	20

void OmxDspAacDecoder::Create_ADIF_Header(void)
{
    char *Header_Buf;

    TRACE (DEBUG, _b("Generating ADIF header for audio stream"));

/* allocate buffer for the ADIF header */
    Header_Buf = (char *)malloc(MAX_AAC_HEADER_LENGTH);

/* flush allocated buffer */
    memset(Header_Buf, 0, MAX_AAC_HEADER_LENGTH);

/* store id field in ADIF header */
    memcpy(Header_Buf, "ADIF", 4);

/* reset copyright_id_present field in ADIF header */
    Header_Buf[4] &= ~0x1;

/*
 * Store profile value in ADIF header
 * 0 - MAIN, 1 - LC, 2 - SCR, 3 - LTR (2-bit long)
 *
 * LC - 0x1 is used
 */
    Header_Buf[10] |= (0x1 & 0x2), Header_Buf[11] |= ((0x1 & 0x1) << 7);

/*
 * store sampling frequency index value in ADIF header
 */
    Header_Buf[11] |= (rate_Idx << 3);

/*
 * store front_channel_element value in ADIF header
 */
    Header_Buf[11] |= (n_Channels >> 1);
    Header_Buf[12] |= ((n_Channels & 0x1) << 7);

/* store comment field value in ADIF header */
    Header_Buf[16] |= 0x3;

/* Push the header to the decoder */
    oscl_memcpy (pDSPBuffer, Header_Buf, MAX_AAC_HEADER_LENGTH);

    free(Header_Buf);
}

/*******************************************************************************
 * OmxDspAacDecoder::DSPPut
 *
 * Copy data from input buffer to DSP buffer
 ******************************************************************************/

OMX_BOOL OmxDspAacDecoder::DSPPut (OMX_BUFFERHEADERTYPE* b)
{
    OMX_U32     n = b->nFilledLen;
    OMX_U32     e;
    
    TRACE (DATA, _b("copy %u bytes: [%lu, %lu)"), (unsigned)n, iDSPWriteIndex, iDSPWriteIndex + n);
    
    /* ...check out there is enough room to store data (TBD) */
    if ((e = iDSPWriteIndex + n) > iDSPBufferSize) return TRACE (DATA, _b("Input buffer to large")), OMX_FALSE;

/* Look for the AAC-stream header. Add if missing.
 * 2 bytes length frame is an opencore config frame. 
 * Otherwise it's data frame.
 */
    if (e == 2) 
    {
        unsigned char uc = 0;

        iFirstDataFrame = OMX_TRUE;
        iEndOfFrameFlag = OMX_FALSE;

        /* ...getting sample-rate */
        uc |= (b->pBuffer[0] & 7) << 1;
        uc |= (b->pBuffer[1] >> 7);
	memcpy(&rate_Idx, &uc, 1);

        /* ...getting number of channels */
        uc = 0;
        uc |= (b->pBuffer[1] >> 3) & 0xf;
	memcpy(&n_Channels, &uc, 1);

        return OMX_TRUE;
    }
    else if (iFirstDataFrame == OMX_TRUE)
    {
        iFirstDataFrame = OMX_FALSE;

        /* check for the ADTS header: (0xff 0xf1 .. .. .. .. ..) */
        if ((b->pBuffer[b->nOffset + 0] == 0xff) &&
            (b->pBuffer[b->nOffset + 1] == 0xf1))
        {
            TRACE(INFO, _b("Found ADTS header, continue decoding without changes"));
        } else
        /* No ADTS header => mp4 container is used */
        {
            TRACE(INFO, _b("No ADTS header found, needs to generate ADIF header"));

            /* Currently hardcoded */
            Create_ADIF_Header();
            e += 20;
            iDSPWriteIndex = 20;
        }
    }
    
    /* ...copy input buffer content into DSP-owned buffer */
    oscl_memcpy (pDSPBuffer + iDSPWriteIndex, b->pBuffer + b->nOffset, n);
    
    /* ...set input buffer length and reset writing pointer for a final chunk */
    (iEndOfFrameFlag == OMX_TRUE ? TRACE (DATA, _b("Received frame: %lu bytes"), e), Buffer_setNumBytesUsed (hDSPBuffer, e), e = 0 : 0);

    /* ...advance writing index and return success result */
    return iDSPWriteIndex += n, OMX_TRUE;
}

int OmxDspAacDecoder::DSPResize (Buffer_Handle hBuf)
{
    ComponentPortType  *pInPort, *pOutPort;
    Int                 SampleRate;
    OMX_U32             OutputLength;

    pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX], pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    /***************************************************************************
    * Setting rate for output port. Call once after first frame received.
    **************************************************************************/

    SampleRate = Adec1_getSampleRate(hAd);

    pInPort->AudioAacParam.nSampleRate = SampleRate;
    pInPort->AudioAacParam.eAACProfile = OMX_AUDIO_AACObjectMain;
    pInPort->AudioAacParam.nFrameLength = 2048;
    pOutPort->AudioPcmMode.nSamplingRate = SampleRate;
    pOutPort->AudioPcmMode.nChannels = 2;

    iResizePending = OMX_TRUE;

    iFrameCount++;

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

