/** \file
 * \brief Unix Desktop Notifications - Shared Implementation
 *
 * This file provides a unified XDG Desktop Notifications implementation
 * using D-Bus for Unix drivers (GTK2, GTK3, GTK4, Qt, Motif).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus_dlopen.h"
#else
#include <dbus/dbus.h>
#endif

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_notify.h"
#include "iup_tray.h"
#include "iup_image.h"

#define IUPUNIX_NOTIFY_MAX_ICON_SIZE 128

#define NOTIFY_BUS_NAME "org.freedesktop.Notifications"
#define NOTIFY_OBJECT_PATH "/org/freedesktop/Notifications"
#define NOTIFY_INTERFACE "org.freedesktop.Notifications"

typedef struct _IupUnixNotify {
  DBusConnection* connection;
  Ihandle* ih;
  dbus_uint32_t notification_id;
  Ihandle* timer;
  int cleanup_in_progress;
  int has_actions;
  int has_persistence;
} IupUnixNotify;

/****************************************************************************
 * D-Bus Timer Dispatch
 ****************************************************************************/

static int unixNotifyTimerCallback(Ihandle* timer)
{
  IupUnixNotify* notify = (IupUnixNotify*)iupAttribGet(timer, "_IUPUNIX_NOTIFY_DATA");
  DBusConnection* conn;

  if (!notify || !notify->connection || notify->cleanup_in_progress)
    return IUP_DEFAULT;

  conn = notify->connection;

  dbus_connection_read_write(conn, 0);

  if (!notify->cleanup_in_progress && notify->connection == conn)
  {
    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS)
    {
      if (notify->cleanup_in_progress || notify->connection != conn)
        break;
      dbus_connection_dispatch(conn);
    }
  }

  return IUP_DEFAULT;
}

/****************************************************************************
 * D-Bus Signal Handler
 ****************************************************************************/

