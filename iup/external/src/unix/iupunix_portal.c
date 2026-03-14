/** \file
 * \brief XDG Desktop Portal - FileChooser and OpenURI via D-Bus
 *
 * Provides portal-based file dialogs and URI opening.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus.h"
#else
#include <dbus/dbus.h>
#endif

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_array.h"
#include "iup_strmessage.h"

#define PORTAL_BUS_NAME    "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_FILECHOOSER "org.freedesktop.portal.FileChooser"
#define PORTAL_OPENURI     "org.freedesktop.portal.OpenURI"
#define PORTAL_REQUEST     "org.freedesktop.portal.Request"

#define PORTAL_TIMEOUT_FILE 120
#define PORTAL_TIMEOUT_URI  10

typedef struct {
  DBusConnection* connection;
  char request_path[512];
  int response_code;
  int done;
  char** uris;
  int uri_count;
} PortalRequest;

static unsigned long portal_token_counter = 0;

/****************************************************************************
 * Helpers
 ****************************************************************************/

static void portalGenerateToken(char* token, int token_size)
{
  snprintf(token, token_size, "iup%d_%lu", (int)getpid(), portal_token_counter++);
}

static void portalBuildRequestPath(const char* unique_name, const char* token, char* path, int path_size)
{
  char sender[256];
  int i;

  if (!unique_name || unique_name[0] != ':')
  {
    snprintf(path, path_size, "/org/freedesktop/portal/desktop/request/UNKNOWN/%s", token);
    return;
  }

  strncpy(sender, unique_name + 1, sizeof(sender) - 1);
  sender[sizeof(sender) - 1] = 0;

  for (i = 0; sender[i]; i++)
  {
    if (sender[i] == '.')
      sender[i] = '_';
  }

  snprintf(path, path_size, "/org/freedesktop/portal/desktop/request/%s/%s", sender, token);
}

static char* portalUriToPath(const char* uri)
{
  const char* src;
  char* path;
  char* dst;

  if (strncmp(uri, "file://", 7) == 0)
    src = uri + 7;
  else
    return strdup(uri);

  path = (char*)malloc(strlen(src) + 1);
  dst = path;

  while (*src)
  {
    if (*src == '%' && src[1] && src[2])
    {
      char hex[3] = { src[1], src[2], 0 };
      *dst++ = (char)strtol(hex, NULL, 16);
      src += 3;
    }
    else
    {
      *dst++ = *src++;
    }
  }
  *dst = 0;

  return path;
}

static int portalIsFile(const char* name)
{
  struct stat st;
  if (stat(name, &st) != 0)
    return 0;
  return S_ISREG(st.st_mode);
}

static int portalIsDirectory(const char* name)
{
  struct stat st;
  if (stat(name, &st) != 0)
    return 0;
  return S_ISDIR(st.st_mode);
}

static void portalFreeRequest(PortalRequest* req)
{
  if (req->uris)
  {
    int i;
    for (i = 0; i < req->uri_count; i++)
      free(req->uris[i]);
    free(req->uris);
    req->uris = NULL;
  }
  req->uri_count = 0;
}

/****************************************************************************
 * Dict Entry Helpers
 ****************************************************************************/

static void portalAppendDictEntry_String(DBusMessageIter* dict_iter, const char* key, const char* value)
{
  DBusMessageIter dict_entry_iter, variant_iter;

  dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
  dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
  dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
  dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
  dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
}

static void portalAppendDictEntry_Boolean(DBusMessageIter* dict_iter, const char* key, dbus_bool_t value)
{
  DBusMessageIter dict_entry_iter, variant_iter;

  dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
  dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
  dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
  dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
  dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
}

static void portalAppendDictEntry_ByteArray(DBusMessageIter* dict_iter, const char* key, const unsigned char* data, int len)
{
  DBusMessageIter dict_entry_iter, variant_iter, array_iter;

  dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
  dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
  dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "ay", &variant_iter);
  dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "y", &array_iter);
  {
    int i;
    for (i = 0; i < len; i++)
      dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_BYTE, &data[i]);
  }
  dbus_message_iter_close_container(&variant_iter, &array_iter);
  dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
  dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
}

