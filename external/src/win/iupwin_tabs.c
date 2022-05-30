/** \file
* \brief Tabs Control
*
* See Copyright Notice in "iup.h"
*/

#include <windows.h>
#include <commctrl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_tabs.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_assert.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_childtree.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_info.h"
#include "iupwin_str.h"


#ifndef WS_EX_COMPOSITED
#define WS_EX_COMPOSITED        0x02000000L
#endif

#define ITABS_CLOSE_SIZE 12

static void winTabsInitializeCloseImage(void)
{
  Ihandle *image_close;

  unsigned char img_close[ITABS_CLOSE_SIZE * ITABS_CLOSE_SIZE] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 
    0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 
    0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 
    0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  };
  
  image_close = IupImage(ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, img_close);
  IupSetAttribute(image_close, "0", "BGCOLOR");
  IupSetAttribute(image_close, "1", "0 0 0");
  IupSetHandle("IMGCLOSE", image_close);

  image_close = IupImage(ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, img_close);
  IupSetAttribute(image_close, "0", "BGCOLOR");
  IupSetStrAttribute(image_close, "1", IupGetGlobal("TXTHLCOLOR"));
  IupSetHandle("IMGCLOSEHIGH", image_close);
}

static void winTabsInitVisibleArray(Iarray* visible_array, int count)
{
  int old_count = iupArrayCount(visible_array);
  if (old_count != count)
  {
    iupArrayRemove(visible_array, 0, old_count);
    iupArrayAdd(visible_array, count);
  }

  /* all visible by default */
  {
    int pos;
    int* visible_array_array_data = (int*)iupArrayGetData(visible_array);
    for (pos = 0; pos < count; pos++)
      visible_array_array_data[pos] = 1;  
  }
}

static Iarray* winTabsGetVisibleArray(Ihandle* ih)
{
  Iarray* visible_array = (Iarray*)iupAttribGet(ih, "_IUPWIN_VISIBLEARRAY");

  if (!visible_array)
  {
    int count = IupGetChildCount(ih);

    /* create the array if does not exist */
    visible_array = iupArrayCreate(50, sizeof(int));
    iupAttribSet(ih, "_IUPWIN_VISIBLEARRAY", (char*)visible_array);

    winTabsInitVisibleArray(visible_array, count);
  }

  return visible_array;
}

/* #define PRINT_VISIBLE_ARRAY 1 */
#if PRINT_VISIBLE_ARRAY
/* For debugging  */
static void winTabsPrintVisibleArray(Ihandle* ih)
{
  Iarray* visible_array = winTabsGetVisibleArray(ih);
  int i, count = iupArrayCount(visible_array);
  int* visible_array_array_data = (int*)iupArrayGetData(visible_array);

  for (i = 0; i < count; i++)
    printf("%d=%d ", i, visible_array_array_data[i]);

  printf("\n");
}
#endif

static int winTabsHasInVisible(Iarray* visible_array)
{
  int i;
  int count = iupArrayCount(visible_array);
  int* visible_array_array_data = (int*)iupArrayGetData(visible_array);

  for (i = 0; i < count; i++)
  {
    if (visible_array_array_data[i] == 0)
      return 1;
  }

  return 0;
}

static void winTabsSetVisibleArrayItem(Ihandle* ih, int pos, int visible)
{
  /* if set to visible and all are visible, just return */
  if (visible && !ih->data->has_invisible)
    return;
  else
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    int* visible_array_array_data = (int*)iupArrayGetData(visible_array);

    /* first invisible init array */
    if (!visible && !ih->data->has_invisible)
    {
      int count = IupGetChildCount(ih);
      winTabsInitVisibleArray(visible_array, count);
    }

    visible_array_array_data[pos] = visible;

    if (!visible)
      ih->data->has_invisible = 1;
    else
      ih->data->has_invisible = winTabsHasInVisible(visible_array);
  }
}

static void winTabsInsertVisibleArrayItem(Ihandle* ih, int pos)
{
  if (ih->data->has_invisible)
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    iupArrayInsert(visible_array, pos, 1);
    winTabsSetVisibleArrayItem(ih, pos, 1);
  }
}

static void winTabsDeleteVisibleArrayItem(Ihandle* ih, int pos)
{
  if (ih->data->has_invisible)
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    iupArrayRemove(visible_array, pos, 1);
    ih->data->has_invisible = winTabsHasInVisible(visible_array);
  }
}

static int winTabsIsTabVisible(Ihandle* ih, int pos)
{
  if (ih->data->has_invisible)
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    int* visible_array_array_data = (int*)iupArrayGetData(visible_array);
    return visible_array_array_data[pos];
  }
  else
    return 1;
}

static int winTabsGetInsertPos(Ihandle* ih, int pos)
{
  if (ih->data->has_invisible)
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    int* visible_array_array_data = (int*)iupArrayGetData(visible_array);
    int i, p;

    for (i = 0, p = 0; i < pos; i++)
    {
      if (visible_array_array_data[i] != 0)
        p++;
    }

    return p;
  }
  else
    return pos;
}

static int winTabsPosFixToWin(Ihandle* ih, int pos)
{
  if (ih->data->has_invisible)
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    int* visible_array_array_data = (int*)iupArrayGetData(visible_array);
    int i, p;

    if (!visible_array_array_data[pos])
      return -1;

    for (i = 0, p = 0; i<pos; i++)
    {
      if (visible_array_array_data[i] != 0)
        p++;
    }

    return p;
  }
  else
    return pos;
}

