/*
 * WinDrawLib
 * Copyright (c) 2015-2017 Martin Mitas
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

#include "backend-d2d.h"
#include "lock.h"


static HMODULE d2d_dll = NULL;

dummy_ID2D1Factory* d2d_factory = NULL;


static inline void
d2d_matrix_mult(dummy_D2D1_MATRIX_3X2_F* res,
                const dummy_D2D1_MATRIX_3X2_F* a, const dummy_D2D1_MATRIX_3X2_F* b)
{
    res->_11 = a->_11 * b->_11 + a->_12 * b->_21;
    res->_12 = a->_11 * b->_12 + a->_12 * b->_22;
    res->_21 = a->_21 * b->_11 + a->_22 * b->_21;
    res->_22 = a->_21 * b->_12 + a->_22 * b->_22;
    res->_31 = a->_31 * b->_11 + a->_32 * b->_21 + b->_31;
    res->_32 = a->_31 * b->_12 + a->_32 * b->_22 + b->_32;
}

int
d2d_init(void)
{
    static const dummy_D2D1_FACTORY_OPTIONS factory_options = { dummy_D2D1_DEBUG_LEVEL_NONE };
    HRESULT (WINAPI* fn_D2D1CreateFactory)(dummy_D2D1_FACTORY_TYPE, REFIID, const dummy_D2D1_FACTORY_OPTIONS*, void**);
    HRESULT hr;

    /* Load D2D1.DLL. */
    d2d_dll = wd_load_system_dll(_T("D2D1.DLL"));
    if(d2d_dll == NULL) {
        WD_TRACE_ERR("d2d_init: wd_load_system_dll(D2D1.DLL) failed.");
        goto err_LoadLibrary;
    }

    fn_D2D1CreateFactory = (HRESULT (WINAPI*)(dummy_D2D1_FACTORY_TYPE, REFIID, const dummy_D2D1_FACTORY_OPTIONS*, void**))
                GetProcAddress(d2d_dll, "D2D1CreateFactory");
    if(fn_D2D1CreateFactory == NULL) {
        WD_TRACE_ERR("d2d_init: GetProcAddress(D2D1CreateFactory) failed.");
        goto err_GetProcAddress;
    }

    /* Create D2D factory object. Note we use D2D1_FACTORY_TYPE_SINGLE_THREADED
     * for performance reasons and manually synchronize calls to the factory.
     * This still allows usage in multi-threading environment but all the
     * created resources can only be used from the respective threads where
     * they were created. */
    hr = fn_D2D1CreateFactory(dummy_D2D1_FACTORY_TYPE_SINGLE_THREADED,
                &dummy_IID_ID2D1Factory, &factory_options, (void**) &d2d_factory);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_init: D2D1CreateFactory() failed.");
        goto err_CreateFactory;
    }

    /* Success */
    return 0;

    /* Error path unwinding */
err_CreateFactory:
err_GetProcAddress:
    FreeLibrary(d2d_dll);
    d2d_dll = NULL;
err_LoadLibrary:
    return -1;
}

void
d2d_fini(void)
{
    dummy_ID2D1Factory_Release(d2d_factory);
    FreeLibrary(d2d_dll);
    d2d_dll = NULL;
}

d2d_canvas_t*
d2d_canvas_alloc(dummy_ID2D1RenderTarget* target, WORD type, UINT width, BOOL rtl)
{
    d2d_canvas_t* c;

    c = (d2d_canvas_t*) malloc(sizeof(d2d_canvas_t));
    if(c == NULL) {
        WD_TRACE("d2d_canvas_alloc: malloc() failed.");
        return NULL;
    }

    memset(c, 0, sizeof(d2d_canvas_t));

    c->type = type;
    c->flags = (rtl ? D2D_CANVASFLAG_RTL : 0);
    c->width = width;
    c->target = target;

    /* We use raw pixels as units. D2D by default works with DIPs ("device
     * independent pixels"), which map 1:1 to physical pixels when DPI is 96.
     * So we enforce the render target to think we have this DPI. */
    dummy_ID2D1RenderTarget_SetDpi(c->target, 96.0f, 96.0f);

    d2d_reset_transform(c);

    return c;
}

void
d2d_reset_clip(d2d_canvas_t* c)
{
    if(c->clip_layer != NULL) {
        dummy_ID2D1RenderTarget_PopLayer(c->target);
        dummy_ID2D1Layer_Release(c->clip_layer);
        c->clip_layer = NULL;
    }
    if(c->flags & D2D_CANVASFLAG_RECTCLIP) {
        dummy_ID2D1RenderTarget_PopAxisAlignedClip(c->target);
        c->flags &= ~D2D_CANVASFLAG_RECTCLIP;
    }
}

void
d2d_reset_transform(d2d_canvas_t* c)
{
    dummy_D2D1_MATRIX_3X2_F m;

    if(c->flags & D2D_CANVASFLAG_RTL) {
        m._11 = -1.0f;  m._12 = 0.0f;
        m._21 = 0.0f;   m._22 = 1.0f;
        m._31 = (float)c->width - 1.0f + D2D_BASEDELTA_X;
        m._32 = D2D_BASEDELTA_Y;
    } else {
        m._11 = 1.0f;   m._12 = 0.0f;
        m._21 = 0.0f;   m._22 = 1.0f;
        m._31 = D2D_BASEDELTA_X;
        m._32 = D2D_BASEDELTA_Y;
    }

    dummy_ID2D1RenderTarget_SetTransform(c->target, &m);
}

