/** \file
 * \brief GTK4 System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <limits.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "iup.h"
#include "iup_export.h"
#include "iup_object.h"
#include "iup_drvinfo.h"

#include "iupgtk4_x11.h"
#include "iupgtk4_drv.h"

#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32.h>
#endif
#ifdef GDK_WINDOWING_MACOS
#include <gdk/macos/gdkmacos.h>
#endif


static GdkMonitor* gtk4GetMainMonitor(void)
{
  GListModel* monitors = gdk_display_get_monitors(gdk_display_get_default());

  if (g_list_model_get_n_items(monitors) > 0)
    return g_list_model_get_item(monitors, 0);
  return NULL;
}

static void gtk4GetMonitorWorkarea(GdkMonitor* monitor, GdkRectangle* workarea)
{
  gdk_monitor_get_geometry(monitor, workarea);

#ifdef GDK_WINDOWING_X11
  if (iupgtk4X11GetMonitorWorkarea(monitor, workarea))
    return;
#endif
#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_MONITOR(monitor))
  {
    gdk_win32_monitor_get_workarea(monitor, workarea);
    return;
  }
#endif
#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_MONITOR(monitor))
    gdk_macos_monitor_get_workarea(monitor, workarea);
#endif
}

IUP_SDK_API void iupdrvAddScreenOffset(int* x, int* y, int add)
{
  GdkMonitor* monitor;
  GdkRectangle geometry, workarea;
  int dx, dy;

  if (add == 0)
    return;

  monitor = gtk4GetMainMonitor();
  if (!monitor)
    return;

  gdk_monitor_get_geometry(monitor, &geometry);
  gtk4GetMonitorWorkarea(monitor, &workarea);
  g_object_unref(monitor);

  dx = workarea.x - geometry.x;
  dy = workarea.y - geometry.y;
  if (add < 0)
  {
    dx = -dx;
    dy = -dy;
  }

  if (x) *x += dx;
  if (y) *y += dy;
}

IUP_SDK_API void iupdrvGetScreenSize(int* width, int* height)
{
  GdkMonitor* monitor = gtk4GetMainMonitor();

  if (monitor)
  {
    GdkRectangle rect;
    gtk4GetMonitorWorkarea(monitor, &rect);
    *width = rect.width;
    *height = rect.height;
    g_object_unref(monitor);
  }
}

IUP_SDK_API void iupdrvGetFullSize(int* width, int* height)
{
  GdkDisplay* display = gdk_display_get_default();
  GListModel* monitors = gdk_display_get_monitors(display);
  GdkMonitor* monitor = NULL;

  /* gdk_monitor_is_primary removed, just use first monitor */
  if (g_list_model_get_n_items(monitors) > 0)
    monitor = g_list_model_get_item(monitors, 0);

  if (monitor)
  {
    GdkRectangle rect;
    gdk_monitor_get_geometry(monitor, &rect);
    *width = rect.width;
    *height = rect.height;
    g_object_unref(monitor);
  }
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  return 32;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  GdkDisplay* display = gdk_display_get_default();
  GListModel* monitors = gdk_display_get_monitors(display);
  GdkMonitor* monitor = g_list_model_get_item(monitors, 0);

  if (monitor)
  {
    /* GTK4 doesn't provide direct DPI access. Use 96 as default and apply monitor scale factor */
    double dpi = 96.0;
    int scale_factor = gdk_monitor_get_scale_factor(monitor);
    g_object_unref(monitor);
    return dpi * scale_factor;
  }

  return 96.0;
}

IUP_SDK_API double iupdrvGetScreenScale(void)
{
  GListModel* monitors = gdk_display_get_monitors(gdk_display_get_default());
  GdkMonitor* monitor = g_list_model_get_item(monitors, 0);
  double scale = 1;

  if (monitor)
  {
#if GTK_CHECK_VERSION(4, 14, 0)
    scale = gdk_monitor_get_scale(monitor);
#else
    scale = gdk_monitor_get_scale_factor(monitor);
#endif
    g_object_unref(monitor);
  }

  return scale > 0 ? scale : 1;
}

IUP_SDK_API double iupdrvGetDevicePixelRatio(void)
{
  return iupdrvGetScreenScale();
}

IUP_SDK_API void iupdrvGetCursorPos(int* x, int* y)
{
#ifdef GDK_WINDOWING_X11
  if (iupgtk4X11IsBackend() && iupgtk4X11QueryPointer(x, y))
  {
    iupdrvAddScreenOffset(x, y, -1);
    return;
  }
#endif

  {
    GdkDisplay* display = gdk_display_get_default();
    GdkSeat* seat = gdk_display_get_default_seat(display);
    GdkDevice* device = gdk_seat_get_pointer(seat);

    if (device)
    {
      GdkSurface* surface = gdk_device_get_surface_at_position(device, NULL, NULL);

      if (surface)
      {
        double dx, dy;
        GtkNative* native = gtk_native_get_for_surface(surface);
        gdk_surface_get_device_position(surface, device, &dx, &dy, NULL);
        if (native)
        {
          double native_x, native_y;
          graphene_point_t src_point, dest_point;
          gtk_native_get_surface_transform(native, &native_x, &native_y);
          src_point.x = (float)(dx - native_x);
          src_point.y = (float)(dy - native_y);
          if (gtk_widget_compute_point(GTK_WIDGET(native), iupgtk4NativeGetContent(native), &src_point, &dest_point))
          {
            dx = dest_point.x;
            dy = dest_point.y;
          }
        }
        *x = (int)dx;
        *y = (int)dy;
      }
      else
      {
        *x = 0;
        *y = 0;
      }
    }
  }
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  GdkDisplay* display = gdk_display_get_default();
  GdkSeat* seat = gdk_display_get_default_seat(display);
  GdkDevice* device = gdk_seat_get_keyboard(seat);
  GdkModifierType aModifierType = 0;

  /* Get modifier state directly - keyboard device doesn't have surface position */
  if (device)
    aModifierType = gdk_device_get_modifier_state(device);

  if (aModifierType & GDK_SHIFT_MASK)
    key[0] = 'S';
  else
    key[0] = ' ';

  if (aModifierType & GDK_CONTROL_MASK)
    key[1] = 'C';
  else
    key[1] = ' ';

  if ((aModifierType & GDK_ALT_MASK))
    key[2] = 'A';
  else
    key[2] = ' ';

  if (aModifierType & GDK_SUPER_MASK)
    key[3] = 'Y';
  else
    key[3] = ' ';

  key[4] = 0;
}

