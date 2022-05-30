/*
 * WinDrawLib
 * Copyright (c) 2017 Martin Mitas
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


#ifndef DUMMY_D2D1_H
#define DUMMY_D2D1_H

#include <windows.h>
#include <unknwn.h>

#include <dcommon.h>
#include <d2dbasetypes.h>
#include <d2derr.h>

#include <wincodec.h>   /* IWICBitmapSource */


static const GUID dummy_IID_ID2D1Factory =
        {0x06152247,0x6f50,0x465a,{0x92,0x45,0x11,0x8b,0xfd,0x3b,0x60,0x07}};

static const GUID dummy_IID_ID2D1GdiInteropRenderTarget =
        {0xe0db51c3,0x6f77,0x4bae,{0xb3,0xd5,0xe4,0x75,0x09,0xb3,0x58,0x38}};


/******************************
 ***  Forward declarations  ***
 ******************************/

typedef struct dummy_IDWriteTextLayout_tag              dummy_IDWriteTextLayout;    /* from "dummy/dwrite.h" */

typedef struct dummy_ID2D1Bitmap_tag                    dummy_ID2D1Bitmap;
typedef struct dummy_ID2D1BitmapRenderTarget_tag        dummy_ID2D1BitmapRenderTarget;
typedef struct dummy_ID2D1Brush_tag                     dummy_ID2D1Brush;
typedef struct dummy_ID2D1StrokeStyle_tag               dummy_ID2D1StrokeStyle;
typedef struct dummy_ID2D1DCRenderTarget_tag            dummy_ID2D1DCRenderTarget;
typedef struct dummy_ID2D1Factory_tag                   dummy_ID2D1Factory;
typedef struct dummy_ID2D1GdiInteropRenderTarget_tag    dummy_ID2D1GdiInteropRenderTarget;
typedef struct dummy_ID2D1Geometry_tag                  dummy_ID2D1Geometry;
typedef struct dummy_ID2D1GeometrySink_tag              dummy_ID2D1GeometrySink;
typedef struct dummy_ID2D1HwndRenderTarget_tag          dummy_ID2D1HwndRenderTarget;
typedef struct dummy_ID2D1Layer_tag                     dummy_ID2D1Layer;
typedef struct dummy_ID2D1PathGeometry_tag              dummy_ID2D1PathGeometry;
typedef struct dummy_ID2D1RenderTarget_tag              dummy_ID2D1RenderTarget;
typedef struct dummy_ID2D1SolidColorBrush_tag           dummy_ID2D1SolidColorBrush;


/*****************************
 ***  Helper Enumerations  ***
 *****************************/

#define dummy_D2D1_DRAW_TEXT_OPTIONS_CLIP               0x00000002
#define dummy_D2D1_PRESENT_OPTIONS_NONE                 0x00000000
#define dummy_D2D1_LAYER_OPTIONS_NONE                   0x00000000
#define dummy_D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE   0x00000002

typedef enum dummy_D2D1_TEXT_ANTIALIAS_MODE_tag dummy_D2D1_TEXT_ANTIALIAS_MODE;
enum  dummy_D2D1_TEXT_ANTIALIAS_MODE_tag {
  dummy_D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE = 1
} ;

typedef enum dummy_DXGI_FORMAT_tag dummy_DXGI_FORMAT;
enum dummy_DXGI_FORMAT_tag {
    dummy_DXGI_FORMAT_B8G8R8A8_UNORM = 87
};

typedef enum dummy_D2D1_ANTIALIAS_MODE_tag dummy_D2D1_ANTIALIAS_MODE;
enum dummy_D2D1_ANTIALIAS_MODE_tag {
    dummy_D2D1_ANTIALIAS_MODE_PER_PRIMITIVE = 0
};

typedef enum dummy_D2D1_ALPHA_MODE_tag dummy_D2D1_ALPHA_MODE;
enum dummy_D2D1_ALPHA_MODE_tag {
    dummy_D2D1_ALPHA_MODE_UNKNOWN = 0,
    dummy_D2D1_ALPHA_MODE_PREMULTIPLIED = 1
};

typedef enum dummy_D2D1_ARC_SIZE_tag dummy_D2D1_ARC_SIZE;
enum dummy_D2D1_ARC_SIZE_tag {
    dummy_D2D1_ARC_SIZE_SMALL = 0,
    dummy_D2D1_ARC_SIZE_LARGE = 1
};

typedef enum dummy_D2D1_DC_INITIALIZE_MODE_tag dummy_D2D1_DC_INITIALIZE_MODE;
enum dummy_D2D1_DC_INITIALIZE_MODE_tag {
    dummy_D2D1_DC_INITIALIZE_MODE_COPY = 0,
    dummy_D2D1_DC_INITIALIZE_MODE_CLEAR = 1
};

typedef enum dummy_D2D1_DEBUG_LEVEL_tag dummy_D2D1_DEBUG_LEVEL;
enum dummy_D2D1_DEBUG_LEVEL_tag {
    dummy_D2D1_DEBUG_LEVEL_NONE = 0
};

typedef enum dummy_D2D1_FACTORY_TYPE_tag dummy_D2D1_FACTORY_TYPE;
enum dummy_D2D1_FACTORY_TYPE_tag {
    dummy_D2D1_FACTORY_TYPE_SINGLE_THREADED = 0,
    dummy_D2D1_FACTORY_TYPE_MULTI_THREADED = 1
};

typedef enum dummy_D2D1_FEATURE_LEVEL_tag dummy_D2D1_FEATURE_LEVEL;
enum dummy_D2D1_FEATURE_LEVEL_tag {
    dummy_D2D1_FEATURE_LEVEL_DEFAULT = 0
};

typedef enum dummy_D2D1_FIGURE_BEGIN_tag dummy_D2D1_FIGURE_BEGIN;
enum dummy_D2D1_FIGURE_BEGIN_tag {
    dummy_D2D1_FIGURE_BEGIN_FILLED = 0,
    dummy_D2D1_FIGURE_BEGIN_HOLLOW = 1
};

typedef enum dummy_D2D1_FIGURE_END_tag dummy_D2D1_FIGURE_END;
enum dummy_D2D1_FIGURE_END_tag {
    dummy_D2D1_FIGURE_END_OPEN = 0,
    dummy_D2D1_FIGURE_END_CLOSED = 1
};

typedef enum dummy_D2D1_BITMAP_INTERPOLATION_MODE_tag dummy_D2D1_BITMAP_INTERPOLATION_MODE;
enum dummy_D2D1_BITMAP_INTERPOLATION_MODE_tag {
    dummy_D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR = 0,
    dummy_D2D1_BITMAP_INTERPOLATION_MODE_LINEAR = 1
};

typedef enum dummy_D2D1_RENDER_TARGET_TYPE_tag dummy_D2D1_RENDER_TARGET_TYPE;
enum dummy_D2D1_RENDER_TARGET_TYPE_tag {
    dummy_D2D1_RENDER_TARGET_TYPE_DEFAULT = 0
};

typedef enum dummy_D2D1_SWEEP_DIRECTION_tag dummy_D2D1_SWEEP_DIRECTION;
enum dummy_D2D1_SWEEP_DIRECTION_tag {
    dummy_D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE = 0,
    dummy_D2D1_SWEEP_DIRECTION_CLOCKWISE = 1
};

