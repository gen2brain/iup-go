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

#ifndef WD_BACKEND_DWRITE_H
#define WD_BACKEND_DWRITE_H

#include "misc.h"
#include "dummy/dwrite.h"


extern dummy_IDWriteFactory* dwrite_factory;


typedef struct dwrite_font_tag dwrite_font_t;
struct dwrite_font_tag {
    dummy_IDWriteTextFormat* tf;
    dummy_DWRITE_FONT_METRICS metrics;
};


int dwrite_init(void);
void dwrite_fini(void);

void dwrite_default_user_locale(WCHAR buffer[LOCALE_NAME_MAX_LENGTH]);

dummy_IDWriteTextFormat* dwrite_create_text_format(const WCHAR* locale_name,
            const LOGFONTW* logfont, dummy_DWRITE_FONT_METRICS* metrics);

dummy_IDWriteTextLayout* dwrite_create_text_layout(dummy_IDWriteTextFormat* tf,
            const WD_RECT* rect, const WCHAR* str, int len, DWORD flags);


#endif  /* WD_BACKEND_DWRITE_H */