/****************************************************************************
 * Response Signal Filter
 ****************************************************************************/

static DBusHandlerResult portalSignalFilter(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  PortalRequest* req = (PortalRequest*)user_data;
  DBusMessageIter iter, results_iter;
  const char* path;

  (void)connection;

  if (!req || req->done)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (!dbus_message_is_signal(message, PORTAL_REQUEST, "Response"))
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  path = dbus_message_get_path(message);
  if (!path || strcmp(path, req->request_path) != 0)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (!dbus_message_iter_init(message, &iter))
  {
    req->response_code = 2;
    req->done = 1;
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32)
  {
    dbus_uint32_t response;
    dbus_message_iter_get_basic(&iter, &response);
    req->response_code = (int)response;
  }
  else
  {
    req->response_code = 2;
    req->done = 1;
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (req->response_code != 0)
  {
    req->done = 1;
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (!dbus_message_iter_next(&iter) || dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
  {
    req->done = 1;
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  dbus_message_iter_recurse(&iter, &results_iter);

  while (dbus_message_iter_get_arg_type(&results_iter) == DBUS_TYPE_DICT_ENTRY)
  {
    DBusMessageIter entry_iter, variant_iter;
    const char* dict_key = NULL;

    dbus_message_iter_recurse(&results_iter, &entry_iter);

    if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_STRING)
      dbus_message_iter_get_basic(&entry_iter, &dict_key);

    if (dict_key && strcmp(dict_key, "uris") == 0 && dbus_message_iter_next(&entry_iter))
    {
      if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_VARIANT)
      {
        DBusMessageIter uri_array_iter;

        dbus_message_iter_recurse(&entry_iter, &variant_iter);

        if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY)
        {
          int count = 0, capacity = 8;

          req->uris = (char**)malloc(capacity * sizeof(char*));

          dbus_message_iter_recurse(&variant_iter, &uri_array_iter);

          while (dbus_message_iter_get_arg_type(&uri_array_iter) == DBUS_TYPE_STRING)
          {
            const char* uri = NULL;
            dbus_message_iter_get_basic(&uri_array_iter, &uri);

            if (uri)
            {
              if (count >= capacity)
              {
                capacity *= 2;
                req->uris = (char**)realloc(req->uris, capacity * sizeof(char*));
              }
              req->uris[count++] = portalUriToPath(uri);
            }

            dbus_message_iter_next(&uri_array_iter);
          }

          req->uri_count = count;
        }
      }
    }

    dbus_message_iter_next(&results_iter);
  }

  req->done = 1;
  return DBUS_HANDLER_RESULT_HANDLED;
}

/****************************************************************************
 * Wait for Portal Response
 ****************************************************************************/

static int portalWaitForResponse(PortalRequest* req, int timeout_seconds)
{
  DBusError error;
  char match_rule[600];
  int elapsed = 0;

  dbus_error_init(&error);

  dbus_connection_add_filter(req->connection, portalSignalFilter, req, NULL);

  snprintf(match_rule, sizeof(match_rule),
    "type='signal',"
    "interface='" PORTAL_REQUEST "',"
    "path='%s',"
    "member='Response'",
    req->request_path);

  dbus_bus_add_match(req->connection, match_rule, &error);
  if (dbus_error_is_set(&error))
  {
    dbus_error_free(&error);
    dbus_connection_remove_filter(req->connection, portalSignalFilter, req);
    return 0;
  }

  while (!req->done && elapsed < timeout_seconds * 1000)
  {
    dbus_connection_read_write(req->connection, 100);

    while (dbus_connection_get_dispatch_status(req->connection) == DBUS_DISPATCH_DATA_REMAINS)
    {
      dbus_connection_dispatch(req->connection);
      if (req->done)
        break;
    }

    elapsed += 100;
  }

  dbus_bus_remove_match(req->connection, match_rule, &error);
  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  dbus_connection_remove_filter(req->connection, portalSignalFilter, req);

  return req->done;
}

/****************************************************************************
 * Filter Conversion
 ****************************************************************************/

static char* portalGetNextStr(char* str)
{
  return str + strlen(str) + 1;
}

static void portalAppendFilters(DBusMessageIter* dict_iter, Ihandle* ih)
{
  char* value;
  DBusMessageIter dict_entry_iter, variant_iter, filter_array_iter;
  const char* key = "filters";
  int has_filters = 0;

  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
  {
    char* filters = iupStrDup(value);
    char* name;
    int pair_count;

    pair_count = iupStrReplace(filters, '|', 0) / 2;
    if (pair_count > 0)
    {
      has_filters = 1;

      dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(sa(us))", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(sa(us))", &filter_array_iter);

      name = filters;
      while (pair_count > 0 && name[0])
      {
        char* pattern_str = portalGetNextStr(name);
        char* patterns_dup;
        char* pat;
        int pat_count, j;
        DBusMessageIter filter_struct_iter, pattern_array_iter;

        dbus_message_iter_open_container(&filter_array_iter, DBUS_TYPE_STRUCT, NULL, &filter_struct_iter);
        dbus_message_iter_append_basic(&filter_struct_iter, DBUS_TYPE_STRING, &name);

        dbus_message_iter_open_container(&filter_struct_iter, DBUS_TYPE_ARRAY, "(us)", &pattern_array_iter);

        patterns_dup = iupStrDup(pattern_str);
        pat_count = iupStrReplace(patterns_dup, ';', 0) + 1;
        pat = patterns_dup;

        for (j = 0; j < pat_count; j++)
        {
          DBusMessageIter pat_struct_iter;
          dbus_uint32_t glob_type = 0;

          dbus_message_iter_open_container(&pattern_array_iter, DBUS_TYPE_STRUCT, NULL, &pat_struct_iter);
          dbus_message_iter_append_basic(&pat_struct_iter, DBUS_TYPE_UINT32, &glob_type);
          dbus_message_iter_append_basic(&pat_struct_iter, DBUS_TYPE_STRING, &pat);
          dbus_message_iter_close_container(&pattern_array_iter, &pat_struct_iter);

          if (j < pat_count - 1)
            pat = portalGetNextStr(pat);
        }

        free(patterns_dup);

        dbus_message_iter_close_container(&filter_struct_iter, &pattern_array_iter);
        dbus_message_iter_close_container(&filter_array_iter, &filter_struct_iter);

        name = portalGetNextStr(pattern_str);
        pair_count--;
      }

      dbus_message_iter_close_container(&variant_iter, &filter_array_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
    }

    free(filters);
  }
  else
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
    {
      char* info = iupAttribGet(ih, "FILTERINFO");
      char* patterns_dup;
      char* pat;
      int pat_count, j;
      DBusMessageIter filter_struct_iter, pattern_array_iter;

      if (!info)
        info = value;

      has_filters = 1;

      dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &key);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(sa(us))", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(sa(us))", &filter_array_iter);

      dbus_message_iter_open_container(&filter_array_iter, DBUS_TYPE_STRUCT, NULL, &filter_struct_iter);
      dbus_message_iter_append_basic(&filter_struct_iter, DBUS_TYPE_STRING, &info);

      dbus_message_iter_open_container(&filter_struct_iter, DBUS_TYPE_ARRAY, "(us)", &pattern_array_iter);

      patterns_dup = iupStrDup(value);
      pat_count = iupStrReplace(patterns_dup, ';', 0) + 1;
      pat = patterns_dup;

      for (j = 0; j < pat_count; j++)
      {
        DBusMessageIter pat_struct_iter;
        dbus_uint32_t glob_type = 0;

        dbus_message_iter_open_container(&pattern_array_iter, DBUS_TYPE_STRUCT, NULL, &pat_struct_iter);
        dbus_message_iter_append_basic(&pat_struct_iter, DBUS_TYPE_UINT32, &glob_type);
        dbus_message_iter_append_basic(&pat_struct_iter, DBUS_TYPE_STRING, &pat);
        dbus_message_iter_close_container(&pattern_array_iter, &pat_struct_iter);

        if (j < pat_count - 1)
          pat = portalGetNextStr(pat);
      }

      free(patterns_dup);

      dbus_message_iter_close_container(&filter_struct_iter, &pattern_array_iter);
      dbus_message_iter_close_container(&filter_array_iter, &filter_struct_iter);

      dbus_message_iter_close_container(&variant_iter, &filter_array_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
    }
  }

  (void)has_filters;
}

