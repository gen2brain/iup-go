/*
 * WinDrawLib
 * Copyright (c) 2015-2016 Martin Mitas
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
#include "backend-d2d.h"
#include "backend-wic.h"
#include "backend-gdix.h"
#include "lock.h"
#include "memstream.h"


WD_HIMAGE
wdCreateImageFromHBITMAP(HBITMAP hBmp)
{
    if(d2d_enabled()) {
        IWICBitmap* bitmap;
        IWICBitmapSource* converted_bitmap;
        HRESULT hr;

        if(wic_factory == NULL) {
            WD_TRACE("wdCreateImageFromHBITMAP: Image API disabled.");
            return NULL;
        }

        hr = IWICImagingFactory_CreateBitmapFromHBITMAP(wic_factory, hBmp,
                                                        NULL, WICBitmapUsePremultipliedAlpha, &bitmap);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateImageFromHBITMAP: "
                        "IWICImagingFactory::CreateBitmapFromHBITMAP() failed.");
            return NULL;
        }

        converted_bitmap = wic_convert_bitmap((IWICBitmapSource*) bitmap);
        if(converted_bitmap == NULL)
            WD_TRACE("wdCreateImageFromHBITMAP: wic_convert_bitmap() failed.");

        IWICBitmap_Release(bitmap);

        return (WD_HIMAGE) converted_bitmap;
    } else {
        dummy_GpBitmap* b;
        int status;

        /* OLD status = gdix_vtable->fn_CreateBitmapFromHBITMAP(hBmp, NULL, &b); - does not support alpha */
        status = gdix_create_bitmap(hBmp, &b);
        if (status != 0) {
            WD_TRACE("wdCreateImageFromHBITMAP: "
                     "GdipCreateBitmapFromHBITMAP() failed. [%d]", status);
            return NULL;
        }

        return (WD_HIMAGE) b;
    }
}

WD_HIMAGE
wdLoadImageFromFile(const WCHAR* pszPath)
{
    if(d2d_enabled()) {
        IWICBitmapDecoder* decoder;
        IWICBitmapFrameDecode* bitmap;
        IWICBitmapSource* converted_bitmap = NULL;
        HRESULT hr;

        if(wic_factory == NULL) {
            WD_TRACE("wdLoadImageFromFile: Image API disabled.");
            return NULL;
        }

        hr = IWICImagingFactory_CreateDecoderFromFilename(wic_factory, pszPath,
                NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdLoadImageFromFile: "
                        "IWICImagingFactory::CreateDecoderFromFilename() failed.");
            goto err_CreateDecoderFromFilename;
        }

        hr = IWICBitmapDecoder_GetFrame(decoder, 0, &bitmap);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdLoadImageFromFile: "
                        "IWICBitmapDecoder::GetFrame() failed.");
            goto err_GetFrame;
        }

        converted_bitmap = wic_convert_bitmap((IWICBitmapSource*) bitmap);
        if(converted_bitmap == NULL)
            WD_TRACE("wdLoadImageFromFile: wic_convert_bitmap() failed.");

        IWICBitmapFrameDecode_Release(bitmap);
err_GetFrame:
        IWICBitmapDecoder_Release(decoder);
err_CreateDecoderFromFilename:
        return (WD_HIMAGE) converted_bitmap;
    } else {
        dummy_GpImage* img;
        int status;

        status = gdix_vtable->fn_LoadImageFromFile(pszPath, &img);
        if(status != 0) {
            WD_TRACE("wdLoadImageFromFile: "
                     "GdipLoadImageFromFile() failed. [%d]", status);
            return NULL;
        }

        return (WD_HIMAGE) img;
    }
}

