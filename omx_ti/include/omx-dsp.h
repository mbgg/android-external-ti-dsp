/*******************************************************************************
 * omx-dsp.h
 *
 * Base include for OMX DSP components
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

#ifdef __OMX_DSP_H
#error "File 'omx-dsp.h' already included"
#endif

#define __OMX_DSP_H

/*******************************************************************************
 * Android includes
 ******************************************************************************/

#include <utils/Log.h>

/*******************************************************************************
 * Local configuration
 ******************************************************************************/

/* ...global tracing */
#define OMX_DSP_TRACE                   1

/* ...global performance monitoring */
#define OMX_DSP_PC                      1

/* ...global capturing */
#define OMX_DSP_CAPTURE                 1

/* ...runtime bugchecks */
#define OMX_DSP_BUG                     1

/* ...use local thread time base */
#define OMX_DSP_THREAD_TIMESTAMP        0

/*******************************************************************************
 * Local includes
 ******************************************************************************/

/* ...debugging facilities */
#include "omx-dsp-debug.h"
