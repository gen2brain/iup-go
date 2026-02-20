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
#include "iup_array.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
}

#include "iupwinui_drv.h"

struct IwinuiFont
{
  char font[200];
  IDWriteTextFormat* textFormat;
  float fontSize;
  int charwidth;
  int charheight;
  float charheight_f;
  int ascent;
  int descent;
};

static std::deque<IwinuiFont> winui_fonts;
static IDWriteFactory* winui_dwrite_factory = NULL;
static float winui_screen_dpi = 96.0f;

#define iupWINUI_PT2PIXEL(_pt, _dpi) ((_pt) * (_dpi) / 72.0f)
#define iupWINUI_PIXEL2PT(_px, _dpi) ((_px) * 72.0f / (_dpi))

static void winuiDWriteMeasureText(IDWriteTextFormat* format, const wchar_t* text, int len, float* width, float* height)
{
  if (!winui_dwrite_factory || !format || !text || len == 0)
  {
    if (width) *width = 0;
    if (height) *height = 0;
    return;
  }

  IDWriteTextLayout* layout = NULL;
  HRESULT hr = winui_dwrite_factory->CreateTextLayout(
    text, len, format, 100000.0f, 100000.0f, &layout);

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

static IwinuiFont* winuiFindFont(const char* font)
{
  char typeface[50] = "";
  int size = 9;
  int is_bold = 0;
  int is_italic = 0;
  int is_underline = 0;
  int is_strikeout = 0;

  for (size_t i = 0; i < winui_fonts.size(); i++)
  {
    if (iupStrEqualNoCase(font, winui_fonts[i].font))
      return &winui_fonts[i];
  }

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  const char* mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    strcpy(typeface, mapped_name);

  float fontSize;
  if (size < 0)
    fontSize = (float)(-size);
  else
    fontSize = iupWINUI_PT2PIXEL((float)size, winui_screen_dpi);

  if (fontSize <= 0)
    return NULL;

  int wlen = MultiByteToWideChar(CP_UTF8, 0, typeface, -1, NULL, 0);
  std::wstring wtypeface(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, typeface, -1, &wtypeface[0], wlen);

  DWRITE_FONT_WEIGHT weight = is_bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE style = is_italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

  /* Get user locale for proper text measurement */
  wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
  if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) == 0)
    wcscpy(localeName, L"en-US");

  IDWriteTextFormat* textFormat = NULL;
  HRESULT hr = winui_dwrite_factory->CreateTextFormat(
    wtypeface.c_str(),
    NULL,
    weight,
    style,
    DWRITE_FONT_STRETCH_NORMAL,
    fontSize,
    localeName,
    &textFormat);

  if (FAILED(hr) || !textFormat)
    return NULL;

  /* Disable word wrapping, IUP measures text line by line */
  textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  IwinuiFont newfont = {};
  strncpy(newfont.font, font, sizeof(newfont.font) - 1);
  newfont.textFormat = textFormat;
  newfont.fontSize = fontSize;

  /* Measure character dimensions using "W" for width and line metrics for height */
  float charW, charH;
  winuiDWriteMeasureText(textFormat, L"W", 1, &charW, &charH);
  newfont.charwidth = (int)ceil(charW);
  newfont.charheight = (int)ceil(charH);
  newfont.charheight_f = charH;

  /* Get font metrics for ascent/descent */
  IDWriteFontCollection* fontCollection = NULL;
  winui_dwrite_factory->GetSystemFontCollection(&fontCollection, FALSE);
  if (fontCollection)
  {
    UINT32 index;
    BOOL exists;
    fontCollection->FindFamilyName(wtypeface.c_str(), &index, &exists);
    if (exists)
    {
      IDWriteFontFamily* fontFamily = NULL;
      fontCollection->GetFontFamily(index, &fontFamily);
      if (fontFamily)
      {
        IDWriteFont* dwFont = NULL;
        fontFamily->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, style, &dwFont);
        if (dwFont)
        {
          DWRITE_FONT_METRICS fontMetrics;
          dwFont->GetMetrics(&fontMetrics);
          float scale = fontSize / (float)fontMetrics.designUnitsPerEm;
          newfont.ascent = (int)ceil(fontMetrics.ascent * scale);
          newfont.descent = (int)ceil(fontMetrics.descent * scale);
          dwFont->Release();
        }
        fontFamily->Release();
      }
    }
    fontCollection->Release();
  }

  if (newfont.ascent == 0)
    newfont.ascent = (int)(newfont.charheight * 0.8f);
  if (newfont.descent == 0)
    newfont.descent = newfont.charheight - newfont.ascent;

  winui_fonts.push_back(newfont);
  return &winui_fonts.back();
}

