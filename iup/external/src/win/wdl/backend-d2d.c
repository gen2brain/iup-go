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

#include <d3d11.h>


static HMODULE d2d_dll = NULL;

dummy_ID2D1Factory* d2d_factory = NULL;

static const GUID dummy_IID_IDXGIDevice =
        {0x54ec77fa,0x1377,0x44e6,{0x8c,0x32,0x88,0xfd,0x5f,0x44,0xc8,0x4c}};

static const GUID dummy_IID_IDXGIFactory2 =
        {0x50c83a1c,0xe072,0x4c48,{0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0}};

static const GUID dummy_IID_IDXGISurface =
        {0xcafcb56c,0x6ac3,0x4889,{0xbf,0x47,0x9e,0x23,0xbb,0xd2,0x60,0xec}};

static HMODULE d3d11_dll = NULL;
static ID3D11Device* d2d_d3d_device = NULL;
static IDXGIFactory2* d2d_dxgi_factory = NULL;
static dummy_ID2D1Device* d2d_device = NULL;
static UINT d2d_device_generation = 0;
static BOOL d2d_device_unsupported = FALSE;


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

static void
d2d_device_release(void)
{
    if(d2d_device != NULL) {
        dummy_ID2D1Device_Release(d2d_device);
        d2d_device = NULL;
    }
    if(d2d_dxgi_factory != NULL) {
        d2d_dxgi_factory->lpVtbl->Release(d2d_dxgi_factory);
        d2d_dxgi_factory = NULL;
    }
    if(d2d_d3d_device != NULL) {
        d2d_d3d_device->lpVtbl->Release(d2d_d3d_device);
        d2d_d3d_device = NULL;
    }
}

static BOOL
d2d_device_create(void)
{
    PFN_D3D11_CREATE_DEVICE fn_D3D11CreateDevice;
    dummy_ID2D1Factory1* factory1 = NULL;
    IDXGIDevice* dxgi_device = NULL;
    IDXGIAdapter* adapter = NULL;
    HRESULT hr;

    if(d2d_device != NULL)
        return TRUE;
    if(d2d_device_unsupported)
        return FALSE;

    if(d3d11_dll == NULL) {
        d3d11_dll = wd_load_system_dll(_T("D3D11.DLL"));
        if(d3d11_dll == NULL)
            goto err;
    }

    fn_D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE) GetProcAddress(d3d11_dll, "D3D11CreateDevice");
    if(fn_D3D11CreateDevice == NULL)
        goto err;

    hr = fn_D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                NULL, 0, D3D11_SDK_VERSION, &d2d_d3d_device, NULL, NULL);
    if(FAILED(hr))
        hr = fn_D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    NULL, 0, D3D11_SDK_VERSION, &d2d_d3d_device, NULL, NULL);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_device_create: D3D11CreateDevice() failed.");
        goto err;
    }

    hr = d2d_d3d_device->lpVtbl->QueryInterface(d2d_d3d_device, &dummy_IID_IDXGIDevice, (void**) &dxgi_device);
    if(FAILED(hr))
        goto err;
    hr = dxgi_device->lpVtbl->GetAdapter(dxgi_device, &adapter);
    if(FAILED(hr))
        goto err;
    hr = adapter->lpVtbl->GetParent(adapter, &dummy_IID_IDXGIFactory2, (void**) &d2d_dxgi_factory);
    if(FAILED(hr))
        goto err;

    hr = dummy_ID2D1Factory_QueryInterface(d2d_factory, &dummy_IID_ID2D1Factory1, (void**) &factory1);
    if(FAILED(hr))
        goto err;
    hr = dummy_ID2D1Factory1_CreateDevice(factory1, (IUnknown*) dxgi_device, &d2d_device);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_device_create: ID2D1Factory1::CreateDevice() failed.");
        goto err;
    }

    dummy_ID2D1Factory1_Release(factory1);
    adapter->lpVtbl->Release(adapter);
    dxgi_device->lpVtbl->Release(dxgi_device);
    d2d_device_generation++;
    return TRUE;

