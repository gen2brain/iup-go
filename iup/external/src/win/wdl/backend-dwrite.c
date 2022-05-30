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

#include "backend-dwrite.h"


static HMODULE dwrite_dll;

dummy_IDWriteFactory* dwrite_factory = NULL;


static int (WINAPI* fn_GetUserDefaultLocaleName)(WCHAR*, int) = NULL;

int
dwrite_init(void)
{
    HMODULE dll_kernel32;
    HRESULT (WINAPI* fn_DWriteCreateFactory)(int, REFIID, void**);
    HRESULT hr;

    dwrite_dll = wd_load_system_dll(_T("DWRITE.DLL"));
    if(dwrite_dll == NULL) {
        WD_TRACE_ERR("dwrite_init: LoadLibrary('DWRITE.DLL') failed.");
        goto err_LoadLibrary;
    }

    fn_DWriteCreateFactory = (HRESULT (WINAPI*)(int, REFIID, void**))
                GetProcAddress(dwrite_dll, "DWriteCreateFactory");
    if(fn_DWriteCreateFactory == NULL) {
        WD_TRACE_ERR("dwrite_init: "
                     "GetProcAddress('DWriteCreateFactory') failed.");
        goto err_GetProcAddress;
    }

    hr = fn_DWriteCreateFactory(dummy_DWRITE_FACTORY_TYPE_SHARED,
                &dummy_IID_IDWriteFactory, (void**) &dwrite_factory);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_init: DWriteCreateFactory() failed.");
        goto err_DWriteCreateFactory;
    }

    /* We need locale name for creation of dummy_IDWriteTextFormat. This
     * functions is available since Vista (which covers all systems with
     * Direct2D and DirectWrite). */
    dll_kernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
    if(dll_kernel32 != NULL) {
        fn_GetUserDefaultLocaleName = (int (WINAPI*)(WCHAR*, int))
                GetProcAddress(dll_kernel32, "GetUserDefaultLocaleName");
    }

    /* Success. */
    return 0;

    /* Error path unwinding. */
err_DWriteCreateFactory:
err_GetProcAddress:
    FreeLibrary(dwrite_dll);
err_LoadLibrary:
    return -1;
}

void
dwrite_fini(void)
{
    dummy_IDWriteFactory_Release(dwrite_factory);
    FreeLibrary(dwrite_dll);
    dwrite_factory = NULL;
}

void
dwrite_default_user_locale(WCHAR buffer[LOCALE_NAME_MAX_LENGTH])
{
    if(fn_GetUserDefaultLocaleName != NULL) {
        if(fn_GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH) > 0)
            return;
        WD_TRACE_ERR("dwrite_default_user_locale: "
                     "GetUserDefaultLocaleName() failed.");
    } else {
        WD_TRACE_ERR("dwrite_default_user_locale: "
                     "function GetUserDefaultLocaleName() not available.");
    }

    buffer[0] = L'\0';
}

dummy_IDWriteTextFormat*
dwrite_create_text_format(const WCHAR* locale_name, const LOGFONTW* logfont,
                          dummy_DWRITE_FONT_METRICS* metrics)
{
    dummy_IDWriteTextFormat* tf = NULL;
    dummy_IDWriteGdiInterop* gdi_interop;
    dummy_IDWriteFont* font;
    dummy_IDWriteFontFamily* family;
    dummy_IDWriteLocalizedStrings* family_names;
    UINT32 family_name_buffer_size;
    WCHAR* family_name_buffer;
    float font_size;
    HRESULT hr;

    hr = dummy_IDWriteFactory_GetGdiInterop(dwrite_factory, &gdi_interop);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteFactory::GetGdiInterop() failed.");
        goto err_IDWriteFactory_GetGdiInterop;
    }

    hr = dummy_IDWriteGdiInterop_CreateFontFromLOGFONT(gdi_interop, logfont, &font);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteGdiInterop::CreateFontFromLOGFONT() failed.");
        goto err_IDWriteGdiInterop_CreateFontFromLOGFONT;
    }

    dummy_IDWriteFont_GetMetrics(font, metrics);

    hr = dummy_IDWriteFont_GetFontFamily(font, &family);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteFont::GetFontFamily() failed.");
        goto err_IDWriteFont_GetFontFamily;
    }

    hr = dummy_IDWriteFontFamily_GetFamilyNames(family, &family_names);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteFontFamily::GetFamilyNames() failed.");
        goto err_IDWriteFontFamily_GetFamilyNames;
    }

    hr = dummy_IDWriteLocalizedStrings_GetStringLength(family_names, 0, &family_name_buffer_size);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteLocalizedStrings::GetStringLength() failed.");
        goto err_IDWriteLocalizedStrings_GetStringLength;
    }

    family_name_buffer = (WCHAR*) _malloca(sizeof(WCHAR) * (family_name_buffer_size + 1));
    if(family_name_buffer == NULL) {
        WD_TRACE("dwrite_create_text_format: _malloca() failed.");
        goto err_malloca;
    }

    hr = dummy_IDWriteLocalizedStrings_GetString(family_names, 0,
            family_name_buffer, family_name_buffer_size + 1);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteLocalizedStrings::GetString() failed.");
        goto err_IDWriteLocalizedStrings_GetString;
    }

    if(logfont->lfHeight < 0) {
        font_size = (float) -logfont->lfHeight;
    } else if(logfont->lfHeight > 0) {
        font_size = ((float)metrics->ascent + (float)metrics->descent)
                / (float)metrics->designUnitsPerEm;
    } else {
        font_size = 12.0f;
    }

    hr = dummy_IDWriteFactory_CreateTextFormat(dwrite_factory, family_name_buffer,
            NULL, dummy_IDWriteFont_GetWeight(font), dummy_IDWriteFont_GetStyle(font),
            dummy_IDWriteFont_GetStretch(font), font_size, locale_name, &tf);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_format: "
                    "IDWriteFactory::CreateTextFormat() failed.");
        goto err_IDWriteFactory_CreateTextFormat;
    }

