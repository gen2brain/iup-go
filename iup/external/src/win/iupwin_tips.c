/** \file
 * \brief Windows Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <windows.h>
#include <commctrl.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_markup.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_str.h"
#include "iupwin_darkmode.h"


#ifndef TTTOOLINFO
#ifdef UNICODE
#define TTTOOLINFO TTTOOLINFOW
#else
#define TTTOOLINFO TTTOOLINFOA
#endif
#endif

#ifndef TTM_SETTITLE
#ifdef UNICODE
#define TTM_SETTITLE TTM_SETTITLEW
#else
#define TTM_SETTITLE TTM_SETTITLEA
#endif
#endif

#ifndef TTM_POPUP   /* Not defined for MingW and Cygwin */
#define TTM_POPUP  (WM_USER + 34)
#endif

static HWND winTipsCreate(HWND hParent)
{
  RECT rect = {1,1,1,1};
  HWND tips_hwnd = iupwinCreateWindowEx(hParent, TOOLTIPS_CLASS, WS_EX_TOPMOST, TTS_ALWAYSTIP|TTS_NOPREFIX, 0, NULL);
  SendMessage(tips_hwnd, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT)3000);
  SendMessage(tips_hwnd, TTM_SETMARGIN, (WPARAM)0, (LPARAM)&rect);
  return tips_hwnd;
}

static void winTipsSendMessage(Ihandle* ih, HWND tips_hwnd, UINT msg)
{
  TTTOOLINFO ti;

  ZeroMemory(&ti, sizeof(TTTOOLINFO));
  if (iupwin_comctl32ver6)
    ti.cbSize = sizeof(TTTOOLINFO);
  else
    ti.cbSize = sizeof(TTTOOLINFO)-sizeof(void*);  /* fix for no visual styles and Unicode */
  ti.uFlags = TTF_SUBCLASS;
  ti.hinst = iupwin_hinstance;
  ti.uId = 0;
  ti.hwnd = ih->handle;
  ti.lpszText = LPSTR_TEXTCALLBACK;
  ti.rect.right = 3000;
  ti.rect.bottom = 3000;

  SendMessage(tips_hwnd, msg, 0, (LPARAM)&ti);
}

IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  HWND tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  if (!tips_hwnd)
  {
    tips_hwnd = winTipsCreate(ih->handle);

    iupwinHandleAdd(ih, tips_hwnd);
    iupAttribSet(ih, "_IUPWIN_TIPSWIN", (char*)tips_hwnd);
  }

  if (value)
  {
    int tool_exists = (int)SendMessage(tips_hwnd, TTM_GETCURRENTTOOL, 0, 0);
    if (!tool_exists)
      winTipsSendMessage(ih, tips_hwnd, TTM_ADDTOOL);
  }
  else
    winTipsSendMessage(ih, tips_hwnd, TTM_DELTOOL);

  return 1;
}

IUP_DRV_API void iupwinTipsDestroy(Ihandle* ih)
{
  HWND tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  if (tips_hwnd)
  {
    winTipsSendMessage(ih, tips_hwnd, TTM_DELTOOL);

    iupAttribSet(ih, "_IUPWIN_TIPSWIN", NULL);

    iupwinHandleRemove(tips_hwnd);
    DestroyWindow(tips_hwnd);
  }
}

IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  HWND tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  if (!tips_hwnd)
    return 0;

  if (iupStrBoolean(value))
    SendMessage(tips_hwnd, TTM_POPUP, 0, 0);  /* Works in Visual Styles Only */
  else
    SendMessage(tips_hwnd, TTM_POP, 0, 0);

  return 0;
}

IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  HWND tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  if (!tips_hwnd)
    return NULL;

  return iupStrReturnBoolean(IsWindowVisible(tips_hwnd));
}