typedef enum dummy_D2D1_CAP_STYLE_tag dummy_D2D1_CAP_STYLE;
enum dummy_D2D1_CAP_STYLE_tag {
  dummy_D2D1_CAP_STYLE_FLAT = 0,
  dummy_D2D1_CAP_STYLE_SQUARE = 1,
  dummy_D2D1_CAP_STYLE_ROUND = 2,
  dummy_D2D1_CAP_STYLE_TRIANGLE = 3,
};

typedef enum dummy_D2D1_DASH_STYLE_tag dummy_D2D1_DASH_STYLE;
enum dummy_D2D1_DASH_STYLE_tag {
  dummy_D2D1_DASH_STYLE_SOLID = 0,
  dummy_D2D1_DASH_STYLE_DASH = 1,
  dummy_D2D1_DASH_STYLE_DOT = 2,
  dummy_D2D1_DASH_STYLE_DASH_DOT = 3,
  dummy_D2D1_DASH_STYLE_DASH_DOT_DOT = 4,
  dummy_D2D1_DASH_STYLE_CUSTOM = 5
};

typedef enum dummy_D2D1_LINE_JOIN_tag dummy_D2D1_LINE_JOIN;
enum dummy_D2D1_LINE_JOIN_tag {
  dummy_D2D1_LINE_JOIN_MITER = 0,
  dummy_D2D1_LINE_JOIN_BEVEL = 1,
  dummy_D2D1_LINE_JOIN_ROUND = 2
};


/*************************
 ***  Helper Typedefs  ***
 *************************/

typedef struct dummy_D2D1_BITMAP_PROPERTIES_tag dummy_D2D1_BITMAP_PROPERTIES;
typedef D2D_COLOR_F                             dummy_D2D1_COLOR_F;
typedef struct D2D_MATRIX_3X2_F                 dummy_D2D1_MATRIX_3X2_F;
typedef struct D2D_POINT_2F                     dummy_D2D1_POINT_2F;
typedef struct D2D_RECT_F                       dummy_D2D1_RECT_F;
typedef struct D2D_SIZE_F                       dummy_D2D1_SIZE_F;
typedef struct D2D_SIZE_U                       dummy_D2D1_SIZE_U;


/***************************
 ***  Helper Structures  ***
 ***************************/

typedef struct dummy_D2D1_ARC_SEGMENT_tag dummy_D2D1_ARC_SEGMENT;
struct dummy_D2D1_ARC_SEGMENT_tag {
    dummy_D2D1_POINT_2F point;
    dummy_D2D1_SIZE_F size;
    FLOAT rotationAngle;
    dummy_D2D1_SWEEP_DIRECTION sweepDirection;
    dummy_D2D1_ARC_SIZE arcSize;
};

typedef struct dummy_D2D1_ELLIPSE_tag dummy_D2D1_ELLIPSE;
struct dummy_D2D1_ELLIPSE_tag {
    dummy_D2D1_POINT_2F point;
    FLOAT radiusX;
    FLOAT radiusY;
};

typedef struct dummy_D2D1_FACTORY_OPTIONS_tag dummy_D2D1_FACTORY_OPTIONS;
struct dummy_D2D1_FACTORY_OPTIONS_tag {
    dummy_D2D1_DEBUG_LEVEL debugLevel;
};

typedef struct dummy_D2D1_HWND_RENDER_TARGET_PROPERTIES_tag dummy_D2D1_HWND_RENDER_TARGET_PROPERTIES;
struct dummy_D2D1_HWND_RENDER_TARGET_PROPERTIES_tag {
    HWND hwnd;
    dummy_D2D1_SIZE_U pixelSize;
    unsigned presentOptions;
};

typedef struct dummy_D2D1_PIXEL_FORMAT_tag dummy_D2D1_PIXEL_FORMAT;
struct dummy_D2D1_PIXEL_FORMAT_tag {
    dummy_DXGI_FORMAT format;
    dummy_D2D1_ALPHA_MODE alphaMode;
};

typedef struct dummy_D2D1_RENDER_TARGET_PROPERTIES_tag dummy_D2D1_RENDER_TARGET_PROPERTIES;
struct dummy_D2D1_RENDER_TARGET_PROPERTIES_tag {
    dummy_D2D1_RENDER_TARGET_TYPE type;
    dummy_D2D1_PIXEL_FORMAT pixelFormat;
    FLOAT dpiX;
    FLOAT dpiY;
    unsigned usage;
    dummy_D2D1_FEATURE_LEVEL minLevel;
};

typedef struct dummy_D2D1_STROKE_STYLE_PROPERTIES_tag dummy_D2D1_STROKE_STYLE_PROPERTIES;
struct dummy_D2D1_STROKE_STYLE_PROPERTIES_tag {
  dummy_D2D1_CAP_STYLE startCap;
  dummy_D2D1_CAP_STYLE endCap;
  dummy_D2D1_CAP_STYLE dashCap;
  dummy_D2D1_LINE_JOIN lineJoin;
  FLOAT miterLimit;
  dummy_D2D1_DASH_STYLE dashStyle;
  FLOAT dashOffset;
};

typedef struct dummy_D2D1_LAYER_PARAMETERS_tag dummy_D2D1_LAYER_PARAMETERS;
struct dummy_D2D1_LAYER_PARAMETERS_tag {
    dummy_D2D1_RECT_F contentBounds;
    dummy_ID2D1Geometry* geometricMask;
    dummy_D2D1_ANTIALIAS_MODE maskAntialiasMode;
    dummy_D2D1_MATRIX_3X2_F maskTransform;
    FLOAT opacity;
    dummy_ID2D1Brush* opacityBrush;
    unsigned layerOptions;
};


/*******************************
 ***  Interface ID2D1Bitmap  ***
 *******************************/

typedef struct dummy_ID2D1BitmapVtbl_tag dummy_ID2D1BitmapVtbl;
struct dummy_ID2D1BitmapVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1Bitmap*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1Bitmap*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1Bitmap*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1Image methods */
    /* none */

    /* ID2D1Bitmap methods */
    STDMETHOD(dummy_GetSize)(void);
#if 0
    /* Original vanilla method prototype. But this seems to be problematic
     * as compiler use different ABI when returning aggregate types.
     *
     * When built with incompatible compiler, it usually results in crash.
     *
     * For gcc, it seems to be completely incompatible:
     *  -- https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64384
     *  -- https://sourceforge.net/p/mingw-w64/mailman/message/36238073/
     *  -- https://source.winehq.org/git/wine.git/commitdiff/b42a15513eaa973b40ab967014b311af64acbb98
     *  -- https://www.winehq.org/pipermail/wine-devel/2017-July/118470.html
     *  -- https://bugzilla.mozilla.org/show_bug.cgi?id=1411401
     *
     * For MSVC, it is compatible only when building as C++. In C, it crashes
     * as well.
     */
    STDMETHOD_(dummy_D2D1_SIZE_U, GetPixelSize)(dummy_ID2D1Bitmap*);
#else
    /* This prototype corresponds more literally to what the COM calling
     * convention is expecting from us.
     *
     * Tested with MSVC 2017 (64-bit build), gcc (32-bit build).
     */
    STDMETHOD_(void, GetPixelSize)(dummy_ID2D1Bitmap*, dummy_D2D1_SIZE_U*);
