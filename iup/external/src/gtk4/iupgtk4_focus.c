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

#include "iupgtk4_drv.h"


IUP_DRV_API void iupgtk4SetCanFocus(GtkWidget *widget, int can)
{
  gtk_widget_set_can_focus(widget, can);
}

IUP_SDK_API void iupdrvSetFocus(Ihandle *ih)
{
  Ihandle* dialog = IupGetDialog(ih);

  if (dialog && dialog->handle)
  {
    if (!gtk_window_is_active((GtkWindow*)dialog->handle))
      gtk_window_present(GTK_WINDOW(dialog->handle));
  }

  gtk_widget_grab_focus(ih->handle);
}

static void gtk4FocusEnter(GtkEventControllerFocus* controller, Ihandle* ih)
{
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  /* even when ACTIVE=NO the widget gets this event */
  if (!iupdrvIsActive(ih))
    return;

  iupCallGetFocusCb(ih);
}

static void gtk4FocusLeave(GtkEventControllerFocus* controller, Ihandle* ih)
{
  (void)controller;

  if (!iupObjectCheck(ih))
    return;

  iupCallKillFocusCb(ih);
}

void iupgtk4SetupFocusEvents(GtkWidget* widget, Ihandle* ih)
{
  GtkEventController* focus_controller = gtk_event_controller_focus_new();
  gtk_widget_add_controller(widget, focus_controller);
  g_signal_connect(focus_controller, "enter", G_CALLBACK(gtk4FocusEnter), ih);
  g_signal_connect(focus_controller, "leave", G_CALLBACK(gtk4FocusLeave), ih);
}

