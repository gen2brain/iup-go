/** \file
 * \brief Motif System Tray (SNI/StatusNotifierItem)
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus_dlopen.h"
#else
#include <dbus/dbus.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"

#define SNI_WATCHER_BUS_NAME "org.kde.StatusNotifierWatcher"
#define SNI_WATCHER_OBJECT_PATH "/StatusNotifierWatcher"
#define SNI_WATCHER_INTERFACE "org.kde.StatusNotifierWatcher"

#define SNI_ITEM_INTERFACE "org.kde.StatusNotifierItem"
#define SNI_ITEM_OBJECT_PATH "/StatusNotifierItem"

static const char introspection_xml[] =
  "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
  "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
  "<node>\n"
  "  <interface name=\"org.kde.StatusNotifierItem\">\n"
  "    <property name=\"Category\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Id\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Title\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"WindowId\" type=\"i\" access=\"read\"/>\n"
  "    <property name=\"IconName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
  "    <property name=\"OverlayIconName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"OverlayIconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
  "    <property name=\"AttentionIconName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"AttentionIconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
  "    <property name=\"AttentionMovieName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"ToolTip\" type=\"(sa(iiay)ss)\" access=\"read\"/>\n"
  "    <property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/>\n"
  "    <method name=\"Activate\">\n"
  "      <arg name=\"x\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"y\" type=\"i\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"SecondaryActivate\">\n"
  "      <arg name=\"x\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"y\" type=\"i\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"ContextMenu\">\n"
  "      <arg name=\"x\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"y\" type=\"i\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"Scroll\">\n"
  "      <arg name=\"delta\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"orientation\" type=\"s\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <signal name=\"NewTitle\"/>\n"
  "    <signal name=\"NewIcon\"/>\n"
  "    <signal name=\"NewAttentionIcon\"/>\n"
  "    <signal name=\"NewOverlayIcon\"/>\n"
  "    <signal name=\"NewToolTip\"/>\n"
  "    <signal name=\"NewStatus\">\n"
  "      <arg name=\"status\" type=\"s\"/>\n"
  "    </signal>\n"
  "  </interface>\n"
  "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
  "    <method name=\"Introspect\">\n"
  "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
  "    </method>\n"
  "  </interface>\n"
  "</node>\n";

typedef struct _IupmotSNI {
  DBusConnection* connection;
  char* bus_name;
  Ihandle* ih;
  Pixmap icon_pixmap;
  Pixmap icon_mask;
  int icon_width;
  int icon_height;
  char* tooltip;
  int visible;
  struct timeval last_click_time;
  int last_button;
  int cleanup_in_progress;
  int ref_count;
} IupmotSNI;

static int motSNIDoubleClick(IupmotSNI* sni, int button)
{
  struct timeval now;
  long diff_ms;

  gettimeofday(&now, NULL);

  if (sni->last_button == -1 || sni->last_button != button)
  {
    sni->last_button = button;
    sni->last_click_time = now;
    return 0;
  }

  diff_ms = (now.tv_sec - sni->last_click_time.tv_sec) * 1000 +
            (now.tv_usec - sni->last_click_time.tv_usec) / 1000;

  if (diff_ms < 400)
  {
    sni->last_button = -1;
    return 1;
  }

  sni->last_click_time = now;
  return 0;
}

static void motSNIInvokeCallback(IupmotSNI* sni, int button, int pressed, int x, int y)
{
  int dclick = motSNIDoubleClick(sni, button);
  IFniii cb = (IFniii)IupGetCallback(sni->ih, "TRAYCLICK_CB");

  if (cb)
  {
    if (cb(sni->ih, button, pressed, dclick) == IUP_CLOSE)
      IupExitLoop();
  }
}

static void motSNIAppendIconPixmap(DBusMessageIter* iter, IupmotSNI* sni)
{
  DBusMessageIter array_iter, struct_iter, byte_array_iter;

  dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);

  if (sni->icon_pixmap && sni->icon_width > 0 && sni->icon_height > 0)
  {
    XImage* image = XGetImage(iupmot_display, sni->icon_pixmap, 0, 0, sni->icon_width, sni->icon_height, AllPlanes, ZPixmap);
    XImage* mask_image = NULL;

    if (sni->icon_mask)
      mask_image = XGetImage(iupmot_display, sni->icon_mask, 0, 0, sni->icon_width, sni->icon_height, AllPlanes, ZPixmap);

    if (image)
    {
      int x, y;
      dbus_int32_t w = (dbus_int32_t)sni->icon_width;
      dbus_int32_t h = (dbus_int32_t)sni->icon_height;

      dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &w);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &h);

      dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &byte_array_iter);

      for (y = 0; y < sni->icon_height; y++)
      {
        for (x = 0; x < sni->icon_width; x++)
        {
          unsigned long pixel = XGetPixel(image, x, y);
          unsigned char a = 0xff;
          unsigned char r, g, b;

          iupmotColorGetRGB(pixel, &r, &g, &b);

          if (mask_image)
          {
            unsigned long mask_pixel = XGetPixel(mask_image, x, y);
            a = mask_pixel ? 0xff : 0x00;
          }

          dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &a);
          dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &r);
          dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &g);
          dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &b);
        }
      }

      dbus_message_iter_close_container(&struct_iter, &byte_array_iter);
      dbus_message_iter_close_container(&array_iter, &struct_iter);

      XDestroyImage(image);
      if (mask_image)
        XDestroyImage(mask_image);
    }
  }

  dbus_message_iter_close_container(iter, &array_iter);
}

static DBusHandlerResult motSNIMessageHandler(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  IupmotSNI* sni = (IupmotSNI*)user_data;
  const char* interface = dbus_message_get_interface(message);
  const char* member = dbus_message_get_member(message);

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Introspectable", "Introspect"))
  {
    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_xml, DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(interface, SNI_ITEM_INTERFACE) != 0 &&
      strcmp(interface, "org.freedesktop.DBus.Properties") != 0)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "Activate"))
  {
    dbus_int32_t x, y;
    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    motSNIInvokeCallback(sni, 1, 1, x, y);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "SecondaryActivate"))
  {
    dbus_int32_t x, y;
    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    motSNIInvokeCallback(sni, 2, 1, x, y);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "ContextMenu"))
  {
    dbus_int32_t x, y;
    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    motSNIInvokeCallback(sni, 3, 1, x, y);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "Scroll"))
  {
    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "Get"))
  {
    const char* prop_interface;
    const char* prop_name;
    DBusMessage* reply;
    DBusMessageIter iter, variant_iter;

    dbus_message_get_args(message, NULL,
                         DBUS_TYPE_STRING, &prop_interface,
                         DBUS_TYPE_STRING, &prop_name,
                         DBUS_TYPE_INVALID);

    if (strcmp(prop_interface, SNI_ITEM_INTERFACE) != 0)
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    if (strcmp(prop_name, "Category") == 0)
    {
      const char* value = "ApplicationStatus";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Id") == 0)
    {
      const char* value = sni->bus_name;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Title") == 0)
    {
      const char* value = "IUP Application";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Status") == 0)
    {
      const char* value = sni->visible ? "Active" : "Passive";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "WindowId") == 0)
    {
      dbus_int32_t value = 0;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "i", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "IconName") == 0)
    {
      const char* value = "";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "IconPixmap") == 0)
    {
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      motSNIAppendIconPixmap(&variant_iter, sni);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "OverlayIconName") == 0 || strcmp(prop_name, "AttentionIconName") == 0 || strcmp(prop_name, "AttentionMovieName") == 0)
    {
      const char* value = "";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "OverlayIconPixmap") == 0 || strcmp(prop_name, "AttentionIconPixmap") == 0)
    {
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "ToolTip") == 0)
    {
      DBusMessageIter struct_iter, array_iter;
      const char* tooltip_text = sni->tooltip ? sni->tooltip : "";
      const char* empty = "";

      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "(sa(iiay)ss)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&struct_iter, &array_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &tooltip_text);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_close_container(&variant_iter, &struct_iter);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "ItemIsMenu") == 0)
    {
      dbus_bool_t value = FALSE;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "GetAll"))
  {
    const char* prop_interface;
    DBusMessage* reply;
    DBusMessageIter iter, dict_iter, dict_entry_iter, variant_iter;

    dbus_message_get_args(message, NULL,
                         DBUS_TYPE_STRING, &prop_interface,
                         DBUS_TYPE_INVALID);

    if (strcmp(prop_interface, SNI_ITEM_INTERFACE) != 0)
    {
      reply = dbus_message_new_method_return(message);
      dbus_message_iter_init_append(reply, &iter);
      dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
      dbus_message_iter_close_container(&iter, &dict_iter);
      dbus_connection_send(connection, reply, NULL);
      dbus_message_unref(reply);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

    {
      const char* prop_name = "Category";
      const char* value = "ApplicationStatus";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Id";
      const char* value = sni->bus_name;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Title";
      const char* value = "IUP Application";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Status";
      const char* value = sni->visible ? "Active" : "Passive";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "WindowId";
      dbus_int32_t value = 0;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "i", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "IconName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "IconPixmap";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      motSNIAppendIconPixmap(&variant_iter, sni);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "OverlayIconName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "OverlayIconPixmap";
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "AttentionIconName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "AttentionIconPixmap";
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "AttentionMovieName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "ToolTip";
      DBusMessageIter struct_iter, array_iter;
      const char* tooltip_text = sni->tooltip ? sni->tooltip : "";
      const char* empty = "";

      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "(sa(iiay)ss)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&struct_iter, &array_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &tooltip_text);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_close_container(&variant_iter, &struct_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "ItemIsMenu";
      dbus_bool_t value = FALSE;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    dbus_message_iter_close_container(&iter, &dict_iter);

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void motSNIRegisterToWatcher(IupmotSNI* sni)
{
  DBusMessage* msg;
  DBusMessage* reply;
  DBusError error;

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(SNI_WATCHER_BUS_NAME, SNI_WATCHER_OBJECT_PATH, SNI_WATCHER_INTERFACE, "RegisterStatusNotifierItem");

  if (!msg)
    return;

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &sni->bus_name, DBUS_TYPE_INVALID);

  reply = dbus_connection_send_with_reply_and_block(sni->connection, msg, 1000, &error);

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  if (reply)
    dbus_message_unref(reply);

  dbus_message_unref(msg);

  dbus_connection_flush(sni->connection);
}

static void motSNIEmitSignal(IupmotSNI* sni, const char* signal_name)
{
  DBusMessage* signal = dbus_message_new_signal(SNI_ITEM_OBJECT_PATH, SNI_ITEM_INTERFACE, signal_name);

  if (signal)
  {
    dbus_connection_send(sni->connection, signal, NULL);
    dbus_connection_flush(sni->connection);
    dbus_message_unref(signal);
  }
}

static void motSNIProcessDBus(XtPointer client_data, XtIntervalId* id)
{
  IupmotSNI* sni = (IupmotSNI*)client_data;
  DBusConnection* conn;
  Ihandle* ih;

  if (!sni)
    return;

  sni->ref_count++;

  conn = sni->connection;
  ih = sni->ih;

  if (!ih || !iupObjectCheck(ih) || sni->cleanup_in_progress || !conn)
  {
    sni->ref_count--;
    return;
  }

  if ((IupmotSNI*)iupAttribGet(ih, "_IUPMOT_SNI") != sni)
  {
    sni->ref_count--;
    return;
  }

  /* Non-blocking pump to service incoming method/property calls */
  dbus_connection_read_write(conn, 0);

  if (!sni->cleanup_in_progress && sni->connection == conn)
  {
    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS)
    {
      if (sni->cleanup_in_progress || sni->connection != conn)
        break;
      dbus_connection_dispatch(conn);
    }
  }

  /* Reschedule only if cleanup is not in progress and handle is still valid */
  if (!sni->cleanup_in_progress && ih && iupObjectCheck(ih) && sni->connection)
  {
    XtIntervalId tid = XtAppAddTimeOut(iupmot_appcontext, 50, motSNIProcessDBus, client_data);
    if (ih)
      iupAttribSet(ih, "_IUPMOT_SNI_TIMER", (char*)tid);
  }

  sni->ref_count--;
  if (sni->ref_count == 0 && sni->cleanup_in_progress)
  {
    if (sni->connection)
    {
      dbus_connection_close(sni->connection);
      dbus_connection_unref(sni->connection);
    }

    if (sni->icon_pixmap)
      XFreePixmap(iupmot_display, sni->icon_pixmap);

    if (sni->icon_mask)
      XFreePixmap(iupmot_display, sni->icon_mask);

    if (sni->bus_name)
      free(sni->bus_name);

    if (sni->tooltip)
      free(sni->tooltip);

    free(sni);
  }

  (void)id;
}

