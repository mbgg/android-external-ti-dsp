/*******************************************************************************
 * omx-dsp-debug.h
 *
 * Debugging interface for OMX DSP components
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

#ifndef __OMX_DSP_DEBUG_H
#define __OMX_DSP_DEBUG_H

/*******************************************************************************
 * Auxiliary macros
 ******************************************************************************/

/* ...define a stub for unused declarator */
#define __omx_dsp_stub(tag, line)       __omx_dsp_stub2(tag, line)
#define __omx_dsp_stub2(tag, line)      typedef int __omx_dsp_##tag##_##line

/* ...convert anything into string */
#define __omx_dsp_string(x)             __omx_dsp_string2(x)
#define __omx_dsp_string2(x)            #x

/*******************************************************************************
 * Time support
 ******************************************************************************/

#if OMX_DSP_THREAD_TIMESTAMP
/* ...thread-specific current time in usec */
#define omx_dsp_time_now()                                      \
({                                                              \
    struct timespec tm;                                         \
                                                                \
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tm);                \
                                                                \
    (unsigned long)(tm.tv_sec * 1000000LL + tm.tv_nsec / 1000); \
})  
#else
/* ...system-wide curreny time in usec */
#define omx_dsp_time_now()                                  \
({                                                          \
    struct timeval tv;                                      \
                                                            \
    gettimeofday(&tv, NULL);                                \
                                                            \
    (unsigned long)(tv.tv_sec * 1000000LL + tv.tv_usec);    \
})
#endif

/* ...time difference */
#define omx_dsp_time_sub(a, b)          (unsigned long) ((a) - (b))

/*******************************************************************************
 * Tracing facility
 ******************************************************************************/

#if OMX_DSP_TRACE

/*******************************************************************************
 * TRACE_TAG
 *
 * Trace tag definition
 ******************************************************************************/

#define TRACE_TAG(tag, on)              enum { __omx_dsp_trace_##tag = on }

/*******************************************************************************
 * TRACE
 *
 * Tagged tracing primitive
 ******************************************************************************/

#define TRACE(tag, fmt, ...)            (void)(__omx_dsp_trace_##tag ? __omx_dsp_trace (tag, __omx_dsp_format##fmt, ## __VA_ARGS__), 1 : 0)

/*******************************************************************************
 * Tagged tracing formats
 ******************************************************************************/

