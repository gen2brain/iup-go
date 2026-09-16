/** \file
 * \brief Separator Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_drvinfo.h"


enum { ISEPARATOR_VERT, ISEPARATOR_HORIZ };
enum { ISEPARATOR_FILL, ISEPARATOR_LINE, ISEPARATOR_SUNKENLINE, ISEPARATOR_DUALLINES, ISEPARATOR_GRIP, ISEPARATOR_EMPTY };

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  int orientation,
      barsize,
      barsize_hw,    /* barsize scaled to HW pixels; cached at set-time. */
      style,
      hover;
};

/****************************************************************/

static long iDrawGetDarkerColor(long color)
{
  unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
  r = (r * 80) / 100;
  g = (g * 80) / 100;
  b = (b * 80) / 100;
  return iupDrawColor(r, g, b, a);
}

static long iSeparatorDefaultColor(void)
{
  if (IupGetInt(NULL, "DARKMODE"))
    return iupDrawColor(90, 90, 90, 255);
  return iupDrawColor(160, 160, 160, 255);
}

static int iSeparatorRedraw_CB(Ihandle* ih)
{
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);

  iupDrawParentBackground(dc, ih);

  if (ih->data->style != ISEPARATOR_FILL && ih->data->style != ISEPARATOR_EMPTY)
  {
    int x, y, w, h;
    long color = iupDrawStrToColor(iupAttribGet(ih, "COLOR"), iSeparatorDefaultColor());

    iupdrvDrawGetSize(dc, &w, &h);

    if (ih->data->style == ISEPARATOR_GRIP)
    {
      int len, thick = iupdrvScaleNaturalPx(2);

      if (!ih->data->hover)
      {
        iupdrvDrawFlush(dc);
        iupdrvDrawKillCanvas(dc);
        return IUP_DEFAULT;
      }

      if (ih->data->orientation == ISEPARATOR_VERT)
      {
        len = h / 8;
        if (len > iupdrvScaleNaturalPx(28))
          len = iupdrvScaleNaturalPx(28);

        if (len >= thick)
        {
          x = (w - thick) / 2;
          y = (h - len) / 2;
          iupdrvDrawRectangle(dc, x, y, x + thick - 1, y + len - 1, color, IUP_DRAW_FILL, 1);
        }
      }
      else
      {
        len = w / 8;
        if (len > iupdrvScaleNaturalPx(28))
          len = iupdrvScaleNaturalPx(28);

        if (len >= thick)
        {
          x = (w - len) / 2;
          y = (h - thick) / 2;
          iupdrvDrawRectangle(dc, x, y, x + len - 1, y + thick - 1, color, IUP_DRAW_FILL, 1);
        }
      }
    }
    else if (ih->data->style == ISEPARATOR_DUALLINES)
    {
      if (ih->data->orientation == ISEPARATOR_VERT)
      {
        x = w / 2;

        iupdrvDrawLine(dc, x - 1, 0, x - 1, h - 1, color, IUP_DRAW_STROKE, 1);
        iupdrvDrawLine(dc, x + 1, 0, x + 1, h - 1, color, IUP_DRAW_STROKE, 1);
      }
      else
      {
        y = h / 2;

        iupdrvDrawLine(dc, 0, y - 1, w - 1, y - 1, color, IUP_DRAW_STROKE, 1);
        iupdrvDrawLine(dc, 0, y + 1, w - 1, y + 1, color, IUP_DRAW_STROKE, 1);
      }
    }
    else if (ih->data->style == ISEPARATOR_SUNKENLINE)
    {
      /* the groove highlight is a slight lift off the background, not a fixed white */
      long bgcolor = iupDrawStrToColor(iupBaseNativeParentGetBgColorAttrib(ih), 0);
      long sunken_color = iupDrawColor((unsigned char)(iupDrawRed(bgcolor) + ((255 - iupDrawRed(bgcolor)) * 20) / 100),
                                       (unsigned char)(iupDrawGreen(bgcolor) + ((255 - iupDrawGreen(bgcolor)) * 20) / 100),
                                       (unsigned char)(iupDrawBlue(bgcolor) + ((255 - iupDrawBlue(bgcolor)) * 20) / 100),
                                       255);

      if (ih->data->orientation == ISEPARATOR_VERT)
      {
        x = w / 2;

        iupdrvDrawLine(dc, x, 0, x, h - 1, color, IUP_DRAW_STROKE, 1);
        iupdrvDrawLine(dc, x + 1, 0, x + 1, h - 1, sunken_color, IUP_DRAW_STROKE, 1);
      }
      else
      {
        y = h / 2;

        iupdrvDrawLine(dc, 0, y, w - 1, y, color, IUP_DRAW_STROKE, 1);
        iupdrvDrawLine(dc, 0, y + 1, w - 1, y + 1, sunken_color, IUP_DRAW_STROKE, 1);
      }
    }
    else /* ISEPARATOR_LINE */
    {
      if (ih->data->orientation == ISEPARATOR_VERT)
      {
        x = w / 2;

        iupdrvDrawLine(dc, x, 0, x, h - 1, color, IUP_DRAW_STROKE, 1);
      }
      else
      {
        y = h / 2;

        iupdrvDrawLine(dc, 0, y, w - 1, y, color, IUP_DRAW_STROKE, 1);
      }
    }
  }
  else if (ih->data->style == ISEPARATOR_FILL)
  {
    int w, h;
    long color = iupDrawStrToColor(iupAttribGet(ih, "COLOR"), iSeparatorDefaultColor());
    long border_color = iDrawGetDarkerColor(color);

    iupdrvDrawGetSize(dc, &w, &h);

    iupdrvDrawRectangle(dc, 1, 1, w - 2, h - 2, color, IUP_DRAW_FILL, 1);
    iupdrvDrawRectangle(dc, 0, 0, w - 1, h - 1, border_color, IUP_DRAW_STROKE, 1);
  }

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

