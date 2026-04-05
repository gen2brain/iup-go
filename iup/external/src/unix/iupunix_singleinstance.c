/** \file
 * \brief Unix Single Instance Support - D-Bus Implementation
 *
 * This file provides single instance detection using D-Bus for
 * Unix drivers (GTK2, GTK3, GTK4, Qt, Motif, EFL).
 *
 * When the first instance calls iupdrvSingleInstanceSet(), it claims
 * a well-known D-Bus name on the session bus and registers an object
 * path handler to receive data from subsequent instances.
 *
 * When a second instance calls iupdrvSingleInstanceSet(), it detects
 * that the name is already owned, sends the command-line arguments
 * via a D-Bus method call to the first instance, and returns 1.
 *
 * The first instance delivers received data to the first dialog
 * that has COPYDATA_CB registered, matching the Windows behavior.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus.h"
#else
#include <dbus/dbus.h>
#endif

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_dlglist.h"

#include "iup_singleinstance.h"

#define IUP_SI_INTERFACE "org.iup.SingleInstance"
#define IUP_SI_OBJECT_PATH "/org/iup/SingleInstance"

static DBusConnection* si_connection = NULL;
static Ihandle* si_timer = NULL;
static char* si_bus_name = NULL;

static void iupUnixSIBuildBusName(const char* name, char* bus_name, int max_len)
{
  int i, j;
  int name_len = (int)strlen(name);

  strncpy(bus_name, "org.iup.si.", max_len - 1);
  j = (int)strlen(bus_name);

  for (i = 0; i < name_len && j < max_len - 1; i++)
  {
    char c = name[i];
    if (isalnum((unsigned char)c))
      bus_name[j++] = c;
    else if (c == '.' || c == '-' || c == '_')
      bus_name[j++] = c;
    else
      bus_name[j++] = '_';
  }
  bus_name[j] = '\0';
}

static void iupUnixSIDeliverData(const char* data, int len)
{
  Ihandle* dlg;

  for (dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    IFnsi cb = (IFnsi)IupGetCallback(dlg, "COPYDATA_CB");
    if (cb)
    {
      cb(dlg, (char*)data, len);
      return;
    }
  }
}

static DBusHandlerResult iupUnixSIMessageHandler(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  (void)user_data;

  if (dbus_message_is_method_call(message, IUP_SI_INTERFACE, "Activate"))
  {
    const char* data = NULL;
    DBusMessage* reply;

    if (dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID))
    {
      if (data && data[0])
        iupUnixSIDeliverData(data, (int)strlen(data) + 1);
    }

    reply = dbus_message_new_method_return(message);
    if (reply)
    {
      dbus_connection_send(connection, reply, NULL);
      dbus_message_unref(reply);
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int iupUnixSITimerCallback(Ihandle* timer)
{
  DBusConnection* conn;

  (void)timer;

  if (!si_connection)
    return IUP_DEFAULT;

  conn = si_connection;

  dbus_connection_read_write(conn, 0);

  while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS)
    dbus_connection_dispatch(conn);

  return IUP_DEFAULT;
}

static char* iupUnixSIGetCommandLine(void)
{
  FILE* f;
  char buf[4096];
  int total = 0;
  int c;

  f = fopen("/proc/self/cmdline", "r");
  if (!f)
    return NULL;

  while ((c = fgetc(f)) != EOF && total < (int)sizeof(buf) - 1)
  {
    if (c == '\0')
      buf[total++] = ' ';
    else
      buf[total++] = (char)c;
  }
  fclose(f);

  if (total > 0 && buf[total - 1] == ' ')
    total--;
  buf[total] = '\0';

  if (total == 0)
    return NULL;

  return iupStrDup(buf);
}

static void iupUnixSISendToFirst(const char* bus_name)
{
  DBusMessage* msg;
  DBusError error;
  char* cmdline;

  cmdline = iupUnixSIGetCommandLine();
  if (!cmdline)
    cmdline = iupStrDup("");

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(bus_name, IUP_SI_OBJECT_PATH, IUP_SI_INTERFACE, "Activate");
  if (msg)
  {
    const char* data = cmdline;
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID);

    dbus_connection_send_with_reply_and_block(si_connection, msg, 2000, &error);
    dbus_message_unref(msg);
  }

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  free(cmdline);
}

IUP_SDK_API int iupdrvSingleInstanceSet(const char* name)
{
  DBusError error;
  char bus_name[256];
  int result;
  static DBusObjectPathVTable vtable = { NULL, iupUnixSIMessageHandler, NULL, NULL, NULL, NULL };

  if (!name || !name[0])
    return 0;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return 0;
#endif

  iupUnixSIBuildBusName(name, bus_name, sizeof(bus_name));

  dbus_error_init(&error);

  if (!si_connection)
  {
    si_connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (!si_connection || dbus_error_is_set(&error))
    {
      if (dbus_error_is_set(&error))
        dbus_error_free(&error);
      return 0;
    }
  }

  result = dbus_bus_request_name(si_connection, bus_name, DBUS_NAME_FLAG_DO_NOT_QUEUE, &error);

  if (dbus_error_is_set(&error))
  {
    dbus_error_free(&error);
    dbus_connection_unref(si_connection);
    si_connection = NULL;
    return 0;
  }

  if (result == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    si_bus_name = iupStrDup(bus_name);

    dbus_connection_register_object_path(si_connection, IUP_SI_OBJECT_PATH, &vtable, NULL);

    si_timer = IupTimer();
    IupSetCallback(si_timer, "ACTION_CB", (Icallback)iupUnixSITimerCallback);
    IupSetAttribute(si_timer, "TIME", "50");
    IupSetAttribute(si_timer, "RUN", "YES");

    return 0;
  }

  iupUnixSISendToFirst(bus_name);

  dbus_connection_unref(si_connection);
  si_connection = NULL;

  return 1;
}

IUP_SDK_API void iupdrvSingleInstanceClose(void)
{
  if (si_timer)
  {
    IupSetAttribute(si_timer, "RUN", "NO");
    IupDestroy(si_timer);
    si_timer = NULL;
  }

  if (si_connection)
  {
    if (si_bus_name)
    {
      DBusError error;
      dbus_error_init(&error);
      dbus_connection_unregister_object_path(si_connection, IUP_SI_OBJECT_PATH);
      dbus_bus_release_name(si_connection, si_bus_name, &error);
      if (dbus_error_is_set(&error))
        dbus_error_free(&error);
    }

    dbus_connection_unref(si_connection);
    si_connection = NULL;
  }

  if (si_bus_name)
  {
    free(si_bus_name);
    si_bus_name = NULL;
  }
}
