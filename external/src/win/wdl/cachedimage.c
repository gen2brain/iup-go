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
#include "backend-d2d.h"
#include "backend-wic.h"
#include "backend-gdix.h"


WD_HCACHEDIMAGE
wdCreateCachedImage(WD_HCANVAS hCanvas, WD_HIMAGE hImage)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_ID2D1Bitmap* b;
        HRESULT hr;

        hr = dummy_ID2D1RenderTarget_CreateBitmapFromWicBitmap(c->target,
                (IWICBitmapSource*) hImage, NULL, &b);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateCachedImage: "
                        "ID2D1RenderTarget::CreateBitmapFromWicBitmap() failed.");
            return NULL;
        }

        return (WD_HCACHEDIMAGE) b;
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        dummy_GpCachedBitmap* cb;
        int status;

        status = gdix_vtable->fn_CreateCachedBitmap((dummy_GpImage*) hImage,
                c->graphics, &cb);
        if(status != 0) {
            WD_TRACE("wdCreateCachedImage: "
                     "GdipCreateCachedBitmap() failed. [%d]", status);
            return NULL;
        }

        return (WD_HCACHEDIMAGE) cb;
    }
}

void
wdDestroyCachedImage(WD_HCACHEDIMAGE hCachedImage)
{
    if(d2d_enabled()) {
        dummy_ID2D1Bitmap_Release((dummy_ID2D1Bitmap*) hCachedImage);
    } else {
        gdix_vtable->fn_DeleteCachedBitmap((dummy_GpCachedBitmap*) hCachedImage);
    }
}