static int winTabsPosFixFromWin(Ihandle* ih, int p)
{
  if (ih->data->has_invisible)
  {
    Iarray* visible_array = winTabsGetVisibleArray(ih);
    int* visible_array_array_data = (int*)iupArrayGetData(visible_array);
    int pos, pp, count = iupArrayCount(visible_array);

    for (pos = 0, pp = -1; pos<count; pos++)
    {
      if (visible_array_array_data[pos] != 0)
        pp++;

      if (pp == p)
        return pos;
    }

    iupERROR("IupTabs Error. Invalid internal visible state.");
    return 0;  /* INTERNAL ERROR: must always be found */
  }
  else
    return p;
}

static int winTabsGetPageWindowPos(Ihandle* ih, HWND tab_container)
{
  TCITEM tie;
  int p, num_tabs;

  num_tabs = (int)SendMessage(ih->handle, TCM_GETITEMCOUNT, 0, 0);
  tie.mask = TCIF_PARAM;

  for (p = 0; p<num_tabs; p++)
  {
    SendMessage(ih->handle, TCM_GETITEM, p, (LPARAM)&tie);
    if (tab_container == (HWND)tie.lParam)
      return p;
  }

  return -1;
}

int iupdrvTabsExtraMargin(void)
{
  if (iupwin_comctl32ver6 && iupwinIsWin10OrNew())
    return 2;
  return 0;
}

int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  return (int)SendMessage(ih->handle, TCM_GETROWCOUNT, 0, 0);
}

static HWND winTabsGetPageWindow(Ihandle* ih, int pos)
{
  int p = winTabsPosFixToWin(ih, pos);
  if (p >= 0)
  {
    TCITEM tie;
    tie.mask = TCIF_PARAM;
    SendMessage(ih->handle, TCM_GETITEM, p, (LPARAM)&tie);
    return (HWND)tie.lParam;
  }
  else
    return NULL;  /* invisible */
}

void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  int p = winTabsPosFixToWin(ih, pos);
  if (p >= 0)
  {
    int curr_pos = iupdrvTabsGetCurrentTab(ih);
    HWND tab_container = winTabsGetPageWindow(ih, curr_pos);
    if (tab_container)
      ShowWindow(tab_container, SW_HIDE);

    SendMessage(ih->handle, TCM_SETCURSEL, p, 0);

    tab_container = winTabsGetPageWindow(ih, pos);
    if (tab_container)
      ShowWindow(tab_container, SW_SHOW);
  }
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  return winTabsPosFixFromWin(ih, (int)SendMessage(ih->handle, TCM_GETCURSEL, 0, 0));
}

static int winTabsGetImageIndex(Ihandle* ih, const char* name)
{
  HIMAGELIST image_list;
  int count, i, ret;
  int width, height;
  Iarray* bmp_array;
  HBITMAP *bmp_array_data;
  HBITMAP bmp = iupImageGetImage(name, ih, 0, NULL);
  if (!bmp)
    return -1;

  /* the array is used to avoid adding the same bitmap twice */
  bmp_array = (Iarray*)iupAttribGet(ih, "_IUPWIN_BMPARRAY");
  if (!bmp_array)
  {
    /* create the array if does not exist */
    bmp_array = iupArrayCreate(50, sizeof(HBITMAP));
    iupAttribSet(ih, "_IUPWIN_BMPARRAY", (char*)bmp_array);
  }

  bmp_array_data = iupArrayGetData(bmp_array);

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(bmp, &width, &height, NULL);

  image_list = (HIMAGELIST)SendMessage(ih->handle, TCM_GETIMAGELIST, 0, 0);
  if (!image_list)
  {
    /* create the image list if does not exist */
    image_list = ImageList_Create(width, height, ILC_COLOR32, 0, 50);
    SendMessage(ih->handle, TCM_SETIMAGELIST, 0, (LPARAM)image_list);
  }

  /* check if that bitmap is already added to the list,
     but we can not compare with the actual bitmap at the list since it is a copy */
  count = ImageList_GetImageCount(image_list);
  for (i=0; i<count; i++)
  {
    if (bmp_array_data[i] == bmp)
      return i;
  }

  bmp_array_data = iupArrayInc(bmp_array);
  bmp_array_data[i] = bmp;
  ret = ImageList_Add(image_list, bmp, NULL);  /* the bmp is duplicated at the list */
  return ret;
}

static void winTabGetPageWindowRect(Ihandle* ih, RECT *rect)
{
  /* Calculate the display rectangle, assuming the
     tab control is the size of the client area. */
#if 0
  GetClientRect(ih->handle, rect);
  SendMessage(ih->handle, TCM_ADJUSTRECT, FALSE, (LPARAM)rect);
#else
  {
    int x, y, w, h;
    IupGetIntInt(ih, "CLIENTOFFSET", &x, &y);
    IupGetIntInt(ih, "CLIENTSIZE", &w, &h);
    rect->left = x;
    rect->right = x + w;
    rect->top = y;
    rect->bottom = y + h;
  }
#endif
}

static void winTabSetPageWindowPos(HWND tab_container, RECT *rect)
{ 
  SetWindowPos(tab_container, NULL,
                rect->left, rect->top,  
                rect->right - rect->left, rect->bottom - rect->top, 
                SWP_NOACTIVATE|SWP_NOZORDER);
}

static void winTabsPlacePageWindows(Ihandle* ih, RECT* rect)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    HWND tab_container = (HWND)iupAttribGet(child, "_IUPTAB_CONTAINER");
    winTabSetPageWindowPos(tab_container, rect);
  }
}

static int winTabsUsingXPStyles(Ihandle* ih)
{
  return iupwin_comctl32ver6 && ih->data->type == ITABS_TOP && !(ih->data->show_close);
}

