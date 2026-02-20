/** \file
 * \brief WinUI Driver - Font Dialog
 *
 * Uses Win32 ChooseFont API (no WinUI equivalent exists).
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commdlg.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_dialog.h"
#include "iup_drvfont.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

#ifndef CF_NOSCRIPTSEL
#define CF_NOSCRIPTSEL 0x00800000L
#endif

#define IUP_FONTFAMILYCOMBOBOX   0x0470
#define IUP_FONTCOLORLTEXT       0x0443
#define IUP_FONTCOLORCOMBOBOX    0x0473
#define IUP_FONTSCRIPTLTEXT      0x0446
#define IUP_FONTSCRIPTCOMBOBOX   0x0474

static UINT_PTR CALLBACK winuiFontDlgHookProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  (void)wParam;
  if (uiMsg == WM_INITDIALOG)
  {
    HWND hWndItem;
    CHOOSEFONTW* choosefont = (CHOOSEFONTW*)lParam;
    Ihandle* ih = (Ihandle*)choosefont->lCustData;

    char* title = iupAttribGet(ih, "TITLE");
    if (title)
    {
      std::wstring wtitle = iupwinuiStringToWString(title);
      SetWindowTextW(hWnd, wtitle.c_str());
    }

    ih->handle = (InativeHandle*)hWnd;
    iupDialogUpdatePosition(ih);
    ih->handle = NULL;

    hWndItem = GetDlgItem(hWnd, IUP_FONTFAMILYCOMBOBOX);
    SetFocus(hWndItem);

    if (!iupAttribGetBoolean(ih, "SHOWCOLOR"))
    {
      hWndItem = GetDlgItem(hWnd, IUP_FONTCOLORLTEXT);
      if (hWndItem) ShowWindow(hWndItem, SW_HIDE);
      hWndItem = GetDlgItem(hWnd, IUP_FONTCOLORCOMBOBOX);
      if (hWndItem) ShowWindow(hWndItem, SW_HIDE);
    }

    hWndItem = GetDlgItem(hWnd, IUP_FONTSCRIPTLTEXT);
    if (hWndItem) ShowWindow(hWndItem, SW_HIDE);
    hWndItem = GetDlgItem(hWnd, IUP_FONTSCRIPTCOMBOBOX);
    if (hWndItem) ShowWindow(hWndItem, SW_HIDE);
  }
  return 0;
}

static int winuiFontDlgPopup(Ihandle* ih, int x, int y)
{
  HWND parent = (HWND)iupDialogGetNativeParent(ih);
  unsigned char r, g, b;
  CHOOSEFONTW choosefont;
  LOGFONTW logfont;
  char* font;
  int height_pixels;
  char typeface[50] = "";
  int height = 8;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  HDC hdc;
  int res;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (!parent)
    parent = GetActiveWindow();

  hdc = GetDC(NULL);
  res = GetDeviceCaps(hdc, LOGPIXELSY);
  ReleaseDC(NULL, hdc);

  font = iupAttribGet(ih, "VALUE");
  if (!font)
  {
    if (!iupGetFontInfo(IupGetGlobal("DEFAULTFONT"), typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return IUP_ERROR;
  }
  else
  {
    if (!iupGetFontInfo(font, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      if (!iupGetFontInfo(IupGetGlobal("DEFAULTFONT"), typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
        return IUP_ERROR;
    }
  }

  if (height < 0)
    height_pixels = height;
  else
    height_pixels = -MulDiv(height, res, 72);

  if (height_pixels == 0)
    return IUP_ERROR;

  ZeroMemory(&choosefont, sizeof(CHOOSEFONTW));
  choosefont.lStructSize = sizeof(CHOOSEFONTW);

  if (iupStrToRGB(iupAttribGet(ih, "COLOR"), &r, &g, &b))
    choosefont.rgbColors = RGB(r, g, b);

  choosefont.hwndOwner = parent;
  choosefont.lpLogFont = &logfont;
  choosefont.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
  choosefont.lCustData = (LPARAM)ih;
  choosefont.lpfnHook = (LPCFHOOKPROC)winuiFontDlgHookProc;
  choosefont.Flags |= CF_ENABLEHOOK;

  ZeroMemory(&logfont, sizeof(LOGFONTW));
  MultiByteToWideChar(CP_UTF8, 0, typeface, -1, logfont.lfFaceName, LF_FACESIZE);

  logfont.lfHeight = height_pixels;
  logfont.lfWeight = is_bold ? FW_BOLD : FW_NORMAL;
  logfont.lfItalic = (BYTE)is_italic;
  logfont.lfUnderline = (BYTE)is_underline;
  logfont.lfStrikeOut = (BYTE)is_strikeout;
  logfont.lfCharSet = DEFAULT_CHARSET;
  logfont.lfOutPrecision = OUT_TT_PRECIS;
  logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality = DEFAULT_QUALITY;
  logfont.lfPitchAndFamily = FF_DONTCARE | DEFAULT_PITCH;

  if (!ChooseFontW(&choosefont))
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "COLOR", NULL);
    iupAttribSet(ih, "STATUS", NULL);
    return IUP_NOERROR;
  }

  is_bold = (logfont.lfWeight == FW_NORMAL) ? 0 : 1;
  is_italic = logfont.lfItalic;
  is_underline = logfont.lfUnderline;
  is_strikeout = logfont.lfStrikeOut;
  height_pixels = logfont.lfHeight;

  if (height < 0)
    height = height_pixels;
  else
    height = MulDiv(-height_pixels, 72, res);

  char result_typeface[50];
  WideCharToMultiByte(CP_UTF8, 0, logfont.lfFaceName, -1, result_typeface, sizeof(result_typeface), NULL, NULL);

  iupAttribSetStrf(ih, "VALUE", "%s, %s%s%s%s%d", result_typeface,
                   is_bold ? "Bold " : "",
                   is_italic ? "Italic " : "",
                   is_underline ? "Underline " : "",
                   is_strikeout ? "Strikeout " : "",
                   height);

  iupAttribSetStrf(ih, "COLOR", "%d %d %d", GetRValue(choosefont.rgbColors),
                                             GetGValue(choosefont.rgbColors),
                                             GetBValue(choosefont.rgbColors));
  iupAttribSet(ih, "STATUS", "1");

  return IUP_NOERROR;
}

extern "C" void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winuiFontDlgPopup;

  iupClassRegisterAttribute(ic, "COLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
