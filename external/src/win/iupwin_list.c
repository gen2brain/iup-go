/** \file
 * \brief List Control
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
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_mask.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_list.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"


/* Not defined in Cygwin and MingW */
#ifndef EM_SETCUEBANNER      
#define ECM_FIRST               0x1500      /* Edit control messages */
#define  EM_SETCUEBANNER      (ECM_FIRST + 1)
#endif

#define WM_IUPCARET WM_APP+1   /* Custom IUP message */

#define WIN_GETCOUNT(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_GETCOUNT: LB_GETCOUNT)
#define WIN_GETTEXTLEN(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_GETLBTEXTLEN: LB_GETTEXTLEN)
#define WIN_GETTEXT(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_GETLBTEXT: LB_GETTEXT)
#define WIN_ADDSTRING(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_ADDSTRING: LB_ADDSTRING)
#define WIN_DELETESTRING(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_DELETESTRING: LB_DELETESTRING)
#define WIN_INSERTSTRING(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_INSERTSTRING: LB_INSERTSTRING)
#define WIN_RESETCONTENT(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_RESETCONTENT: LB_RESETCONTENT)
#define WIN_SETCURSEL(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_SETCURSEL: LB_SETCURSEL)
#define WIN_GETCURSEL(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_GETCURSEL: LB_GETCURSEL)
#define WIN_SETHORIZONTALEXTENT(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_SETHORIZONTALEXTENT: LB_SETHORIZONTALEXTENT)
#define WIN_GETHORIZONTALEXTENT(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_GETHORIZONTALEXTENT: LB_GETHORIZONTALEXTENT)
#define WIN_SETITEMDATA(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_SETITEMDATA: LB_SETITEMDATA)
#define WIN_GETITEMDATA(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_GETITEMDATA: LB_GETITEMDATA)
#define WIN_SETTOPINDEX(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_SETTOPINDEX: LB_SETTOPINDEX)
#define WIN_SETITEMHEIGHT(_ih) ((_ih->data->is_dropdown || _ih->data->has_editbox)? CB_SETITEMHEIGHT: LB_SETITEMHEIGHT)


typedef struct _winListItemData
{
  int text_width;
  HBITMAP hBitmap;
} winListItemData;

static void winListUpdateScrollWidthItem(Ihandle* ih, int item_width, int add);
static void winListUpdateShowImageItemHeight(Ihandle* ih, winListItemData* itemdata, int pos);

static winListItemData* winListGetItemData(Ihandle* ih, int pos)
{
  LRESULT ret = (LRESULT)SendMessage(ih->handle, WIN_GETITEMDATA(ih), pos, 0);
  if (ret == CB_ERR)
    return NULL;
  else
    return (winListItemData*)ret;
}

static int winListRemoveItemData(Ihandle* ih, int pos)
{
  winListItemData* itemdata = winListGetItemData(ih, pos);
  if (itemdata)
  {
    int text_width = itemdata->text_width;
    free(itemdata);
    SendMessage(ih->handle, WIN_SETITEMDATA(ih), pos, (LPARAM)NULL);
    return text_width;
  }
  return 0;
}

static void winListSetItemData(Ihandle* ih, int pos, const char* str, HBITMAP hBitmap)
{
  winListItemData* itemdata = winListGetItemData(ih, pos);

  if (!itemdata)
  {
    itemdata = malloc(sizeof(winListItemData));
    SendMessage(ih->handle, WIN_SETITEMDATA(ih), pos, (LPARAM)itemdata);
  }

  if (str)
  {
    itemdata->text_width = iupdrvFontGetStringWidth(ih, str);
    winListUpdateScrollWidthItem(ih, itemdata->text_width, 1);
  }
  else
    itemdata->text_width = 0;

  if (ih->data->show_image)
  {
    itemdata->hBitmap = hBitmap;

    winListUpdateShowImageItemHeight(ih, itemdata, pos);
  }
}

static void winListUpdateShowImageItemHeight(Ihandle* ih, winListItemData* itemdata, int pos)
{
  int txt_h, height;
  iupdrvFontGetCharSize(ih, NULL, &txt_h);

  height = txt_h;

  if (itemdata->hBitmap)
  {
    int img_w, img_h;
    iupdrvImageGetInfo(itemdata->hBitmap, &img_w, &img_h, NULL);

    if (img_h > txt_h)
    {
      height = img_h;

      if (ih->data->is_dropdown && 
          !ih->data->has_editbox && 
          img_h >= ih->data->maximg_h)
      {
        /* set also for the selection box of the dropdown */
        SendMessage(ih->handle, WIN_SETITEMHEIGHT(ih), (WPARAM)-1, img_h);  
      }
    }

    if (img_w > ih->data->maximg_w)
      ih->data->maximg_w = img_w;
    if (img_h > ih->data->maximg_h)
      ih->data->maximg_h = img_h;
  }

  if (!ih->data->is_dropdown)
    height += 2*ih->data->spacing;

  /* According to this documentation, the maximum height is 255 pixels. 
     http://msdn.microsoft.com/en-us/library/ms997541.aspx */
  if (height > 255)
    height = 255;

  SendMessage(ih->handle, WIN_SETITEMHEIGHT(ih), pos, height);
}

void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  (void)ih;
  (void)h;
}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  /* LAYOUT_DECORATION_ESTIMATE */
  int border_size = 2 * 4;
  (*x) += border_size;
  (*y) += border_size;

  if (ih->data->is_dropdown)
  {
    (*x) += 3; /* extra space for the dropdown button */

    /* IMPORTANT: In Windows the DROPDOWN box is always sized by the system
       to have the height just right to include the borders and the text.
       So the user height from RASTERSIZE or SIZE will be always ignored. */
  }
  else
  {
    if (ih->data->has_editbox)
      (*y) += 2*3; /* internal border between editbox and list */
  }
}

int iupdrvListGetCount(Ihandle* ih)
{
  return (int)SendMessage(ih->handle, WIN_GETCOUNT(ih), 0, 0);
}

static int winListConvertXYToPos(Ihandle* ih, int x, int y)
{
  DWORD ret;

  if (ih->data->is_dropdown)
    return -1;

  if (ih->data->has_editbox)
  {
    HWND cblist = (HWND)iupAttribGet(ih, "_IUPWIN_LISTBOX");
    ret = (DWORD)SendMessage(cblist, LB_ITEMFROMPOINT, 0, MAKELPARAM(x, y));
  }
  else
    ret = (DWORD)SendMessage(ih->handle, LB_ITEMFROMPOINT, 0, MAKELPARAM(x, y));

  if (HIWORD(ret))
    return -1;
  else
  {
    int pos = LOWORD(ret);
    return pos+1; /* IUP Starts at 1 */
  }
}