#endif
    STDMETHOD(dummy_GetPixelFormat)(void);
    STDMETHOD(dummy_GetDpi)(void);
    STDMETHOD(dummy_CopyFromBitmap)(void);
    STDMETHOD(dummy_CopyFromRenderTarget)(void);
    STDMETHOD(dummy_CopyFromMemory)(void);
};

struct dummy_ID2D1Bitmap_tag {
    dummy_ID2D1BitmapVtbl* vtbl;
};

#define dummy_ID2D1Bitmap_QueryInterface(self,a,b)  (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1Bitmap_AddRef(self)              (self)->vtbl->AddRef(self)
#define dummy_ID2D1Bitmap_Release(self)             (self)->vtbl->Release(self)
#define dummy_ID2D1Bitmap_GetPixelSize(self,a)      (self)->vtbl->GetPixelSize(self,a)


/*******************************************
 ***  Interface ID2D1BitmapRenderTarget  ***
 *******************************************/

typedef struct dummy_ID2D1BitmapRenderTargetVtbl_tag dummy_ID2D1BitmapRenderTargetVtbl;
struct dummy_ID2D1BitmapRenderTargetVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1BitmapRenderTarget*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1BitmapRenderTarget*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1BitmapRenderTarget*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1RenderTarget methods */
    STDMETHOD(dummy_CreateBitmap)(void);
    STDMETHOD(dummy_CreateBitmapFromWicBitmap)(void);
    STDMETHOD(dummy_CreateSharedBitmap)(void);
    STDMETHOD(dummy_CreateBitmapBrush)(void);
    STDMETHOD(dummy_CreateSolidColorBrush)(void);
    STDMETHOD(dummy_CreateGradientStopCollection)(void);
    STDMETHOD(dummy_CreateLinearGradientBrush)(void);
    STDMETHOD(dummy_CreateRadialGradientBrush)(void);
    STDMETHOD(dummy_CreateCompatibleRenderTarget)(void);
    STDMETHOD(dummy_CreateLayer)(void);
    STDMETHOD(dummy_CreateMesh)(void);
    STDMETHOD(dummy_DrawLine)(void);
    STDMETHOD(dummy_DrawRectangle)(void);
    STDMETHOD(dummy_FillRectangle)(void);
    STDMETHOD(dummy_DrawRoundedRectangle)(void);
    STDMETHOD(dummy_FillRoundedRectangle)(void);
    STDMETHOD(dummy_DrawEllipse)(void);
    STDMETHOD(dummy_FillEllipse)(void);
    STDMETHOD(dummy_DrawGeometry)(void);
    STDMETHOD(dummy_FillGeometry)(void);
    STDMETHOD(dummy_FillMesh)(void);
    STDMETHOD(dummy_FillOpacityMask)(void);
    STDMETHOD(dummy_DrawBitmap)(void);
    STDMETHOD(dummy_DrawText)(void);
    STDMETHOD(dummy_DrawTextLayout)(void);
    STDMETHOD(dummy_DrawGlyphRun)(void);
    STDMETHOD(dummy_SetTransform)(void);
    STDMETHOD(dummy_GetTransform)(void);
    STDMETHOD(dummy_SetAntialiasMode)(void);
    STDMETHOD(dummy_GetAntialiasMode)(void);
    STDMETHOD(dummy_SetTextAntialiasMode)(void);
    STDMETHOD(dummy_GetTextAntialiasMode)(void);
    STDMETHOD(dummy_SetTextRenderingParams)(void);
    STDMETHOD(dummy_GetTextRenderingParams)(void);
    STDMETHOD(dummy_SetTags)(void);
    STDMETHOD(dummy_GetTags)(void);
    STDMETHOD(dummy_PushLayer)(void);
    STDMETHOD(dummy_PopLayer)(void);
    STDMETHOD(dummy_Flush)(void);
    STDMETHOD(dummy_SaveDrawingState)(void);
    STDMETHOD(dummy_RestoreDrawingState)(void);
    STDMETHOD(dummy_PushAxisAlignedClip)(void);
    STDMETHOD(dummy_PopAxisAlignedClip)(void);
    STDMETHOD(dummy_Clear)(void);
    STDMETHOD(dummy_BeginDraw)(void);
    STDMETHOD(dummy_EndDraw)(void);
    STDMETHOD(dummy_GetPixelFormat)(void);
    STDMETHOD(dummy_SetDpi)(void);
    STDMETHOD(dummy_GetDpi)(void);
    STDMETHOD(dummy_GetSize)(void);
    STDMETHOD(dummy_GetPixelSize)(void);
    STDMETHOD(dummy_GetMaximumBitmapSize)(void);
    STDMETHOD(dummy_IsSupported)(void);

    /* ID2D1BitmapRenderTarget methods */
    STDMETHOD(dummy_GetBitmap)(void);
};

struct dummy_ID2D1BitmapRenderTarget_tag {
    dummy_ID2D1BitmapRenderTargetVtbl* vtbl;
};

#define dummy_ID2D1BitmapRenderTarget_QueryInterface(self,a,b)  (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1BitmapRenderTarget_AddRef(self)              (self)->vtbl->AddRef(self)
#define dummy_ID2D1BitmapRenderTarget_Release(self)             (self)->vtbl->Release(self)


/******************************
 ***  Interface ID2D1Brush  ***
 ******************************/

typedef struct dummy_ID2D1BrushVtbl_tag dummy_ID2D1BrushVtbl;
struct dummy_ID2D1BrushVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1Brush*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1Brush*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1Brush*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1Brush methods */
    STDMETHOD(dummy_SetOpacity)(void);
    STDMETHOD(dummy_SetTransform)(void);
    STDMETHOD(dummy_GetOpacity)(void);
    STDMETHOD(dummy_GetTransform)(void);
};

struct dummy_ID2D1Brush_tag {
    dummy_ID2D1BrushVtbl* vtbl;
};

#define dummy_ID2D1Brush_QueryInterface(self,a,b)               (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1Brush_AddRef(self)                           (self)->vtbl->AddRef(self)
#define dummy_ID2D1Brush_Release(self)                          (self)->vtbl->Release(self)


/***********************************
***  Interface ID2D1StrokeStyle  ***
***********************************/

typedef struct dummy_ID2D1StrokeStyleVtbl_tag dummy_ID2D1StrokeStyleVtbl;
struct dummy_ID2D1StrokeStyleVtbl_tag {
  /* IUnknown methods */
  STDMETHOD(QueryInterface)(dummy_ID2D1StrokeStyle*, REFIID, void**);
  STDMETHOD_(ULONG, AddRef)(dummy_ID2D1StrokeStyle*);
  STDMETHOD_(ULONG, Release)(dummy_ID2D1StrokeStyle*);

  /* ID2D1Resource methods */
  STDMETHOD(dummy_GetFactory)(void);

  /* ID2D1StrokeStyle methods */
  STDMETHOD(dummy_GetStartCap)(void);
  STDMETHOD(dummy_GetEndCap)(void);
  STDMETHOD(dummy_GetDashCap)(void);
  STDMETHOD(dummy_GetMiterLimit)(void);
  STDMETHOD(dummy_GetLineJoin)(void);
  STDMETHOD(dummy_GetDashOffset)(void);
  STDMETHOD(dummy_GetDashStyle)(void);
  STDMETHOD(dummy_GetDashesCount)(void);
  STDMETHOD(dummy_GetDashes)(void);
};

