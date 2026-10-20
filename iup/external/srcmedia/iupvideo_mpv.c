/** \file
 * \brief Video control, libmpv engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "iup_video.h"
#include "iup_str.h"
#include "iupvideo_mpv_dlopen.h"

enum
{
  IMPV_LOADFILE = 1,
  IMPV_TIME_POS,
  IMPV_DURATION,
  IMPV_PAUSE,
  IMPV_PAUSED_FOR_CACHE,
  IMPV_EOF_REACHED,
  IMPV_SEEKABLE,
  IMPV_DWIDTH,
  IMPV_DHEIGHT,
  IMPV_VID,
  IMPV_AID,
  IMPV_CORE_IDLE,
  IMPV_ROTATE,
  IMPV_TRACK_LIST
};

typedef struct
{
  IvideoSession* session;
  mpv_handle* player;
  mpv_render_context* renderer;
  pthread_t thread;
  pthread_mutex_t lock;
  pthread_cond_t condition;
  int initialized, started, dirty, quit, width, height;
  int paused, buffering, idle, eof, rotate;
  int64_t* subtitles;
  int64_t subtitle_auto;
  int subtitle_count, subtitle_capacity, subtitle_explicit;
  IvideoInfo info;
} IvideoMpv;

static int iVideoMpvLoad(void)
{
  return iupMpvOpen();
}

static void iVideoMpvUpdate(void* opaque)
{
  IvideoMpv* context = (IvideoMpv*)opaque;
  pthread_mutex_lock(&context->lock);
  context->dirty = 1;
  pthread_cond_signal(&context->condition);
  pthread_mutex_unlock(&context->lock);
}

static void* iVideoMpvRender(void* opaque)
{
  IvideoMpv* context = (IvideoMpv*)opaque;
  unsigned char* pixels = NULL;
  size_t capacity = 0;
  int frame = 0;
  for (;;)
  {
    int size[2], quit;
    size_t stride, bytes;
    mpv_render_param params[5];
    pthread_mutex_lock(&context->lock);
    while (!context->dirty && !context->quit)
      pthread_cond_wait(&context->condition, &context->lock);
    quit = context->quit;
    context->dirty = 0;
    size[0] = context->width;
    size[1] = context->height;
    pthread_mutex_unlock(&context->lock);
    if (quit)
      break;
    if (mpv_render_context_update(context->renderer) & MPV_RENDER_UPDATE_FRAME)
      frame = 1;
    if (!frame || size[0] <= 0 || size[1] <= 0)
      continue;
    if (size[0] > (INT_MAX - 63) / 4 || size[1] > INT_MAX / 4 / size[0])
    {
      iupVideoError(context->session, "Invalid mpv video dimensions");
      break;
    }
    frame = 0;
    stride = ((size_t)size[0] * 4 + 63) & ~(size_t)63;
    bytes = stride * size[1];
    if (bytes > capacity)
    {
      free(pixels);
      pixels = NULL;
      if (posix_memalign((void**)&pixels, 64, bytes) != 0)
      {
        iupVideoError(context->session, "Cannot allocate mpv video frame");
        break;
      }
      capacity = bytes;
    }
    params[0] = (mpv_render_param){MPV_RENDER_PARAM_SW_SIZE, size};
    params[1] = (mpv_render_param){MPV_RENDER_PARAM_SW_FORMAT, (void*)"rgb0"};
    params[2] = (mpv_render_param){MPV_RENDER_PARAM_SW_STRIDE, &stride};
    params[3] = (mpv_render_param){MPV_RENDER_PARAM_SW_POINTER, pixels};
    params[4] = (mpv_render_param){MPV_RENDER_PARAM_INVALID, NULL};
    if (mpv_render_context_render(context->renderer, params) < 0)
    {
      iupVideoError(context->session, "mpv video rendering failed");
      break;
    }
    iupVideoFrame(context->session, pixels, size[0], size[1], stride, 4);
    mpv_render_context_report_swap(context->renderer);
  }
  free(pixels);
  return NULL;
}

static void iVideoMpvClose(IvideoSession* session)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  if (!context)
    return;
  if (context->started)
  {
    pthread_mutex_lock(&context->lock);
    context->quit = 1;
    pthread_cond_signal(&context->condition);
    pthread_mutex_unlock(&context->lock);
    pthread_join(context->thread, NULL);
  }
  if (context->renderer)
  {
    mpv_render_context_set_update_callback(context->renderer, NULL, NULL);
    mpv_render_context_free(context->renderer);
  }
  if (context->player)
  {
    if (context->initialized)
      mpv_terminate_destroy(context->player);
    else
      mpv_destroy(context->player);
  }
  pthread_cond_destroy(&context->condition);
  pthread_mutex_destroy(&context->lock);
  free(context->subtitles);
  free(context);
  session->context = NULL;
}

static int iVideoMpvOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  IvideoMpv* context;
  mpv_render_param params[] = {{MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_SW}, {MPV_RENDER_PARAM_INVALID, NULL}};
  const char* command[] = {"loadfile", source, NULL};
  char* path = NULL;
  const char* flags[] = {"pause", "keep-open", "idle"};
  const char* disabled[] = {"terminal", "osc", "config", "load-scripts", "audio-display", "ytdl", "video-rotate", "sub-auto"};
  unsigned i;
  (void)ih;
  context = (IvideoMpv*)calloc(1, sizeof(IvideoMpv));
  if (!context)
    return 0;
  if (pthread_mutex_init(&context->lock, NULL) != 0)
  {
    free(context);
    return 0;
  }
  if (pthread_cond_init(&context->condition, NULL) != 0)
  {
    pthread_mutex_destroy(&context->lock);
    free(context);
    return 0;
  }
  session->context = context;
  context->session = session;
  context->paused = 1;
  context->idle = 1;
  context->player = mpv_create();
  if (!context->player)
  {
    iupVideoError(session, "Cannot create mpv player");
    return 0;
  }
  for (i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
  {
    if (mpv_set_option_string(context->player, flags[i], "yes") < 0)
    {
      iupVideoError(session, "Cannot configure mpv player");
      return 0;
    }
  }
  for (i = 0; i < sizeof(disabled) / sizeof(disabled[0]); i++)
  {
    int status = mpv_set_option_string(context->player, disabled[i], "no");
    if (status < 0 && status != MPV_ERROR_OPTION_NOT_FOUND)
    {
      iupVideoError(session, "Cannot configure mpv player");
      return 0;
    }
  }
  if (mpv_set_option_string(context->player, "vo", "libmpv") < 0 || mpv_initialize(context->player) < 0)
  {
    iupVideoError(session, "Cannot initialize mpv player");
    return 0;
  }
  context->initialized = 1;
  if (mpv_render_context_create(&context->renderer, context->player, params) < 0)
  {
    iupVideoError(session, "Cannot create mpv software renderer");
    return 0;
  }
  mpv_render_context_set_update_callback(context->renderer, iVideoMpvUpdate, context);
  if (pthread_create(&context->thread, NULL, iVideoMpvRender, context) != 0)
    return 0;
  context->started = 1;
  mpv_observe_property(context->player, IMPV_TIME_POS, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_observe_property(context->player, IMPV_DURATION, "duration", MPV_FORMAT_DOUBLE);
  mpv_observe_property(context->player, IMPV_PAUSE, "pause", MPV_FORMAT_FLAG);
  mpv_observe_property(context->player, IMPV_PAUSED_FOR_CACHE, "paused-for-cache", MPV_FORMAT_FLAG);
  mpv_observe_property(context->player, IMPV_EOF_REACHED, "eof-reached", MPV_FORMAT_FLAG);
  mpv_observe_property(context->player, IMPV_SEEKABLE, "seekable", MPV_FORMAT_FLAG);
  mpv_observe_property(context->player, IMPV_DWIDTH, "dwidth", MPV_FORMAT_INT64);
  mpv_observe_property(context->player, IMPV_DHEIGHT, "dheight", MPV_FORMAT_INT64);
  mpv_observe_property(context->player, IMPV_VID, "vid", MPV_FORMAT_INT64);
  mpv_observe_property(context->player, IMPV_AID, "aid", MPV_FORMAT_INT64);
  mpv_observe_property(context->player, IMPV_CORE_IDLE, "core-idle", MPV_FORMAT_FLAG);
  mpv_observe_property(context->player, IMPV_ROTATE, "video-dec-params/rotate", MPV_FORMAT_INT64);
  mpv_observe_property(context->player, IMPV_TRACK_LIST, "track-list", MPV_FORMAT_NODE);
  if (!url)
  {
    path = realpath(source, NULL);
    if (!path)
    {
      iupVideoError(session, "Cannot open this file");
      return 0;
    }
    command[1] = path;
  }
  if (mpv_command_async(context->player, IMPV_LOADFILE, command) < 0)
  {
    free(path);
    iupVideoError(session, "mpv cannot open this source");
    return 0;
  }
  free(path);
  return 1;
}

static void iVideoMpvPlay(IvideoSession* session, int play)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  int pause = !play;
  mpv_set_property_async(context->player, 0, "pause", MPV_FORMAT_FLAG, &pause);
}

static void iVideoMpvSeek(IvideoSession* session, double position)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  mpv_node arguments[3], command;
  mpv_node_list list;
  memset(arguments, 0, sizeof(arguments));
  arguments[0].format = MPV_FORMAT_STRING;
  arguments[0].u.string = (char*)"seek";
  arguments[1].format = MPV_FORMAT_DOUBLE;
  arguments[1].u.double_ = position;
  arguments[2].format = MPV_FORMAT_STRING;
  arguments[2].u.string = (char*)"absolute+exact";
  list.num = 3;
  list.values = arguments;
  list.keys = NULL;
  command.format = MPV_FORMAT_NODE_ARRAY;
  command.u.list = &list;
  mpv_command_node_async(context->player, 0, &command);
}

static void iVideoMpvVolume(IvideoSession* session, int volume, int mute)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  double value = volume;
  mpv_set_property_async(context->player, 0, "volume", MPV_FORMAT_DOUBLE, &value);
  mpv_set_property_async(context->player, 0, "mute", MPV_FORMAT_FLAG, &mute);
}

static void iVideoMpvRotate(IvideoMpv* context, int64_t rotate)
{
  const char* filter = "";
  if (rotate == context->rotate)
    return;
  context->rotate = (int)rotate;
  if (rotate == 90)
    filter = "lavfi=[transpose=clock]";
  else if (rotate == 180)
    filter = "lavfi=[hflip,vflip]";
  else if (rotate == 270)
    filter = "lavfi=[transpose=cclock]";
  mpv_set_property_async(context->player, 0, "vf", MPV_FORMAT_STRING, &filter);
}

static mpv_node* iVideoMpvField(mpv_node* node, const char* key)
{
  int i;
  if (node->format != MPV_FORMAT_NODE_MAP)
    return NULL;
  for (i = 0; i < node->u.list->num; i++)
  {
    if (strcmp(node->u.list->keys[i], key) == 0)
      return &node->u.list->values[i];
  }
  return NULL;
}

static int iVideoMpvFlag(mpv_node* node, const char* key)
{
  mpv_node* field = iVideoMpvField(node, key);
  return field && field->format == MPV_FORMAT_FLAG && field->u.flag;
}

static int iVideoMpvSubtitleTrack(mpv_node* node)
{
  mpv_node* type = iVideoMpvField(node, "type");
  return type && type->format == MPV_FORMAT_STRING && strcmp(type->u.string, "sub") == 0 && !iVideoMpvFlag(node, "external");
}

static void iVideoMpvTracks(IvideoMpv* context, mpv_node* tracks)
{
  int i, count = 0;
  int64_t selected = 0;
  context->subtitle_count = 0;
  if (tracks->format != MPV_FORMAT_NODE_ARRAY)
    return;
  for (i = 0; i < tracks->u.list->num; i++)
  {
    if (iVideoMpvSubtitleTrack(&tracks->u.list->values[i]))
      count++;
  }
  if (count > context->subtitle_capacity)
  {
    int64_t* subtitles = (int64_t*)realloc(context->subtitles, (size_t)count * sizeof(int64_t));
    if (!subtitles)
      return;
    context->subtitles = subtitles;
    context->subtitle_capacity = count;
  }
  for (i = 0; i < tracks->u.list->num; i++)
  {
    mpv_node* track = &tracks->u.list->values[i];
    mpv_node* id = iVideoMpvField(track, "id");
    if (!iVideoMpvSubtitleTrack(track) || !id || id->format != MPV_FORMAT_INT64)
      continue;
    context->subtitles[context->subtitle_count++] = id->u.int64;
    if (iVideoMpvFlag(track, "selected"))
      selected = id->u.int64;
  }
  if (!context->subtitle_explicit)
    context->subtitle_auto = selected;
}

static void iVideoMpvProperty(IvideoMpv* context, uint64_t id, void* data)
{
  switch (id)
  {
  case IMPV_TIME_POS:
    context->info.position = *(double*)data;
    break;
  case IMPV_DURATION:
    context->info.duration = *(double*)data;
    break;
  case IMPV_PAUSE:
    context->paused = *(int*)data;
    break;
  case IMPV_PAUSED_FOR_CACHE:
    context->buffering = *(int*)data;
    break;
  case IMPV_EOF_REACHED:
    if (*(int*)data && !context->eof)
      context->info.ended = 1;
    context->eof = *(int*)data;
    break;
  case IMPV_SEEKABLE:
    context->info.seekable = *(int*)data;
    break;
  case IMPV_DWIDTH:
    if (*(int64_t*)data > 0 && *(int64_t*)data <= INT_MAX)
      context->info.width = (int)*(int64_t*)data;
    break;
  case IMPV_DHEIGHT:
    if (*(int64_t*)data > 0 && *(int64_t*)data <= INT_MAX)
      context->info.height = (int)*(int64_t*)data;
    break;
  case IMPV_VID:
    context->info.video = *(int64_t*)data > 0;
    break;
  case IMPV_AID:
    context->info.audio = *(int64_t*)data > 0;
    break;
  case IMPV_CORE_IDLE:
    context->idle = *(int*)data;
    break;
  case IMPV_ROTATE:
    iVideoMpvRotate(context, *(int64_t*)data);
    break;
  case IMPV_TRACK_LIST:
    iVideoMpvTracks(context, (mpv_node*)data);
    break;
  }
}

static void iVideoMpvPoll(IvideoSession* session)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  mpv_event* event;
  unsigned events = 0;
  while (events++ < 256 && (event = mpv_wait_event(context->player, 0))->event_id != MPV_EVENT_NONE)
  {
    if (event->event_id == MPV_EVENT_PLAYBACK_RESTART)
      context->info.ready = 1;
    else if (event->event_id == MPV_EVENT_SHUTDOWN)
      iupVideoError(session, "mpv player shut down");
    else if (event->event_id == MPV_EVENT_END_FILE)
    {
      mpv_event_end_file* end = (mpv_event_end_file*)event->data;
      if (end->reason == MPV_END_FILE_REASON_ERROR)
        iupVideoError(session, mpv_error_string(end->error));
    }
    else if (event->event_id == MPV_EVENT_COMMAND_REPLY && event->reply_userdata == IMPV_LOADFILE && event->error < 0)
      iupVideoError(session, mpv_error_string(event->error));
    else if (event->event_id == MPV_EVENT_PROPERTY_CHANGE)
    {
      mpv_event_property* property = (mpv_event_property*)event->data;
      if (property->data)
        iVideoMpvProperty(context, event->reply_userdata, property->data);
    }
  }
  if (context->info.width > 0 && context->info.height > 0)
  {
    context->info.aspect = (double)context->info.width / context->info.height;
    pthread_mutex_lock(&context->lock);
    if (context->width != context->info.width || context->height != context->info.height)
    {
      context->width = context->info.width;
      context->height = context->info.height;
      context->dirty = 1;
      pthread_cond_signal(&context->condition);
    }
    pthread_mutex_unlock(&context->lock);
  }
  if (!context->info.ready)
    context->info.state = IVIDEO_LOADING;
  else if (context->buffering || (!context->paused && context->idle))
    context->info.state = IVIDEO_BUFFERING;
  else
    context->info.state = context->paused ? IVIDEO_PAUSED : IVIDEO_PLAYING;
  iupVideoInfo(session, &context->info);
  context->info.ended = 0;
}

static int iVideoMpvSubtitleCount(IvideoSession* session)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  return context->subtitle_count;
}

static char* iVideoMpvSubtitleName(IvideoSession* session, int index, int language)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  mpv_node tracks = {0};
  char* result = NULL;
  int i, count = 0;
  if (mpv_get_property(context->player, "track-list", MPV_FORMAT_NODE, &tracks) < 0)
    return NULL;
  if (tracks.format == MPV_FORMAT_NODE_ARRAY)
  {
    for (i = 0; i < tracks.u.list->num; i++)
    {
      mpv_node* field;
      if (!iVideoMpvSubtitleTrack(&tracks.u.list->values[i]) || ++count != index)
        continue;
      field = iVideoMpvField(&tracks.u.list->values[i], language ? "lang" : "title");
      if (field && field->format == MPV_FORMAT_STRING)
        result = iupStrReturnStr(field->u.string);
      else if (!language)
        result = iupStrReturnStrf("Track %d", index);
      break;
    }
  }
  mpv_free_node_contents(&tracks);
  return result;
}

static void iVideoMpvSubtitleSet(IvideoMpv* context, int64_t id)
{
  if (id > 0)
  {
    mpv_set_property_async(context->player, 0, "sid", MPV_FORMAT_INT64, &id);
  }
  else
  {
    const char* value = "no";
    mpv_set_property_async(context->player, 0, "sid", MPV_FORMAT_STRING, &value);
  }
}

static int iVideoMpvSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IvideoMpv* context = (IvideoMpv*)session->context;
  if (index > context->subtitle_count)
    return 0;
  if (index >= 0)
  {
    context->subtitle_explicit = 1;
    iVideoMpvSubtitleSet(context, index ? context->subtitles[index - 1] : 0);
  }
  else if (context->subtitle_explicit)
    iVideoMpvSubtitleSet(context, context->subtitle_auto);
  mpv_set_property_async(context->player, 0, "sub-delay", MPV_FORMAT_DOUBLE, &delay);
  return 1;
}

const IvideoBackend iupVideoMpvBackend = {"MPV", iVideoMpvLoad, iVideoMpvOpen, iVideoMpvClose, iVideoMpvPlay, iVideoMpvSeek, iVideoMpvVolume, iVideoMpvPoll, NULL, iVideoMpvSubtitleCount, iVideoMpvSubtitleName, iVideoMpvSubtitleSelect, NULL};