static int winListGetMaxWidth(Ihandle* ih)
{
  int i, item_w, max_w = 0,
    count = (int)SendMessage(ih->handle, WIN_GETCOUNT(ih), 0, 0);

  for (i=0; i<count; i++)
  { 
    winListItemData* itemdata = winListGetItemData(ih, i);
    item_w = itemdata->text_width;
    if (item_w > max_w)
      max_w = item_w;
  }

  return max_w;
}

static void winListUpdateScrollWidth(Ihandle* ih)
{
  if (ih->data->is_dropdown && iupAttribGetBoolean(ih, "DROPEXPAND"))
  {
    int w = 3+winListGetMaxWidth(ih)+iupdrvGetScrollbarSize()+3;
    SendMessage(ih->handle, CB_SETDROPPEDWIDTH, w, 0);
  }
  else
    SendMessage(ih->handle, WIN_SETHORIZONTALEXTENT(ih), winListGetMaxWidth(ih), 0);
}

static void winListUpdateScrollWidthItem(Ihandle* ih, int item_width, int add)
{
  int max_w;

  if (ih->data->is_dropdown && iupAttribGetBoolean(ih, "DROPEXPAND"))
  {
    max_w = (int)SendMessage(ih->handle, CB_GETDROPPEDWIDTH, 0, 0);
    max_w = max_w -3 -3 -iupdrvGetScrollbarSize();
  }
  else
    max_w = (int)SendMessage(ih->handle, WIN_GETHORIZONTALEXTENT(ih), 0, 0);

  if (add)
  {
    if (item_width > max_w)
    {
      if (ih->data->is_dropdown && iupAttribGetBoolean(ih, "DROPEXPAND"))
      {
        int w = 3+item_width+iupdrvGetScrollbarSize()+3;
        SendMessage(ih->handle, CB_SETDROPPEDWIDTH, w, 0);
      }
      else
        SendMessage(ih->handle, WIN_SETHORIZONTALEXTENT(ih), item_width, 0);
    }
  }
  else
  {
    if (item_width >= max_w)
      winListUpdateScrollWidth(ih);
  }
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  int pos = (int)SendMessage(ih->handle, WIN_ADDSTRING(ih), 0, (LPARAM)iupwinStrToSystem(value));
  winListSetItemData(ih, pos, value, NULL);
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  SendMessage(ih->handle, WIN_INSERTSTRING(ih), pos, (LPARAM)iupwinStrToSystem(value));
  winListSetItemData(ih, pos, value, NULL);
  iupListUpdateOldValue(ih, pos, 0);
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  int text_width;

  if (ih->data->is_dropdown && !ih->data->has_editbox)
  {
    /* must check if removing the current item */
    int curpos = (int)SendMessage(ih->handle, WIN_GETCURSEL(ih), 0, 0);
    if (pos == curpos)
    {
      if (curpos > 0) 
        curpos--;
      else 
      {
        curpos=1;
        if (iupdrvListGetCount(ih)==1)
          curpos = -1; /* remove the selection */
      }

      SendMessage(ih->handle, WIN_SETCURSEL(ih), curpos, 0);
    }
  }

  text_width = winListRemoveItemData(ih, pos);
  SendMessage(ih->handle, WIN_DELETESTRING(ih), pos, 0L);
  winListUpdateScrollWidthItem(ih, text_width, 0);

  iupListUpdateOldValue(ih, pos, 1);
}

static void winListRemoveAllItemData(Ihandle* ih)
{
  int pos, count = iupdrvListGetCount(ih);
  for (pos = 0; pos < count; pos++)
    winListRemoveItemData(ih, pos);
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  winListRemoveAllItemData(ih);
  SendMessage(ih->handle, WIN_RESETCONTENT(ih), 0, 0L);
  if (ih->data->is_dropdown && iupAttribGetBoolean(ih, "DROPEXPAND"))
    SendMessage(ih->handle, CB_SETDROPPEDWIDTH, 0, 0);
  else
    SendMessage(ih->handle, WIN_SETHORIZONTALEXTENT(ih), 0, 0);
}

static int winListGetCaretPos(HWND cbedit)
{
  int pos = 0;
  POINT point;

  if (GetFocus() != cbedit || !GetCaretPos(&point))
  {
    /* if does not have the focus, or could not get caret position,
       then use the selection start position */
    SendMessage(cbedit, EM_GETSEL, (WPARAM)&pos, 0);
  }
  else
  {
    pos = (int)SendMessage(cbedit, EM_CHARFROMPOS, 0, MAKELPARAM(point.x, point.y));
    pos = LOWORD(pos);
  }

  return pos;
}

static char* winListGetText(Ihandle* ih, int pos)
{
  int len = (int)SendMessage(ih->handle, WIN_GETTEXTLEN(ih), (WPARAM)pos, 0);
  TCHAR* str = (TCHAR*)iupStrGetMemory((len+1)*sizeof(TCHAR));
  SendMessage(ih->handle, WIN_GETTEXT(ih), (WPARAM)pos, (LPARAM)str);
  return iupwinStrFromSystem(str);
}


/*********************************************************************************/


static void winListUpdateItemWidth(Ihandle* ih)
{
  int i, count = (int)SendMessage(ih->handle, WIN_GETCOUNT(ih), 0, 0);
  for (i=0; i<count; i++)
  { 
    winListItemData* itemdata = winListGetItemData(ih, i);
    char* str = winListGetText(ih, i);
    itemdata->text_width = iupdrvFontGetStringWidth(ih, str);
  }
}

static int winListSetBgColorAttrib(Ihandle *ih, const char *value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}

static int winListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    winListUpdateItemWidth(ih);
    winListUpdateScrollWidth(ih);
  }
  return 1;
}

static char* winListGetIdValueAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos >= 0)
    return iupStrReturnStr(winListGetText(ih, pos));
  return NULL;
}

