/** \file
 * \brief Draw Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <uxtheme.h>

#if (_MSC_VER >= 1700)  /* Visual C++ 11.0 ( Visual Studio 2012) */
#include <vssym32.h>
#else
#include <tmschema.h>
#endif


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_image.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupwin_drv.h"
#include "iupwin_info.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"


#ifndef TABP_AEROWIZARDBODY
#define TABP_AEROWIZARDBODY  11  /* Not defined for MingW and Cygwin */
#endif


/******************************************************************************
                             Themes
*******************************************************************************/


typedef HTHEME  (STDAPICALLTYPE *_winThemeOpenData)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (STDAPICALLTYPE *_winThemeCloseData)(HTHEME hTheme);
typedef HRESULT (STDAPICALLTYPE *_winThemeDrawBackground)(HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);
typedef HRESULT (STDAPICALLTYPE *_winThemeGetColor)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF *pColor);

static _winThemeOpenData winThemeOpenData = NULL;
static _winThemeCloseData winThemeCloseData = NULL;
static _winThemeDrawBackground winThemeDrawBackground = NULL;
static _winThemeGetColor winThemeGetColor = NULL;

typedef BOOL (CALLBACK* _winAlphaBlendFunc)( HDC hdcDest, 
                             int xoriginDest, int yoriginDest, 
                             int wDest, int hDest, HDC hdcSrc, 
                             int xoriginSrc, int yoriginSrc, 
                             int wSrc, int hSrc, 
                             BLENDFUNCTION ftn);
static _winAlphaBlendFunc winAlphaBlend = NULL;

static int winDrawThemeEnabled(void)
{
  return winThemeOpenData? 1: 0;
}

void iupwinDrawThemeInit(void)
{
  if (!winAlphaBlend)
  {
    HINSTANCE lib = LoadLibrary(TEXT("Msimg32"));
    if (lib)
      winAlphaBlend = (_winAlphaBlendFunc)GetProcAddress(lib, "AlphaBlend");
  }

  if (!winThemeOpenData && iupwin_comctl32ver6)
  {
    HMODULE hinstDll = LoadLibrary(TEXT("uxtheme.dll"));
    if (hinstDll)
    {
      winThemeOpenData = (_winThemeOpenData)GetProcAddress(hinstDll, "OpenThemeData");
      winThemeCloseData = (_winThemeCloseData)GetProcAddress(hinstDll, "CloseThemeData");
      winThemeDrawBackground = (_winThemeDrawBackground)GetProcAddress(hinstDll, "DrawThemeBackground");
      winThemeGetColor = (_winThemeGetColor)GetProcAddress(hinstDll, "GetThemeColor");
    }
  }
}

static int winDrawGetThemeStateId(int itemState)
{
  if (itemState & ODS_DISABLED)
    return PBS_DISABLED; 
  else if (itemState & ODS_SELECTED)
    return PBS_PRESSED;  
  else if (itemState & ODS_HOTLIGHT)
    return PBS_HOT;      
  else if (itemState & ODS_DEFAULT)
    return PBS_DEFAULTED;
  else
    return PBS_NORMAL;       
}

static int winDrawThemeButtonBorder(HWND hWnd, HDC hDC, RECT *rect, UINT itemState)
{
  int iStateId;
  HTHEME hTheme;

  if (!winDrawThemeEnabled()) 
    return 0; 

  hTheme = winThemeOpenData(hWnd, L"BUTTON");
  if (!hTheme) 
    return 0;

  iStateId = winDrawGetThemeStateId(itemState);

  winThemeDrawBackground(hTheme, hDC, BP_PUSHBUTTON, iStateId, rect, NULL);

  winThemeCloseData(hTheme);
  return 1;
}

static int winDrawTheme3StateButton(HWND hWnd, HDC hDC, RECT *rect)
{
  HTHEME hTheme;

  if (!winDrawThemeEnabled()) 
    return 0; 

  hTheme = winThemeOpenData(hWnd, L"BUTTON");
  if (!hTheme) 
    return 0;

  winThemeDrawBackground(hTheme, hDC, BP_CHECKBOX, CBS_MIXEDNORMAL, rect, NULL);

  winThemeCloseData(hTheme);
  return 1;
}

void iupwinDrawThemeFrameBorder(HWND hWnd, HDC hDC, RECT *rect, UINT itemState)
{
  int iStateId = GBS_NORMAL;
  HTHEME hTheme;

  if (!winDrawThemeEnabled()) 
    return; 

  hTheme = winThemeOpenData(hWnd, L"BUTTON");
  if (!hTheme) 
    return;

  if (itemState & ODS_DISABLED)
    iStateId = GBS_DISABLED;

  winThemeDrawBackground(hTheme, hDC, BP_GROUPBOX, iStateId, rect, NULL);

  winThemeCloseData(hTheme);
}

