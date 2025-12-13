/** \file
 * \brief GTK4 Focus
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

#include "iupgtk4_drv.h"


IUP_DRV_API void iupgtk4SetCanFocus(GtkWidget *widget, int can)
{
  gtk_widget_set_can_focus(widget, can);
}

void iupdrvSetFocus(Ihandle *ih)
{
  Ihandle* dialog = IupGetDialog(ih);

  if (!gtk_window_is_active((GtkWindow*)dialog->handle))
  {
    gtk_window_present(GTK_WINDOW(dialog->handle));
  }

  gtk_widget_grab_focus(ih->handle);
}

IUP_DRV_API void iupgtk4FocusInOutEvent(GtkEventControllerFocus *controller, Ihandle *ih)
{
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  /* even when ACTIVE=NO the dialog gets this evt */
  if (!iupdrvIsActive(ih))
    return;

  iupCallGetFocusCb(ih);
}

IUP_DRV_API void iupgtk4FocusOutEvent(GtkEventControllerFocus *controller, Ihandle *ih)
{
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  iupCallKillFocusCb(ih);
}
