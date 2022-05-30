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

#include "backend-wic.h"


static HMODULE wic_dll = NULL;

IWICImagingFactory* wic_factory = NULL;

/* According to MSDN, GUID_WICPixelFormat32bppPBGRA is the recommended pixel
 * format for cooperation with Direct2D. Note we define it here manually to
 * avoid need to link with UUID.LIB. */
const GUID wic_pixel_format =
        {0x6fddc324,0x4e03,0x4bfe,{0xb1,0x85,0x3d,0x77,0x76,0x8d,0xc9,0x10} };


int
wic_init(void)
{
#ifdef WINCODEC_SDK_VERSION1
    /* This is only available in newer SDK headers. */
    static const UINT wic_version = WINCODEC_SDK_VERSION1;
#else
    static const UINT wic_version = WINCODEC_SDK_VERSION;
#endif

    HRESULT (WINAPI* fn_WICCreateImagingFactory_Proxy)(UINT, IWICImagingFactory**);
    HRESULT hr;

    wic_dll = wd_load_system_dll(_T("WINDOWSCODECS.DLL"));
    if(wic_dll == NULL) {
        WD_TRACE_ERR("wic_init: wd_load_system_dll(WINDOWSCODECS.DLL) failed.");
        goto err_LoadLibrary;
    }

    fn_WICCreateImagingFactory_Proxy =
            (HRESULT (WINAPI*)(UINT, IWICImagingFactory**))
            GetProcAddress(wic_dll, "WICCreateImagingFactory_Proxy");
    if(fn_WICCreateImagingFactory_Proxy == NULL) {
        WD_TRACE_ERR("wic_init: GetProcAddress(WICCreateImagingFactory_Proxy) failed.");
        goto err_GetProcAddress;
    }

    hr = fn_WICCreateImagingFactory_Proxy(wic_version, &wic_factory);
    if(FAILED(hr)) {
        WD_TRACE_HR("wic_init: WICCreateImagingFactory_Proxy() failed.");
        goto err_WICCreateImagingFactory_Proxy;
    }

    /* Success. */
    return 0;

    /* Error path unwinding. */
err_WICCreateImagingFactory_Proxy:
err_GetProcAddress:
    FreeLibrary(wic_dll);
    wic_dll = NULL;
err_LoadLibrary:
    return -1;
}

void
wic_fini(void)
{
    IWICImagingFactory_Release(wic_factory);
    wic_factory = NULL;

    FreeLibrary(wic_dll);
    wic_dll = NULL;
}


IWICBitmapSource*
wic_convert_bitmap(IWICBitmapSource* bitmap)
{
    GUID pixel_format;
    IWICFormatConverter* converter;
    HRESULT hr;

    hr = IWICBitmapSource_GetPixelFormat(bitmap, &pixel_format);
    if(FAILED(hr)) {
        WD_TRACE_HR("wc_convert_bitmap: "
                    "IWICBitmapSource::GetPixelFormat() failed.");
        return NULL;
    }

    if(IsEqualGUID(&pixel_format, &wic_pixel_format)) {
        /* No conversion needed. */
        IWICBitmapSource_AddRef(bitmap);
        return bitmap;
    }

    hr = IWICImagingFactory_CreateFormatConverter(wic_factory, &converter);
    if(FAILED(hr)) {
        WD_TRACE_HR("wc_convert_bitmap: "
                    "IWICImagingFactory::CreateFormatConverter() failed.");
        return NULL;
    }

    hr = IWICFormatConverter_Initialize(converter, bitmap, &wic_pixel_format,
            WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
    if(FAILED(hr)) {
        WD_TRACE_HR("wc_convert_bitmap: "
                    "IWICFormatConverter::Initialize() failed.");
        IWICFormatConverter_Release(converter);
        return NULL;
    }

    return (IWICBitmapSource*) converter;
}
