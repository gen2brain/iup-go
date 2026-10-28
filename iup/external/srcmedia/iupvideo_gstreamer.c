/** \file
 * \brief Video control, GStreamer engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iup_video.h"
#include "iup_drv.h"
#include "iup_thread.h"
#include "iup_str.h"
#include "iupvideo_gstreamer_dlopen.h"

typedef struct
{
  IvideoSession* session;
  GstElement* player;
  GstElement* sink;
  GstElement* balance;
  GstElement* gamma;
  GstBus* bus;
  int play, buffering, live;
  int seeking, next_fast;
  unsigned int seek_ticks;
  double next_seek;
  int64_t restore_position;
  int width, height;
  double aspect;
  IvideoInfo info;
} IvideoGst;

static GstFlowReturn iVideoGstFrame(IvideoGst* context, GstSample* sample)
{
  GstStructure* structure;
  GstBuffer* buffer;
  GstCaps* caps;
  GstMapInfo map;
  int width = 0, height = 0, par_n = 1, par_d = 1;
  if (!sample)
    return GST_FLOW_FLUSHING;
  buffer = gst_sample_get_buffer(sample);
  caps = gst_sample_get_caps(sample);
  structure = caps ? gst_caps_get_structure(caps, 0) : NULL;
  if (buffer && structure && gst_structure_get_int(structure, "width", &width) && gst_structure_get_int(structure, "height", &height) &&
      width > 0 && height > 0 && gst_buffer_map(buffer, &map, GST_MAP_READ))
  {
    if (map.size / (size_t)height / 4 >= (size_t)width)
      iupVideoFrame(context->session, map.data, width, height, (size_t)width * 4);
    gst_buffer_unmap(buffer, &map);
    if (!gst_structure_get_fraction(structure, "pixel-aspect-ratio", &par_n, &par_d) || par_n <= 0 || par_d <= 0)
    {
      par_n = 1;
      par_d = 1;
    }
    iupdrvMutexLock(context->session->mutex);
    if (par_n > par_d)
    {
      context->width = (int)((long long)width * par_n / par_d);
      context->height = height;
    }
    else
    {
      context->width = width;
      context->height = (int)((long long)height * par_d / par_n);
    }
    context->aspect = (double)width * par_n / ((double)height * par_d);
    iupdrvMutexUnlock(context->session->mutex);
  }
  gst_mini_object_unref((GstMiniObject*)sample);
  return GST_FLOW_OK;
}

static GstFlowReturn iVideoGstSample(GstAppSink* sink, void* opaque)
{
  return iVideoGstFrame((IvideoGst*)opaque, gst_app_sink_pull_sample(sink));
}

static GstFlowReturn iVideoGstPreroll(GstAppSink* sink, void* opaque)
{
  return iVideoGstFrame((IvideoGst*)opaque, gst_app_sink_pull_preroll(sink));
}

static int iVideoGstAvailable(void)
{
  GstElementFactory* player;
  GstElementFactory* sink;
  int available;
  if (!iupGstOpen())
    return 0;
  player = gst_element_factory_find("playbin");
  sink = gst_element_factory_find("appsink");
  available = player != NULL && sink != NULL;
  if (player)
    g_object_unref(player);
  if (sink)
    g_object_unref(sink);
  return available;
}

static GstElement* iVideoGstDeviceSink(GstDevice* device)
{
  GstElement* sink = gst_device_create_element(device, NULL);
  GstElementFactory* factory;
  if (!sink)
    return NULL;
  g_object_ref_sink(sink);
  factory = gst_element_get_factory(sink);
  if (!factory || gst_plugin_feature_get_rank((GstPluginFeature*)factory) < 64)
  {
    g_object_unref(sink);
    return NULL;
  }
  return sink;
}

static int iVideoGstAudioSinks(IvideoAudioDevice** devices, const char* id, GstElement** found)
{
  GstDeviceMonitor* monitor = gst_device_monitor_new();
  GList* list;
  GList* item;
  int i, count = 0;
  if (!monitor)
    return 0;
  gst_device_monitor_set_show_all_devices(monitor, 1);
  gst_device_monitor_add_filter(monitor, "Audio/Sink", NULL);
  list = gst_device_monitor_get_devices(monitor);
  for (item = list; item; item = item->next)
  {
    char* name = gst_device_get_display_name((GstDevice*)item->data);
    GstElement* sink = name ? iVideoGstDeviceSink((GstDevice*)item->data) : NULL;
    int seen = 0;
    for (i = 0; devices && i < count; i++)
    {
      if (strcmp((*devices)[i].id, name) == 0)
        seen = 1;
    }
    if (sink && !seen)
    {
      if (found && !*found && strcmp(name, id) == 0)
      {
        *found = sink;
        sink = NULL;
      }
      if (devices)
        count = iupVideoAddAudioDevice(devices, count, name, name);
    }
    if (sink)
      g_object_unref(sink);
    g_free(name);
    g_object_unref(item->data);
  }
  g_list_free(list);
  g_object_unref(monitor);
  return count;
}

static GstElement* iVideoGstAudioSink(const char* id)
{
  GstElement* sink = NULL;
  if (id)
    iVideoGstAudioSinks(NULL, id, &sink);
  return sink;
}

static void iVideoGstClose(IvideoSession* session)
{
  IvideoGst* context = (IvideoGst*)session->context;
  if (!context)
    return;
  if (context->player)
    gst_element_set_state(context->player, GST_STATE_NULL);
  if (context->sink)
  {
    GstAppSinkCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    gst_app_sink_set_callbacks((GstAppSink*)context->sink, &callbacks, NULL, NULL);
    g_object_unref(context->sink);
  }
  if (context->bus)
    g_object_unref(context->bus);
  if (context->balance)
    g_object_unref(context->balance);
  if (context->gamma)
    g_object_unref(context->gamma);
  if (context->player)
    g_object_unref(context->player);
  free(context);
  session->context = NULL;
}

static int iVideoGstOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  IvideoGst* context;
  GstElement* flip;
  GstAppSinkCallbacks callbacks;
  GstCaps* caps;
  GstStateChangeReturn result;
  char* uri = NULL;
  unsigned flags = 0;
  (void)ih;
  context = (IvideoGst*)calloc(1, sizeof(IvideoGst));
  if (!context)
    return 0;
  session->context = context;
  context->session = session;
  context->next_seek = -1;
  context->player = gst_element_factory_make("playbin", NULL);
  context->sink = gst_element_factory_make("appsink", NULL);
  if (context->player)
    g_object_ref_sink(context->player);
  if (context->sink)
    g_object_ref_sink(context->sink);
  if (!context->player || !context->sink)
  {
    iupVideoError(session, "Cannot create GStreamer playbin");
    return 0;
  }
  caps = gst_caps_from_string("video/x-raw,format=BGRx");
  if (!caps)
    return 0;
  gst_app_sink_set_caps((GstAppSink*)context->sink, caps);
  gst_mini_object_unref((GstMiniObject*)caps);
  gst_app_sink_set_max_buffers((GstAppSink*)context->sink, 1);
  gst_app_sink_set_drop((GstAppSink*)context->sink, 1);
  gst_app_sink_set_wait_on_eos((GstAppSink*)context->sink, 0);
  g_object_set(context->sink, "sync", 1, NULL);
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.new_sample = iVideoGstSample;
  callbacks.new_preroll = iVideoGstPreroll;
  gst_app_sink_set_callbacks((GstAppSink*)context->sink, &callbacks, context, NULL);
  if (!url)
  {
    char* path = g_canonicalize_filename(source, NULL);
    if (path)
      uri = gst_filename_to_uri(path, NULL);
    g_free(path);
    if (!uri)
    {
      iupVideoError(session, "Invalid file name");
      return 0;
    }
  }
  g_object_get(context->player, "flags", &flags, NULL);
  g_object_set(context->player, "uri", url ? source : uri, "video-sink", context->sink, "flags", flags & ~4u, NULL);
  g_free(uri);
  if (session->audio_device)
  {
    GstElement* audio = iVideoGstAudioSink(session->audio_device);
    if (audio)
    {
      g_object_set(context->player, "audio-sink", audio, NULL);
      g_object_unref(audio);
    }
  }
  flip = gst_parse_bin_from_description("videoconvert ! videoflip video-direction=auto ! videobalance name=balance ! gamma name=gamma ! videoconvert", 1, NULL);
  if (flip)
  {
    context->balance = gst_bin_get_by_name((GstBin*)flip, "balance");
    context->gamma = gst_bin_get_by_name((GstBin*)flip, "gamma");
  }
  else
  {
    flip = gst_element_factory_make("videoflip", NULL);
    if (flip)
      gst_util_set_object_arg(flip, "video-direction", "auto");
  }
  if (flip)
    g_object_set(context->player, "video-filter", flip, NULL);
  context->bus = gst_element_get_bus(context->player);
  if (!context->bus)
    return 0;
  result = gst_element_set_state(context->player, GST_STATE_PAUSED);
  context->live = result == GST_STATE_CHANGE_NO_PREROLL;
  if (result == GST_STATE_CHANGE_FAILURE)
  {
    iupVideoError(session, "GStreamer cannot open this source");
    return 0;
  }
  return 1;
}

static void iVideoGstPlay(IvideoSession* session, int play)
{
  IvideoGst* context = (IvideoGst*)session->context;
  GstState state = play && (!context->buffering || context->live) ? GST_STATE_PLAYING : GST_STATE_PAUSED;
  context->play = play;
  if (gst_element_set_state(context->player, state) == GST_STATE_CHANGE_FAILURE)
    iupVideoError(session, "GStreamer playback state change failed");
}

static void iVideoGstSeek(IvideoSession* session, double position, int fast)
{
  IvideoGst* context = (IvideoGst*)session->context;
  GstSeekFlags flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE;
  if (fast)
    flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT | (fast > 0 ? GST_SEEK_FLAG_SNAP_AFTER : GST_SEEK_FLAG_SNAP_BEFORE);
  if (position > (double)INT64_MAX / GST_SECOND)
    return;
  if (context->seeking)
  {
    context->next_seek = position;
    context->next_fast = fast;
    return;
  }
  context->seeking = gst_element_seek_simple(context->player, GST_FORMAT_TIME, flags, (int64_t)(position * GST_SECOND)) != 0;
  context->seek_ticks = iupdrvGetTickCount();
}

static void iVideoGstSeekDone(IvideoSession* session, IvideoGst* context)
{
  context->seeking = 0;
  if (context->next_seek >= 0)
  {
    double position = context->next_seek;
    context->next_seek = -1;
    iVideoGstSeek(session, position, context->next_fast);
  }
}

static void iVideoGstVolume(IvideoSession* session, int volume, int mute)
{
  IvideoGst* context = (IvideoGst*)session->context;
  g_object_set(context->player, "volume", volume / 100.0, "mute", mute ? 1 : 0, NULL);
}

static void iVideoGstMessages(IvideoSession* session, IvideoGst* context)
{
  GstMessage* message;
  unsigned events = 0;
  while (events++ < 256 && (message = gst_bus_pop(context->bus)) != NULL)
  {
    if (message->type == GST_MESSAGE_ERROR)
    {
      GError* error = NULL;
      char* debug = NULL;
      gst_message_parse_error(message, &error, &debug);
      iupVideoError(session, error ? error->message : "GStreamer playback failed");
      if (error)
        g_error_free(error);
      g_free(debug);
    }
    else if (message->type == GST_MESSAGE_EOS)
      context->info.ended = 1;
    else if (message->type == GST_MESSAGE_ASYNC_DONE && context->seeking)
      iVideoGstSeekDone(session, context);
    else if (message->type == GST_MESSAGE_BUFFERING)
    {
      int percent;
      gst_message_parse_buffering(message, &percent);
      context->buffering = percent < 100;
      if (!context->live)
        gst_element_set_state(context->player, context->play && !context->buffering ? GST_STATE_PLAYING : GST_STATE_PAUSED);
    }
    gst_mini_object_unref((GstMiniObject*)message);
  }
}

static void iVideoGstPoll(IvideoSession* session)
{
  IvideoGst* context = (IvideoGst*)session->context;
  GstState state = GST_STATE_NULL, pending = GST_STATE_VOID_PENDING;
  int64_t time;
  GstQuery* query;
  int video = 0, audio = 0;
  iVideoGstMessages(session, context);
  if (context->seeking && iupdrvGetTickCount() - context->seek_ticks > 1000)
    iVideoGstSeekDone(session, context);
  if (gst_element_get_state(context->player, &state, &pending, 0) == GST_STATE_CHANGE_FAILURE)
  {
    iupVideoError(session, "GStreamer playback state query failed");
    return;
  }
  if (context->restore_position > 0 && pending == GST_STATE_VOID_PENDING && state >= GST_STATE_PAUSED)
  {
    gst_element_seek_simple(context->player, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, context->restore_position);
    context->restore_position = 0;
  }
  if (state >= GST_STATE_PAUSED || context->live)
    context->info.ready = 1;
  if (context->buffering)
    context->info.state = IVIDEO_BUFFERING;
  else
    context->info.state = state == GST_STATE_PLAYING ? IVIDEO_PLAYING : IVIDEO_PAUSED;
  if (gst_element_query_position(context->player, GST_FORMAT_TIME, &time) && time >= 0)
    context->info.position = (double)time / GST_SECOND;
  if (gst_element_query_duration(context->player, GST_FORMAT_TIME, &time) && time >= 0)
    context->info.duration = (double)time / GST_SECOND;
  query = gst_query_new_seeking(GST_FORMAT_TIME);
  if (query)
  {
    int seekable = 0;
    if (gst_element_query(context->player, query))
      gst_query_parse_seeking(query, NULL, &seekable, NULL, NULL);
    context->info.seekable = seekable;
    gst_mini_object_unref((GstMiniObject*)query);
  }
  g_object_get(context->player, "n-video", &video, "n-audio", &audio, NULL);
  context->info.video = video > 0;
  context->info.audio = audio > 0;
  iupdrvMutexLock(session->mutex);
  context->info.width = context->width;
  context->info.height = context->height;
  context->info.aspect = context->aspect;
  iupdrvMutexUnlock(session->mutex);
  iupVideoInfo(session, &context->info);
  context->info.ended = 0;
}

static int iVideoGstSubtitleCount(IvideoSession* session)
{
  IvideoGst* context = (IvideoGst*)session->context;
  int count = 0;
  g_object_get(context->player, "n-text", &count, NULL);
  return count;
}

static char* iVideoGstSubtitleName(IvideoSession* session, int index, int language)
{
  IvideoGst* context = (IvideoGst*)session->context;
  void* tags = NULL;
  char* value = NULL;
  char* result = NULL;
  if (index <= 0 || index > iVideoGstSubtitleCount(session))
    return NULL;
  g_signal_emit_by_name(context->player, "get-text-tags", index - 1, &tags);
  if (tags)
  {
    if (gst_tag_list_get_string(tags, language ? "language-code" : "title", &value))
    {
      result = iupStrReturnStr(value);
      g_free(value);
    }
    gst_mini_object_unref((GstMiniObject*)tags);
  }
  return result ? result : (language ? NULL : iupStrReturnStrf("Track %d", index));
}

static int iVideoGstSubtitleDefault(IvideoGst* context, int count)
{
  int i;
  for (i = 0; i < count; i++)
  {
    GstPad* pad = NULL;
    GstEvent* event;
    GstStreamFlags flags = GST_STREAM_FLAG_NONE;
    g_signal_emit_by_name(context->player, "get-text-pad", i, &pad);
    if (!pad)
      continue;
    event = gst_pad_get_sticky_event(pad, GST_EVENT_STREAM_START, 0);
    if (event)
    {
      gst_event_parse_stream_flags(event, &flags);
      gst_mini_object_unref((GstMiniObject*)event);
    }
    g_object_unref(pad);
    if (flags & GST_STREAM_FLAG_SELECT)
      return i;
  }
  return -1;
}

static int iVideoGstSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IvideoGst* context = (IvideoGst*)session->context;
  GstState state = GST_STATE_NULL;
  unsigned flags = 0, old_flags = 0;
  int64_t offset = (int64_t)(-delay * GST_SECOND), old_offset = 0, position;
  int count = iVideoGstSubtitleCount(session), current = -1, old_current = -1;
  if (index > count)
    return 0;
  g_object_get(context->player, "flags", &old_flags, "current-text", &old_current, "text-offset", &old_offset, NULL);
  if (index < 0)
    index = iVideoGstSubtitleDefault(context, count) + 1;
  flags = index ? old_flags | 4u : old_flags & ~4u;
  current = index ? index - 1 : old_current;
  if (flags == old_flags && current == old_current && offset == old_offset)
    return 1;
  g_object_set(context->player, "flags", flags, "text-offset", offset, NULL);
  if (current != old_current)
    g_object_set(context->player, "current-text", current, NULL);
  gst_element_get_state(context->player, &state, NULL, 0);
  if (state == GST_STATE_PAUSED && gst_element_query_position(context->player, GST_FORMAT_TIME, &position) && position >= 0)
    gst_element_seek_simple(context->player, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, position);
  return 1;
}

static int iVideoGstAudioDevices(IvideoAudioDevice** devices)
{
  if (!iVideoGstAvailable())
    return 0;
  return iVideoGstAudioSinks(devices, NULL, NULL);
}

static void iVideoGstAudioDevice(IvideoSession* session, const char* id)
{
  IvideoGst* context = (IvideoGst*)session->context;
  GstElement* sink = id ? iVideoGstAudioSink(id) : gst_element_factory_make("autoaudiosink", NULL);
  int64_t position = -1;
  if (!sink)
    return;
  if (gst_element_query_position(context->player, GST_FORMAT_TIME, &position) && position > 0)
    context->restore_position = position;
  gst_element_set_state(context->player, GST_STATE_READY);
  g_object_set(context->player, "audio-sink", sink, NULL);
  if (id)
    g_object_unref(sink);
  iVideoGstPlay(session, context->play);
}

static void iVideoGstAdjust(IvideoSession* session, const int* values)
{
  IvideoGst* context = (IvideoGst*)session->context;
  double contrast = 1 + values[IVIDEO_CONTRAST] / 100.0;
  double brightness = values[IVIDEO_BRIGHTNESS] / 100.0 + (1 - contrast) / 2;
  if (brightness > 1) brightness = 1;
  if (brightness < -1) brightness = -1;
  if (context->balance)
    g_object_set(context->balance, "brightness", brightness, "contrast", contrast, "saturation", 1 + values[IVIDEO_SATURATION] / 100.0, NULL);
  if (context->gamma)
    g_object_set(context->gamma, "gamma", pow(2, values[IVIDEO_GAMMA] / 50.0), NULL);
}

const IvideoBackend iupVideoGstreamerBackend = {"GSTREAMER", iVideoGstAvailable, iVideoGstOpen, iVideoGstClose, iVideoGstPlay, iVideoGstSeek, iVideoGstVolume, iVideoGstPoll, NULL, iVideoGstSubtitleCount, iVideoGstSubtitleName, iVideoGstSubtitleSelect, NULL, 0, iVideoGstAudioDevices, iVideoGstAudioDevice, iVideoGstAdjust};