static char* winListGetValueAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    if (iupAttribGet(ih, "_IUPWIN_GETFROMLIST"))
    {
      int pos = (int)SendMessage(ih->handle, WIN_GETCURSEL(ih), 0, 0);
      return iupStrReturnStr(winListGetText(ih, pos));
    }
    else
    {
      TCHAR* value = iupwinGetWindowText(ih->handle);
      if (value)
        return iupStrReturnStr(iupwinStrFromSystem(value));
      else
        return "";
    }
  }
  else 
  {
    if (ih->data->is_dropdown || !ih->data->is_multiple)
    {
      int pos = (int)SendMessage(ih->handle, WIN_GETCURSEL(ih), 0, 0);
      return iupStrReturnInt(pos+1);  /* IUP starts at 1 */
    }
    else
    {
      int i, count = (int)SendMessage(ih->handle, LB_GETCOUNT, 0, 0);
      int* pos = malloc(sizeof(int)*count);
      int sel_count = (int)SendMessage(ih->handle, LB_GETSELITEMS, count, (LPARAM)pos);
      char* str = iupStrGetMemory(count+1);
      memset(str, '-', count);
      str[count]=0;
      for (i=0; i<sel_count; i++)
        str[pos[i]] = '+';
      str[count]=0;
      return str;
    }
  }
}

static int winListSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_editbox)
    iupwinSetTitleAttrib(ih, value);
  else 
  {
    if (ih->data->is_dropdown || !ih->data->is_multiple)
    {
      int pos;
      if (iupStrToInt(value, &pos)==1)
      {
        SendMessage(ih->handle, WIN_SETCURSEL(ih), pos-1, 0);   /* IUP starts at 1 */
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
      }
      else
      {
        SendMessage(ih->handle, WIN_SETCURSEL(ih), (WPARAM)-1, 0);   /* no selection */
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
      }
    }
    else
    {
      /* User has changed a multiple selection on a simple list. */
      int i, len, count;

      /* Clear all selections */
      SendMessage(ih->handle, LB_SETSEL, FALSE, -1);

      if (!value)
      {
        iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        return 0;
      }

      count = (int)SendMessage(ih->handle, LB_GETCOUNT, 0, 0L);
      len = (int)strlen(value);
      if (len < count) 
        count = len;

      /* update selection list */
      for (i = 0; i<count; i++)
      {
        if (value[i]=='+')
          SendMessage(ih->handle, LB_SETSEL, TRUE, i);
      }

      iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
    }
  }

  return 0;
}

static int winListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    SendMessage(ih->handle, CB_SHOWDROPDOWN, iupStrBoolean(value), 0); 
  return 0;
}

static int winListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
      SendMessage(ih->handle, WIN_SETTOPINDEX(ih), pos-1, 0);  /* IUP starts at 1 */
  }
  return 0;
}

static int winListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    return 0;

  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;

  if (ih->handle)
  {
    /* set for all items */
    if (!ih->data->show_image)
    {
      /* since this is done once and affects all items, there is no need to do this 
         every time a new item is added */
      int txt_h;
      iupdrvFontGetCharSize(ih, NULL, &txt_h);
      txt_h += 2*ih->data->spacing;

      SendMessage(ih->handle, WIN_SETITEMHEIGHT(ih), 0, txt_h);
    }
    else
    {
      /* must manually set for each item */
      int i, count = (int)SendMessage(ih->handle, WIN_GETCOUNT(ih), 0, 0);

      for (i=0; i<count; i++)
      { 
        winListItemData* itemdata = winListGetItemData(ih, i);
        winListUpdateShowImageItemHeight(ih, itemdata, i);
      }
    }
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int winListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  iupStrToIntInt(value, &(ih->data->horiz_padding), &(ih->data->vert_padding), 'x');
  ih->data->vert_padding = 0;

  if (ih->handle)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELPARAM(ih->data->horiz_padding, ih->data->horiz_padding));
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int winListSetFilterAttrib(Ihandle *ih, const char *value)
{
  int style = 0;

  if (!ih->data->has_editbox)
    return 0;

  if (iupStrEqualNoCase(value, "LOWERCASE"))
    style = ES_LOWERCASE;
  else if (iupStrEqualNoCase(value, "NUMBER"))
    style = ES_NUMBER;
  else if (iupStrEqualNoCase(value, "UPPERCASE"))
    style = ES_UPPERCASE;

  if (style)
  {
    HWND old_handle = ih->handle;
    ih->handle = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    iupwinMergeStyle(ih, ES_LOWERCASE|ES_NUMBER|ES_UPPERCASE, style);
    ih->handle = old_handle;
  }

  return 1;
}

static int winListSetCueBannerAttrib(Ihandle *ih, const char *value)
{
  if (ih->data->has_editbox && iupwin_comctl32ver6)
  {
    WCHAR* wstr = iupwinStrChar2Wide(value);
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_SETCUEBANNER, (WPARAM)FALSE, (LPARAM)wstr);  /* always an Unicode string here */
    free(wstr);
    return 1;
  }
  return 0;
}

static int winListSetClipboardAttrib(Ihandle *ih, const char *value)
{
  UINT msg = 0;

  if (!ih->data->has_editbox)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
    msg = WM_COPY;
  else if (iupStrEqualNoCase(value, "CUT"))
    msg = WM_CUT;
  else if (iupStrEqualNoCase(value, "PASTE"))
    msg = WM_PASTE;
  else if (iupStrEqualNoCase(value, "CLEAR"))
    msg = WM_CLEAR;
  else if (iupStrEqualNoCase(value, "UNDO"))
    msg = WM_UNDO;

  if (msg)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, msg, 0, 0);
  }

  return 0;
}

static int winListHasSelection(HWND cbedit)
{
  int start = 0, end = 0;
  SendMessage(cbedit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  if (start == end)
    return 0;
  return 1;
}

static int winListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (value)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    if (!winListHasSelection(cbedit))
      return 0;
    SendMessage(cbedit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)iupwinStrToSystem(value));
  }
  return 0;
}

static char* winListGetSelectedTextAttrib(Ihandle* ih)
{
  TCHAR* str;
  HWND cbedit;

  if (!ih->data->has_editbox)
    return 0;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  str = iupwinGetWindowText(cbedit);
  if (str)
  {
    int start = 0, end = 0;
    
    SendMessage(cbedit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    if (start == end)
      return NULL;

    str[end] = 0; /* returns only the selected text */
    str += start;

    return iupStrReturnStr(iupwinStrFromSystem(str));
  }
  else
    return NULL;
}

static int winListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = 0;

  if (ih->handle)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_LIMITTEXT, ih->data->nc, 0L);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int winListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  HWND cbedit;
  int start=1, end=1;
  if (!ih->data->has_editbox)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<1 || end<1) 
    return 0;

  start--; /* IUP starts at 1 */
  end--;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_SETSEL, (WPARAM)start, (LPARAM)end);

  return 0;
}

