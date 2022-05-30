/** \file
 * \brief Gauge control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iupdraw.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_image.h"

#define IGAUGE_DEFAULTCOLOR "0 120 220"
#define IGAUGE_DEFAULTSIZE  "120x14"
#define IGAUGE_DASHED_GAP     3
#define IGAUGE_DASHED_BLOCKS 20

/* Orientation */
enum { IGAUGE_HORIZONTAL, IGAUGE_VERTICAL };

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  int flat;
  int show_text;
  int dashed;
  int horiz_padding, vert_padding;  /* internal margin */

  long bgcolor;
  long fgcolor;
  long light_shadow;
  long mid_shadow;
  long dark_shadow;
  long flatcolor;
  int orientation;

  double value;  /* min<=value<max */
  double vmin;
  double vmax;
};

static void iGaugeDrawText(Ihandle* ih, int xmid, int w, int h, long fgcolor)
{
  int x, y, min, max, xmin, xmax, ymin, ymax, text_w, text_h;
  char* text = iupAttribGet(ih, "TEXT");
  char buffer[30];

  iupAttribSet(ih, "DRAWTEXTALIGNMENT", "ACENTER");

  x = (int)(0.5 * w);
  y = (int)(0.5 * h);

  if (text == NULL)
  {
    sprintf(buffer, "%.1f%%", 100 * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));
    text = buffer;
  }

  IupDrawGetTextSize(ih, text, 0, &text_w, &text_h);
  x -= text_w / 2;
  y -= text_h / 2;
  xmin = x;
  xmax = x + text_w;
  ymin = y;
  ymax = y + text_h;

  min = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xmin : ymin;
  max = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xmax : ymax;

  if (xmid < min)
  {
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawText(ih, text, 0, x, y, text_w, text_h);
  }
  else if (xmid > max)
  {
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->bgcolor);
    IupDrawText(ih, text, 0, x, y, text_w, text_h);
  }
  else
  {
    if (ih->data->orientation == IGAUGE_HORIZONTAL)
      IupDrawSetClipRect(ih, xmin, ymin, xmid, ymax);
    else
      IupDrawSetClipRect(ih, xmin, h - xmid, xmax, ymax);
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->bgcolor);
    IupDrawText(ih, text, 0, x, y, text_w, text_h);

    if (ih->data->orientation == IGAUGE_HORIZONTAL)
      IupDrawSetClipRect(ih, xmid, ymin, xmax, ymax);
    else
      IupDrawSetClipRect(ih, xmin, ymin, xmax, h - xmid);
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawText(ih, text, 0, x, y, text_w, text_h);
    IupDrawResetClip(ih);
  }
}

