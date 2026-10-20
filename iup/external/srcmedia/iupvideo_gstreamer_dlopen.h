/** \file
 * \brief GStreamer 1.x Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPVIDEO_GSTREAMER_DLOPEN_H
#define __IUPVIDEO_GSTREAMER_DLOPEN_H

#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>

typedef struct _GstElement GstElement;
typedef struct _GstElementFactory GstElementFactory;
typedef struct _GstBus GstBus;
typedef struct _GstQuery GstQuery;
typedef struct _GstCaps GstCaps;
typedef struct _GstStructure GstStructure;
typedef struct _GstSample GstSample;
typedef struct _GstBuffer GstBuffer;
typedef struct _GstMemory GstMemory;
typedef struct _GstAppSink GstAppSink;
typedef struct _GstPad GstPad;
typedef struct _GstEvent GstEvent;

typedef enum
{
  GST_STATE_VOID_PENDING = 0,
  GST_STATE_NULL = 1,
  GST_STATE_READY = 2,
  GST_STATE_PAUSED = 3,
  GST_STATE_PLAYING = 4
} GstState;

typedef enum
{
  GST_STATE_CHANGE_FAILURE = 0,
  GST_STATE_CHANGE_SUCCESS = 1,
  GST_STATE_CHANGE_ASYNC = 2,
  GST_STATE_CHANGE_NO_PREROLL = 3
} GstStateChangeReturn;

typedef enum
{
  GST_MESSAGE_EOS = 1 << 0,
  GST_MESSAGE_ERROR = 1 << 1,
  GST_MESSAGE_BUFFERING = 1 << 5
} GstMessageType;

typedef enum
{
  GST_FLOW_EOS = -3,
  GST_FLOW_FLUSHING = -2,
  GST_FLOW_OK = 0
} GstFlowReturn;

typedef enum
{
  GST_FORMAT_TIME = 3
} GstFormat;

typedef enum
{
  GST_SEEK_FLAG_FLUSH = 1 << 0,
  GST_SEEK_FLAG_ACCURATE = 1 << 1
} GstSeekFlags;

typedef enum
{
  GST_MAP_READ = 1 << 0
} GstMapFlags;

typedef enum
{
  GST_EVENT_STREAM_START = (40 << 8) | (1 << 1) | (1 << 2) | (1 << 3)
} GstEventType;

typedef enum
{
  GST_STREAM_FLAG_NONE = 0,
  GST_STREAM_FLAG_SELECT = 1 << 1
} GstStreamFlags;

#define GST_SECOND ((int64_t)1000000000)

typedef struct
{
  size_t type;
  int refcount;
  int lockstate;
  unsigned int flags;
  void* copy;
  void* dispose;
  void* free;
  unsigned int priv_uint;
  void* priv_pointer;
} GstMiniObject;

typedef struct
{
  GstMiniObject mini_object;
  GstMessageType type;
} GstMessage;

typedef struct
{
  uint32_t domain;
  int code;
  char* message;
} GError;

typedef struct
{
  GstMemory* memory;
  GstMapFlags flags;
  uint8_t* data;
  size_t size;
  size_t maxsize;
  void* user_data[4];
  void* _gst_reserved[4];
} GstMapInfo;

typedef struct
{
  void (*eos)(GstAppSink* sink, void* data);
  GstFlowReturn (*new_preroll)(GstAppSink* sink, void* data);
  GstFlowReturn (*new_sample)(GstAppSink* sink, void* data);
  void* _gst_reserved[4];
} GstAppSinkCallbacks;

static int iupgst_loaded = 0;
static int iupgst_failed = 0;

static int (*iupgst_init_check)(int*, char***, GError**);
static void (*iupgst_version)(unsigned*, unsigned*, unsigned*, unsigned*);
static GstElement* (*iupgst_element_factory_make)(const char*, const char*);
static GstElementFactory* (*iupgst_element_factory_find)(const char*);
static GstStateChangeReturn (*iupgst_element_set_state)(GstElement*, GstState);
static GstStateChangeReturn (*iupgst_element_get_state)(GstElement*, GstState*, GstState*, uint64_t);
static GstBus* (*iupgst_element_get_bus)(GstElement*);
static int (*iupgst_element_seek_simple)(GstElement*, GstFormat, GstSeekFlags, int64_t);
static int (*iupgst_element_query_position)(GstElement*, GstFormat, int64_t*);
static int (*iupgst_element_query_duration)(GstElement*, GstFormat, int64_t*);
static int (*iupgst_element_query)(GstElement*, GstQuery*);
static GstMessage* (*iupgst_bus_pop)(GstBus*);
static void (*iupgst_message_parse_error)(GstMessage*, GError**, char**);
static void (*iupgst_message_parse_buffering)(GstMessage*, int*);
static GstQuery* (*iupgst_query_new_seeking)(GstFormat);
static void (*iupgst_query_parse_seeking)(GstQuery*, GstFormat*, int*, int64_t*, int64_t*);
static void (*iupgst_mini_object_unref)(GstMiniObject*);
static GstCaps* (*iupgst_caps_from_string)(const char*);
static GstStructure* (*iupgst_caps_get_structure)(const GstCaps*, unsigned);
static int (*iupgst_structure_get_int)(const GstStructure*, const char*, int*);
static int (*iupgst_structure_get_fraction)(const GstStructure*, const char*, int*, int*);
static GstBuffer* (*iupgst_sample_get_buffer)(GstSample*);
static GstCaps* (*iupgst_sample_get_caps)(GstSample*);
static int (*iupgst_buffer_map)(GstBuffer*, GstMapInfo*, GstMapFlags);
static void (*iupgst_buffer_unmap)(GstBuffer*, GstMapInfo*);
static char* (*iupgst_filename_to_uri)(const char*, GError**);
static void (*iupgst_util_set_object_arg)(void*, const char*, const char*);
static void (*iupgst_app_sink_set_callbacks)(GstAppSink*, GstAppSinkCallbacks*, void*, void (*)(void*));
static void (*iupgst_app_sink_set_caps)(GstAppSink*, const GstCaps*);
static void (*iupgst_app_sink_set_max_buffers)(GstAppSink*, unsigned);
static void (*iupgst_app_sink_set_drop)(GstAppSink*, int);
static void (*iupgst_app_sink_set_wait_on_eos)(GstAppSink*, int);
static GstSample* (*iupgst_app_sink_pull_sample)(GstAppSink*);
static GstSample* (*iupgst_app_sink_pull_preroll)(GstAppSink*);
static void (*iupgst_g_object_set)(void*, const char*, ...);
static void (*iupgst_g_object_get)(void*, const char*, ...);
static void (*iupgst_g_object_unref)(void*);
static void* (*iupgst_g_object_ref_sink)(void*);
static void (*iupgst_g_error_free)(GError*);
static void (*iupgst_g_free)(void*);
static char* (*iupgst_g_canonicalize_filename)(const char*, const char*);
static void (*iupgst_g_signal_emit_by_name)(void*, const char*, ...);
static int (*iupgst_tag_list_get_string)(const void*, const char*, char**);
static GstEvent* (*iupgst_pad_get_sticky_event)(GstPad*, GstEventType, unsigned);
static void (*iupgst_event_parse_stream_flags)(GstEvent*, GstStreamFlags*);

#define gst_element_factory_make iupgst_element_factory_make
#define gst_element_factory_find iupgst_element_factory_find
#define gst_element_set_state iupgst_element_set_state
#define gst_element_get_state iupgst_element_get_state
#define gst_element_get_bus iupgst_element_get_bus
#define gst_element_seek_simple iupgst_element_seek_simple
#define gst_element_query_position iupgst_element_query_position
#define gst_element_query_duration iupgst_element_query_duration
#define gst_element_query iupgst_element_query
#define gst_bus_pop iupgst_bus_pop
#define gst_message_parse_error iupgst_message_parse_error
#define gst_message_parse_buffering iupgst_message_parse_buffering
#define gst_query_new_seeking iupgst_query_new_seeking
#define gst_query_parse_seeking iupgst_query_parse_seeking
#define gst_mini_object_unref iupgst_mini_object_unref
#define gst_caps_from_string iupgst_caps_from_string
#define gst_caps_get_structure iupgst_caps_get_structure
#define gst_structure_get_int iupgst_structure_get_int
#define gst_structure_get_fraction iupgst_structure_get_fraction
#define gst_sample_get_buffer iupgst_sample_get_buffer
#define gst_sample_get_caps iupgst_sample_get_caps
#define gst_buffer_map iupgst_buffer_map
#define gst_buffer_unmap iupgst_buffer_unmap
#define gst_filename_to_uri iupgst_filename_to_uri
#define gst_util_set_object_arg iupgst_util_set_object_arg
#define gst_app_sink_set_callbacks iupgst_app_sink_set_callbacks
#define gst_app_sink_set_caps iupgst_app_sink_set_caps
#define gst_app_sink_set_max_buffers iupgst_app_sink_set_max_buffers
#define gst_app_sink_set_drop iupgst_app_sink_set_drop
#define gst_app_sink_set_wait_on_eos iupgst_app_sink_set_wait_on_eos
#define gst_app_sink_pull_sample iupgst_app_sink_pull_sample
#define gst_app_sink_pull_preroll iupgst_app_sink_pull_preroll
#define g_object_set iupgst_g_object_set
#define g_object_get iupgst_g_object_get
#define g_object_unref iupgst_g_object_unref
#define g_object_ref_sink iupgst_g_object_ref_sink
#define g_error_free iupgst_g_error_free
#define g_free iupgst_g_free
#define g_canonicalize_filename iupgst_g_canonicalize_filename
#define g_signal_emit_by_name iupgst_g_signal_emit_by_name
#define gst_tag_list_get_string iupgst_tag_list_get_string
#define gst_pad_get_sticky_event iupgst_pad_get_sticky_event
#define gst_event_parse_stream_flags iupgst_event_parse_stream_flags

static void* iupGstSymbol(void* handle, const char* name, int* missing)
{
  void* symbol = dlsym(handle, name);
  if (!symbol)
    *missing = 1;
  return symbol;
}

static int iupGstOpen(void)
{
  void* handles[4];
  unsigned major = 0, minor, micro, nano;
  int i, missing = 0;

  if (iupgst_loaded)
    return 1;
  if (iupgst_failed)
    return 0;

  handles[0] = dlopen("libgstreamer-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
  handles[1] = dlopen("libgstapp-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
  handles[2] = dlopen("libgobject-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
  handles[3] = dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
  for (i = 0; i < 4; i++)
  {
    if (!handles[i])
      missing = 1;
  }

  if (!missing)
  {
    *(void**)&iupgst_init_check = iupGstSymbol(handles[0], "gst_init_check", &missing);
    *(void**)&iupgst_version = iupGstSymbol(handles[0], "gst_version", &missing);
    *(void**)&iupgst_element_factory_make = iupGstSymbol(handles[0], "gst_element_factory_make", &missing);
    *(void**)&iupgst_element_factory_find = iupGstSymbol(handles[0], "gst_element_factory_find", &missing);
    *(void**)&iupgst_element_set_state = iupGstSymbol(handles[0], "gst_element_set_state", &missing);
    *(void**)&iupgst_element_get_state = iupGstSymbol(handles[0], "gst_element_get_state", &missing);
    *(void**)&iupgst_element_get_bus = iupGstSymbol(handles[0], "gst_element_get_bus", &missing);
    *(void**)&iupgst_element_seek_simple = iupGstSymbol(handles[0], "gst_element_seek_simple", &missing);
    *(void**)&iupgst_element_query_position = iupGstSymbol(handles[0], "gst_element_query_position", &missing);
    *(void**)&iupgst_element_query_duration = iupGstSymbol(handles[0], "gst_element_query_duration", &missing);
    *(void**)&iupgst_element_query = iupGstSymbol(handles[0], "gst_element_query", &missing);
    *(void**)&iupgst_bus_pop = iupGstSymbol(handles[0], "gst_bus_pop", &missing);
    *(void**)&iupgst_message_parse_error = iupGstSymbol(handles[0], "gst_message_parse_error", &missing);
    *(void**)&iupgst_message_parse_buffering = iupGstSymbol(handles[0], "gst_message_parse_buffering", &missing);
    *(void**)&iupgst_query_new_seeking = iupGstSymbol(handles[0], "gst_query_new_seeking", &missing);
    *(void**)&iupgst_query_parse_seeking = iupGstSymbol(handles[0], "gst_query_parse_seeking", &missing);
    *(void**)&iupgst_mini_object_unref = iupGstSymbol(handles[0], "gst_mini_object_unref", &missing);
    *(void**)&iupgst_caps_from_string = iupGstSymbol(handles[0], "gst_caps_from_string", &missing);
    *(void**)&iupgst_caps_get_structure = iupGstSymbol(handles[0], "gst_caps_get_structure", &missing);
    *(void**)&iupgst_structure_get_int = iupGstSymbol(handles[0], "gst_structure_get_int", &missing);
    *(void**)&iupgst_structure_get_fraction = iupGstSymbol(handles[0], "gst_structure_get_fraction", &missing);
    *(void**)&iupgst_sample_get_buffer = iupGstSymbol(handles[0], "gst_sample_get_buffer", &missing);
    *(void**)&iupgst_sample_get_caps = iupGstSymbol(handles[0], "gst_sample_get_caps", &missing);
    *(void**)&iupgst_buffer_map = iupGstSymbol(handles[0], "gst_buffer_map", &missing);
    *(void**)&iupgst_buffer_unmap = iupGstSymbol(handles[0], "gst_buffer_unmap", &missing);
    *(void**)&iupgst_filename_to_uri = iupGstSymbol(handles[0], "gst_filename_to_uri", &missing);
    *(void**)&iupgst_util_set_object_arg = iupGstSymbol(handles[0], "gst_util_set_object_arg", &missing);
    *(void**)&iupgst_app_sink_set_callbacks = iupGstSymbol(handles[1], "gst_app_sink_set_callbacks", &missing);
    *(void**)&iupgst_app_sink_set_caps = iupGstSymbol(handles[1], "gst_app_sink_set_caps", &missing);
    *(void**)&iupgst_app_sink_set_max_buffers = iupGstSymbol(handles[1], "gst_app_sink_set_max_buffers", &missing);
    *(void**)&iupgst_app_sink_set_drop = iupGstSymbol(handles[1], "gst_app_sink_set_drop", &missing);
    *(void**)&iupgst_app_sink_set_wait_on_eos = iupGstSymbol(handles[1], "gst_app_sink_set_wait_on_eos", &missing);
    *(void**)&iupgst_app_sink_pull_sample = iupGstSymbol(handles[1], "gst_app_sink_pull_sample", &missing);
    *(void**)&iupgst_app_sink_pull_preroll = iupGstSymbol(handles[1], "gst_app_sink_pull_preroll", &missing);
    *(void**)&iupgst_g_object_set = iupGstSymbol(handles[2], "g_object_set", &missing);
    *(void**)&iupgst_g_object_get = iupGstSymbol(handles[2], "g_object_get", &missing);
    *(void**)&iupgst_g_object_unref = iupGstSymbol(handles[2], "g_object_unref", &missing);
    *(void**)&iupgst_g_object_ref_sink = iupGstSymbol(handles[2], "g_object_ref_sink", &missing);
    *(void**)&iupgst_g_error_free = iupGstSymbol(handles[3], "g_error_free", &missing);
    *(void**)&iupgst_g_free = iupGstSymbol(handles[3], "g_free", &missing);
    *(void**)&iupgst_g_canonicalize_filename = iupGstSymbol(handles[3], "g_canonicalize_filename", &missing);
    *(void**)&iupgst_g_signal_emit_by_name = iupGstSymbol(handles[2], "g_signal_emit_by_name", &missing);
    *(void**)&iupgst_tag_list_get_string = iupGstSymbol(handles[0], "gst_tag_list_get_string", &missing);
    *(void**)&iupgst_pad_get_sticky_event = iupGstSymbol(handles[0], "gst_pad_get_sticky_event", &missing);
    *(void**)&iupgst_event_parse_stream_flags = iupGstSymbol(handles[0], "gst_event_parse_stream_flags", &missing);
  }

  if (!missing)
    iupgst_version(&major, &minor, &micro, &nano);

  if (missing || major != 1 || !iupgst_init_check(NULL, NULL, NULL))
  {
    for (i = 3; i >= 0; i--)
    {
      if (handles[i])
        dlclose(handles[i]);
    }
    iupgst_failed = 1;
    return 0;
  }

  iupgst_loaded = 1;
  return 1;
}

#endif