static char* winListGetSelectionAttrib(Ihandle* ih)
{
  int start = 0, end = 0;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return NULL;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  if (start == end)
    return NULL;

  start++; /* IUP starts at 1 */
  end++;
  return iupStrReturnIntInt(start, end, ':');
}

static int winListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_SETSEL, (WPARAM)start, (LPARAM)end);

  return 0;
}

static char* winListGetSelectionPosAttrib(Ihandle* ih)
{
  int start = 0, end = 0;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return NULL;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  if (start == end)
    return NULL;

  return iupStrReturnIntInt(start, end, ':');
}

static int winListSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (value && ih->data->has_editbox)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    SendMessage(cbedit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)iupwinStrToSystem(value));
  }
  return 0;
}

static int winListSetAppendAttrib(Ihandle* ih, const char* value)
{
  int len;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value) value = "";
  
  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  len = GetWindowTextLength(cbedit)+1;
  SendMessage(cbedit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
  SendMessage(cbedit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)iupwinStrToSystem(value));

  return 0;
}

static int winListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_SETREADONLY, (WPARAM)iupStrBoolean(value), 0);
  return 0;
}

static char* winListGetReadOnlyAttrib(Ihandle* ih)
{
  DWORD style;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return NULL;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  style = GetWindowLong(cbedit, GWL_STYLE);
  return iupStrReturnBoolean (style & ES_READONLY); 
}

static int winListSetCaretAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;
  pos--; /* IUP starts at 1 */

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_SETSEL, (WPARAM)pos, (LPARAM)pos);
  SendMessage(cbedit, EM_SCROLLCARET, 0L, 0L);

  return 0;
}

static char* winListGetCaretAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    return iupStrReturnInt(winListGetCaretPos(cbedit)+1);
  }
  else
    return NULL;
}

static int winListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);    /* be permissive in SetCaret, do not abort if invalid */
  if (pos < 0) pos = 0;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_SETSEL, (WPARAM)pos, (LPARAM)pos);
  SendMessage(cbedit, EM_SCROLLCARET, 0L, 0L);

  return 0;
}

static char* winListGetCaretPosAttrib(Ihandle* ih)
{
  if (ih->data->has_editbox)
  {
    HWND cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
    return iupStrReturnInt(winListGetCaretPos(cbedit));
  }
  else
    return NULL;
}

static int winListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int pos = 1;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;

  pos--;  /* return to Windows reference */

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_LINESCROLL, (WPARAM)pos, (LPARAM)0);

  return 0;
}

static int winListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;
  HWND cbedit;
  if (!ih->data->has_editbox)
    return 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  cbedit = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  SendMessage(cbedit, EM_LINESCROLL, (WPARAM)pos, (LPARAM)0);

  return 0;
}

static int winListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  HBITMAP hBitmap;
  int pos = iupListGetPosAttrib(ih, id);

  if (!ih->data->show_image || pos < 0)
    return 0;

  hBitmap = iupImageGetImage(value, ih, 0, NULL);
  winListSetItemData(ih, pos, NULL, hBitmap);

  iupdrvRedrawNow(ih);
  return 0;
}

static char* winListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  winListItemData *itemdata = winListGetItemData(ih, id - 1);
  if (itemdata)
    return (char*)itemdata->hBitmap;
  else
    return NULL;
}

void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  winListItemData *itemdata = winListGetItemData(ih, id-1);
  return itemdata->hBitmap;
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  winListSetItemData(ih, id, NULL, (HBITMAP)hImage);
  iupdrvRedrawNow(ih);
  return 0;
}

/*********************************************************************************/

static void winListDrawRect(HWND hWnd, HDC hDC, int nIndex)
{
  RECT rect;

  if (nIndex == -2)
    return;

  if (nIndex == -1)
  {
    int h;
    nIndex = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0)-1;
    SendMessage(hWnd, LB_GETITEMRECT, (WPARAM)nIndex, (LPARAM)&rect);

    h = rect.bottom-rect.top;
    rect.top += h;
    rect.bottom += h/2;
  }
  else
    SendMessage(hWnd, LB_GETITEMRECT, (WPARAM)nIndex, (LPARAM)&rect);

	PatBlt(hDC, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, PATINVERT);
}

static void winListDrawDropFeedback(Ihandle *ih, int nIndex)
{
  int nLastIndex = iupAttribGetInt(ih, "_IUPLIST_LASTITEM");
  if (nLastIndex != nIndex)
  {
    RECT rect;
    HRGN rgn;
    HDC hDC;
    HWND hWnd = ih->handle;

    GetClientRect(hWnd, &rect);
    rgn = CreateRectRgnIndirect(&rect);

    hDC = GetDC(hWnd);

    /* Prevent drawing outside of listbox.
       This can happen at the top of the listbox since 
       the listbox's DC is the parent's DC */
    SelectClipRgn(hDC, rgn);

    winListDrawRect(hWnd, hDC, nLastIndex);
    winListDrawRect(hWnd, hDC, nIndex);
    iupAttribSetInt(ih, "_IUPLIST_LASTITEM", nIndex);

    ReleaseDC(hWnd, hDC);
  }
}

static int winInClient(HWND hWnd, POINT pt)
{
  RECT rect;
  GetWindowRect(hWnd, &rect);
  return PtInRect(&rect, pt);
}

