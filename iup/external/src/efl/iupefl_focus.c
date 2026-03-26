/** \file
 * \brief EFL Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_drv.h"

#include "iupefl_drv.h"


IUP_DRV_API void iupeflSetCanFocus(Eo *widget, int can)
{
  if (efl_isa(widget, EFL_UI_WIDGET_CLASS))
    efl_ui_widget_focus_allow_set(widget, can ? EINA_TRUE : EINA_FALSE);
}

IUP_SDK_API void iupdrvSetFocus(Ihandle *ih)
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

IUP_DRV_API void iupeflFocusChangedEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Bool focused = *((Eina_Bool*)ev->info);

  if (!iupObjectCheck(ih))
    return;

  if (focused)
  {
    if (!iupdrvIsActive(ih))
      return;

    iupCallGetFocusCb(ih);
  }
  else
    iupCallKillFocusCb(ih);
}

IUP_DRV_API void iupeflManagerFocusChangedEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eina_Bool* prev_focused = (Eina_Bool*)iupAttribGet(ih, "_IUP_EFL_MANAGER_FOCUSED");
  Eina_Bool currently_focused = (efl_ui_focus_manager_focus_get(ev->object) != NULL);

  if (!iupObjectCheck(ih))
    return;

  if (prev_focused && *prev_focused == currently_focused)
    return;

  if (!prev_focused)
  {
    prev_focused = malloc(sizeof(Eina_Bool));
    iupAttribSet(ih, "_IUP_EFL_MANAGER_FOCUSED", (char*)prev_focused);
  }
  *prev_focused = currently_focused;

  if (currently_focused)
  {
    if (!iupdrvIsActive(ih))
      return;

    iupCallGetFocusCb(ih);
  }
  else
    iupCallKillFocusCb(ih);
}

IUP_DRV_API void iupeflDialogSetFocus(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return;

  efl_ui_focus_util_focus(widget);
  efl_ui_win_activate(widget);
}
