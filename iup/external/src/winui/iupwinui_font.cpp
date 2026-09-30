/** \file
 * \brief WinUI Driver - Font Management using DirectWrite
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <deque>

#include <windows.h>
#include <dwrite.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
}

#include "iupwinui_drv.h"

struct IwinuiFont
{
  char font[200];
  float dpi;
  IDWriteTextFormat* textFormat;
  float fontSize;
  int charwidth;
  int charheight;
  float charheight_f;
  int drawcharheight;
  int ascent;
  int descent;
};

static std::deque<IwinuiFont> winui_fonts;
IDWriteFactory* winui_dwrite_factory = nullptr;
float winui_screen_dpi = 96.0f;

#define iupWINUI_PIXEL2PT(_px, _dpi) ((_px) * 72.0f / (_dpi))

static void winuiDWriteMeasureText(IDWriteTextFormat* format, const wchar_t* text, int len, int gdi, float* width, float* height)
{
  if (!winui_dwrite_factory || !format || !text || len == 0)
  {
    if (width) *width = 0;
    if (height) *height = 0;
    return;
  }

  IDWriteTextLayout* layout = nullptr;
  HRESULT hr;
  if (gdi)
    hr = winui_dwrite_factory->CreateGdiCompatibleTextLayout(text, len, format, 100000.0f, 100000.0f, 1.0f, nullptr, FALSE, &layout);
  else
    hr = winui_dwrite_factory->CreateTextLayout(text, len, format, 100000.0f, 100000.0f, &layout);

  if (SUCCEEDED(hr) && layout)
  {
    DWRITE_TEXT_METRICS metrics;
    hr = layout->GetMetrics(&metrics);
    if (SUCCEEDED(hr))
    {
      if (width) *width = metrics.widthIncludingTrailingWhitespace;
      if (height) *height = metrics.height;
    }
    layout->Release();
  }
  else
  {
    if (width) *width = 0;
    if (height) *height = 0;
  }
}

static IwinuiFont* winuiFindFont(const char* font, float dpi)
{
  char typeface[50] = "";
  int size = 9;
  int is_bold = 0;
  int is_italic = 0;
  int is_underline = 0;
  int is_strikeout = 0;

  for (auto& winui_font : winui_fonts)
  {
    if (winui_font.dpi == dpi && iupStrEqualNoCase(font, winui_font.font))
      return &winui_font;
  }

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return nullptr;

  const char* mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    iupStrCopyN(typeface, sizeof(typeface), mapped_name);

  float fontSize;
  if (size < 0)
    fontSize = static_cast<float>(-size);
  else
    fontSize = iupWINUI_PT2PIXEL(static_cast<float>(size), dpi);

  if (fontSize <= 0)
    return nullptr;

  int wlen = MultiByteToWideChar(CP_UTF8, 0, typeface, -1, nullptr, 0);
  std::wstring wtypeface(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, typeface, -1, &wtypeface[0], wlen);

  DWRITE_FONT_WEIGHT weight = is_bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE style = is_italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

  wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
  if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) == 0)
    wcscpy(localeName, L"en-US");

  IDWriteTextFormat* textFormat = nullptr;
  HRESULT hr = winui_dwrite_factory->CreateTextFormat(
    wtypeface.c_str(),
    nullptr,
    weight,
    style,
    DWRITE_FONT_STRETCH_NORMAL,
    fontSize,
    localeName,
    &textFormat);

  if (FAILED(hr) || !textFormat)
    return nullptr;

  /* Disable word wrapping, IUP measures text line by line */
  textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  IwinuiFont newfont = {};
  iupStrCopyN(newfont.font, sizeof(newfont.font), font);
  newfont.dpi = dpi;
  newfont.textFormat = textFormat;
  newfont.fontSize = fontSize;

  float charW, charH;
  winuiDWriteMeasureText(textFormat, L"abcdefghijklmnopqrstuvwxyz", 26, 0, &charW, &charH);
  newfont.charwidth = static_cast<int>(ceil(charW / 26.0f));
  newfont.charheight = static_cast<int>(ceil(charH));
  newfont.charheight_f = charH;

  winuiDWriteMeasureText(textFormat, L"abcdefghijklmnopqrstuvwxyz", 26, 1, &charW, &charH);
  newfont.drawcharheight = static_cast<int>(ceil(charH));

  IDWriteFontCollection* fontCollection = nullptr;
  winui_dwrite_factory->GetSystemFontCollection(&fontCollection, FALSE);
  if (fontCollection)
  {
    UINT32 index;
    BOOL exists;
    fontCollection->FindFamilyName(wtypeface.c_str(), &index, &exists);
    if (exists)
    {
      IDWriteFontFamily* fontFamily = nullptr;
      fontCollection->GetFontFamily(index, &fontFamily);
      if (fontFamily)
      {
        IDWriteFont* dwFont = nullptr;
        fontFamily->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, style, &dwFont);
        if (dwFont)
        {
          DWRITE_FONT_METRICS fontMetrics;
          dwFont->GetMetrics(&fontMetrics);
          float scale = fontSize / static_cast<float>(fontMetrics.designUnitsPerEm);
          newfont.ascent = static_cast<int>(ceil(fontMetrics.ascent * scale));
          newfont.descent = static_cast<int>(ceil(fontMetrics.descent * scale));
          dwFont->Release();
        }
        fontFamily->Release();
      }
    }
    fontCollection->Release();
  }

  if (newfont.ascent == 0)
    newfont.ascent = static_cast<int>(newfont.charheight * 0.8f);
  if (newfont.descent == 0)
    newfont.descent = newfont.charheight - newfont.ascent;

  winui_fonts.push_back(newfont);
  return &winui_fonts.back();
}