/****************************************************************************
 * Multiple Files Helper
 ****************************************************************************/

static void portalGetMultipleFiles(Ihandle* ih, char** uris, int uri_count)
{
  char* dir;
  int dir_len;

  if (!uris || uri_count == 0)
    return;

  dir = iupStrFileGetPath(uris[0]);
  dir_len = (int)strlen(dir);
  iupAttribSetStr(ih, "DIRECTORY", dir);

  if (uri_count == 1)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    iupAttribSetStrId(ih, "MULTIVALUE", 1, uris[0] + dir_len);
    iupAttribSetStr(ih, "VALUE", uris[0]);
    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));
    char* all_names;
    int cur_len, count = 0;
    int len = dir_len;
    int i;

    if (dir[dir_len - 1] == '/')
      len--;

    all_names = iupArrayAdd(names_array, len + 1);
    memcpy(all_names, dir, len);
    all_names[len] = '|';

    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);
    count++;

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    for (i = 0; i < uri_count; i++)
    {
      len = (int)strlen(uris[i]) - dir_len;
      if (len <= 0)
        continue;

      cur_len = iupArrayCount(names_array);

      all_names = iupArrayAdd(names_array, len + 1);
      memcpy(all_names + cur_len, uris[i] + dir_len, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", count, uris[i] + dir_len);
      count++;
    }

    iupAttribSetInt(ih, "MULTIVALUECOUNT", count);

    cur_len = iupArrayCount(names_array);
    all_names = iupArrayInc(names_array);
    all_names[cur_len] = 0;

    iupAttribSetStr(ih, "VALUE", all_names);

    iupArrayDestroy(names_array);
  }

  free(dir);
}