int iupwinDrawGetThemeTabsBgColor(HWND hWnd, COLORREF *color)
{
  HTHEME hTheme;
  HRESULT ret;

  if (!winDrawThemeEnabled()) 
    return 0; 

  hTheme = winThemeOpenData(hWnd, L"TAB");
  if (!hTheme) 
    return 0;

  if (iupwinIsVistaOrNew())
    ret = winThemeGetColor(hTheme, TABP_AEROWIZARDBODY, TIS_NORMAL, TMT_FILLCOLORHINT, color);
  else
    ret = winThemeGetColor(hTheme, TABP_BODY, TIS_NORMAL, TMT_FILLCOLORHINT, color);

  winThemeCloseData(hTheme);
  return (ret == S_OK)? 1: 0;
}

int iupwinDrawGetThemeButtonBgColor(HWND hWnd, COLORREF *color)
{
  HTHEME hTheme;
  HRESULT ret;

  if (!winDrawThemeEnabled()) 
    return 0; 

  hTheme = winThemeOpenData(hWnd, L"BUTTON");
  if (!hTheme) 
    return 0;

  ret = winThemeGetColor(hTheme, BP_PUSHBUTTON, PBS_NORMAL, TMT_FILLCOLORHINT, color);

  winThemeCloseData(hTheme);
  return (ret == S_OK)? 1: 0;
}

int iupwinDrawGetThemeFrameFgColor(HWND hWnd, COLORREF *color)
{
  HTHEME hTheme;
  HRESULT ret;

  if (!winDrawThemeEnabled()) 
    return 0; 

  hTheme = winThemeOpenData(hWnd, L"BUTTON");
  if (!hTheme) 
    return 0;

  ret = winThemeGetColor(hTheme, BP_GROUPBOX, GBS_NORMAL, TMT_TEXTCOLOR, color);

  winThemeCloseData(hTheme);
  return (ret == S_OK)? 1: 0;
}

void iupwinDrawRemoveTheme(HWND hwnd)
{
  typedef HRESULT (STDAPICALLTYPE *winSetWindowTheme)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
  static winSetWindowTheme mySetWindowTheme = NULL;
  if (!mySetWindowTheme)
  {
    HMODULE hinstDll = LoadLibrary(TEXT("uxtheme.dll"));
    if (hinstDll)
      mySetWindowTheme = (winSetWindowTheme)GetProcAddress(hinstDll, "SetWindowTheme");
  }

  if (mySetWindowTheme)
    mySetWindowTheme(hwnd, L"", L"");
}


/******************************************************************************
                             Utilities
*******************************************************************************/


void iupwinDrawText(HDC hDC, const char* text, int x, int y, int width, int height, HFONT hFont, COLORREF fgcolor, int style)
{
  COLORREF oldcolor;
  RECT rect;
  HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

  rect.left = x;
  rect.top = y;
  rect.right = x+width;
  rect.bottom = y+height;

  SetTextAlign(hDC, TA_TOP|TA_LEFT);
  SetBkMode(hDC, TRANSPARENT);
  oldcolor = SetTextColor(hDC, fgcolor);

  DrawText(hDC, iupwinStrToSystem(text), -1, &rect, style|DT_NOCLIP);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, oldcolor);
  SetBkMode(hDC, OPAQUE);
}

void iupwinDrawBitmap(HDC hDC, HBITMAP hBitmap, int x, int y, int w, int h, int img_w, int img_h, int bpp)
{
  HDC hMemDC = CreateCompatibleDC(hDC);
  HBITMAP oldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

  if (bpp == 32 && winAlphaBlend)
  {
    BLENDFUNCTION blendfunc;
    blendfunc.BlendOp = AC_SRC_OVER;
    blendfunc.BlendFlags = 0;
    blendfunc.SourceConstantAlpha = 0xFF;
    blendfunc.AlphaFormat = AC_SRC_ALPHA;

    winAlphaBlend(hDC, x, y, w, h, hMemDC, 0, 0, img_w, img_h, blendfunc);
  }
  else
  {
    if (w == img_w && h == img_h)
      BitBlt(hDC, x, y, img_w, img_h, hMemDC, 0, 0, SRCCOPY);
    else
      StretchBlt(hDC, x, y, w, h, hMemDC, 0, 0, img_w, img_h, SRCCOPY);
  }

  SelectObject(hMemDC, oldBitmap);
  DeleteDC(hMemDC);  /* to match CreateCompatibleDC */
}