static IwinuiFont* winuiFontGet(Ihandle* ih)
{
  auto dpi = static_cast<float>(iupwinuiGetDpi(ih));
  IwinuiFont* winfont = winuiFindFont(iupGetFontValue(ih), dpi);
  if (!winfont)
    winfont = winuiFindFont(IupGetGlobal("DEFAULTFONT"), dpi);
  if (!winfont)
    winfont = winuiFindFont("Segoe UI, 9", dpi);
  return winfont;
}


extern "C" IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char str[200];
  NONCLIENTMETRICSW ncm;
  ncm.cbSize = sizeof(NONCLIENTMETRICSW);

  if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, FALSE))
  {
    LOGFONTW* lf = &ncm.lfMessageFont;
    int is_bold = (lf->lfWeight == FW_NORMAL) ? 0 : 1;
    int is_italic = lf->lfItalic;
    int height_pixels = lf->lfHeight;
    int size = static_cast<int>(iupWINUI_PIXEL2PT(static_cast<float>(-height_pixels), winui_screen_dpi));

    char facename[64];
    WideCharToMultiByte(CP_UTF8, 0, lf->lfFaceName, -1, facename, sizeof(facename), nullptr, nullptr);

    snprintf(str, sizeof(str), "%s, %s%s%d", facename,
            is_bold ? "Bold " : "",
            is_italic ? "Italic " : "",
            size);
  }
  else
  {
    iupStrCopyN(str, sizeof(str), "Segoe UI, 9");
  }
  return str;
}

extern "C" IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!value || !value[0])
    value = iupGetFontValue(ih);
  if (!value || !value[0])
    value = IupGetGlobal("DEFAULTFONT");
  if (!value || !value[0])
    value = "Segoe UI, 9";

  IwinuiFont* winfont = winuiFindFont(value, static_cast<float>(iupwinuiGetDpi(ih)));
  if (!winfont)
    return 0;

  iupAttribSet(ih, "_IUP_WINUIFONT", reinterpret_cast<char*>(winfont));
  iupBaseUpdateAttribFromFont(ih);

  if (ih->handle && ih->iclass->nativetype == IUP_TYPECONTROL && !winuiHandleIsHWND(ih))
  {
    auto control = winuiGetHandle<winrt::Microsoft::UI::Xaml::Controls::Control>(ih);
    if (control)
      iupwinuiUpdateControlFont(ih, control);
  }

  return 1;
}