static IupmotSNI* motGetSNI(Ihandle* ih)
{
  IupmotSNI* sni = (IupmotSNI*)iupAttribGet(ih, "_IUPMOT_SNI");

  if (!sni)
  {
    DBusError error;
    static unsigned int counter = 0;
    int result;
    DBusObjectPathVTable vtable = { NULL, motSNIMessageHandler, NULL, NULL, NULL, NULL };

#ifdef IUPDBUS_USE_DLOPEN
    if (!iupDBusOpen())
      return NULL;
#endif

    dbus_error_init(&error);

    sni = (IupmotSNI*)calloc(1, sizeof(IupmotSNI));
    sni->ih = ih;
    sni->visible = 0;
    sni->last_button = -1;

    sni->connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error))
    {
      dbus_error_free(&error);
      free(sni);
      return NULL;
    }

    sni->bus_name = (char*)malloc(64);
    snprintf(sni->bus_name, 64, "org.kde.StatusNotifierItem-%u-%u", getpid(), counter++);

    result = dbus_bus_request_name(sni->connection, sni->bus_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &error);

    if (dbus_error_is_set(&error) || result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      if (dbus_error_is_set(&error))
        dbus_error_free(&error);
      dbus_connection_unref(sni->connection);
      free(sni->bus_name);
      free(sni);
      return NULL;
    }

    if (!dbus_connection_register_object_path(sni->connection, SNI_ITEM_OBJECT_PATH, &vtable, sni))
    {
      dbus_bus_release_name(sni->connection, sni->bus_name, NULL);
      dbus_connection_unref(sni->connection);
      free(sni->bus_name);
      free(sni);
      return NULL;
    }

    motSNIRegisterToWatcher(sni);

    /* Start a small periodic dispatcher to service libdbus on the Xt loop */
    {
      XtIntervalId tid = XtAppAddTimeOut(iupmot_appcontext, 50, motSNIProcessDBus, sni);
      iupAttribSet(ih, "_IUPMOT_SNI_TIMER", (char*)tid);
    }

    iupAttribSet(ih, "_IUPMOT_SNI", (char*)sni);
  }

  return sni;
}

