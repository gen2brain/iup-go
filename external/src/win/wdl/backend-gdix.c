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

#include "backend-gdix.h"


#ifdef _MSC_VER
    /* warning C4996: 'GetVersionExW': was declared deprecated */
    #pragma warning( disable : 4996 )
#endif


static HMODULE gdix_dll = NULL;

static ULONG_PTR gdix_token;
static void (WINAPI* gdix_Shutdown)(ULONG_PTR);

gdix_vtable_t* gdix_vtable = NULL;


int
gdix_init(void)
{
    int (WINAPI* gdix_Startup)(ULONG_PTR*, const dummy_GpStartupInput*, void*);
    dummy_GpStartupInput input = { 0 };
    int status;

    gdix_dll = wd_load_system_dll(_T("GDIPLUS.DLL"));
    if(gdix_dll == NULL) {
        /* On Windows 2000, we may need to use redistributable version of
         * GDIPLUS.DLL packaged with the application as GDI+.DLL is not part
         * of vanilla system. (However it MAY be present. Some later updates,
         * versions of MSIE or other software by Microsoft install it.) */
        OSVERSIONINFO version;
        version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&version);
        if(version.dwMajorVersion == 5 && version.dwMinorVersion == 0) {
            gdix_dll = LoadLibrary(_T("GDIPLUS.DLL"));
            if(gdix_dll == NULL) {
                WD_TRACE_ERR("gdix_init: LoadLibrary(GDIPLUS.DLL) failed");
                goto err_LoadLibrary;
            }
        } else {
            WD_TRACE_ERR("gdix_init: wd_load_system_dll(GDIPLUS.DLL) failed");
            goto err_LoadLibrary;
        }
    }

    gdix_vtable = (gdix_vtable_t*) malloc(sizeof(gdix_vtable_t));
    if(gdix_vtable == NULL) {
        WD_TRACE("gdix_init: malloc() failed.");
        goto err_malloc;
    }


    gdix_Startup = (int (WINAPI*)(ULONG_PTR*, const dummy_GpStartupInput*, void*))
                        GetProcAddress(gdix_dll, "GdiplusStartup");
    if(gdix_Startup == NULL) {
        WD_TRACE_ERR("gdix_init: GetProcAddress(GdiplusStartup) failed");
        goto err_GetProcAddress;
    }

    gdix_Shutdown = (void (WINAPI*)(ULONG_PTR))
                        GetProcAddress(gdix_dll, "GdiplusShutdown");
    if(gdix_Shutdown == NULL) {
        WD_TRACE_ERR("gdix_init: GetProcAddress(GdiplusShutdown) failed");
        goto err_GetProcAddress;
    }