err_IDWriteFactory_CreateTextFormat:
err_IDWriteLocalizedStrings_GetString:
    _freea(family_name_buffer);
err_malloca:
err_IDWriteLocalizedStrings_GetStringLength:
    dummy_IDWriteLocalizedStrings_Release(family_names);
err_IDWriteFontFamily_GetFamilyNames:
    dummy_IDWriteFontFamily_Release(family);
err_IDWriteFont_GetFontFamily:
    dummy_IDWriteFont_Release(font);
err_IDWriteGdiInterop_CreateFontFromLOGFONT:
    dummy_IDWriteGdiInterop_Release(gdi_interop);
err_IDWriteFactory_GetGdiInterop:
    return tf;
}

dummy_IDWriteTextLayout*
dwrite_create_text_layout(dummy_IDWriteTextFormat* tf, const WD_RECT* rect,
                          const WCHAR* str, int len, DWORD flags)
{
    dummy_IDWriteTextLayout* layout;
    HRESULT hr;
    int tla;

    if(len < 0)
        len = wcslen(str);

    hr = dummy_IDWriteFactory_CreateTextLayout(dwrite_factory, str, len, tf,
                rect->x1 - rect->x0, rect->y1 - rect->y0, &layout);
    if(FAILED(hr)) {
        WD_TRACE_HR("dwrite_create_text_layout: "
                    "IDWriteFactory::CreateTextLayout() failed.");
        return NULL;
    }

    if(flags & WD_STR_RIGHTALIGN)
        tla = dummy_DWRITE_TEXT_ALIGNMENT_TRAILING;
    else if(flags & WD_STR_CENTERALIGN)
        tla = dummy_DWRITE_TEXT_ALIGNMENT_CENTER;
    else
        tla = dummy_DWRITE_TEXT_ALIGNMENT_LEADING;
    dummy_IDWriteTextLayout_SetTextAlignment(layout, tla);

    if(flags & WD_STR_BOTTOMALIGN)
        tla = dummy_DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    else if(flags & WD_STR_MIDDLEALIGN)
        tla = dummy_DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    else
        tla = dummy_DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    dummy_IDWriteTextLayout_SetParagraphAlignment(layout, tla);

    if(flags & WD_STR_NOWRAP)
        dummy_IDWriteTextLayout_SetWordWrapping(layout, dummy_DWRITE_WORD_WRAPPING_NO_WRAP);

    if((flags & WD_STR_ELLIPSISMASK) != 0) {
        static const dummy_DWRITE_TRIMMING trim_end = { dummy_DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        static const dummy_DWRITE_TRIMMING trim_word = { dummy_DWRITE_TRIMMING_GRANULARITY_WORD, 0, 0 };
        static const dummy_DWRITE_TRIMMING trim_path = { dummy_DWRITE_TRIMMING_GRANULARITY_WORD, L'\\', 1 };

        const dummy_DWRITE_TRIMMING* trim_options = NULL;
        dummy_IDWriteInlineObject* trim_sign;

        hr = dummy_IDWriteFactory_CreateEllipsisTrimmingSign(dwrite_factory, tf, &trim_sign);
        if(FAILED(hr)) {
            WD_TRACE_HR("dwrite_create_text_layout: "
                        "IDWriteFactory::CreateEllipsisTrimmingSign() failed.");
            goto err_CreateEllipsisTrimmingSign;
        }

        switch(flags & WD_STR_ELLIPSISMASK) {
            case WD_STR_ENDELLIPSIS:    trim_options = &trim_end; break;
            case WD_STR_WORDELLIPSIS:   trim_options = &trim_word; break;
            case WD_STR_PATHELLIPSIS:   trim_options = &trim_path; break;
        }

        if(trim_options != NULL)
            dummy_IDWriteTextLayout_SetTrimming(layout, trim_options, trim_sign);

        dummy_IDWriteInlineObject_Release(trim_sign);
    }

err_CreateEllipsisTrimmingSign:
    return layout;
}