int iupmotSetTrayAttrib(Ihandle* ih, const char* value)
{
  IupmotSNI* sni = motGetSNI(ih);
  if (!sni)
    return 0;

  sni->visible = iupStrBoolean(value);
  motSNIEmitSignal(sni, "NewStatus");

  return 1;
}

int iupmotSetTrayTipAttrib(Ihandle* ih, const char* value)
{
  IupmotSNI* sni = motGetSNI(ih);
  if (!sni)
    return 0;

  if (sni->tooltip)
    free(sni->tooltip);

  sni->tooltip = value ? strdup(value) : NULL;
  motSNIEmitSignal(sni, "NewToolTip");

  return 1;
}

int iupmotSetTrayImageAttrib(Ihandle* ih, const char* value)
{
  IupmotSNI* sni = motGetSNI(ih);
  Pixmap icon, mask;
  Window root;
  int x, y;
  unsigned int width, height, border, depth;

  if (!sni)
    return 0;

  if (sni->icon_pixmap)
  {
    XFreePixmap(iupmot_display, sni->icon_pixmap);
    sni->icon_pixmap = 0;
  }
  if (sni->icon_mask)
  {
    XFreePixmap(iupmot_display, sni->icon_mask);
    sni->icon_mask = 0;
  }

  sni->icon_width = 0;
  sni->icon_height = 0;

  icon = (Pixmap)iupImageGetIcon(value);
  mask = iupmotImageGetMask(value);

  if (icon && XGetGeometry(iupmot_display, icon, &root, &x, &y, &width, &height, &border, &depth))
  {
    GC gc;
    int screen_depth = DefaultDepth(iupmot_display, iupmot_screen);

    sni->icon_pixmap = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen), width, height, screen_depth);

    if (sni->icon_pixmap)
    {
      gc = XCreateGC(iupmot_display, sni->icon_pixmap, 0, NULL);

      if (depth == screen_depth)
      {
        XCopyArea(iupmot_display, icon, sni->icon_pixmap, gc, 0, 0, width, height, 0, 0);
      }
      else
      {
        XImage* src_image = XGetImage(iupmot_display, icon, 0, 0, width, height, AllPlanes, ZPixmap);
        if (src_image)
        {
          int px, py;
          for (py = 0; py < (int)height; py++)
          {
            for (px = 0; px < (int)width; px++)
            {
              unsigned long pixel = XGetPixel(src_image, px, py);
              XSetForeground(iupmot_display, gc, pixel);
              XDrawPoint(iupmot_display, sni->icon_pixmap, gc, px, py);
            }
          }
          XDestroyImage(src_image);
        }
      }

      XFreeGC(iupmot_display, gc);

      sni->icon_width = (int)width;
      sni->icon_height = (int)height;
    }

    if (mask)
    {
      sni->icon_mask = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen), width, height, 1);
      if (sni->icon_mask)
      {
        GC mask_gc = XCreateGC(iupmot_display, sni->icon_mask, 0, NULL);
        XCopyArea(iupmot_display, mask, sni->icon_mask, mask_gc, 0, 0, width, height, 0, 0);
        XFreeGC(iupmot_display, mask_gc);
      }
    }
  }

  motSNIEmitSignal(sni, "NewIcon");

  return 1;
}