/****************************************************************************
 * File Extension Check
 ****************************************************************************/

static char* portalFileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = (int)strlen(filename);
    int ext_len = (int)strlen(ext);
    if (len > 0 && filename[len - ext_len - 1] != '.')
    {
      char* new_filename = (char*)malloc(len + ext_len + 2);
      memcpy(new_filename, filename, len);
      new_filename[len] = '.';
      memcpy(new_filename + len + 1, ext, ext_len);
      new_filename[len + ext_len + 1] = 0;
      return new_filename;
    }
  }
  return (char*)filename;
}

/****************************************************************************
 * Public API
 ****************************************************************************/

int iupUnixPortalAvailable(void)
{
  DBusConnection* connection;
  DBusMessage* msg;
  DBusMessage* reply;
  DBusError error;
  int available = 0;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return 0;
#endif

  dbus_error_init(&error);
  connection = dbus_bus_get(DBUS_BUS_SESSION, &error);

  if (!connection || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    return 0;
  }

  msg = dbus_message_new_method_call(
    "org.freedesktop.DBus",
    "/org/freedesktop/DBus",
    "org.freedesktop.DBus",
    "NameHasOwner");

  if (msg)
  {
    const char* name = PORTAL_BUS_NAME;
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);

    reply = dbus_connection_send_with_reply_and_block(connection, msg, 2000, &error);
    dbus_message_unref(msg);

    if (reply)
    {
      dbus_bool_t has_owner = FALSE;
      if (dbus_message_get_args(reply, NULL, DBUS_TYPE_BOOLEAN, &has_owner, DBUS_TYPE_INVALID))
        available = has_owner ? 1 : 0;
      dbus_message_unref(reply);
    }
  }

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  dbus_connection_unref(connection);
  return available;
}