static void winTabsDrawPageBackground(Ihandle* ih, HDC hDC, RECT* rect)
{
  unsigned char r=0, g=0, b=0;
  char* color = iupAttribGetInheritNativeParent(ih, "BGCOLOR");
  if (!color) color = iupAttribGetInheritNativeParent(ih, "BACKGROUND");
  if (!color) color = iupAttribGet(ih, "BACKGROUND");
  if (!color) color = IupGetGlobal("DLGBGCOLOR");
  iupStrToRGB(color, &r, &g, &b);
  SetDCBrushColor(hDC, RGB(r,g,b));
  FillRect(hDC, rect, (HBRUSH)GetStockObject(DC_BRUSH));
}

static LRESULT CALLBACK winTabsPageWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  switch (msg)
  {
  case WM_ERASEBKGND:
    {
      Ihandle* ih = iupwinHandleGet(hWnd);
      if (iupObjectCheck(ih)) /* should always be ok */
      {
        RECT rect;
        HDC hDC = (HDC)wp;
        GetClientRect(ih->handle, &rect);
        winTabsDrawPageBackground(ih, hDC, &rect);

        /* return non zero value */
        return 1;
      }
      break;
    }
  case WM_COMMAND:
  case WM_CTLCOLORSCROLLBAR:
  case WM_CTLCOLORBTN:
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORSTATIC:
  case WM_DRAWITEM:
  case WM_HSCROLL:
  case WM_NOTIFY:
  case WM_VSCROLL:
    /* Forward the container messages to its parent. */
    return SendMessage(GetParent(hWnd), msg, wp, lp);
  }

  return DefWindowProc(hWnd, msg, wp, lp);
}

static HWND winTabsCreatePageWindow(Ihandle* ih) 
{ 
  HWND hWnd;
  DWORD dwStyle = WS_CHILD|WS_CLIPSIBLINGS, 
      dwExStyle = 0;

  iupwinGetNativeParentStyle(ih, &dwExStyle, &dwStyle);

  hWnd = iupwinCreateWindowEx(ih->handle, TEXT("IupTabsPage"), dwExStyle, dwStyle, 0, NULL);

  iupwinHandleAdd(ih, hWnd);

  return hWnd;
} 

static void winTabsInsertItem(Ihandle* ih, Ihandle* child, int pos, HWND tab_container)
{
  TCITEM tie;
  char *tabtitle, *tabimage;
  int old_rowcount = 0, old_num_tabs, p;
  RECT rect;

  tabtitle = iupAttribGet(child, "TABTITLE");
  if (!tabtitle) 
  {
    tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
    if (tabtitle)
      iupAttribSetStr(child, "TABTITLE", tabtitle);
  }
  tabimage = iupAttribGet(child, "TABIMAGE");
  if (!tabimage) 
  {
    tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
    if (tabimage)
      iupAttribSetStr(child, "TABIMAGE", tabimage);
  }
  if (!tabtitle && !tabimage)
    tabtitle = "     ";

  old_num_tabs = (int)SendMessage(ih->handle, TCM_GETITEMCOUNT, 0, 0);

  if (ih->data->is_multiline)
    old_rowcount = (int)SendMessage(ih->handle, TCM_GETROWCOUNT, 0, 0);

  tie.mask = TCIF_PARAM;

  if (tabtitle)
  {
    tie.mask |= TCIF_TEXT;
    tie.pszText = iupwinStrToSystem(tabtitle);
    tie.cchTextMax = lstrlen(tie.pszText);

    iupwinSetMnemonicTitle(ih, pos, tabtitle);
  }

  if (tabimage)
  {
    tie.mask |= TCIF_IMAGE;
    tie.iImage = winTabsGetImageIndex(ih, tabimage);
  }

  p = winTabsGetInsertPos(ih, pos);

  /* create tabs and label them */
  tie.lParam = (LPARAM)tab_container;
  SendMessage(ih->handle, TCM_INSERTITEM, p, (LPARAM)&tie);

  winTabGetPageWindowRect(ih, &rect);
  winTabSetPageWindowPos(tab_container, &rect);

  if (ih->data->is_multiline)
  {
    int rowcount = (int)SendMessage(ih->handle, TCM_GETROWCOUNT, 0, 0);
    if (rowcount != old_rowcount)
    {
      winTabsPlacePageWindows(ih, &rect);

      IupRefreshChildren(ih);
    }

    iupdrvRedrawNow(ih);
  }

  /* the first page of an empty tabs must be shown (set as current) */
  if (old_num_tabs == 0)
  {
    ShowWindow(tab_container, SW_SHOW);
    SendMessage(ih->handle, TCM_SETCURSEL, 0, 0);
  }

#if PRINT_VISIBLE_ARRAY
  winTabsPrintVisibleArray(ih);
#endif
}

static void winTabsDeleteItem(Ihandle* ih, int p, HWND tab_container)
{
  int old_rowcount = 0;
  if (ih->data->is_multiline)
    old_rowcount = (int)SendMessage(ih->handle, TCM_GETROWCOUNT, 0, 0);

  /* Make sure tab container is hidden */
  ShowWindow(tab_container, SW_HIDE);

  SendMessage(ih->handle, TCM_DELETEITEM, p, 0);

  if (ih->data->is_multiline)
  {
    int rowcount = (int)SendMessage(ih->handle, TCM_GETROWCOUNT, 0, 0);
    if (rowcount != old_rowcount)
    {
      RECT rect;
      winTabGetPageWindowRect(ih, &rect);
      winTabsPlacePageWindows(ih, &rect);

      IupRefreshChildren(ih);
    }

    iupdrvRedrawNow(ih);
  }

#if PRINT_VISIBLE_ARRAY
  winTabsPrintVisibleArray(ih);
#endif
}