struct dummy_ID2D1StrokeStyle_tag {
  dummy_ID2D1StrokeStyleVtbl* vtbl;
};

#define dummy_ID2D1StrokeStyle_QueryInterface(self,a,b)               (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1StrokeStyle_AddRef(self)                           (self)->vtbl->AddRef(self)
#define dummy_ID2D1StrokeStyle_Release(self)                          (self)->vtbl->Release(self)


/*****************************************
 ***  Interface ID2D1DCRenderTarget  ***
 *****************************************/

typedef struct dummy_ID2D1DCRenderTargetVtbl_tag dummy_ID2D1DCRenderTargetVtbl;
struct dummy_ID2D1DCRenderTargetVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1DCRenderTarget*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1DCRenderTarget*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1DCRenderTarget*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1RenderTarget methods */
    STDMETHOD(dummy_CreateBitmap)(void);
    STDMETHOD(dummy_CreateBitmapFromWicBitmap)(void);
    STDMETHOD(dummy_CreateSharedBitmap)(void);
    STDMETHOD(dummy_CreateBitmapBrush)(void);
    STDMETHOD(dummy_CreateSolidColorBrush)(void);
    STDMETHOD(dummy_CreateGradientStopCollection)(void);
    STDMETHOD(dummy_CreateLinearGradientBrush)(void);
    STDMETHOD(dummy_CreateRadialGradientBrush)(void);
    STDMETHOD(dummy_CreateCompatibleRenderTarget)(void);
    STDMETHOD(dummy_CreateLayer)(void);
    STDMETHOD(dummy_CreateMesh)(void);
    STDMETHOD(dummy_DrawLine)(void);
    STDMETHOD(dummy_DrawRectangle)(void);
    STDMETHOD(dummy_FillRectangle)(void);
    STDMETHOD(dummy_DrawRoundedRectangle)(void);
    STDMETHOD(dummy_FillRoundedRectangle)(void);
    STDMETHOD(dummy_DrawEllipse)(void);
    STDMETHOD(dummy_FillEllipse)(void);
    STDMETHOD(dummy_DrawGeometry)(void);
    STDMETHOD(dummy_FillGeometry)(void);
    STDMETHOD(dummy_FillMesh)(void);
    STDMETHOD(dummy_FillOpacityMask)(void);
    STDMETHOD(dummy_DrawBitmap)(void);
    STDMETHOD(dummy_DrawText)(void);
    STDMETHOD(dummy_DrawTextLayout)(void);
    STDMETHOD(dummy_DrawGlyphRun)(void);
    STDMETHOD(dummy_SetTransform)(void);
    STDMETHOD(dummy_GetTransform)(void);
    STDMETHOD(dummy_SetAntialiasMode)(void);
    STDMETHOD(dummy_GetAntialiasMode)(void);
    STDMETHOD(dummy_SetTextAntialiasMode)(void);
    STDMETHOD(dummy_GetTextAntialiasMode)(void);
    STDMETHOD(dummy_SetTextRenderingParams)(void);
    STDMETHOD(dummy_GetTextRenderingParams)(void);
    STDMETHOD(dummy_SetTags)(void);
    STDMETHOD(dummy_GetTags)(void);
    STDMETHOD(dummy_PushLayer)(void);
    STDMETHOD(dummy_PopLayer)(void);
    STDMETHOD(dummy_Flush)(void);
    STDMETHOD(dummy_SaveDrawingState)(void);
    STDMETHOD(dummy_RestoreDrawingState)(void);
    STDMETHOD(dummy_PushAxisAlignedClip)(void);
    STDMETHOD(dummy_PopAxisAlignedClip)(void);
    STDMETHOD(dummy_Clear)(void);
    STDMETHOD(dummy_BeginDraw)(void);
    STDMETHOD(dummy_EndDraw)(void);
    STDMETHOD(dummy_GetPixelFormat)(void);
    STDMETHOD(dummy_SetDpi)(void);
    STDMETHOD(dummy_GetDpi)(void);
    STDMETHOD(dummy_GetSize)(void);
    STDMETHOD(dummy_GetPixelSize)(void);
    STDMETHOD(dummy_GetMaximumBitmapSize)(void);
    STDMETHOD(dummy_IsSupported)(void);

    /* ID2D1DCRenderTarget methods */
    STDMETHOD(BindDC)(dummy_ID2D1DCRenderTarget*, const HDC, const RECT*);
};

struct dummy_ID2D1DCRenderTarget_tag {
    dummy_ID2D1DCRenderTargetVtbl* vtbl;
};

#define dummy_ID2D1DCRenderTarget_QueryInterface(self,a,b)      (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1DCRenderTarget_AddRef(self)                  (self)->vtbl->AddRef(self)
#define dummy_ID2D1DCRenderTarget_Release(self)                 (self)->vtbl->Release(self)
#define dummy_ID2D1DCRenderTarget_BindDC(self,a,b)              (self)->vtbl->BindDC(self,a,b)


/********************************
 ***  Interface ID2D1Factory  ***
 ********************************/

typedef struct dummy_ID2D1FactoryVtbl_tag dummy_ID2D1FactoryVtbl;
struct dummy_ID2D1FactoryVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1Factory*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1Factory*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1Factory*);

    /* ID2D1Factory methods */
    STDMETHOD(dummy_ReloadSystemMetrics)(void);
    STDMETHOD(dummy_GetDesktopDpi)(void);
    STDMETHOD(dummy_CreateRectangleGeometry)(void);
    STDMETHOD(dummy_CreateRoundedRectangleGeometry)(void);
    STDMETHOD(dummy_CreateEllipseGeometry)(void);
    STDMETHOD(dummy_CreateGeometryGroup)(void);
    STDMETHOD(dummy_CreateTransformedGeometry)(void);
    STDMETHOD(CreatePathGeometry)(dummy_ID2D1Factory*, dummy_ID2D1PathGeometry**);
    STDMETHOD(CreateStrokeStyle)(dummy_ID2D1Factory*, const dummy_D2D1_STROKE_STYLE_PROPERTIES*, const FLOAT*, UINT32, dummy_ID2D1StrokeStyle**);
    STDMETHOD(dummy_CreateDrawingStateBlock)(void);
    STDMETHOD(dummy_CreateWicBitmapRenderTarget)(void);
    STDMETHOD(CreateHwndRenderTarget)(dummy_ID2D1Factory*, const dummy_D2D1_RENDER_TARGET_PROPERTIES*,
              const dummy_D2D1_HWND_RENDER_TARGET_PROPERTIES*, dummy_ID2D1HwndRenderTarget**);
    STDMETHOD(dummy_CreateDxgiSurfaceRenderTarget)(void);
    STDMETHOD(CreateDCRenderTarget)(dummy_ID2D1Factory*, const dummy_D2D1_RENDER_TARGET_PROPERTIES*, dummy_ID2D1DCRenderTarget**);
};

struct dummy_ID2D1Factory_tag {
    dummy_ID2D1FactoryVtbl* vtbl;
};

