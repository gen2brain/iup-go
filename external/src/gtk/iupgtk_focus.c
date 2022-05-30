/** \file
 * \brief GTK Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_assert.h" 
#include "iup_drv.h" 

#include "iupgtk_drv.h"


IUP_DRV_API void iupgtkSetCanFocus(GtkWidget *widget, int can)
{
#if GTK_CHECK_VERSION(2, 18, 0)
  gtk_widget_set_can_focus(widget, can);
#else
  if (can)
    GTK_WIDGET_FLAGS(widget) |= GTK_CAN_FOCUS;
  else
    GTK_WIDGET_FLAGS(widget) &= ~GTK_CAN_FOCUS;
#endif
}

void iupdrvSetFocus(Ihandle *ih)
{
  Ihandle* dialog = IupGetDialog(ih);
  if (!gtk_window_is_active((GtkWindow*)dialog->handle))
    gdk_window_focus(iupgtkGetWindow(dialog->handle), gtk_get_current_event_time());
  gtk_widget_grab_focus(ih->handle);
}

IUP_DRV_API gboolean iupgtkFocusInOutEvent(GtkWidget *widget, GdkEventFocus *evt, Ihandle *ih)
{
  (void)widget;

  if (!iupObjectCheck(ih))
    return TRUE;

  if (evt->in)
  {
    /* even when ACTIVE=NO the dialog gets this evt */
    if (!iupdrvIsActive(ih))
      return TRUE;

    iupCallGetFocusCb(ih);
  }
  else
    iupCallKillFocusCb(ih);

  return FALSE;
}
