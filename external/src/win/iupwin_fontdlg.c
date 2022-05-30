/** \file
 * \brief IupFontDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */
#include <windows.h>
#include <commdlg.h>

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drvfont.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"
#include "iupwin_str.h"


#ifndef CF_NOSCRIPTSEL
#define CF_NOSCRIPTSEL             0x00800000L
#endif

#define IUP_FONTFAMILYCOMBOBOX        0x0470
#define IUP_FONTCOLORLTEXT            0x0443
#define IUP_FONTCOLORCOMBOBOX         0x0473
#define IUP_FONTSCRIPTLTEXT           0x0446
#define IUP_FONTSCRIPTCOMBOBOX        0x0474

static UINT_PTR CALLBACK winFontDlgHookProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  (void)wParam;
  if (uiMsg == WM_INITDIALOG)
  {
    HWND hWndItem;
    CHOOSEFONT* choosefont = (CHOOSEFONT*)lParam;
    Ihandle* ih = (Ihandle*)choosefont->lCustData;

    char* value = iupAttribGet(ih, "TITLE");
    if (value)
      SetWindowText(hWnd, iupwinStrToSystem(value));

    ih->handle = hWnd;
    iupDialogUpdatePosition(ih);
    ih->handle = NULL;  /* reset handle */

    iupAttribSet(ih, "HWND", (char*)hWnd);  /* used by HELP_CB in winDialogBaseProc */

    hWndItem = GetDlgItem(hWnd, IUP_FONTFAMILYCOMBOBOX);
    SetFocus(hWndItem);

    if (!iupAttribGetBoolean(ih, "SHOWCOLOR"))
    {
      hWndItem = GetDlgItem(hWnd, IUP_FONTCOLORLTEXT);
      if (hWndItem)
        ShowWindow(hWndItem, SW_HIDE);
      hWndItem = GetDlgItem(hWnd, IUP_FONTCOLORCOMBOBOX);
      if (hWndItem)
        ShowWindow(hWndItem, SW_HIDE);
    }

    hWndItem = GetDlgItem(hWnd, IUP_FONTSCRIPTLTEXT);
    if (hWndItem)
      ShowWindow(hWndItem, SW_HIDE);
    hWndItem = GetDlgItem(hWnd, IUP_FONTSCRIPTCOMBOBOX);
    if (hWndItem)
      ShowWindow(hWndItem, SW_HIDE);
  }
  return 0;
}

static int winFontDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  unsigned char r, g, b;
  CHOOSEFONT choosefont;
  LOGFONT logfont;
  char* font;
  int height_pixels;
  char typeface[50] = "";
  int height = 8;
  int is_bold = 0,
    is_italic = 0, 
    is_underline = 0,
    is_strikeout = 0;
  int res = iupwinGetScreenRes();
  const char* mapped_name;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  /* if NOT set will NOT be Modal */
  /* anyway it will be modal only relative to its parent */
  if (!parent)
    parent = GetActiveWindow();

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
      char def_typeface[50] = "";
      int def_height = 8;
      int def_is_bold = 0,
        def_is_italic = 0,
        def_is_underline = 0,
        def_is_strikeout = 0;

      if (!iupGetFontInfo(IupGetGlobal("DEFAULTFONT"), def_typeface, &def_height, &def_is_bold, &def_is_italic, &def_is_underline, &def_is_strikeout))
        return IUP_ERROR;

      if (typeface[0] == 0)
        strcpy(typeface, def_typeface);
      if (height == 0)
        height = def_height;
      if (is_bold == 0)
        is_bold = def_is_bold;
      if (is_italic == 0)
        is_italic = def_is_italic;
      if (is_underline == 0)
        is_underline = def_is_underline;
      if (is_strikeout == 0)
        is_strikeout = def_is_strikeout;
    }
  }

  /* Map standard names to native names */
  mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    strcpy(typeface, mapped_name);

  /* get size in pixels */
  if (height < 0)
    height_pixels = height;  /* already in pixels */
  else
    height_pixels = -iupWIN_PT2PIXEL(height, res);

  if (height_pixels == 0)
    return IUP_ERROR;

  ZeroMemory(&choosefont, sizeof(CHOOSEFONT));
  choosefont.lStructSize = sizeof(CHOOSEFONT);

  if (iupStrToRGB(iupAttribGet(ih, "COLOR"), &r, &g, &b))
    choosefont.rgbColors = RGB(r, g, b);
  
  choosefont.hwndOwner = parent;
  choosefont.lpLogFont = &logfont;
  choosefont.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
  choosefont.lCustData = (LPARAM)ih;
  choosefont.lpfnHook = (LPCFHOOKPROC)winFontDlgHookProc;
  choosefont.Flags |= CF_ENABLEHOOK;

  if (IupGetCallback(ih, "HELP_CB"))
    choosefont.Flags |= CF_SHOWHELP;

  iupwinStrCopy(logfont.lfFaceName, typeface, sizeof(logfont.lfFaceName));

  logfont.lfHeight = height_pixels;
  logfont.lfWeight = (is_bold)? FW_BOLD: FW_NORMAL;
  logfont.lfItalic = (BYTE)is_italic;
  logfont.lfUnderline = (BYTE)is_underline;
  logfont.lfStrikeOut = (BYTE)is_strikeout;

  logfont.lfCharSet = DEFAULT_CHARSET;
  logfont.lfEscapement = 0; 
  logfont.lfOrientation = 0;
  logfont.lfWidth = 0; 
  logfont.lfOutPrecision = OUT_TT_PRECIS; 
  logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS; 
  logfont.lfQuality = DEFAULT_QUALITY; 
  logfont.lfPitchAndFamily = FF_DONTCARE|DEFAULT_PITCH; 

  if (!ChooseFont(&choosefont))
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "COLOR", NULL);
    iupAttribSet(ih, "STATUS", NULL);
    return IUP_NOERROR;
  }

  is_bold = (logfont.lfWeight == FW_NORMAL)? 0: 1;
  is_italic = logfont.lfItalic;
  is_underline = logfont.lfUnderline;
  is_strikeout = logfont.lfStrikeOut;
  height_pixels = logfont.lfHeight;

  if (height < 0) /* not an error, use old value as a reference for the units */
    height = height_pixels;   /* return in pixels */
  else
    height = iupWIN_PIXEL2PT(-height_pixels, res);   /* return in points */

  iupAttribSetStrf(ih, "VALUE", "%s, %s%s%s%s %d", iupwinStrFromSystem(logfont.lfFaceName), 
                                                    is_bold?"Bold ":"", 
                                                    is_italic?"Italic ":"", 
                                                    is_underline?"Underline ":"", 
                                                    is_strikeout?"Strikeout ":"", 
                                                    height);

  iupAttribSetStrf(ih, "COLOR", "%d %d %d", GetRValue(choosefont.rgbColors),
                                            GetGValue(choosefont.rgbColors),
                                            GetBValue(choosefont.rgbColors));
  iupAttribSet(ih, "STATUS", "1");

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winFontDlgPopup;

  /* IupFontDialog Windows Only */
  iupClassRegisterAttribute(ic, "COLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
