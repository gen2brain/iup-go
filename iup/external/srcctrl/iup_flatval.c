/** \file
 * \brief FlatVal control
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
#include "iup_drvfont.h"
#include "iup_register.h"
#include "iup_image.h"


/* Orientation */
enum { IFLATVAL_HORIZONTAL, IFLATVAL_VERTICAL };

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */

  int orientation;
  double value;  /* min<=value<max */
  double vmin;
  double vmax;
  double pageStep;
  double step;

  int border_width;
  int focus_width;

  /* aux */
  int has_focus,
      highlighted,
      pressed,
      start_x, start_y,
      dragging;
};


static void iFlatValGetHandlerSize(Ihandle* ih, int is_horizontal, int *width, int *height)
{
  char *image = iupAttribGet(ih, "IMAGE");
  if (image)
  {
    *width = 0;
    *height = 0;
    iupImageGetInfo(image, width, height, NULL);
  }
  else
  {
    int handler_size = iupAttribGetInt(ih, "HANDLERSIZE");

    if (is_horizontal)
    {
      if (handler_size == 0)
        handler_size = (ih->currentheight - 2 * ih->data->focus_width) / 2;

      *width = handler_size;
      *height = ih->currentheight;
    }
    else
    {
      if (handler_size == 0)
        handler_size = (ih->currentwidth - 2 * ih->data->focus_width) / 2;

      *width = ih->currentwidth;
      *height = handler_size;
    }
  }
}

static int iFlatValGetSliderInfo(Ihandle* ih, int dx, int dy, int is_horizontal, int *p, int *p1, int *p2, int *handler_op_size)
{
  int handler_width, handler_height;
  iFlatValGetHandlerSize(ih, is_horizontal, &handler_width, &handler_height);

  if (is_horizontal)
  {
    *p1 = handler_width / 2;
    *p2 = (ih->currentwidth - 2 * ih->data->focus_width) - 1 - handler_width / 2;
    *p = dx;
    if (handler_op_size) *handler_op_size = handler_height;
    return handler_width;
  }
  else
  {
    *p1 = handler_height / 2;
    *p2 = (ih->currentheight - 2 * ih->data->focus_width) - 1 - handler_height / 2;
    *p = dy;
    if (handler_op_size) *handler_op_size = handler_width;
    return handler_height;
  }
}

static void iFlatValGetSliderOpositeInfo(Ihandle* ih, int dx, int dy, int is_horizontal, int *q, int *op_size)
{
  if (is_horizontal)
  {
    *op_size = ih->currentheight - 2 * ih->data->focus_width;
    *q = dy;
  }
  else
  {
    *op_size = ih->currentwidth - 2 * ih->data->focus_width;
    *q = dx;
  }
}

static int iFlatValMoveHandler(Ihandle* ih, int dx, int dy)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  double percent, old_percent, delta;
  int dp, p1, p2;

  iFlatValGetSliderInfo(ih, dx, dy, is_horizontal, &dp, &p1, &p2, NULL);

  if (dp == 0)
    return 0;

  old_percent = percent = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);

  delta = (double)dp / (double)(p2 - p1);
  if (!is_horizontal) delta *= -1.0;

  percent += delta;
  if (percent < 0.0) percent = 0;
  if (percent > 1.0) percent = 1.0;

  if (old_percent == percent)
    return 0;

  ih->data->value = percent * (ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  return 1;
}

static int iFlatValIsInsideHandler(Ihandle* ih, int x, int y)
{
  int handler_size, is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  double percent;
  int p, p1, p2, pmid, handler_op_size;

  if (x < 0 || x > ih->currentwidth - 1 ||
      y < 0 || y > ih->currentheight - 1)
    return 0;

  handler_size = iFlatValGetSliderInfo(ih, x, y, is_horizontal, &p, &p1, &p2, &handler_op_size);

  percent = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
  if (!is_horizontal)
    percent = 1.0 - percent;
  pmid = p1 + iupRound((p2 - p1) * percent);

  p1 = pmid - handler_size / 2;
  p2 = pmid + handler_size / 2;

  if (p >= p1 && p <= p2)
  {
    int q, q1, q2, qmid, op_size;

    iFlatValGetSliderOpositeInfo(ih, x, y, is_horizontal, &q, &op_size);

    qmid = op_size / 2;
    q1 = qmid - handler_op_size / 2;
    q2 = qmid + handler_op_size / 2;

    if (q >= q1 && q <= q2)
      return 1;
    else
      return 0;
  }
  else
    return 0;
}

