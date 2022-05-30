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
#include "lock.h"


WD_HCANVAS
wdCreateCanvasWithPaintStruct(HWND hWnd, PAINTSTRUCT* pPS, DWORD dwFlags)
{
    RECT rect;

    GetClientRect(hWnd, &rect);

    if(d2d_enabled()) {
        dummy_D2D1_RENDER_TARGET_PROPERTIES props = {
            dummy_D2D1_RENDER_TARGET_TYPE_DEFAULT,
            { dummy_DXGI_FORMAT_B8G8R8A8_UNORM, dummy_D2D1_ALPHA_MODE_PREMULTIPLIED },
            0.0f, 0.0f,
            ((dwFlags & WD_CANVAS_NOGDICOMPAT) ?
                        0 : dummy_D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE),
            dummy_D2D1_FEATURE_LEVEL_DEFAULT
        };
        dummy_D2D1_HWND_RENDER_TARGET_PROPERTIES props2;
        d2d_canvas_t* c;
        dummy_ID2D1HwndRenderTarget* target;
        HRESULT hr;

        props2.hwnd = hWnd;
        props2.pixelSize.width = rect.right - rect.left;
        props2.pixelSize.height = rect.bottom - rect.top;
        props2.presentOptions = dummy_D2D1_PRESENT_OPTIONS_NONE;

        wd_lock();
        /* Note ID2D1HwndRenderTarget is implicitly double-buffered. */
        hr = dummy_ID2D1Factory_CreateHwndRenderTarget(d2d_factory, &props, &props2, &target);
        wd_unlock();
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateCanvasWithPaintStruct: "
                        "ID2D1Factory::CreateHwndRenderTarget() failed.");
            return NULL;
        }

        c = d2d_canvas_alloc((dummy_ID2D1RenderTarget*)target, D2D_CANVASTYPE_HWND,
                    rect.right, (dwFlags & WD_CANVAS_LAYOUTRTL));
        if(c == NULL) {
            WD_TRACE("wdCreateCanvasWithPaintStruct: d2d_canvas_alloc() failed.");
            dummy_ID2D1RenderTarget_Release((dummy_ID2D1RenderTarget*)target);
            return NULL;
        }

        /* make sure text anti-aliasing is clear type */
        dummy_ID2D1RenderTarget_SetTextAntialiasMode(c->target, dummy_D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

        return (WD_HCANVAS) c;
    } else {
        BOOL use_doublebuffer = (dwFlags & WD_CANVAS_DOUBLEBUFFER);
        gdix_canvas_t* c;

        c = gdix_canvas_alloc(pPS->hdc, (use_doublebuffer ? &pPS->rcPaint : NULL),
                    rect.right, (dwFlags & WD_CANVAS_LAYOUTRTL));
        if(c == NULL) {
            WD_TRACE("wdCreateCanvasWithPaintStruct: gdix_canvas_alloc() failed.");
            return NULL;
        }
        return (WD_HCANVAS) c;
    }
}

