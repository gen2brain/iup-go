/*
 * C Reusables
 * <http://github.com/mity/c-reusables>
 *
 * Copyright (c) 2015-2018 Martin Mitas
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

#ifndef CRE_MEMSTREAM_H
#define CRE_MEMSTREAM_H

#include <windows.h>
#include <objidl.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Trivial read-only IStream implementation.
 *
 * This is more lightweight alternative to SHCreateMemStream() from SHLWAPI.DLL.
 *
 * This implementation provides these main benefits:
 *   (1) We do not copy the data.
 *   (2) Application does not need SHLWAPI.DLL.
 *
 * (Note that caller is responsible the data in the provided buffer remain
 * valid and immutable for the life time of the IStream.)
 *
 * When not needed anymore, the caller should release the stream as a standard
 * COM object, i.e. via method IStream::Release().
 */

HRESULT memstream_create(const BYTE* buffer, ULONG size, IStream** p_stream);

HRESULT memstream_create_from_resource(HINSTANCE instance,
                        const WCHAR* res_type, const WCHAR* res_name,
                        IStream** p_stream);


#ifdef __cplusplus
}  /* extern "C" { */
#endif

#endif  /* CRE_MEMSTREAM_H */