static int iFlatValHandlerPos(Ihandle *ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  double percent = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
  int p, p1, p2, handler_op_size;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);

  if (!is_horizontal)
    percent = 1.0 - percent;

  return p1 + iupRound((p2 - p1) * percent);
}

static int iFlatValRedraw_CB(Ihandle* ih)
{
  char* bordercolor = iupAttribGetStr(ih, "BORDERCOLOR");
  char* sliderbordercolor = iupAttribGetStr(ih, "SLIDERBORDERCOLOR");
  char* slidercolor = iupAttribGetStr(ih, "SLIDERCOLOR");
  int active = IupGetInt(ih, "ACTIVE");
  char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  int slider_size = iupAttribGetInt(ih, "SLIDERSIZE");
  int border_width = iupAttribGetInt(ih, "BORDERWIDTH");
  int focus_feedback = iupAttribGetBoolean(ih, "FOCUSFEEDBACK");
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int handler_width, handler_height;
  char *image = iupAttribGet(ih, "IMAGE");
  char* bgimage = iupAttribGet(ih, "BACKIMAGE");
  double percent = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
  IdrawCanvas* dc = iupdrvDrawCreateCanvas(ih);
  int currentwidth = ih->currentwidth - (2 * ih->data->focus_width);
  int currentheight = ih->currentheight - (2 * ih->data->focus_width);
  int x1, y1, x2, y2;

  iupDrawParentBackground(dc, ih);

  iFlatValGetHandlerSize(ih, is_horizontal, &handler_width, &handler_height);

  if (is_horizontal)
  {
    x1 = ih->data->focus_width + handler_width / 2;
    x2 = ih->data->focus_width + currentwidth - 1 - handler_width / 2;
    y1 = ih->data->focus_width + (currentheight - slider_size) / 2;
    y2 = y1 + slider_size;
  }
  else
  {
    y1 = ih->data->focus_width + handler_height / 2;
    y2 = ih->data->focus_width + currentheight - 1 - handler_height / 2;
    x1 = ih->data->focus_width + (currentwidth - slider_size) / 2;
    x2 = x1 + slider_size;
  }

  if (bgimage)
  {
    int make_inactive;
    int backimage_zoom = iupAttribGetBoolean(ih, "BACKIMAGEZOOM");
    const char* draw_image = iupFlatGetImageName(ih, "BACKIMAGE", bgimage, 0, 0, 1, &make_inactive);
    if (backimage_zoom)
      iupdrvDrawImage(dc, draw_image, make_inactive, bgcolor, 0, 0, ih->currentwidth, ih->currentheight);
    else
      iupdrvDrawImage(dc, draw_image, make_inactive, bgcolor, 0, 0, -1, -1);
  }

  /* draw slider background */
  iupFlatDrawBox(dc, x1, x2, y1, y2, slidercolor, NULL, active);

  /* draw slider border - can be disabled setting bwidth=0 */
  iupFlatDrawBorder(dc, x1, x2, y1, y2, 1, sliderbordercolor, bgcolor, active);

  if (is_horizontal)
  {
    int xmid = x1 + iupRound((x2 - x1) * percent);

    x1 = xmid - handler_width / 2;  if (x1 < ih->data->focus_width) x1 = ih->data->focus_width;
    x2 = xmid + handler_width / 2;  if (x2 > ih->data->focus_width + currentwidth - 1) x2 = ih->data->focus_width + currentwidth - 1;
    y1 = ih->data->focus_width;
    y2 = ih->data->focus_width + currentheight - 1;
  }
  else
  {
    int ymid = y1 + iupRound((y2 - y1) * (1.0 - percent));

    y1 = ymid - handler_height / 2;  if (y1 < ih->data->focus_width) y1 = ih->data->focus_width;
    y2 = ymid + handler_height / 2;  if (y2 > ih->data->focus_width + currentheight - 1) y2 = ih->data->focus_width + currentheight - 1;
    x1 = ih->data->focus_width;
    x2 = ih->data->focus_width + currentwidth - 1;
  }

  if (image)
  {
    int x, y, width = 0, height = 0;
    iupImageGetInfo(image, &width, &height, NULL);

    /* always center the image */
    x = (x2 - x1 + 1 - width) / 2;
    y = (y2 - y1 + 1 - height) / 2;

    iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
    iupdrvDrawImage(dc, image, active, bgcolor, x1 + x, y1 + y, -1, -1);
    iupdrvDrawResetClip(dc);
  }
  else
  {
    char* fgcolor = iupAttribGetStr(ih, "FGCOLOR");

    if (ih->data->pressed)
    {
      char* presscolor = iupAttribGetStr(ih, "PSCOLOR");
      if (presscolor)
        fgcolor = presscolor;
    }
    else if (ih->data->highlighted)
    {
      char* hlcolor = iupAttribGetStr(ih, "HLCOLOR");
      if (hlcolor)
        fgcolor = hlcolor;
    }

    /* draw handler foreground */
    iupFlatDrawBox(dc, x1 + border_width, x2 - border_width, y1 + border_width, y2 - border_width,
                   fgcolor, bgcolor, active);

    if (ih->data->pressed)
    {
      char* presscolor = iupAttribGetStr(ih, "BORDERPSCOLOR");
      if (presscolor)
        bordercolor = presscolor;
    }
    else if (ih->data->highlighted)
    {
      char* hlcolor = iupAttribGetStr(ih, "BORDERHLCOLOR");
      if (hlcolor)
        bordercolor = hlcolor;
    }

    /* draw handler border - can still be disabled setting bwidth=0
    after the background because of the round rect */
    iupFlatDrawBorder(dc, x1, x2, y1, y2,
                      border_width, bordercolor, bgcolor, active);
  }

  if (ih->data->has_focus && focus_feedback)
    iupdrvDrawFocusRect(dc, 0, 0, ih->currentwidth - 1, ih->currentheight - 1);

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);

  return IUP_DEFAULT;
}