static IwinuiFont* winuiFontGet(Ihandle* ih)
{
  IwinuiFont* winfont = (IwinuiFont*)iupAttribGet(ih, "_IUP_WINUIFONT");
  if (!winfont)
  {
    const char* fontvalue = iupGetFontValue(ih);
    if (fontvalue)
      winfont = winuiFindFont(fontvalue);
    if (!winfont)
    {
      const char* defaultfont = IupGetGlobal("DEFAULTFONT");
      if (defaultfont)
        winfont = winuiFindFont(defaultfont);
    }
    if (!winfont)
      winfont = winuiFindFont("Segoe UI, 9");
    if (winfont)
      iupAttribSet(ih, "_IUP_WINUIFONT", (char*)winfont);
  }
  return winfont;
}

extern "C" void iupwinuiFontInit(void)
{
  HRESULT hr = DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    reinterpret_cast<IUnknown**>(&winui_dwrite_factory));

  if (FAILED(hr))
    winui_dwrite_factory = NULL;

  HDC hdc = GetDC(NULL);
  winui_screen_dpi = (float)GetDeviceCaps(hdc, LOGPIXELSY);
  ReleaseDC(NULL, hdc);
}

extern "C" void iupwinuiFontFinish(void)
{
  for (size_t i = 0; i < winui_fonts.size(); i++)
  {
    if (winui_fonts[i].textFormat)
      winui_fonts[i].textFormat->Release();
  }
  winui_fonts.clear();

  if (winui_dwrite_factory)
  {
    winui_dwrite_factory->Release();
    winui_dwrite_factory = NULL;
  }
}

extern "C" char* iupdrvGetSystemFont(void)
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
    int size = (int)iupWINUI_PIXEL2PT((float)(-height_pixels), winui_screen_dpi);

    char facename[64];
    WideCharToMultiByte(CP_UTF8, 0, lf->lfFaceName, -1, facename, sizeof(facename), NULL, NULL);

    sprintf(str, "%s, %s%s%d", facename,
            is_bold ? "Bold " : "",
            is_italic ? "Italic " : "",
            size);
  }
  else
  {
    strcpy(str, "Segoe UI, 9");
  }
  return str;
}

extern "C" int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  /* If value is NULL, get the effective font (inherited or default) */
  if (!value || !value[0])
    value = iupGetFontValue(ih);
  if (!value || !value[0])
    value = IupGetGlobal("DEFAULTFONT");
  if (!value || !value[0])
    value = "Segoe UI, 9";

  IwinuiFont* winfont = winuiFindFont(value);
  if (!winfont)
    return 0;

  iupAttribSet(ih, "_IUP_WINUIFONT", (char*)winfont);
  iupBaseUpdateAttribFromFont(ih);

  if (ih->handle && ih->iclass->nativetype == IUP_TYPECONTROL && !winuiHandleIsHWND(ih))
  {
    winrt::Microsoft::UI::Xaml::Controls::Control control = winuiGetHandle<winrt::Microsoft::UI::Xaml::Controls::Control>(ih);
    if (control)
      iupwinuiUpdateControlFont(ih, control);
  }

  return 1;
}

extern "C" void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
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

extern "C" int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  if (!str || str[0] == 0)
    return 0;

  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont || !winfont->textFormat)
    return 0;

  const char* line_end = strchr(str, '\n');
  int len = line_end ? (int)(line_end - str) : (int)strlen(str);

  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, len, NULL, 0);
  std::wstring wstr(wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, len, &wstr[0], wlen);

  float width;
  winuiDWriteMeasureText(winfont->textFormat, wstr.c_str(), wlen, &width, NULL);

  return (int)ceil(width);
}