extern "C" IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont)
  {
    if (charwidth) *charwidth = 8;
    if (charheight) *charheight = 16;
    return;
  }

  if (charwidth)
    *charwidth = winfont->charwidth;
  if (charheight)
    *charheight = winfont->charheight;
}

extern "C" IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  if (!str || str[0] == 0)
    return 0;

  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont || !winfont->textFormat)
    return 0;

  const char* line_end = strchr(str, '\n');
  int len = line_end ? static_cast<int>(line_end - str) : static_cast<int>(strlen(str));

  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, len, nullptr, 0);
  std::wstring wstr(wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, len, &wstr[0], wlen);

  float width;
  winuiDWriteMeasureText(winfont->textFormat, wstr.c_str(), wlen, 0, &width, nullptr);

  return static_cast<int>(ceil(width));
}

extern "C" IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = winfont->charheight;
    return;
  }

  if (ih && iupAttribGetBoolean(ih, "MARKUP"))
  {
    iupwinuiMeasureMarkupText(ih, str, w, h);
    return;
  }

  int max_w = 0;
  int line_count = 1;

  if (str[0])
  {
    const char* curstr = str;
    while (*curstr)
    {
      const char* nextstr = strchr(curstr, '\n');
      int l_len = nextstr ? static_cast<int>(nextstr - curstr) : static_cast<int>(strlen(curstr));

      if (l_len > 0)
      {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, curstr, l_len, nullptr, 0);
        std::wstring wstr(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, curstr, l_len, &wstr[0], wlen);

        float width;
        winuiDWriteMeasureText(winfont->textFormat, wstr.c_str(), wlen, 0, &width, nullptr);
        if (static_cast<int>(ceil(width)) > max_w)
          max_w = static_cast<int>(ceil(width));
      }

      if (nextstr)
      {
        line_count++;
        curstr = nextstr + 1;
      }
      else
        break;
    }
  }

  if (w) *w = max_w;
  if (h) *h = winfont->charheight * line_count;
}

extern "C" IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IwinuiFont* winfont = winuiFindFont(font, winui_screen_dpi);
  if (!winfont)
  {
    winfont = winuiFindFont("Segoe UI, 9", winui_screen_dpi);
    if (!winfont)
    {
      if (w) *w = str ? static_cast<int>(strlen(str)) * 8 : 0;
      if (h) *h = 16;
      return;
    }
  }

  if (!str || len == 0)
  {
    if (w) *w = 0;
    if (h) *h = winfont->drawcharheight;
    return;
  }

  int actual_len = (len < 0) ? static_cast<int>(strlen(str)) : len;
  const char* end = str + actual_len;
  const char* curstr = str;
  int max_w = 0, line_count = 1;

  while (curstr < end)
  {
    const char* nextstr = static_cast<const char*>(memchr(curstr, '\n', end - curstr));
    int l_len = nextstr ? static_cast<int>(nextstr - curstr) : static_cast<int>(end - curstr);

    if (l_len > 0)
    {
      int wlen = MultiByteToWideChar(CP_UTF8, 0, curstr, l_len, nullptr, 0);
      std::wstring wstr(wlen, L'\0');
      MultiByteToWideChar(CP_UTF8, 0, curstr, l_len, &wstr[0], wlen);

      float width;
      winuiDWriteMeasureText(winfont->textFormat, wstr.c_str(), wlen, 1, &width, nullptr);
      if (static_cast<int>(ceil(width)) > max_w)
        max_w = static_cast<int>(ceil(width));
    }

    if (!nextstr || nextstr + 1 >= end)
      break;
    line_count++;
    curstr = nextstr + 1;
  }

  if (w) *w = max_w;
  if (h) *h = winfont->drawcharheight * line_count;
}

extern "C" IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IwinuiFont* winfont = winuiFindFont(font, winui_screen_dpi);
  if (!winfont)
  {
    winfont = winuiFindFont("Segoe UI, 9", winui_screen_dpi);
    if (!winfont)
    {
      if (max_width) *max_width = 8;
      if (line_height) *line_height = 16;
      if (ascent) *ascent = 12;
      if (descent) *descent = 4;
      return;
    }
  }

  if (max_width)
    *max_width = winfont->charwidth;
  if (line_height)
    *line_height = winfont->drawcharheight;
  if (ascent)
    *ascent = winfont->ascent;
  if (descent)
    *descent = winfont->descent;
}