static int winDrawGetStateId(int itemState)
{
  if (itemState & ODS_DISABLED)
    return DFCS_INACTIVE; 
  else if (itemState & ODS_SELECTED)
    return DFCS_PUSHED;  
  else if (itemState & ODS_HOTLIGHT)
    return DFCS_HOT;      
  else
    return 0;   
}

void iupwinDrawButtonBorder(HWND hWnd, HDC hDC, RECT *rect, UINT itemState)
{
  if (!winDrawThemeButtonBorder(hWnd, hDC, rect, itemState))
  {
    DrawFrameControl(hDC, rect, DFC_BUTTON, DFCS_BUTTONPUSH | winDrawGetStateId(itemState));
    if (itemState & ODS_DEFAULT)  /* if a button has the focus, must draw the default button frame */
      FrameRect(hDC, rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
  }
}

void iupwinDraw3StateButton(HWND hWnd, HDC hDC, RECT *rect)
{
  if (!winDrawTheme3StateButton(hWnd, hDC, rect))
  {
    /* only used from TreeView where the 3state images are 3px smaller than icon size */
    rect->left += 2;
    rect->top += 2;
    rect->right -= 1;
    rect->bottom -= 1;
    DrawFrameControl(hDC, rect, DFC_BUTTON, DFCS_BUTTON3STATE | DFCS_CHECKED | DFCS_FLAT);
  }
}

void iupwinDrawParentBackground(Ihandle* ih, HDC hDC, RECT* rect)
{
  char* color_str = iupBaseNativeParentGetBgColorAttrib(ih);
  long color = iupDrawStrToColor(color_str, 0);
  SetDCBrushColor(hDC, RGB(iupDrawRed(color),iupDrawGreen(color),iupDrawBlue(color)));
  FillRect(hDC, rect, (HBRUSH)GetStockObject(DC_BRUSH));
}

HDC iupwinDrawCreateBitmapDC(iupwinBitmapDC *bmpDC, HDC hDC, int x, int y, int w, int h)
{
  bmpDC->x = x;
  bmpDC->y = y;
  bmpDC->w = w;
  bmpDC->h = h;
  bmpDC->hDC = hDC;

  bmpDC->hBitmap = CreateCompatibleBitmap(bmpDC->hDC, w, h);
  bmpDC->hBitmapDC = CreateCompatibleDC(bmpDC->hDC);
  bmpDC->hOldBitmap = SelectObject(bmpDC->hBitmapDC, bmpDC->hBitmap);
  return bmpDC->hBitmapDC;
}

void iupwinDrawDestroyBitmapDC(iupwinBitmapDC *bmpDC)
{
  BitBlt(bmpDC->hDC, bmpDC->x, bmpDC->y, bmpDC->w, bmpDC->h, bmpDC->hBitmapDC, 0, 0, SRCCOPY);
  SelectObject(bmpDC->hBitmapDC, bmpDC->hOldBitmap);
  DeleteObject(bmpDC->hBitmap);
  DeleteDC(bmpDC->hBitmapDC);  /* to match CreateCompatibleDC */
}

int iupwinCustomDrawToDrawItem(Ihandle* ih, NMHDR* msg_info, int *result, IFdrawItem drawitem_cb)
{
  NMCUSTOMDRAW *customdraw = (NMCUSTOMDRAW*)msg_info;

  if (customdraw->dwDrawStage == CDDS_PREERASE)
  {
    DRAWITEMSTRUCT drawitem;
    drawitem.itemState = 0;

    /* combination of the following flags. */
    if (customdraw->uItemState & CDIS_DISABLED)
      drawitem.itemState |= ODS_DISABLED;
    if (customdraw->uItemState & CDIS_SELECTED)
      drawitem.itemState |= ODS_SELECTED;
    if (customdraw->uItemState & CDIS_HOT)
      drawitem.itemState |= ODS_HOTLIGHT;
    if (customdraw->uItemState & CDIS_DEFAULT)
      drawitem.itemState |= ODS_DEFAULT;

    if (customdraw->uItemState & CDIS_FOCUS)
      drawitem.itemState |= ODS_FOCUS;

    if (!(customdraw->uItemState & CDIS_SHOWKEYBOARDCUES))
      drawitem.itemState |= ODS_NOFOCUSRECT | ODS_NOACCEL;

    drawitem.hDC = customdraw->hdc;
    drawitem.rcItem = customdraw->rc;

    drawitem_cb(ih, (void*)&drawitem);

    *result = CDRF_SKIPDEFAULT;
    return 1;
  }
  else
    return 0;
}