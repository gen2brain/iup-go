/** \file
 * \brief Scrollbar Control (EFL)
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
#include "iup_scrollbar.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static void eflScrollbarChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  double old_val = ih->data->val;
  double val;

  val = efl_ui_range_value_get(ev->object);

  ih->data->val = val;
  iupScrollbarCropValue(ih);

  {
    IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (scroll_cb)
    {
      float posx = 0, posy = 0;
      int op;

      if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      {
        posx = (float)ih->data->val;
        op = IUP_SBPOSH;
      }
      else
      {
        posy = (float)ih->data->val;
        op = IUP_SBPOSV;
      }

      scroll_cb(ih, op, posx, posy);
    }
  }

  {
    IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
    if (valuechanged_cb)
    {
      if (ih->data->val != old_val)
        valuechanged_cb(ih);
    }
  }
}

static int eflScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);
  double val;

  if (!slider)
    return 0;

  if (iupStrToDouble(value, &val))
  {
    ih->data->val = val;
    iupScrollbarCropValue(ih);
    efl_ui_range_value_set(slider, ih->data->val);
  }

  return 0;
}

static int eflScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  Eo* slider = iupeflGetWidget(ih);

  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
  {
    if (slider)
      efl_ui_range_step_set(slider, ih->data->linestep);
  }

  return 0;
}

static int eflScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
  return 0;
}

static int eflScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
    iupScrollbarCropValue(ih);
  return 0;
}

static int eflScrollbarMapMethod(Ihandle* ih)
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

  if (ih->data->orientation == ISCROLLBAR_VERTICAL)
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

  if (ih->data->linestep > 0)
    efl_ui_range_step_set(slider, ih->data->linestep);

  efl_event_callback_add(slider, EFL_UI_RANGE_EVENT_CHANGED, eflScrollbarChangedCallback, ih);

  iupeflBaseAddCallbacks(ih, slider);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflScrollbarUnMapMethod(Ihandle* ih)
{
  Eo* slider = iupeflGetWidget(ih);

  if (slider)
  {
    efl_event_callback_del(slider, EFL_UI_RANGE_EVENT_CHANGED, eflScrollbarChangedCallback, ih);
    iupeflDelete(slider);
  }
}

void iupdrvScrollbarGetMinSize(Ihandle* ih, int* w, int* h)
{
  if (ih->data->orientation == ISCROLLBAR_VERTICAL)
  {
    *w = 15;
    *h = 100;
  }
  else
  {
    *w = 100;
    *h = 15;
  }
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = eflScrollbarMapMethod;
  ic->UnMap = eflScrollbarUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, eflScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, eflScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, eflScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, eflScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