WD_HCANVAS
wdCreateCanvasWithHDC(HDC hDC, const RECT* pRect, DWORD dwFlags)
{
    if(d2d_enabled()) {
        dummy_D2D1_RENDER_TARGET_PROPERTIES props = {
            dummy_D2D1_RENDER_TARGET_TYPE_DEFAULT,
            { dummy_DXGI_FORMAT_B8G8R8A8_UNORM, dummy_D2D1_ALPHA_MODE_PREMULTIPLIED },
            0.0f, 0.0f,
            ((dwFlags & WD_CANVAS_NOGDICOMPAT) ?
                        0 : dummy_D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE),
            dummy_D2D1_FEATURE_LEVEL_DEFAULT
        };
        d2d_canvas_t* c;
        dummy_ID2D1DCRenderTarget* target;
        HRESULT hr;

        wd_lock();
        hr = dummy_ID2D1Factory_CreateDCRenderTarget(d2d_factory, &props, &target);
        wd_unlock();
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateCanvasWithHDC: "
                        "ID2D1Factory::CreateDCRenderTarget() failed.");
            goto err_CreateDCRenderTarget;
        }

        hr = dummy_ID2D1DCRenderTarget_BindDC(target, hDC, pRect);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdCreateCanvasWithHDC: "
                        "ID2D1Factory::BindDC() failed.");
            goto err_BindDC;
        }

        c = d2d_canvas_alloc((dummy_ID2D1RenderTarget*)target, D2D_CANVASTYPE_DC,
                pRect->right - pRect->left, (dwFlags & WD_CANVAS_LAYOUTRTL));
        if(c == NULL) {
            WD_TRACE("wdCreateCanvasWithHDC: d2d_canvas_alloc() failed.");
            goto err_d2d_canvas_alloc;
        }

        /* make sure text anti-aliasing is clear type */
        dummy_ID2D1RenderTarget_SetTextAntialiasMode(c->target, dummy_D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

        return (WD_HCANVAS) c;

err_d2d_canvas_alloc:
err_BindDC:
        dummy_ID2D1RenderTarget_Release((dummy_ID2D1RenderTarget*)target);
err_CreateDCRenderTarget:
        return NULL;
    } else {
        BOOL use_doublebuffer = (dwFlags & WD_CANVAS_DOUBLEBUFFER);
        gdix_canvas_t* c;

        c = gdix_canvas_alloc(hDC, (use_doublebuffer ? pRect : NULL),
                pRect->right - pRect->left, (dwFlags & WD_CANVAS_LAYOUTRTL));
        if(c == NULL) {
            WD_TRACE("wdCreateCanvasWithHDC: gdix_canvas_alloc() failed.");
            return NULL;
        }
        return (WD_HCANVAS) c;
    }
}

void
wdDestroyCanvas(WD_HCANVAS hCanvas)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;

        /* Check for common logical errors. */
        if(c->clip_layer != NULL  ||  (c->flags & D2D_CANVASFLAG_RECTCLIP))
            WD_TRACE("wdDestroyCanvas: Logical error: Canvas has dangling clip.");
        if(c->gdi_interop != NULL)
            WD_TRACE("wdDestroyCanvas: Logical error: Unpaired wdStartGdi()/wdEndGdi().");

        dummy_ID2D1RenderTarget_Release(c->target);
        free(c);
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;

        gdix_vtable->fn_DeleteStringFormat(c->string_format);
        gdix_vtable->fn_DeletePen(c->pen);
        gdix_vtable->fn_DeleteGraphics(c->graphics);

        if(c->real_dc != NULL) {
            HBITMAP mem_bmp;

            mem_bmp = SelectObject(c->dc, c->orig_bmp);
            DeleteObject(mem_bmp);
            DeleteObject(c->dc);
        }

        free(c);
    }
}

void
wdBeginPaint(WD_HCANVAS hCanvas)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_ID2D1RenderTarget_BeginDraw(c->target);
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        SetLayout(c->dc, 0);
    }
}

BOOL
wdEndPaint(WD_HCANVAS hCanvas)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        HRESULT hr;

        d2d_reset_clip(c);

        hr = dummy_ID2D1RenderTarget_EndDraw(c->target, NULL, NULL);
        if(FAILED(hr)) {
            if(hr != D2DERR_RECREATE_TARGET)
                WD_TRACE_HR("wdEndPaint: ID2D1RenderTarget::EndDraw() failed.");
            return FALSE;
        }
        return TRUE;
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;

        /* If double-buffering, blit the memory DC to the display DC. */
        if(c->real_dc != NULL)
            BitBlt(c->real_dc, c->x, c->y, c->cx, c->cy, c->dc, 0, 0, SRCCOPY);

        SetLayout(c->real_dc, c->dc_layout);

        /* For GDI+, disable caching. */
        return FALSE;
    }
}