int iupUnixPortalFileDialog(Ihandle* ih)
{
  DBusConnection* connection;
  DBusMessage* msg;
  DBusMessage* reply;
  DBusError error;
  DBusMessageIter iter, dict_iter;
  PortalRequest req;
  char token[64];
  const char* unique_name;
  char* dialogtype;
  char* value;
  int is_save = 0;
  int is_dir = 0;
  int is_multi = 0;
  const char* parent_window = "";
  const char* title;
  const char* method;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return IUP_ERROR;
#endif

  dbus_error_init(&error);
  connection = dbus_bus_get(DBUS_BUS_SESSION, &error);

  if (!connection || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    return IUP_ERROR;
  }

  if (!iupUnixPortalAvailable())
  {
    dbus_connection_unref(connection);
    return IUP_ERROR;
  }

  memset(&req, 0, sizeof(req));
  req.connection = connection;

  portalGenerateToken(token, sizeof(token));

  unique_name = dbus_bus_get_unique_name(connection);
  portalBuildRequestPath(unique_name, token, req.request_path, sizeof(req.request_path));

  dialogtype = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(dialogtype, "SAVE"))
    is_save = 1;
  else if (iupStrEqualNoCase(dialogtype, "DIR"))
    is_dir = 1;

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && !is_save && !is_dir)
    is_multi = 1;

  value = iupAttribGet(ih, "FILE");
  if (value && value[0] == '/')
  {
    char* dir = iupStrFileGetPath(value);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    free(dir);
  }

  title = iupAttribGet(ih, "TITLE");
  if (!title)
  {
    if (is_save)
      title = IupGetLanguageString("IUP_SAVEAS");
    else if (is_dir)
      title = IupGetLanguageString("IUP_SELECTDIR");
    else
      title = IupGetLanguageString("IUP_OPEN");
  }

  method = is_save ? "SaveFile" : "OpenFile";

  msg = dbus_message_new_method_call(PORTAL_BUS_NAME, PORTAL_OBJECT_PATH, PORTAL_FILECHOOSER, method);
  if (!msg)
  {
    dbus_connection_unref(connection);
    return IUP_ERROR;
  }

  dbus_message_iter_init_append(msg, &iter);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &parent_window);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &title);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

  portalAppendDictEntry_String(&dict_iter, "handle_token", token);

  if (is_dir)
    portalAppendDictEntry_Boolean(&dict_iter, "directory", TRUE);

  if (is_multi)
    portalAppendDictEntry_Boolean(&dict_iter, "multiple", TRUE);

  if (!is_dir)
    portalAppendFilters(&dict_iter, ih);

  value = iupAttribGet(ih, "DIRECTORY");
  if (value && value[0])
  {
    int dir_len = (int)strlen(value) + 1;
    portalAppendDictEntry_ByteArray(&dict_iter, "current_folder", (const unsigned char*)value, dir_len);
  }

  if (is_save)
  {
    value = iupAttribGet(ih, "FILE");
    if (value && value[0])
    {
      const char* basename = strrchr(value, '/');
      if (basename)
        basename++;
      else
        basename = value;
      portalAppendDictEntry_String(&dict_iter, "current_name", basename);
    }
  }

  dbus_message_iter_close_container(&iter, &dict_iter);

  reply = dbus_connection_send_with_reply_and_block(connection, msg, 5000, &error);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    if (reply)
      dbus_message_unref(reply);
    dbus_connection_unref(connection);
    return IUP_ERROR;
  }

  dbus_message_unref(reply);

  if (!portalWaitForResponse(&req, PORTAL_TIMEOUT_FILE))
  {
    portalFreeRequest(&req);
    dbus_connection_unref(connection);
    return IUP_ERROR;
  }

  if (req.response_code != 0 || !req.uris || req.uri_count == 0)
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "DIRECTORY", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "STATUS", "-1");

    portalFreeRequest(&req);
    dbus_connection_unref(connection);
    return IUP_NOERROR;
  }

  if (is_dir)
  {
    iupAttribSetStr(ih, "VALUE", req.uris[0]);
    iupAttribSet(ih, "STATUS", "0");
    iupAttribSet(ih, "FILEEXIST", NULL);
  }
  else if (is_multi)
  {
    portalGetMultipleFiles(ih, req.uris, req.uri_count);
    iupAttribSet(ih, "STATUS", "0");
    iupAttribSet(ih, "FILEEXIST", "YES");
  }
  else
  {
    char* filename = req.uris[0];
    char* final_filename = portalFileCheckExt(ih, filename);

    iupAttribSetStr(ih, "VALUE", final_filename);

    {
      char* dir = iupStrFileGetPath(final_filename);
      iupAttribSetStr(ih, "DIRECTORY", dir);
      free(dir);
    }

    if (portalIsDirectory(final_filename))
    {
      iupAttribSet(ih, "FILEEXIST", NULL);
      iupAttribSet(ih, "STATUS", "0");
    }
    else if (portalIsFile(final_filename))
    {
      iupAttribSet(ih, "FILEEXIST", "YES");
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      iupAttribSet(ih, "FILEEXIST", "NO");
      iupAttribSet(ih, "STATUS", "1");
    }

    if (final_filename != filename)
      free(final_filename);
  }

  if (!is_dir && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
  {
    char* cur_dir = iupAttribGet(ih, "DIRECTORY");
    if (cur_dir)
    {
      int ret = chdir(cur_dir);
      (void)ret;
    }
  }

  portalFreeRequest(&req);
  dbus_connection_unref(connection);
  return IUP_NOERROR;
}

