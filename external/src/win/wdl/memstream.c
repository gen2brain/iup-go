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

/* Enable C preprocessor wrappers for COM methods in <objidl.h> */
#ifndef COBJMACROS
    #define COBJMACROS
#endif

#include "memstream.h"


typedef struct MEMSTREAM_TAG MEMSTREAM;
struct MEMSTREAM_TAG {
    IStream stream;  /* COM interface */
    LONG refs;

    const BYTE* buffer;
    ULONG pos;
    ULONG size;
};


#define OFFSETOF(type, member)              ((size_t) &((type*)0)->member)
#define CONTAINEROF(ptr, type, member)      ((type*)((BYTE*)(ptr) - OFFSETOF(type, member)))

#define MEMSTREAM_FROM_IFACE(stream_iface)  CONTAINEROF(stream_iface, MEMSTREAM, stream)


static HRESULT STDMETHODCALLTYPE
memstream_QueryInterface(IStream* self, REFIID riid, void** obj)
{
    if(IsEqualGUID(riid, &IID_IUnknown)  ||
       IsEqualGUID(riid, &IID_IDispatch)  ||
       IsEqualGUID(riid, &IID_ISequentialStream)  ||
       IsEqualGUID(riid, &IID_IStream))
    {
        MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);
        InterlockedIncrement(&s->refs);
        *obj = s;
        return S_OK;
    } else {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
memstream_AddRef(IStream* self)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);
    return InterlockedIncrement(&s->refs);
}

static ULONG STDMETHODCALLTYPE
memstream_Release(IStream* self)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);
    ULONG refs;

    refs = InterlockedDecrement(&s->refs);
    if(refs == 0)
        free(s);
    return refs;
}

static HRESULT STDMETHODCALLTYPE
memstream_Read(IStream* self, void* buf, ULONG n, ULONG* n_read)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);

    /* Return S_FALSE, if we are already in the end-of-file situation. */
    if(s->pos >= s->size) {
        n = 0;
        if(n_read != NULL)
            *n_read = 0;
        return S_FALSE;
    }

    if(n > s->size - s->pos)
        n = s->size - s->pos;
    memcpy(buf, s->buffer + s->pos, n);
    s->pos += n;

    if(n_read != NULL)
        *n_read = n;

    /* From https://msdn.microsoft.com/en-us/library/windows/desktop/aa380037(v=vs.85).aspx:
     *
     * Reads a specified number of bytes from the stream object into memory
     * starting at the current seek pointer. This implementation returns S_OK
     * if the end of the stream was reached during the read. (This is the same
     * as the "end of file" behavior found in the MS-DOS FAT file system.)
     */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
memstream_Write(IStream* self, const void* buf, ULONG n, ULONG* n_written)
{
    /* We are read-only stream. */
    if(n_written != NULL)
        *n_written = 0;
    return STG_E_ACCESSDENIED;
}

static HRESULT STDMETHODCALLTYPE
memstream_Seek(IStream* self, LARGE_INTEGER delta, DWORD origin,
                    ULARGE_INTEGER* p_new_pos)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);
    LARGE_INTEGER pos;
    HRESULT hr;

    switch(origin) {
        case STREAM_SEEK_SET:  pos.QuadPart = delta.QuadPart; break;
        case STREAM_SEEK_CUR:  pos.QuadPart = s->pos + delta.QuadPart; break;
        case STREAM_SEEK_END:  pos.QuadPart = s->size + delta.QuadPart; break;
        default:               hr = STG_E_INVALIDPARAMETER; goto end;
    }

    /* Note MSDN states it is an error, if the result is negative, but it is
     * in principle OK, if we are beyond end-of-file. (In such case subsequent
     * Read() just shall return S_FALSE to indicate end-of-file situation.)
     */
    if(pos.QuadPart < 0) {
        hr = STG_E_INVALIDFUNCTION;
        goto end;
    }

    /* In 32-bit, there is a danger of overflow. */
    if(pos.QuadPart != (ULONG) pos.QuadPart) {
        hr = STG_E_INVALIDFUNCTION;
        goto end;
    }

    s->pos = (ULONG) pos.QuadPart;
    hr = S_OK;

end:
    if(p_new_pos != NULL)
        p_new_pos->QuadPart = s->pos;
    return hr;
}

static HRESULT STDMETHODCALLTYPE
memstream_SetSize(IStream* self, ULARGE_INTEGER new_size)
{
    /* We are read-only stream. */
    return STG_E_INVALIDFUNCTION;
}