static void iFlatCallValueChangedCb(Ihandle* ih)
{
  IFni cb = (IFni)IupGetCallback(ih, "VALUECHANGING_CB");
  if (cb) cb(ih, 0);

  iupBaseCallValueChangedCb(ih);
}

static int iFlatValUpdateHighlighted(Ihandle* ih, int x, int y)
{
  /* handle when mouse is pressed and moved to/from inside the handler area */
  if (!iFlatValIsInsideHandler(ih, x, y))
  {
    if (ih->data->highlighted)
    {
      ih->data->highlighted = 0;
      return 1;
    }
  }
  else
  {
    if (!ih->data->highlighted)
    {
      ih->data->highlighted = 1;
      return 1;
    }
  }

  return 0;
}

static int iFlatValButton_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  IFniiiis button_cb = (IFniiiis)IupGetCallback(ih, "FLAT_BUTTON_CB");
  if (button_cb)
  {
    if (button_cb(ih, button, pressed, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (button == IUP_BUTTON1)
  {
    iFlatValUpdateHighlighted(ih, x, y);

    if (pressed)
    {
      if (!iFlatValIsInsideHandler(ih, x, y))
        ih->data->pressed = 0;
      else
      {
        ih->data->pressed = 1;
        ih->data->start_x = x;
        ih->data->start_y = y;
      }
    }
    else
    {
      if (ih->data->dragging)
      {
        IFni cb = (IFni)IupGetCallback(ih, "VALUECHANGING_CB");
        if (cb) cb(ih, 0);

        ih->data->dragging = 0;
      }
      else if (!ih->data->pressed) /* click outside the handler */
      {
        int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
        int p, p1, p2, dx, dy, handler_op_size, pginc, handPos;

        iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);
        pginc = (int)ceil(ih->data->pageStep * (p2 - p1));

        handPos = iFlatValHandlerPos(ih);

        dx = (is_horizontal) ? (x > handPos) ? pginc : -pginc : 0;
        dy = (is_horizontal) ? 0 : (y > handPos) ? pginc : -pginc;

        if (iFlatValMoveHandler(ih, dx, dy))
          iFlatCallValueChangedCb(ih);
      }
      ih->data->pressed = 0;
    }

    iupdrvRedrawNow(ih);
  }

  (void)status;
  return IUP_DEFAULT;
}

static int iFlatValMotion_CB(Ihandle* ih, int x, int y, char* status)
{
  IFniis motion_cb = (IFniis)IupGetCallback(ih, "FLAT_MOTION_CB");
  int redraw = 0;

  if (motion_cb)
  {
    if (motion_cb(ih, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  /* must update always, and not just when button1 is pressed */
  redraw = iFlatValUpdateHighlighted(ih, x, y);

  if (iup_isbutton1(status))
  {
    if (iFlatValMoveHandler(ih, x - ih->data->start_x, y - ih->data->start_y))
    {
      iupdrvRedrawNow(ih);
      redraw = 0;

      if (!ih->data->dragging)
      {
        IFni cb = (IFni)IupGetCallback(ih, "VALUECHANGING_CB");
        if (cb) cb(ih, 1);
      }

      iupBaseCallValueChangedCb(ih);

      ih->data->dragging = 1;
    }

    ih->data->start_x = x;
    ih->data->start_y = y;
  }

  if (redraw)
    iupdrvRedrawNow(ih);

  return IUP_DEFAULT;
}

static int iFlatValEnterWindow_CB(Ihandle* ih, int x, int y)
{
  IFn cb = (IFn)IupGetCallback(ih, "FLAT_ENTERWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  /* special highlight processing for handler area */
  if (iFlatValIsInsideHandler(ih, x, y))
    ih->data->highlighted = 1;
  else
    ih->data->highlighted = 0;

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValFocus_CB(Ihandle* ih, int focus)
{
  IFni cb = (IFni)IupGetCallback(ih, "FLAT_FOCUS_CB");
  if (cb)
  {
    if (cb(ih, focus) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  ih->data->has_focus = focus;
  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValLeaveWindow_CB(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "FLAT_LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  ih->data->highlighted = 0;

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static void iFlatValCropValue(Ihandle* ih)
{
  if (ih->data->value > ih->data->vmax)
    ih->data->value = ih->data->vmax;
  else if (ih->data->value < ih->data->vmin)
    ih->data->value = ih->data->vmin;
}

static int iFlatValKUp_CB(Ihandle* ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int inc, p, p1, p2, handler_op_size, dx, dy;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);
  inc = (int)ceil(ih->data->step * (p2 - p1));

  dx = (is_horizontal) ? inc : 0;
  dy = (is_horizontal) ? 0 : -inc;

  if (iFlatValMoveHandler(ih, dx, dy))
    iFlatCallValueChangedCb(ih);

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValKDown_CB(Ihandle* ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int inc, p, p1, p2, handler_op_size, dx, dy;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);
  inc = (int)ceil(ih->data->step * (p2 - p1));

  dx = (is_horizontal) ? -inc : 0;
  dy = (is_horizontal) ? 0 : inc;

  if (iFlatValMoveHandler(ih, dx, dy))
    iFlatCallValueChangedCb(ih);

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValWheel_CB(Ihandle* ih, float delta, int x, int y, char* status)
{
  IFnfiis cb = (IFnfiis)IupGetCallback(ih, "FLAT_WHEEL_CB");
  if (cb)
  {
    if (cb(ih, delta, x, y, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (delta > 0)
    iFlatValKUp_CB(ih);
  else
    iFlatValKDown_CB(ih);

  return IUP_DEFAULT;
}

static int iFlatValKHome_CB(Ihandle* ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int p, p1, p2, dx, dy, handler_op_size, handPos;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);

  handPos = iFlatValHandlerPos(ih);

  dx = (is_horizontal) ? p1 - handPos : 0;
  dy = (is_horizontal) ? 0 : handPos - p1;

  if (iFlatValMoveHandler(ih, dx, dy))
    iFlatCallValueChangedCb(ih);

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValKEnd_CB(Ihandle* ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int p, p1, p2, dx, dy, handler_op_size, handPos;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);

  handPos = iFlatValHandlerPos(ih);

  dx = (is_horizontal) ? p2 - handPos : 0;
  dy = (is_horizontal) ? 0 : handPos - p2;

  if (iFlatValMoveHandler(ih, dx, dy))
    iFlatCallValueChangedCb(ih);

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValKPgUp_CB(Ihandle* ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int p, p1, p2, dx, dy, handler_op_size, pginc;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);
  pginc = (int)ceil(ih->data->pageStep * (p2 - p1));

  dx = (is_horizontal) ? pginc : 0;
  dy = (is_horizontal) ? 0 : -pginc;

  if (iFlatValMoveHandler(ih, dx, dy))
    iFlatCallValueChangedCb(ih);

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValKPgDn_CB(Ihandle* ih)
{
  int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;
  int p, p1, p2, dx, dy, handler_op_size, pginc;

  iFlatValGetSliderInfo(ih, 0, 0, is_horizontal, &p, &p1, &p2, &handler_op_size);
  pginc = (int)ceil(ih->data->pageStep * (p2 - p1));

  dx = (is_horizontal) ? -pginc : 0;
  dy = (is_horizontal) ? 0 : pginc;

  if (iFlatValMoveHandler(ih, dx, dy))
    iFlatCallValueChangedCb(ih);

  IupUpdate(ih);

  return IUP_DEFAULT;
}

static int iFlatValSetAttribPostRedraw(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    iupdrvPostRedraw(ih);
  return 1;
}

static int iFlatValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (value == NULL)
    ih->data->value = 0;
  else
  {
    if (iupStrToDouble(value, &(ih->data->value)))
      iFlatValCropValue(ih);
  }

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iFlatValGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->value);
}

static int iFlatValSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
    iFlatValCropValue(ih);

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iFlatValGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static int iFlatValSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
    iFlatValCropValue(ih);

  IupUpdate(ih);
  return 0; /* do not store value in hash table */
}

static char* iFlatValGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static char* iFlatValGetStepAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->step);
}

static char* iFlatValGetPageStepAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->pageStep);
}

static int iFlatValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pageStep), 0.1);
  return 0; /* do not store value in hash table */
}

static int iFlatValSetStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->step), 0.01);
  return 0; /* do not store value in hash table */
}