/***********************************************************************************************/

static int iSeparatorSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "HORIZONTAL"))
  {
    ih->data->orientation = ISEPARATOR_HORIZ;
    ih->expand = IUP_EXPAND_WFREE;
  }
  else  /* Default = VERTICAL */
  {
    ih->data->orientation = ISEPARATOR_VERT;
    ih->expand = IUP_EXPAND_HFREE;
  }

  return 0;  /* do not store value in hash table */
}

static char* iSeparatorGetOrientationAttrib(Ihandle* ih)
{
  const char* orientation_str[] = { "VERTICAL", "HORIZONTAL" };
  return (char*)orientation_str[ih->data->orientation];
}

static char* iSeparatorGetStyleAttrib(Ihandle* ih)
{
  const char* style_str[] = { "FILL", "LINE", "SUNKENLINE", "DUALLINES", "GRIP", "EMPTY" };
  return (char*)style_str[ih->data->style];
}

static int iSeparatorSetStyleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "FILL"))
    ih->data->style = ISEPARATOR_FILL;
  else if (iupStrEqualNoCase(value, "LINE"))
    ih->data->style = ISEPARATOR_LINE;
  else if (iupStrEqualNoCase(value, "DUALLINES"))
    ih->data->style = ISEPARATOR_DUALLINES;
  else if (iupStrEqualNoCase(value, "EMPTY"))
    ih->data->style = ISEPARATOR_EMPTY;
  else if (iupStrEqualNoCase(value, "GRIP"))
    ih->data->style = ISEPARATOR_GRIP;
  else
    ih->data->style = ISEPARATOR_SUNKENLINE;
  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static int iSeparatorSetBarSizeAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->barsize);
  ih->data->barsize_hw = iupdrvScaleNaturalPx(ih->data->barsize);
  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iSeparatorGetBarSizeAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->barsize);
}

/*****************************************************************************************/

static int iSeparatorEnterWindow_CB(Ihandle* ih)
{
  if (ih->data->style == ISEPARATOR_GRIP)
  {
    ih->data->hover = 1;
    IupUpdate(ih);
  }
  return IUP_DEFAULT;
}

static int iSeparatorLeaveWindow_CB(Ihandle* ih)
{
  if (ih->data->hover)
  {
    ih->data->hover = 0;
    IupUpdate(ih);
  }
  return IUP_DEFAULT;
}

static int iSeparatorCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  ih->data->barsize = 5;
  ih->data->barsize_hw = iupdrvScaleNaturalPx(5);
  ih->data->style = ISEPARATOR_SUNKENLINE;
  ih->data->orientation = ISEPARATOR_VERT;
  ih->expand = IUP_EXPAND_HFREE;

  /* change the IupCanvas default values */
  iupAttribSet(ih, "BORDER", "NO");
  iupAttribSet(ih, "CANFOCUS", "NO");

  /* internal callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iSeparatorRedraw_CB);
  IupSetCallback(ih, "ENTERWINDOW_CB", (Icallback)iSeparatorEnterWindow_CB);
  IupSetCallback(ih, "LEAVEWINDOW_CB", (Icallback)iSeparatorLeaveWindow_CB);

  return IUP_NOERROR;
}

static void iSeparatorComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0,
      natural_h = 0;
  int barsize = ih->data->barsize_hw;

  if (ih->data->orientation == ISEPARATOR_HORIZ)
    natural_h = barsize;
  else
    natural_w = barsize;

  *w = natural_w;
  *h = natural_h;

  (void)children_expand; /* unset if not a container */
}

/******************************************************************************/

Iclass* iupSeparatorNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "separator";
  ic->cons = "Separator";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupSeparatorNewClass;
  ic->Create = iSeparatorCreateMethod;
  ic->ComputeNaturalSize = iSeparatorComputeNaturalSizeMethod;

  iupClassRegisterAttribute(ic, "ORIENTATION", iSeparatorGetOrientationAttrib, iSeparatorSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "VERTICAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STYLE", iSeparatorGetStyleAttrib, iSeparatorSetStyleAttrib, IUPAF_SAMEASSYSTEM, "SUNKENLINE", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "160, 160, 160", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BARSIZE", iSeparatorGetBarSizeAttrib, iSeparatorSetBarSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupSeparator(void)
{
  return IupCreate("separator");
}