err:
    if(factory1 != NULL)
        dummy_ID2D1Factory1_Release(factory1);
    if(adapter != NULL)
        adapter->lpVtbl->Release(adapter);
    if(dxgi_device != NULL)
        dxgi_device->lpVtbl->Release(dxgi_device);
    d2d_device_release();
    d2d_device_unsupported = TRUE;
    return FALSE;
}

/* Canvases still on an older device must not drop the current one. */
void
d2d_device_lost(UINT generation)
{
    wd_lock();
    if(generation == d2d_device_generation)
        d2d_device_release();
    wd_unlock();
}

static BOOL
d2d_swap_chain_attach_target(d2d_canvas_t* c)
{
    dummy_D2D1_BITMAP_PROPERTIES1 props;
    IDXGISurface* surface;
    HRESULT hr;

    hr = c->swap_chain->lpVtbl->GetBuffer(c->swap_chain, 0, &dummy_IID_IDXGISurface, (void**) &surface);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_swap_chain_attach_target: IDXGISwapChain::GetBuffer() failed.");
        return FALSE;
    }

    memset(&props, 0, sizeof(props));
    props.pixelFormat.format = dummy_DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = dummy_D2D1_ALPHA_MODE_IGNORE;
    props.dpiX = 96.0f;
    props.dpiY = 96.0f;
    props.bitmapOptions = dummy_D2D1_BITMAP_OPTIONS_TARGET | dummy_D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    hr = dummy_ID2D1DeviceContext_CreateBitmapFromDxgiSurface(c->device_context,
                (IUnknown*) surface, &props, &c->swap_target);
    surface->lpVtbl->Release(surface);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_swap_chain_attach_target: "
                    "ID2D1DeviceContext::CreateBitmapFromDxgiSurface() failed.");
        return FALSE;
    }

    dummy_ID2D1DeviceContext_SetTarget(c->device_context, (dummy_ID2D1Image*) c->swap_target);
    return TRUE;
}

static void
d2d_swap_chain_detach_target(d2d_canvas_t* c)
{
    if(c->swap_target != NULL) {
        dummy_ID2D1DeviceContext_SetTarget(c->device_context, NULL);
        dummy_ID2D1Bitmap1_Release(c->swap_target);
        c->swap_target = NULL;
    }
}

d2d_canvas_t*
d2d_swap_chain_canvas_alloc(HWND hwnd, UINT width, UINT height, BOOL rtl)
{
    DXGI_SWAP_CHAIN_DESC1 desc;
    IDXGISwapChain1* swap_chain;
    dummy_ID2D1DeviceContext* device_context;
    d2d_canvas_t* c;
    UINT generation;
    HRESULT hr;

    wd_lock();
    if(!d2d_device_create()) {
        wd_unlock();
        return NULL;
    }

    /* Bitblt model with a single buffer keeps the back buffer between frames,
     * so partial repaints work like D2D1_PRESENT_OPTIONS_RETAINCONTENTS. */
    memset(&desc, 0, sizeof(desc));
    desc.Width = (width > 0 ? width : 1);
    desc.Height = (height > 0 ? height : 1);
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    hr = d2d_dxgi_factory->lpVtbl->CreateSwapChainForHwnd(d2d_dxgi_factory,
                (IUnknown*) d2d_d3d_device, hwnd, &desc, NULL, NULL, &swap_chain);
    if(FAILED(hr)) {
        wd_unlock();
        WD_TRACE_HR("d2d_swap_chain_canvas_alloc: "
                    "IDXGIFactory2::CreateSwapChainForHwnd() failed.");
        return NULL;
    }
    d2d_dxgi_factory->lpVtbl->MakeWindowAssociation(d2d_dxgi_factory, hwnd, DXGI_MWA_NO_WINDOW_CHANGES);

    hr = dummy_ID2D1Device_CreateDeviceContext(d2d_device, 0, &device_context);
    generation = d2d_device_generation;
    wd_unlock();
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_swap_chain_canvas_alloc: "
                    "ID2D1Device::CreateDeviceContext() failed.");
        swap_chain->lpVtbl->Release(swap_chain);
        return NULL;
    }

    c = d2d_canvas_alloc((dummy_ID2D1RenderTarget*) device_context, D2D_CANVASTYPE_SWAPCHAIN, width, rtl);
    if(c == NULL) {
        dummy_ID2D1DeviceContext_Release(device_context);
        swap_chain->lpVtbl->Release(swap_chain);
        return NULL;
    }
    c->swap_chain = swap_chain;
    c->device_generation = generation;

    if(!d2d_swap_chain_attach_target(c)) {
        d2d_swap_chain_release(c);
        dummy_ID2D1DeviceContext_Release(c->device_context);
        free(c);
        return NULL;
    }

    return c;
}

