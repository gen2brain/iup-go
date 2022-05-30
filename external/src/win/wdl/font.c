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
#include "backend-dwrite.h"
#include "backend-gdix.h"
#include "lock.h"

static void
wd_get_default_gui_fontface(WCHAR buffer[LF_FACESIZE])
{
    NONCLIENTMETRICSW metrics;

#if WINVER < 0x0600
    metrics.cbSize = sizeof(NONCLIENTMETRICSW);
#else
    metrics.cbSize = WD_OFFSETOF(NONCLIENTMETRICSW, iPaddedBorderWidth);
#endif
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, (void*) &metrics, 0);
    wcsncpy(buffer, metrics.lfMessageFont.lfFaceName, LF_FACESIZE);
}


WD_HFONT
wdCreateFont(const LOGFONTW* pLogFont)
{
    if(d2d_enabled()) {
        static WCHAR no_locale[] = L"";
        static WCHAR enus_locale[] = L"en-us";

        WCHAR user_locale[LOCALE_NAME_MAX_LENGTH];
        WCHAR* locales[] = { user_locale, no_locale, enus_locale };
        WCHAR default_fontface[LF_FACESIZE];
        dwrite_font_t* font;
        int i;

        font = (dwrite_font_t*) malloc(sizeof(dwrite_font_t));
        if(font == NULL) {
            WD_TRACE("wdCreateFont: malloc() failed.");
            return NULL;
        }

        dwrite_default_user_locale(user_locale);

        /* Direct 2D seems to not understand "MS Shell Dlg" and "MS Shell Dlg 2"
         * so we skip the attempts to use it. */
        if(wcscmp(pLogFont->lfFaceName, L"MS Shell Dlg") != 0  &&
           wcscmp(pLogFont->lfFaceName, L"MS Shell Dlg 2") != 0) {
            for(i = 0; i < WD_SIZEOF_ARRAY(locales); i++) {
                font->tf = dwrite_create_text_format(locales[i], pLogFont, &font->metrics);
                if(font->tf != NULL)
                    return (WD_HFONT) font;
            }
        }

        /* In case of a failure, we retry with a default GUI font face. */
        wd_get_default_gui_fontface(default_fontface);
        if(wcscmp(default_fontface, pLogFont->lfFaceName) != 0) {
            /* Make a temporary copy of pLogFont to not overwrite caller's
             * data. */
            LOGFONTW tmp;

            memcpy(&tmp, pLogFont, sizeof(LOGFONTW));
            wcsncpy(tmp.lfFaceName, default_fontface, LF_FACESIZE);

            for(i = 0; i < WD_SIZEOF_ARRAY(locales); i++) {
                font->tf = dwrite_create_text_format(locales[i], &tmp, &font->metrics);
                if(font->tf != NULL)
                    return (WD_HFONT) font;
            }
        }

        WD_TRACE("wdCreateFont: dwrite_create_text_format(%S, %S) failed.",
                 pLogFont->lfFaceName, user_locale);
        free(font);
        return NULL;
    } else {
        HDC dc;
        dummy_GpFont* f;
        int status;

        dc = GetDC(NULL);
        status = gdix_vtable->fn_CreateFontFromLogfontW(dc, pLogFont, &f);
        if(status != 0) {
            LOGFONTW fallback_logfont;

            /* Failure: This may happen because GDI+ does not support
             * non-TrueType fonts. Fallback to default GUI font, typically
             * Tahoma or Segoe UI on newer Windows. */
            memcpy(&fallback_logfont, pLogFont, sizeof(LOGFONTW));
            wd_get_default_gui_fontface(fallback_logfont.lfFaceName);
            status = gdix_vtable->fn_CreateFontFromLogfontW(dc, &fallback_logfont, &f);
        }
        ReleaseDC(NULL, dc);

        if(status != 0) {
            WD_TRACE("wdCreateFont: GdipCreateFontFromLogfontW(%S) failed. [%d]",
                     pLogFont->lfFaceName, status);
            return NULL;
        }

        return (WD_HFONT) f;
    }
}

WD_HFONT
wdCreateFontWithGdiHandle(HFONT hGdiFont)
{
    LOGFONTW lf;

    if(hGdiFont == NULL)
        hGdiFont = GetStockObject(SYSTEM_FONT);

    GetObjectW(hGdiFont, sizeof(LOGFONTW), &lf);
    return wdCreateFont(&lf);
}

void
wdDestroyFont(WD_HFONT hFont)
{
    if(d2d_enabled()) {
        dwrite_font_t* font = (dwrite_font_t*) hFont;

        dummy_IDWriteTextFormat_Release(font->tf);
        free(font);
    } else {
        gdix_vtable->fn_DeleteFont((dummy_GpFont*) hFont);
    }
}

void
wdFontMetrics(WD_HFONT hFont, WD_FONTMETRICS* pMetrics)
{
    if(hFont == NULL) {
        /* Treat NULL as "no font". This simplifies paint code when font
         * creation fails. */
        WD_TRACE("wdFontMetrics: font == NULL");
        goto err;
    }

    if(d2d_enabled()) {
        dwrite_font_t* font = (dwrite_font_t*) hFont;
        float factor;

        pMetrics->fEmHeight = dummy_IDWriteTextFormat_GetFontSize(font->tf);

        factor = (pMetrics->fEmHeight / (float)font->metrics.designUnitsPerEm);

        pMetrics->fAscent = factor * (float)font->metrics.ascent;
        pMetrics->fDescent = factor * (float)WD_ABS(font->metrics.descent);
        pMetrics->fLeading = factor * (float)(font->metrics.ascent +
                WD_ABS(font->metrics.descent) + font->metrics.lineGap);
    } else {
        int font_style;
        float font_size;
        void* font_family;
        UINT16 cell_ascent;
        UINT16 cell_descent;
        UINT16 em_height;
        UINT16 line_spacing;
        int status;

        gdix_vtable->fn_GetFontSize((void*) hFont, &font_size);
        gdix_vtable->fn_GetFontStyle((void*) hFont, &font_style);

        status = gdix_vtable->fn_GetFamily((void*) hFont, &font_family);
        if(status != 0) {
            WD_TRACE("wdFontMetrics: GdipGetFamily() failed. [%d]", status);
            goto err;
        }
        gdix_vtable->fn_GetCellAscent(font_family, font_style, &cell_ascent);
        gdix_vtable->fn_GetCellDescent(font_family, font_style, &cell_descent);
        gdix_vtable->fn_GetEmHeight(font_family, font_style, &em_height);
        gdix_vtable->fn_GetLineSpacing(font_family, font_style, &line_spacing);
        gdix_vtable->fn_DeleteFontFamily(font_family);

        pMetrics->fEmHeight = font_size;
        pMetrics->fAscent = font_size * (float)cell_ascent / (float)em_height;
        pMetrics->fDescent = WD_ABS(font_size * (float)cell_descent / (float)em_height);
        pMetrics->fLeading = font_size * (float)line_spacing / (float)em_height;
    }

    return;

err:
    pMetrics->fEmHeight = 0.0f;
    pMetrics->fAscent = 0.0f;
    pMetrics->fDescent = 0.0f;
    pMetrics->fLeading = 0.0f;
}