IUP_DRV_API float iupwinuiFontGetMultilineLineHeightF(Ihandle* ih)
{
  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont)
    return 16.0f;

  return winfont->charheight_f;
}

struct WinUIFontProps
{
  float fontSize{0};
  std::wstring typeface;
  bool isBold{false};
  bool isItalic{false};
  bool isUnderline{false};
  bool isStrikeout{false};
};

static bool winuiGetFontProps(Ihandle* ih, WinUIFontProps* props)
{
  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont)
    return false;

  props->fontSize = winfont->fontSize;

  const char* fontvalue = iupGetFontValue(ih);
  if (!fontvalue)
    fontvalue = IupGetGlobal("DEFAULTFONT");
  if (!fontvalue)
    return false;

  char typeface[50] = "";
  int size, is_bold, is_italic, is_underline, is_strikeout;
  if (!iupGetFontInfo(fontvalue, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return false;

  const char* mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    iupStrCopyN(typeface, sizeof(typeface), mapped_name);

  int wlen = MultiByteToWideChar(CP_UTF8, 0, typeface, -1, nullptr, 0);
  props->typeface.assign(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, typeface, -1, &props->typeface[0], wlen);

  props->isBold = is_bold != 0;
  props->isItalic = is_italic != 0;
  props->isUnderline = is_underline != 0;
  props->isStrikeout = is_strikeout != 0;
  return true;
}

IUP_DRV_API void iupwinuiUpdateControlFont(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::Control control)
{
  if (!ih || !control)
    return;

  WinUIFontProps props;
  if (!winuiGetFontProps(ih, &props))
    return;

  control.FontSize(static_cast<double>(props.fontSize) / iupwinuiGetScale(ih));
  control.FontFamily(winrt::Microsoft::UI::Xaml::Media::FontFamily(props.typeface.c_str()));

  if (props.isBold)
    control.FontWeight(winrt::Windows::UI::Text::FontWeights::Bold());
  else
    control.FontWeight(winrt::Windows::UI::Text::FontWeights::Normal());

  if (props.isItalic)
    control.FontStyle(winrt::Windows::UI::Text::FontStyle::Italic);
  else
    control.FontStyle(winrt::Windows::UI::Text::FontStyle::Normal);
}

IUP_DRV_API void iupwinuiUpdateTextBlockFontStr(winrt::Microsoft::UI::Xaml::Controls::TextBlock textBlock, const char* value, Ihandle* ih)
{
  if (!textBlock || !value || !value[0])
    return;

  char typeface[50] = "";
  int size, is_bold, is_italic, is_underline, is_strikeout;
  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return;

  const char* mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    iupStrCopyN(typeface, sizeof(typeface), mapped_name);

  float fontSize;
  if (size < 0)
    fontSize = static_cast<float>(-size);
  else
    fontSize = iupWINUI_PT2PIXEL(static_cast<float>(size), static_cast<float>(iupwinuiGetDpi(ih)));

  if (fontSize > 0)
    textBlock.FontSize(static_cast<double>(fontSize) / iupwinuiGetScale(ih));

  textBlock.FontFamily(winrt::Microsoft::UI::Xaml::Media::FontFamily(iupwinuiStringToHString(typeface)));

  if (is_bold)
    textBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::Bold());
  else
    textBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::Normal());

  if (is_italic)
    textBlock.FontStyle(winrt::Windows::UI::Text::FontStyle::Italic);
  else
    textBlock.FontStyle(winrt::Windows::UI::Text::FontStyle::Normal);

  auto decorations = winrt::Windows::UI::Text::TextDecorations::None;
  if (is_underline)
    decorations = decorations | winrt::Windows::UI::Text::TextDecorations::Underline;
  if (is_strikeout)
    decorations = decorations | winrt::Windows::UI::Text::TextDecorations::Strikethrough;
  textBlock.TextDecorations(decorations);
}