WD_HIMAGE
wdLoadImageFromIStream(IStream* pStream)
{
    if(d2d_enabled()) {
        IWICBitmapDecoder* decoder;
        IWICBitmapFrameDecode* bitmap;
        IWICBitmapSource* converted_bitmap = NULL;
        HRESULT hr;

        if(wic_factory == NULL) {
            WD_TRACE("wdLoadImageFromIStream: Image API disabled.");
            return NULL;
        }

        hr = IWICImagingFactory_CreateDecoderFromStream(wic_factory, pStream,
                NULL, WICDecodeMetadataCacheOnLoad, &decoder);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdLoadImageFromIStream: "
                        "IWICImagingFactory::CreateDecoderFromFilename() failed.");
            goto err_CreateDecoderFromFilename;
        }

        hr = IWICBitmapDecoder_GetFrame(decoder, 0, &bitmap);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdLoadImageFromIStream: "
                        "IWICBitmapDecoder::GetFrame() failed.");
            goto err_GetFrame;
        }

        converted_bitmap = wic_convert_bitmap((IWICBitmapSource*) bitmap);
        if(converted_bitmap == NULL)
            WD_TRACE("wdLoadImageFromIStream: wic_convert_bitmap() failed.");

        IWICBitmapFrameDecode_Release(bitmap);
err_GetFrame:
        IWICBitmapDecoder_Release(decoder);
err_CreateDecoderFromFilename:
        return (WD_HIMAGE) converted_bitmap;
    } else {
        dummy_GpImage* img;
        int status;

        status = gdix_vtable->fn_LoadImageFromStream(pStream, &img);
        if(status != 0) {
            WD_TRACE("wdLoadImageFromIStream: "
                     "GdipLoadImageFromFile() failed. [%d]", status);
            return NULL;
        }

        return (WD_HIMAGE) img;
    }
}

WD_HIMAGE
wdLoadImageFromResource(HINSTANCE hInstance, const WCHAR* pszResType,
                        const WCHAR* pszResName)
{
    IStream* stream;
    WD_HIMAGE img;
    HRESULT hr;

    hr = memstream_create_from_resource(hInstance, pszResType, pszResName, &stream);
    if(FAILED(hr)) {
        WD_TRACE_HR("wdLoadImageFromResource: "
                 "memstream_create_from_resource() failed.");
        return NULL;
    }

    img = wdLoadImageFromIStream(stream);
    if(img == NULL)
        WD_TRACE("wdLoadImageFromResource: wdLoadImageFromIStream() failed.");

    IStream_Release(stream);
    return img;
}

void
wdDestroyImage(WD_HIMAGE hImage)
{
    if(d2d_enabled()) {
        IWICBitmapSource_Release((IWICBitmapSource*) hImage);
    } else {
        gdix_vtable->fn_DisposeImage((dummy_GpImage*) hImage);
    }
}

void
wdGetImageSize(WD_HIMAGE hImage, UINT* puWidth, UINT* puHeight)
{
    if(d2d_enabled()) {
        UINT w, h;

        IWICBitmapSource_GetSize((IWICBitmapSource*) hImage, &w, &h);
        if(puWidth != NULL)
            *puWidth = w;
        if(puHeight != NULL)
            *puHeight = h;
    } else {
        if(puWidth != NULL)
            gdix_vtable->fn_GetImageWidth((dummy_GpImage*) hImage, puWidth);
        if(puHeight != NULL)
            gdix_vtable->fn_GetImageHeight((dummy_GpImage*) hImage, puHeight);
    }
}


#define RAW_BUFFER_FLAG_BOTTOMUP            0x0001
#define RAW_BUFFER_FLAG_HASALPHA            0x0002
#define RAW_BUFFER_FLAG_PREMULTIPLYALPHA    0x0004

