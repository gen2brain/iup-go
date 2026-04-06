/** \file
 * \brief Progress bar Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Progress.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"
}

#include "iupfltk_drv.h"


class IupFltkVertProgress : public Fl_Widget
{
public:
  double vmin, vmax, val;

  IupFltkVertProgress(int x, int y, int w, int h)
    : Fl_Widget(x, y, w, h), vmin(0), vmax(100), val(0) {}

  void value(double v) { val = v; redraw(); }

  void draw() override
  {
    fl_draw_box(FL_DOWN_BOX, x(), y(), w(), h(), FL_BACKGROUND2_COLOR);

    double frac = 0;
    if (vmax > vmin)
      frac = (val - vmin) / (vmax - vmin);
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;

    int bx = Fl::box_dx(FL_DOWN_BOX);
    int by = Fl::box_dy(FL_DOWN_BOX);
    int bw = w() - Fl::box_dw(FL_DOWN_BOX);
    int bh = h() - Fl::box_dh(FL_DOWN_BOX);
    int fh = (int)(frac * bh + 0.5);

    if (fh > 0)
    {
      fl_color(selection_color());
      fl_rectf(x() + bx, y() + by + bh - fh, bw, fh);
    }
  }
};

typedef struct _IupFltkMarquee {
  double pos;
  int dir;
} IupFltkMarquee;

static void fltkProgressBarMarqueeTimeout(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  if (!ih || !iupObjectCheck(ih))
    return;

  if (!ih->handle)
    return;

  IupFltkMarquee* mq = (IupFltkMarquee*)iupAttribGet(ih, "_IUP_FLTK_MARQUEE");
  if (!mq)
    return;

  mq->pos += mq->dir * 5.0;
  if (mq->pos >= 100.0)
  {
    mq->pos = 100.0;
    mq->dir = -1;
  }
  else if (mq->pos <= 0.0)
  {
    mq->pos = 0.0;
    mq->dir = 1;
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    IupFltkVertProgress* vp = (IupFltkVertProgress*)ih->handle;
    vp->value(mq->pos);
  }
  else
  {
    Fl_Progress* pbar = (Fl_Progress*)ih->handle;
    pbar->value((float)mq->pos);
  }

  Fl::repeat_timeout(0.05, fltkProgressBarMarqueeTimeout, data);
}

extern "C" IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int *w, int *h)
{
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    *w = 20;
    *h = 100;
  }
  else
  {
    *w = 100;
    *h = 20;
  }
}

static int fltkProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->marquee)
    return 0;

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  if (ih->handle)
  {
    double range = ih->data->vmax - ih->data->vmin;
    float fraction = (range != 0) ? (float)((ih->data->value - ih->data->vmin) / range) : 0;

    if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
    {
      IupFltkVertProgress* vp = (IupFltkVertProgress*)ih->handle;
      vp->value(fraction * 100.0);
    }
    else
    {
      Fl_Progress* pbar = (Fl_Progress*)ih->handle;
      pbar->value(fraction * 100.0f);
    }
  }

  return 0;
}

static int fltkProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee)
    return 0;

  if (iupStrBoolean(value))
  {
    IupFltkMarquee* mq = (IupFltkMarquee*)iupAttribGet(ih, "_IUP_FLTK_MARQUEE");
    if (!mq)
    {
      mq = (IupFltkMarquee*)malloc(sizeof(IupFltkMarquee));
      iupAttribSet(ih, "_IUP_FLTK_MARQUEE", (char*)mq);
    }
    mq->pos = 0.0;
    mq->dir = 1;
    Fl::add_timeout(0.05, fltkProgressBarMarqueeTimeout, (void*)ih);
  }
  else
  {
    Fl::remove_timeout(fltkProgressBarMarqueeTimeout, (void*)ih);

    if (ih->handle)
    {
      if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
      {
        IupFltkVertProgress* vp = (IupFltkVertProgress*)ih->handle;
        vp->value(0);
      }
      else
      {
        Fl_Progress* pbar = (Fl_Progress*)ih->handle;
        pbar->value(0);
      }
    }
  }

  return 1;
}

static int fltkProgressBarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
    widget->color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkProgressBarSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
    widget->selection_color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkProgressBarMapMethod(Ihandle* ih)
{
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    IupFltkVertProgress* pbar = new IupFltkVertProgress(0, 0, 20, 100);
    ih->handle = (InativeHandle*)pbar;
    pbar->selection_color(FL_SELECTION_COLOR);
  }
  else
  {
    Fl_Progress* pbar = new Fl_Progress(0, 0, 100, 20);
    ih->handle = (InativeHandle*)pbar;
    pbar->minimum(0);
    pbar->maximum(100);
    pbar->value(0);
    pbar->selection_color(FL_SELECTION_COLOR);
  }

  iupfltkAddToParent(ih);

  return IUP_NOERROR;
}

static void fltkProgressBarUnMapMethod(Ihandle* ih)
{
  Fl::remove_timeout(fltkProgressBarMarqueeTimeout, (void*)ih);

  IupFltkMarquee* mq = (IupFltkMarquee*)iupAttribGet(ih, "_IUP_FLTK_MARQUEE");
  if (mq)
  {
    free(mq);
    iupAttribSet(ih, "_IUP_FLTK_MARQUEE", NULL);
  }

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
  {
    delete widget;
    ih->handle = NULL;
  }
}

extern "C" IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = fltkProgressBarMapMethod;
  ic->UnMap = fltkProgressBarUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkProgressBarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkProgressBarSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, fltkProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, fltkProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