BOOL
wdResizeCanvas(WD_HCANVAS hCanvas, UINT uWidth, UINT uHeight)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        if(c->type == D2D_CANVASTYPE_HWND) {
            dummy_D2D1_SIZE_U size = { uWidth, uHeight };
            HRESULT hr;

            hr = dummy_ID2D1HwndRenderTarget_Resize(c->hwnd_target, &size);
            if(FAILED(hr)) {
                WD_TRACE_HR("wdResizeCanvas: "
                            "ID2D1HwndRenderTarget_Resize() failed.");
                return FALSE;
            }

            /* In RTL mode, we have to update the transformation matrix
             * accordingly. */
            if(c->flags & D2D_CANVASFLAG_RTL) {
                dummy_D2D1_MATRIX_3X2_F m;

                dummy_ID2D1RenderTarget_GetTransform(c->target, &m);
                m._31 = m._11 * (float)(uWidth - c->width);
                m._32 = m._12 * (float)(uWidth - c->width);
                dummy_ID2D1RenderTarget_SetTransform(c->target, &m);

                c->width = uWidth;
            }
            return TRUE;
        } else {
            /* Operation not supported. */
            WD_TRACE("wdResizeCanvas: Not supported (not ID2D1HwndRenderTarget).");
            return FALSE;
        }
    } else {
        /* Actually we should never be here as GDI+ back-end never allows
         * caching of the canvas so there is no need to ever resize it. */
        WD_TRACE("wdResizeCanvas: Not supported (GDI+ back-end).");
        return FALSE;
    }
}

HDC
wdStartGdi(WD_HCANVAS hCanvas, BOOL bKeepContents)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_ID2D1GdiInteropRenderTarget* gdi_interop;
        dummy_D2D1_DC_INITIALIZE_MODE init_mode;
        HRESULT hr;
        HDC dc;

        hr = dummy_ID2D1RenderTarget_QueryInterface(c->target,
                    &dummy_IID_ID2D1GdiInteropRenderTarget, (void**) &gdi_interop);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdStartGdi: ID2D1RenderTarget::"
                        "QueryInterface(IID_ID2D1GdiInteropRenderTarget) failed.");
            return NULL;
        }

        init_mode = (bKeepContents ? dummy_D2D1_DC_INITIALIZE_MODE_COPY
                        : dummy_D2D1_DC_INITIALIZE_MODE_CLEAR);
        hr = dummy_ID2D1GdiInteropRenderTarget_GetDC(gdi_interop, init_mode, &dc);
        if(FAILED(hr)) {
            WD_TRACE_HR("wdStartGdi: ID2D1GdiInteropRenderTarget::GetDC() failed.");
            dummy_ID2D1GdiInteropRenderTarget_Release(gdi_interop);
            return NULL;
        }

        c->gdi_interop = gdi_interop;

        if(c->flags & D2D_CANVASFLAG_RTL)
            SetLayout(dc, LAYOUT_RTL);

        return dc;
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        int status;
        HDC dc;

        status = gdix_vtable->fn_GetDC(c->graphics, &dc);
        if(status != 0) {
            WD_TRACE_ERR_("wdStartGdi: GdipGetDC() failed.", status);
            return NULL;
        }

        SetLayout(dc, c->dc_layout);

        if(c->dc_layout & LAYOUT_RTL)
            SetViewportOrgEx(dc, c->x + c->cx - (c->width-1), -c->y, NULL);

        return dc;
    }
}

void
wdEndGdi(WD_HCANVAS hCanvas, HDC hDC)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;

        dummy_ID2D1GdiInteropRenderTarget_ReleaseDC(c->gdi_interop, NULL);
        dummy_ID2D1GdiInteropRenderTarget_Release(c->gdi_interop);
        c->gdi_interop = NULL;
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;

        if(c->rtl)
            SetLayout(hDC, 0);
        gdix_vtable->fn_ReleaseDC(c->graphics, hDC);
    }
}

void
wdClear(WD_HCANVAS hCanvas, WD_COLOR color)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_D2D1_COLOR_F clr;

        d2d_init_color(&clr, color);
        dummy_ID2D1RenderTarget_Clear(c->target, &clr);
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        gdix_vtable->fn_GraphicsClear(c->graphics, color);
    }
}

