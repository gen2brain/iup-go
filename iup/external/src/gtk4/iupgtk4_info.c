/** \file
 * \brief GTK4 System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "iup_export.h"
#include "iup_str.h"
#include "iup_drvinfo.h"


IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  GdkDisplay *display = gdk_display_get_default();
  GListModel *monitors = gdk_display_get_monitors(display);
  GdkMonitor *monitor = NULL;

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

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  GdkDisplay *display = gdk_display_get_default();
  GListModel *monitors = gdk_display_get_monitors(display);
  GdkMonitor *monitor = NULL;

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
  GdkDisplay *display = gdk_display_get_default();
  GListModel *monitors = gdk_display_get_monitors(display);
  GdkMonitor *monitor = g_list_model_get_item(monitors, 0);

  if (monitor)
  {
    /* GTK4 doesn't provide direct DPI access. Use 96 as default and apply monitor scale factor */
    double dpi = 96.0;
    int scale_factor = gdk_monitor_get_scale_factor(monitor);
    g_object_unref(monitor);
    return dpi * scale_factor;
  }

  return 96.0;  /* Default DPI */
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkSeat *seat = gdk_display_get_default_seat(display);
  GdkDevice *device = gdk_seat_get_pointer(seat);

  if (device)
  {
    GdkSurface *surface = gdk_display_get_default_seat(display) ?
                          gdk_device_get_surface_at_position(device, NULL, NULL) : NULL;

    if (surface)
    {
      double dx, dy;
      gdk_surface_get_device_position(surface, device, &dx, &dy, NULL);
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

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  GdkDisplay *display = gdk_display_get_default();
  GdkSeat *seat = gdk_display_get_default_seat(display);
  GdkDevice *device = gdk_seat_get_keyboard(seat);
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

IUP_SDK_API char *iupdrvGetComputerName(void)
{
  return (char*)g_get_host_name();
}

IUP_SDK_API char *iupdrvGetUserName(void)
{
  return (char*)g_get_user_name();
}