/* ------------------------------------------------------------------------- */
/* winTabs - Sets and Gets Attrib                                           */
/* ------------------------------------------------------------------------- */

static int winTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    int extra_h = (ih->data->show_close)? ITABS_CLOSE_SIZE: 0;
    SendMessage(ih->handle, TCM_SETPADDING, 0, MAKELPARAM(ih->data->horiz_padding + extra_h, ih->data->vert_padding));
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int winTabsSetMultilineAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* allow to set only before mapping */
    return 0;

  if (iupStrBoolean(value))
    ih->data->is_multiline = 1;
  else
  {
    if (ih->data->type == ITABS_BOTTOM || ih->data->type == ITABS_TOP)
      ih->data->is_multiline = 0;
    else
      ih->data->is_multiline = 1;   /* always true if left/right */
  }

  return 0;
}

static char* winTabsGetMultilineAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (ih->data->is_multiline); 
}

static int winTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* allow to set only before mapping */
    return 0;

  if(iupStrEqualNoCase(value, "BOTTOM"))
  {
    ih->data->type = ITABS_BOTTOM;
    ih->data->orientation = ITABS_HORIZONTAL;  /* TABTYPE controls TABORIENTATION in Windows */
  }
  else if(iupStrEqualNoCase(value, "LEFT"))
  {
    ih->data->type = ITABS_LEFT;
    ih->data->orientation = ITABS_VERTICAL;  /* TABTYPE controls TABORIENTATION in Windows */
    ih->data->is_multiline = 1; /* VERTICAL works only with MULTILINE */
  }
  else if(iupStrEqualNoCase(value, "RIGHT"))
  {
    ih->data->type = ITABS_RIGHT;
    ih->data->orientation = ITABS_VERTICAL;  /* TABTYPE controls TABORIENTATION in Windows */
    ih->data->is_multiline = 1; /* VERTICAL works only with MULTILINE */
  }
  else /* "TOP" */
  {
    ih->data->type = ITABS_TOP;
    ih->data->orientation = ITABS_HORIZONTAL;  /* TABTYPE controls TABORIENTATION in Windows */
  }

  return 0;
}

static int winTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
    iupAttribSetStr(child, "TABTITLE", value);

  if (value)
  {
    int p = winTabsPosFixToWin(ih, pos);
    if (p >= 0)
    {
      TCITEM tie;

      tie.mask = TCIF_TEXT;
      tie.pszText = iupwinStrToSystem(value);
      tie.cchTextMax = lstrlen(tie.pszText);

      iupwinSetMnemonicTitle(ih, pos, value);

      SendMessage(ih->handle, TCM_SETITEM, p, (LPARAM)&tie);
    }
  }

  return 0;
}

static int winTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  int p;
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
    iupAttribSetStr(child, "TABIMAGE", value);

  p = winTabsPosFixToWin(ih, pos);
  if (p >= 0)
  {
    TCITEM tie;

    tie.mask = TCIF_IMAGE;
    if (value)
      tie.iImage = winTabsGetImageIndex(ih, value);
    else
      tie.iImage = -1;

    SendMessage(ih->handle, TCM_SETITEM, p, (LPARAM)&tie);
  }

  return 1;
}

static int winTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    int p = winTabsPosFixToWin(ih, pos);
    if (iupStrBoolean(value))
    {
      if (p < 0)  /* is invisible */
      {
        HWND tab_container = (HWND)iupAttribGet(child, "_IUPTAB_CONTAINER");

        winTabsSetVisibleArrayItem(ih, pos, 1);  /* to visible */

        winTabsInsertItem(ih, child, pos, tab_container);
      }
    }
    else
    {
      if (p >= 0)  /* is visible */
      {
        HWND tab_container = (HWND)iupAttribGet(child, "_IUPTAB_CONTAINER");

        iupTabsCheckCurrentTab(ih, pos, 0);
        winTabsSetVisibleArrayItem(ih, pos, 0);  /* to invisible */

        winTabsDeleteItem(ih, p, tab_container);
      }
    }
  }
  return 0;
}

int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  Ihandle* ih = child->parent;
  return winTabsIsTabVisible(ih, pos);
}

