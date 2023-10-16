/** \file
 * \brief Button Control
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
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_info.h"
#include "iupwin_str.h"



static int winButtonGetBorder(void)
{
  return 4;
}

void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  /* LAYOUT_DECORATION_ESTIMATE */
  int border_size = winButtonGetBorder() * 2;
  (void)ih;
  (*x) += border_size;
  (*y) += border_size;
}

/****************************************************************/

static int winButtonCalcAlignPosX(int horiz_alignment, int rect_width, int width, int xpad, int shift)
{
  int x;

  if (horiz_alignment == IUP_ALIGN_ARIGHT)
    x = rect_width - (width + 2*xpad);
  else if (horiz_alignment == IUP_ALIGN_ACENTER)
    x = (rect_width - (width + 2*xpad))/2;
  else  /* ALEFT */
    x = 0;

  x += xpad;

  if (shift)
    x++;

  return x;
}

static int winButtonCalcAlignPosY(int vert_alignment, int rect_height, int height, int ypad, int shift)
{
  int y;

  if (vert_alignment == IUP_ALIGN_ABOTTOM)
    y = rect_height - (height + 2*ypad);
  else if (vert_alignment == IUP_ALIGN_ATOP)
    y = 0;
  else  /* ACENTER */
    y = (rect_height - (height + 2*ypad))/2;

  y += ypad;

  if (shift)
    y++;

  return y;
}

static HBITMAP winButtonGetBitmap(Ihandle* ih, UINT itemState, int *shift, int *w, int *h, int *bpp)
{
  char *name;
  int make_inactive = 0;
  HBITMAP hBitmap;

  if (itemState & ODS_DISABLED)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
    {
      name = iupAttribGet(ih, "IMAGE");
      make_inactive = 1;
    }
  }
  else
  {
    name = iupAttribGet(ih, "IMPRESS");
    if (itemState & ODS_SELECTED && name)
    {
      if (shift && !iupAttribGetBoolean(ih, "IMPRESSBORDER")) 
        *shift = 0;
    }
    else
      name = iupAttribGet(ih, "IMAGE");
  }

  hBitmap = iupImageGetImage(name, ih, make_inactive, NULL);

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(hBitmap, w, h, bpp);

  return hBitmap;
}

static void winButtonDrawImageText(Ihandle* ih, HDC hDC, int rect_width, int rect_height, int border, UINT itemState)
{
  int xpad = ih->data->horiz_padding + border, 
      ypad = ih->data->vert_padding + border;
  int x, y, width, height, 
      txt_x, txt_y, txt_width, txt_height, 
      img_x, img_y, img_width, img_height, 
      bpp, shift = 0, style = 0;
  HFONT hFont = (HFONT)iupwinGetHFontAttrib(ih);
  HBITMAP hBitmap;
  COLORREF fgcolor;

  char* title = iupAttribGet(ih, "TITLE");
  char* str = iupStrProcessMnemonic(title, NULL, 0);   /* remove & */
  iupdrvFontGetMultiLineStringSize(ih, str, &txt_width, &txt_height);
  if (str && str!=title) free(str);

  if (itemState & ODS_DISABLED)
    fgcolor = GetSysColor(COLOR_GRAYTEXT);
  else
    fgcolor = ih->data->fgcolor;

  hBitmap = winButtonGetBitmap(ih, itemState, NULL, &img_width, &img_height, &bpp);
  if (!hBitmap)
    return;

  if (ih->data->img_position == IUP_IMGPOS_RIGHT ||
      ih->data->img_position == IUP_IMGPOS_LEFT)
  {
    width  = img_width + txt_width + ih->data->spacing;
    height = iupMAX(img_height, txt_height);
  }
  else
  {
    width = iupMAX(img_width, txt_width);
    height = img_height + txt_height + ih->data->spacing;
  }

  if (itemState & ODS_SELECTED && !iupwin_comctl32ver6)
    shift = 1;

  if (itemState & ODS_NOACCEL && !iupwinGetKeyBoardCues())
    style |= DT_HIDEPREFIX;

  x = winButtonCalcAlignPosX(ih->data->horiz_alignment, rect_width, width, xpad, shift);
  y = winButtonCalcAlignPosY(ih->data->vert_alignment, rect_height, height, ypad, shift);

  switch(ih->data->img_position)
  {
  case IUP_IMGPOS_TOP:
    img_y = y;
    txt_y = y + img_height + ih->data->spacing;
    if (img_width > txt_width)
    {
      img_x = x;
      txt_x = x + (img_width-txt_width)/2;
    }
    else
    {
      img_x = x + (txt_width-img_width)/2;
      txt_x = x;
    }
    break;
  case IUP_IMGPOS_BOTTOM:
    img_y = y + txt_height + ih->data->spacing;
    txt_y = y;
    if (img_width > txt_width)
    {
      img_x = x;
      txt_x = x + (img_width-txt_width)/2;
    }
    else
    {
      img_x = x + (txt_width-img_width)/2;
      txt_x = x;
    }
    break;
  case IUP_IMGPOS_RIGHT:
    img_x = x + txt_width + ih->data->spacing;
    txt_x = x;
    if (img_height > txt_height)
    {
      img_y = y;
      txt_y = y + (img_height-txt_height)/2;
    }
    else
    {
      img_y = y + (txt_height-img_height)/2;
      txt_y = y;
    }
    break;
  default: /* IUP_IMGPOS_LEFT */
    img_x = x;
    txt_x = x + img_width + ih->data->spacing;
    if (img_height > txt_height)
    {
      img_y = y;
      txt_y = y + (img_height-txt_height)/2;
    }
    else
    {
      img_y = y + (txt_height-img_height)/2;
      txt_y = y;
    }
    break;
  }

  if (ih->data->horiz_alignment == IUP_ALIGN_ACENTER)
    style |= DT_CENTER;  /* let DrawText do the internal horizontal alignment, useful for multiple lines */
  else if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
    style |= DT_RIGHT;

  iupwinDrawBitmap(hDC, hBitmap, img_x, img_y, img_width, img_height, img_width, img_height, bpp);
  iupwinDrawText(hDC, title, txt_x, txt_y, txt_width, txt_height, hFont, fgcolor, style);
}

