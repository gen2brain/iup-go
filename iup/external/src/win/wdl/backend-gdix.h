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

#ifndef WD_BACKEND_GDIX_H
#define WD_BACKEND_GDIX_H

#include "misc.h"
#include "dummy/gdiplus.h"


typedef struct gdix_strokestyle_tag gdix_strokestyle_t;
struct gdix_strokestyle_tag {
  dummy_GpLineCap lineCap;
  dummy_GpLineJoin lineJoin;
  dummy_GpDashStyle dashStyle;
  UINT dashesCount;
  float dashes[1];
};

typedef struct gdix_canvas_tag gdix_canvas_t;
struct gdix_canvas_tag {
    HDC dc;
    dummy_GpGraphics* graphics;
    dummy_GpPen* pen;
    dummy_GpStringFormat* string_format;
    int dc_layout;
    UINT width  : 31;
    UINT rtl    :  1;

    HDC real_dc;        /* non-NULL if double buffering is enabled. */
    HBITMAP orig_bmp;
    int x;
    int y;
    int cx;
    int cy;
};


typedef struct gdix_vtable_tag gdix_vtable_t;
struct gdix_vtable_tag {
    /* Graphics functions */
    int (WINAPI* fn_CreateFromHDC)(HDC, dummy_GpGraphics**);
    int (WINAPI* fn_DeleteGraphics)(dummy_GpGraphics*);
    int (WINAPI* fn_GraphicsClear)(dummy_GpGraphics*, dummy_ARGB);
    int (WINAPI* fn_GetDC)(dummy_GpGraphics*, HDC*);
    int (WINAPI* fn_ReleaseDC)(dummy_GpGraphics*, HDC);
    int (WINAPI* fn_ResetClip)(dummy_GpGraphics*);
    int (WINAPI* fn_ResetWorldTransform)(dummy_GpGraphics*);
    int (WINAPI* fn_RotateWorldTransform)(dummy_GpGraphics*, float, dummy_GpMatrixOrder);
    int (WINAPI* fn_ScaleWorldTransform)(dummy_GpGraphics*, float, float, dummy_GpMatrixOrder);
    int (WINAPI* fn_SetClipPath)(dummy_GpGraphics*, dummy_GpPath*, dummy_GpCombineMode);
    int (WINAPI* fn_SetClipRect)(dummy_GpGraphics*, float, float, float, float, dummy_GpCombineMode);
    int (WINAPI* fn_SetPageUnit)(dummy_GpGraphics*, dummy_GpUnit);
    int (WINAPI* fn_SetPixelOffsetMode)(dummy_GpGraphics*, dummy_GpPixelOffsetMode);
    int (WINAPI* fn_SetSmoothingMode)(dummy_GpGraphics*, dummy_GpSmoothingMode);
    int (WINAPI* fn_TranslateWorldTransform)(dummy_GpGraphics*, float, float, dummy_GpMatrixOrder);

    /* Brush functions */
    int (WINAPI* fn_CreateSolidFill)(dummy_ARGB, dummy_GpSolidFill**);
    int (WINAPI* fn_DeleteBrush)(dummy_GpBrush*);
    int (WINAPI* fn_SetSolidFillColor)(dummy_GpSolidFill*, dummy_ARGB);

    /* Pen functions */
    int (WINAPI* fn_CreatePen1)(dummy_ARGB, float, dummy_GpUnit, dummy_GpPen**);
    int (WINAPI* fn_DeletePen)(dummy_GpPen*);
    int (WINAPI* fn_SetPenBrushFill)(dummy_GpPen*, dummy_GpBrush*);
    int (WINAPI* fn_SetPenWidth)(dummy_GpPen*, float);
    int (WINAPI* fn_SetPenStartCap)(dummy_GpPen*, dummy_GpLineCap);
    int (WINAPI* fn_SetPenEndCap)(dummy_GpPen*, dummy_GpLineCap);
    int (WINAPI* fn_SetPenLineJoin)(dummy_GpPen*, dummy_GpLineJoin);
    int (WINAPI* fn_SetPenMiterLimit)(dummy_GpPen*, float);
    int (WINAPI* fn_SetPenDashStyle)(dummy_GpPen*, dummy_GpDashStyle);
    int (WINAPI* fn_SetPenDashArray)(dummy_GpPen*, const float*, INT);
    int (WINAPI* fn_SetPenDashOffset)(dummy_GpPen*, float);

    /* Path functions */
    int (WINAPI* fn_CreatePath)(dummy_GpFillMode, dummy_GpPath**);
    int (WINAPI* fn_DeletePath)(dummy_GpPath*);
    int (WINAPI* fn_ClosePathFigure)(dummy_GpPath*);
    int (WINAPI* fn_StartPathFigure)(dummy_GpPath*);
    int (WINAPI* fn_GetPathLastPoint)(dummy_GpPath*, dummy_GpPointF*);
    int (WINAPI* fn_AddPathArc)(dummy_GpPath*, float, float, float, float, float, float);
    int (WINAPI* fn_AddPathLine)(dummy_GpPath*, float, float, float, float);

    /* Font functions */
    int (WINAPI* fn_CreateFontFromLogfontW)(HDC, const LOGFONTW*, dummy_GpFont**);
    int (WINAPI* fn_DeleteFont)(dummy_GpFont*);
    int (WINAPI* fn_DeleteFontFamily)(dummy_GpFont*);
    int (WINAPI* fn_GetCellAscent)(const dummy_GpFont*, int, UINT16*);
    int (WINAPI* fn_GetCellDescent)(const dummy_GpFont*, int, UINT16*);
    int (WINAPI* fn_GetEmHeight)(const dummy_GpFont*, int, UINT16*);
    int (WINAPI* fn_GetFamily)(dummy_GpFont*, void**);
    int (WINAPI* fn_GetFontSize)(dummy_GpFont*, float*);
    int (WINAPI* fn_GetFontStyle)(dummy_GpFont*, int*);
    int (WINAPI* fn_GetLineSpacing)(const dummy_GpFont*, int, UINT16*);