static int iFlatValSetOrientationAttrib(Ihandle* ih, const char* value)
{
  /* valid only before map */
  if (ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->orientation = IFLATVAL_VERTICAL;
  else /* "HORIZONTAL" */
    ih->data->orientation = IFLATVAL_HORIZONTAL;

  return 0; /* do not store value in hash table */
}

static char* iFlatValGetOrientationAttrib(Ihandle* ih)
{
  if (ih->data->orientation == IFLATVAL_VERTICAL)
    return "VERTICAL";
  else /* (ih->data->orientation == IFLATVAL_HORIZONTAL) */
    return "HORIZONTAL";
}

static char* iFlatValGetHasFocusAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->has_focus);
}

static int iFlatValSetBorderWidthAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->border_width);
  if (ih->handle)
    IupUpdate(ih);
  return 0;
}

static char* iFlatValGetBorderWidthAttrib(Ihandle *ih)
{
  return iupStrReturnInt(ih->data->border_width);
}

static void iFlatValComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0,
      natural_h = 0;
  int fit2backimage = iupAttribGetBoolean(ih, "FITTOBACKIMAGE");
  char* bgimage = iupAttribGet(ih, "BACKIMAGE");

  if (fit2backimage && bgimage)
  {
    iupAttribSet(ih, "BORDERWIDTH", "0");
    iupImageGetInfo(bgimage, &natural_w, &natural_h, NULL);
  }
  else
  {
    int charwidth, charheight;
    int is_horizontal = ih->data->orientation == IFLATVAL_HORIZONTAL;

    iupdrvFontGetCharSize(ih, &charwidth, &charheight);

    if (is_horizontal)
    {
      natural_h = charheight;
      if (ih->userwidth <= 0)
        natural_w = 15 * charwidth;
    }
    else
    {
      natural_w = charheight;
      if (ih->userheight <= 0)
        natural_h = 15 * charwidth;
    }
  }

  *w = natural_w + 2 * ih->data->focus_width;
  *h = natural_h + 2 * ih->data->focus_width;

  (void)children_expand; /* unset if not a container */
}