static void winButtonDrawImage(Ihandle* ih, HDC hDC, int rect_width, int rect_height, int border, UINT itemState)
{
  int xpad = ih->data->horiz_padding + border, 
      ypad = ih->data->vert_padding + border;
  int x, y, width, height, bpp, shift = 0;
  HBITMAP hBitmap;

  if (itemState & ODS_SELECTED && !iupwin_comctl32ver6)
    shift = 1;

  hBitmap = winButtonGetBitmap(ih, itemState, &shift, &width, &height, &bpp);
  if (!hBitmap)
    return;

  x = winButtonCalcAlignPosX(ih->data->horiz_alignment, rect_width, width, xpad, shift);
  y = winButtonCalcAlignPosY(ih->data->vert_alignment, rect_height, height, ypad, shift);

  iupwinDrawBitmap(hDC, hBitmap, x, y, width, height, width, height, bpp);
}

static void winButtonDrawText(Ihandle* ih, HDC hDC, int rect_width, int rect_height, int border, UINT itemState)
{
  int xpad = ih->data->horiz_padding + border, 
      ypad = ih->data->vert_padding + border;
  char* title = iupAttribGet(ih, "TITLE");

  if (title)
  {
	int x, y;
	int width, height;
	int shift = 0;
    int style = 0;
    COLORREF fgcolor;
    HFONT hFont = (HFONT)iupwinGetHFontAttrib(ih);
    char* str = iupStrProcessMnemonic(title, NULL, 0);   /* remove & */
    iupdrvFontGetMultiLineStringSize(ih, str, &width, &height);
    if (str && str!=title) free(str);

    if (itemState & ODS_DISABLED)
      fgcolor = GetSysColor(COLOR_GRAYTEXT);
    else
      fgcolor = ih->data->fgcolor;

    if (itemState & ODS_SELECTED && !iupwin_comctl32ver6)
      shift = 1;

    if (itemState & ODS_NOACCEL && !iupwinGetKeyBoardCues())
      style |= DT_HIDEPREFIX;

    x = winButtonCalcAlignPosX(ih->data->horiz_alignment, rect_width, width, xpad, shift);
    y = winButtonCalcAlignPosY(ih->data->vert_alignment, rect_height, height, ypad, shift);

    if (ih->data->horiz_alignment == IUP_ALIGN_ACENTER)
      style |= DT_CENTER;  /* let DrawText do the internal horizontal alignment, useful for multiple lines */
    else if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
      style |= DT_RIGHT;

    iupwinDrawText(hDC, title, x, y, width, height, hFont, fgcolor, style);
  }
  else
  {
    /* fill with the background color if defined at the element */
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    if (bgcolor)
    {
      RECT rect;
      unsigned char r=0, g=0, b=0;
      iupStrToRGB(bgcolor, &r, &g, &b);
      SetDCBrushColor(hDC, RGB(r,g,b));
      SetRect(&rect, xpad, ypad, rect_width - xpad, rect_height - ypad);
      FillRect(hDC, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    }
  }
}

static void winButtonDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{ 
  int has_border = 1;
  HDC hDC;
  iupwinBitmapDC bmpDC;
  int border, draw_border;
  int width = drawitem->rcItem.right - drawitem->rcItem.left;
  int height = drawitem->rcItem.bottom - drawitem->rcItem.top;

  hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, 0, 0, width, height);

  iupwinDrawParentBackground(ih, hDC, &drawitem->rcItem);

  /* force default when in focus but not highlighted */
  if ((drawitem->itemState & ODS_FOCUS) && !(drawitem->itemState & ODS_HOTLIGHT))
    drawitem->itemState |= ODS_DEFAULT;

  /* force selected state */
  if (iupAttribGet(ih, "_IUPWINBUT_SELECTED"))
    drawitem->itemState |= ODS_SELECTED;

  if (iupAttribGet(ih, "_IUPWINBUT_ENTERWIN"))
    drawitem->itemState |= ODS_HOTLIGHT;

  border = winButtonGetBorder();

  if (ih->data->type & IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    has_border = 0;

  if (!has_border)
  {
    draw_border = 0;
  }
  else
  {
    if (iupAttribGetBoolean(ih, "FLAT"))
    {
      if (drawitem->itemState & ODS_HOTLIGHT)
        draw_border = 1;
      else
        draw_border = 0;
    }
    else
      draw_border = 1;
  }

  if (draw_border)
    iupwinDrawButtonBorder(ih->handle, hDC, &drawitem->rcItem, drawitem->itemState);

  if (ih->data->type == IUP_BUTTON_IMAGE)
    winButtonDrawImage(ih, hDC, width, height, border, drawitem->itemState);
  else if (ih->data->type == IUP_BUTTON_TEXT)
    winButtonDrawText(ih, hDC, width, height, border, drawitem->itemState);
  else  /* both */
    winButtonDrawImageText(ih, hDC, width, height, border, drawitem->itemState);

  if (drawitem->itemState & ODS_FOCUS &&
      !(drawitem->itemState & ODS_NOFOCUSRECT) &&
      iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    border--;
    iupwinDrawFocusRect(hDC, border, border, width - 2 * border, height - 2 * border);
  }

  iupwinDrawDestroyBitmapDC(&bmpDC);
}


/***********************************************************************************************/


static int winButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type != IUP_BUTTON_TEXT)
  {
    iupdrvPostRedraw(ih);
    return 1;
  }
  else
    return 0;
}

