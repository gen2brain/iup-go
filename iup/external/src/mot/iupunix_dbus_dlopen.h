/** \file
 * \brief DBus Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPDBUS_DLOPEN_H
#define __IUPDBUS_DLOPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dlfcn.h>

typedef struct DBusError DBusError;
typedef struct DBusConnection DBusConnection;
typedef struct DBusMessage DBusMessage;
typedef struct DBusMessageIter DBusMessageIter;
typedef struct DBusObjectPathVTable DBusObjectPathVTable;
typedef unsigned int dbus_uint32_t;
typedef int dbus_int32_t;
typedef unsigned int dbus_bool_t;

struct DBusError {
  const char *name;
  const char *message;
  unsigned int dummy1 : 1;
  unsigned int dummy2 : 1;
  unsigned int dummy3 : 1;
  unsigned int dummy4 : 1;
  unsigned int dummy5 : 1;
  void *padding1;
};

struct DBusMessageIter {
  void *dummy1;
  void *dummy2;
  dbus_uint32_t dummy3;
  int dummy4;
  int dummy5;
  int dummy6;
  int dummy7;
  int dummy8;
  int dummy9;
  int dummy10;
  int dummy11;
  int pad1;
  int pad2;
  void *pad3;
};

typedef enum {
  DBUS_BUS_SESSION,
  DBUS_BUS_SYSTEM,
  DBUS_BUS_STARTER
} DBusBusType;

typedef enum {
  DBUS_HANDLER_RESULT_HANDLED,
  DBUS_HANDLER_RESULT_NOT_YET_HANDLED,
  DBUS_HANDLER_RESULT_NEED_MEMORY
} DBusHandlerResult;

typedef enum {
  DBUS_DISPATCH_DATA_REMAINS,
  DBUS_DISPATCH_COMPLETE,
  DBUS_DISPATCH_NEED_MEMORY
} DBusDispatchStatus;

typedef DBusHandlerResult (*DBusHandleMessageFunction)(DBusConnection*, DBusMessage*, void*);

struct DBusObjectPathVTable {
  void (*unregister_function)(DBusConnection*, void*);
  DBusHandleMessageFunction message_function;
  void (*dbus_internal_pad1)(void*);
  void (*dbus_internal_pad2)(void*);
  void (*dbus_internal_pad3)(void*);
  void (*dbus_internal_pad4)(void*);
};

#define DBUS_NAME_FLAG_ALLOW_REPLACEMENT 0x1
#define DBUS_NAME_FLAG_REPLACE_EXISTING 0x2
#define DBUS_NAME_FLAG_DO_NOT_QUEUE 0x4
#define DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER 1

#define DBUS_TYPE_INVALID       ((int) '\0')
#define DBUS_TYPE_BYTE          ((int) 'y')
#define DBUS_TYPE_BOOLEAN       ((int) 'b')
#define DBUS_TYPE_INT16         ((int) 'n')
#define DBUS_TYPE_UINT16        ((int) 'q')
#define DBUS_TYPE_INT32         ((int) 'i')
#define DBUS_TYPE_UINT32        ((int) 'u')
#define DBUS_TYPE_INT64         ((int) 'x')
#define DBUS_TYPE_UINT64        ((int) 't')
#define DBUS_TYPE_DOUBLE        ((int) 'd')
#define DBUS_TYPE_STRING        ((int) 's')
#define DBUS_TYPE_OBJECT_PATH   ((int) 'o')
#define DBUS_TYPE_SIGNATURE     ((int) 'g')
#define DBUS_TYPE_UNIX_FD      ((int) 'h')
#define DBUS_TYPE_ARRAY         ((int) 'a')
#define DBUS_TYPE_VARIANT       ((int) 'v')
#define DBUS_TYPE_STRUCT        ((int) 'r')
#define DBUS_TYPE_DICT_ENTRY    ((int) 'e')

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static void* iupdbus_handle = NULL;

static void (*iupdbus_error_init)(DBusError*);
static dbus_bool_t (*iupdbus_error_is_set)(const DBusError*);
static void (*iupdbus_error_free)(DBusError*);
static DBusConnection* (*iupdbus_bus_get)(DBusBusType, DBusError*);
static int (*iupdbus_bus_request_name)(DBusConnection*, const char*, unsigned int, DBusError*);
static int (*iupdbus_bus_release_name)(DBusConnection*, const char*, DBusError*);
static dbus_bool_t (*iupdbus_connection_register_object_path)(DBusConnection*, const char*, const DBusObjectPathVTable*, void*);
static dbus_bool_t (*iupdbus_connection_unregister_object_path)(DBusConnection*, const char*);
static dbus_bool_t (*iupdbus_connection_send)(DBusConnection*, DBusMessage*, dbus_uint32_t*);
static DBusMessage* (*iupdbus_connection_send_with_reply_and_block)(DBusConnection*, DBusMessage*, int, DBusError*);
static void (*iupdbus_connection_flush)(DBusConnection*);
static dbus_bool_t (*iupdbus_connection_read_write)(DBusConnection*, int);
static DBusDispatchStatus (*iupdbus_connection_get_dispatch_status)(DBusConnection*);
static DBusDispatchStatus (*iupdbus_connection_dispatch)(DBusConnection*);
static void (*iupdbus_connection_close)(DBusConnection*);
static void (*iupdbus_connection_unref)(DBusConnection*);
static DBusMessage* (*iupdbus_message_new_method_call)(const char*, const char*, const char*, const char*);
static DBusMessage* (*iupdbus_message_new_method_return)(DBusMessage*);
static DBusMessage* (*iupdbus_message_new_signal)(const char*, const char*, const char*);
static void (*iupdbus_message_unref)(DBusMessage*);
static dbus_bool_t (*iupdbus_message_append_args)(DBusMessage*, int, ...);
static dbus_bool_t (*iupdbus_message_get_args)(DBusMessage*, DBusError*, int, ...);
static dbus_bool_t (*iupdbus_message_is_method_call)(DBusMessage*, const char*, const char*);
static const char* (*iupdbus_message_get_interface)(DBusMessage*);
static const char* (*iupdbus_message_get_member)(DBusMessage*);
static void (*iupdbus_message_iter_init_append)(DBusMessage*, DBusMessageIter*);
static dbus_bool_t (*iupdbus_message_iter_open_container)(DBusMessageIter*, int, const char*, DBusMessageIter*);
static dbus_bool_t (*iupdbus_message_iter_close_container)(DBusMessageIter*, DBusMessageIter*);
static dbus_bool_t (*iupdbus_message_iter_append_basic)(DBusMessageIter*, int, const void*);

#define dbus_error_init iupdbus_error_init
#define dbus_error_is_set iupdbus_error_is_set
#define dbus_error_free iupdbus_error_free
#define dbus_bus_get iupdbus_bus_get
#define dbus_bus_request_name iupdbus_bus_request_name
#define dbus_bus_release_name iupdbus_bus_release_name
#define dbus_connection_register_object_path iupdbus_connection_register_object_path
#define dbus_connection_unregister_object_path iupdbus_connection_unregister_object_path
#define dbus_connection_send iupdbus_connection_send
#define dbus_connection_send_with_reply_and_block iupdbus_connection_send_with_reply_and_block
#define dbus_connection_flush iupdbus_connection_flush
#define dbus_connection_read_write iupdbus_connection_read_write
#define dbus_connection_get_dispatch_status iupdbus_connection_get_dispatch_status
#define dbus_connection_dispatch iupdbus_connection_dispatch
#define dbus_connection_close iupdbus_connection_close
#define dbus_connection_unref iupdbus_connection_unref
#define dbus_message_new_method_call iupdbus_message_new_method_call
#define dbus_message_new_method_return iupdbus_message_new_method_return
#define dbus_message_new_signal iupdbus_message_new_signal
#define dbus_message_unref iupdbus_message_unref
#define dbus_message_append_args iupdbus_message_append_args
#define dbus_message_get_args iupdbus_message_get_args
#define dbus_message_is_method_call iupdbus_message_is_method_call
#define dbus_message_get_interface iupdbus_message_get_interface
#define dbus_message_get_member iupdbus_message_get_member
#define dbus_message_iter_init_append iupdbus_message_iter_init_append
#define dbus_message_iter_open_container iupdbus_message_iter_open_container
#define dbus_message_iter_close_container iupdbus_message_iter_close_container
#define dbus_message_iter_append_basic iupdbus_message_iter_append_basic

static inline int iupDBusOpen(void)
{
  if (iupdbus_handle)
    return 1;

  iupdbus_handle = dlopen("libdbus-1.so.3", RTLD_LAZY | RTLD_LOCAL);
  if (!iupdbus_handle)
    iupdbus_handle = dlopen("libdbus-1.so", RTLD_LAZY | RTLD_LOCAL);

  if (!iupdbus_handle)
    return 0;

  iupdbus_error_init = dlsym(iupdbus_handle, "dbus_error_init");
  iupdbus_error_is_set = dlsym(iupdbus_handle, "dbus_error_is_set");
  iupdbus_error_free = dlsym(iupdbus_handle, "dbus_error_free");
  iupdbus_bus_get = dlsym(iupdbus_handle, "dbus_bus_get");
  iupdbus_bus_request_name = dlsym(iupdbus_handle, "dbus_bus_request_name");
  iupdbus_bus_release_name = dlsym(iupdbus_handle, "dbus_bus_release_name");
  iupdbus_connection_register_object_path = dlsym(iupdbus_handle, "dbus_connection_register_object_path");
  iupdbus_connection_unregister_object_path = dlsym(iupdbus_handle, "dbus_connection_unregister_object_path");
  iupdbus_connection_send = dlsym(iupdbus_handle, "dbus_connection_send");
  iupdbus_connection_send_with_reply_and_block = dlsym(iupdbus_handle, "dbus_connection_send_with_reply_and_block");
  iupdbus_connection_flush = dlsym(iupdbus_handle, "dbus_connection_flush");
  iupdbus_connection_read_write = dlsym(iupdbus_handle, "dbus_connection_read_write");
  iupdbus_connection_get_dispatch_status = dlsym(iupdbus_handle, "dbus_connection_get_dispatch_status");
  iupdbus_connection_dispatch = dlsym(iupdbus_handle, "dbus_connection_dispatch");
  iupdbus_connection_close = dlsym(iupdbus_handle, "dbus_connection_close");
  iupdbus_connection_unref = dlsym(iupdbus_handle, "dbus_connection_unref");
  iupdbus_message_new_method_call = dlsym(iupdbus_handle, "dbus_message_new_method_call");
  iupdbus_message_new_method_return = dlsym(iupdbus_handle, "dbus_message_new_method_return");
  iupdbus_message_new_signal = dlsym(iupdbus_handle, "dbus_message_new_signal");
  iupdbus_message_unref = dlsym(iupdbus_handle, "dbus_message_unref");
  iupdbus_message_append_args = dlsym(iupdbus_handle, "dbus_message_append_args");
  iupdbus_message_get_args = dlsym(iupdbus_handle, "dbus_message_get_args");
  iupdbus_message_is_method_call = dlsym(iupdbus_handle, "dbus_message_is_method_call");
  iupdbus_message_get_interface = dlsym(iupdbus_handle, "dbus_message_get_interface");
  iupdbus_message_get_member = dlsym(iupdbus_handle, "dbus_message_get_member");
  iupdbus_message_iter_init_append = dlsym(iupdbus_handle, "dbus_message_iter_init_append");
  iupdbus_message_iter_open_container = dlsym(iupdbus_handle, "dbus_message_iter_open_container");
  iupdbus_message_iter_close_container = dlsym(iupdbus_handle, "dbus_message_iter_close_container");
  iupdbus_message_iter_append_basic = dlsym(iupdbus_handle, "dbus_message_iter_append_basic");

  if (!iupdbus_error_init || !iupdbus_bus_get || !iupdbus_connection_send)
  {
    dlclose(iupdbus_handle);
    iupdbus_handle = NULL;
    return 0;
  }

  return 1;
}

static inline void iupDBusClose(void)
{
  if (iupdbus_handle)
  {
    dlclose(iupdbus_handle);
    iupdbus_handle = NULL;
  }
}

#ifdef __cplusplus
}
#endif

#endif