static int iFlatValCreateMethod(Ihandle* ih, void **params)
{
  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  if (params && params[0])
    iFlatValSetOrientationAttrib(ih, params[0]);

  /* change the IupCanvas default values */
  IupSetAttribute(ih, "EXPAND", "NO");

  ih->data->vmax = 1;
  ih->data->step = 0.01;
  ih->data->pageStep = 0.10;
  ih->data->focus_width = 2;

  iupAttribSet(ih, "HLCOLOR", "30 150 250");
  iupAttribSet(ih, "PSCOLOR", "0 60 190");
  iupAttribSet(ih, "BORDERHLCOLOR", "0 120 220");

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iFlatValRedraw_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iFlatValButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iFlatValMotion_CB);
  IupSetCallback(ih, "LEAVEWINDOW_CB", (Icallback)iFlatValLeaveWindow_CB);
  IupSetCallback(ih, "ENTERWINDOW_CB", (Icallback)iFlatValEnterWindow_CB);
  IupSetCallback(ih, "WHEEL_CB", (Icallback)iFlatValWheel_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)iFlatValFocus_CB);
  IupSetCallback(ih, "K_UP", (Icallback)iFlatValKUp_CB);
  IupSetCallback(ih, "K_DOWN", (Icallback)iFlatValKDown_CB);
  IupSetCallback(ih, "K_RIGHT", (Icallback)iFlatValKUp_CB);
  IupSetCallback(ih, "K_LEFT", (Icallback)iFlatValKDown_CB);
  IupSetCallback(ih, "K_HOME", (Icallback)iFlatValKHome_CB);
  IupSetCallback(ih, "K_END", (Icallback)iFlatValKEnd_CB);
  IupSetCallback(ih, "K_PGUP", (Icallback)iFlatValKPgUp_CB);
  IupSetCallback(ih, "K_PGDN", (Icallback)iFlatValKPgDn_CB);

  return IUP_NOERROR;
}