static int winButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type != IUP_BUTTON_TEXT)
  {
    iupdrvPostRedraw(ih);
    return 1;
  }
  else
    return 0;
}

static int winButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type != IUP_BUTTON_TEXT)
  {
    iupdrvPostRedraw(ih);
    return 1;
  }
  else
    return 0;
}

static int winButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* redraw IMINACTIVE image if any */
  if (ih->data->type != IUP_BUTTON_TEXT)
    iupdrvPostRedraw(ih);

  return iupBaseSetActiveAttrib(ih, value);
}

static int winButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
  else /* "ACENTER" (default) */
    ih->data->horiz_alignment = IUP_ALIGN_ACENTER;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else /* "ACENTER" (default) */
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  if (ih->handle)
    iupdrvRedrawNow(ih);

  return 0;
}

static char* winButtonGetAlignmentAttrib(Ihandle *ih)
{
  char* horiz_align2str[3] = {"ALEFT", "ACENTER", "ARIGHT"};
  char* vert_align2str[3] = {"ATOP", "ACENTER", "ABOTTOM"};
  return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment], vert_align2str[ih->data->vert_alignment]);
}

static int winButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
    iupdrvRedrawNow(ih);
  return 0;
}

static int winButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  iupwinSetMnemonicTitle(ih, 0, value);
  iupwinSetTitleAttrib(ih, value);
  iupdrvPostRedraw(ih);
  return 1;
}

