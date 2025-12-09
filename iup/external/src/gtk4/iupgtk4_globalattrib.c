/** \file
 * \brief GTK4 Driver iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"

#include "iupgtk4_drv.h"


IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    /* Global event handler mechanism removed. INPUTCALLBACKS not supported in GTK4 */
    return 0;
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    /* GTK4 always uses UTF-8, no conversion needed */
    return 1;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    /* GTK4 always uses UTF-8, no conversion needed */
    return 0;
  }
  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    /* gtk-enable-mnemonics property was removed in GTK4. Mnemonics are always enabled in GTK4 */
    (void)value;
    return 1;
  }
  return 1;
}

IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    GdkDisplay* display = gdk_display_get_default();
    GListModel* monitors = gdk_display_get_monitors(display);
    guint n_monitors = g_list_model_get_n_items(monitors);

    if (n_monitors > 0)
    {
      GdkMonitor* monitor = g_list_model_get_item(monitors, 0);
      GdkRectangle rect;
      gdk_monitor_get_geometry(monitor, &rect);
      g_object_unref(monitor);

      /* For multi-monitor setup, calculate bounding box */
      int min_x = rect.x;
      int min_y = rect.y;
      int max_x = rect.x + rect.width;
      int max_y = rect.y + rect.height;

      guint i;
      for (i = 1; i < n_monitors; i++)
      {
        monitor = g_list_model_get_item(monitors, i);
        gdk_monitor_get_geometry(monitor, &rect);

        if (rect.x < min_x) min_x = rect.x;
        if (rect.y < min_y) min_y = rect.y;
        if (rect.x + rect.width > max_x) max_x = rect.x + rect.width;
        if (rect.y + rect.height > max_y) max_y = rect.y + rect.height;

        g_object_unref(monitor);
      }

      return iupStrReturnStrf("%d %d %d %d", min_x, min_y, max_x - min_x, max_y - min_y);
    }

    return iupStrReturnStrf("0 0 800 600");
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    GdkDisplay* display = gdk_display_get_default();
    GListModel* monitors = gdk_display_get_monitors(display);
    guint n_monitors = g_list_model_get_n_items(monitors);
    char* str = iupStrGetMemory((int)n_monitors * 50);
    char* pstr = str;
    guint i;

    for (i = 0; i < n_monitors; i++)
    {
      GdkMonitor* monitor = g_list_model_get_item(monitors, i);
      GdkRectangle rect;
      gdk_monitor_get_geometry(monitor, &rect);
      pstr += sprintf(pstr, "%d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
      g_object_unref(monitor);
    }

    return str;
  }
  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
    GdkDisplay* display = gdk_display_get_default();
    GListModel* monitors = gdk_display_get_monitors(display);
    int monitors_count = (int)g_list_model_get_n_items(monitors);
    return iupStrReturnInt(monitors_count);
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    /* Always true color */
    return iupStrReturnBoolean(1);
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    /* Always UTF-8 */
    return iupStrReturnBoolean(1);
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    /* No conversion needed */
    return iupStrReturnBoolean(0);
  }
#ifndef WIN32
  if (iupStrEqual(name, "EXEFILENAME"))
  {
    char* argv0 = IupGetGlobal("ARGV0");
    if (argv0)
    {
      char* exefilename = realpath(argv0, NULL);
      if (exefilename)
      {
        char* str = iupStrReturnStr(exefilename);
        free(exefilename);
        return str;
      }
    }
  }
#endif
  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    return iupStrReturnBoolean(TRUE);
  }
  if (iupStrEqual(name, "DARKMODE"))
  {
    return iupStrReturnBoolean(iupgtk4IsSystemDarkMode());
  }
  if (iupStrEqual(name, "OVERLAYSCROLLBAR"))
  {
    gboolean overlay_scrolling;
    g_object_get(gtk_settings_get_default(), "gtk-overlay-scrolling", &overlay_scrolling, NULL);
    return iupStrReturnBoolean(overlay_scrolling);
  }
  return NULL;
}