IUP_DRV_API void iupwinTipsUpdateInfo(Ihandle* ih, HWND tips_hwnd)
{
  COLORREF color, tip_color;
  unsigned char r, g, b;
  char* value;

  if (!IsWindow(ih->handle))
    return;

  if (!IsWindow(tips_hwnd))
    return;

  {
    HFONT hfont;
    value = iupAttribGetStr(ih, "TIPFONT");
    if (value)
    {
      if (iupStrEqualNoCase(value, "SYSTEM"))
        hfont = NULL;
      else
        hfont = iupwinGetHFont(ih, value);
    }
    else
      hfont = (HFONT)iupwinGetHFontAttrib(ih);

    if (hfont)
    {
      HFONT tip_hfont = (HFONT)SendMessage(tips_hwnd, WM_GETFONT, 0, 0);
      if (tip_hfont != hfont)
        SendMessage(tips_hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE,0));
    }
  }

  if (iupwinGetColorRef(ih, "TIPBGCOLOR", &color))
  {
    tip_color = (COLORREF)SendMessage(tips_hwnd, TTM_GETTIPBKCOLOR, 0, 0);
    if (color != tip_color)
      SendMessage(tips_hwnd, TTM_SETTIPBKCOLOR, (WPARAM)color, 0);
  }
  else if (iupwinDarkModeEnabled() && iupStrToRGB(IupGetGlobal("TXTBGCOLOR"), &r, &g, &b))
    SendMessage(tips_hwnd, TTM_SETTIPBKCOLOR, (WPARAM)RGB(r, g, b), 0);

  if (iupwinGetColorRef(ih, "TIPFGCOLOR", &color))
  {
    tip_color = (COLORREF)SendMessage(tips_hwnd, TTM_GETTIPTEXTCOLOR, 0, 0);
    if (color != tip_color)
      SendMessage(tips_hwnd, TTM_SETTIPTEXTCOLOR, (WPARAM)color, 0);
  }
  else if (iupwinDarkModeEnabled() && iupStrToRGB(IupGetGlobal("TXTFGCOLOR"), &r, &g, &b))
    SendMessage(tips_hwnd, TTM_SETTIPTEXTCOLOR, (WPARAM)RGB(r, g, b), 0);

  {
    int balloon = IupGetInt(ih, "TIPBALLOON");  /* must use IupGetInt to use inheritance */
    DWORD style = GetWindowLong(tips_hwnd, GWL_STYLE);
    int tip_balloon = (style & TTS_BALLOON)? 1: 0;
    if (tip_balloon != balloon)
    {
      if (balloon)
        style |= TTS_BALLOON;
      else
        style &= ~TTS_BALLOON;
      SetWindowLong(tips_hwnd, GWL_STYLE, style);
    }

    if (balloon)
    {
      int balloon_icon = IupGetInt(ih, "TIPBALLOONTITLEICON");  /* must use IupGetInt to use inheritance */
      TCHAR* balloon_title = iupwinStrToSystem(IupGetAttribute(ih, "TIPBALLOONTITLE")); /* must use IupGetAttribute to use inheritance */
      SendMessage(tips_hwnd, TTM_SETTITLE, balloon_icon, (LPARAM)balloon_title);
    }
    else
      SendMessage(tips_hwnd, TTM_SETTITLE, 0, 0);
  }

  {
    int delay = IupGetInt(ih, "TIPDELAY"); /* must use IupGetInt to use inheritance */
    int tip_delay = (int)SendMessage(tips_hwnd, TTM_GETDELAYTIME, TTDT_AUTOPOP, 0);
    if (delay != tip_delay)
      SendMessage(tips_hwnd, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAKELONG(delay, 0));
  }

  {
    TTTOOLINFO ti;

    ZeroMemory(&ti, sizeof(TTTOOLINFO));
    if (iupwin_comctl32ver6)
      ti.cbSize = sizeof(TTTOOLINFO);
    else
      ti.cbSize = sizeof(TTTOOLINFO)-sizeof(void*);  /* fix for no visual styles and Unicode */
    ti.uId = 0;
    ti.hwnd = ih->handle;

    value = iupAttribGet(ih, "TIPRECT");
    if (value)
    {
      int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
      if (iupStrToRect(value, &x1, &y1, &x2, &y2))
      {
        ti.rect.left = x1; ti.rect.right = x2;
        ti.rect.top = y1; ti.rect.bottom = y2;
      }
      else
        GetClientRect(ih->handle, &ti.rect);
    }
    else
      GetClientRect(ih->handle, &ti.rect);

    SendMessage(tips_hwnd, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
  }
}

