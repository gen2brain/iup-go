/** \file
 * \brief UNIX System Information
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

/* From GDK docs: nowadays it is more common to have a single GdkScreen which combines several physical monitors */

#if 0 /* this is the entire screen area */
GdkScreen* screen = gdk_screen_get_default();
*width = gdk_screen_get_width(screen);
*height = gdk_screen_get_height(screen);
#endif

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  /* This function should return the primary monitor's work area */
#if GTK_CHECK_VERSION(3, 22, 0)
  GdkDisplay *display = gdk_display_get_default();
  GdkMonitor* monitor = gdk_display_get_primary_monitor(display);
  if (!monitor)
    monitor = gdk_display_get_monitor(display, 0);
  GdkRectangle rect;
  gdk_monitor_get_workarea(monitor, &rect);
  *width = rect.width;
  *height = rect.height;
#else
  GdkScreen* screen = gdk_screen_get_default();
  GdkRectangle rect;
#if GTK_CHECK_VERSION(2, 20, 0)
  gint monitor = gdk_screen_get_primary_monitor(screen);
#else
  gint monitor = gdk_screen_get_monitor_at_point(screen, 0, 0);
#endif
  gdk_screen_get_monitor_workarea(screen, monitor, &rect);
  *width = rect.width;
  *height = rect.height;
#endif
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  GdkScreen* screen = gdk_screen_get_default();
  GdkRectangle rect;
#if GTK_CHECK_VERSION(2, 20, 0)
  gint monitor = gdk_screen_get_primary_monitor(screen);
#else
  gint monitor = gdk_screen_get_monitor_at_point(screen, 0, 0);
#endif
  gdk_screen_get_monitor_geometry(screen, monitor, &rect);
  *width = rect.width;
  *height = rect.height;
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  GdkVisual* visual = gdk_visual_get_system();
#if GTK_CHECK_VERSION(2, 22, 0)
  return gdk_visual_get_depth(visual);
#else
  return visual->depth;
#endif
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  return gdk_screen_get_resolution(gdk_screen_get_default());
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDeviceManager* device_manager = gdk_display_get_device_manager(gdk_display_get_default());
  GdkDevice* device = gdk_device_manager_get_client_pointer(device_manager);
  gdk_device_get_position(device, NULL, x, y);
#else
  gdk_display_get_pointer(gdk_display_get_default(), NULL, x, y, NULL);
#endif
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  GdkModifierType aModifierType = 0;

#if GTK_CHECK_VERSION(3, 4, 0)
  GdkKeymap* keymap = gdk_keymap_get_default();
  if (keymap)
    aModifierType = gdk_keymap_get_modifier_state(keymap);
#else
  /* Fallback for GTK2 and GTK < 3.4 */
  /* gdk_display_get_pointer is deprecated in GTK3 but is the only option here */
  gdk_display_get_pointer(gdk_display_get_default(), NULL, NULL, NULL, &aModifierType);
#endif

  if (aModifierType & GDK_SHIFT_MASK)
    key[0] = 'S';
  else
    key[0] = ' ';

  if (aModifierType & GDK_CONTROL_MASK)
    key[1] = 'C';
  else
    key[1] = ' ';

  if ((aModifierType & GDK_MOD1_MASK) || (aModifierType & GDK_MOD5_MASK))
    key[2] = 'A';
  else
    key[2] = ' ';

  if (aModifierType & GDK_MOD4_MASK)
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