static DBusHandlerResult unixNotifySignalFilter(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  IupUnixNotify* notify = (IupUnixNotify*)user_data;

  (void)connection;

  if (!notify || !notify->ih || notify->cleanup_in_progress)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_is_signal(message, NOTIFY_INTERFACE, "NotificationClosed"))
  {
    dbus_uint32_t id, reason;

    if (dbus_message_get_args(message, NULL,
        DBUS_TYPE_UINT32, &id,
        DBUS_TYPE_UINT32, &reason,
        DBUS_TYPE_INVALID))
    {
      if (id == notify->notification_id)
      {
        IFni close_cb = (IFni)IupGetCallback(notify->ih, "CLOSE_CB");
        if (close_cb)
        {
          int ret = close_cb(notify->ih, (int)reason);
          if (ret == IUP_CLOSE)
            IupExitLoop();
        }
        notify->notification_id = 0;
        return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_signal(message, NOTIFY_INTERFACE, "ActionInvoked"))
  {
    dbus_uint32_t id;
    const char* action_key = NULL;

    if (dbus_message_get_args(message, NULL, DBUS_TYPE_UINT32, &id, DBUS_TYPE_STRING, &action_key, DBUS_TYPE_INVALID))
    {
      if (id == notify->notification_id && action_key)
      {
        IFni notify_cb = (IFni)IupGetCallback(notify->ih, "NOTIFY_CB");
        if (notify_cb)
        {
          int action_id = 0;

          if (strcmp(action_key, "default") == 0)
            action_id = 0;
          else if (strcmp(action_key, "action1") == 0)
            action_id = 1;
          else if (strcmp(action_key, "action2") == 0)
            action_id = 2;
          else if (strcmp(action_key, "action3") == 0)
            action_id = 3;
          else if (strcmp(action_key, "action4") == 0)
            action_id = 4;

          int ret = notify_cb(notify->ih, action_id);
          if (ret == IUP_CLOSE)
            IupExitLoop();
        }
        return DBUS_HANDLER_RESULT_HANDLED;
      }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/****************************************************************************
 * Server Capabilities
 ****************************************************************************/

static int unixNotifyHasCapability(DBusConnection* connection, const char* capability)
{
  DBusMessage* msg;
  DBusMessage* reply;
  DBusError error;
  DBusMessageIter iter, array_iter;
  int has_capability = 0;

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(NOTIFY_BUS_NAME, NOTIFY_OBJECT_PATH, NOTIFY_INTERFACE, "GetCapabilities");

  if (!msg)
    return 0;

  reply = dbus_connection_send_with_reply_and_block(connection, msg, 2000, &error);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    return 0;
  }

  if (dbus_message_iter_init(reply, &iter) && dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY)
  {
    dbus_message_iter_recurse(&iter, &array_iter);
    while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRING)
    {
      const char* cap = NULL;
      dbus_message_iter_get_basic(&array_iter, &cap);
      if (cap && strcmp(cap, capability) == 0)
      {
        has_capability = 1;
        break;
      }
      dbus_message_iter_next(&array_iter);
    }
  }

  dbus_message_unref(reply);
  return has_capability;
}

/****************************************************************************
 * D-Bus Connection Setup
 ****************************************************************************/

static IupUnixNotify* unixNotifyCreate(Ihandle* ih)
{
  IupUnixNotify* notify;
  DBusError error;
  DBusConnection* connection;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return NULL;
#endif

  dbus_error_init(&error);

  connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (!connection || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    return NULL;
  }

  notify = (IupUnixNotify*)calloc(1, sizeof(IupUnixNotify));
  if (!notify)
  {
    dbus_connection_unref(connection);
    return NULL;
  }

  notify->connection = connection;
  notify->ih = ih;
  notify->notification_id = 0;
  notify->cleanup_in_progress = 0;
  notify->has_actions = unixNotifyHasCapability(connection, "actions");
  notify->has_persistence = unixNotifyHasCapability(connection, "persistence");

  dbus_connection_add_filter(connection, unixNotifySignalFilter, notify, NULL);

  dbus_bus_add_match(connection,
      "type='signal',"
      "interface='" NOTIFY_INTERFACE "',"
      "path='" NOTIFY_OBJECT_PATH "',"
      "member='ActionInvoked'",
      &error);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  dbus_bus_add_match(connection,
      "type='signal',"
      "interface='" NOTIFY_INTERFACE "',"
      "path='" NOTIFY_OBJECT_PATH "',"
      "member='NotificationClosed'",
      &error);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  notify->timer = IupTimer();
  iupAttribSet(notify->timer, "_IUPUNIX_NOTIFY_DATA", (char*)notify);
  IupSetCallback(notify->timer, "ACTION_CB", (Icallback)unixNotifyTimerCallback);
  IupSetAttribute(notify->timer, "TIME", "50");
  IupSetAttribute(notify->timer, "RUN", "YES");

  iupAttribSet(ih, "_IUPUNIX_NOTIFY", (char*)notify);

  return notify;
}

static void unixNotifyDestroy(IupUnixNotify* notify)
{
  if (!notify)
    return;

  notify->cleanup_in_progress = 1;

  if (notify->timer)
  {
    IupSetAttribute(notify->timer, "RUN", "NO");
    IupDestroy(notify->timer);
    notify->timer = NULL;
  }

  if (notify->connection)
  {
    dbus_connection_remove_filter(notify->connection, unixNotifySignalFilter, notify);
    dbus_connection_unref(notify->connection);
    notify->connection = NULL;
  }

  free(notify);
}

/****************************************************************************
 * Send Notification
 ****************************************************************************/

static int unixNotifySend(IupUnixNotify* notify)
{
  DBusMessage* msg;
  DBusMessage* reply;
  DBusMessageIter iter, array_iter, dict_iter;
  DBusError error;
  Ihandle* ih = notify->ih;

  const char* app_name = IupGetGlobal("APPNAME");
  const char* title = IupGetAttribute(ih, "TITLE");
  const char* body = IupGetAttribute(ih, "BODY");
  const char* icon = IupGetAttribute(ih, "ICON");
  const char* appicon = IupGetAttribute(ih, "APPICON");
  const char* app_icon_param = "";
  int timeout = IupGetInt(ih, "TIMEOUT");
  int use_image_data = 0;

  if (!app_name) app_name = "";
  if (!title) title = "";
  if (!body) body = "";
  if (!icon) icon = appicon ? appicon : "";

  if (icon && icon[0])
  {
    int img_width, img_height;
    unsigned char* pixels = NULL;
    if (iupdrvGetIconPixels(ih, icon, &img_width, &img_height, &pixels))
    {
      use_image_data = 1;
      free(pixels);
    }
    else
    {
      app_icon_param = icon;
    }
  }

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(NOTIFY_BUS_NAME, NOTIFY_OBJECT_PATH, NOTIFY_INTERFACE, "Notify");

  if (!msg)
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, "Failed to create D-Bus message");
    return 0;
  }

  dbus_message_iter_init_append(msg, &iter);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &app_name);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &notify->notification_id);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &app_icon_param);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &title);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &body);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &array_iter);
  {
    const char* action1 = IupGetAttribute(ih, "ACTION1");
    const char* action2 = IupGetAttribute(ih, "ACTION2");
    const char* action3 = IupGetAttribute(ih, "ACTION3");
    const char* action4 = IupGetAttribute(ih, "ACTION4");
    int has_actions = (action1 || action2 || action3 || action4) && notify->has_actions;

    const char* default_id = "default";
    const char* default_label = "";
    dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &default_id);
    dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &default_label);

    if (!has_actions)
    {
      action1 = action2 = action3 = action4 = NULL;
    }

    if (action1)
    {
      const char* id = "action1";
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &id);
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &action1);
    }
    if (action2)
    {
      const char* id = "action2";
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &id);
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &action2);
    }
    if (action3)
    {
      const char* id = "action3";
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &id);
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &action3);
    }
    if (action4)
    {
      const char* id = "action4";
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &id);
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &action4);
    }
  }
  dbus_message_iter_close_container(&iter, &array_iter);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
  {
    int urgency = IupGetInt(ih, "URGENCY");
    if (urgency >= 0 && urgency <= 2)
    {
      DBusMessageIter dict_entry_iter, variant_iter;
      const char* key = "urgency";
      unsigned char val = (unsigned char)urgency;

      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "y", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BYTE, &val);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    if (IupGetInt(ih, "SILENT"))
    {
      DBusMessageIter dict_entry_iter, variant_iter;
      const char* key = "suppress-sound";
      dbus_bool_t val = TRUE;

      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &val);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    if (IupGetInt(ih, "TRANSIENT"))
    {
      DBusMessageIter dict_entry_iter, variant_iter;
      const char* key = "transient";
      dbus_bool_t val = TRUE;

      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &val);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    if ((IupGetAttribute(ih, "ACTION1") || IupGetAttribute(ih, "ACTION2") ||
         IupGetAttribute(ih, "ACTION3") || IupGetAttribute(ih, "ACTION4")) &&
        notify->has_persistence)
    {
      DBusMessageIter dict_entry_iter, variant_iter;
      const char* key = "resident";
      dbus_bool_t val = TRUE;

      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &val);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    if (use_image_data)
    {
      int img_width, img_height;
      unsigned char* pixels = NULL;

      if (iupdrvGetIconPixels(ih, icon, &img_width, &img_height, &pixels))
      {
        DBusMessageIter dict_entry_iter, variant_iter, struct_iter, byte_array_iter;
        const char* key = "image-data";
        unsigned char* use_pixels = pixels;
        int use_width = img_width;
        int use_height = img_height;
        unsigned char* scaled_pixels = NULL;
        dbus_int32_t width, height, rowstride;
        dbus_bool_t has_alpha = TRUE;
        dbus_int32_t bits_per_sample = 8;
        dbus_int32_t channels = 4;
        int x, y;

        if (img_width > IUPUNIX_NOTIFY_MAX_ICON_SIZE || img_height > IUPUNIX_NOTIFY_MAX_ICON_SIZE)
        {
          double scale;
          if (img_width > img_height)
            scale = (double)IUPUNIX_NOTIFY_MAX_ICON_SIZE / img_width;
          else
            scale = (double)IUPUNIX_NOTIFY_MAX_ICON_SIZE / img_height;

          use_width = (int)(img_width * scale);
          use_height = (int)(img_height * scale);
          if (use_width < 1) use_width = 1;
          if (use_height < 1) use_height = 1;

          scaled_pixels = (unsigned char*)malloc(use_width * use_height * 4);
          if (scaled_pixels)
          {
            iupImageResizeRGBA(img_width, img_height, pixels, use_width, use_height, scaled_pixels, 4);
            use_pixels = scaled_pixels;
          }
          else
          {
            use_width = img_width;
            use_height = img_height;
          }
        }

        width = use_width;
        height = use_height;
        rowstride = use_width * 4;

        dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "(iiibiiay)", &variant_iter);
        dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &width);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &height);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &rowstride);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_BOOLEAN, &has_alpha);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &bits_per_sample);
        dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &channels);

        dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &byte_array_iter);

        for (y = 0; y < use_height; y++)
        {
          for (x = 0; x < use_width; x++)
          {
            int offset = (y * use_width + x) * 4;
            unsigned char a = use_pixels[offset + 0];
            unsigned char r = use_pixels[offset + 1];
            unsigned char g = use_pixels[offset + 2];
            unsigned char b = use_pixels[offset + 3];

            dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &r);
            dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &g);
            dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &b);
            dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &a);
          }
        }

        dbus_message_iter_close_container(&struct_iter, &byte_array_iter);
        dbus_message_iter_close_container(&variant_iter, &struct_iter);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);

        if (scaled_pixels)
          free(scaled_pixels);
        free(pixels);
      }
    }
  }
  dbus_message_iter_close_container(&iter, &dict_iter);

  {
    dbus_int32_t timeout_ms = (dbus_int32_t)timeout;
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &timeout_ms);
  }

  reply = dbus_connection_send_with_reply_and_block(
      notify->connection, msg, 5000, &error);

  dbus_message_unref(msg);

  if (reply)
  {
    dbus_uint32_t id;
    if (dbus_message_get_args(reply, NULL, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID))
    {
      notify->notification_id = id;
      IupSetInt(ih, "ID", (int)id);
    }
    dbus_message_unref(reply);
  }

  if (dbus_error_is_set(&error))
  {
    IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (error_cb)
      error_cb(ih, (char*)error.message);
    dbus_error_free(&error);
    return 0;
  }

  return 1;
}

