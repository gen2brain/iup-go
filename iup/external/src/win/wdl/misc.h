/*
 * WinDrawLib
 * Copyright (c) 2016 Martin Mitas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef WD_MISC_H
#define WD_MISC_H


#include <float.h>
#include <math.h>
#include <tchar.h>
#include <windows.h>
#include <malloc.h>

#include "wdl.h"


/***********************
 ***  Debug Logging  ***
 ***********************/

#if !defined _MSC_VER || (defined _MSC_VER && _MSC_VER > 1200)
#define no_log(...)     do { } while(0)
#else
static void no_log(const char* fmt, ...) {}
#endif

#ifdef DEBUG
    void wd_log(const char* fmt, ...);
    #define WD_TRACE    wd_log
#else
    #define WD_TRACE    no_log
#endif

#define WD_TRACE_ERR_(msg, err)          WD_TRACE(msg " [%lu]", (err))
#define WD_TRACE_ERR(msg)                WD_TRACE(msg " [%lu]", GetLastError())
#define WD_TRACE_HR_(msg, hr)            WD_TRACE(msg " [0x%lx]", (hr))
#define WD_TRACE_HR(msg)                 WD_TRACE(msg " [0x%lx]", hr)


/**************************
 ***  Helper Functions  ***
 **************************/

#define WD_PI               3.14159265358979323846f

#define WD_MIN(a,b)         ((a) < (b) ? (a) : (b))
#define WD_MAX(a,b)         ((a) > (b) ? (a) : (b))

#define WD_ABS(a)           ((a) > 0 ? (a) : -(a))

#define WD_SIZEOF_ARRAY(a)  (sizeof((a)) / sizeof((a)[0]))

#define WD_OFFSETOF(type, member)           ((size_t) &((type*)0)->member)
#define WD_CONTAINEROF(ptr, type, member)   ((type*)((BYTE*)(ptr) - WD_OFFSETOF(type, member)))


/* Safer LoadLibrary() replacement for system DLLs. */
HMODULE wd_load_system_dll(const TCHAR* dll_name);


#ifdef _MSC_VER
    /* MSVC does not understand "inline" when building as pure C (not C++).
     * However it understands "__inline" */
    #ifndef __cplusplus
        #define inline __inline
    #endif

    #if _MSC_VER <= 1200
        /* With MSVC 6.0, these are missing in <malloc.h>. */
        #define _malloca    malloc
        #define _freea      free

        /* With MSVC 6.0, these are missing in <math.h>. */
        static inline float floorf(float x)             { return (float)floor((double)x); }
        static inline float ceilf(float x)              { return (float)ceil((double)x); }
        static inline float powf(float x, float y)      { return (float)pow((double)x, (double)y); }
        static inline float cosf(float x)               { return (float)cos((double)x); }
        static inline float sinf(float x)               { return (float)sin((double)x); }
        static inline float atan2f(float x, float y)    { return (float)atan2((double)x, (double)y); }
        static inline float sqrtf(float x)              { return (float)sqrt((double)x); }
    #endif
#endif


#endif  /* WD_MISC_H */
