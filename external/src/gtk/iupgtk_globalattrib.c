/** \file
 * \brief GTK Driver iupdrvSetGlobal
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

#include "iupgtk_drv.h"



static void iGdkEventFunc(GdkEvent *evt, gpointer	data)
{
  switch(evt->type)
  {
  case GDK_BUTTON_PRESS:
  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
  case GDK_BUTTON_RELEASE:
    {
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (cb)
      {
        GdkEventButton* evt_button = (GdkEventButton*)evt;
        gint win_x = 0, win_y = 0;
        int doubleclick = 0, press = 1;
        int b = IUP_BUTTON1+(evt_button->button-1);
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        int x = (int)evt_button->x;
        int y = (int)evt_button->y;

        if (evt_button->type == GDK_BUTTON_RELEASE)
          press = 0;

        if (evt_button->type == GDK_2BUTTON_PRESS)
          doubleclick = 1;

        iupgtkButtonKeySetStatus(evt_button->state, evt_button->button, status, doubleclick);

        gdk_window_get_origin(evt_button->window, &win_x, &win_y);  /* GDK window relative to screen */
        x += win_x;
        y += win_y;

        if (doubleclick)
        {
          /* Must compensate the fact that in GTK there is an extra button press event 
             when occurs a double click, we compensate that completing the event 
             with a button release before the double click. */
          status[5] = ' '; /* clear double click */
          cb(b, 0, x, y, status);  /* release */
          status[5] = 'D'; /* restore double click */
        }

        cb(b, press, x, y, status);
      }
      break;
    }
  case GDK_MOTION_NOTIFY:
    {
      IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
      if (cb)
      {
        GdkEventMotion* evt_motion = (GdkEventMotion*)evt;
        gint win_x = 0, win_y = 0;
        int x = (int)evt_motion->x;
        int y = (int)evt_motion->y;
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

        iupgtkButtonKeySetStatus(evt_motion->state, 0, status, 0);

        if (evt_motion->is_hint)
          iupgtkWindowGetPointer(evt_motion->window, &x, &y, NULL);

        gdk_window_get_origin(evt_motion->window, &win_x, &win_y);  /* GDK window relative to screen */
        x += win_x;
        y += win_y;

        cb(x, y, status);
      }
      break;
    }
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE:
    {
      IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
      if (cb)
      {
        int pressed = (evt->type==GDK_KEY_PRESS)? 1: 0;
        int code = iupgtkKeyDecode((GdkEventKey*)evt);
        if (code != 0)
          cb(code, pressed);
      }
      break;
    }
  default:
    break;
  }

  (void)data;
  gtk_main_do_event(evt);
}

IUP_SDK_API int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
      gdk_event_handler_set(iGdkEventFunc, NULL, NULL);
    else 
      gdk_event_handler_set((GdkEventFunc)gtk_main_do_event, NULL, NULL);
    return 1;
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    iupgtkStrSetUTF8Mode(iupStrBoolean(value));
    return 1;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    iupgtkStrSetUTF8Mode(!iupStrBoolean(value));
    return 0;
  }
  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
#if !GTK_CHECK_VERSION(3, 10, 0)  /* deprecated since 3.10 */
    /* make sure the type is realized */
    g_type_class_unref (g_type_class_ref (GTK_TYPE_IMAGE_MENU_ITEM));
#endif

    if (iupStrBoolean(value))
      g_object_set (gtk_settings_get_default (), "gtk-menu-images", TRUE, NULL);
    else
      g_object_set (gtk_settings_get_default (), "gtk-menu-images", FALSE, NULL);
  }
  return 1;
}

IUP_SDK_API char *iupdrvGetGlobal(const char *name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    GdkScreen *screen = gdk_screen_get_default();
    GdkWindow *root = gdk_screen_get_root_window(gdk_screen_get_default());
    int x = 0;
    int y = 0;
    int w = gdk_screen_get_width(screen);
    int h = gdk_screen_get_height(screen);
    gdk_window_get_root_origin(root, &x, &y);
    return iupStrReturnStrf("%d %d %d %d", x, y, w, h);
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int i;
#if GTK_CHECK_VERSION(3, 22, 0)
    GdkDisplay *display = gdk_display_get_default();
    int monitors_count = gdk_display_get_n_monitors(display);
#else
    GdkScreen *screen = gdk_screen_get_default();
    int monitors_count = gdk_screen_get_n_monitors(screen);
#endif
    char *str = iupStrGetMemory(monitors_count * 50);
    char* pstr = str;
    GdkRectangle rect;

    for (i = 0; i < monitors_count; i++)
    {
#if GTK_CHECK_VERSION(3, 22, 0)
      GdkMonitor* monitor = gdk_display_get_monitor(display, i);
      gdk_monitor_get_geometry(monitor, &rect);
#else
      gdk_screen_get_monitor_geometry(screen, i, &rect);
#endif
      pstr += sprintf(pstr, "%d %d %d %d\n", rect.x, rect.y, rect.width, rect.height);
    }

    return str;
  }
  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
#if GTK_CHECK_VERSION(3, 22, 0)
    GdkDisplay *display = gdk_display_get_default();
    int monitors_count = gdk_display_get_n_monitors(display);
#else
    GdkScreen *screen = gdk_screen_get_default();
    int monitors_count = gdk_screen_get_n_monitors(screen);
#endif
    return iupStrReturnInt(monitors_count);
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    return iupStrReturnBoolean(gdk_visual_get_best_depth() > 8);
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    return iupStrReturnBoolean(iupgtkStrGetUTF8Mode());
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    return iupStrReturnBoolean(!iupgtkStrGetUTF8Mode());
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
    gboolean menu_images;
    g_object_get (gtk_settings_get_default (), "gtk-menu-images", &menu_images, NULL);
    return iupStrReturnBoolean(menu_images);
  }
  return NULL;
}
