/** \file
 * \brief Location (GeoClue2 over D-Bus)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus.h"
#else
#include <dbus/dbus.h>
#endif

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_location.h"

#define GEOCLUE_BUS_NAME     "org.freedesktop.GeoClue2"
#define GEOCLUE_MANAGER_PATH "/org/freedesktop/GeoClue2/Manager"
#define GEOCLUE_MANAGER_IFACE "org.freedesktop.GeoClue2.Manager"
#define GEOCLUE_CLIENT_IFACE  "org.freedesktop.GeoClue2.Client"
#define GEOCLUE_LOCATION_IFACE "org.freedesktop.GeoClue2.Location"
#define DBUS_PROPERTIES_IFACE "org.freedesktop.DBus.Properties"

#define GEOCLUE_ACCURACY_CITY  4
#define GEOCLUE_ACCURACY_EXACT 8

#define GEOCLUE_ALTITUDE_UNKNOWN (-1.0e300)

typedef struct _IupUnixLocation
{
  Ihandle* ih;
  Ihandle* timer;
  DBusConnection* connection;
  char client_path[256];
  char match_rule[512];
  int filter_added;
  int permission;
} IupUnixLocation;

enum { LOCATION_PERMISSION_PROMPT, LOCATION_PERMISSION_GRANTED, LOCATION_PERMISSION_DENIED };


static void unixLocationPostError(Ihandle* ih, const char* text)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), text);
  iupLocationPost(ih, &msg);
}

static void unixLocationPostPermission(Ihandle* ih, int granted)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_PERMISSION;
  msg.granted = granted;
  iupLocationPost(ih, &msg);
}

static DBusMessage* unixLocationCall(DBusConnection* connection, DBusMessage* request, char* error_text, int error_size)
{
  DBusError error;
  DBusMessage* reply;

  dbus_error_init(&error);
  reply = dbus_connection_send_with_reply_and_block(connection, request, 5000, &error);
  dbus_message_unref(request);

  if (dbus_error_is_set(&error))
  {
    if (error_text)
      iupStrCopyN(error_text, error_size, error.name);
    dbus_error_free(&error);
    return NULL;
  }
  return reply;
}

static int unixLocationSetProperty(IupUnixLocation* loc, const char* name, int type, const void* value)
{
  DBusMessage* request;
  DBusMessage* reply;
  DBusMessageIter iter, variant;
  const char* iface = GEOCLUE_CLIENT_IFACE;
  const char* signature = (type == DBUS_TYPE_STRING)? "s": "u";

  request = dbus_message_new_method_call(GEOCLUE_BUS_NAME, loc->client_path, DBUS_PROPERTIES_IFACE, "Set");
  if (!request)
    return 0;

  dbus_message_iter_init_append(request, &iter);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &iface);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &name);
  dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, signature, &variant);
  dbus_message_iter_append_basic(&variant, type, value);
  dbus_message_iter_close_container(&iter, &variant);

  reply = unixLocationCall(loc->connection, request, NULL, 0);
  if (!reply)
    return 0;
  dbus_message_unref(reply);
  return 1;
}

static void unixLocationReadFix(IupUnixLocation* loc, const char* location_path)
{
  DBusMessage* request;
  DBusMessage* reply;
  DBusMessageIter iter, dict;
  const char* iface = GEOCLUE_LOCATION_IFACE;
  IlocationMsg msg;

  request = dbus_message_new_method_call(GEOCLUE_BUS_NAME, location_path, DBUS_PROPERTIES_IFACE, "GetAll");
  if (!request)
    return;
  dbus_message_append_args(request, DBUS_TYPE_STRING, &iface, DBUS_TYPE_INVALID);

  reply = unixLocationCall(loc->connection, request, NULL, 0);
  if (!reply)
    return;

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_FIX;
  msg.altitude = GEOCLUE_ALTITUDE_UNKNOWN;
  msg.speed = -1;
  msg.heading = -1;

  if (dbus_message_iter_init(reply, &iter) && dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY)
  {
    dbus_message_iter_recurse(&iter, &dict);
    while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY)
    {
      DBusMessageIter entry, variant;
      const char* key = NULL;

      dbus_message_iter_recurse(&dict, &entry);
      dbus_message_iter_get_basic(&entry, &key);
      dbus_message_iter_next(&entry);
      dbus_message_iter_recurse(&entry, &variant);

      if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_DOUBLE)
      {
        double value = 0;
        dbus_message_iter_get_basic(&variant, &value);
        if (iupStrEqual(key, "Latitude")) msg.latitude = value;
        else if (iupStrEqual(key, "Longitude")) msg.longitude = value;
        else if (iupStrEqual(key, "Accuracy")) msg.accuracy = value;
        else if (iupStrEqual(key, "Altitude")) msg.altitude = value;
        else if (iupStrEqual(key, "Speed")) msg.speed = value;
        else if (iupStrEqual(key, "Heading")) msg.heading = value;
      }
      else if (dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRUCT && iupStrEqual(key, "Timestamp"))
      {
        DBusMessageIter pair;
        dbus_uint64_t seconds = 0, microseconds = 0;
        dbus_message_iter_recurse(&variant, &pair);
        dbus_message_iter_get_basic(&pair, &seconds);
        dbus_message_iter_next(&pair);
        dbus_message_iter_get_basic(&pair, &microseconds);
        msg.timestamp = (long long)seconds * 1000 + (long long)(microseconds / 1000);
      }

      dbus_message_iter_next(&dict);
    }
  }
  dbus_message_unref(reply);

  msg.has_altitude = msg.altitude > GEOCLUE_ALTITUDE_UNKNOWN;
  msg.has_speed = msg.speed >= 0;
  msg.has_heading = msg.heading >= 0;

  iupLocationPost(loc->ih, &msg);
}

static DBusHandlerResult unixLocationSignalFilter(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  IupUnixLocation* loc = (IupUnixLocation*)user_data;
  const char* old_path = NULL;
  const char* new_path = NULL;
  (void)connection;

  if (!dbus_message_is_signal(message, GEOCLUE_CLIENT_IFACE, "LocationUpdated"))
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_get_args(message, NULL, DBUS_TYPE_OBJECT_PATH, &old_path, DBUS_TYPE_OBJECT_PATH, &new_path, DBUS_TYPE_INVALID) && new_path)
    unixLocationReadFix(loc, new_path);

  return DBUS_HANDLER_RESULT_HANDLED;
}

static int unixLocationTimerCallback(Ihandle* timer)
{
  IupUnixLocation* loc = (IupUnixLocation*)iupAttribGet(timer, "_IUPUNIX_LOCATION_DATA");
  DBusConnection* connection;

  if (!loc || !loc->connection)
    return IUP_DEFAULT;

  connection = loc->connection;
  dbus_connection_read_write(connection, 0);
  while (loc->connection == connection && dbus_connection_get_dispatch_status(connection) == DBUS_DISPATCH_DATA_REMAINS)
    dbus_connection_dispatch(connection);

  return IUP_DEFAULT;
}

static DBusConnection* unixLocationConnect(void)
{
  DBusError error;
  DBusConnection* connection;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return NULL;
#endif

  dbus_error_init(&error);
  connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (dbus_error_is_set(&error))
  {
    dbus_error_free(&error);
    return NULL;
  }
  return connection;
}

static int unixLocationServiceExists(DBusConnection* connection)
{
  DBusMessage* request;
  DBusMessage* reply;
  char** names = NULL;
  int count = 0, i, found = 0;

  if (dbus_bus_name_has_owner(connection, GEOCLUE_BUS_NAME, NULL))
    return 1;

  request = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListActivatableNames");
  if (!request)
    return 0;
  reply = unixLocationCall(connection, request, NULL, 0);
  if (!reply)
    return 0;

  if (dbus_message_get_args(reply, NULL, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &names, &count, DBUS_TYPE_INVALID))
  {
    for (i = 0; i < count; i++)
    {
      if (iupStrEqual(names[i], GEOCLUE_BUS_NAME))
        found = 1;
    }
    dbus_free_string_array(names);
  }
  dbus_message_unref(reply);
  return found;
}

static void unixLocationRelease(IupUnixLocation* loc)
{
  if (loc->timer)
  {
    IupDestroy(loc->timer);
    loc->timer = NULL;
  }
  if (loc->connection)
  {
    DBusConnection* connection = loc->connection;
    loc->connection = NULL;
    if (loc->filter_added)
      dbus_connection_remove_filter(connection, unixLocationSignalFilter, loc);
    loc->filter_added = 0;
    if (loc->match_rule[0])
      dbus_bus_remove_match(connection, loc->match_rule, NULL);
    dbus_connection_unref(connection);
  }
  loc->client_path[0] = 0;
  loc->match_rule[0] = 0;
}

IUP_SDK_API int iupdrvLocationIsAvailable(void)
{
  DBusConnection* connection = unixLocationConnect();
  int available;
  if (!connection)
    return 0;
  available = unixLocationServiceExists(connection);
  dbus_connection_unref(connection);
  return available;
}

IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  IupUnixLocation* loc = (IupUnixLocation*)iupAttribGet(ih, "_IUPUNIX_LOCATION");
  DBusMessage* request;
  DBusMessage* reply;
  DBusError error;
  const char* client_path = NULL;
  const char* desktop_id;
  char error_name[128];
  dbus_uint32_t level, threshold;

  if (!loc)
  {
    loc = (IupUnixLocation*)calloc(1, sizeof(IupUnixLocation));
    if (!loc)
      return 0;
    loc->ih = ih;
    iupAttribSet(ih, "_IUPUNIX_LOCATION", (char*)loc);
  }

  loc->connection = unixLocationConnect();
  if (!loc->connection || !unixLocationServiceExists(loc->connection))
  {
    unixLocationRelease(loc);
    unixLocationPostError(ih, "GeoClue2 service not available");
    return 0;
  }

  request = dbus_message_new_method_call(GEOCLUE_BUS_NAME, GEOCLUE_MANAGER_PATH, GEOCLUE_MANAGER_IFACE, "GetClient");
  reply = request? unixLocationCall(loc->connection, request, error_name, sizeof(error_name)): NULL;
  if (!reply || !dbus_message_get_args(reply, NULL, DBUS_TYPE_OBJECT_PATH, &client_path, DBUS_TYPE_INVALID))
  {
    char text[192];
    if (reply) dbus_message_unref(reply);
    else snprintf(text, sizeof(text), "GeoClue2 client not available (%s)", error_name);
    unixLocationRelease(loc);
    unixLocationPostError(ih, reply? "GeoClue2 client not available": text);
    return 0;
  }
  iupStrCopyN(loc->client_path, sizeof(loc->client_path), client_path);
  dbus_message_unref(reply);

  desktop_id = IupGetGlobal("APPID");
  if (!desktop_id || !desktop_id[0])
    desktop_id = "iup";
  unixLocationSetProperty(loc, "DesktopId", DBUS_TYPE_STRING, &desktop_id);

  level = iupStrEqualNoCase(iupAttribGetStr(ih, "ACCURACY"), "FINE")? GEOCLUE_ACCURACY_EXACT: GEOCLUE_ACCURACY_CITY;
  unixLocationSetProperty(loc, "RequestedAccuracyLevel", DBUS_TYPE_UINT32, &level);

  threshold = (dbus_uint32_t)iupAttribGetInt(ih, "DISTANCE");
  unixLocationSetProperty(loc, "DistanceThreshold", DBUS_TYPE_UINT32, &threshold);

  threshold = (dbus_uint32_t)(iupAttribGetInt(ih, "INTERVAL") / 1000);
  unixLocationSetProperty(loc, "TimeThreshold", DBUS_TYPE_UINT32, &threshold);

  snprintf(loc->match_rule, sizeof(loc->match_rule),
           "type='signal',interface='" GEOCLUE_CLIENT_IFACE "',path='%s',member='LocationUpdated'", loc->client_path);
  dbus_error_init(&error);
  dbus_bus_add_match(loc->connection, loc->match_rule, &error);
  if (dbus_error_is_set(&error))
  {
    dbus_error_free(&error);
    loc->match_rule[0] = 0;
  }
  loc->filter_added = dbus_connection_add_filter(loc->connection, unixLocationSignalFilter, loc, NULL)? 1: 0;

  request = dbus_message_new_method_call(GEOCLUE_BUS_NAME, loc->client_path, GEOCLUE_CLIENT_IFACE, "Start");
  reply = request? unixLocationCall(loc->connection, request, error_name, sizeof(error_name)): NULL;
  if (!reply)
  {
    int denied = strstr(error_name, "AccessDenied") != NULL;
    loc->permission = denied? LOCATION_PERMISSION_DENIED: LOCATION_PERMISSION_PROMPT;
    unixLocationRelease(loc);
    if (denied)
    {
      unixLocationPostPermission(ih, 0);
      unixLocationPostError(ih, "Location access denied");
    }
    else
    {
      char text[192];
      snprintf(text, sizeof(text), "GeoClue2 client failed to start (%s)", error_name);
      unixLocationPostError(ih, text);
    }
    return 0;
  }
  dbus_message_unref(reply);

  if (loc->permission != LOCATION_PERMISSION_GRANTED)
  {
    loc->permission = LOCATION_PERMISSION_GRANTED;
    unixLocationPostPermission(ih, 1);
  }

  loc->timer = IupTimer();
  iupAttribSet(loc->timer, "_IUPUNIX_LOCATION_DATA", (char*)loc);
  IupSetCallback(loc->timer, "ACTION_CB", (Icallback)unixLocationTimerCallback);
  IupSetAttribute(loc->timer, "TIME", "100");
  IupSetAttribute(loc->timer, "RUN", "YES");

  return 1;
}

IUP_SDK_API void iupdrvLocationStop(Ihandle* ih)
{
  IupUnixLocation* loc = (IupUnixLocation*)iupAttribGet(ih, "_IUPUNIX_LOCATION");
  DBusMessage* request;
  DBusMessage* reply;

  if (!loc || !loc->connection)
    return;

  request = dbus_message_new_method_call(GEOCLUE_BUS_NAME, loc->client_path, GEOCLUE_CLIENT_IFACE, "Stop");
  reply = request? unixLocationCall(loc->connection, request, NULL, 0): NULL;
  if (reply)
    dbus_message_unref(reply);

  unixLocationRelease(loc);
}

IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  IupUnixLocation* loc = (IupUnixLocation*)iupAttribGet(ih, "_IUPUNIX_LOCATION");

  if (loc && loc->permission == LOCATION_PERMISSION_GRANTED)
    return "GRANTED";
  if (loc && loc->permission == LOCATION_PERMISSION_DENIED)
    return "DENIED";
  if (!iupdrvLocationIsAvailable())
    return "UNAVAILABLE";
  return "PROMPT";
}

IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih)
{
  IupUnixLocation* loc = (IupUnixLocation*)iupAttribGet(ih, "_IUPUNIX_LOCATION");
  if (!loc)
    return;
  unixLocationRelease(loc);
  free(loc);
  iupAttribSet(ih, "_IUPUNIX_LOCATION", NULL);
}

IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic)
{
  (void)ic;
}
