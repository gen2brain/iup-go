/** \file
 * \brief libmpv 2 Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPVIDEO_MPV_DLOPEN_H
#define __IUPVIDEO_MPV_DLOPEN_H

#include <stdint.h>
#include <dlfcn.h>

typedef struct mpv_handle mpv_handle;
typedef struct mpv_render_context mpv_render_context;

typedef enum
{
  MPV_ERROR_SUCCESS = 0,
  MPV_ERROR_OPTION_NOT_FOUND = -5
} mpv_error;

typedef enum
{
  MPV_FORMAT_NONE = 0,
  MPV_FORMAT_STRING = 1,
  MPV_FORMAT_OSD_STRING = 2,
  MPV_FORMAT_FLAG = 3,
  MPV_FORMAT_INT64 = 4,
  MPV_FORMAT_DOUBLE = 5,
  MPV_FORMAT_NODE = 6,
  MPV_FORMAT_NODE_ARRAY = 7,
  MPV_FORMAT_NODE_MAP = 8,
  MPV_FORMAT_BYTE_ARRAY = 9
} mpv_format;

typedef enum
{
  MPV_EVENT_NONE = 0,
  MPV_EVENT_SHUTDOWN = 1,
  MPV_EVENT_COMMAND_REPLY = 5,
  MPV_EVENT_END_FILE = 7,
  MPV_EVENT_PLAYBACK_RESTART = 21,
  MPV_EVENT_PROPERTY_CHANGE = 22
} mpv_event_id;

typedef enum
{
  MPV_END_FILE_REASON_EOF = 0,
  MPV_END_FILE_REASON_STOP = 2,
  MPV_END_FILE_REASON_QUIT = 3,
  MPV_END_FILE_REASON_ERROR = 4,
  MPV_END_FILE_REASON_REDIRECT = 5
} mpv_end_file_reason;

typedef struct mpv_node
{
  union
  {
    char* string;
    int flag;
    int64_t int64;
    double double_;
    struct mpv_node_list* list;
    struct mpv_byte_array* ba;
  } u;
  mpv_format format;
} mpv_node;

typedef struct mpv_node_list
{
  int num;
  mpv_node* values;
  char** keys;
} mpv_node_list;

typedef struct
{
  const char* name;
  mpv_format format;
  void* data;
} mpv_event_property;

typedef struct
{
  mpv_end_file_reason reason;
  int error;
  int64_t playlist_entry_id;
  int64_t playlist_insert_id;
  int playlist_insert_num_entries;
} mpv_event_end_file;

typedef struct
{
  mpv_event_id event_id;
  int error;
  uint64_t reply_userdata;
  void* data;
} mpv_event;

enum mpv_render_param_type
{
  MPV_RENDER_PARAM_INVALID = 0,
  MPV_RENDER_PARAM_API_TYPE = 1,
  MPV_RENDER_PARAM_SW_SIZE = 17,
  MPV_RENDER_PARAM_SW_FORMAT = 18,
  MPV_RENDER_PARAM_SW_STRIDE = 19,
  MPV_RENDER_PARAM_SW_POINTER = 20
};

typedef struct
{
  enum mpv_render_param_type type;
  void* data;
} mpv_render_param;

#define MPV_RENDER_API_TYPE_SW "sw"
#define MPV_RENDER_UPDATE_FRAME 1

typedef void (*mpv_render_update_fn)(void* data);

static void* iupmpv_handle = NULL;

static unsigned long (*iupmpv_client_api_version)(void);
static const char* (*iupmpv_error_string)(int);
static mpv_handle* (*iupmpv_create)(void);
static int (*iupmpv_initialize)(mpv_handle*);
static void (*iupmpv_destroy)(mpv_handle*);
static void (*iupmpv_terminate_destroy)(mpv_handle*);
static int (*iupmpv_set_option_string)(mpv_handle*, const char*, const char*);
static int (*iupmpv_command_async)(mpv_handle*, uint64_t, const char**);
static int (*iupmpv_command_node_async)(mpv_handle*, uint64_t, mpv_node*);
static int (*iupmpv_set_property_async)(mpv_handle*, uint64_t, const char*, mpv_format, void*);
static int (*iupmpv_observe_property)(mpv_handle*, uint64_t, const char*, mpv_format);
static mpv_event* (*iupmpv_wait_event)(mpv_handle*, double);
static int (*iupmpv_get_property)(mpv_handle*, const char*, mpv_format, void*);
static void (*iupmpv_free_node_contents)(mpv_node*);
static int (*iupmpv_render_context_create)(mpv_render_context**, mpv_handle*, mpv_render_param*);
static void (*iupmpv_render_context_set_update_callback)(mpv_render_context*, mpv_render_update_fn, void*);
static uint64_t (*iupmpv_render_context_update)(mpv_render_context*);
static int (*iupmpv_render_context_render)(mpv_render_context*, mpv_render_param*);
static void (*iupmpv_render_context_report_swap)(mpv_render_context*);
static void (*iupmpv_render_context_free)(mpv_render_context*);

#define mpv_client_api_version iupmpv_client_api_version
#define mpv_error_string iupmpv_error_string
#define mpv_create iupmpv_create
#define mpv_initialize iupmpv_initialize
#define mpv_destroy iupmpv_destroy
#define mpv_terminate_destroy iupmpv_terminate_destroy
#define mpv_set_option_string iupmpv_set_option_string
#define mpv_command_async iupmpv_command_async
#define mpv_command_node_async iupmpv_command_node_async
#define mpv_set_property_async iupmpv_set_property_async
#define mpv_observe_property iupmpv_observe_property
#define mpv_wait_event iupmpv_wait_event
#define mpv_get_property iupmpv_get_property
#define mpv_free_node_contents iupmpv_free_node_contents
#define mpv_render_context_create iupmpv_render_context_create
#define mpv_render_context_set_update_callback iupmpv_render_context_set_update_callback
#define mpv_render_context_update iupmpv_render_context_update
#define mpv_render_context_render iupmpv_render_context_render
#define mpv_render_context_report_swap iupmpv_render_context_report_swap
#define mpv_render_context_free iupmpv_render_context_free

static void* iupMpvSymbol(const char* name, int* missing)
{
  void* symbol = dlsym(iupmpv_handle, name);
  if (!symbol)
    *missing = 1;
  return symbol;
}

static int iupMpvOpen(void)
{
  int missing = 0;

  if (iupmpv_handle)
    return 1;

  iupmpv_handle = dlopen("libmpv.so.2", RTLD_NOW | RTLD_LOCAL);
  if (!iupmpv_handle)
    return 0;

  *(void**)&iupmpv_client_api_version = iupMpvSymbol("mpv_client_api_version", &missing);
  *(void**)&iupmpv_error_string = iupMpvSymbol("mpv_error_string", &missing);
  *(void**)&iupmpv_create = iupMpvSymbol("mpv_create", &missing);
  *(void**)&iupmpv_initialize = iupMpvSymbol("mpv_initialize", &missing);
  *(void**)&iupmpv_destroy = iupMpvSymbol("mpv_destroy", &missing);
  *(void**)&iupmpv_terminate_destroy = iupMpvSymbol("mpv_terminate_destroy", &missing);
  *(void**)&iupmpv_set_option_string = iupMpvSymbol("mpv_set_option_string", &missing);
  *(void**)&iupmpv_command_async = iupMpvSymbol("mpv_command_async", &missing);
  *(void**)&iupmpv_command_node_async = iupMpvSymbol("mpv_command_node_async", &missing);
  *(void**)&iupmpv_set_property_async = iupMpvSymbol("mpv_set_property_async", &missing);
  *(void**)&iupmpv_observe_property = iupMpvSymbol("mpv_observe_property", &missing);
  *(void**)&iupmpv_wait_event = iupMpvSymbol("mpv_wait_event", &missing);
  *(void**)&iupmpv_get_property = iupMpvSymbol("mpv_get_property", &missing);
  *(void**)&iupmpv_free_node_contents = iupMpvSymbol("mpv_free_node_contents", &missing);
  *(void**)&iupmpv_render_context_create = iupMpvSymbol("mpv_render_context_create", &missing);
  *(void**)&iupmpv_render_context_set_update_callback = iupMpvSymbol("mpv_render_context_set_update_callback", &missing);
  *(void**)&iupmpv_render_context_update = iupMpvSymbol("mpv_render_context_update", &missing);
  *(void**)&iupmpv_render_context_render = iupMpvSymbol("mpv_render_context_render", &missing);
  *(void**)&iupmpv_render_context_report_swap = iupMpvSymbol("mpv_render_context_report_swap", &missing);
  *(void**)&iupmpv_render_context_free = iupMpvSymbol("mpv_render_context_free", &missing);

  if (missing || (mpv_client_api_version() >> 16) != 2)
  {
    dlclose(iupmpv_handle);
    iupmpv_handle = NULL;
    return 0;
  }

  return 1;
}

#endif
