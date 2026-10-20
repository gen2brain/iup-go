/** \file
 * \brief Video control, libVLC, GStreamer and libmpv engines
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
#include "iup_video.h"

extern const IvideoBackend iupVideoVlcBackend;
extern const IvideoBackend iupVideoGstreamerBackend;
extern const IvideoBackend iupVideoMpvBackend;

char* iupdrvVideoReadSubtitle(const char* filename)
{
  return iupVideoReadSubtitleStream(fopen(filename, "rb"));
}

#ifdef IUPDBUS_USE_DLOPEN
static DBusConnection* unix_video_bus = NULL;
static dbus_uint32_t unix_video_cookie = 0;

static DBusMessage* unixVideoScreenSaverMessage(const char* method)
{
  return dbus_message_new_method_call("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver", method);
}

static DBusMessage* unixVideoScreenSaverSend(DBusMessage* message)
{
  DBusMessage* reply;
  DBusError error;
  dbus_error_init(&error);
  reply = dbus_connection_send_with_reply_and_block(unix_video_bus, message, 1000, &error);
  dbus_message_unref(message);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);
  return reply;
}

static void unixVideoInhibit(void)
{
  DBusMessage* message = unixVideoScreenSaverMessage("Inhibit");
  DBusMessage* reply;
  DBusError error;
  const char* application = IupGetGlobal("APPNAME");
  const char* reason = "Playing video";
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
  reply = unixVideoScreenSaverSend(message);
  if (!reply)
    return;
  dbus_error_init(&error);
  if (!dbus_message_get_args(reply, &error, DBUS_TYPE_UINT32, &unix_video_cookie, DBUS_TYPE_INVALID))
    unix_video_cookie = 0;
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);
  dbus_message_unref(reply);
}

static void unixVideoUnInhibit(void)
{
  DBusMessage* message = unixVideoScreenSaverMessage("UnInhibit");
  DBusMessage* reply;
  if (!message)
    return;
  dbus_message_append_args(message, DBUS_TYPE_UINT32, &unix_video_cookie, DBUS_TYPE_INVALID);
  unix_video_cookie = 0;
  reply = unixVideoScreenSaverSend(message);
  if (reply)
    dbus_message_unref(reply);
}

typedef struct _XDisplay Display;

static void* unix_video_x11 = NULL;
static void* unix_video_xss = NULL;
static Display* unix_video_display = NULL;
static Display* (*unix_video_XOpenDisplay)(const char*);
static int (*unix_video_XCloseDisplay)(Display*);
static int (*unix_video_XFlush)(Display*);
static void (*unix_video_XScreenSaverSuspend)(Display*, int);

static int unixVideoX11Load(void)
{
  if (unix_video_XScreenSaverSuspend)
    return 1;
  if (!getenv("DISPLAY") || getenv("WAYLAND_DISPLAY"))
    return 0;
  if (!unix_video_x11)
    unix_video_x11 = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL);
  if (!unix_video_xss)
    unix_video_xss = dlopen("libXss.so.1", RTLD_LAZY | RTLD_LOCAL);
  if (!unix_video_x11 || !unix_video_xss)
    return 0;
  *(void**)&unix_video_XOpenDisplay = dlsym(unix_video_x11, "XOpenDisplay");
  *(void**)&unix_video_XCloseDisplay = dlsym(unix_video_x11, "XCloseDisplay");
  *(void**)&unix_video_XFlush = dlsym(unix_video_x11, "XFlush");
  if (!unix_video_XOpenDisplay || !unix_video_XCloseDisplay || !unix_video_XFlush)
    return 0;
  *(void**)&unix_video_XScreenSaverSuspend = dlsym(unix_video_xss, "XScreenSaverSuspend");
  return unix_video_XScreenSaverSuspend != NULL;
}

static void unixVideoX11Suspend(int inhibit)
{
  if (inhibit && !unix_video_display && unixVideoX11Load())
  {
    unix_video_display = unix_video_XOpenDisplay(NULL);
    if (unix_video_display)
    {
      unix_video_XScreenSaverSuspend(unix_video_display, 1);
      unix_video_XFlush(unix_video_display);
    }
  }
  else if (!inhibit && unix_video_display)
  {
    unix_video_XScreenSaverSuspend(unix_video_display, 0);
    unix_video_XCloseDisplay(unix_video_display);
    unix_video_display = NULL;
  }
}

static int unixVideoBusOpen(void)
{
  DBusError error;
  if (unix_video_bus)
    return 1;
  if (!iupDBusOpen())
    return 0;
  dbus_error_init(&error);
  unix_video_bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);
  return unix_video_bus != NULL;
}

void iupdrvVideoInhibitScreenSaver(int inhibit)
{
  if (inhibit)
  {
    if (!unix_video_cookie && unixVideoBusOpen())
      unixVideoInhibit();
    if (!unix_video_cookie)
      unixVideoX11Suspend(1);
  }
  else
  {
    if (unix_video_cookie)
      unixVideoUnInhibit();
    unixVideoX11Suspend(0);
  }
}
#else
void iupdrvVideoInhibitScreenSaver(int inhibit)
{
  (void)inhibit;
}
#endif

const IvideoBackend* iupdrvVideoBackend(int index)
{
  static const IvideoBackend* backends[] = {&iupVideoVlcBackend, &iupVideoGstreamerBackend, &iupVideoMpvBackend};
  if (index < 0 || index >= (int)(sizeof(backends) / sizeof(backends[0])))
    return NULL;
  return backends[index];
}
