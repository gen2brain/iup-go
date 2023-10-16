/** \file
 * \brief Label Control
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
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"


/* Not defined in MingW and Cygwin */
#ifndef ODS_NOACCEL
#define ODS_NOACCEL   0x0100
#endif
#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX   0x00100000
#endif

void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

static void winLabelDrawImage(Ihandle* ih, HDC hDC, int rect_width, int rect_height)
{
  int xpad = ih->data->horiz_padding, 
      ypad = ih->data->vert_padding;
  int x, y, width, height, bpp;
  HBITMAP hBitmap;
  char *name;
  int make_inactive = 0;

  if (iupdrvIsActive(ih))
    name = iupAttribGet(ih, "IMAGE");
  else
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
    {
      name = iupAttribGet(ih, "IMAGE");
      make_inactive = 1;
    }
  }

  hBitmap = iupImageGetImage(name, ih, make_inactive, NULL);
  if (!hBitmap)
    return;

  /* must use this info, since image can be a driver image loaded from resources */
  iupdrvImageGetInfo(hBitmap, &width, &height, &bpp);

  if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
    x = rect_width - (width + 2*xpad);
  else if (ih->data->horiz_alignment == IUP_ALIGN_ACENTER)
    x = (rect_width - (width + 2*xpad))/2;
  else  /* ALEFT */
    x = 0;

  if (ih->data->vert_alignment == IUP_ALIGN_ABOTTOM)
    y = rect_height - (height + 2*ypad);
  else if (ih->data->vert_alignment == IUP_ALIGN_ATOP)
    y = 0;
  else  /* ACENTER */
    y = (rect_height - (height + 2*ypad))/2;

  x += xpad;
  y += ypad;

  iupwinDrawBitmap(hDC, hBitmap, x, y, width, height, width, height, bpp);
}

static void winLabelDrawText(Ihandle* ih, HDC hDC, int rect_width, int rect_height, UINT itemState)
{
  int xpad = ih->data->horiz_padding, 
      ypad = ih->data->vert_padding;
  int x, y, width, height, style;
  HFONT hFont = (HFONT)iupwinGetHFontAttrib(ih);
  COLORREF fgcolor;

  char* title = iupAttribGet(ih, "TITLE");
  char* str = iupStrProcessMnemonic(title, NULL, 0);   /* remove & */
  iupdrvFontGetMultiLineStringSize(ih, str, &width, &height);
  if (str && str!=title) free(str);

  if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
    style = DT_RIGHT;
  else if (ih->data->horiz_alignment == IUP_ALIGN_ACENTER)
    style = DT_CENTER;
  else  /* ALEFT */
    style = DT_LEFT;

  if (ih->data->vert_alignment == IUP_ALIGN_ABOTTOM)
    y = rect_height - (height + 2*ypad);
  else if (ih->data->vert_alignment == IUP_ALIGN_ATOP)
    y = 0;
  else  /* ACENTER */
    y = (rect_height - (height + 2*ypad))/2;

  /* let DrawText do the horizontal alignment */
  x = xpad;
  width = rect_width - 2*xpad;
  y += ypad;

  if (iupdrvIsActive(ih))
    fgcolor = ih->data->fgcolor;
  else
    fgcolor = GetSysColor(COLOR_GRAYTEXT);

  /* WORDWRAP and ELLIPSIS */
  style |= ih->data->text_style;

  if (itemState & ODS_NOACCEL && !iupwinGetKeyBoardCues())
    style |= DT_HIDEPREFIX;

  iupwinDrawText(hDC, title, x, y, width, height, hFont, fgcolor, style);
}

static void winLabelDrawItem(Ihandle* ih, DRAWITEMSTRUCT *drawitem)
{ 
  HDC hDC;
  iupwinBitmapDC bmpDC;
  int width, height;

  if (!(drawitem->itemAction & ODA_DRAWENTIRE))
    return;

  width = drawitem->rcItem.right - drawitem->rcItem.left;
  height = drawitem->rcItem.bottom - drawitem->rcItem.top;

  hDC = iupwinDrawCreateBitmapDC(&bmpDC, drawitem->hDC, 0, 0, width, height);

  iupwinDrawParentBackground(ih, hDC, &drawitem->rcItem);

  if (ih->data->type == IUP_LABEL_IMAGE)
    winLabelDrawImage(ih, hDC, width, height);
  else
    winLabelDrawText(ih, hDC, width, height, drawitem->itemState);

  iupwinDrawDestroyBitmapDC(&bmpDC);
}

/************************************************************************************************/

static int winLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  iupwinSetMnemonicTitle(ih, 0, value);
  iupwinSetTitleAttrib(ih, value);
  iupdrvPostRedraw(ih);
  return 1;
}