static HRESULT STDMETHODCALLTYPE
memstream_CopyTo(IStream* self, IStream* other, ULARGE_INTEGER n,
                      ULARGE_INTEGER* n_read, ULARGE_INTEGER* n_written)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);
    ULONG written;
    HRESULT hr;

    if(s->pos + n.QuadPart >= s->size)
        n.QuadPart = (s->pos < s->size ? s->size - s->pos : 0);

    hr = IStream_Write(other, s->buffer + s->pos, (ULONG) n.QuadPart, &written);

    /* In case of failure, MSDN states that the seek pointers are invalid
     * in source as well as destinations streams. So lets just abort. */
    if(FAILED(hr))
        return hr;

    s->pos += written;

    /* And in the case of success, MSDN specifies that *n_read and *n_written
     * are set to the same value. */
    if(n_read != NULL)
        n_read->QuadPart = written;
    if(n_written != NULL)
        n_written->QuadPart = written;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
memstream_Commit(IStream* self, DWORD flags)
{
    /* Noop, since we never change contents of the stream. */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
memstream_Revert(IStream* self)
{
    /* Noop, since we never change contents of the stream. */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
memstream_LockRegion(IStream* self, ULARGE_INTEGER offset,
                          ULARGE_INTEGER n, DWORD type)
{
    /* We do not support locking. */
    return STG_E_INVALIDFUNCTION;
}

static HRESULT STDMETHODCALLTYPE
memstream_UnlockRegion(IStream* self, ULARGE_INTEGER offset,
                            ULARGE_INTEGER n, DWORD type)
{
    /* We do not support locking. */
    return STG_E_INVALIDFUNCTION;
}

static HRESULT STDMETHODCALLTYPE
memstream_Stat(IStream* self, STATSTG* stat, DWORD flag)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);

    memset(stat, 0, sizeof(STATSTG));
    stat->type = STGTY_STREAM;
    stat->cbSize.QuadPart = s->size;
    stat->grfMode = STGM_READ;          /* We are read-only. */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
memstream_Clone(IStream* self, IStream** p_other)
{
    MEMSTREAM* s = MEMSTREAM_FROM_IFACE(self);
    IStream* o;
    HRESULT hr;

    hr = memstream_create(s->buffer, s->size, &o);
    if(o != NULL) {
        MEMSTREAM* so = MEMSTREAM_FROM_IFACE(o);
        so->pos = s->pos;
    }

    *p_other = o;
    return hr;
}


static IStreamVtbl memstream_vtable = {
    memstream_QueryInterface,
    memstream_AddRef,
    memstream_Release,
    memstream_Read,
    memstream_Write,
    memstream_Seek,
    memstream_SetSize,
    memstream_CopyTo,
    memstream_Commit,
    memstream_Revert,
    memstream_LockRegion,
    memstream_UnlockRegion,
    memstream_Stat,
    memstream_Clone
};


HRESULT
memstream_create(const BYTE* buffer, ULONG size, IStream** p_stream)
{
    MEMSTREAM* s;

    s = (MEMSTREAM*) malloc(sizeof(MEMSTREAM));
    if(s == NULL) {
        *p_stream = NULL;
        return ERROR_OUTOFMEMORY;
    }

    s->buffer = buffer;
    s->pos = 0;
    s->size = size;
    s->refs = 1;
    s->stream.lpVtbl = &memstream_vtable;

    *p_stream = &s->stream;
    return S_OK;
}

HRESULT
memstream_create_from_resource(HINSTANCE instance,
            const WCHAR* res_type, const WCHAR* res_name, IStream** p_stream)
{
    HRSRC res;
    DWORD res_size;
    HGLOBAL res_global;
    void* res_data;

    /* We rely on the fact that UnlockResource() and FreeResource() do nothing:
     *  -- MSDN docs for LockResource() says no unlocking is needed.
     *  -- MSDN docs for FreeResource() says it just returns FALSE on 32/64-bit
     *     Windows.
     *
     * See also http://blogs.msdn.com/b/oldnewthing/archive/2011/03/07/10137456.aspx
     *
     * It may look a bit ugly, but it simplifies things a lot as the stream
     * does not need to do any bookkeeping for the resource.
     */

    if((res = FindResourceW(instance, res_name, res_type)) == NULL  ||
       (res_size = SizeofResource(instance, res)) == 0  ||
       (res_global = LoadResource(instance, res)) == NULL  ||
       (res_data = LockResource(res_global)) == NULL)
    {
        *p_stream = NULL;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return memstream_create(res_data, res_size, p_stream);
}

