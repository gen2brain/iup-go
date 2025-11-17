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
#include "backend-gdix.h"


WD_HBRUSH
wdCreateSolidBrush(WD_HCANVAS hCanvas, WD_COLOR color)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_ID2D1SolidColorBrush* b;
        dummy_D2D1_COLOR_F clr;
        HRESULT hr;

        d2d_init_color(&clr, color);
        hr = dummy_ID2D1RenderTarget_CreateSolidColorBrush(
                        c->target, &clr, NULL, &b);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateSolidBrush: "
                        "ID2D1RenderTarget::CreateSolidColorBrush() failed.");
            return NULL;
        }
        return (WD_HBRUSH) b;
    } else {
        dummy_GpSolidFill* b;
        int status;

        status = gdix_vtable->fn_CreateSolidFill(color, &b);
        if(status != 0) {
            WD_TRACE("wdCreateSolidBrush: "
                     "GdipCreateSolidFill() failed. [%d]", status);
            return NULL;
        }
        return (WD_HBRUSH) b;
    }
}

void
wdDestroyBrush(WD_HBRUSH hBrush)
{
    if(d2d_enabled()) {
        dummy_ID2D1Brush_Release((dummy_ID2D1Brush*) hBrush);
    } else {
        gdix_vtable->fn_DeleteBrush((void*) hBrush);
    }
}

void
wdSetSolidBrushColor(WD_HBRUSH hBrush, WD_COLOR color)
{
    if(d2d_enabled()) {
        dummy_D2D1_COLOR_F clr;

        d2d_init_color(&clr, color);
        dummy_ID2D1SolidColorBrush_SetColor((dummy_ID2D1SolidColorBrush*) hBrush, &clr);
    } else {
        dummy_GpSolidFill* b = (dummy_GpSolidFill*) hBrush;

        gdix_vtable->fn_SetSolidFillColor(b, (dummy_ARGB) color);
    }
}

WD_HBRUSH
wdCreateLinearGradientBrush(WD_HCANVAS hCanvas, float x0, float y0, float x1, float y1, WD_COLOR color0, WD_COLOR color1)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_ID2D1LinearGradientBrush* b;
        dummy_ID2D1GradientStopCollection* stopCollection;
        dummy_D2D1_GRADIENT_STOP stops[2];
        dummy_D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props;
        HRESULT hr;

        /* Create gradient stops */
        stops[0].position = 0.0f;
        d2d_init_color(&stops[0].color, color0);
        stops[1].position = 1.0f;
        d2d_init_color(&stops[1].color, color1);

        /* Create gradient stop collection */
        hr = dummy_ID2D1RenderTarget_CreateGradientStopCollection(c->target, stops, 2,
                    0 /* D2D1_GAMMA_2_2 */, 0 /* D2D1_EXTEND_MODE_CLAMP */, &stopCollection);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateLinearGradientBrush: "
                        "ID2D1RenderTarget::CreateGradientStopCollection() failed.");
            return NULL;
        }

        /* Create linear gradient brush */
        props.startPoint.x = x0;
        props.startPoint.y = y0;
        props.endPoint.x = x1;
        props.endPoint.y = y1;

        hr = dummy_ID2D1RenderTarget_CreateLinearGradientBrush(c->target, &props, NULL, stopCollection, &b);
        dummy_ID2D1GradientStopCollection_Release(stopCollection);

        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateLinearGradientBrush: "
                        "ID2D1RenderTarget::CreateLinearGradientBrush() failed.");
            return NULL;
        }

        return (WD_HBRUSH) b;
    } else {
        /* GDI+ backend doesn't support gradient brushes in WDL vtable */
        /* Return NULL - caller will fall back to manual gradient */
        return NULL;
    }
}

WD_HBRUSH
wdCreateRadialGradientBrush(WD_HCANVAS hCanvas, float cx, float cy, float rx, float ry, WD_COLOR colorCenter, WD_COLOR colorEdge)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_ID2D1RadialGradientBrush* b;
        dummy_ID2D1GradientStopCollection* stopCollection;
        dummy_D2D1_GRADIENT_STOP stops[2];
        dummy_D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES props;
        HRESULT hr;

        /* Create gradient stops */
        stops[0].position = 0.0f;
        d2d_init_color(&stops[0].color, colorCenter);
        stops[1].position = 1.0f;
        d2d_init_color(&stops[1].color, colorEdge);

        /* Create gradient stop collection */
        hr = dummy_ID2D1RenderTarget_CreateGradientStopCollection(c->target, stops, 2,
                    0 /* D2D1_GAMMA_2_2 */, 0 /* D2D1_EXTEND_MODE_CLAMP */, &stopCollection);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateRadialGradientBrush: "
                        "ID2D1RenderTarget::CreateGradientStopCollection() failed.");
            return NULL;
        }

        /* Create radial gradient brush */
        props.center.x = cx;
        props.center.y = cy;
        props.gradientOriginOffset.x = 0.0f;
        props.gradientOriginOffset.y = 0.0f;
        props.radiusX = rx;
        props.radiusY = ry;

        hr = dummy_ID2D1RenderTarget_CreateRadialGradientBrush(c->target, &props, NULL, stopCollection, &b);
        dummy_ID2D1GradientStopCollection_Release(stopCollection);

        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateRadialGradientBrush: "
                        "ID2D1RenderTarget::CreateRadialGradientBrush() failed.");
            return NULL;
        }

        return (WD_HBRUSH) b;
    } else {
        /* GDI+ backend doesn't support gradient brushes in WDL vtable */
        /* Return NULL - caller will fall back to manual gradient */
        return NULL;
    }
}