static void
raw_buffer_to_bitmap_data(UINT width, UINT height,
            BYTE* dst_buffer, int dst_stride, int dst_bytes_per_pixel,
            const BYTE* src_buffer, int src_stride, int src_bytes_per_pixel,
            int red_offset, int green_offset, int blue_offset, int alpha_offset,
            DWORD flags)
{
    UINT x, y;
    BYTE* dst_line = dst_buffer;
    BYTE* dst;
    const BYTE* src_line = src_buffer;
    const BYTE* src;

    if(src_stride == 0)
        src_stride = width * src_bytes_per_pixel;

    if(flags & RAW_BUFFER_FLAG_BOTTOMUP) {
        src_line = src_buffer + (height-1) * src_stride;
        src_stride = -src_stride;
    }

    for(y = 0; y < height; y++) {
        dst = dst_line;
        src = src_line;

        for(x = 0; x < width; x++) {
            dst[0] = src[blue_offset];
            dst[1] = src[green_offset];
            dst[2] = src[red_offset];

            if(dst_bytes_per_pixel >= 4) {
                dst[3] = (flags & RAW_BUFFER_FLAG_HASALPHA) ? src[alpha_offset] : 0xff;

                if(flags & RAW_BUFFER_FLAG_PREMULTIPLYALPHA) {
                    dst[0] = (dst[0] + dst[3]) / 255;
                    dst[1] = (dst[1] + dst[3]) / 255;
                    dst[2] = (dst[2] + dst[3]) / 255;
                }
            }

            dst += dst_bytes_per_pixel;
            src += src_bytes_per_pixel;
        }

        dst_line += dst_stride;
        src_line += src_stride;
    }
}

static void
colormap_buffer_to_bitmap_data(UINT width, UINT height,
            BYTE* dst_buffer, int dst_stride, int dst_bytes_per_pixel,
            const BYTE* src_buffer, int src_stride,
            const COLORREF* palette, UINT palette_size)
{
    UINT x, y;
    BYTE* dst_line = dst_buffer;
    BYTE* dst;
    const BYTE* src_line = src_buffer;
    const BYTE* src;

    if(src_stride == 0)
        src_stride = width;

    for(y = 0; y < height; y++) {
        dst = dst_line;
        src = src_line;

        for(x = 0; x < width; x++) {
            dst[0] = GetBValue(palette[*src]);
            dst[1] = GetGValue(palette[*src]);
            dst[2] = GetRValue(palette[*src]);

            if(dst_bytes_per_pixel >= 4)
                dst[3] = 0xff;

            dst += dst_bytes_per_pixel;
            src++;
        }

        dst_line += dst_stride;
        src_line += src_stride;
    }
}