#define dummy_ID2D1Factory_QueryInterface(self,a,b)             (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1Factory_AddRef(self)                         (self)->vtbl->AddRef(self)
#define dummy_ID2D1Factory_Release(self)                        (self)->vtbl->Release(self)
#define dummy_ID2D1Factory_CreatePathGeometry(self,a)           (self)->vtbl->CreatePathGeometry(self,a)
#define dummy_ID2D1Factory_CreateHwndRenderTarget(self,a,b,c)   (self)->vtbl->CreateHwndRenderTarget(self,a,b,c)
#define dummy_ID2D1Factory_CreateDCRenderTarget(self,a,b)       (self)->vtbl->CreateDCRenderTarget(self,a,b)
#define dummy_ID2D1Factory_CreateStrokeStyle(self,a,b,c,d)      (self)->vtbl->CreateStrokeStyle(self,a,b,c,d)


/*****************************************************
 ***  Interface dummy_ID2D1GdiInteropRenderTarget  ***
 *****************************************************/

typedef struct dummy_ID2D1GdiInteropRenderTargetVtbl_tag dummy_ID2D1GdiInteropRenderTargetVtbl;
struct dummy_ID2D1GdiInteropRenderTargetVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1GdiInteropRenderTarget*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1GdiInteropRenderTarget*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1GdiInteropRenderTarget*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1RenderTarget methods */
    STDMETHOD(dummy_CreateBitmap)(void);
    STDMETHOD(dummy_CreateBitmapFromWicBitmap)(void);
    STDMETHOD(dummy_CreateSharedBitmap)(void);
    STDMETHOD(dummy_CreateBitmapBrush)(void);
    STDMETHOD(dummy_CreateSolidColorBrush)(void);
    STDMETHOD(dummy_CreateGradientStopCollection)(void);
    STDMETHOD(dummy_CreateLinearGradientBrush)(void);
    STDMETHOD(dummy_CreateRadialGradientBrush)(void);
    STDMETHOD(dummy_CreateCompatibleRenderTarget)(void);
    STDMETHOD(dummy_CreateLayer)(void);
    STDMETHOD(dummy_CreateMesh)(void);
    STDMETHOD(dummy_DrawLine)(void);
    STDMETHOD(dummy_DrawRectangle)(void);
    STDMETHOD(dummy_FillRectangle)(void);
    STDMETHOD(dummy_DrawRoundedRectangle)(void);
    STDMETHOD(dummy_FillRoundedRectangle)(void);
    STDMETHOD(dummy_DrawEllipse)(void);
    STDMETHOD(dummy_FillEllipse)(void);
    STDMETHOD(dummy_DrawGeometry)(void);
    STDMETHOD(dummy_FillGeometry)(void);
    STDMETHOD(dummy_FillMesh)(void);
    STDMETHOD(dummy_FillOpacityMask)(void);
    STDMETHOD(dummy_DrawBitmap)(void);
    STDMETHOD(dummy_DrawText)(void);
    STDMETHOD(dummy_DrawTextLayout)(void);
    STDMETHOD(dummy_DrawGlyphRun)(void);
    STDMETHOD(dummy_SetTransform)(void);
    STDMETHOD(dummy_GetTransform)(void);
    STDMETHOD(dummy_SetAntialiasMode)(void);
    STDMETHOD(dummy_GetAntialiasMode)(void);
    STDMETHOD(dummy_SetTextAntialiasMode)(void);
    STDMETHOD(dummy_GetTextAntialiasMode)(void);
    STDMETHOD(dummy_SetTextRenderingParams)(void);
    STDMETHOD(dummy_GetTextRenderingParams)(void);
    STDMETHOD(dummy_SetTags)(void);
    STDMETHOD(dummy_GetTags)(void);
    STDMETHOD(dummy_PushLayer)(void);
    STDMETHOD(dummy_PopLayer)(void);
    STDMETHOD(dummy_Flush)(void);
    STDMETHOD(dummy_SaveDrawingState)(void);
    STDMETHOD(dummy_RestoreDrawingState)(void);
    STDMETHOD(dummy_PushAxisAlignedClip)(void);
    STDMETHOD(dummy_PopAxisAlignedClip)(void);
    STDMETHOD(dummy_Clear)(void);
    STDMETHOD(dummy_BeginDraw)(void);
    STDMETHOD(dummy_EndDraw)(void);
    STDMETHOD(dummy_GetPixelFormat)(void);
    STDMETHOD(dummy_SetDpi)(void);
    STDMETHOD(dummy_GetDpi)(void);
    STDMETHOD(dummy_GetSize)(void);
    STDMETHOD(dummy_GetPixelSize)(void);
    STDMETHOD(dummy_GetMaximumBitmapSize)(void);
    STDMETHOD(dummy_IsSupported)(void);

    /* ID2D1GdiInteropRenderTarget methods */
    STDMETHOD(GetDC)(dummy_ID2D1GdiInteropRenderTarget*, dummy_D2D1_DC_INITIALIZE_MODE, HDC*);
    STDMETHOD(ReleaseDC)(dummy_ID2D1GdiInteropRenderTarget*, const RECT*);
};

struct dummy_ID2D1GdiInteropRenderTarget_tag {
    dummy_ID2D1GdiInteropRenderTargetVtbl* vtbl;
};

#define dummy_ID2D1GdiInteropRenderTarget_QueryInterface(self,a,b)      (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1GdiInteropRenderTarget_AddRef(self)                  (self)->vtbl->AddRef(self)
#define dummy_ID2D1GdiInteropRenderTarget_Release(self)                 (self)->vtbl->Release(self)
#define dummy_ID2D1GdiInteropRenderTarget_GetDC(self,a,b)               (self)->vtbl->GetDC(self,a,b)
#define dummy_ID2D1GdiInteropRenderTarget_ReleaseDC(self,a)             (self)->vtbl->ReleaseDC(self,a)


/*********************************
 ***  Interface ID2D1Geometry  ***
 *********************************/

typedef struct dummy_ID2D1GeometryVtbl_tag dummy_ID2D1GeometryVtbl;
struct dummy_ID2D1GeometryVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1Geometry*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1Geometry*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1Geometry*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1Geometry methods */
    STDMETHOD(dummy_GetBounds)(void);
    STDMETHOD(dummy_GetWidenedBounds)(void);
    STDMETHOD(dummy_StrokeContainsPoint)(void);
    STDMETHOD(dummy_FillContainsPoint)(void);
    STDMETHOD(dummy_CompareWithGeometry)(void);
    STDMETHOD(dummy_Simplify)(void);
    STDMETHOD(dummy_Tessellate)(void);
    STDMETHOD(dummy_CombineWithGeometry)(void);
    STDMETHOD(dummy_Outline)(void);
    STDMETHOD(dummy_ComputeArea)(void);
    STDMETHOD(dummy_ComputeLength)(void);
    STDMETHOD(dummy_ComputePointAtLength)(void);
    STDMETHOD(dummy_Widen)(void);
};

struct dummy_ID2D1Geometry_tag {
    dummy_ID2D1GeometryVtbl* vtbl;
};

#define dummy_ID2D1Geometry_QueryInterface(self,a,b)    (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1Geometry_AddRef(self)                (self)->vtbl->AddRef(self)
#define dummy_ID2D1Geometry_Release(self)               (self)->vtbl->Release(self)


/*************************************
 ***  Interface ID2D1GeometrySink  ***
 *************************************/

