/** \file
 * \brief Frame Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_info.h"
#include "iupwin_str.h"


void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  /* LAYOUT_DECORATION_ESTIMATE */
  if (iupwin_comctl32ver6)
  {
    *x = 3;
    *y = 3;
  }
  else
  {
    *x = 2;
    *y = 2;
  }
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 1;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  (void)ih;
  (void)h;
  return 0;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  (void)w;
  (void)h;
  return 0;
}

static int winFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)value;

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    IupUpdate(ih); /* post a redraw */
    return 1;  /* save on the hash table */
  }
  else
    return 0;
}

static int winFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  iupwinSetTitleAttrib(ih, value);
  iupdrvPostRedraw(ih);
  return 1;
}

static void winFrameDrawText(HDC hDC, const char* text, int x, int y, COLORREF fgcolor)
{
  COLORREF oldcolor;

  SetTextAlign(hDC, TA_TOP|TA_LEFT);
  SetBkMode(hDC, TRANSPARENT);
  oldcolor = SetTextColor(hDC, fgcolor);

  {
    int len = (int)strlen(text);
    TCHAR* str = iupwinStrToSystemLen(text, &len);
    TextOut(hDC, x, y, str, len);
  }

  SetTextColor(hDC, oldcolor);
  SetBkMode(hDC, OPAQUE);
}

static void winFrameDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{ 
  iupwinBitmapDC bmpDC;
  HDC hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, 0, 0, drawitem->rcItem.right-drawitem->rcItem.left, 
                                                                  drawitem->rcItem.bottom-drawitem->rcItem.top);

  iupwinDrawParentBackground(ih, hDC, &drawitem->rcItem);

  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE"))
  {
    int x, y;
    HFONT hOldFont, hFont = (HFONT)iupwinGetHFontAttrib(ih);
    int txt_height = iupFrameGetTitleHeight(ih);
    COLORREF fgcolor;
    SIZE size;

    char* title = iupAttribGet(ih, "TITLE");
    if (!title) title = "";

    x = drawitem->rcItem.left+7;
    y = drawitem->rcItem.top;

    hOldFont = SelectObject(hDC, hFont);
    {
      TCHAR* str = iupwinStrToSystem(title);
      int len = lstrlen(str);
      GetTextExtentPoint32(hDC, str, len, &size);
    }
    ExcludeClipRect(hDC, x-2, y, x+size.cx+2, y+size.cy);

    drawitem->rcItem.top += txt_height/2;
    if (iupwin_comctl32ver6)
      iupwinDrawThemeFrameBorder(ih->handle, hDC, &drawitem->rcItem, drawitem->itemState);
    else
      DrawEdge(hDC, &drawitem->rcItem, EDGE_ETCHED, BF_RECT);

    SelectClipRgn(hDC, NULL);

    if (drawitem->itemState & ODS_DISABLED)
      fgcolor = GetSysColor(COLOR_GRAYTEXT);
    else
    {
      unsigned char r, g, b;
      char* color = iupAttribGetInherit(ih, "FGCOLOR");
      if (!color)
      {
        if (!iupwinDrawGetThemeFrameFgColor(ih->handle, &fgcolor))
          fgcolor = 0;  /* black */
      }
      else
      {
        if (iupStrToRGB(color, &r, &g, &b))
          fgcolor = RGB(r,g,b);
        else
          fgcolor = 0;  /* black */
      }
    }

    winFrameDrawText(hDC, title, x, y, fgcolor);

    SelectObject(hDC, hOldFont);
  }
  else
  {
    char* value = iupAttribGetStr(ih, "SUNKEN");
    if (iupStrBoolean(value))
      DrawEdge(hDC, &drawitem->rcItem, EDGE_SUNKEN, BF_RECT);
    else
      DrawEdge(hDC, &drawitem->rcItem, EDGE_ETCHED, BF_RECT);

    if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    {
      unsigned char r=0, g=0, b=0;
      char* color = iupAttribGetStr(ih, "BGCOLOR");
      iupStrToRGB(color, &r, &g, &b);
      SetDCBrushColor(hDC, RGB(r,g,b));
      InflateRect(&drawitem->rcItem, -2, -2);
      FillRect(hDC, &drawitem->rcItem, (HBRUSH)GetStockObject(DC_BRUSH));
    }
  }

  iupwinDrawDestroyBitmapDC(&bmpDC);
}

static int winFrameMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  switch (msg)
  {
  case WM_GETDLGCODE:
    {
      *result = DLGC_STATIC;  /* same return as GROUPBOX */
      return 1;
    }
  case WM_NCHITTEST:
    {
      *result = HTTRANSPARENT;  /* same return as GROUPBOX */
      return 1;
    }
  case WM_ERASEBKGND:
    {
      /* just to ignore the internal processing */
      *result = 1;
      return 1;
    }
  }

  return iupwinBaseContainerMsgProc(ih, msg, wp, lp, result);
}

static int winFrameMapMethod(Ihandle* ih)
{
  char *title;
  DWORD dwStyle = WS_CHILD|WS_CLIPSIBLINGS|
                  BS_OWNERDRAW, /* owner draw necessary because BS_GROUPBOX does not work ok */
      dwExStyle = 0;

  if (!ih->parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");
  if (title)
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  else
  {
    if (iupAttribGet(ih, "BGCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  iupwinGetNativeParentStyle(ih, &dwExStyle, &dwStyle);

  if (!iupwinCreateWindow(ih, WC_BUTTON, dwExStyle, dwStyle, NULL))
    return IUP_ERROR;

  /* replace the WinProc to handle other messages */
  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winFrameMsgProc);

  /* Process WM_DRAWITEM */
  IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winFrameDrawItem);  

  return IUP_NOERROR;
}

void iupdrvFrameInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winFrameMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, winFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);  

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TITLE", NULL, winFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