/* ...tracing primitive */
#define __omx_dsp_trace(tag, fmt, ...)              \
({                                                  \
    const char *__omx_dsp_tag = #tag;               \
    unsigned long __omx_dsp_time = omx_dsp_time_now();\
                                                    \
    /* ...use Android output system */              \
    LOG (LOG_INFO, "omx-dsp", "%010lu "fmt, ## __VA_ARGS__); \
})

/* ...just a format sring */
#define __omx_dsp_format_n(fmt)         fmt, __omx_dsp_time

/* ...module tag shown */
#define __omx_dsp_format_B(fmt)         "[" __omx_dsp_string(MODULE_TAG) "] " fmt "\n", __omx_dsp_time

/* ...module tag and specific tag shown */
#define __omx_dsp_format_b(fmt)         "[" __omx_dsp_string(MODULE_TAG) ".%s] " fmt "\n", __omx_dsp_time, __omx_dsp_tag

/* ...module tag, file name and line shown */
#define __omx_dsp_format_X(fmt)         "[" __omx_dsp_string(MODULE_TAG) "] - %s@%d - " fmt "\n", __omx_dsp_time, __FILE__, __LINE__

/* ...module tag, trace tage, file name and line shown */
#define __omx_dsp_format_x(fmt)         "[" __omx_dsp_string(MODULE_TAG) ".%s] - %s@%d - " fmt "\n", __omx_dsp_time, __omx_dsp_tag, __FILE__, __LINE__

/*******************************************************************************
 * Globally defined tags
 ******************************************************************************/

/* ...unconditionally OFF */
TRACE_TAG(0, 0);

/* ...unconditionally ON */
TRACE_TAG(1, 1);

#else

#define TRACE_TAG(tag, on)              __omx_dsp_stub(trace_##tag, __LINE__)
#define TRACE(tag, fmt, ...)            (void)0

#endif  /* OMX_DSP_TRACE */

/*******************************************************************************
 * Capturing facility
 ******************************************************************************/

#if OMX_DSP_CAPTURE

/*******************************************************************************
 * CAPTURE_TAG
 *
 * Capture tag definition
 ******************************************************************************/

#define CAPTURE_TAG(tag, type, on)              \
    static FILE *__omx_f_##tag = 0;             \
    __CAPTURE_TAG_EX(tag, type)                 \
    enum { __omx_dsp_capture_##tag = on }

/*******************************************************************************
 * CAPTURE
 *
 * Tagged capturing primitive
 ******************************************************************************/
    
#define CAPTURE(tag, x)                 (void)(__omx_dsp_capture_##tag ? omx_dsp_capture_##tag (x), 1 : 0)

/* ...file name for capturing data */
#define __CAPTURE_FILE(tag, type)       "/data/omx_" #tag "." #type

/* ...capturing tag definition */
#define __CAPTURE_TAG_EX(tag, type)                                                             \
__attribute__ ((unused)) static void omx_dsp_capture_##tag (type x)                             \
{                                                                                               \
    /* ...open file on first invokation */                                                      \
    if (__omx_f_##tag == 0 && (__omx_f_##tag = fopen ( __CAPTURE_FILE(tag, type), "wb")) == 0)  \
    {                                                                                           \
        perror ("Failed to open tag file '" __CAPTURE_FILE(tag, type) "'");                     \
                                                                                                \
        return;                                                                                 \
    }                                                                                           \
                                                                                                \
    /* ...write portion of data */                                                              \
    if (fwrite ((void*)&x, sizeof(type), 1, __omx_f_##tag) != 1)                                \
    {                                                                                           \
        perror ("Couldn't write into '"__CAPTURE_FILE(tag, type) "'");                          \
                                                                                                \
        return;                                                                                 \
    }                                                                                           \
}

/*******************************************************************************
 * CAPTURE
 *
 * Close capturing file
 ******************************************************************************/

#define CAPTURE_CLOSE(tag)              (__omx_f_##tag != 0 ? fclose (__omx_f_##tag), 1 : 0)

#else

#define CAPTURE_TAG(tag, type, on)      __omx_dsp_stub(capture_##tag, __LINE__)
#define CAPTURE(tag, v)                 (void)0
#define CAPTURE_CLOSE(tag)              (void)0

#endif  /* OMX_DSP_CAPTURE */

/*******************************************************************************
 * Performance counters
 ******************************************************************************/

#if OMX_DSP_PC

/*******************************************************************************
 * PC_TAG
 *
 * Performance counter tag definition
 ******************************************************************************/

#define PC_TAG(tag, on)                                                 \
    static __attribute__ ((unused)) uint32  __omx_dsp_pc_##tag##_stamp; \
    CAPTURE_TAG (PC_##tag, uint32, on);                                 \
    enum { __omx_dsp_pc_##tag = on }


/*******************************************************************************
 * PC_START
 *
 * Start performance counter
 ******************************************************************************/

#define PC_START(tag)                                           \
    (void)(__omx_dsp_pc_##tag ? __omx_dsp_pc_##tag##_stamp = omx_dsp_time_now() : 0)

/*******************************************************************************
 * PC_STOP
 *
 * Stop performance counter
 ******************************************************************************/

#define PC_STOP(tag)                                            \
    (void)(__omx_dsp_pc_##tag ? __omx_dsp_pc (tag, omx_dsp_time_sub(omx_dsp_time_now(), __omx_dsp_pc_##tag##_stamp)), 1 : 0)

/* ...internal performance counter collection primitive (to-be-defined) */
#define __omx_dsp_pc(tag, d)                                    \
    CAPTURE (PC_##tag, d)

/* ...collect the performance counter as trace (not used) */
#define __omx_dsp_pc_as_trace(tag, d)                           \
    TRACE(1, _n("[PC." #tag "] %lu\n"), (d))

/*******************************************************************************
 * PC_CLOSE
 *
 * Close performance counter capturing file
 ******************************************************************************/

#define PC_CLOSE(tag)                                           \
    CAPTURE_CLOSE(PC_##tag)

#else

#define PC_TAG(tag, on)                 __omx_dsp_stub(pc_##tag, __LINE__)
#define PC_START(tag)                   (void)0
#define PC_STOP(tag)                    (void)0
#define PC_CLOSE(tag)                   (void)0

#endif  /* OMX_DSP_PC */

/*******************************************************************************
 * Bugchecks
 ******************************************************************************/

#if OMX_DSP_BUG

/*******************************************************************************
 * C_BUG
 *
 * Compilation-time bugcheck
 ******************************************************************************/

#define C_BUG(cond, msg)                __omx_dsp_stub(bug, __LINE__)[(cond ? -1 : 0)]

/*******************************************************************************
 * BUG
 *
 * Run-time bugcheck
 ******************************************************************************/

#define BUG(cond, fmt, ...)                                             \
do                                                                      \
{                                                                       \
    if (cond)                                                           \
    {                                                                   \
        /* ...output message */                                         \
        __omx_dsp_trace(BUG, __omx_dsp_format##fmt, ## __VA_ARGS__);    \
                                                                        \
        /* ...and die (tbd) */                                          \
        while (1);                                                      \
    }                                                                   \
}                                                                       \
while (0)

#else

#define C_BUG(cond, msg)                typedef __unused_c_bug_##__LINE__
#define BUG(cond, fmt, ...)             (void)0

#endif  /* OMX_DSP_BUG */

#endif  /* __OMX_DSP_DEBUG_H */
