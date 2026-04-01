/** \file
 * \brief FLTK Focus Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>

#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_assert.h"
}

#include "iupfltk_drv.h"


IUP_DRV_API void iupfltkSetCanFocus(Fl_Widget* widget, int can)
{
  if (!widget)
    return;

  if (can)
    widget->set_visible_focus();
  else
    widget->clear_visible_focus();
}

extern "C" IUP_SDK_API void iupdrvSetFocus(Ihandle* ih)
{
  Fl_Widget* widget = (Fl_Widget*)ih->handle;

  if (!widget)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (dialog && dialog->handle)
  {
    Fl_Window* dialog_window = ((Fl_Widget*)dialog->handle)->as_window();
    if (dialog_window && dialog_window->shown())
      dialog_window->show();
  }

  widget->take_focus();
}

IUP_DRV_API int iupfltkFocusInOutEvent(Fl_Widget* widget, Ihandle* ih, int event)
{
  (void)widget;

  if (!iupObjectCheck(ih))
    return 1;

  if (event == FL_FOCUS)
  {
    if (!iupdrvIsActive(ih))
      return 1;

    Ihandle* dialog = IupGetDialog(ih);
    if (dialog && ih != dialog)
      iupAttribSet(dialog, "_IUPFLTK_LASTFOCUS", (char*)ih);

    iupCallGetFocusCb(ih);
  }
  else if (event == FL_UNFOCUS)
  {
    iupCallKillFocusCb(ih);
  }

  return 0;
}