IUP_DRV_API void iupwinTipsGetDispInfo(LPARAM lp)
{
  Ihandle* ih;
  HWND tips_hwnd;
  NMTTDISPINFO* tips_info;
  IFnii cb;

  if (!lp)
    return;

  tips_info = (NMTTDISPINFO*)lp;
  ih = iupwinHandleGet(tips_info->hdr.hwndFrom);  /* hwndFrom is the tooltip window */
  if (!iupObjectCheck(ih))
    return;

  tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  if (tips_hwnd != tips_info->hdr.hwndFrom)
    return;

  tips_info->hinst = NULL;

  cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
  if (cb)
  {
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    iupdrvScreenToClient(ih, &x, &y);
    cb(ih, x, y);
  }

  if (iupAttribGetBoolean(ih, "TIPMARKUP"))
  {
    char* plain = iupMarkupStripTags(iupAttribGet(ih, "TIP"));
    tips_info->lpszText = iupwinStrToSystem(plain);
    free(plain);
  }
  else
    tips_info->lpszText = iupwinStrToSystem(iupAttribGet(ih, "TIP"));

  iupwinTipsUpdateInfo(ih, tips_hwnd);
}

static HFONT winTipsMarkupFont(HDC hdc, const LOGFONT* base, ImarkupRun* run)
{
  LOGFONT lf = *base;
  double scale = pow(1.2, run->big) * pow(0.83, run->small_size);

  if (run->superscript || run->subscript)
    scale *= 0.8;

  if (run->font_size > 0)
    lf.lfHeight = -MulDiv(run->font_size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  lf.lfHeight = (LONG)floor(lf.lfHeight * scale + (lf.lfHeight < 0 ? -0.5 : 0.5));

  if (run->bold || run->font_weight >= 600)
    lf.lfWeight = FW_BOLD;
  else if (run->font_weight > 0)
    lf.lfWeight = run->font_weight;
  if (run->italic || run->font_style)
    lf.lfItalic = TRUE;
  if (run->underline)
    lf.lfUnderline = TRUE;
  if (run->strikethrough)
    lf.lfStrikeOut = TRUE;
  if (run->font_family)
    MultiByteToWideChar(CP_UTF8, 0, run->font_family, -1, lf.lfFaceName, LF_FACESIZE);

  return CreateFontIndirect(&lf);
}

static SIZE winTipsMarkupPaint(Ihandle* ih, HDC hdc, HFONT base_font, const RECT* rect, int draw)
{
  SIZE total = {0, 0};
  LOGFONT base;
  ImarkupData* data = iupMarkupParse(iupAttribGet(ih, "TIP"));
  int line = 0, max_lines = 1, i, pass;
  int* ascent, *descent, *width;
  HFONT old_font;

  if (!data)
    return total;

  GetObject(base_font, sizeof(LOGFONT), &base);

  for (i = 0; i < data->count; i++)
  {
    const char* c;
    for (c = data->runs[i].text; c && *c; c++)
    {
      if (*c == '\n')
        max_lines++;
    }
  }

  ascent = (int*)calloc(max_lines, sizeof(int));
  descent = (int*)calloc(max_lines, sizeof(int));
  width = (int*)calloc(max_lines, sizeof(int));
  old_font = (HFONT)SelectObject(hdc, base_font);

  if (ascent && descent && width)
  {
    for (pass = 0; pass < 2; pass++)
    {
      int x = rect->left, y = rect->top;
      line = 0;

      if (pass == 1)
      {
        if (!draw)
          break;
        SetBkMode(hdc, TRANSPARENT);
        SetTextAlign(hdc, TA_BASELINE | TA_LEFT);
      }

      for (i = 0; i < data->count; i++)
      {
        ImarkupRun* run = &data->runs[i];
        const char* text = run->text;
        HFONT font = winTipsMarkupFont(hdc, &base, run);
        TEXTMETRIC tm;
        int shift;

        SelectObject(hdc, font);
        GetTextMetrics(hdc, &tm);
        shift = run->superscript ? -(tm.tmAscent / 2) : (run->subscript ? tm.tmAscent / 3 : 0);

        while (text)
        {
          const char* nl = strchr(text, '\n');
          int len = nl ? (int)(nl - text) : (int)strlen(text);
          int wlen = len ? MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0) : 0;
          WCHAR* wtext = wlen ? (WCHAR*)malloc(wlen * sizeof(WCHAR)) : NULL;
          SIZE ext = {0, 0};

          if (wtext)
          {
            MultiByteToWideChar(CP_UTF8, 0, text, len, wtext, wlen);
            GetTextExtentPoint32W(hdc, wtext, wlen, &ext);
          }

          if (pass == 0)
          {
            if (tm.tmAscent - shift > ascent[line]) ascent[line] = tm.tmAscent - shift;
            if (tm.tmDescent + shift > descent[line]) descent[line] = tm.tmDescent + shift;
            width[line] += ext.cx;
          }
          else if (wtext)
          {
            int baseline = y + ascent[line];
            unsigned char r, g, b;

            if (run->bg_color && iupStrToRGB(run->bg_color, &r, &g, &b))
            {
              RECT bg = {x, y, x + ext.cx, y + ascent[line] + descent[line]};
              SetDCBrushColor(hdc, RGB(r, g, b));
              FillRect(hdc, &bg, (HBRUSH)GetStockObject(DC_BRUSH));
            }

            if (run->fg_color && iupStrToRGB(run->fg_color, &r, &g, &b))
              SetTextColor(hdc, RGB(r, g, b));
            else
              SetTextColor(hdc, (COLORREF)SendMessage((HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN"), TTM_GETTIPTEXTCOLOR, 0, 0));

            TextOutW(hdc, x, baseline + shift, wtext, wlen);
          }

          x += ext.cx;
          free(wtext);

          if (nl)
          {
            if (pass == 1)
              y += ascent[line] + descent[line];
            line++;
            x = rect->left;
            text = nl + 1;
          }
          else
            text = NULL;
        }

        SelectObject(hdc, base_font);
        DeleteObject(font);
      }
    }

    for (i = 0; i < max_lines; i++)
    {
      if (width[i] > total.cx)
        total.cx = width[i];
      total.cy += ascent[i] + descent[i];
    }
  }

  SelectObject(hdc, old_font);
  free(ascent);
  free(descent);
  free(width);
  iupMarkupFree(data);
  return total;
}

IUP_DRV_API int iupwinTipsNotify(Ihandle* ih, NMHDR* msg_info, LRESULT* result)
{
  HWND tips_hwnd = (HWND)iupAttribGet(ih, "_IUPWIN_TIPSWIN");
  if (!tips_hwnd || msg_info->hwndFrom != tips_hwnd || !iupAttribGetBoolean(ih, "TIPMARKUP"))
    return 0;

  if (msg_info->code == NM_CUSTOMDRAW)
  {
    NMTTCUSTOMDRAW* cd = (NMTTCUSTOMDRAW*)msg_info;
    if (cd->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
      winTipsMarkupPaint(ih, cd->nmcd.hdc, (HFONT)SendMessage(tips_hwnd, WM_GETFONT, 0, 0), &cd->nmcd.rc, 1);
      *result = CDRF_SKIPDEFAULT;
      return 1;
    }
  }
  else if (msg_info->code == TTN_SHOW)
  {
    HDC hdc = GetDC(tips_hwnd);
    RECT rect, text_rect = {0, 0, 0, 0};
    SIZE size = winTipsMarkupPaint(ih, hdc, (HFONT)SendMessage(tips_hwnd, WM_GETFONT, 0, 0), &text_rect, 0);
    ReleaseDC(tips_hwnd, hdc);

    GetWindowRect(tips_hwnd, &rect);
    text_rect.right = size.cx;
    text_rect.bottom = size.cy;
    SendMessage(tips_hwnd, TTM_ADJUSTRECT, TRUE, (LPARAM)&text_rect);
    SetWindowPos(tips_hwnd, NULL, rect.left, rect.top, text_rect.right - text_rect.left, text_rect.bottom - text_rect.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    *result = TRUE;
    return 1;
  }

  return 0;
}