static int iGaugeRedraw_CB(Ihandle* ih)
{
  char* backcolor = iupAttribGetStr(ih, "BACKCOLOR");
  int border = ih->data->flat ? 1 : 2;
  int xstart, xend, ystart, yend, w, h;
  long fgcolor = ih->data->fgcolor;

  IupDrawBegin(ih);

  IupDrawGetSize(ih, &w, &h);

  IupDrawParentBackground(ih);

  /* draw border */
  if (ih->data->flat)
  {
    iupAttribSet(ih, "DRAWSTYLE", "STROKE");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->flatcolor);
    IupDrawRectangle(ih, 0, 0, w - 1, h - 1);
  }
  else
    iupDrawSunkenRect(ih, 0, 0, w - 1, h - 1, ih->data->light_shadow, ih->data->mid_shadow, ih->data->dark_shadow);

  xstart = ih->data->horiz_padding + border;
  ystart = ih->data->vert_padding + border;
  xend = w - 1 - (ih->data->horiz_padding + border);
  yend = h - 1 - (ih->data->vert_padding + border);

  if (backcolor)
  {
    iupAttribSetStr(ih, "DRAWCOLOR", backcolor);
    iupAttribSet(ih, "DRAWSTYLE", "FILL");
    if (ih->data->orientation == IGAUGE_HORIZONTAL)
      IupDrawRectangle(ih, xstart, ystart, xend, yend);
    else
      IupDrawRectangle(ih, xstart, ih->currentheight - ystart, xend, ih->currentheight - yend);
  }

  xstart++;
  ystart++;
  xend--;
  yend--;

  if (!iupdrvIsActive(ih))
    fgcolor = iupDrawColorMakeInactive(fgcolor, ih->data->bgcolor);

  iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);

  if (ih->data->dashed)
  {
    if (ih->data->value != ih->data->vmin)
    {
      int start = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xstart : ystart;
      int end = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xend : yend;
      double step = (double)(end - start + 1) / (double)IGAUGE_DASHED_BLOCKS;
      double step_fill = step - IGAUGE_DASHED_GAP;
      double range = (double)((end - start + 1) * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));
      int range_percent = (int)(100 * range);
      double i = 0;

      while (iupRound(100 * (i + step_fill)) <= range_percent)
      {
        iupAttribSet(ih, "DRAWSTYLE", "FILL");
        if (ih->data->orientation == IGAUGE_HORIZONTAL)
          IupDrawRectangle(ih, start + iupRound(i), ih->currentheight - ystart,
                               start + iupRound(i + step_fill) - 1, ih->currentheight - yend);
        else
          IupDrawRectangle(ih, xstart, ih->currentheight - (start + iupRound(i)),
                               xend, ih->currentheight - (start + iupRound(i + step_fill) - 1));
        i += step;
      }
    }
  }
  else
  {
    int start = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xstart : ystart;
    int end = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xend : yend;
    int mid = start + iupRound((end - start + 1) * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));

    if (ih->data->value != ih->data->vmin)
    {
      iupAttribSet(ih, "DRAWSTYLE", "FILL");
      if (ih->data->orientation == IGAUGE_HORIZONTAL)
        IupDrawRectangle(ih, xstart, ystart, mid, yend);
      else
        IupDrawRectangle(ih, xstart, ih->currentheight - ystart, xend, ih->currentheight - mid);
    }

    if (ih->data->show_text)
      iGaugeDrawText(ih, mid, w, h, fgcolor);
  }

  IupDrawEnd(ih);
  return IUP_DEFAULT;
}

static void iGaugeCropValue(Ihandle* ih)
{
  if (ih->data->value > ih->data->vmax)
    ih->data->value = ih->data->vmax;
  else if (ih->data->value < ih->data->vmin)
    ih->data->value = ih->data->vmin;
}

static int iGaugeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->fgcolor = iupDrawStrToColor(value, ih->data->fgcolor);
  IupUpdate(ih);
  return 1;
}

static int iGaugeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = iupBaseNativeParentGetBgColorAttrib(ih);

  ih->data->bgcolor = iupDrawStrToColor(value, ih->data->bgcolor);

  iupDrawCalcShadows(ih->data->bgcolor, &ih->data->light_shadow, &ih->data->mid_shadow, &ih->data->dark_shadow);

  if (!iupdrvIsActive(ih))
    ih->data->light_shadow = ih->data->mid_shadow;

  IupUpdate(ih);
  return 1;
}

static int iGaugeSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);

  iupDrawCalcShadows(ih->data->bgcolor, &ih->data->light_shadow, &ih->data->mid_shadow, &ih->data->dark_shadow);

  if (!iupdrvIsActive(ih))
    ih->data->light_shadow = ih->data->mid_shadow;

  IupUpdate(ih);
  return 0;   /* do not store value in hash table */
}

static int iGaugeSetValueAttrib(Ihandle* ih, const char* value)
{
  if (value == NULL)
    ih->data->value = 0;
  else
  {
    if (iupStrToDouble(value, &(ih->data->value)))
      iGaugeCropValue(ih);
  }

  IupRedraw(ih, 0); /* redraw now */
  return 0; /* do not store value in hash table */
}

static char* iGaugeGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->value);
}

static int iGaugeSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
    iGaugeCropValue(ih);

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iGaugeGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static int iGaugeSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
    iGaugeCropValue(ih);

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iGaugeGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int iGaugeSetShowTextAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_text = 1;
  else
    ih->data->show_text = 0;

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iGaugeGetShowTextAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->show_text);
}

static int iGaugeSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->flat = 1;
  else
    ih->data->flat = 0;

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iGaugeGetFlatAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->flat);
}

static int iGaugeSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  IupUpdate(ih);
  return 0;
}