static char* winTabsGetBgColorAttrib(Ihandle* ih)
{
  /* the most important use of this is to provide
     the correct background for images */
  if (iupwin_comctl32ver6)
  {
    COLORREF cr;
    if (iupwinDrawGetThemeTabsBgColor(ih->handle, &cr))
      return iupStrReturnStrf("%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
  }

  return IupGetGlobal("DLGBGCOLOR");
}

static int winTabsSetBgColorAttrib(Ihandle *ih, const char *value)
{
  (void)value;
  iupdrvPostRedraw(ih);
  return 1;
}

static int winTabsIsInsideCloseButton(Ihandle* ih, int p)
{
  RECT rect;
  POINT pt;
  int border = 4, start, end;

  GetCursorPos(&pt);
  ScreenToClient(ih->handle, &pt);
  
  /* Get tab rectangle and define start/end positions of the image */
  SendMessage(ih->handle, TCM_GETITEMRECT, (WPARAM)p, (LPARAM)&rect);

  if(ih->data->type == ITABS_BOTTOM || ih->data->type == ITABS_TOP)
  {
    end = rect.right - border;
    start = end - ITABS_CLOSE_SIZE;
    if (pt.x >= start && pt.x <= end)
    {
      start = ((rect.bottom - rect.top) - ITABS_CLOSE_SIZE) / 2 + rect.top;
      end = start + ITABS_CLOSE_SIZE;
      if (pt.y >= start && pt.y <= end)
        return 1;
    }
  }
  else if(ih->data->type == ITABS_LEFT)
  {
    start = rect.top + border;
    end = start + ITABS_CLOSE_SIZE;
    if (pt.y >= start && pt.y <= end)
    {
      start = ((rect.right - rect.left) - ITABS_CLOSE_SIZE) / 2 + rect.left;
      end = start + ITABS_CLOSE_SIZE;
      if (pt.x >= start && pt.x <= end)
        return 1;
    }
  }
  else  /* ITABS_RIGHT */
  {
    end = rect.bottom - border;
    start = end - ITABS_CLOSE_SIZE;
    if (pt.y >= start && pt.y <= end)
    {
      start = ((rect.right - rect.left) - ITABS_CLOSE_SIZE) / 2 + rect.left;
      end = start + ITABS_CLOSE_SIZE;
      if (pt.x >= start && pt.x <= end)
        return 1;
    }
  }

  return 0;
}

static int winTabsCtlColor(Ihandle* ih, HDC hdc, LRESULT *result)
{
  /* works only when NOT winTabsUsingXPStyles */
  COLORREF bgcolor;
  if (iupwinGetParentBgColor(ih, &bgcolor))
  {
    SetDCBrushColor(hdc, bgcolor);
    *result = (LRESULT)GetStockObject(DC_BRUSH);
    return 1;
  }
  return 0;
}

static int winTabsWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  (void)result;

  if (msg_info->code == TCN_SELCHANGING)
  {
    IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
    int prev_pos = iupdrvTabsGetCurrentTab(ih);
    iupAttribSetInt(ih, "_IUPWINTABS_PREV_CHILD_POS", prev_pos);

    /* save the previous handle if callback exists */
    if (cb)
    {
      Ihandle* prev_child = IupGetChild(ih, prev_pos);
      iupAttribSet(ih, "_IUPWINTABS_PREV_CHILD", (char*)prev_child);
    }

    return 0;
  }

  if (msg_info->code == TCN_SELCHANGE)
  {
    IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
    int pos = iupdrvTabsGetCurrentTab(ih);
    int prev_pos = iupAttribGetInt(ih, "_IUPWINTABS_PREV_CHILD_POS");

    HWND tab_container = winTabsGetPageWindow(ih, pos);
    HWND prev_tab_container = winTabsGetPageWindow(ih, prev_pos);

    if (tab_container) ShowWindow(tab_container, SW_SHOW);   /* show new page, if any */
    if (prev_tab_container) ShowWindow(prev_tab_container, SW_HIDE); /* hide previous page, if any */

    if (cb)
    {
      Ihandle* child = IupGetChild(ih, pos);
      Ihandle* prev_child = (Ihandle*)iupAttribGet(ih, "_IUPWINTABS_PREV_CHILD");
      iupAttribSet(ih, "_IUPWINTABS_PREV_CHILD", NULL);

      /* avoid duplicate calls when a Tab is inside another Tab. */
      if (prev_child)
        cb(ih, child, prev_child);
    }
    else
    {
      IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
      if (cb2)
        cb2(ih, pos, prev_pos);
    }

    return 0;
  }

  if (msg_info->code == NM_RCLICK)
  {
    IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cb)
    {
      TCHITTESTINFO ht;
      int p;

      GetCursorPos(&ht.pt);
      ScreenToClient(ih->handle, &ht.pt);
      
      p = (int)SendMessage(ih->handle, TCM_HITTEST, 0, (LPARAM)&ht);
      if (p >= 0)
      {
        int pos = winTabsPosFixFromWin(ih, p);
        cb(ih, pos);
      }
    }

    return 0;
  }

  return 0; /* result not used */
}

