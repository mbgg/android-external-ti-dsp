/*******************************************************************************
 * iface.cpp
 *
 * Simple interface for Android mediaserver to open Codec Engine.  
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

#define MODULE_TAG                      INTERFACE

#include "omx-dsp.h"
#include "iface.h"
#include <ti/sdo/dmai/ce/Adec1.h>

#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/osal/Lock.h>
#include <ti/sdo/ce/ipc/Comm.h>
#include <ti/sdo/ce/ipc/Processor.h>
#include <ti/sdo/ce/osal/Queue.h>
#include <ti/sdo/ce/osal/Global.h>

/* ...tracing configuration */
TRACE_TAG (INIT,    1);
TRACE_TAG (DATA,    1);
TRACE_TAG (PROCESS, 1);
TRACE_TAG (DEBUG,   1);
TRACE_TAG (ERROR,   1);
TRACE_TAG (INFO,    1);

InterfaceClass::InterfaceClass()
{
    TRACE(INFO, _b("Construct interface class"));
}

InterfaceClass::~InterfaceClass()
{
    TRACE(INFO, _b("Destroy interface class"));
}

extern "C" {
Engine_Handle global_engine_handle;
}

void InterfaceClass::EngineInit()
{
    Engine_Error        ec;

    TRACE(INFO, _b("EngineInit IN"));

    /* ...initialize codec engine runtime */
    CERuntime_init ();

    /* ...initialize DMAI framework */
    Dmai_init ();

    if ((global_engine_handle = Engine_open(String("codecServer"), NULL, &ec)) == NULL)
    {
        TRACE (ERROR, _b("Failed to open engine 'codecServer': %X"), (unsigned) ec);
        return;
    }

    TRACE(INFO, _b("EngineInit OUT"));
}

/*
 * Currently there is no method to call this function. No
 * exit signals or callbacks from the mediaserver weren't found. But the call is important 
 * becasuse if mediaserver crashes, codec server cannot be re-opened without
 * closing.
 */
void InterfaceClass::EngineDeInit(void)
{
    TRACE(INFO, _b("EngineClose IN-OUT"));

    Engine_close(global_engine_handle);
    global_engine_handle = NULL;

    CERuntime_exit ();
}

/*
 * Temporary debug function
 * to-be-removed
 */
void InterfaceClass::Adec(void)
{
    return;
}
