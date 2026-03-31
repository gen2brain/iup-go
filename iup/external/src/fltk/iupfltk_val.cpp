/** \file
 * \brief Valuator Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Slider.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
}

#include "iupfltk_drv.h"


class IupFltkSlider : public Fl_Slider
{
public:
  Ihandle* iup_handle;
  int button_pressed;

  IupFltkSlider(int x, int y, int w, int h, Ihandle* ih, int horizontal)
    : Fl_Slider(x, y, w, h), iup_handle(ih), button_pressed(0)
  {
    if (horizontal)
      type(FL_HOR_NICE_SLIDER);
    else
      type(FL_VERT_NICE_SLIDER);

    range(0.0, 1.0);
    step(0.0);
  }

protected:
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
  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 100;
    *h = 20;
  }
  else
  {
    *w = 20;
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

static int fltkValSetInvertedAttrib(Ihandle* ih, const char* value)
{
  ih->data->inverted = iupStrBoolean(value);
  return 0;
}

static int fltkValMapMethod(Ihandle* ih)
{
  int horizontal = (ih->data->orientation == IVAL_HORIZONTAL);

  IupFltkSlider* slider = new IupFltkSlider(0, 0, 10, 10, ih, horizontal);
  ih->handle = (InativeHandle*)slider;

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

  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_DEFAULT);
}