int iupwinListDND(Ihandle *ih, UINT uNotification, POINT pt)
{
  switch(uNotification)
  {
  case DL_BEGINDRAG:
    {
      int idDrag = LBItemFromPt(ih->handle, pt, TRUE);  /* starts at 0 */
      if (idDrag != -1)
      {
        iupAttribSetInt(ih, "_IUPLIST_DRAGITEM", idDrag);
        iupAttribSetInt(ih, "_IUPLIST_LASTITEM", -2);
        return TRUE;
      }
      iupAttribSet(ih, "_IUPLIST_DRAGITEM", NULL);
      break;
    }
  case DL_DRAGGING:
    {
      int idDrop = LBItemFromPt(ih->handle, pt, TRUE);  /* starts at 0 */
      if (idDrop!=-1 || winInClient(ih->handle, pt))
        winListDrawDropFeedback(ih, idDrop);

      if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        return DL_COPYCURSOR;
      else
        return DL_MOVECURSOR;
    }
  case DL_DROPPED:
    {
      int is_ctrl;
      int idDrop = LBItemFromPt(ih->handle, pt, TRUE);   /* starts at 0 */
      int idDrag = iupAttribGetInt(ih, "_IUPLIST_DRAGITEM");

      winListDrawDropFeedback(ih, -2);

      if (iupListCallDragDropCb(ih, idDrag, idDrop, &is_ctrl) == IUP_CONTINUE)  /* starts at 0 */
      {
        winListItemData *itemdata = winListGetItemData(ih, idDrag);  /* starts at 0 */
        HBITMAP hBitmap = itemdata->hBitmap;
        char* text = winListGetText(ih, idDrag);  /* starts at 0 */
        int count = iupdrvListGetCount(ih);

        /* Copy the item to the idDrop position */
        if (idDrop >= 0 && idDrop < count)  /* starts at 0 */
        {
          iupdrvListInsertItem(ih, idDrop, text);   /* starts at 0, insert before */
          if (idDrag > idDrop)
            idDrag++;
        }
        else  /* idDrop = -1 */
        {
          iupdrvListAppendItem(ih, text);
          idDrop = count;  /* new item is the previous count */
        }

        if (hBitmap)
          winListSetItemData(ih, idDrop, NULL, hBitmap);  /* starts at 0 */

        /* Selects the drop item */
        SendMessage(ih->handle, WIN_SETCURSEL(ih), idDrop, 0);  /* starts at 0 */
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", idDrop+1);  /* starts at 1 */

        /* Remove the Drag item if moving */
        if (!is_ctrl)
          iupdrvListRemoveItem(ih, idDrag);  /* starts at 0 */
      }

      iupdrvRedrawNow(ih);  /* Redraw the list */
      iupAttribSet(ih, "_IUPLIST_DRAGITEM", NULL);
      break;
    }
  case DL_CANCELDRAG:
    {
      winListDrawDropFeedback(ih, -2);
      iupAttribSet(ih, "_IUPLIST_DRAGITEM", NULL);
      break;
    }
  }

  return 0;
}

static void winListEnableDragDrop(Ihandle* ih)
{
  MakeDragList(ih->handle);
}


/*********************************************************************************/


static int winListCtlColor(Ihandle* ih, HDC hdc, LRESULT *result)
{
  COLORREF cr;

  if (iupwinGetColorRef(ih, "FGCOLOR", &cr))
    SetTextColor(hdc, cr);

  if (iupwinGetColorRef(ih, "BGCOLOR", &cr))
  {
    SetBkColor(hdc, cr);
    SetDCBrushColor(hdc, cr);
    *result = (LRESULT)GetStockObject(DC_BRUSH);
    return 1;
  }
  return 0;
}

static int winListWmCommand(Ihandle* ih, WPARAM wp, LPARAM lp)
{
  (void)lp;

  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    switch (HIWORD(wp))
    {
    case CBN_EDITCHANGE:
      {
        /* called only when the edit was changed by the user */
        iupBaseCallValueChangedCb(ih);
        break;
      }
    case CBN_SETFOCUS:
      iupwinWmSetFocus(ih);
      break;
    case CBN_KILLFOCUS:
      iupCallKillFocusCb(ih);
      break;
    case CBN_CLOSEUP:
    case CBN_DROPDOWN:
      {
        IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
        if (cb)
          cb(ih, HIWORD(wp) == CBN_DROPDOWN ? 1 : 0);
        break;
      }
    case CBN_DBLCLK:
      {
        IFnis cb = (IFnis) IupGetCallback(ih, "DBLCLICK_CB");
        if (cb)
        {
          int pos = (int)SendMessage(ih->handle, CB_GETCURSEL, 0, 0);
          pos++;  /* IUP starts at 1 */
          iupListSingleCallDblClickCb(ih, cb, pos);
        }
        break;
      }
    case CBN_SELCHANGE:
      {
        IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
        if (cb)
        {
          int pos = (int)SendMessage(ih->handle, CB_GETCURSEL, 0, 0);
          pos++;  /* IUP starts at 1 */
          iupListSingleCallActionCb(ih, cb, pos);
        }

        /* the edit is not updated yet, so prepare to return correct VALUE during callback */
        if (ih->data->has_editbox) 
          iupAttribSet(ih, "_IUPWIN_GETFROMLIST", "1");

        iupBaseCallValueChangedCb(ih);

        if (ih->data->has_editbox)
          iupAttribSet(ih, "_IUPWIN_GETFROMLIST", NULL);
        break;
      }
    }
  }
  else
  {
    switch (HIWORD(wp))
    {
    case LBN_DBLCLK:
      {
        IFnis cb = (IFnis) IupGetCallback(ih, "DBLCLICK_CB");
        if (cb)
        {
          int pos = (int)SendMessage(ih->handle, LB_GETCURSEL, 0, 0);
          pos++;  /* IUP starts at 1 */
          iupListSingleCallDblClickCb(ih, cb, pos);
        }
        break;
      }
    case LBN_SELCHANGE:
      {
        if (!ih->data->is_multiple)
        {
          IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
          if (cb)
          {
            int pos = (int)SendMessage(ih->handle, LB_GETCURSEL, 0, 0);
            pos++;  /* IUP starts at 1 */
            iupListSingleCallActionCb(ih, cb, pos);
          }
        }
        else
        {
          IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
          IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
          if (multi_cb || cb)
          {
            int sel_count = (int)SendMessage(ih->handle, LB_GETSELCOUNT, 0, 0);
            int* pos = malloc(sizeof(int)*sel_count);
            SendMessage(ih->handle, LB_GETSELITEMS, sel_count, (LPARAM)pos);
            iupListMultipleCallActionCb(ih, cb, multi_cb, pos, sel_count);
            free(pos);
          }
        }

        iupBaseCallValueChangedCb(ih);
        break;
      }
    }
  }

  return 0; /* not used */
}

static void winListCallCaretCb(Ihandle* ih, HWND cbedit)
{
  int pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = winListGetCaretPos(cbedit);

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, 1, pos+1, pos);
  }
}