int iupUnixPortalHelp(const char* url)
{
  DBusConnection* connection;
  DBusMessage* msg;
  DBusMessage* reply;
  DBusError error;
  DBusMessageIter iter, dict_iter;
  PortalRequest req;
  char token[64];
  const char* unique_name;
  const char* parent_window = "";

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return -1;
#endif

  dbus_error_init(&error);
  connection = dbus_bus_get(DBUS_BUS_SESSION, &error);

  if (!connection || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    return -1;
  }

  if (!iupUnixPortalAvailable())
  {
    dbus_connection_unref(connection);
    return -1;
  }

  memset(&req, 0, sizeof(req));
  req.connection = connection;

  portalGenerateToken(token, sizeof(token));

  unique_name = dbus_bus_get_unique_name(connection);
  portalBuildRequestPath(unique_name, token, req.request_path, sizeof(req.request_path));

  msg = dbus_message_new_method_call(PORTAL_BUS_NAME, PORTAL_OBJECT_PATH, PORTAL_OPENURI, "OpenURI");
  if (!msg)
  {
    dbus_connection_unref(connection);
    return -1;
  }

  dbus_message_iter_init_append(msg, &iter);

  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &parent_window);
  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &url);

  dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
  portalAppendDictEntry_String(&dict_iter, "handle_token", token);
  dbus_message_iter_close_container(&iter, &dict_iter);

  reply = dbus_connection_send_with_reply_and_block(connection, msg, 5000, &error);
  dbus_message_unref(msg);

  if (!reply || dbus_error_is_set(&error))
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    if (reply)
      dbus_message_unref(reply);
    dbus_connection_unref(connection);
    return -1;
  }

  dbus_message_unref(reply);

  portalWaitForResponse(&req, PORTAL_TIMEOUT_URI);

  portalFreeRequest(&req);
  dbus_connection_unref(connection);

  if (req.done && req.response_code == 0)
    return 1;

  return -1;
}
