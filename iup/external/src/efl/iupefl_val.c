/** \file
 * \brief Val Control (Slider)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static void eflValChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFnd cb;
  double val;

  val = efl_ui_range_value_get(ev->object);

  ih->data->val = val;

  cb = (IFnd)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    cb(ih, val);
  }
  else
  {
    IFnd cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    if (cb_old)
      cb_old(ih, ih->data->val);
  }
}

static void eflValDragStartCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFnd cb;

  ih->data->val = efl_ui_range_value_get(ev->object);

  cb = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");
  if (cb)
    cb(ih, ih->data->val);
}

static void eflValDragStopCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFnd cb;

  ih->data->val = efl_ui_range_value_get(ev->object);

  cb = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
  if (cb)
    cb(ih, ih->data->val);
}

static int eflValSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);
  double val;

  if (!slider)
    return 0;

  if (iupStrToDouble(value, &val))
  {
    ih->data->val = val;
    efl_ui_range_value_set(slider, val);
  }

  return 0;
}

static int eflValSetMinAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);
  double min;

  if (!slider)
    return 0;

  if (iupStrToDouble(value, &min))
  {
    ih->data->vmin = min;
    efl_ui_range_limits_set(slider, min, ih->data->vmax);
  }

  return 0;
}

static int eflValSetMaxAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);
  double max;

  if (!slider)
    return 0;

  if (iupStrToDouble(value, &max))
  {
    ih->data->vmax = max;
    efl_ui_range_limits_set(slider, ih->data->vmin, max);
  }

  return 0;
}

static int eflValSetStepAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);
  double step;

  if (!slider)
    return 0;

  if (iupStrToDouble(value, &step))
  {
    ih->data->step = step;
    efl_ui_range_step_set(slider, step);
  }

  return 0;
}

static int eflValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  double pagestep;

  if (iupStrToDouble(value, &pagestep))
    ih->data->pagestep = pagestep;

  return 0;
}

static int eflValSetInvertedAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);

  if (!slider)
    return 0;

  if (iupStrBoolean(value))
    ih->data->inverted = 1;
  else
    ih->data->inverted = 0;

  return 0;
}

static int eflValSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflValSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflValMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* slider;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  slider = efl_add(EFL_UI_SLIDER_CLASS, parent);
  if (!slider)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)slider;

  if (ih->data->orientation == IVAL_VERTICAL)
    efl_ui_layout_orientation_set(slider, EFL_UI_LAYOUT_ORIENTATION_VERTICAL);
  else
    efl_ui_layout_orientation_set(slider, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);

  efl_ui_range_limits_set(slider, ih->data->vmin, ih->data->vmax);
  efl_ui_range_value_set(slider, ih->data->val);

  if (ih->data->step > 0)
    efl_ui_range_step_set(slider, ih->data->step);

  efl_event_callback_add(slider, EFL_UI_RANGE_EVENT_CHANGED, eflValChangedCallback, ih);
  efl_event_callback_add(slider, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_START, eflValDragStartCallback, ih);
  efl_event_callback_add(slider, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_STOP, eflValDragStopCallback, ih);

  iupeflBaseAddCallbacks(ih, slider);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflValUnMapMethod(Ihandle* ih)
{
  Eo* slider = iupeflGetWidget(ih);

  if (slider)
  {
    efl_event_callback_del(slider, EFL_UI_RANGE_EVENT_CHANGED, eflValChangedCallback, ih);
    efl_event_callback_del(slider, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_START, eflValDragStartCallback, ih);
    efl_event_callback_del(slider, EFL_UI_SLIDER_EVENT_SLIDER_DRAG_STOP, eflValDragStopCallback, ih);
    iupeflDelete(slider);
  }
}

void iupdrvValGetMinSize(Ihandle* ih, int* w, int* h)
{
  if (ih->data->orientation == IVAL_VERTICAL)
  {
    *w = 25;
    *h = 100;
  }
  else
  {
    *w = 100;
    *h = 25;
  }
}

void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = eflValMapMethod;
  ic->UnMap = eflValUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, eflValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIN", NULL, eflValSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MAX", NULL, eflValSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, eflValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, eflValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", NULL, eflValSetInvertedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflValSetBgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflValSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
}