static int winListCallEditCb(Ihandle* ih, HWND cbedit, char* insert_value, int remove_dir)
{
  int ret, start, end;
  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  SendMessage(cbedit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  ret = iupEditCallActionCb(ih, cb, insert_value, start, end, (char*)ih->data->mask, ih->data->nc, remove_dir, iupwinStrGetUTF8Mode());
  if (ret == 0)
    return 0;
  if (ret != -1)
  {
    WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_EDITOLDWNDPROC_CB");
    CallWindowProc(oldProc, cbedit, WM_CHAR, ret, 0);  /* replace key */
    return 0;
  }
  return 1;
}

static int winListEditProc(Ihandle* ih, HWND cbedit, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  int ret = 0;

  if (ih->data->is_dropdown)
  {
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
      {
        int wincode = (int)wp;
        if (wincode == VK_ESCAPE || wincode == VK_RETURN)
        {
          int dropped = (int)SendMessage(ih->handle, CB_GETDROPPEDSTATE, 0, 0);
          if (dropped)
          {
            if (wincode == VK_RETURN) SendMessage(ih->handle, CB_SHOWDROPDOWN, FALSE, 0);
            return 0;  /* do not call base procedure to allow internal key processing */
          }
        }
      }
    }
  }

  if (msg==WM_KEYDOWN) /* process K_ANY before text callbacks */
  {
    ret = iupwinBaseMsgProc(ih, msg, wp, lp, result);
    if (!iupObjectCheck(ih))
    {
      *result = 0;
      return 1;
    }

    if (ret) 
    {
      iupAttribSet(ih, "_IUPWIN_IGNORE_CHAR", "1");
      *result = 0;
      return 1;
    }
    else
      iupAttribSet(ih, "_IUPWIN_IGNORE_CHAR", NULL);
  }

  switch (msg)
  {
  case WM_CHAR:
    {
      TCHAR c = (TCHAR)wp;

      if (iupAttribGet(ih, "_IUPWIN_IGNORE_CHAR"))
      {
        iupAttribSet(ih, "_IUPWIN_IGNORE_CHAR", NULL);
        *result = 0;
        return 1;
      }

      if (c == TEXT('\b'))
      {              
        if (!winListCallEditCb(ih, cbedit, NULL, -1))
          ret = 1;
      }
      else if (c == TEXT('\n') || c == TEXT('\r'))
      {
        ret = 1;
      }
      else
      {
        int has_ctrl = GetKeyState(VK_CONTROL) & 0x8000;
        int has_alt = GetKeyState(VK_MENU) & 0x8000;
        int has_sys = (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000);

        if ((has_ctrl && has_alt && !has_sys) ||
            (!has_ctrl && !has_alt && !has_sys))
        {
          TCHAR insert_value[2];
          insert_value[0] = c;
          insert_value[1] = 0;

          if (!winListCallEditCb(ih, cbedit, iupwinStrFromSystem(insert_value), 0))
            ret = 1;
        }
      }

      PostMessage(cbedit, WM_IUPCARET, 0, 0L);

      if (wp==VK_TAB)  /* the keys have the same definitions as the chars */
        ret = 1;  /* abort default processing to avoid beep */

      break;
    }
  case WM_KEYDOWN:
    {
      if (wp == VK_DELETE) /* Del does not generates a WM_CHAR */
      {
        if (!winListCallEditCb(ih, cbedit, NULL, 1))
          ret = 1;
      }
      else if (wp == TEXT('A') && GetKeyState(VK_CONTROL) & 0x8000)   /* Ctrl+A = Select All */
      {
        SendMessage(cbedit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
      }

      PostMessage(cbedit, WM_IUPCARET, 0, 0L);

      break;
    }
  case WM_CLEAR:
  case WM_CUT:
    {
      if (!winListCallEditCb(ih, cbedit, NULL, 1))
        ret = 1;

      PostMessage(cbedit, WM_IUPCARET, 0, 0L);

      break;
    }
  case WM_PASTE:
    {
      if (IupGetCallback(ih, "EDIT_CB") || ih->data->mask) /* test before to avoid alocate clipboard text memory */
      {
        Ihandle* clipboard = IupClipboard();
        char* insert_value = IupGetAttribute(clipboard, "TEXT");
        IupDestroy(clipboard);
        if (insert_value)
        {
          if (!winListCallEditCb(ih, cbedit, insert_value, 0))
            ret = 1;
        }
      }

      PostMessage(cbedit, WM_IUPCARET, 0, 0L);

      break;
    }
  case WM_UNDO:
    {
      IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
      if (cb)
      {
        char* value;
        WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_EDITOLDWNDPROC_CB");
        CallWindowProc(oldProc, cbedit, WM_UNDO, 0, 0);

        value = winListGetValueAttrib(ih);
        cb(ih, 0, (char*)value);

        ret = 1;
      }

      PostMessage(cbedit, WM_IUPCARET, 0, 0L);

      break;
    }
  case WM_KEYUP:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_LBUTTONUP:
    PostMessage(cbedit, WM_IUPCARET, 0, 0L);
    break;
  case WM_IUPCARET:
    winListCallCaretCb(ih, cbedit);
    break;
  }

  if (ret)       /* if abort processing, then the result is 0 */
  {
    *result = 0;
    return 1;
  }
  else
  {
    if (msg==WM_KEYDOWN)
      return 0;
    else
      return iupwinBaseMsgProc(ih, msg, wp, lp, result);
  }
}

static LRESULT CALLBACK winListEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  int ret = 0;
  LRESULT result = 0;
  WNDPROC oldProc;
  Ihandle *ih;

  ih = iupwinHandleGet(hwnd); 
  if (!iupObjectCheck(ih))
    return DefWindowProc(hwnd, msg, wp, lp);  /* should never happen */

  /* retrieve the control previous procedure for subclassing */
  oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_EDITOLDWNDPROC_CB");

  ret = winListEditProc(ih, hwnd, msg, wp, lp, &result);

  if (ret)
    return result;
  else
    return CallWindowProc(oldProc, hwnd, msg, wp, lp);
}