WD_HIMAGE
wdCreateImageFromBuffer(UINT uWidth, UINT uHeight, UINT srcStride, const BYTE* pBuffer,
                        int pixelFormat, const COLORREF* cPalette, UINT uPaletteSize)
{
    if (d2d_enabled()) {
        IWICBitmap* bitmap = NULL;
        HRESULT hr;
        WICRect rect = { 0, 0, uWidth, uHeight};
        IWICBitmapLock *bitmap_lock = NULL;
        UINT cbBufferSize = 0;
        UINT dstStride = 0;
        BYTE *Scan0 = NULL;

        if(wic_factory == NULL) {
            WD_TRACE("wdCreateImageFromBuffer: Image API disabled.");
            return NULL;
        }

        /* wic_pixel_format is GUID_WICPixelFormat32bppPBGRA;
         * i.e. pre-multiplied alpha, BGRA order */
        hr = IWICImagingFactory_CreateBitmap(wic_factory, uWidth, uHeight,
                                &wic_pixel_format, WICBitmapCacheOnDemand, &bitmap);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateImageFromBuffer: "
                        "IWICImagingFactory::CreateBitmap() failed.");
            return NULL;
        }

        hr = IWICBitmap_Lock(bitmap, &rect, WICBitmapLockWrite, &bitmap_lock);
        if (FAILED(hr)) {
            WD_TRACE_HR("wdCreateImageFromBuffer: "
                        "IWICBitmap::Lock() failed.");
            IWICBitmap_Release(bitmap);
            return NULL;
        }

        IWICBitmapLock_GetStride(bitmap_lock, &dstStride);
        IWICBitmapLock_GetDataPointer(bitmap_lock, &cbBufferSize, &Scan0);

        switch(pixelFormat) {
            case WD_PIXELFORMAT_PALETTE:
                colormap_buffer_to_bitmap_data(uWidth, uHeight, Scan0, dstStride, 4,
                                pBuffer, srcStride, cPalette, uPaletteSize);
                break;

            case WD_PIXELFORMAT_R8G8B8:
                raw_buffer_to_bitmap_data(uWidth, uHeight, Scan0, dstStride, 4,
                                pBuffer, srcStride, 3, 0, 1, 2, 0, 0);
                break;

            case WD_PIXELFORMAT_R8G8B8A8:
                raw_buffer_to_bitmap_data(uWidth, uHeight, Scan0, dstStride, 4,
                                pBuffer, srcStride, 4, 0, 1, 2, 3, RAW_BUFFER_FLAG_HASALPHA | RAW_BUFFER_FLAG_PREMULTIPLYALPHA);
                break;

            case WD_PIXELFORMAT_B8G8R8A8:
                raw_buffer_to_bitmap_data(uWidth, uHeight, Scan0, dstStride, 4,
                                pBuffer, srcStride, 4, 2, 1, 0, 3, RAW_BUFFER_FLAG_HASALPHA | RAW_BUFFER_FLAG_BOTTOMUP);
                break;
        }

        IWICBitmapLock_Release(bitmap_lock);
        return (WD_HIMAGE)bitmap;
    }
    else {
        dummy_GpPixelFormat format;
        int status;
        dummy_GpBitmap *bitmap = NULL;
        dummy_GpBitmapData bitmapData;
        dummy_GpRectI rect = { 0, 0, uWidth, uHeight };

        if (pixelFormat == WD_PIXELFORMAT_R8G8B8 || pixelFormat == WD_PIXELFORMAT_PALETTE)
            format = dummy_PixelFormat24bppRGB;
        else if (pixelFormat == WD_PIXELFORMAT_R8G8B8A8)
            format = dummy_PixelFormat32bppARGB;
        else
            format = dummy_PixelFormat32bppPARGB;

        status = gdix_vtable->fn_CreateBitmapFromScan0(uWidth, uHeight, 0, format, NULL, &bitmap);
        if(status != 0) {
            WD_TRACE("wdCreateImageFromBuffer: "
                     "GdipCreateBitmapFromScan0() failed. [%d]", status);
            return NULL;
        }

        gdix_vtable->fn_BitmapLockBits(bitmap, &rect, dummy_ImageLockModeWrite, format, &bitmapData);

        switch(pixelFormat) {
            case WD_PIXELFORMAT_PALETTE:
                colormap_buffer_to_bitmap_data(uWidth, uHeight, (BYTE*)bitmapData.Scan0, bitmapData.Stride, 3,
                    pBuffer, srcStride, cPalette, uPaletteSize);
                break;

            case WD_PIXELFORMAT_R8G8B8:
                raw_buffer_to_bitmap_data(uWidth, uHeight, (BYTE*)bitmapData.Scan0, bitmapData.Stride, 3,
                                pBuffer, srcStride, 3, 0, 1, 2, 0, 0);
                break;

            case WD_PIXELFORMAT_R8G8B8A8:
                raw_buffer_to_bitmap_data(uWidth, uHeight, (BYTE*)bitmapData.Scan0, bitmapData.Stride, 4,
                                pBuffer, srcStride, 4, 0, 1, 2, 3, RAW_BUFFER_FLAG_HASALPHA);
                break;

            case WD_PIXELFORMAT_B8G8R8A8:
                raw_buffer_to_bitmap_data(uWidth, uHeight, (BYTE*)bitmapData.Scan0, bitmapData.Stride, 4,
                                pBuffer, srcStride, 4, 2, 1, 0, 3, RAW_BUFFER_FLAG_HASALPHA | RAW_BUFFER_FLAG_BOTTOMUP);
                break;
        }

        gdix_vtable->fn_BitmapUnlockBits(bitmap, &bitmapData);
        return (WD_HIMAGE)bitmap;
    }
}