/****************************************************************************
 * Close Notification
 ****************************************************************************/

static int unixNotifyClose(IupUnixNotify* notify)
{
  DBusMessage* msg;
  DBusError error;

  if (!notify || !notify->connection || notify->notification_id == 0)
    return 0;

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(NOTIFY_BUS_NAME, NOTIFY_OBJECT_PATH, NOTIFY_INTERFACE, "CloseNotification");

  if (!msg)
    return 0;

  dbus_message_append_args(msg, DBUS_TYPE_UINT32, &notify->notification_id, DBUS_TYPE_INVALID);

  dbus_connection_send(notify->connection, msg, NULL);
  dbus_connection_flush(notify->connection);

  dbus_message_unref(msg);

  notify->notification_id = 0;

  return 1;
}

/****************************************************************************
 * Driver Interface
 ****************************************************************************/

int iupdrvNotifyShow(Ihandle* ih)
{
  IupUnixNotify* notify = (IupUnixNotify*)iupAttribGet(ih, "_IUPUNIX_NOTIFY");

  if (!notify)
  {
    notify = unixNotifyCreate(ih);
    if (!notify)
    {
      IFns error_cb = (IFns)IupGetCallback(ih, "ERROR_CB");
      if (error_cb)
        error_cb(ih, "Failed to connect to D-Bus session bus");
      return 0;
    }
  }

  return unixNotifySend(notify);
}

int iupdrvNotifyClose(Ihandle* ih)
{
  IupUnixNotify* notify = (IupUnixNotify*)iupAttribGet(ih, "_IUPUNIX_NOTIFY");
  return unixNotifyClose(notify);
}

void iupdrvNotifyDestroy(Ihandle* ih)
{
  IupUnixNotify* notify = (IupUnixNotify*)iupAttribGet(ih, "_IUPUNIX_NOTIFY");

  if (notify)
  {
    unixNotifyDestroy(notify);
    iupAttribSet(ih, "_IUPUNIX_NOTIFY", NULL);
  }
}

int iupdrvNotifyIsAvailable(void)
{
  DBusConnection* connection;
  DBusError error;
  int available = 0;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return 0;
#endif

  dbus_error_init(&error);
  connection = dbus_bus_get(DBUS_BUS_SESSION, &error);

  if (connection && !dbus_error_is_set(&error))
  {
    available = 1;
    dbus_connection_unref(connection);
  }

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  return available;
}

void iupdrvNotifyInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "URGENCY", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPICON", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRANSIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