static int winListComboListProc(Ihandle* ih, HWND cblist, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  (void)cblist;

  switch (msg)
  {
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
    iupwinFlagButtonDown(ih, msg);

    if (iupwinButtonDown(ih, msg, wp, lp)==-1)
    {
      *result = 0;
      return 1;
    }
    break;
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_LBUTTONUP:
    if (!iupwinFlagButtonUp(ih, msg))
    {
      *result = 0;
      return 1;
    }

    if (iupwinButtonUp(ih, msg, wp, lp)==-1)
    {
      *result = 0;
      return 1;
    }
    break;
  case WM_MOUSEMOVE:
    iupwinMouseMove(ih, msg, wp, lp);
    iupwinBaseMsgProc(ih, msg, wp, lp, result); /* to process ENTER/LEAVE */
    break;
  case WM_MOUSELEAVE:
    iupwinBaseMsgProc(ih, msg, wp, lp, result); /* to process ENTER/LEAVE */
    break;
  }

  return 0;
}

static LRESULT CALLBACK winListComboListWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{   
  int ret = 0;
  LRESULT result = 0;
  WNDPROC oldProc;
  Ihandle *ih;

  ih = iupwinHandleGet(hwnd); 
  if (!iupObjectCheck(ih))
    return DefWindowProc(hwnd, msg, wp, lp);  /* should never happen */

  /* retrieve the control previous procedure for subclassing */
  oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_LISTOLDWNDPROC_CB");

  ret = winListComboListProc(ih, hwnd, msg, wp, lp, &result);

  if (ret)
    return result;
  else
    return CallWindowProc(oldProc, hwnd, msg, wp, lp);
}

static int winListMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  if (ih->data->is_dropdown)
  {
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
      {
        int wincode = (int)wp;
        if (wincode == VK_ESCAPE || wincode == VK_RETURN)
        {
          int dropped = (int)SendMessage(ih->handle, CB_GETDROPPEDSTATE, 0, 0);
          if (dropped)
          {
            return 0;  /* do not call base procedure to allow internal key processing */
          }
        }
      }
    }
  }

  if (!ih->data->is_dropdown && !ih->data->has_editbox)
  {
    switch (msg)
    {
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      if (iupwinButtonDown(ih, msg, wp, lp)==-1)
      {
        *result = 0;
        return 1;
      }
      break;
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
      if (iupwinButtonUp(ih, msg, wp, lp)==-1)
      {
        *result = 0;
        return 1;
      }
      break;
    case WM_MOUSEMOVE:
      iupwinMouseMove(ih, msg, wp, lp);
      break;
    }
  }

  switch (msg)
  {
  case WM_CHAR:
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
      /* avoid item search when Ctrl is pressed */
      *result = 0;
      return 1;
    }
    break;
  case WM_SETFOCUS:
  case WM_KILLFOCUS:
  case WM_MOUSELEAVE:
  case WM_MOUSEMOVE:
    if (ih->data->has_editbox)
      return 0;  /* do not call base procedure to avoid duplicate messages */
    break;
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static void winListDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{
  char* text;
  int txt_w, txt_h;
  winListItemData* itemdata;
  HFONT hFont = (HFONT)iupwinGetHFontAttrib(ih);
  iupwinBitmapDC bmpDC;
  HDC hDC;
  RECT rect;
  COLORREF fgcolor, bgcolor;

  /* called only when SHOWIMAGE=Yes */

  int x = drawitem->rcItem.left;
  int y = drawitem->rcItem.top;
  int width = drawitem->rcItem.right - drawitem->rcItem.left;
  int height = drawitem->rcItem.bottom - drawitem->rcItem.top;

  /* If there are no list box items, skip this message */
  if (drawitem->itemID == -1)
    return;

  hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, x, y, width, height);

  if (drawitem->itemState & ODS_SELECTED)
    bgcolor = GetSysColor(COLOR_HIGHLIGHT);
  else if (!iupwinGetColorRef(ih, "BGCOLOR", &bgcolor))
    bgcolor = GetSysColor(COLOR_WINDOW);
  SetDCBrushColor(hDC, bgcolor);
  SetRect(&rect, 0, 0, width, height);
  FillRect(hDC, &rect, (HBRUSH)GetStockObject(DC_BRUSH));

  if (iupdrvIsActive(ih))
  {
    if (drawitem->itemState & ODS_SELECTED)
      fgcolor = GetSysColor(COLOR_HIGHLIGHTTEXT);
    else if (!iupwinGetColorRef(ih, "FGCOLOR", &fgcolor))
      fgcolor = GetSysColor(COLOR_WINDOWTEXT);
  }
  else
    fgcolor = GetSysColor(COLOR_GRAYTEXT);

  /* Get the bitmap associated with the item */
  itemdata = winListGetItemData(ih, drawitem->itemID);

  /* Get and draw the string associated with the item */
  text = winListGetText(ih, drawitem->itemID);
  iupdrvFontGetMultiLineStringSize(ih, text, &txt_w, &txt_h);

  x = ih->data->maximg_w + 3; /* align text + spacing between text and image */
  y = (height - txt_h)/2;  /* vertically centered */
  iupwinDrawText(hDC, text, x, y, txt_w, txt_h, hFont, fgcolor, 0);

  /* Draw the bitmap associated with the item */
  if (itemdata->hBitmap)
  {
    int bpp, img_w, img_h;

    iupdrvImageGetInfo(itemdata->hBitmap, &img_w, &img_h, &bpp);

    x = 0;
    y = (height - img_h)/2;  /* vertically centered */
    iupwinDrawBitmap(hDC, itemdata->hBitmap, x, y, img_w, img_h, img_w, img_h, bpp);
  }

  /* If the item has the focus, draw the focus rectangle */
  if (drawitem->itemState & ODS_FOCUS)
    iupwinDrawFocusRect(hDC, 0, 0, width, height);

  iupwinDrawDestroyBitmapDC(&bmpDC);
}