IUP_DRV_API void iupwinuiUpdateTextBlockFont(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::TextBlock textBlock)
{
  if (!ih || !textBlock)
    return;

  WinUIFontProps props;
  if (!winuiGetFontProps(ih, &props))
    return;

  textBlock.FontSize(static_cast<double>(props.fontSize) / iupwinuiGetScale(ih));
  textBlock.FontFamily(winrt::Microsoft::UI::Xaml::Media::FontFamily(props.typeface.c_str()));

  if (props.isBold)
    textBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::Bold());
  else
    textBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::Normal());

  if (props.isItalic)
    textBlock.FontStyle(winrt::Windows::UI::Text::FontStyle::Italic);
  else
    textBlock.FontStyle(winrt::Windows::UI::Text::FontStyle::Normal);

  auto decorations = winrt::Windows::UI::Text::TextDecorations::None;
  if (props.isUnderline)
    decorations = decorations | winrt::Windows::UI::Text::TextDecorations::Underline;
  if (props.isStrikeout)
    decorations = decorations | winrt::Windows::UI::Text::TextDecorations::Strikethrough;
  textBlock.TextDecorations(decorations);
}

static int winuiFontFamilyCompare(const void* a, const void* b)
{
  return iupStrCompare(*static_cast<const char* const*>(a), *static_cast<const char* const*>(b), 0, 1);
}

extern "C" IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
  IDWriteFontCollection* collection = nullptr;
  UINT32 i, family_count;
  int count = 0;
  char** temp;

  if (!winui_dwrite_factory)
  {
    *list = nullptr;
    return 0;
  }

  winui_dwrite_factory->GetSystemFontCollection(&collection, FALSE);
  if (!collection)
  {
    *list = nullptr;
    return 0;
  }

  family_count = collection->GetFontFamilyCount();
  if (family_count == 0)
  {
    collection->Release();
    *list = nullptr;
    return 0;
  }

  temp = static_cast<char**>(malloc(family_count * sizeof(char*)));

  for (i = 0; i < family_count; i++)
  {
    IDWriteFontFamily* family = nullptr;
    collection->GetFontFamily(i, &family);
    if (!family)
      continue;

    IDWriteLocalizedStrings* names = nullptr;
    family->GetFamilyNames(&names);
    if (names)
    {
      UINT32 idx = 0;
      BOOL exists = FALSE;
      names->FindLocaleName(L"en-us", &idx, &exists);
      if (!exists)
        idx = 0;

      UINT32 len = 0;
      names->GetStringLength(idx, &len);
      if (len > 0)
      {
        auto* wname = static_cast<wchar_t*>(malloc((len + 1) * sizeof(wchar_t)));
        names->GetString(idx, wname, len + 1);

        char name[256];
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, sizeof(name), nullptr, nullptr);
        free(wname);

        if (utf8_len > 0)
        {
          temp[count] = iupStrDup(name);
          count++;
        }
      }
      names->Release();
    }
    family->Release();
  }

  collection->Release();

  if (count == 0)
  {
    free(temp);
    *list = nullptr;
    return 0;
  }

  *list = static_cast<char**>(realloc(temp, count * sizeof(char*)));
  qsort(*list, count, sizeof(char*), winuiFontFamilyCompare);

  return count;
}

extern "C" IUP_SDK_API void iupdrvFontInit(void)
{
  HRESULT hr = DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(&winui_dwrite_factory));

  if (FAILED(hr))
    winui_dwrite_factory = nullptr;

  HDC hdc = GetDC(nullptr);
  winui_screen_dpi = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSY));
  ReleaseDC(nullptr, hdc);
}

extern "C" IUP_SDK_API void iupdrvFontFinish(void)
{
  for (auto& winui_font : winui_fonts)
  {
    if (winui_font.textFormat)
      winui_font.textFormat->Release();
  }
  winui_fonts.clear();

  if (winui_dwrite_factory)
  {
    winui_dwrite_factory->Release();
    winui_dwrite_factory = nullptr;
  }
}
