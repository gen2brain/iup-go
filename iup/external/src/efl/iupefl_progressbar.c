/** \file
 * \brief ProgressBar Control
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
#include "iup_progressbar.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static int eflProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* pbar = iupeflGetWidget(ih);
  double val;

  if (!pbar)
    return 0;

  if (!value)
    ih->data->value = 0;
  else if (iupStrToDouble(value, &val))
    ih->data->value = val;
  else
    return 0;

  iProgressBarCropValue(ih);

  if (ih->data->marquee)
    return 0;

  efl_ui_range_value_set(pbar, ih->data->value);

  return 0;
}

static int eflProgressBarSetMinAttrib(Ihandle* ih, const char* value)
{
  Eo* pbar = iupeflGetWidget(ih);
  double min;

  if (iupStrToDouble(value, &min))
  {
    ih->data->vmin = min;
    iProgressBarCropValue(ih);

    if (pbar)
      efl_ui_range_limits_set(pbar, ih->data->vmin, ih->data->vmax);
  }

  return 0;
}

static int eflProgressBarSetMaxAttrib(Ihandle* ih, const char* value)
{
  Eo* pbar = iupeflGetWidget(ih);
  double max;

  if (iupStrToDouble(value, &max))
  {
    ih->data->vmax = max;
    iProgressBarCropValue(ih);

    if (pbar)
      efl_ui_range_limits_set(pbar, ih->data->vmin, ih->data->vmax);
  }

  return 0;
}

static int eflProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  Eo* pbar = iupeflGetWidget(ih);

  if (!pbar)
    return 0;

  if (iupStrBoolean(value))
  {
    ih->data->marquee = 1;
    efl_ui_progressbar_infinite_mode_set(pbar, EINA_TRUE);
  }
  else
  {
    ih->data->marquee = 0;
    efl_ui_progressbar_infinite_mode_set(pbar, EINA_FALSE);
    efl_ui_range_value_set(pbar, ih->data->value);
  }

  return 0;
}

static char* eflProgressBarGetMarqueeAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->marquee);
}

static int eflProgressBarSetOrientationAttrib(Ihandle* ih, const char* value)
{
  Eo* pbar = iupeflGetWidget(ih);

  if (!pbar)
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
    efl_ui_layout_orientation_set(pbar, EFL_UI_LAYOUT_ORIENTATION_VERTICAL);
  else
    efl_ui_layout_orientation_set(pbar, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);

  return 0;
}

static char* eflProgressBarGetOrientationAttrib(Ihandle* ih)
{
  Eo* pbar = iupeflGetWidget(ih);

  if (!pbar)
    return "HORIZONTAL";

  Efl_Ui_Layout_Orientation orient = efl_ui_layout_orientation_get(pbar);
  return (orient == EFL_UI_LAYOUT_ORIENTATION_VERTICAL) ? "VERTICAL" : "HORIZONTAL";
}

static int eflProgressBarMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* pbar;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  pbar = efl_add(EFL_UI_PROGRESSBAR_CLASS, parent);
  if (!pbar)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)pbar;

  if (iupStrEqualNoCase(iupAttribGet(ih, "ORIENTATION"), "VERTICAL"))
    efl_ui_layout_orientation_set(pbar, EFL_UI_LAYOUT_ORIENTATION_VERTICAL);
  else
    efl_ui_layout_orientation_set(pbar, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);

  efl_ui_range_limits_set(pbar, ih->data->vmin, ih->data->vmax);

  efl_ui_progressbar_show_progress_label_set(pbar, EINA_FALSE);

  if (ih->data->marquee)
  {
    efl_ui_progressbar_infinite_mode_set(pbar, EINA_TRUE);
  }
  else
  {
    efl_ui_range_value_set(pbar, ih->data->value);
  }

  iupeflBaseAddCallbacks(ih, pbar);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflProgressBarUnMapMethod(Ihandle* ih)
{
  Eo* pbar = iupeflGetWidget(ih);

  if (pbar)
    iupeflDelete(pbar);
}

void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  if (iupStrEqualNoCase(iupAttribGet(ih, "ORIENTATION"), "VERTICAL"))
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

void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = eflProgressBarMapMethod;
  ic->UnMap = eflProgressBarUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, eflProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIN", NULL, eflProgressBarSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MAX", NULL, eflProgressBarSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARQUEE", eflProgressBarGetMarqueeAttrib, eflProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", iProgressBarGetDashedAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", eflProgressBarGetOrientationAttrib, eflProgressBarSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}