static void winListLayoutUpdateMethod(Ihandle *ih)
{
  if (ih->data->is_dropdown)
  {
    /* Must add the dropdown area, or it will not be visible */
    RECT rect;
    int charheight, calc_h, win_h, win_w, voptions;

    if (iupAttribGet(ih, "VISIBLEITEMS"))
      voptions = iupAttribGetInt(ih, "VISIBLEITEMS");
    else
      voptions = iupAttribGetInt(ih, "VISIBLE_ITEMS");
    if (voptions <= 0)
      voptions = 1;

    iupdrvFontGetCharSize(ih, NULL, &charheight);
    calc_h = ih->currentheight + voptions*charheight;

    SendMessage(ih->handle, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rect);
    win_h = rect.bottom-rect.top;
    win_w = rect.right-rect.left;

    if (ih->currentwidth != win_w || calc_h != win_h)
      SetWindowPos(ih->handle, HWND_TOP, ih->x, ih->y, ih->currentwidth, calc_h, 
                   SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
    else
      SetWindowPos(ih->handle, HWND_TOP, ih->x, ih->y, 0, 0, 
                   SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
  }                
  else
    iupdrvBaseLayoutUpdateMethod(ih);
}

static void winListUnMapMethod(Ihandle* ih)
{
  HWND handle;

  winListRemoveAllItemData(ih);

  handle = (HWND)iupAttribGet(ih, "_IUPWIN_LISTBOX");
  if (handle)
  {
    iupwinHandleRemove(handle);
    iupAttribSet(ih, "_IUPWIN_LISTBOX", NULL);
  }

  handle = (HWND)iupAttribGet(ih, "_IUPWIN_EDITBOX");
  if (handle)
  {
    iupwinHandleRemove(handle);
    iupAttribSet(ih, "_IUPWIN_EDITBOX", NULL);
  }

  iupdrvBaseUnMapMethod(ih);
}

static int winListMapMethod(Ihandle* ih)
{
  TCHAR* class_name;
  DWORD dwStyle = WS_CHILD|WS_CLIPSIBLINGS,
      dwExStyle = WS_EX_CLIENTEDGE;

  if (!ih->parent)
    return IUP_ERROR;

  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    class_name = TEXT("COMBOBOX");

    dwStyle |= CBS_NOINTEGRALHEIGHT;

    if (ih->data->show_image)
      dwStyle |= CBS_OWNERDRAWVARIABLE|CBS_HASSTRINGS;

    if (ih->data->is_dropdown)
      dwStyle |= WS_VSCROLL|WS_HSCROLL;
    else if (ih->data->sb)
    {
      dwStyle |= WS_VSCROLL|WS_HSCROLL;

      if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
        dwStyle |= CBS_DISABLENOSCROLL;
    }

    if (ih->data->has_editbox)
    {
      dwStyle |= CBS_AUTOHSCROLL;

      if (ih->data->is_dropdown)
        dwStyle |= CBS_DROPDOWN;  /* hidden-list+edit */
      else
        dwStyle |= CBS_SIMPLE;  /* visible-list+edit */
    }
    else
      dwStyle |= CBS_DROPDOWNLIST;  /* hidden-list */

    if (iupAttribGetBoolean(ih, "SORT"))
      dwStyle |= CBS_SORT;
  }
  else
  {
    class_name = TEXT("LISTBOX");

    dwStyle |= LBS_NOINTEGRALHEIGHT|LBS_NOTIFY;

    if (ih->data->is_multiple)
      dwStyle |= LBS_EXTENDEDSEL;

    if (ih->data->show_image)
      dwStyle |= LBS_OWNERDRAWVARIABLE|LBS_HASSTRINGS;

    if (ih->data->sb)
    {
      dwStyle |= WS_VSCROLL|WS_HSCROLL;

      if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
        dwStyle |= LBS_DISABLENOSCROLL;
    }

    if (iupAttribGetBoolean(ih, "SORT"))
      dwStyle |= LBS_SORT;
  }

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (!iupwinCreateWindow(ih, class_name, dwExStyle, dwStyle, NULL))
    return IUP_ERROR;

  /* Custom Procedure */
  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winListMsgProc);

  /* Process background color */
  IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winListCtlColor);

  /* Process WM_COMMAND */
  IupSetCallback(ih, "_IUPWIN_COMMAND_CB", (Icallback)winListWmCommand);

  if (ih->data->is_dropdown || ih->data->has_editbox)
  {
    COMBOBOXINFO boxinfo;

    ZeroMemory(&boxinfo, sizeof(COMBOBOXINFO));
    boxinfo.cbSize = sizeof(COMBOBOXINFO);

    GetComboBoxInfo(ih->handle, &boxinfo);

    iupwinHandleAdd(ih, boxinfo.hwndList);
    iupAttribSet(ih, "_IUPWIN_LISTBOX", (char*)boxinfo.hwndList);

    /* subclass the list box. */
    IupSetCallback(ih, "_IUPWIN_LISTOLDWNDPROC_CB", (Icallback)GetWindowLongPtr(boxinfo.hwndList, GWLP_WNDPROC));
    SetWindowLongPtr(boxinfo.hwndList, GWLP_WNDPROC, (LONG_PTR)winListComboListWndProc);

    if (ih->data->has_editbox)
    {
      iupwinHandleAdd(ih, boxinfo.hwndItem);
      iupAttribSet(ih, "_IUPWIN_EDITBOX", (char*)boxinfo.hwndItem);

      /* subclass the edit box. */
      IupSetCallback(ih, "_IUPWIN_EDITOLDWNDPROC_CB", (Icallback)GetWindowLongPtr(boxinfo.hwndItem, GWLP_WNDPROC));
      SetWindowLongPtr(boxinfo.hwndItem, GWLP_WNDPROC, (LONG_PTR)winListEditWndProc);

      /* set defaults */
      SendMessage(ih->handle, CB_LIMITTEXT, 0, 0L);
    }
  }

  /* Enable internal drag and drop support */
  if(ih->data->show_dragdrop && !ih->data->is_dropdown && !ih->data->is_multiple)
    winListEnableDragDrop(ih);

  if(ih->data->show_image)
    IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winListDrawItem);  /* Process WM_DRAWITEM */

  /* configure for DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)winListConvertXYToPos);

  iupListSetInitialItems(ih);

  return IUP_NOERROR;
}

void iupdrvListInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winListMapMethod;
  ic->LayoutUpdate = winListLayoutUpdateMethod;
  ic->UnMap = winListUnMapMethod;

  /* Driver Dependent Attribute functions */

  iupClassRegisterAttribute(ic, "FONT", NULL, winListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_NOT_MAPPED);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, iupwinSetAutoRedrawAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupList only */
  iupClassRegisterAttributeId(ic, "IDVALUE", winListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", winListGetValueAttrib, winListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, winListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, winListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
  /*OLD*/iupClassRegisterAttribute(ic, "VISIBLE_ITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, winListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, winListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", winListGetSelectedTextAttrib, winListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", winListGetSelectionAttrib, winListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", winListGetSelectionPosAttrib, winListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", winListGetCaretAttrib, winListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", winListGetCaretPosAttrib, winListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, winListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, winListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", winListGetReadOnlyAttrib, winListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, winListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, winListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, winListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, winListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, winListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", winListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, winListSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, winListSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
