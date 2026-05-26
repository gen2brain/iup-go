/** \file
 * \brief Val Control (Slider)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"

#include "iupefl_drv.h"


static void eflValChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFn cb;

  ih->data->val = efl_ui_range_value_get(ev->object);

  cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
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

  if (iupStrToDouble(value, &min))
  {
    ih->data->vmin = min;
    if (slider)
      efl_ui_range_limits_set(slider, min, ih->data->vmax);
  }

  return 0;
}

static int eflValSetMaxAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);
  double max;

  if (iupStrToDouble(value, &max))
  {
    ih->data->vmax = max;
    if (slider)
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
  Efl_Ui_Layout_Orientation dir;

  if (iupStrBoolean(value))
    ih->data->inverted = 1;
  else
    ih->data->inverted = 0;

  if (!slider)
    return 0;

  if (ih->data->orientation == IVAL_VERTICAL)
  {
    if (ih->data->inverted)
      dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL | EFL_UI_LAYOUT_ORIENTATION_INVERTED;
    else
      dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
  }
  else
  {
    if (ih->data->inverted)
      dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL | EFL_UI_LAYOUT_ORIENTATION_INVERTED;
    else
      dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
  }
  efl_ui_layout_orientation_set(slider, dir);

  return 0;
}

static int eflValMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* slider;
  Efl_Ui_Layout_Orientation dir;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  slider = efl_add(EFL_UI_SLIDER_CLASS, parent);
  if (!slider)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)slider;

  if (ih->data->orientation == IVAL_VERTICAL)
  {
    if (ih->data->inverted)
      dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL | EFL_UI_LAYOUT_ORIENTATION_INVERTED;
    else
      dir = EFL_UI_LAYOUT_ORIENTATION_VERTICAL;
  }
  else
  {
    if (ih->data->inverted)
      dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL | EFL_UI_LAYOUT_ORIENTATION_INVERTED;
    else
      dir = EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL;
  }
  efl_ui_layout_orientation_set(slider, dir);

  efl_ui_range_limits_set(slider, ih->data->vmin, ih->data->vmax);
  efl_ui_range_value_set(slider, ih->data->val);

  if (ih->data->step > 0)
    efl_ui_range_step_set(slider, ih->data->step);

  efl_event_callback_add(slider, EFL_UI_RANGE_EVENT_CHANGED, eflValChangedCallback, ih);

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
  }

  iupdrvBaseUnMapMethod(ih);
}

IUP_SDK_API void iupdrvValGetMinSize(Ihandle* ih, int* w, int* h)
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

IUP_SDK_API void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = eflValMapMethod;
  ic->UnMap = eflValUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, eflValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIN", NULL, eflValSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MAX", NULL, eflValSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, eflValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, eflValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", NULL, eflValSetInvertedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupeflSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