Iclass* iupFlatValNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "flatval";
  ic->cons = "FlatVal";
  ic->format = NULL; /* no parameters */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupFlatValNewClass;
  ic->Create = iFlatValCreateMethod;
  ic->ComputeNaturalSize = iFlatValComputeNaturalSizeMethod;

  /* Do not need to set base attributes because they are inherited from IupCanvas */

  /* replace IupCanvas behavior */
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribFlags(ic, "BORDER", IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* Callbacks */
  iupClassRegisterCallback(ic, "FLAT_BUTTON_CB", "iiiis");
  iupClassRegisterCallback(ic, "FLAT_MOTION_CB", "iis");
  iupClassRegisterCallback(ic, "FLAT_ENTERWINDOW_CB", "ii");
  iupClassRegisterCallback(ic, "FLAT_LEAVEWINDOW_CB", "");
  iupClassRegisterCallback(ic, "FLAT_FOCUS_CB", "i");
  iupClassRegisterCallback(ic, "FLAT_WHEEL_CB", "fiis");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  iupClassRegisterCallback(ic, "VALUECHANGING_CB", "i");

  iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupFlatSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* IupFlatVal only */
  iupClassRegisterAttribute(ic, "MIN", iFlatValGetMinAttrib, iFlatValSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", iFlatValGetMaxAttrib, iFlatValSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iFlatValGetValueAttrib, iFlatValSetValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", iFlatValGetOrientationAttrib, iFlatValSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iFlatValGetPageStepAttrib, iFlatValSetPageStepAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "STEP", iFlatValGetStepAttrib, iFlatValSetStepAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);   /* force new default value */
  iupClassRegisterAttribute(ic, "FOCUSFEEDBACK", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HASFOCUS", iFlatValGetHasFocusAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SLIDERSIZE", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HANDLERSIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BORDERCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, IUP_FLAT_BORDERCOLOR, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERPSCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERHLCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "BORDERWIDTH", iFlatValGetBorderWidthAttrib, iFlatValSetBorderWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED);  /* inheritable */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iFlatValSetAttribPostRedraw, "0 120 220", NULL, IUPAF_DEFAULT);  /* force the new default value */
  iupClassRegisterAttribute(ic, "HLCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "PSCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "SLIDERBORDERCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "160 160 160", IUPAF_DEFAULT);  /* inheritable */
  iupClassRegisterAttribute(ic, "SLIDERCOLOR", NULL, iFlatValSetAttribPostRedraw, IUPAF_SAMEASSYSTEM, "220 220 220", IUPAF_DEFAULT);  /* inheritable */

  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEHIGHLIGHT", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKIMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITTOBACKIMAGE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupFlatVal(const char *orientation)
{
  void *params[2];
  params[0] = (void*)orientation;
  params[1] = NULL;
  return IupCreatev("flatval", params);
}