static int winButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  iupdrvPostRedraw(ih);
  return 1;
}

static char* winButtonGetBgColorAttrib(Ihandle* ih)
{
  /* the most important use of this is to provide
     the correct background for images */
  if (iupwin_comctl32ver6 && ih->data->type&IUP_BUTTON_IMAGE && !iupAttribGet(ih, "IMPRESS"))
  {
    COLORREF cr;
    if (iupwinDrawGetThemeButtonBgColor(ih->handle, &cr))
      return iupStrReturnStrf("%d %d %d", (int)GetRValue(cr), (int)GetGValue(cr), (int)GetBValue(cr));
  }

  if (iupAttribGet(ih, "IMPRESS"))
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
    return NULL;
}

static int winButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    ih->data->fgcolor = RGB(r,g,b);

    if (ih->handle)
      iupdrvRedrawNow(ih);
  }
  return 1;
}

/****************************************************************************************/

static int winButtonMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  if (ih->data->type != IUP_BUTTON_TEXT)
  {
    /* redraw IMPRESS image if any */
    if ((msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) && iupAttribGet(ih, "IMPRESS"))
      iupdrvRedrawNow(ih);
  }

  switch (msg)
  {
  case WM_XBUTTONDBLCLK:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_XBUTTONDOWN:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
    {
      iupwinFlagButtonDown(ih, msg);

      /* Process BUTTON_CB */
      (void)iupwinButtonDown(ih, msg, wp, lp); /* ignore return value */

      /* Feedback will NOT be done when not receiving the focus or when in double click */
      if ((msg==WM_LBUTTONDOWN && !iupAttribGetBoolean(ih, "CANFOCUS")) ||
          msg==WM_LBUTTONDBLCLK)
      {
        iupAttribSet(ih, "_IUPWINBUT_SELECTED", "1");
        iupdrvRedrawNow(ih);
      }
      break;
    }
  case WM_XBUTTONUP:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    {
      if (!iupwinFlagButtonUp(ih, msg))
      {
        *result = 0;
        return 1;
      }

      /* Process BUTTON_CB */
      (void)iupwinButtonUp(ih, msg, wp, lp); /* ignore return value */
      
      if (!iupObjectCheck(ih))
      {
        *result = 0;
        return 1;
      }

      if (msg==WM_LBUTTONUP)
      {
        if (iupAttribGet(ih, "_IUPWINBUT_SELECTED"))
        {
          iupAttribSet(ih, "_IUPWINBUT_SELECTED", NULL);
          iupdrvRedrawNow(ih);
        }

        /* BN_CLICKED will NOT be notified when not receiving the focus */
        if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        {
          Icallback cb = IupGetCallback(ih, "ACTION");
          if (cb && cb(ih) == IUP_CLOSE)
            IupExitLoop();
        }
      }

      if (!iupwinIsVistaOrNew() && iupObjectCheck(ih))
      {
        /* TIPs disappear forever after a button click in XP,
           so we force an update. */
        char* tip = iupAttribGet(ih, "TIP");
        if (tip)
          iupdrvBaseSetTipAttrib(ih, tip);
      }
      break;
    }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    if (wp==VK_RETURN)
    {
      /* enter activates the button */
      iupdrvActivate(ih);

      *result = 0;
      return 1;   /* abort default processing, or the default button will be activated, 
                     in this case even if there is a default button, this button must be activated instead. */
    }
    break;
  case WM_MOUSELEAVE:
    iupAttribSet(ih, "_IUPWINBUT_SELECTED", NULL);
    iupAttribSet(ih, "_IUPWINBUT_ENTERWIN", NULL);
    iupdrvRedrawNow(ih);
    break;
  case WM_MOUSEMOVE:
    if ((!iupwin_comctl32ver6 && iupAttribGetBoolean(ih, "FLAT")) ||
        !iupAttribGetBoolean(ih, "CANFOCUS"))
    {
      if (!iupAttribGet(ih, "_IUPWINBUT_ENTERWIN"))
      {
        if (!iupAttribGetBoolean(ih, "CANFOCUS") && LOWORD(wp) & MK_LBUTTON)
          iupAttribSet(ih, "_IUPWINBUT_SELECTED", "1");

        /* this will not affect the process in iupwinBaseMsgProc */

        /* must be called so WM_MOUSELEAVE will be called */
        iupwinTrackMouseLeave(ih);

        iupAttribSet(ih, "_IUPWINBUT_ENTERWIN", "1");
        iupdrvRedrawNow(ih);
      }
    }
    break;
  case WM_SETFOCUS:
    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    {
      HWND previous = (HWND)wp;
      if (previous && previous != ih->handle)
      {
        SetFocus(previous);
        *result = 0;
        return 1;
      }
    }
    break;
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winButtonWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  if (msg_info->code == NM_CUSTOMDRAW)
    return iupwinCustomDrawToDrawItem(ih, msg_info, result, (IFdrawItem)winButtonDrawItem);

  return 0; /* result not used */
}