static int winLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    char value1[30], value2[30];

    iupStrToStrStr(value, value1, value2, ':');

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
    else /* "ALEFT" (default) */
      ih->data->horiz_alignment = IUP_ALIGN_ALEFT;

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
    else if (iupStrEqualNoCase(value2, "ATOP"))
      ih->data->vert_alignment = IUP_ALIGN_ATOP;
    else /* "ACENTER" (default) */
      ih->data->vert_alignment = IUP_ALIGN_ACENTER;

    iupdrvRedrawNow(ih);
  }
  return 0;
}

static char* winLabelGetAlignmentAttrib(Ihandle *ih)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    char* horiz_align2str[3] = {"ALEFT", "ACENTER", "ARIGHT"};
    char* vert_align2str[3] = {"ATOP", "ACENTER", "ABOTTOM"};
    return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment], vert_align2str[ih->data->vert_alignment]);
  }
  else
    return NULL;
}

static int winLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
    iupdrvRedrawNow(ih);

  return 0;
}

static int winLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    if (iupStrBoolean(value))
      ih->data->text_style |= DT_WORDBREAK;
    else
      ih->data->text_style &= ~DT_WORDBREAK;

    iupdrvRedrawNow(ih);
  }

  return 1;
}

static int winLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    if (iupStrBoolean(value))
      ih->data->text_style |= DT_END_ELLIPSIS;
    else
      ih->data->text_style &= ~DT_END_ELLIPSIS;

    iupdrvRedrawNow(ih);
  }

  return 1;
}

static int winLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  /* this method can be called before map */
  int type = iupLabelGetTypeBeforeMap(ih);
  if (type != IUP_LABEL_SEP_HORIZ && type != IUP_LABEL_SEP_VERT)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      ih->data->fgcolor = RGB(r,g,b);

      if (ih->handle)
        iupdrvRedrawNow(ih);
    }
  }
  return 1;
}

static int winLabelPostRedrawSetAttrib(Ihandle* ih, const char* value)
{
  (void)value;

  if (ih->handle)
    iupdrvPostRedraw(ih);  /* Post a redraw */

  return 1;
}

static int winLabelMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
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
      if (IupGetCallback(ih, "BUTTON_CB"))
        SetCapture(ih->handle);

      (void)iupwinButtonDown(ih, msg, wp, lp); /* ignore return value */
      break;
    }
  case WM_XBUTTONUP:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    {
      if (IupGetCallback(ih, "BUTTON_CB") && GetCapture() == ih->handle)
        ReleaseCapture();

      (void)iupwinButtonUp(ih, msg, wp, lp); /* ignore return value */
      break;
    }
  case WM_MOUSEMOVE:
  {
    if (iupwinMouseMove(ih, msg, wp, lp))
    {
      /* refresh the cursor, it could have been changed in MOTION_CB */
      iupwinRefreshCursor(ih);
    }

    break; /* let iupwinBaseMsgProc process enter/leavewin */
  }
  case WM_NCCALCSIZE:
    {
      if (wp == TRUE)
      {
        *result = WVR_HREDRAW|WVR_VREDRAW;
        return 1;
      }
      break;
    }
  case WM_NCHITTEST:
    {
      if (iupAttribGetBoolean(ih, "HTTRANSPARENT"))
      {
        *result = HTTRANSPARENT;
        return 1;
      }

      break;
    }
  }

  return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static int winLabelMapMethod(Ihandle* ih)
{
  char* value;
  DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS |
                  SS_NOTIFY; /* SS_NOTIFY is necessary because of the base messages */

  if (!ih->parent)
    return IUP_ERROR;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;
      dwStyle |= SS_ETCHEDHORZ;
    }
    else /* "VERTICAL" */
    {
      ih->data->type = IUP_LABEL_SEP_VERT;
      dwStyle |= SS_ETCHEDVERT;
    }
  }
  else
  {
    /* The lack for good alignment support in STATIC control forces IUP to draw its own label,
       but uses the Windows functions to draw text and images in native format. */
    dwStyle |= SS_OWNERDRAW;

    value = iupAttribGet(ih, "IMAGE");
    if (value)
      ih->data->type = IUP_LABEL_IMAGE;
    else
      ih->data->type = IUP_LABEL_TEXT;
  }

  if (!iupwinCreateWindow(ih, WC_STATIC, 0, dwStyle, NULL))
    return IUP_ERROR;

  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    /* replace the WinProc to handle other messages */
    IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winLabelMsgProc);

    IupSetCallback(ih, "_IUPWIN_DRAWITEM_CB", (Icallback)winLabelDrawItem);
  }

  /* configure for DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

void iupdrvLabelInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winLabelMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  /* the most important use of this is to provide the correct background for images */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);  

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winLabelSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_NOT_MAPPED);    /* force new default value */
  iupClassRegisterAttribute(ic, "TITLE", NULL, winLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupLabel only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", winLabelGetAlignmentAttrib, winLabelSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winLabelPostRedrawSetAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, winLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* IupLabel Windows and GTK only */
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, winLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, winLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);

  /* IupLabel Windows only */
  iupClassRegisterAttribute(ic, "HTTRANSPARENT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
