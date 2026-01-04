/** \file
 * \brief EFL Driver tooltips
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

#include "iupefl_drv.h"


int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  Evas_Object* widget;

  widget = iupeflGetWidget(ih);
  if (!widget)
    return 1;

  if (value && *value)
    elm_object_tooltip_text_set(widget, value);
  else
    elm_object_tooltip_unset(widget);

  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  Evas_Object* widget;

  widget = iupeflGetWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
    elm_object_tooltip_show(widget);
  else
    elm_object_tooltip_hide(widget);

  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}