    /* Image & bitmap functions */
    int (WINAPI* fn_LoadImageFromFile)(const WCHAR*, dummy_GpImage**);
    int (WINAPI* fn_LoadImageFromStream)(IStream*, dummy_GpImage**);
    int (WINAPI* fn_CreateBitmapFromHBITMAP)(HBITMAP, HPALETTE, dummy_GpBitmap**);
    int (WINAPI* fn_CreateBitmapFromHICON)(HICON, dummy_GpBitmap**);
    int (WINAPI* fn_DisposeImage)(dummy_GpImage*);
    int (WINAPI* fn_GetImageWidth)(dummy_GpImage*, UINT*);
    int (WINAPI* fn_GetImageHeight)(dummy_GpImage*, UINT*);
    int (WINAPI* fn_CreateBitmapFromScan0)(UINT, UINT, INT, dummy_GpPixelFormat, BYTE*, dummy_GpBitmap**);
    int (WINAPI* fn_BitmapLockBits)(dummy_GpBitmap*, const dummy_GpRectI*, UINT, dummy_GpPixelFormat, dummy_GpBitmapData*);
    int (WINAPI* fn_BitmapUnlockBits)(dummy_GpBitmap*, dummy_GpBitmapData*);
    int (WINAPI* fn_CreateBitmapFromGdiDib)(const BITMAPINFO*, void*, dummy_GpBitmap**);

    /* Cached bitmap functions */
    int (WINAPI* fn_CreateCachedBitmap)(dummy_GpBitmap*, dummy_GpGraphics*, dummy_GpCachedBitmap**);
    int (WINAPI* fn_DeleteCachedBitmap)(dummy_GpCachedBitmap*);
    int (WINAPI* fn_DrawCachedBitmap)(dummy_GpGraphics*, dummy_GpCachedBitmap*, INT, INT);

    /* String format functions */
    int (WINAPI* fn_CreateStringFormat)(int, LANGID, dummy_GpStringFormat**);
    int (WINAPI* fn_DeleteStringFormat)(dummy_GpStringFormat*);
    int (WINAPI* fn_SetStringFormatAlign)(dummy_GpStringFormat*, dummy_GpStringAlignment);
    int (WINAPI* fn_SetStringFormatLineAlign)(dummy_GpStringFormat*, dummy_GpStringAlignment);
    int (WINAPI* fn_SetStringFormatFlags)(dummy_GpStringFormat*, int);
    int (WINAPI* fn_SetStringFormatTrimming)(dummy_GpStringFormat*, dummy_GpStringTrimming);

    /* Draw/fill functions */
    int (WINAPI* fn_DrawArc)(dummy_GpGraphics*, dummy_GpPen*, float, float, float, float, float, float);
    int (WINAPI* fn_DrawImageRectRect)(dummy_GpGraphics*, dummy_GpImage*, float, float, float, float, float, float, float, float, dummy_GpUnit, const void*, void*, void*);
    int (WINAPI* fn_DrawEllipse)(dummy_GpGraphics*, dummy_GpPen*, float, float, float, float);
    int (WINAPI* fn_DrawLine)(dummy_GpGraphics*, dummy_GpPen*, float, float, float, float);
    int (WINAPI* fn_DrawPath)(dummy_GpGraphics*, dummy_GpPen*, dummy_GpPath*);
    int (WINAPI* fn_DrawPie)(dummy_GpGraphics*, dummy_GpPen*, float, float, float, float, float, float);
    int (WINAPI* fn_DrawRectangle)(dummy_GpGraphics*, void*, float, float, float, float);
    int (WINAPI* fn_DrawString)(dummy_GpGraphics*, const WCHAR*, int, const dummy_GpFont*, const dummy_GpRectF*, const dummy_GpStringFormat*, const dummy_GpBrush*);
    int (WINAPI* fn_FillEllipse)(dummy_GpGraphics*, dummy_GpBrush*, float, float, float, float);
    int (WINAPI* fn_FillPath)(dummy_GpGraphics*, dummy_GpBrush*, dummy_GpPath*);
    int (WINAPI* fn_FillPie)(dummy_GpGraphics*, dummy_GpBrush*, float, float, float, float, float, float);
    int (WINAPI* fn_FillRectangle)(dummy_GpGraphics*, void*, float, float, float, float);
    int (WINAPI* fn_MeasureString)(dummy_GpGraphics*, const WCHAR*, int, const dummy_GpFont*, const dummy_GpRectF*, const dummy_GpStringFormat*, dummy_GpRectF*, int*, int*);
};


extern gdix_vtable_t* gdix_vtable;

static inline BOOL
gdix_enabled(void)
{
    return (gdix_vtable != NULL);
}


int gdix_init(void);
void gdix_fini(void);


/* Helpers */
gdix_canvas_t* gdix_canvas_alloc(HDC dc, const RECT* doublebuffer_rect, UINT width, BOOL rtl);
void gdix_rtl_transform(gdix_canvas_t* c);
void gdix_reset_transform(gdix_canvas_t* c);
void gdix_canvas_apply_string_flags(gdix_canvas_t* c, DWORD flags);
void gdix_setpen(dummy_GpPen* pen, dummy_GpBrush* brush, float width, gdix_strokestyle_t* style);
int gdix_create_bitmap(HBITMAP hBmp, dummy_GpBitmap* *bitmap);


#endif  /* WD_BACKEND_GDIX_H */
