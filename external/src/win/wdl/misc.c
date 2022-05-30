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

#include "misc.h"


#ifdef DEBUG

#include <stdio.h>
#include <stdarg.h>

void
wd_log(const char* fmt, ...)
{
    DWORD last_error;
    va_list args;
    char buffer[512];
    int offset = 0;
    int n;

    last_error = GetLastError();

    offset += sprintf(buffer, "[%08x] ", (unsigned) GetCurrentThreadId());

    va_start(args, fmt);
    n = _vsnprintf(buffer + offset, sizeof(buffer)-offset-2, fmt, args);
    va_end(args);
    if(n < 0  ||  n > (int)sizeof(buffer)-offset-2)
        n = sizeof(buffer)-offset-2;
    offset += n;
    buffer[offset++] = '\n';
    buffer[offset++] = '\0';
    OutputDebugStringA(buffer);
    SetLastError(last_error);
}

#endif  /* DEBUG */

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif

HMODULE
wd_load_system_dll(const TCHAR* dll_name)
{
    HMODULE dll_kernel32;
    HMODULE dll;

    /* Check whether flag LOAD_LIBRARY_SEARCH_SYSTEM32 is supported on this
     * system. It has been added in Win Vista/7 with the security update
     * KB2533623. The update also added new symbol AddDllDirectory() so we
     * use that as a canary. */
    dll_kernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
    if(dll_kernel32 != NULL  &&
       GetProcAddress(dll_kernel32, "AddDllDirectory") != NULL)
    {
        dll = LoadLibraryEx(dll_name, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(dll == NULL) {
            WD_TRACE("wd_load_system_library: "
                     "LoadLibraryEx(%s, LOAD_LIBRARY_SEARCH_SYSTEM32) [%lu]",
                     dll_name, GetLastError());
        }
    } else {
        TCHAR path[MAX_PATH];
        UINT dllname_len;
        UINT sysdir_len;

        dllname_len = _tcslen(dll_name);
        sysdir_len = GetSystemDirectory(path, MAX_PATH);
        if(sysdir_len + 1 + dllname_len >= MAX_PATH) {
            WD_TRACE("wd_load_system_library: Buffer too small.");
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return NULL;
        }

        path[sysdir_len] = _T('\\');
        memcpy(path + sysdir_len + 1, dll_name, (dllname_len + 1) * sizeof(TCHAR));
        dll = LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if(dll == NULL) {
            WD_TRACE("wd_load_system_library: "
                     "LoadLibraryEx(%s, LOAD_WITH_ALTERED_SEARCH_PATH) [%lu]",
                     dll_name, GetLastError());
        }
    }

    return dll;
}
