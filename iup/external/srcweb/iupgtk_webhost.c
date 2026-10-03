/** \file
 * \brief Web Browser Control - GTK 3 host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_classbase.h"

#include "iupgtk_drv.h"
#include "iupweb_host.h"


static HWND gtkWebHostNative(GtkWidget* area)
{
  GdkWindow* window = gtk_widget_get_window(area);
  if (!window || !gdk_window_ensure_native(window))
    return NULL;
  return gdk_win32_window_get_handle(window);
}

static void gtkWebHostPlace(GtkWidget* area, GdkRectangle* allocation, Ihandle* ih)
{
  HWND hwnd = gtk_widget_get_realized(area) ? gtkWebHostNative(area) : NULL;
  RECT rect;
  (void)allocation;

  if (!hwnd)
    return;

  GetClientRect(hwnd, &rect);
  iupwebHostSetBounds(ih, 0, 0, rect.right, rect.bottom, 0, 0, rect.right, rect.bottom);
}

static void gtkWebHostRealize(GtkWidget* area, Ihandle* ih)
{
  HWND hwnd = gtkWebHostNative(area);
  if (!hwnd)
    return;

  iupwebHostSetParent(ih, hwnd);
  gtkWebHostPlace(area, NULL, ih);
}

static void gtkWebHostUnrealize(GtkWidget* area, Ihandle* ih)
{
  (void)area;
  iupwebHostSetParent(ih, NULL);
}

void* iupwebHostMap(Ihandle* ih)
{
  GtkWidget* area = gtk_drawing_area_new();
  ih->handle = area;

  g_signal_connect(area, "realize", G_CALLBACK(gtkWebHostRealize), ih);
  g_signal_connect(area, "unrealize", G_CALLBACK(gtkWebHostUnrealize), ih);
  g_signal_connect(area, "size-allocate", G_CALLBACK(gtkWebHostPlace), ih);

  iupgtkAddToParent(ih);
  gtk_widget_realize(area);

  return gtk_widget_get_realized(area) ? gtkWebHostNative(area) : NULL;
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

void iupwebHostUnMap(Ihandle* ih)
{
  if (!ih->handle)
    return;

  g_signal_handlers_disconnect_by_data(ih->handle, ih);
  iupdrvBaseUnMapMethod(ih);
  ih->handle = NULL;
}
