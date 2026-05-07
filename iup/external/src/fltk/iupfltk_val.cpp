/** \file
 * \brief Valuator Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Slider.H>
#include <FL/fl_draw.H>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
}

#include "iupfltk_drv.h"


class IupFltkSlider : public Fl_Slider
{
public:
  Ihandle* iup_handle;
  int button_pressed;
  int show_ticks;     /* 0 = none, otherwise number of ticks (>=2) */
  int ticks_pos;      /* 0=NORMAL, 1=REVERSE, 2=BOTH */

  IupFltkSlider(int x, int y, int w, int h, Ihandle* ih, int horizontal)
    : Fl_Slider(x, y, w, h), iup_handle(ih), button_pressed(0), show_ticks(0), ticks_pos(0)
  {
    if (horizontal)
      type(FL_HOR_NICE_SLIDER);
    else
      type(FL_VERT_NICE_SLIDER);

    range(0.0, 1.0);
    step(0.0);
  }

  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        break;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
      case FL_PUSH:
        button_pressed = 1;
        break;
      case FL_RELEASE:
        button_pressed = 0;
        break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;
        break;
    }
    return Fl_Slider::handle(event);
  }

  void draw() override
  {
    Fl_Slider::draw();
    if (show_ticks < 2) return;

    int horizontal = (type() == FL_HOR_NICE_SLIDER);
    int tick_len = 5;
    int n = show_ticks;
    fl_color(fl_contrast(FL_FOREGROUND_COLOR, color()));

    if (horizontal)
    {
      int x_track = x() + tick_len;
      int track_len = w() - 2 * tick_len;
      int y_top = y();
      int y_bot = y() + h() - 1;
      for (int i = 0; i < n; i++)
      {
        int xi = x_track + (i * track_len) / (n - 1);
        if (ticks_pos != 1) fl_yxline(xi, y_bot, y_bot - tick_len);    /* NORMAL or BOTH */
        if (ticks_pos == 1 || ticks_pos == 2) fl_yxline(xi, y_top, y_top + tick_len);  /* REVERSE or BOTH */
      }
    }
    else
    {
      int y_track = y() + tick_len;
      int track_len = h() - 2 * tick_len;
      int x_left = x();
      int x_right = x() + w() - 1;
      for (int i = 0; i < n; i++)
      {
        int yi = y_track + (i * track_len) / (n - 1);
        if (ticks_pos != 1) fl_xyline(x_left, yi, x_left + tick_len);     /* NORMAL or BOTH */
        if (ticks_pos == 1 || ticks_pos == 2) fl_xyline(x_right, yi, x_right - tick_len);  /* REVERSE or BOTH */
      }
    }
  }
};

static void fltkValUpdateValue(Ihandle* ih, int button_release)
{
  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (!slider)
    return;

  double old_val = ih->data->val;
  double fval = slider->value();

  ih->data->val = fval * (ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  iupValCropValue(ih);

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (ih->data->val == old_val)
      return;
    cb(ih);
  }
  else
  {
    IFnd cb_old = NULL;

    if (button_release)
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
    else if (slider->button_pressed)
      cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    else
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");

    if (cb_old)
      cb_old(ih, ih->data->val);
  }
}

static void fltkValCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IupFltkSlider* slider = (IupFltkSlider*)w;
  fltkValUpdateValue(ih, !slider->button_pressed);
}

extern "C" IUP_SDK_API void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  int ticks_size = 0;
  if (iupAttribGetInt(ih, "SHOWTICKS"))
  {
    char* tickspos = iupAttribGetStr(ih, "TICKSPOS");
    ticks_size = iupStrEqualNoCase(tickspos, "BOTH") ? 2 * 6 : 6;
  }

  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 100;
    *h = 20 + ticks_size;
  }
  else
  {
    *w = 20 + ticks_size;
    *h = 100;
  }
}

static int fltkValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
    if (slider)
    {
      double range = ih->data->vmax - ih->data->vmin;
      if (range == 0)
        return 0;

      double fval = (ih->data->val - ih->data->vmin) / range;
      if (fval < 0.0) fval = 0.0;
      if (fval > 1.0) fval = 1.0;

      slider->value(fval);
    }
  }
  return 0;
}

static int fltkValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
  {
    IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
    if (slider)
    {
      double range = ih->data->vmax - ih->data->vmin;
      if (range > 0)
        slider->step(ih->data->step / range);
    }
  }
  return 0;
}

static int fltkValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
  return 0;
}

static int fltkValSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (slider)
    slider->color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkValSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (slider)
    slider->selection_color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkValTicksPosFromString(const char* value)
{
  if (iupStrEqualNoCase(value, "REVERSE")) return 1;
  if (iupStrEqualNoCase(value, "BOTH"))    return 2;
  return 0;
}

static int fltkValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int n = 0;
  if (value) iupStrToInt(value, &n);
  if (n < 0) n = 0;
  if (n > 0 && n < 2) n = 2;
  ih->data->show_ticks = n;

  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (slider)
  {
    slider->show_ticks = n;
    slider->redraw();
  }
  return 1;
}

static int fltkValSetTicksPosAttrib(Ihandle* ih, const char* value)
{
  int pos = fltkValTicksPosFromString(value);
  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (slider)
  {
    slider->ticks_pos = pos;
    slider->redraw();
  }
  return 1;
}

static int fltkValSetInvertedAttrib(Ihandle* ih, const char* value)
{
  ih->data->inverted = iupStrBoolean(value);

  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (slider)
  {
    if (ih->data->inverted)
      slider->range(1.0, 0.0);
    else
      slider->range(0.0, 1.0);

    fltkValSetValueAttrib(ih, iupValGetValueAttrib(ih));
  }
  return 0;
}

static int fltkValMapMethod(Ihandle* ih)
{
  int horizontal = (ih->data->orientation == IVAL_HORIZONTAL);

  IupFltkSlider* slider = new IupFltkSlider(0, 0, 10, 10, ih, horizontal);
  ih->handle = (InativeHandle*)slider;

  slider->show_ticks = ih->data->show_ticks;
  slider->ticks_pos = fltkValTicksPosFromString(iupAttribGetStr(ih, "TICKSPOS"));

  if (ih->data->inverted)
    slider->range(1.0, 0.0);

  {
    double range = ih->data->vmax - ih->data->vmin;
    if (range != 0)
    {
      double fval = (ih->data->val - ih->data->vmin) / range;
      if (fval < 0.0) fval = 0.0;
      if (fval > 1.0) fval = 1.0;
      slider->value(fval);
    }
  }

  slider->callback(fltkValCallback, (void*)ih);

  iupfltkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    slider->visible_focus(0);

  return IUP_NOERROR;
}

static void fltkValUnMapMethod(Ihandle* ih)
{
  IupFltkSlider* slider = (IupFltkSlider*)ih->handle;
  if (slider)
  {
    delete slider;
    ih->handle = NULL;
  }
}

extern "C" IUP_SDK_API void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = fltkValMapMethod;
  ic->UnMap = fltkValUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, fltkValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, fltkValSetStepAttrib, IUPAF_SAMEASSYSTEM, "0.01", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, fltkValSetPageStepAttrib, IUPAF_SAMEASSYSTEM, "0.1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", NULL, fltkValSetInvertedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkValSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkValSetFgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, fltkValSetShowTicksAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, fltkValSetTicksPosAttrib, "NORMAL", NULL, IUPAF_DEFAULT);
}