static int winTabsMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  switch (msg)
  {
  case WM_SIZE:
  {
    RECT rect;
    WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDWNDPROC_CB");
    CallWindowProc(oldProc, ih->handle, msg, wp, lp);

    winTabGetPageWindowRect(ih, &rect);
    winTabsPlacePageWindows(ih, &rect);

    *result = 0;
    return 1;
  }
  case WM_MOUSELEAVE:
    if (ih->data->show_close)
    {
      int high_p = iupAttribGetInt(ih, "_IUPTABS_CLOSEHIGH");
      if (high_p != -1)
      {
        iupAttribSetInt(ih, "_IUPTABS_CLOSEHIGH", -1);
        iupdrvRedrawNow(ih);
      }
    }
    break;
  case WM_MOUSEMOVE:
    if (ih->data->show_close)
    {
      TCHITTESTINFO ht;
      int p, high_p, press_p;

      ht.pt.x = GET_X_LPARAM(lp);
      ht.pt.y = GET_Y_LPARAM(lp);
      p = (int)SendMessage(ih->handle, TCM_HITTEST, 0, (LPARAM)&ht);

      high_p = iupAttribGetInt(ih, "_IUPTABS_CLOSEHIGH");
      if (winTabsIsInsideCloseButton(ih, p))
      {
        if (high_p != p)
        {
          /* must be called so WM_MOUSELEAVE will be called */
          iupwinTrackMouseLeave(ih);

          iupAttribSetInt(ih, "_IUPTABS_CLOSEHIGH", p);
          iupdrvRedrawNow(ih);
        }
      }
      else
      {
        if (high_p != -1)
        {
          iupAttribSetInt(ih, "_IUPTABS_CLOSEHIGH", -1);
          iupdrvRedrawNow(ih);
        }
      }

      press_p = iupAttribGetInt(ih, "_IUPTABS_CLOSEPRESS");
      if (press_p != -1 && !winTabsIsInsideCloseButton(ih, press_p))
      {
        iupAttribSetInt(ih, "_IUPTABS_CLOSEPRESS", -1);
        iupdrvRedrawNow(ih);
      }
    }
    break;
  case WM_LBUTTONDOWN:
    if (ih->data->show_close)
    {
      TCHITTESTINFO ht;
      int p;

      ht.pt.x = GET_X_LPARAM(lp);
      ht.pt.y = GET_Y_LPARAM(lp);
      p = (int)SendMessage(ih->handle, TCM_HITTEST, 0, (LPARAM)&ht);

      if (p >= 0 && winTabsIsInsideCloseButton(ih, p))
      {
        iupAttribSetInt(ih, "_IUPTABS_CLOSEPRESS", p);  /* used for press feedback */
        iupdrvRedrawNow(ih);

        *result = 0;
        return 1;
      }
      else
        iupAttribSetInt(ih, "_IUPTABS_CLOSEPRESS", -1);
    }
    break;
  case WM_LBUTTONUP:
    if (ih->data->show_close)
    {
      int press_p = iupAttribGetInt(ih, "_IUPTABS_CLOSEPRESS");
      if (press_p != -1)
      {
        if (winTabsIsInsideCloseButton(ih, press_p))
        {
          int pos = winTabsPosFixFromWin(ih, press_p);
          Ihandle *child = IupGetChild(ih, pos);
          HWND tab_container = (HWND)iupAttribGet(child, "_IUPTAB_CONTAINER");

          iupAttribSetInt(ih, "_IUPTABS_CLOSEPRESS", -1);

          if (tab_container)
          {
            int ret = IUP_DEFAULT;
            IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
            if (cb)
              ret = cb(ih, pos);

            if (ret == IUP_CONTINUE) /* destroy tab and children */
            {
              IupDestroy(child);
              IupRefreshChildren(ih);
            }
            else if (ret == IUP_DEFAULT) /* hide tab and children */
            {
              iupTabsCheckCurrentTab(ih, pos, 0);
              winTabsSetVisibleArrayItem(ih, pos, 0);  /* to invisible */
              winTabsDeleteItem(ih, press_p, tab_container);
            }
            else if (ret == IUP_IGNORE)
            {
              *result = 0;
              return 1;
            }
          }
        }

        iupdrvRedrawNow(ih);
      }
    }
    break;
  }

  return iupwinBaseContainerMsgProc(ih, msg, wp, lp, result);
}

static void winTabsDrawRotateText(HDC hDC, char* text, int x, int y, HFONT hFont, COLORREF fgcolor, int align)
{
  COLORREF oldcolor;
  HFONT hOldFont;

  LOGFONT lf;
  GetObject(hFont, sizeof(LOGFONT), &lf);  /* get current font */
  if (align)   /* bottom to top */
  {
    lf.lfEscapement = 900;  /* rotate 90 degrees CCW */
    lf.lfOrientation = 900;
  }
  else         /* top to bottom */
  {
    lf.lfEscapement = -900;  /* rotate 90 degrees CW */
    lf.lfOrientation = -900;
  }
  hFont = CreateFontIndirect(&lf);  /* set font in order to rotate text */

  hOldFont = (HFONT)SelectObject(hDC, hFont);

  SetTextAlign(hDC, align ? TA_TOP | TA_LEFT : TA_BOTTOM | TA_LEFT);
  SetBkMode(hDC, TRANSPARENT);
  oldcolor = SetTextColor(hDC, fgcolor);

  {
    int len = (int)strlen(text);
    TCHAR* str = iupwinStrToSystemLen(text, &len);
    TextOut(hDC, x, y, str, len);
  }

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, oldcolor);
  SetBkMode(hDC, OPAQUE);

  DeleteObject(hFont);
}

