/** \file
 * \brief Keep the display awake, D-Bus screen saver inhibit or X11
 *
 * See Copyright Notice in "iup.h"
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus.h"
#endif

#include "iup.h"

#include "iup_drvinfo.h"


#ifdef IUPDBUS_USE_DLOPEN
static DBusConnection* unix_keepawake_bus = NULL;
static dbus_uint32_t unix_keepawake_cookie = 0;

static DBusMessage* unixKeepAwakeScreenSaverMessage(const char* method)
{
  return dbus_message_new_method_call("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver", method);
}

static DBusMessage* unixKeepAwakeScreenSaverSend(DBusMessage* message)
{
  DBusMessage* reply;
  DBusError error;
  dbus_error_init(&error);
  reply = dbus_connection_send_with_reply_and_block(unix_keepawake_bus, message, 1000, &error);
  dbus_message_unref(message);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);
  return reply;
}

static void unixKeepAwakeInhibit(void)
{
  DBusMessage* message = unixKeepAwakeScreenSaverMessage("Inhibit");
  DBusMessage* reply;
  DBusError error;
  const char* application = IupGetGlobal("APPNAME");
  const char* reason = "Keep awake";
  char name[32];
  if (!message)
    return;
  if (!application || !application[0])
    application = IupGetGlobal("APPID");
  if (!application || !application[0])
  {
    snprintf(name, sizeof(name), "pid %d", (int)getpid());
    application = name;
  }
  dbus_message_append_args(message, DBUS_TYPE_STRING, &application, DBUS_TYPE_STRING, &reason, DBUS_TYPE_INVALID);
  reply = unixKeepAwakeScreenSaverSend(message);
  if (!reply)
    return;
  dbus_error_init(&error);
  if (!dbus_message_get_args(reply, &error, DBUS_TYPE_UINT32, &unix_keepawake_cookie, DBUS_TYPE_INVALID))
    unix_keepawake_cookie = 0;
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);
  dbus_message_unref(reply);
}

static void unixKeepAwakeUnInhibit(void)
{
  DBusMessage* message = unixKeepAwakeScreenSaverMessage("UnInhibit");
  DBusMessage* reply;
  if (!message)
    return;
  dbus_message_append_args(message, DBUS_TYPE_UINT32, &unix_keepawake_cookie, DBUS_TYPE_INVALID);
  unix_keepawake_cookie = 0;
  reply = unixKeepAwakeScreenSaverSend(message);
  if (reply)
    dbus_message_unref(reply);
}

typedef struct _XDisplay Display;

static void* unix_keepawake_x11 = NULL;
static void* unix_keepawake_xss = NULL;
static Display* unix_keepawake_display = NULL;
static Display* (*unix_keepawake_XOpenDisplay)(const char*);
static int (*unix_keepawake_XCloseDisplay)(Display*);
static int (*unix_keepawake_XFlush)(Display*);
static void (*unix_keepawake_XScreenSaverSuspend)(Display*, int);

static int unixKeepAwakeX11Load(void)
{
  if (unix_keepawake_XScreenSaverSuspend)
    return 1;
  if (!getenv("DISPLAY") || getenv("WAYLAND_DISPLAY"))
    return 0;
  if (!unix_keepawake_x11)
    unix_keepawake_x11 = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL);
  if (!unix_keepawake_xss)
    unix_keepawake_xss = dlopen("libXss.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (!unix_keepawake_x11 || !unix_keepawake_xss)
    return 0;
  *(void**)&unix_keepawake_XOpenDisplay = dlsym(unix_keepawake_x11, "XOpenDisplay");
  *(void**)&unix_keepawake_XCloseDisplay = dlsym(unix_keepawake_x11, "XCloseDisplay");
  *(void**)&unix_keepawake_XFlush = dlsym(unix_keepawake_x11, "XFlush");
  if (!unix_keepawake_XOpenDisplay || !unix_keepawake_XCloseDisplay || !unix_keepawake_XFlush)
    return 0;
  *(void**)&unix_keepawake_XScreenSaverSuspend = dlsym(unix_keepawake_xss, "XScreenSaverSuspend");
  return unix_keepawake_XScreenSaverSuspend != NULL;
}

static void unixKeepAwakeX11Suspend(int inhibit)
{
  if (inhibit && !unix_keepawake_display && unixKeepAwakeX11Load())
  {
    unix_keepawake_display = unix_keepawake_XOpenDisplay(NULL);
    if (unix_keepawake_display)
    {
      unix_keepawake_XScreenSaverSuspend(unix_keepawake_display, 1);
      unix_keepawake_XFlush(unix_keepawake_display);
    }
  }
  else if (!inhibit && unix_keepawake_display)
  {
    unix_keepawake_XScreenSaverSuspend(unix_keepawake_display, 0);
    unix_keepawake_XCloseDisplay(unix_keepawake_display);
    unix_keepawake_display = NULL;
  }
}

static int unixKeepAwakeBusOpen(void)
{
  DBusError error;
  if (unix_keepawake_bus)
    return 1;
  if (!iupDBusOpen())
    return 0;
  dbus_error_init(&error);
  unix_keepawake_bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);
  return unix_keepawake_bus != NULL;
}

IUP_SDK_API void iupdrvSetKeepAwake(int inhibit)
{
  if (inhibit)
  {
    if (!unix_keepawake_cookie && unixKeepAwakeBusOpen())
      unixKeepAwakeInhibit();
    if (!unix_keepawake_cookie)
      unixKeepAwakeX11Suspend(1);
  }
  else
  {
    if (unix_keepawake_cookie)
      unixKeepAwakeUnInhibit();
    unixKeepAwakeX11Suspend(0);
  }
}
#else
IUP_SDK_API void iupdrvSetKeepAwake(int inhibit)
{
  (void)inhibit;
}
#endif