extern "C" void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
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

  int max_w = 0;
  int line_count = 1;

  if (str[0])
  {
    const char* curstr = str;
    while (*curstr)
    {
      const char* nextstr = strchr(curstr, '\n');
      int l_len = nextstr ? (int)(nextstr - curstr) : (int)strlen(curstr);

      if (l_len > 0)
      {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, curstr, l_len, NULL, 0);
        std::wstring wstr(wlen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, curstr, l_len, &wstr[0], wlen);

        float width;
        winuiDWriteMeasureText(winfont->textFormat, wstr.c_str(), wlen, &width, NULL);
        if ((int)ceil(width) > max_w)
          max_w = (int)ceil(width);
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

extern "C" void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IwinuiFont* winfont = winuiFindFont(font);
  if (!winfont)
  {
    winfont = winuiFindFont("Segoe UI, 9");
    if (!winfont)
    {
      if (w) *w = str ? (int)strlen(str) * 8 : 0;
      if (h) *h = 16;
      return;
    }
  }

  if (!str || len == 0)
  {
    if (w) *w = 0;
    if (h) *h = winfont->charheight;
    return;
  }

  int actual_len = (len < 0) ? (int)strlen(str) : len;
  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, actual_len, NULL, 0);
  std::wstring wstr(wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, actual_len, &wstr[0], wlen);

  float width, height;
  winuiDWriteMeasureText(winfont->textFormat, wstr.c_str(), wlen, &width, &height);

  if (w) *w = (int)ceil(width);
  if (h) *h = winfont->charheight;
}

extern "C" void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IwinuiFont* winfont = winuiFindFont(font);
  if (!winfont)
  {
    winfont = winuiFindFont("Segoe UI, 9");
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
    *line_height = winfont->charheight;
  if (ascent)
    *ascent = winfont->ascent;
  if (descent)
    *descent = winfont->descent;
}

extern "C" float iupwinuiFontGetMultilineLineHeightF(Ihandle* ih)
{
  IwinuiFont* winfont = winuiFontGet(ih);
  if (!winfont)
    return 16.0f;

  return winfont->charheight_f;
}

struct WinUIFontProps
{
  float fontSize;
  std::wstring typeface;
  bool isBold;
  bool isItalic;
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
    strcpy(typeface, mapped_name);

  int wlen = MultiByteToWideChar(CP_UTF8, 0, typeface, -1, NULL, 0);
  props->typeface.assign(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, typeface, -1, &props->typeface[0], wlen);

  props->isBold = is_bold != 0;
  props->isItalic = is_italic != 0;
  return true;
}

void iupwinuiUpdateControlFont(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::Control control)
{
  if (!ih || !control)
    return;

  WinUIFontProps props;
  if (!winuiGetFontProps(ih, &props))
    return;

  control.FontSize(static_cast<double>(props.fontSize));
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

void iupwinuiUpdateTextBlockFont(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::TextBlock textBlock)
{
  if (!ih || !textBlock)
    return;

  WinUIFontProps props;
  if (!winuiGetFontProps(ih, &props))
    return;

  textBlock.FontSize(static_cast<double>(props.fontSize));
  textBlock.FontFamily(winrt::Microsoft::UI::Xaml::Media::FontFamily(props.typeface.c_str()));

  if (props.isBold)
    textBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::Bold());
  else
    textBlock.FontWeight(winrt::Windows::UI::Text::FontWeights::Normal());

  if (props.isItalic)
    textBlock.FontStyle(winrt::Windows::UI::Text::FontStyle::Italic);
  else
    textBlock.FontStyle(winrt::Windows::UI::Text::FontStyle::Normal);
}

extern "C" void iupdrvFontInit(void)
{
  iupwinuiFontInit();
}

extern "C" void iupdrvFontFinish(void)
{
  iupwinuiFontFinish();
}