typedef struct dummy_ID2D1GeometrySinkVtbl_tag dummy_ID2D1GeometrySinkVtbl;
struct dummy_ID2D1GeometrySinkVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1GeometrySink*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1GeometrySink*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1GeometrySink*);

    /* ID2D1SimplifiedGeometrySink methods */
    STDMETHOD(dummy_SetFillMode)(void);
    STDMETHOD(dummy_SetSegmentFlags)(void);
    STDMETHOD_(void, BeginFigure)(dummy_ID2D1GeometrySink*, dummy_D2D1_POINT_2F, dummy_D2D1_FIGURE_BEGIN);
    STDMETHOD(dummy_AddLines)(void);
    STDMETHOD(dummy_AddBeziers)(void);
    STDMETHOD_(void, EndFigure)(dummy_ID2D1GeometrySink*, dummy_D2D1_FIGURE_END);
    STDMETHOD(Close)(dummy_ID2D1GeometrySink*) PURE;

    /* ID2D1GeometrySink methods */
    STDMETHOD_(void, AddLine)(dummy_ID2D1GeometrySink*, dummy_D2D1_POINT_2F point);
    STDMETHOD(dummy_AddBezier)(void);
    STDMETHOD(dummy_AddQuadraticBezier)(void);
    STDMETHOD(dummy_AddQuadraticBeziers)(void);
    STDMETHOD_(void, AddArc)(dummy_ID2D1GeometrySink*, const dummy_D2D1_ARC_SEGMENT*);
};

struct dummy_ID2D1GeometrySink_tag {
    dummy_ID2D1GeometrySinkVtbl* vtbl;
};

#define dummy_ID2D1GeometrySink_QueryInterface(self,a,b)    (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1GeometrySink_AddRef(self)                (self)->vtbl->AddRef(self)
#define dummy_ID2D1GeometrySink_Release(self)               (self)->vtbl->Release(self)
#define dummy_ID2D1GeometrySink_BeginFigure(self,a,b)       (self)->vtbl->BeginFigure(self,a,b)
#define dummy_ID2D1GeometrySink_EndFigure(self,a)           (self)->vtbl->EndFigure(self,a)
#define dummy_ID2D1GeometrySink_Close(self)                 (self)->vtbl->Close(self)
#define dummy_ID2D1GeometrySink_AddLine(self,a)             (self)->vtbl->AddLine(self,a)
#define dummy_ID2D1GeometrySink_AddArc(self,a)              (self)->vtbl->AddArc(self,a)


/*****************************************
 ***  Interface ID2D1HwndRenderTarget  ***
 *****************************************/

typedef struct dummy_ID2D1HwndRenderTargetVtbl_tag dummy_ID2D1HwndRenderTargetVtbl;
struct dummy_ID2D1HwndRenderTargetVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1HwndRenderTarget*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1HwndRenderTarget*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1HwndRenderTarget*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1RenderTarget methods */
    STDMETHOD(dummy_CreateBitmap)(void);
    STDMETHOD(dummy_CreateBitmapFromWicBitmap)(void);
    STDMETHOD(dummy_CreateSharedBitmap)(void);
    STDMETHOD(dummy_CreateBitmapBrush)(void);
    STDMETHOD(dummy_CreateSolidColorBrush)(void);
    STDMETHOD(dummy_CreateGradientStopCollection)(void);
    STDMETHOD(dummy_CreateLinearGradientBrush)(void);
    STDMETHOD(dummy_CreateRadialGradientBrush)(void);
    STDMETHOD(dummy_CreateCompatibleRenderTarget)(void);
    STDMETHOD(dummy_CreateLayer)(void);
    STDMETHOD(dummy_CreateMesh)(void);
    STDMETHOD(dummy_DrawLine)(void);
    STDMETHOD(dummy_DrawRectangle)(void);
    STDMETHOD(dummy_FillRectangle)(void);
    STDMETHOD(dummy_DrawRoundedRectangle)(void);
    STDMETHOD(dummy_FillRoundedRectangle)(void);
    STDMETHOD(dummy_DrawEllipse)(void);
    STDMETHOD(dummy_FillEllipse)(void);
    STDMETHOD(dummy_DrawGeometry)(void);
    STDMETHOD(dummy_FillGeometry)(void);
    STDMETHOD(dummy_FillMesh)(void);
    STDMETHOD(dummy_FillOpacityMask)(void);
    STDMETHOD(dummy_DrawBitmap)(void);
    STDMETHOD(dummy_DrawText)(void);
    STDMETHOD(dummy_DrawTextLayout)(void);
    STDMETHOD(dummy_DrawGlyphRun)(void);
    STDMETHOD(dummy_SetTransform)(void);
    STDMETHOD(dummy_GetTransform)(void);
    STDMETHOD(dummy_SetAntialiasMode)(void);
    STDMETHOD(dummy_GetAntialiasMode)(void);
    STDMETHOD(dummy_SetTextAntialiasMode)(void);
    STDMETHOD(dummy_GetTextAntialiasMode)(void);
    STDMETHOD(dummy_SetTextRenderingParams)(void);
    STDMETHOD(dummy_GetTextRenderingParams)(void);
    STDMETHOD(dummy_SetTags)(void);
    STDMETHOD(dummy_GetTags)(void);
    STDMETHOD(dummy_PushLayer)(void);
    STDMETHOD(dummy_PopLayer)(void);
    STDMETHOD(dummy_Flush)(void);
    STDMETHOD(dummy_SaveDrawingState)(void);
    STDMETHOD(dummy_RestoreDrawingState)(void);
    STDMETHOD(dummy_PushAxisAlignedClip)(void);
    STDMETHOD(dummy_PopAxisAlignedClip)(void);
    STDMETHOD(dummy_Clear)(void);
    STDMETHOD(dummy_BeginDraw)(void);
    STDMETHOD(dummy_EndDraw)(void);
    STDMETHOD(dummy_GetPixelFormat)(void);
    STDMETHOD(dummy_SetDpi)(void);
    STDMETHOD(dummy_GetDpi)(void);
    STDMETHOD(dummy_GetSize)(void);
    STDMETHOD(dummy_GetPixelSize)(void);
    STDMETHOD(dummy_GetMaximumBitmapSize)(void);
    STDMETHOD(dummy_IsSupported)(void);

    /* ID2D1HwndRenderTarget methods */
    STDMETHOD(dummy_CheckWindowState)(void);
    STDMETHOD(Resize)(dummy_ID2D1HwndRenderTarget*, const dummy_D2D1_SIZE_U*);
    STDMETHOD(dummy_GetHwnd)(void);
};

struct dummy_ID2D1HwndRenderTarget_tag {
    dummy_ID2D1HwndRenderTargetVtbl* vtbl;
};

#define dummy_ID2D1HwndRenderTarget_QueryInterface(self,a,b)                (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1HwndRenderTarget_AddRef(self)                            (self)->vtbl->AddRef(self)
#define dummy_ID2D1HwndRenderTarget_Release(self)                           (self)->vtbl->Release(self)
#define dummy_ID2D1HwndRenderTarget_Resize(self,a)                          (self)->vtbl->Resize(self,a)


/******************************
 ***  Interface ID2D1Layer  ***
 ******************************/

typedef struct dummy_ID2D1LayerVtbl_tag dummy_ID2D1LayerVtbl;
struct dummy_ID2D1LayerVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1Layer*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1Layer*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1Layer*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1Layer methods */
    STDMETHOD(dummy_GetSize)(void);
};