#define GPA(name, params)                                                      \
        do {                                                                   \
            gdix_vtable->fn_##name = (int (WINAPI*)params)                     \
                        GetProcAddress(gdix_dll, "Gdip"#name);                 \
            if(gdix_vtable->fn_##name == NULL) {                               \
                WD_TRACE_ERR("gdix_init: GetProcAddress(Gdip"#name") failed"); \
                goto err_GetProcAddress;                                       \
            }                                                                  \
        } while(0)

    /* Graphics functions */
    GPA(CreateFromHDC, (HDC, dummy_GpGraphics**));
    GPA(DeleteGraphics, (dummy_GpGraphics*));
    GPA(GraphicsClear, (dummy_GpGraphics*, dummy_ARGB));
    GPA(GetDC, (dummy_GpGraphics*, HDC*));
    GPA(ReleaseDC, (dummy_GpGraphics*, HDC));
    GPA(ResetClip, (dummy_GpGraphics*));
    GPA(ResetWorldTransform, (dummy_GpGraphics*));
    GPA(RotateWorldTransform, (dummy_GpGraphics*, float, dummy_GpMatrixOrder));
    GPA(ScaleWorldTransform, (dummy_GpGraphics*, float, float, dummy_GpMatrixOrder));
    GPA(SetClipPath, (dummy_GpGraphics*, dummy_GpPath*, dummy_GpCombineMode));
    GPA(SetClipRect, (dummy_GpGraphics*, float, float, float, float, dummy_GpCombineMode));
    GPA(SetPageUnit, (dummy_GpGraphics*, dummy_GpUnit));
    GPA(SetPixelOffsetMode, (dummy_GpGraphics*, dummy_GpPixelOffsetMode));
    GPA(SetSmoothingMode, (dummy_GpGraphics*, dummy_GpSmoothingMode));
    GPA(TranslateWorldTransform, (dummy_GpGraphics*, float, float, dummy_GpMatrixOrder));

    /* Brush functions */
    GPA(CreateSolidFill, (dummy_ARGB, dummy_GpSolidFill**));
    GPA(DeleteBrush, (dummy_GpBrush*));
    GPA(SetSolidFillColor, (dummy_GpSolidFill*, dummy_ARGB));

    /* Pen functions */
    GPA(CreatePen1, (DWORD, float, dummy_GpUnit, dummy_GpPen**));
    GPA(DeletePen, (dummy_GpPen*));
    GPA(SetPenBrushFill, (dummy_GpPen*, dummy_GpBrush*));
    GPA(SetPenWidth, (dummy_GpPen*, float));
    GPA(SetPenStartCap, (dummy_GpPen*, dummy_GpLineCap));
    GPA(SetPenEndCap, (dummy_GpPen*, dummy_GpLineCap));
    GPA(SetPenLineJoin, (dummy_GpPen*, dummy_GpLineJoin));
    GPA(SetPenMiterLimit, (dummy_GpPen*, float));
    GPA(SetPenDashStyle, (dummy_GpPen*, dummy_GpDashStyle));
    GPA(SetPenDashArray, (dummy_GpPen*, const float*, INT));

    /* Path functions */
    GPA(CreatePath, (dummy_GpFillMode, dummy_GpPath**));
    GPA(DeletePath, (dummy_GpPath*));
    GPA(ClosePathFigure, (dummy_GpPath*));
    GPA(StartPathFigure, (dummy_GpPath*));
    GPA(GetPathLastPoint, (dummy_GpPath*, dummy_GpPointF*));
    GPA(AddPathArc, (dummy_GpPath*, float, float, float, float, float, float));
    GPA(AddPathLine, (dummy_GpPath*, float, float, float, float));
    GPA(SetPenDashOffset, (dummy_GpPen*, float));

    /* Font functions */
    GPA(CreateFontFromLogfontW, (HDC, const LOGFONTW*, dummy_GpFont**));
    GPA(DeleteFont, (dummy_GpFont*));
    GPA(DeleteFontFamily, (dummy_GpFont*));
    GPA(GetCellAscent, (const dummy_GpFont*, int, UINT16*));
    GPA(GetCellDescent, (const dummy_GpFont*, int, UINT16*));
    GPA(GetEmHeight, (const dummy_GpFont*, int, UINT16*));
    GPA(GetFamily, (dummy_GpFont*, void**));
    GPA(GetFontSize, (dummy_GpFont*, float*));
    GPA(GetFontStyle, (dummy_GpFont*, int*));
    GPA(GetLineSpacing, (const dummy_GpFont*, int, UINT16*));

    /* Image & bitmap functions */
    GPA(LoadImageFromFile, (const WCHAR*, dummy_GpImage**));
    GPA(LoadImageFromStream, (IStream*, dummy_GpImage**));
    GPA(CreateBitmapFromHBITMAP, (HBITMAP, HPALETTE, dummy_GpBitmap**));
    GPA(CreateBitmapFromHICON, (HICON, dummy_GpBitmap**));
    GPA(DisposeImage, (dummy_GpImage*));
    GPA(GetImageWidth, (dummy_GpImage*, UINT*));
    GPA(GetImageHeight, (dummy_GpImage*, UINT*));
    GPA(CreateBitmapFromScan0, (UINT, UINT, INT, dummy_GpPixelFormat format, BYTE*, dummy_GpBitmap**));
    GPA(BitmapLockBits, (dummy_GpBitmap*, const dummy_GpRectI*, UINT, dummy_GpPixelFormat, dummy_GpBitmapData*));
    GPA(BitmapUnlockBits, (dummy_GpBitmap*, dummy_GpBitmapData*));
    GPA(CreateBitmapFromGdiDib, (const BITMAPINFO*, void*, dummy_GpBitmap**));

    /* Cached bitmap functions */
    GPA(CreateCachedBitmap, (dummy_GpBitmap*, dummy_GpGraphics*, dummy_GpCachedBitmap**));
    GPA(DeleteCachedBitmap, (dummy_GpCachedBitmap*));
    GPA(DrawCachedBitmap, (dummy_GpGraphics*, dummy_GpCachedBitmap*, INT, INT));

    /* String format functions */
    GPA(CreateStringFormat, (int, LANGID, dummy_GpStringFormat**));
    GPA(DeleteStringFormat, (dummy_GpStringFormat*));
    GPA(SetStringFormatAlign, (dummy_GpStringFormat*, dummy_GpStringAlignment));
    GPA(SetStringFormatLineAlign, (dummy_GpStringFormat*, dummy_GpStringAlignment));
    GPA(SetStringFormatFlags, (dummy_GpStringFormat*, int));
    GPA(SetStringFormatTrimming, (dummy_GpStringFormat*, dummy_GpStringTrimming));

    /* Draw/fill functions */
    GPA(DrawArc, (dummy_GpGraphics*, dummy_GpPen*, float, float, float, float, float, float));
    GPA(DrawImageRectRect, (dummy_GpGraphics*, dummy_GpImage*, float, float, float, float, float, float, float, float, dummy_GpUnit, const void*, void*, void*));
    GPA(DrawEllipse, (dummy_GpGraphics*, dummy_GpPen*, float, float, float, float));
    GPA(DrawLine, (dummy_GpGraphics*, dummy_GpPen*, float, float, float, float));
    GPA(DrawPath, (dummy_GpGraphics*, dummy_GpPen*, dummy_GpPath*));
    GPA(DrawPie, (dummy_GpGraphics*, dummy_GpPen*, float, float, float, float, float, float));
    GPA(DrawRectangle, (dummy_GpGraphics*, void*, float, float, float, float));
    GPA(DrawString, (dummy_GpGraphics*, const WCHAR*, int, const dummy_GpFont*, const dummy_GpRectF*, const dummy_GpStringFormat*, const dummy_GpBrush*));
    GPA(FillEllipse, (dummy_GpGraphics*, dummy_GpBrush*, float, float, float, float));
    GPA(FillPath, (dummy_GpGraphics*, dummy_GpBrush*, dummy_GpPath*));
    GPA(FillPie, (dummy_GpGraphics*, dummy_GpBrush*, float, float, float, float, float, float));
    GPA(FillRectangle, (dummy_GpGraphics*, void*, float, float, float, float));
    GPA(MeasureString, (dummy_GpGraphics*, const WCHAR*, int, const dummy_GpFont*, const dummy_GpRectF*, const dummy_GpStringFormat*, dummy_GpRectF*, int*, int*));

#undef GPA

    input.GdiplusVersion = 1;
    input.SuppressExternalCodecs = TRUE;
    status = gdix_Startup(&gdix_token, &input, NULL);
    if(status != 0) {
        WD_TRACE("GdiplusStartup() failed. [%d]", status);
        goto err_Startup;
    }

    /* Success */
    return 0;

    /* Error path */
err_Startup:
err_GetProcAddress:
    free(gdix_vtable);
    gdix_vtable = NULL;
err_malloc:
    FreeLibrary(gdix_dll);
    gdix_dll = NULL;
err_LoadLibrary:
    return -1;
}

void
gdix_fini(void)
{
    free(gdix_vtable);
    gdix_vtable = NULL;

    gdix_Shutdown(gdix_token);

    FreeLibrary(gdix_dll);
    gdix_dll = NULL;
}

gdix_canvas_t*
gdix_canvas_alloc(HDC dc, const RECT* doublebuffer_rect, UINT width, BOOL rtl)
{
    gdix_canvas_t* c;
    int status;

    c = (gdix_canvas_t*) malloc(sizeof(gdix_canvas_t));
    if(c == NULL) {
        WD_TRACE("gdix_canvas_alloc: malloc() failed.");
        goto err_malloc;
    }

    memset(c, 0, sizeof(gdix_canvas_t));
    c->width = width;
    c->rtl = (rtl ? TRUE : FALSE);

    if(doublebuffer_rect != NULL) {
        int cx = doublebuffer_rect->right - doublebuffer_rect->left;
        int cy = doublebuffer_rect->bottom - doublebuffer_rect->top;
        HDC mem_dc;
        HBITMAP mem_bmp;

        mem_dc = CreateCompatibleDC(dc);
        if(mem_dc == NULL) {
            WD_TRACE_ERR("gdix_canvas_alloc: CreateCompatibleDC() failed.");
            DeleteDC(mem_dc);
            goto no_doublebuffer;
        }
        SetLayout(mem_dc, 0);
        mem_bmp = CreateCompatibleBitmap(dc, cx, cy);
        if(mem_bmp == NULL) {
            DeleteObject(mem_dc);
            WD_TRACE_ERR("gdix_canvas_alloc: CreateCompatibleBitmap() failed.");
            goto no_doublebuffer;
        }

        c->dc = mem_dc;
        c->real_dc = dc;
        c->orig_bmp = SelectObject(mem_dc, mem_bmp);
        c->x = (GetLayout(dc) & LAYOUT_RTL)
                 ? width - 1 - doublebuffer_rect->right
                 : doublebuffer_rect->left;
        c->y = doublebuffer_rect->top;
        c->cx = doublebuffer_rect->right - doublebuffer_rect->left;
        c->cy = doublebuffer_rect->bottom - doublebuffer_rect->top;
        SetViewportOrgEx(mem_dc, -c->x, -c->y, NULL);
    } else {
no_doublebuffer:
        c->dc = dc;
    }

    /* Different GDIPLUS.DLL versions treat RTL very differently.
     *
     * E.g. on Win 2000 and XP, it installs a reflection transformation is
     * that origin is at right top corner of the window, but this reflection
     * applies also on text (wdDrawString()).
     *
     * Windows 7 or 10 seem to ignore the RTL layout altogether.
     *
     * Hence we enforce left-to-right layout to get consistent behavior from
     * GDIPLUS.DLL and implement RTL manually on top of it.
     */
    c->dc_layout = SetLayout(dc, 0);

    status = gdix_vtable->fn_CreateFromHDC(c->dc, &c->graphics);
    if(status != 0) {
        WD_TRACE_ERR_("gdix_canvas_alloc: GdipCreateFromHDC() failed.", status);
        goto err_creategraphics;
    }

    status = gdix_vtable->fn_SetPageUnit(c->graphics, dummy_UnitPixel);
    if(status != 0) {
        WD_TRACE_ERR_("gdix_canvas_alloc: GdipSetPageUnit() failed.", status);
        goto err_setpageunit;
    }

    status = gdix_vtable->fn_SetSmoothingMode(c->graphics,      /* GDI+ 1.1 */
                dummy_SmoothingModeAntiAlias8x8);
    if(status != 0) {
        gdix_vtable->fn_SetSmoothingMode(c->graphics,           /* GDI+ 1.0 */
                    dummy_SmoothingModeHighQuality);
    }

    /* GDI+ has, unlike D2D, a concept of pens, which are used for "draw"
     * operations, while brushes are used for "fill" operations.
     *
     * Our interface works only with brushes as D2D does. Hence we create
     * a pen as part of GDI+ canvas and we update it with GdipSetPenBrushFill()
     * and GdipSetPenWidth() every time whenever we need to use a pen. */
    status = gdix_vtable->fn_CreatePen1(0, 1.0f, dummy_UnitPixel, &c->pen);
    if(status != 0) {
        WD_TRACE_ERR_("gdix_canvas_alloc: GdipCreatePen1() failed.", status);
        goto err_createpen;
    }

    /* Needed for wdDrawString() and wdMeasureString() */
    status = gdix_vtable->fn_CreateStringFormat(0, LANG_NEUTRAL, &c->string_format);
    if(status != 0) {
        WD_TRACE("gdix_canvas_alloc: "
                 "GdipCreateStringFormat() failed. [%d]", status);
        goto err_createstringformat;
    }

    gdix_reset_transform(c);
    return c;

    /* Error path */
err_createstringformat:
    gdix_vtable->fn_DeletePen(c->pen);
err_createpen:
err_setpageunit:
    gdix_vtable->fn_DeleteGraphics(c->graphics);
err_creategraphics:
    if(c->real_dc != NULL) {
        HBITMAP mem_bmp = SelectObject(c->dc, c->orig_bmp);
        DeleteObject(mem_bmp);
        DeleteDC(c->dc);
    }
    SetLayout(dc, c->dc_layout);
    free(c);
err_malloc:
    return NULL;
}

void
gdix_rtl_transform(gdix_canvas_t* c)
{
    gdix_vtable->fn_ScaleWorldTransform(c->graphics, -1.0f, 1.0f, dummy_MatrixOrderAppend);
    gdix_vtable->fn_TranslateWorldTransform(c->graphics, (float)(c->width-1), 0.0f, dummy_MatrixOrderAppend);
}

void
gdix_reset_transform(gdix_canvas_t* c)
{
    gdix_vtable->fn_ResetWorldTransform(c->graphics);
    if(c->rtl)
        gdix_rtl_transform(c);
}

void
gdix_canvas_apply_string_flags(gdix_canvas_t* c, DWORD flags)
{
    int sfa;
    int sff;
    int trim;

    if(flags & WD_STR_RIGHTALIGN)
        sfa = dummy_StringAlignmentFar;
    else if(flags & WD_STR_CENTERALIGN)
        sfa = dummy_StringAlignmentCenter;
    else
        sfa = dummy_StringAlignmentNear;
    gdix_vtable->fn_SetStringFormatAlign(c->string_format, sfa);

    if(flags & WD_STR_BOTTOMALIGN)
        sfa = dummy_StringAlignmentFar;
    else if(flags & WD_STR_MIDDLEALIGN)
        sfa = dummy_StringAlignmentCenter;
    else
        sfa = dummy_StringAlignmentNear;
    gdix_vtable->fn_SetStringFormatLineAlign(c->string_format, sfa);

    sff = 0;
    if(c->rtl)
        sff |= dummy_StringFormatFlagsDirectionRightToLeft;
    if(flags & WD_STR_NOWRAP)
        sff |= dummy_StringFormatFlagsNoWrap;
    if(flags & WD_STR_NOCLIP)
        sff |= dummy_StringFormatFlagsNoClip;
    gdix_vtable->fn_SetStringFormatFlags(c->string_format, sff);

    switch(flags & WD_STR_ELLIPSISMASK) {
        case WD_STR_ENDELLIPSIS:    trim = dummy_StringTrimmingEllipsisCharacter; break;
        case WD_STR_WORDELLIPSIS:   trim = dummy_StringTrimmingEllipsisWord; break;
        case WD_STR_PATHELLIPSIS:   trim = dummy_StringTrimmingEllipsisPath; break;
        default:                    trim = dummy_StringTrimmingNone; break;
    }
    gdix_vtable->fn_SetStringFormatTrimming(c->string_format, trim);
}

void 
gdix_setpen(dummy_GpPen* pen, dummy_GpBrush* brush, float width, gdix_strokestyle_t* style)
{
    if (style)
    {
        if (style->dashesCount > 0)
        {
            gdix_vtable->fn_SetPenDashOffset(pen, 0.5f);
            gdix_vtable->fn_SetPenDashArray(pen, style->dashes, style->dashesCount);
        }

        gdix_vtable->fn_SetPenDashStyle(pen, style->dashStyle);
        gdix_vtable->fn_SetPenStartCap(pen, style->lineCap);
        gdix_vtable->fn_SetPenEndCap(pen, style->lineCap);
        gdix_vtable->fn_SetPenLineJoin(pen, style->lineJoin);
    }

    gdix_vtable->fn_SetPenBrushFill(pen, brush);
    gdix_vtable->fn_SetPenWidth(pen, width);
}

int gdix_create_bitmap(HBITMAP hBmp, dummy_GpBitmap* *bitmap)
{
  BYTE* bits;
  BITMAPINFO* bi;
  HDC hdcScreen;
  BITMAP bmpScreen;
  int status = 1, header_size;
  DWORD dwStride;

  GetObject(hBmp, sizeof(BITMAP), &bmpScreen);

  if (bmpScreen.bmBitsPixel < 32)
    return gdix_vtable->fn_CreateBitmapFromHBITMAP(hBmp, NULL, bitmap);

  dwStride = ((bmpScreen.bmWidth * bmpScreen.bmBitsPixel + 31) / 32) * 4;

  header_size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 3;  /* maximum of 3 colors if BI_BITFIELDS */
  bi = (BITMAPINFO*)malloc(header_size);
  memset(bi, 0, header_size);
  bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  hdcScreen = GetDC(NULL);
  GetDIBits(hdcScreen, hBmp, 0, (UINT)bmpScreen.bmHeight, NULL, bi, DIB_RGB_COLORS);
  bits = (BYTE*)malloc(bi->bmiHeader.biSizeImage);
  GetDIBits(hdcScreen, hBmp, 0, (UINT)bmpScreen.bmHeight, bits, bi, DIB_RGB_COLORS);
  ReleaseDC(NULL, hdcScreen);

/* None of these approaches worked:
  status = gdix_vtable->fn_CreateBitmapFromGdiDib(bi, bits, bitmap);
  status = gdix_vtable->fn_CreateBitmapFromScan0(bmpScreen.bmWidth, bmpScreen.bmHeight, dwStride, dummy_PixelFormat32bppPARGB, bits, bitmap);
*/
  *bitmap = (dummy_GpBitmap*)wdCreateImageFromBuffer(bmpScreen.bmWidth, bmpScreen.bmHeight, dwStride, bits, WD_PIXELFORMAT_B8G8R8A8, NULL, 0);
  if (*bitmap)
    status = 0;

  free(bi);
  free(bits);
  return status;
}
