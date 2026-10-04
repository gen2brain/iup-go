/** \file
 * \brief Gauge control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcontrols.h"

#include "iupdraw.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_controls.h"

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
  int circular;
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
    snprintf(buffer, sizeof(buffer), "%.1f%%", 100 * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));
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

static int iGaugeClampRadius(int corner_radius, int x1, int y1, int x2, int y2)
{
  int w = abs(x2 - x1), h = abs(y2 - y1);
  int half = (w < h ? w : h) / 2;
  return corner_radius > half ? half : corner_radius;
}

static void iGaugeDrawCircular(Ihandle* ih, int w, int h, const char* backcolor, long fgcolor)
{
  double range = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
  int ring_width = iupAttribGetInt(ih, "RINGWIDTH");
  int size = iupMIN(w - 2 * ih->data->horiz_padding, h - 2 * ih->data->vert_padding);
  int x1, y1, x2, y2;

  if (ring_width <= 0)
    ring_width = iupMAX(3, size / 8);
  if (size <= 2 * ring_width)
    return;

  x1 = (w - size) / 2 + ring_width / 2;
  y1 = (h - size) / 2 + ring_width / 2;
  x2 = x1 + size - 1 - ring_width;
  y2 = y1 + size - 1 - ring_width;

  iupAttribSet(ih, "DRAWSTYLE", "STROKE");
  iupAttribSetInt(ih, "DRAWLINEWIDTH", ring_width);

  if (ih->data->dashed)
  {
    double step = 360.0 / IGAUGE_DASHED_BLOCKS;
    double gap = step / 4;
    int filled = (int)(range * IGAUGE_DASHED_BLOCKS + 0.001);
    int i;

    for (i = 0; i < IGAUGE_DASHED_BLOCKS; i++)
    {
      double a2 = 90 - i * step - gap / 2;

      if (i < filled)
        iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
      else
        iupAttribSetStr(ih, "DRAWCOLOR", backcolor);

      IupDrawArc(ih, x1, y1, x2, y2, a2 - (step - gap), a2);
    }
  }
  else
  {
    iupAttribSetStr(ih, "DRAWCOLOR", backcolor);
    IupDrawEllipse(ih, x1, y1, x2, y2);

    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    if (range >= 1)
      IupDrawEllipse(ih, x1, y1, x2, y2);
    else if (range > 0)
    {
      iupAttribSet(ih, "DRAWLINECAP", "ROUND");
      IupDrawArc(ih, x1, y1, x2, y2, 90 - 360 * range, 90);
      iupAttribSet(ih, "DRAWLINECAP", NULL);
    }
  }

  iupAttribSet(ih, "DRAWLINEWIDTH", NULL);

  if (ih->data->show_text)
  {
    char* text = iupAttribGet(ih, "TEXT");
    char* orientation = iupAttribGet(ih, "DRAWTEXTORIENTATION");
    char buffer[30];
    int text_w, text_h;

    if (text == NULL)
    {
      snprintf(buffer, sizeof(buffer), "%.0f%%", 100 * range);
      text = buffer;
    }

    iupAttribSet(ih, "DRAWTEXTALIGNMENT", "ACENTER");
    iupAttribSet(ih, "DRAWTEXTORIENTATION", NULL);

    IupDrawGetTextSize(ih, text, 0, &text_w, &text_h);
    iupDrawSetColor(ih, "DRAWCOLOR", fgcolor);
    IupDrawText(ih, text, 0, (w - text_w) / 2, (h - text_h) / 2, text_w, text_h);

    iupAttribSet(ih, "DRAWTEXTORIENTATION", orientation);
  }
}

static int iGaugeRedraw_CB(Ihandle* ih)
{
  char* backcolor = iupAttribGetStr(ih, "BACKCOLOR");
  int corner_radius = iupAttribGetInt(ih, "CORNERRADIUS");
  int border = (ih->data->flat || corner_radius > 0) ? 1 : 2;
  int xstart, xend, ystart, yend, w, h;
  int active = iupdrvIsActive(ih);
  long fgcolor = ih->data->fgcolor;

  IupDrawBegin(ih);

  IupDrawGetSize(ih, &w, &h);

  IupDrawParentBackground(ih);

  if (!active)
  {
    int alpha = iupAttribGetInt(ih, "INACTIVEOPACITY");
    if (alpha > 0 && alpha < 255)
    {
      IupDrawSaveLayer(ih, alpha);
      active = 1;
    }
  }

  if (!backcolor)
  {
    long parentbg = iupDrawStrToColor(iupBaseNativeParentGetBgColorAttrib(ih), ih->data->bgcolor);
    int r = iupDrawRed(parentbg), g = iupDrawGreen(parentbg), b = iupDrawBlue(parentbg);
    int fr = iupDrawRed(fgcolor), fg = iupDrawGreen(fgcolor), fb = iupDrawBlue(fgcolor);

    iupAttribSetStrf(ih, "_IUPGAUGE_BACKCOLOR", "%d %d %d", r + ((fr - r) * 12) / 100,
                                                            g + ((fg - g) * 12) / 100,
                                                            b + ((fb - b) * 12) / 100);
    backcolor = iupAttribGet(ih, "_IUPGAUGE_BACKCOLOR");
  }

  if (ih->data->circular)
  {
    if (!active)
      fgcolor = iupDrawColorMakeInactive(fgcolor, ih->data->bgcolor);

    iGaugeDrawCircular(ih, w, h, backcolor, fgcolor);

    IupDrawEnd(ih);
    return IUP_DEFAULT;
  }

  /* draw border */
  if (corner_radius > 0)
  {
    iupAttribSet(ih, "DRAWSTYLE", "STROKE");
    iupDrawSetColor(ih, "DRAWCOLOR", ih->data->flatcolor);
    IupDrawRoundedRectangle(ih, 0, 0, w - 1, h - 1, iGaugeClampRadius(corner_radius, 0, 0, w - 1, h - 1));
  }
  else if (ih->data->flat)
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
    if (corner_radius > 0)
      IupDrawRoundedRectangle(ih, xstart, ystart, xend, yend, iGaugeClampRadius(corner_radius, xstart, ystart, xend, yend));
    else if (ih->data->orientation == IGAUGE_HORIZONTAL)
      IupDrawRectangle(ih, xstart, ystart, xend, yend);
    else
      IupDrawRectangle(ih, xstart, h - ystart, xend, h - yend);
  }

  xstart++;
  ystart++;
  xend--;
  yend--;

  if (corner_radius > 0)
    IupDrawSetClipRoundedRect(ih, xstart, ystart, xend, yend, iGaugeClampRadius(corner_radius, xstart, ystart, xend, yend));

  if (!active)
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
      double range = (end - start + 1) * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
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

    if (corner_radius > 0)
      IupDrawResetClip(ih);
  }
  else
  {
    int start = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xstart : ystart;
    int end = (ih->data->orientation == IGAUGE_HORIZONTAL) ? xend : yend;
    int mid = start + iupRound((end - start + 1) * (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin));

    if (ih->data->value != ih->data->vmin)
    {
      iupAttribSet(ih, "DRAWSTYLE", "FILL");
      if (corner_radius > 0 && ih->data->orientation == IGAUGE_HORIZONTAL)
        IupDrawRoundedRectangle(ih, xstart, ystart, mid, yend, iGaugeClampRadius(corner_radius, xstart, ystart, xend, yend));
      else if (corner_radius > 0)
        IupDrawRoundedRectangle(ih, xstart, ih->currentheight - mid, xend, ih->currentheight - ystart, iGaugeClampRadius(corner_radius, xstart, ystart, xend, yend));
      else if (ih->data->orientation == IGAUGE_HORIZONTAL)
        IupDrawRectangle(ih, xstart, ystart, mid, yend);
      else
        IupDrawRectangle(ih, xstart, ih->currentheight - ystart, xend, ih->currentheight - mid);
    }

    if (corner_radius > 0)
      IupDrawResetClip(ih);

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

static int iGaugeSetAttribPostRedraw(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
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

static int iGaugeSetCircularAttrib(Ihandle* ih, const char* value)
{
  ih->data->circular = iupStrBoolean(value);

  if (!ih->handle && ih->data->circular && ih->userwidth != ih->userheight)
    IupSetAttribute(ih, "RASTERSIZE", "48x48");

  IupUpdate(ih);
  return 0;
}

static char* iGaugeGetCircularAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->circular);
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

static int iGaugeCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* change the IupCanvas default values */
  IupSetAttribute(ih, "SIZE", IGAUGE_DEFAULTSIZE);
  IupSetAttribute(ih, "EXPAND", "NO");

  /* default values */
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
  ih->data->flat = 1;

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
  iupClassRegisterAttribute(ic, "CORNERRADIUS", NULL, iGaugeSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INACTIVEOPACITY", NULL, iGaugeSetAttribPostRedraw, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CIRCULAR", iGaugeGetCircularAttrib, iGaugeSetCircularAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RINGWIDTH", NULL, iGaugeSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iGaugeGetPaddingAttrib, iGaugeSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CPADDING", iupBaseGetCPaddingAttrib, iupBaseSetCPaddingAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TEXT", NULL, iGaugeSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTEXT", iGaugeGetShowTextAttrib, iGaugeSetShowTextAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iGaugeSetFgColorAttrib, IGAUGE_DEFAULTCOLOR, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", iGaugeGetFlatAttrib, iGaugeSetFlatAttrib, "YES", NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLATCOLOR", NULL, iGaugeSetFlatColorAttrib, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ORIENTATION", iGaugeGetOrientationAttrib, iGaugeSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Overwrite IupCanvas Attributes */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iGaugeSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iGaugeSetBgColorAttrib, NULL, "255 255 255", IUPAF_NO_INHERIT);    /* overwrite canvas implementation, set a system default to force a new default */

  return ic;
}

IUPCONTROLS_API Ihandle* IupGauge(void)
{
  return IupCreate("gauge");
}