BOOL
d2d_swap_chain_resize(d2d_canvas_t* c, UINT width, UINT height)
{
    HRESULT hr;

    d2d_swap_chain_detach_target(c);
    hr = c->swap_chain->lpVtbl->ResizeBuffers(c->swap_chain, 0,
                (width > 0 ? width : 1), (height > 0 ? height : 1), DXGI_FORMAT_UNKNOWN, 0);
    if(FAILED(hr)) {
        WD_TRACE_HR("d2d_swap_chain_resize: IDXGISwapChain::ResizeBuffers() failed.");
        return FALSE;
    }
    return d2d_swap_chain_attach_target(c);
}

HRESULT
d2d_swap_chain_present(d2d_canvas_t* c)
{
    HRESULT hr;

    hr = c->swap_chain->lpVtbl->Present(c->swap_chain, 0, 0);
    if(hr == DXGI_ERROR_DEVICE_REMOVED  ||  hr == DXGI_ERROR_DEVICE_RESET)
        d2d_device_lost(c->device_generation);
    return hr;
}

void
d2d_swap_chain_release(d2d_canvas_t* c)
{
    d2d_swap_chain_detach_target(c);
    if(c->swap_chain != NULL) {
        c->swap_chain->lpVtbl->Release(c->swap_chain);
        c->swap_chain = NULL;
    }
}

void
d2d_fini(void)
{
    d2d_device_release();
    if(d3d11_dll != NULL) {
        FreeLibrary(d3d11_dll);
        d3d11_dll = NULL;
    }
    d2d_device_unsupported = FALSE;

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
d2d_update_text_antialias(d2d_canvas_t* c)
{
    dummy_ID2D1RenderTarget_SetTextAntialiasMode(c->target,
            (c->clip_layer != NULL || c->push_count > 0 || c->layers != NULL) ?
            dummy_D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : dummy_D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
}

void
d2d_reset_clip(d2d_canvas_t* c)
{
    if(c->clip_layer != NULL) {
        dummy_ID2D1RenderTarget_PopLayer(c->target);
        dummy_ID2D1Layer_Release(c->clip_layer);
        c->clip_layer = NULL;
        d2d_update_text_antialias(c);
    }
    if(c->flags & D2D_CANVASFLAG_RECTCLIP) {
        dummy_ID2D1RenderTarget_PopAxisAlignedClip(c->target);
        c->flags &= ~D2D_CANVASFLAG_RECTCLIP;
    }
}

void
d2d_pop_layer(d2d_canvas_t* c)
{
    d2d_layer_t* l = c->layers;

    if(l == NULL)
        return;

    d2d_reset_clip(c);

    if(l->layer != NULL) {
        dummy_ID2D1RenderTarget_PopLayer(c->target);
        dummy_ID2D1Layer_Release(l->layer);
    }

    c->clip_layer = l->clip_layer;
    c->flags |= l->clip_flags;
    c->layers = l->next;
    free(l);
    d2d_update_text_antialias(c);
}

void
d2d_reset_layers(d2d_canvas_t* c)
{
    while(c->layers != NULL)
        d2d_pop_layer(c);
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