static int winButtonWmCommand(Ihandle* ih, WPARAM wp, LPARAM lp)
{
  int cmd = HIWORD(wp);
  switch (cmd)
  {
  case BN_DOUBLECLICKED:
  case BN_CLICKED:
    {
      /* BN_CLICKED will NOT be notified when not receiving the focus, but sometimes it does, 
         so we added a test here also */
      if (iupAttribGetBoolean(ih, "CANFOCUS"))
      {
        Icallback cb = IupGetCallback(ih, "ACTION");
        if (cb)
        {
          /* to avoid double calls when pressing enter and a dialog is displayed */
          if (!iupAttribGet(ih, "_IUPBUT_INSIDE_ACTION"))  
          {
            int ret;
            iupAttribSet(ih, "_IUPBUT_INSIDE_ACTION", "1");

            ret = cb(ih);
            if (ret == IUP_CLOSE)
              IupExitLoop();

            if (ret!=IUP_IGNORE && iupObjectCheck(ih))
              iupAttribSet(ih, "_IUPBUT_INSIDE_ACTION", NULL);
          }
        }
      }
    }
  }

  (void)lp;
  return 0; /* not used */
}

static int winButtonMapMethod(Ihandle* ih)
{
  char* value;
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS |
                  BS_NOTIFY; /* necessary because of the base messages */

  if (!ih->parent)
    return IUP_ERROR;

 /* Buttons with the BS_PUSHBUTTON style do NOT use the returned brush in WM_CTLCOLORBTN. 
    Buttons with these styles are always drawn with the default system colors.
      So FGCOLOR and BGCOLOR do NOT work.
    The BS_FLAT style does NOT completely remove the borders. With XP styles is ignored. 
      So FLAT do NOT work.
    BCM_SETTEXTMARGIN is not working either. 
    Buttons with images and with XP styles do NOT draw the focus feedback.
    Can NOT remove the borders when using IMPRESS.
    >>>> So IUP will draw its own button,
    but uses the Windows functions to draw text and images in native format. */
  if (iupwin_comctl32ver6)
    dwStyle |= BS_PUSHBUTTON; /* it will be an ownerdraw because we use NM_CUSTOMDRAW */
  else
    dwStyle |= BS_OWNERDRAW;

  value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    value = iupAttribGet(ih, "TITLE");
    if (value && *value!=0)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (!iupwinCreateWindow(ih, WC_BUTTON, 0, dwStyle, NULL))
    return IUP_ERROR;

  /* Process WM_COMMAND */
  IupSetCallback(ih, "_IUPWIN_COMMAND_CB", (Icallback)winButtonWmCommand);

  /* Process BUTTON_CB and others */
  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winButtonMsgProc);

  if (iupwin_comctl32ver6)
    IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winButtonWmNotify);  /* Process WM_NOTIFY */
  else
    IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winButtonDrawItem);  /* Process WM_DRAWITEM */

  return IUP_NOERROR;
}

void iupdrvButtonInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winButtonMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, winButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", winButtonGetBgColorAttrib, winButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winButtonSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_NOT_MAPPED);  /* force the new default value */  
  iupClassRegisterAttribute(ic, "TITLE", NULL, winButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", winButtonGetAlignmentAttrib, winButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, winButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, winButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, winButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