struct dummy_ID2D1Layer_tag {
    dummy_ID2D1LayerVtbl* vtbl;
};

#define dummy_ID2D1Layer_QueryInterface(self,a,b)   (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1Layer_AddRef(self)               (self)->vtbl->AddRef(self)
#define dummy_ID2D1Layer_Release(self)              (self)->vtbl->Release(self)


/*************************************
 ***  Interface ID2D1PathGeometry  ***
 *************************************/

typedef struct dummy_ID2D1PathGeometryVtbl_tag dummy_ID2D1PathGeometryVtbl;
struct dummy_ID2D1PathGeometryVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1PathGeometry*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1PathGeometry*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1PathGeometry*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1Geometry methods */
    STDMETHOD(dummy_GetBounds)(void);
    STDMETHOD(dummy_GetWidenedBounds)(void);
    STDMETHOD(dummy_StrokeContainsPoint)(void);
    STDMETHOD(dummy_FillContainsPoint)(void);
    STDMETHOD(dummy_CompareWithGeometry)(void);
    STDMETHOD(dummy_Simplify)(void);
    STDMETHOD(dummy_Tessellate)(void);
    STDMETHOD(dummy_CombineWithGeometry)(void);
    STDMETHOD(dummy_Outline)(void);
    STDMETHOD(dummy_ComputeArea)(void);
    STDMETHOD(dummy_ComputeLength)(void);
    STDMETHOD(dummy_ComputePointAtLength)(void);
    STDMETHOD(dummy_Widen)(void);

    /* ID2D1PathGeometry methods */
    STDMETHOD(Open)(dummy_ID2D1PathGeometry*, dummy_ID2D1GeometrySink**);
    STDMETHOD(dummy_Stream)(void);
    STDMETHOD(dummy_GetSegmentCount)(void);
    STDMETHOD(dummy_GetFigureCount)(void);
};

struct dummy_ID2D1PathGeometry_tag {
    dummy_ID2D1PathGeometryVtbl* vtbl;
};

#define dummy_ID2D1PathGeometry_QueryInterface(self,a,b)    (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1PathGeometry_AddRef(self)                (self)->vtbl->AddRef(self)
#define dummy_ID2D1PathGeometry_Release(self)               (self)->vtbl->Release(self)
#define dummy_ID2D1PathGeometry_Open(self,a)                (self)->vtbl->Open(self,a)


/*************************************
 ***  Interface ID2D1RenderTarget  ***
 *************************************/

typedef struct dummy_ID2D1RenderTargetVtbl_tag dummy_ID2D1RenderTargetVtbl;
struct dummy_ID2D1RenderTargetVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1RenderTarget*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1RenderTarget*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1RenderTarget*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1RenderTarget methods */
    STDMETHOD(dummy_CreateBitmap)(void);
    STDMETHOD(CreateBitmapFromWicBitmap)(dummy_ID2D1RenderTarget*, IWICBitmapSource*, const dummy_D2D1_BITMAP_PROPERTIES*, dummy_ID2D1Bitmap**);
    STDMETHOD(dummy_CreateSharedBitmap)(void);
    STDMETHOD(dummy_CreateBitmapBrush)(void);
    STDMETHOD(CreateSolidColorBrush)(dummy_ID2D1RenderTarget*, const dummy_D2D1_COLOR_F*, const void*, dummy_ID2D1SolidColorBrush**);
    STDMETHOD(dummy_CreateGradientStopCollection)(void);
    STDMETHOD(dummy_CreateLinearGradientBrush)(void);
    STDMETHOD(dummy_CreateRadialGradientBrush)(void);
    STDMETHOD(dummy_CreateCompatibleRenderTarget)(void);
    STDMETHOD(CreateLayer)(dummy_ID2D1RenderTarget*, const dummy_D2D1_SIZE_F*, dummy_ID2D1Layer**);
    STDMETHOD(dummy_CreateMesh)(void);
    STDMETHOD_(void, DrawLine)(dummy_ID2D1RenderTarget*, dummy_D2D1_POINT_2F, dummy_D2D1_POINT_2F, dummy_ID2D1Brush*, FLOAT, dummy_ID2D1StrokeStyle*);
    STDMETHOD_(void, DrawRectangle)(dummy_ID2D1RenderTarget*, const dummy_D2D1_RECT_F*, dummy_ID2D1Brush*, FLOAT, dummy_ID2D1StrokeStyle*);
    STDMETHOD_(void, FillRectangle)(dummy_ID2D1RenderTarget*, const dummy_D2D1_RECT_F*, dummy_ID2D1Brush*);
    STDMETHOD(dummy_DrawRoundedRectangle)(void);
    STDMETHOD(dummy_FillRoundedRectangle)(void);
    STDMETHOD_(void, DrawEllipse)(dummy_ID2D1RenderTarget*, const dummy_D2D1_ELLIPSE*, dummy_ID2D1Brush*, FLOAT, dummy_ID2D1StrokeStyle*);
    STDMETHOD_(void, FillEllipse)(dummy_ID2D1RenderTarget*, const dummy_D2D1_ELLIPSE*, dummy_ID2D1Brush*);
    STDMETHOD_(void, DrawGeometry)(dummy_ID2D1RenderTarget*, dummy_ID2D1Geometry*, dummy_ID2D1Brush*, FLOAT, dummy_ID2D1StrokeStyle*);
    STDMETHOD_(void, FillGeometry)(dummy_ID2D1RenderTarget*, dummy_ID2D1Geometry*, dummy_ID2D1Brush*, dummy_ID2D1Brush*);
    STDMETHOD(dummy_FillMesh)(void);
    STDMETHOD(dummy_FillOpacityMask)(void);
    STDMETHOD_(void, DrawBitmap)(dummy_ID2D1RenderTarget*, dummy_ID2D1Bitmap*, const dummy_D2D1_RECT_F*, FLOAT,
                                 dummy_D2D1_BITMAP_INTERPOLATION_MODE, const dummy_D2D1_RECT_F*);
    STDMETHOD(dummy_DrawText)(void);
    STDMETHOD_(void, DrawTextLayout)(dummy_ID2D1RenderTarget*, dummy_D2D1_POINT_2F, dummy_IDWriteTextLayout*, dummy_ID2D1Brush*, unsigned);
    STDMETHOD(dummy_DrawGlyphRun)(void);
    STDMETHOD_(void, SetTransform)(dummy_ID2D1RenderTarget*, const dummy_D2D1_MATRIX_3X2_F*);
    STDMETHOD_(void, GetTransform)(dummy_ID2D1RenderTarget*, dummy_D2D1_MATRIX_3X2_F*);
    STDMETHOD(dummy_SetAntialiasMode)(void);
    STDMETHOD(dummy_GetAntialiasMode)(void);
    STDMETHOD(SetTextAntialiasMode)(dummy_ID2D1RenderTarget*, dummy_D2D1_TEXT_ANTIALIAS_MODE);
    STDMETHOD(dummy_GetTextAntialiasMode)(void);
    STDMETHOD(dummy_SetTextRenderingParams)(void);
    STDMETHOD(dummy_GetTextRenderingParams)(void);
    STDMETHOD(dummy_SetTags)(void);
    STDMETHOD(dummy_GetTags)(void);
    STDMETHOD_(void, PushLayer)(dummy_ID2D1RenderTarget*, const dummy_D2D1_LAYER_PARAMETERS*, dummy_ID2D1Layer*);
    STDMETHOD_(void, PopLayer)(dummy_ID2D1RenderTarget*);
    STDMETHOD(dummy_Flush)(void);
    STDMETHOD(dummy_SaveDrawingState)(void);
    STDMETHOD(dummy_RestoreDrawingState)(void);
    STDMETHOD_(void, PushAxisAlignedClip)(dummy_ID2D1RenderTarget*, const dummy_D2D1_RECT_F*, dummy_D2D1_ANTIALIAS_MODE);
    STDMETHOD_(void, PopAxisAlignedClip)(dummy_ID2D1RenderTarget*);
    STDMETHOD_(void, Clear)(dummy_ID2D1RenderTarget*, const dummy_D2D1_COLOR_F*);
    STDMETHOD_(void, BeginDraw)(dummy_ID2D1RenderTarget*);
    STDMETHOD(EndDraw)(dummy_ID2D1RenderTarget*, void*, void*);
    STDMETHOD(dummy_GetPixelFormat)(void);
    STDMETHOD_(void, SetDpi)(dummy_ID2D1RenderTarget*, FLOAT, FLOAT);
    STDMETHOD(dummy_GetDpi)(void);
    STDMETHOD(dummy_GetSize)(void);
    STDMETHOD(dummy_GetPixelSize)(void);
    STDMETHOD(dummy_GetMaximumBitmapSize)(void);
    STDMETHOD(dummy_IsSupported)(void);
};