static char* iGaugeGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int iGaugeSetFlatColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->flatcolor = iupDrawStrToColor(value, ih->data->flatcolor);
  return 1;
}

static int iGaugeSetDashedAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->dashed = 1;
  else
    ih->data->dashed = 0;

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iGaugeGetDashedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->dashed);
}

static int iGaugeSetTextAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  IupUpdate(ih);
  return 1;
}

static int iGaugeSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* can NOT be changed after map */
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
  {
    ih->data->orientation = IGAUGE_VERTICAL;

    if (ih->userheight < ih->userwidth) /* make size coherent with orientation */
      IupSetStrf(ih, "RASTERSIZE", "%dx%d", ih->userheight, ih->userwidth);

    iupAttribSet(ih, "DRAWTEXTORIENTATION", "90");
    iupAttribSet(ih, "DRAWTEXTLAYOUTCENTER", "YES");
  }
  else if (iupStrEqualNoCase(value, "HORIZONTAL"))
  {
    ih->data->orientation = IGAUGE_HORIZONTAL;

    if (ih->userwidth < ih->userheight) /* make size coherent with orientation */
      IupSetStrf(ih, "RASTERSIZE", "%dx%d", ih->userheight, ih->userwidth);

    iupAttribSet(ih, "DRAWTEXTORIENTATION", NULL);
    iupAttribSet(ih, "DRAWTEXTLAYOUTCENTER", NULL);
  }
  return 0;
}

static char* iGaugeGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation == IGAUGE_HORIZONTAL)
    return "HORIZONTAL";
  else
    return "VERTICAL";
}

static int iGaugeCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* change the IupCanvas default values */
  IupSetAttribute(ih, "SIZE", IGAUGE_DEFAULTSIZE);
  IupSetAttribute(ih, "EXPAND", "NO");

  /* default values */
  iupAttribSet(ih, "BACKCOLOR", "220 220 220");
  iupAttribSet(ih, "FGCOLOR", IGAUGE_DEFAULTCOLOR);
  ih->data->fgcolor = iupDrawColor(0, 120, 220, 255);
  ih->data->vmax = 1;
  ih->data->bgcolor = iupDrawColor(192, 192, 192, 255);
  ih->data->light_shadow = iupDrawColor(255, 255, 255, 255);
  ih->data->mid_shadow = iupDrawColor(192, 192, 192, 255);
  ih->data->dark_shadow = iupDrawColor(128, 128, 128, 255);
  ih->data->flatcolor = iupDrawColor(160, 160, 160, 255);
  ih->data->show_text = 1;
  ih->data->orientation = IGAUGE_HORIZONTAL;

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iGaugeRedraw_CB);

  return IUP_NOERROR;
}

Iclass* iupGaugeNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "gauge";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupGaugeNewClass;
  ic->Create = iGaugeCreateMethod;

  /* Do not need to set base attributes because they are inherited from IupCanvas */

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribDef(ic, "CANFOCUS", "NO", NULL);

  /* IupGauge only */
  iupClassRegisterAttribute(ic, "MIN", iGaugeGetMinAttrib, iGaugeSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", iGaugeGetMaxAttrib, iGaugeSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iGaugeGetValueAttrib, iGaugeSetValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", iGaugeGetDashedAttrib, iGaugeSetDashedAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iGaugeGetPaddingAttrib, iGaugeSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TEXT", NULL, iGaugeSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "SHOW_TEXT", iGaugeGetShowTextAttrib, iGaugeSetShowTextAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SHOWTEXT", iGaugeGetShowTextAttrib, iGaugeSetShowTextAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iGaugeSetFgColorAttrib, IGAUGE_DEFAULTCOLOR, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", iGaugeGetFlatAttrib, iGaugeSetFlatAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLATCOLOR", NULL, iGaugeSetFlatColorAttrib, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ORIENTATION", iGaugeGetOrientationAttrib, iGaugeSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Overwrite IupCanvas Attributes */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iGaugeSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iGaugeSetBgColorAttrib, NULL, "255 255 255", IUPAF_NO_INHERIT);    /* overwrite canvas implementation, set a system default to force a new default */

  return ic;
}

IUP_API Ihandle* IupGauge(void)
{
  return IupCreate("gauge");
}