static void winTabsDrawTab(Ihandle* ih, HDC hDC, int p, int width, int height, COLORREF fgcolor)
{
  HBITMAP hBitmapClose;
  HFONT hFont = (HFONT)iupwinGetHFontAttrib(ih);
	TCHAR title[256] = TEXT("");
	TCITEM tci;
  HIMAGELIST image_list = (HIMAGELIST)SendMessage(ih->handle, TCM_GETIMAGELIST, 0, 0);
  int imgW = 0, imgH = 0, txtW = 0, txtH = 0, 
    bpp, style = 0, x = 0, y = 0, border = 4;
  char *str = NULL, *value = NULL;
  int high_p, press_p;

  tci.mask = TCIF_TEXT | TCIF_IMAGE;
	tci.pszText = title;     
	tci.cchTextMax = 255;
  SendMessage(ih->handle, TCM_GETITEM, (WPARAM)p, (LPARAM)(&tci));
  
  if (title[0])
  {
    value = iupwinStrFromSystem(title);
    str = iupStrProcessMnemonic(value, NULL, 0);   /* remove & */
    iupdrvFontGetMultiLineStringSize(ih, str, &txtW, &txtH);
  }

  if (tci.iImage != -1)
  {
    IMAGEINFO pImgInfo;
    ImageList_GetImageInfo(image_list, tci.iImage, &pImgInfo);
    imgW = pImgInfo.rcImage.right - pImgInfo.rcImage.left;
    imgH = pImgInfo.rcImage.bottom - pImgInfo.rcImage.top;
  }

  /* Create the close button image */
  high_p = iupAttribGetInt(ih, "_IUPTABS_CLOSEHIGH");
  if (high_p == p)
    hBitmapClose = iupImageGetImage("IMGCLOSEHIGH", ih, 0, NULL);
  else
    hBitmapClose = iupImageGetImage("IMGCLOSE", ih, 0, NULL);
  if (!hBitmapClose)
  {
    if (str && str != value) free(str);
    return;
  }

  iupdrvImageGetInfo(hBitmapClose, NULL, NULL, &bpp);

  /* Draw image tab, title tab and close image */
  if (ih->data->type == ITABS_BOTTOM || ih->data->type == ITABS_TOP)
  {
    x = border;

    if (tci.iImage != -1)
    {
      y = (height - imgH) / 2;
      ImageList_Draw(image_list, tci.iImage, hDC, x, y, ILD_NORMAL);
      x += imgW + border;
    }

    if (title[0])
    {
      y = (height - txtH) / 2;
      iupwinDrawText(hDC, str, x, y, txtW, txtH, hFont, fgcolor, style);
    }

    x = width - border - ITABS_CLOSE_SIZE;
    y = (height - ITABS_CLOSE_SIZE) / 2;
  }
  else if(ih->data->type == ITABS_LEFT)
  {
    y = height;

    if (tci.iImage != -1)
    {
      x = (width - imgW) / 2;
      y -= border + imgH;
      ImageList_Draw(image_list, tci.iImage, hDC, x, y, ILD_NORMAL);  /* Tab image is already rotated into the list! */
    }

    if (title[0])
    {
      x = (width - txtH) / 2;  /* text is rotated switch W/H */
      y -= border;
      winTabsDrawRotateText(hDC, str, x, y, hFont, fgcolor, 1);
    }

    x = (width - ITABS_CLOSE_SIZE) / 2;
    y = border;
  }
  else  /* ITABS_RIGHT */
  {
    y = border;

    if (tci.iImage != -1)
    {
      x = (width - imgW) / 2;
      ImageList_Draw(image_list, tci.iImage, hDC, x, y, ILD_NORMAL);  /* Tab image is already rotated into the list! */
      y += imgH + border;
    }

    if (title[0])
    {
      x = (width - txtH) / 2;  /* text is rotated switch W/H */
      winTabsDrawRotateText(hDC, str, x, y, hFont, fgcolor, 0);
    }

    x = (width - ITABS_CLOSE_SIZE) / 2;
    y = height - border - ITABS_CLOSE_SIZE;
  }

  press_p = iupAttribGetInt(ih, "_IUPTABS_CLOSEPRESS");
  if (press_p == p)
  {
    x++;
    y++;
  }

  iupwinDrawBitmap(hDC, hBitmapClose, x, y, ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, ITABS_CLOSE_SIZE, bpp);

  if (str && str != value) free(str);
}

static void winTabsDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{ 
  HDC hDC;
  iupwinBitmapDC bmpDC;
  RECT rect;
  int x, y, width, height, p;
  COLORREF fgcolor;
  COLORREF bgcolor;

  /* called only when SHOWCLOSE=Yes */

  /* If there are no tab items, skip this message */
  if (drawitem->itemID == -1)
    return;

  p = drawitem->itemID;

  x = drawitem->rcItem.left;
  y = drawitem->rcItem.top;
  width = drawitem->rcItem.right - drawitem->rcItem.left;
  height = drawitem->rcItem.bottom - drawitem->rcItem.top;

  hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, x, y, width, height);

  if (iupwinGetParentBgColor(ih, &bgcolor))
  {
    SetDCBrushColor(hDC, bgcolor);
    SetRect(&rect, 0, 0, width, height);
    FillRect(hDC, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
  }

  if (drawitem->itemState & ODS_DISABLED)
    fgcolor = GetSysColor(COLOR_GRAYTEXT);
  else
  {
    if (!iupwinGetColorRef(ih, "FGCOLOR", &fgcolor))
      fgcolor = GetSysColor(COLOR_WINDOWTEXT);
  }

  winTabsDrawTab(ih, hDC, p, width, height, fgcolor);

  /* If the item has the focus, draw the focus rectangle */
  if (drawitem->itemState & ODS_FOCUS)
    iupwinDrawFocusRect(hDC, 0, 0, width, height);

  iupwinDrawDestroyBitmapDC(&bmpDC);
}

/* ------------------------------------------------------------------------- */
/* winTabs - Methods and Init Class                                          */
/* ------------------------------------------------------------------------- */

static void winTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  /* make sure it has at least one name */
  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    int pos = IupGetChildPos(ih, child);

    HWND tab_container = winTabsCreatePageWindow(ih);

    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)tab_container);

    winTabsInsertVisibleArrayItem(ih, pos);  /* add to the array and set to visible */

    winTabsInsertItem(ih, child, pos, tab_container);
  }
}

static void winTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (ih->handle)
  {
    HWND tab_container = (HWND)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (tab_container)
    {
      int p = winTabsGetPageWindowPos(ih, tab_container);

      iupTabsCheckCurrentTab(ih, pos, 1);

      winTabsDeleteVisibleArrayItem(ih, pos);
      if (p != -1) winTabsDeleteItem(ih, p, tab_container);

      iupwinHandleRemove(tab_container);
      DestroyWindow(tab_container);
      iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
    }
  }
}