struct dummy_ID2D1RenderTarget_tag {
    dummy_ID2D1RenderTargetVtbl* vtbl;
};

#define dummy_ID2D1RenderTarget_QueryInterface(self,a,b)                (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1RenderTarget_AddRef(self)                            (self)->vtbl->AddRef(self)
#define dummy_ID2D1RenderTarget_Release(self)                           (self)->vtbl->Release(self)
#define dummy_ID2D1RenderTarget_CreateBitmapFromWicBitmap(self,a,b,c)   (self)->vtbl->CreateBitmapFromWicBitmap(self,a,b,c)
#define dummy_ID2D1RenderTarget_CreateSolidColorBrush(self,a,b,c)       (self)->vtbl->CreateSolidColorBrush(self,a,b,c)
#define dummy_ID2D1RenderTarget_CreateLayer(self,a,b)                   (self)->vtbl->CreateLayer(self,a,b)
#define dummy_ID2D1RenderTarget_DrawLine(self,a,b,c,d,e)                (self)->vtbl->DrawLine(self,a,b,c,d,e)
#define dummy_ID2D1RenderTarget_DrawRectangle(self,a,b,c,d)             (self)->vtbl->DrawRectangle(self,a,b,c,d)
#define dummy_ID2D1RenderTarget_FillRectangle(self,a,b)                 (self)->vtbl->FillRectangle(self,a,b)
#define dummy_ID2D1RenderTarget_DrawEllipse(self,a,b,c,d)               (self)->vtbl->DrawEllipse(self,a,b,c,d)
#define dummy_ID2D1RenderTarget_FillEllipse(self,a,b)                   (self)->vtbl->FillEllipse(self,a,b)
#define dummy_ID2D1RenderTarget_DrawGeometry(self,a,b,c,d)              (self)->vtbl->DrawGeometry(self,a,b,c,d)
#define dummy_ID2D1RenderTarget_FillGeometry(self,a,b,c)                (self)->vtbl->FillGeometry(self,a,b,c)
#define dummy_ID2D1RenderTarget_DrawBitmap(self,a,b,c,d,e)              (self)->vtbl->DrawBitmap(self,a,b,c,d,e)
#define dummy_ID2D1RenderTarget_DrawTextLayout(self,a,b,c,d)            (self)->vtbl->DrawTextLayout(self,a,b,c,d)
#define dummy_ID2D1RenderTarget_SetTransform(self,a)                    (self)->vtbl->SetTransform(self,a)
#define dummy_ID2D1RenderTarget_GetTransform(self,a)                    (self)->vtbl->GetTransform(self,a)
#define dummy_ID2D1RenderTarget_PushLayer(self,a,b)                     (self)->vtbl->PushLayer(self,a,b)
#define dummy_ID2D1RenderTarget_PopLayer(self)                          (self)->vtbl->PopLayer(self)
#define dummy_ID2D1RenderTarget_PushAxisAlignedClip(self,a,b)           (self)->vtbl->PushAxisAlignedClip(self,a,b)
#define dummy_ID2D1RenderTarget_PopAxisAlignedClip(self)                (self)->vtbl->PopAxisAlignedClip(self)
#define dummy_ID2D1RenderTarget_Clear(self,a)                           (self)->vtbl->Clear(self,a)
#define dummy_ID2D1RenderTarget_BeginDraw(self)                         (self)->vtbl->BeginDraw(self)
#define dummy_ID2D1RenderTarget_EndDraw(self,a,b)                       (self)->vtbl->EndDraw(self,a,b)
#define dummy_ID2D1RenderTarget_SetDpi(self,a,b)                        (self)->vtbl->SetDpi(self,a,b)
#define dummy_ID2D1RenderTarget_SetTextAntialiasMode(self,a)            (self)->vtbl->SetTextAntialiasMode(self,a)


/*********************************************
 ***  Interface ID2D1SolidColorBrushBrush  ***
 *********************************************/

typedef struct dummy_ID2D1SolidColorBrushVtbl_tag dummy_ID2D1SolidColorBrushVtbl;
struct dummy_ID2D1SolidColorBrushVtbl_tag {
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(dummy_ID2D1SolidColorBrush*, REFIID, void**);
    STDMETHOD_(ULONG, AddRef)(dummy_ID2D1SolidColorBrush*);
    STDMETHOD_(ULONG, Release)(dummy_ID2D1SolidColorBrush*);

    /* ID2D1Resource methods */
    STDMETHOD(dummy_GetFactory)(void);

    /* ID2D1Brush methods */
    STDMETHOD(dummy_SetOpacity)(void);
    STDMETHOD(dummy_SetTransform)(void);
    STDMETHOD(dummy_GetOpacity)(void);
    STDMETHOD(dummy_GetTransform)(void);

    /* ID2D1SolidColorBrushBrush methods */
    STDMETHOD_(void, SetColor)(dummy_ID2D1SolidColorBrush*, const dummy_D2D1_COLOR_F*);
    STDMETHOD(dummy_GetColor)(void);
};

struct dummy_ID2D1SolidColorBrush_tag {
    dummy_ID2D1SolidColorBrushVtbl* vtbl;
};

#define dummy_ID2D1SolidColorBrush_QueryInterface(self,a,b)     (self)->vtbl->QueryInterface(self,a,b)
#define dummy_ID2D1SolidColorBrush_AddRef(self)                 (self)->vtbl->AddRef(self)
#define dummy_ID2D1SolidColorBrush_Release(self)                (self)->vtbl->Release(self)
#define dummy_ID2D1SolidColorBrush_SetColor(self,a)             (self)->vtbl->SetColor(self,a)


#endif  /* DUMMY_D2D1_H */