void
d2d_apply_transform(d2d_canvas_t* c, const dummy_D2D1_MATRIX_3X2_F* matrix)
{
    dummy_D2D1_MATRIX_3X2_F res;
    dummy_D2D1_MATRIX_3X2_F old_matrix;

    dummy_ID2D1RenderTarget_GetTransform(c->target, &old_matrix);
    d2d_matrix_mult(&res, matrix, &old_matrix);
    dummy_ID2D1RenderTarget_SetTransform(c->target, &res);
}

void
d2d_disable_rtl_transform(d2d_canvas_t* c, dummy_D2D1_MATRIX_3X2_F* old_matrix)
{
    dummy_D2D1_MATRIX_3X2_F r;    /* Reflection + transition for WD_CANVAS_LAYOUTRTL. */
    dummy_D2D1_MATRIX_3X2_F ur;   /* R * user's transformation. */
    dummy_D2D1_MATRIX_3X2_F u;    /* Only user's transformation. */

    r._11 = -1.0f;				r._12 = 0.0f;
    r._21 = 0.0f;				r._22 = 1.0f;
    r._31 = (float) c->width;	r._32 = 0.0f;

    dummy_ID2D1RenderTarget_GetTransform(c->target, &ur);
    if(old_matrix != NULL)
        memcpy(old_matrix, &ur, sizeof(dummy_D2D1_MATRIX_3X2_F));
    ur._31 += D2D_BASEDELTA_X;
    ur._32 -= D2D_BASEDELTA_Y;

    /* Note R is inverse to itself. */
    d2d_matrix_mult(&u, &ur, &r);

    dummy_ID2D1RenderTarget_SetTransform(c->target, &u);
}

void
d2d_setup_arc_segment(dummy_D2D1_ARC_SEGMENT* arc_seg, float cx, float cy, float rx, float ry,
                      float base_angle, float sweep_angle)
{
    float sweep_rads = (base_angle + sweep_angle) * (WD_PI / 180.0f);

    arc_seg->point.x = cx + rx * cosf(sweep_rads);
    arc_seg->point.y = cy + ry * sinf(sweep_rads);
    arc_seg->size.width = rx;
    arc_seg->size.height = ry;
    arc_seg->rotationAngle = 0.0f;

    if(sweep_angle >= 0.0f)
        arc_seg->sweepDirection = dummy_D2D1_SWEEP_DIRECTION_CLOCKWISE;
    else
        arc_seg->sweepDirection = dummy_D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

    if(sweep_angle >= 180.0f)
        arc_seg->arcSize = dummy_D2D1_ARC_SIZE_LARGE;
    else
        arc_seg->arcSize = dummy_D2D1_ARC_SIZE_SMALL;
}

dummy_ID2D1Geometry*
d2d_create_arc_geometry(float cx, float cy, float rx, float ry,
                        float base_angle, float sweep_angle, BOOL pie)
{
    dummy_ID2D1PathGeometry* g = NULL;
    dummy_ID2D1GeometrySink* s;
    HRESULT hr;
    float base_rads = base_angle * (WD_PI / 180.0f);
    dummy_D2D1_POINT_2F pt;
    dummy_D2D1_ARC_SEGMENT arc_seg;

    wd_lock();
    hr = dummy_ID2D1Factory_CreatePathGeometry(d2d_factory, &g);
    wd_unlock();
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_create_arc_geometry: "
                    "ID2D1Factory::CreatePathGeometry() failed.");
        return NULL;
    }
    hr = dummy_ID2D1PathGeometry_Open(g, &s);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_create_arc_geometry: ID2D1PathGeometry::Open() failed.");
        dummy_ID2D1PathGeometry_Release(g);
        return NULL;
    }

    pt.x = cx + rx * cosf(base_rads);
    pt.y = cy + ry * sinf(base_rads);
    dummy_ID2D1GeometrySink_BeginFigure(s, pt, dummy_D2D1_FIGURE_BEGIN_FILLED);

    d2d_setup_arc_segment(&arc_seg, cx, cy, rx, ry, base_angle, sweep_angle);
    dummy_ID2D1GeometrySink_AddArc(s, &arc_seg);

    if(pie) {
        pt.x = cx;
        pt.y = cy;
        dummy_ID2D1GeometrySink_AddLine(s, pt);
        dummy_ID2D1GeometrySink_EndFigure(s, dummy_D2D1_FIGURE_END_CLOSED);
    } else {
        dummy_ID2D1GeometrySink_EndFigure(s, dummy_D2D1_FIGURE_END_OPEN);
    }

    dummy_ID2D1GeometrySink_Close(s);
    dummy_ID2D1GeometrySink_Release(s);

    return (dummy_ID2D1Geometry*) g;
}
