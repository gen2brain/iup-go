/** \file
 * \brief GTK4 Driver iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_singleinstance.h"

#include "iupgtk4_drv.h"


static gboolean gtk4_input_callbacks_enabled = FALSE;
static const char* IUPGTK4_GLOBAL_CONTROLLER = "_IUPGTK4_GLOBAL_CONTROLLER";

static gboolean gtk4GlobalEventCb(GtkEventControllerLegacy* ctrl, GdkEvent* event, gpointer user)
{
  (void)ctrl; (void)user;
  GdkEventType type = gdk_event_get_event_type(event);

  switch (type)
  {
  case GDK_BUTTON_PRESS:
  case GDK_BUTTON_RELEASE:
    {
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (cb)
      {
        guint b = gdk_button_event_get_button(event);
        int button = 0;
        if (b == GDK_BUTTON_PRIMARY) button = IUP_BUTTON1;
        else if (b == GDK_BUTTON_MIDDLE) button = IUP_BUTTON2;
        else if (b == GDK_BUTTON_SECONDARY) button = IUP_BUTTON3;
        else if (b == 4) button = IUP_BUTTON4;
        else if (b == 5) button = IUP_BUTTON5;
        if (button)
        {
          double x, y;
          gdk_event_get_position(event, &x, &y);
          GdkModifierType state = gdk_event_get_modifier_state(event);
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupgtk4ButtonKeySetStatus(state, button, status, 0);
          cb(button, type == GDK_BUTTON_PRESS ? 1 : 0, (int)x, (int)y, status);
        }
      }
      break;
    }
  case GDK_MOTION_NOTIFY:
    {
      IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
      if (cb)
      {
        double x, y;
        gdk_event_get_position(event, &x, &y);
        GdkModifierType state = gdk_event_get_modifier_state(event);
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupgtk4ButtonKeySetStatus(state, 0, status, 0);
        cb((int)x, (int)y, status);
      }
      break;
    }
  case GDK_SCROLL:
    {
      IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
      if (cb)
      {
        double dx = 0, dy = 0;
        gdk_scroll_event_get_deltas(event, &dx, &dy);
        if (dy != 0)
        {
          double x, y;
          gdk_event_get_position(event, &x, &y);
          GdkModifierType state = gdk_event_get_modifier_state(event);
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupgtk4ButtonKeySetStatus(state, 0, status, 0);
          cb((float)-dy, (int)x, (int)y, status);
        }
      }
      break;
    }
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE:
    {
      IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
      if (cb)
      {
        guint keyval = gdk_key_event_get_keyval(event);
        GdkModifierType state = gdk_event_get_modifier_state(event);
        int code = iupgtk4KeyDecode(keyval, state);
        if (code != 0) cb(code, type == GDK_KEY_PRESS ? 1 : 0);
      }
      break;
    }
  default:
    break;
  }
  return FALSE;
}

static void gtk4InstallGlobalCtrl(GtkWidget* widget, gpointer user)
{
  (void)user;
  if (!GTK_IS_WINDOW(widget)) return;
  if (g_object_get_data(G_OBJECT(widget), IUPGTK4_GLOBAL_CONTROLLER)) return;
  GtkEventController* ctrl = gtk_event_controller_legacy_new();
  gtk_event_controller_set_propagation_phase(ctrl, GTK_PHASE_CAPTURE);
  g_signal_connect(ctrl, "event", G_CALLBACK(gtk4GlobalEventCb), NULL);
  gtk_widget_add_controller(widget, ctrl);
  g_object_set_data(G_OBJECT(widget), IUPGTK4_GLOBAL_CONTROLLER, ctrl);
}

static void gtk4RemoveGlobalCtrl(GtkWidget* widget, gpointer user)
{
  (void)user;
  if (!GTK_IS_WINDOW(widget)) return;
  GtkEventController* ctrl = (GtkEventController*)g_object_get_data(G_OBJECT(widget), IUPGTK4_GLOBAL_CONTROLLER);
  if (!ctrl) return;
  gtk_widget_remove_controller(widget, ctrl);
  g_object_set_data(G_OBJECT(widget), IUPGTK4_GLOBAL_CONTROLLER, NULL);
}

/* called from dialog MapMethod so newly-mapped windows pick up the hook */
IUP_DRV_API void iupgtk4InstallGlobalInputController(GtkWidget* window)
{
  if (gtk4_input_callbacks_enabled)
    gtk4InstallGlobalCtrl(window, NULL);
}

static void gtk4ToggleGlobalInput(int enabled)
{
  gtk4_input_callbacks_enabled = enabled ? TRUE : FALSE;
  GListModel* tops = gtk_window_get_toplevels();
  guint n = g_list_model_get_n_items(tops);
  for (guint i = 0; i < n; i++)
  {
    GtkWidget* w = (GtkWidget*)g_list_model_get_item(tops, i);
    if (enabled)
      gtk4InstallGlobalCtrl(w, NULL);
    else
      gtk4RemoveGlobalCtrl(w, NULL);
    g_object_unref(w);
  }
}


IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "GSKRENDERER"))
  {
    GdkDisplay* display = gdk_display_get_default();
    if (display)
      g_object_set_data_full(G_OBJECT(display), "gsk-renderer", g_strdup(value), g_free);
    return 1;
  }
  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    if (iupdrvSingleInstanceSet(value))
      return 0;
    else
      return 1;
  }
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    gtk4ToggleGlobalInput(iupStrBoolean(value));
    return 1;
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
  if (iupStrEqual(name, "GSKRENDERER"))
  {
    GdkDisplay* display = gdk_display_get_default();
    if (display)
    {
      const char* renderer_name = (const char*)g_object_get_data(G_OBJECT(display), "gsk-renderer");
      if (renderer_name)
        return iupStrReturnStr(renderer_name);
    }
    return NULL;
  }
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
      pstr += snprintf(pstr, (n_monitors * 50) - (pstr - str), "%d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
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
  if (iupStrEqual(name, "SANDBOX"))
  {
    if (getenv("FLATPAK_ID"))
      return "FLATPAK";
    if (getenv("SNAP"))
      return "SNAP";
    if (getenv("APPIMAGE"))
      return "APPIMAGE";
    return NULL;
  }
  return NULL;
}