void iupmotTrayCleanup(Ihandle* ih)
{
  IupmotSNI* sni = (IupmotSNI*)iupAttribGet(ih, "_IUPMOT_SNI");

  if (sni)
  {
    iupAttribSet(ih, "_IUPMOT_SNI", NULL);

    sni->cleanup_in_progress = 1;
    sni->ih = NULL;

    {
      XtIntervalId tid = (XtIntervalId)iupAttribGet(ih, "_IUPMOT_SNI_TIMER");
      if (tid)
      {
        XtRemoveTimeOut(tid);
        iupAttribSet(ih, "_IUPMOT_SNI_TIMER", NULL);
      }
    }

    if (sni->connection)
    {
      dbus_connection_flush(sni->connection);
      dbus_connection_unregister_object_path(sni->connection, SNI_ITEM_OBJECT_PATH);

      if (sni->bus_name)
        dbus_bus_release_name(sni->connection, sni->bus_name, NULL);
    }

    if (sni->ref_count == 0)
    {
      if (sni->connection)
      {
        dbus_connection_close(sni->connection);
        dbus_connection_unref(sni->connection);
      }

      if (sni->icon_pixmap)
        XFreePixmap(iupmot_display, sni->icon_pixmap);

      if (sni->icon_mask)
        XFreePixmap(iupmot_display, sni->icon_mask);

      if (sni->bus_name)
        free(sni->bus_name);

      if (sni->tooltip)
        free(sni->tooltip);

      free(sni);

#ifdef IUPDBUS_USE_DLOPEN
      iupDBusClose();
#endif
    }
  }
}