static int winTabsMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | TCS_HOTTRACK | WS_TABSTOP,
      dwExStyle = 0;

  if (!ih->parent)
    return IUP_ERROR;

  if (ih->data->type == ITABS_BOTTOM)
    dwStyle |= TCS_BOTTOM;
  else if (ih->data->type == ITABS_RIGHT)
    dwStyle |= TCS_VERTICAL|TCS_RIGHT;  
  else if (ih->data->type == ITABS_LEFT)
    dwStyle |= TCS_VERTICAL;

  if (ih->data->is_multiline)
    dwStyle |= TCS_MULTILINE;

  if (ih->data->show_close)
    dwStyle |= TCS_OWNERDRAWFIXED;

  iupwinGetNativeParentStyle(ih, &dwExStyle, &dwStyle);

  if (dwExStyle & WS_EX_COMPOSITED && !ih->data->is_multiline && iupwinIsVistaOrNew())
  {
    /* workaround for composite bug in Vista */
    ih->data->is_multiline = 1;  
    dwStyle |= TCS_MULTILINE;
  }

  if (!iupwinCreateWindow(ih, WC_TABCONTROL, dwExStyle, dwStyle, NULL))
    return IUP_ERROR;

  /* replace the WinProc to handle other messages */
  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winTabsMsgProc);

  /* Process WM_NOTIFY */
  IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winTabsWmNotify);

  /* Process background color */
  IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winTabsCtlColor);

  if (ih->data->show_close)
  {
    /* change tab width to draw an close image */
    SendMessage(ih->handle, TCM_SETPADDING, 0, MAKELPARAM(ITABS_CLOSE_SIZE, 0));

    iupAttribSetInt(ih, "_IUPTABS_CLOSEHIGH", -1);
    iupAttribSetInt(ih, "_IUPTABS_CLOSEPRESS", -1);

    /* owner draw tab image + tab title + close image */
    IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winTabsDrawItem);
  }

  if (iupwin_comctl32ver6 && (ih->data->type != ITABS_TOP || ih->data->show_close))
  {
    /* XP Styles support only TABTYPE=TOP,
       show_close ownerdraw will work only with classic style */ 
    iupwinDrawRemoveTheme(ih->handle);
  }

  /* Change children background */
  if (winTabsUsingXPStyles(ih))
  {
    char* color = iupAttribGetInheritNativeParent(ih, "BGCOLOR");
    if (!color) 
      color = iupAttribGetInheritNativeParent(ih, "BACKGROUND");
    if (!color)
    {
      COLORREF cr;
      if (iupwinDrawGetThemeTabsBgColor(ih->handle, &cr))
        iupAttribSetStrf(ih, "BACKGROUND", "%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
    }
  }

  /* Create pages and tabs */
  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child = ih->firstchild; child; child = child->brother)
      winTabsChildAddedMethod(ih, child);

    if (current_child)
    {
      IupSetAttribute(ih, "VALUE_HANDLE", (char*)current_child);

      /* current value is now given by the native system */
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }
  }

  return IUP_NOERROR;
}

static void winTabsUnMapMethod(Ihandle* ih)
{
  Iarray* iarray;

  HIMAGELIST image_list = (HIMAGELIST)SendMessage(ih->handle, TCM_GETIMAGELIST, 0, 0);
  if (image_list)
    ImageList_Destroy(image_list);

  iarray = (Iarray*)iupAttribGet(ih, "_IUPWIN_BMPARRAY");
  if (iarray)
  {
    iupAttribSet(ih, "_IUPWIN_BMPARRAY", NULL);
    iupArrayDestroy(iarray);
  }

  iarray = (Iarray*)iupAttribGet(ih, "_IUPWIN_VISIBLEARRAY");
  if (iarray)
  {
    iupAttribSet(ih, "_IUPWIN_VISIBLEARRAY", NULL);
    iupArrayDestroy(iarray);
  }

  iupdrvBaseUnMapMethod(ih);
}

static void winTabsRegisterClass(void)
{
  WNDCLASS wndclass;
  ZeroMemory(&wndclass, sizeof(WNDCLASS));
  
  wndclass.hInstance      = iupwin_hinstance;
  wndclass.lpszClassName  = TEXT("IupTabsPage");
  wndclass.lpfnWndProc    = (WNDPROC)winTabsPageWndProc;
  wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
  wndclass.style          = CS_PARENTDC;
  wndclass.hbrBackground  = NULL;  /* remove the background to optimize redraw */
   
  RegisterClass(&wndclass);
}

static void winTabsRelease(Iclass* ic)
{
  (void)ic;

  if (iupwinClassExist(TEXT("IupTabsPage")))
    UnregisterClass(TEXT("IupTabsPage"), iupwin_hinstance);
}

void iupdrvTabsInitClass(Iclass* ic)
{
  if (!iupwinClassExist(TEXT("IupTabsPage")))
    winTabsRegisterClass();

  /* Driver Dependent Class functions */
  ic->Map = winTabsMapMethod;
  ic->UnMap = winTabsUnMapMethod;
  ic->ChildAdded     = winTabsChildAddedMethod;
  ic->ChildRemoved   = winTabsChildRemovedMethod;
  ic->Release = winTabsRelease;

  /* Driver Dependent Attribute functions */

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", winTabsGetBgColorAttrib, winTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE|IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NOT_MAPPED);

  /* IupTabs only */
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, winTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);  /* can not be set, depends on TABTYPE in Windows */
  iupClassRegisterAttribute(ic, "MULTILINE", winTabsGetMultilineAttrib, winTabsSetMultilineAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, winTabsSetTabTitleAttrib, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, winTabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, winTabsSetTabVisibleAttrib, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, winTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* necessary because transparent background does not work when not using visual styles */
  if (!iupwin_comctl32ver6)  /* Used by iupdrvImageCreateImage */
    iupClassRegisterAttribute(ic, "FLAT_ALPHA", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* Default node images */
  if (!IupGetHandle("IMGCLOSE"))
    winTabsInitializeCloseImage();

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
