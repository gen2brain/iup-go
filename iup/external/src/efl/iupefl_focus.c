/** \file
 * \brief EFL Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_assert.h"

#include "iupefl_drv.h"


void iupeflSetCanFocus(Eo *widget, int can)
{
  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
    efl_ui_widget_focus_allow_set(widget, can ? EINA_TRUE : EINA_FALSE);
}

void iupdrvSetFocus(Ihandle *ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return;

  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
  {
    efl_ui_focus_util_focus(widget);
  }
  else
  {
    efl_canvas_object_key_focus_set(widget, EINA_TRUE);
  }
}

void iupeflFocusInEvent(void *data, Evas_Object *obj, void *event_info)
{
  Ihandle* ih = (Ihandle*)data;

  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  if (!iupdrvIsActive(ih))
    return;

  iupCallGetFocusCb(ih);
}

void iupeflFocusOutEvent(void *data, Evas_Object *obj, void *event_info)
{
  Ihandle* ih = (Ihandle*)data;

  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  iupCallKillFocusCb(ih);
}

void iupeflDialogSetFocus(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return;

  efl_ui_focus_util_focus(widget);
  efl_ui_win_activate(widget);
}