void
wdSetClip(WD_HCANVAS hCanvas, const WD_RECT* pRect, const WD_HPATH hPath)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;

        d2d_reset_clip(c);

        if(hPath != NULL) {
            dummy_ID2D1PathGeometry* g = (dummy_ID2D1PathGeometry*) hPath;
            dummy_D2D1_LAYER_PARAMETERS layer_params;
            HRESULT hr;

            hr = dummy_ID2D1RenderTarget_CreateLayer(c->target, NULL, &c->clip_layer);
            if(FAILED(hr)) {
                WD_TRACE_HR("wdSetClip: ID2D1RenderTarget::CreateLayer() failed.");
                return;
            }

            if(pRect != NULL) {
                layer_params.contentBounds.left = pRect->x0;
                layer_params.contentBounds.top = pRect->y0;
                layer_params.contentBounds.right = pRect->x1;
                layer_params.contentBounds.bottom = pRect->y1;
            } else {
                layer_params.contentBounds.left = FLT_MIN;
                layer_params.contentBounds.top = FLT_MIN;
                layer_params.contentBounds.right = FLT_MAX;
                layer_params.contentBounds.bottom = FLT_MAX;
            }
            layer_params.geometricMask = (dummy_ID2D1Geometry*) g;
            layer_params.maskAntialiasMode = dummy_D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
            layer_params.maskTransform._11 = 1.0f;
            layer_params.maskTransform._12 = 0.0f;
            layer_params.maskTransform._21 = 0.0f;
            layer_params.maskTransform._22 = 1.0f;
            layer_params.maskTransform._31 = 0.0f;
            layer_params.maskTransform._32 = 0.0f;
            layer_params.opacity = 1.0f;
            layer_params.opacityBrush = NULL;
            layer_params.layerOptions = dummy_D2D1_LAYER_OPTIONS_NONE;

            dummy_ID2D1RenderTarget_PushLayer(c->target, &layer_params, c->clip_layer);
        } else if(pRect != NULL) {
            dummy_ID2D1RenderTarget_PushAxisAlignedClip(c->target,
                    (const dummy_D2D1_RECT_F*) pRect, dummy_D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            c->flags |= D2D_CANVASFLAG_RECTCLIP;
        }
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        int mode;

        if(pRect == NULL  &&  hPath == NULL) {
            gdix_vtable->fn_ResetClip(c->graphics);
            return;
        }

        mode = dummy_CombineModeReplace;

        if(pRect != NULL) {
            gdix_vtable->fn_SetClipRect(c->graphics, pRect->x0, pRect->y0,
                             pRect->x1, pRect->y1, mode);
            mode = dummy_CombineModeIntersect;
        }

        if(hPath != NULL)
            gdix_vtable->fn_SetClipPath(c->graphics, (void*) hPath, mode);
    }
}

void
wdRotateWorld(WD_HCANVAS hCanvas, float cx, float cy, float fAngle)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_D2D1_MATRIX_3X2_F m;
        float a_rads = fAngle * (WD_PI / 180.0f);
        float a_sin = sinf(a_rads);
        float a_cos = cosf(a_rads);

        m._11 = a_cos;  m._12 = a_sin;
        m._21 = -a_sin; m._22 = a_cos;
        m._31 = cx - cx*a_cos + cy*a_sin;
        m._32 = cy - cx*a_sin - cy*a_cos;
        d2d_apply_transform(c, &m);
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;

        gdix_vtable->fn_TranslateWorldTransform(c->graphics, cx, cy, dummy_MatrixOrderPrepend);
        gdix_vtable->fn_RotateWorldTransform(c->graphics, fAngle, dummy_MatrixOrderPrepend);
        gdix_vtable->fn_TranslateWorldTransform(c->graphics, -cx, -cy, dummy_MatrixOrderPrepend);
    }
}

void
wdTranslateWorld(WD_HCANVAS hCanvas, float dx, float dy)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        dummy_D2D1_MATRIX_3X2_F m;

        dummy_ID2D1RenderTarget_GetTransform(c->target, &m);
        m._31 += dx;
        m._32 += dy;
        dummy_ID2D1RenderTarget_SetTransform(c->target, &m);
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        gdix_vtable->fn_TranslateWorldTransform(c->graphics, dx, dy, dummy_MatrixOrderAppend);
    }
}

void
wdResetWorld(WD_HCANVAS hCanvas)
{
    if(d2d_enabled()) {
        d2d_canvas_t* c = (d2d_canvas_t*) hCanvas;
        d2d_reset_transform(c);
    } else {
        gdix_canvas_t* c = (gdix_canvas_t*) hCanvas;
        gdix_reset_transform(c);
    }
}

