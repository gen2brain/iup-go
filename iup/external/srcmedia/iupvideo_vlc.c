/** \file
 * \brief Video control, libVLC engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "iup_video.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iupvideo_vlc_dlopen.h"

typedef struct
{
  IvideoSession* session;
  libvlc_instance_t* instance;
  libvlc_media_t* media;
  libvlc_media_player_t* player;
  libvlc_event_manager_t* events;
  ivideo_mutex_t lock;
  int ready, play, attached, track, cached_track, scanned, audio, video, width, height;
  int subtitle_auto, subtitle_explicit;
  int volume, mute, adjust[IVIDEO_ADJUST_COUNT];
  int restore_spu, restore_spu_id, seekable, restarting;
  int64_t restore_spu_delay;
  libvlc_time_t length;
  double aspect;
  libvlc_state_t previous;
} IvideoVlc;

typedef struct
{
  IvideoSession* session;
  IvideoVlc* context;
  ivideo_mutex_t lock;
  unsigned char* pixels;
  unsigned char* shown;
  unsigned width, height, pitch;
} IvideoVlcPicture;

typedef struct
{
  char** argv;
  int argc, failed;
} IvideoVlcArgs;

static libvlc_instance_t* ivlc_instance = NULL;

static void iVideoVlcTrackSize(const libvlc_video_track_t* video, unsigned* width, unsigned* height)
{
  if (video->i_orientation >= libvlc_video_orient_left_top)
  {
    *width = video->i_height;
    *height = video->i_width;
  }
  else
  {
    *width = video->i_width;
    *height = video->i_height;
  }
}

static void iVideoVlcDisplaySize(const libvlc_video_track_t* video, int* width, int* height)
{
  unsigned long long display_width = video->i_width, display_height = video->i_height;
  if (video->i_sar_num && video->i_sar_den)
  {
    if (video->i_sar_num > video->i_sar_den)
      display_width = display_width * video->i_sar_num / video->i_sar_den;
    else
      display_height = display_height * video->i_sar_den / video->i_sar_num;
  }
  if (display_width > INT_MAX || display_height > INT_MAX)
    display_width = display_height = 0;
  if (video->i_orientation >= libvlc_video_orient_left_top)
  {
    *width = (int)display_height;
    *height = (int)display_width;
  }
  else
  {
    *width = (int)display_width;
    *height = (int)display_height;
  }
}

static int iVideoVlcLoad(void)
{
  return iupVlcOpen();
}

typedef struct
{
  libvlc_media_track_t** items;
  unsigned count;
  libvlc_media_tracklist_t* lists[3];
} IvideoVlcTracks;

static unsigned iVideoVlcTracksGet(libvlc_media_t* media, IvideoVlcTracks* tracks)
{
  const libvlc_track_type_t types[] = {libvlc_track_video, libvlc_track_audio, libvlc_track_text};
  size_t total = 0, i;
  int t;
  memset(tracks, 0, sizeof(IvideoVlcTracks));
  if (iupvlc_major == 3)
  {
    tracks->count = libvlc_media_tracks_get(media, &tracks->items);
    return tracks->count;
  }
  for (t = 0; t < 3; t++)
  {
    tracks->lists[t] = iupvlc4_media_get_tracklist(media, types[t]);
    if (tracks->lists[t])
      total += iupvlc4_media_tracklist_count(tracks->lists[t]);
  }
  if (total)
    tracks->items = (libvlc_media_track_t**)malloc(total * sizeof(libvlc_media_track_t*));
  if (!tracks->items)
    return 0;
  for (t = 0; t < 3; t++)
  {
    size_t count = tracks->lists[t] ? iupvlc4_media_tracklist_count(tracks->lists[t]) : 0;
    for (i = 0; i < count; i++)
      tracks->items[tracks->count++] = iupvlc4_media_tracklist_at(tracks->lists[t], i);
  }
  return tracks->count;
}

static void iVideoVlcTracksRelease(IvideoVlcTracks* tracks)
{
  int t;
  if (iupvlc_major == 3)
  {
    if (tracks->items)
      libvlc_media_tracks_release(tracks->items, tracks->count);
    return;
  }
  free(tracks->items);
  for (t = 0; t < 3; t++)
  {
    if (tracks->lists[t])
      iupvlc4_media_tracklist_delete(tracks->lists[t]);
  }
}

static libvlc_media_player_t* iVideoVlcPlayerNew(libvlc_instance_t* instance)
{
  return iupvlc_major == 3 ? libvlc_media_player_new(instance) : iupvlc4_media_player_new(instance, NULL, NULL);
}

static libvlc_state_t iVideoVlcState(libvlc_media_player_t* player)
{
  int state = (int)libvlc_media_player_get_state(player);
  if (iupvlc_major == 3)
    return (libvlc_state_t)state;
  switch (state)
  {
  case libvlc4_Opening:
    return libvlc_Opening;
  case libvlc4_Playing:
    return libvlc_Playing;
  case libvlc4_Paused:
    return libvlc_Paused;
  case libvlc4_Stopped:
  case libvlc4_Stopping:
    return libvlc_Ended;
  case libvlc4_Error:
    return libvlc_Error;
  default:
    return libvlc_NothingSpecial;
  }
}

static libvlc_time_t iVideoVlcGetTime(libvlc_media_player_t* player)
{
  libvlc_time_t time = libvlc_media_player_get_time(player);
  return iupvlc_major == 3 || time < 0 ? time : time / 1000;
}

static libvlc_time_t iVideoVlcGetLength(libvlc_media_player_t* player)
{
  libvlc_time_t length = libvlc_media_player_get_length(player);
  return iupvlc_major == 3 || length < 0 ? length : length / 1000;
}

static void iVideoVlcSetTime(libvlc_media_player_t* player, libvlc_time_t time)
{
  if (iupvlc_major == 3)
    libvlc_media_player_set_time(player, time);
  else
    iupvlc4_media_player_set_time(player, time * 1000, false);
}

static int iVideoVlcSeekable(libvlc_media_player_t* player)
{
  return iupvlc_major == 3 ? libvlc_media_player_is_seekable(player) : iupvlc4_media_player_is_seekable(player);
}

static void iVideoVlcStop(libvlc_media_player_t* player)
{
  if (iupvlc_major == 3)
    libvlc_media_player_stop(player);
  else
    iupvlc4_media_player_stop_async(player);
}

static int iVideoVlcGetSpu(libvlc_media_player_t* player)
{
  libvlc_media_track_t* track;
  int id;
  if (iupvlc_major == 3)
    return libvlc_video_get_spu(player);
  track = iupvlc4_media_player_get_selected_track(player, libvlc_track_text);
  if (!track)
    return -1;
  id = track->i_id;
  iupvlc4_media_track_release(track);
  return id;
}

static int iVideoVlcSetSpu(libvlc_media_player_t* player, int id)
{
  libvlc_media_tracklist_t* list;
  size_t i, count;
  int found = -1;
  if (iupvlc_major == 3)
    return libvlc_video_set_spu(player, id);
  if (id < 0)
  {
    iupvlc4_media_player_unselect_track_type(player, libvlc_track_text);
    return 0;
  }
  list = iupvlc4_media_player_get_tracklist(player, libvlc_track_text, false);
  if (!list)
    return -1;
  count = iupvlc4_media_tracklist_count(list);
  for (i = 0; i < count; i++)
  {
    libvlc_media_track_t* track = iupvlc4_media_tracklist_at(list, i);
    if (track->i_id == id)
    {
      iupvlc4_media_player_select_track(player, track);
      found = 0;
      break;
    }
  }
  iupvlc4_media_tracklist_delete(list);
  return found;
}

static void iVideoVlcError(const libvlc_event_t* event, void* opaque)
{
  (void)event;
  iupVideoError((IvideoSession*)opaque, "VLC could not play this source");
}

static unsigned iVideoVlcFormat(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines)
{
  IvideoVlc* context = (IvideoVlc*)*opaque;
  IvideoVlcTracks list;
  unsigned i, count = iVideoVlcTracksGet(context->media, &list);
  libvlc_media_track_t** tracks = list.items;
  IvideoVlcPicture* picture;
  size_t size;
  for (i = 0; i < count; i++)
  {
    unsigned track_width, track_height;
    if (tracks[i]->i_type != libvlc_track_video || !tracks[i]->video->i_width || !tracks[i]->video->i_height)
      continue;
    iVideoVlcTrackSize(tracks[i]->video, &track_width, &track_height);
    if (track_width <= *width && *width - track_width < 16 && track_height <= *height && *height - track_height < 16)
    {
      *width = track_width;
      *height = track_height;
      ivideoMutexLock(&context->lock);
      context->track = tracks[i]->i_id;
      ivideoMutexUnlock(&context->lock);
      break;
    }
  }
  iVideoVlcTracksRelease(&list);
  if (!*width || !*height || *width > (unsigned)(INT_MAX - 31) / 4 || *height > (unsigned)INT_MAX / 4 / *width)
    return 0;
  picture = (IvideoVlcPicture*)calloc(1, sizeof(IvideoVlcPicture));
  if (!picture)
    return 0;
  if (!ivideoMutexInit(&picture->lock))
  {
    free(picture);
    return 0;
  }
  picture->session = context->session;
  picture->context = context;
  picture->pitch = (*width * 4 + 31) & ~31u;
  size = (size_t)picture->pitch * *height;
  picture->pixels = (unsigned char*)ivideoAlignedAlloc(32, size);
  if (!picture->pixels)
  {
    ivideoMutexDestroy(&picture->lock);
    free(picture);
    iupVideoError(context->session, "Cannot allocate VLC video frame");
    return 0;
  }
  picture->width = *width;
  picture->height = *height;
  *opaque = picture;
  memcpy(chroma, iupvlc_major == 3 ? "RV32" : "BGRX", 4);
  pitches[0] = picture->pitch;
  lines[0] = *height;
  return 1;
}

static void iVideoVlcCleanup(void* opaque)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  ivideoAlignedFree(context->pixels);
  free(context->shown);
  ivideoMutexDestroy(&context->lock);
  free(context);
}

static void* iVideoVlcLock(void* opaque, void** planes)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  ivideoMutexLock(&context->lock);
  planes[0] = context->pixels;
  return context->pixels;
}

static void iVideoVlcUnlock(void* opaque, void* picture, void* const* planes)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  (void)picture;
  (void)planes;
  ivideoMutexUnlock(&context->lock);
}

static void iVideoVlcDisplay(void* opaque, void* picture)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  size_t size = (size_t)context->pitch * context->height;
  int play;
  ivideoMutexLock(&context->context->lock);
  play = context->context->play;
  ivideoMutexUnlock(&context->context->lock);
  ivideoMutexLock(&context->lock);
  if (play)
  {
    free(context->shown);
    context->shown = NULL;
  }
  else
  {
    if (context->shown && memcmp(context->shown, picture, size) == 0)
    {
      ivideoMutexUnlock(&context->lock);
      return;
    }
    if (!context->shown)
      context->shown = (unsigned char*)malloc(size);
    if (context->shown)
      memcpy(context->shown, picture, size);
  }
  iupVideoFrame(context->session, (const unsigned char*)picture, (int)context->width, (int)context->height, context->pitch);
  ivideoMutexUnlock(&context->lock);
}

static void iVideoVlcReleasePlayer(IvideoSession* session, IvideoVlc* context)
{
  if (!context->player)
    return;
  if (context->attached)
    libvlc_event_detach(context->events, libvlc_MediaPlayerEncounteredError, iVideoVlcError, session);
  context->attached = 0;
  iVideoVlcStop(context->player);
  libvlc_media_player_release(context->player);
  context->player = NULL;
}

static void iVideoVlcClose(IvideoSession* session)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  if (!context)
    return;
  iVideoVlcReleasePlayer(session, context);
  if (context->media)
    libvlc_media_release(context->media);
  if (context->instance)
    libvlc_release(context->instance);
  ivideoMutexDestroy(&context->lock);
  free(context);
  session->context = NULL;
}

static void iVideoVlcArg(IvideoVlcArgs* args, const char* option)
{
  char** argv = (char**)realloc(args->argv, (args->argc + 1) * sizeof(char*));
  char* arg;
  if (!argv)
  {
    args->failed = 1;
    return;
  }
  args->argv = argv;
  arg = (char*)malloc(strlen(option) + 3);
  if (!arg)
  {
    args->failed = 1;
    return;
  }
  strcpy(arg, "--");
  strcpy(arg + 2, option);
  argv[args->argc++] = arg;
}

static void iVideoVlcOption(char* option, void* user)
{
  iVideoVlcArg((IvideoVlcArgs*)user, option);
}

static libvlc_instance_t* iVideoVlcNewInstance(int config, const char* options)
{
  const char* required[] = {"quiet", "no-video-title-show", "no-osd", "no-sub-autodetect-file"};
  IvideoVlcArgs args = {NULL, 0, 0};
  libvlc_instance_t* instance = NULL;
  int i;
  if (config)
    iVideoVlcArg(&args, "no-ignore-config");
  iVideoVlcArg(&args, "swscale-mode=0");
  if (options)
    iupVideoSplitOptions(options, iVideoVlcOption, &args);
  for (i = 0; i < (int)(sizeof(required) / sizeof(required[0])); i++)
    iVideoVlcArg(&args, required[i]);
  if (!args.failed)
    instance = libvlc_new(args.argc, (const char* const*)args.argv);
  for (i = 0; i < args.argc; i++)
    free(args.argv[i]);
  free(args.argv);
  return instance;
}

static int iVideoVlcInstance(void)
{
  if (!ivlc_instance)
    ivlc_instance = iVideoVlcNewInstance(0, NULL);
  return ivlc_instance != NULL;
}

static libvlc_media_player_t* iVideoVlcNewPlayer(IvideoSession* session, IvideoVlc* context, const char* device)
{
  libvlc_media_player_t* player = iVideoVlcPlayerNew(context->instance);
  if (!player)
  {
    iupVideoError(session, "Cannot create VLC player");
    return NULL;
  }
  if (iupvlc_major == 3)
  {
    libvlc_event_manager_t* events = libvlc_media_player_event_manager(player);
    if (!events || libvlc_event_attach(events, libvlc_MediaPlayerEncounteredError, iVideoVlcError, session) != 0)
    {
      iupVideoError(session, "Cannot attach VLC playback events");
      libvlc_media_player_release(player);
      return NULL;
    }
  }
  if (device)
  {
    if (iupvlc_major == 3)
      libvlc_audio_output_device_set(player, NULL, device);
    else
      iupvlc4_audio_output_device_set(player, device);
  }
  libvlc_media_player_set_media(player, context->media);
  libvlc_video_set_callbacks(player, iVideoVlcLock, iVideoVlcUnlock, iVideoVlcDisplay, context);
  libvlc_video_set_format_callbacks(player, iVideoVlcFormat, iVideoVlcCleanup);
  return player;
}

static int iVideoVlcStart(IvideoSession* session, IvideoVlc* context, libvlc_media_player_t* player)
{
  context->player = player;
  context->restarting = iupvlc_major == 4;
  if (iupvlc_major == 3)
  {
    context->events = libvlc_media_player_event_manager(player);
    context->attached = 1;
  }
  if (libvlc_media_player_play(player) != 0)
  {
    iupVideoError(session, "VLC cannot start this source");
    return 0;
  }
  return 1;
}

static int iVideoVlcOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  IvideoVlc* context;
  libvlc_media_player_t* player;
  libvlc_instance_t* instance;
  const char* options = iupAttribGet(ih, "VLCOPTIONS");
  int config = iupAttribGetBoolean(ih, "VLCCONFIG");
  if (config || (options && options[0]))
  {
    instance = iVideoVlcNewInstance(config, options);
    if (!instance)
    {
      iupVideoError(session, options && options[0] ? "VLC rejected VLCOPTIONS" : "Cannot initialize VLC");
      return 0;
    }
  }
  else
  {
    if (!iVideoVlcInstance())
    {
      iupVideoError(session, "Cannot initialize VLC");
      return 0;
    }
    instance = ivlc_instance;
    libvlc_retain(instance);
  }
  context = (IvideoVlc*)calloc(1, sizeof(IvideoVlc));
  if (!context)
  {
    libvlc_release(instance);
    return 0;
  }
  if (!ivideoMutexInit(&context->lock))
  {
    libvlc_release(instance);
    free(context);
    return 0;
  }
  session->context = context;
  context->session = session;
  context->instance = instance;
  context->track = -1;
  context->cached_track = -1;
  context->subtitle_auto = -1;
  if (iupvlc_major == 3)
    context->media = url ? libvlc_media_new_location(instance, source) : libvlc_media_new_path(instance, source);
  else
    context->media = url ? iupvlc4_media_new_location(source) : iupvlc4_media_new_path(source);
  if (!context->media)
  {
    iupVideoError(session, "VLC cannot open this source");
    return 0;
  }
  if (session->audio && libvlc_media_slaves_add(context->media, libvlc_media_slave_type_audio, 4, session->audio) != 0)
  {
    iupVideoError(session, "VLC cannot open AUDIOURL");
    return 0;
  }
  libvlc_media_add_option(context->media, ":start-paused");
  player = iVideoVlcNewPlayer(session, context, session->audio_device);
  return player && iVideoVlcStart(session, context, player);
}

static void iVideoVlcPlay(IvideoSession* session, int play)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  ivideoMutexLock(&context->lock);
  context->play = play;
  ivideoMutexUnlock(&context->lock);
  libvlc_media_player_set_pause(context->player, !play);
}

static void iVideoVlcSeek(IvideoSession* session, double position)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  if (position > (double)LLONG_MAX / 1000)
    return;
  if (iVideoVlcState(context->player) == libvlc_Ended)
  {
    if (iupvlc_major == 3)
      libvlc_media_player_stop(context->player);
    libvlc_media_player_play(context->player);
    context->previous = libvlc_Opening;
    context->restarting = iupvlc_major == 4;
  }
  iVideoVlcSetTime(context->player, (libvlc_time_t)(position * 1000));
  if (iupvlc_major == 4 && iVideoVlcState(context->player) == libvlc_Paused)
    libvlc_media_player_next_frame(context->player);
}

static void iVideoVlcVolume(IvideoSession* session, int volume, int mute)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  context->volume = volume;
  context->mute = mute;
  libvlc_audio_set_volume(context->player, volume);
  libvlc_audio_set_mute(context->player, mute);
}

static void iVideoVlcScanTracks(IvideoVlc* context, int track)
{
  IvideoVlcTracks list;
  unsigned i, count = iVideoVlcTracksGet(context->media, &list);
  libvlc_media_track_t** tracks = list.items;
  int found = 0;
  context->audio = context->video = 0;
  context->width = context->height = 0;
  context->aspect = 0;
  for (i = 0; i < count; i++)
  {
    if (tracks[i]->i_type == libvlc_track_audio)
      context->audio = 1;
    else if (tracks[i]->i_type == libvlc_track_video)
    {
      context->video = 1;
      if (found || (track != -1 && tracks[i]->i_id != track))
        continue;
      found = 1;
      iVideoVlcDisplaySize(tracks[i]->video, &context->width, &context->height);
      context->aspect = context->height ? (double)context->width / context->height : 0;
    }
  }
  iVideoVlcTracksRelease(&list);
  context->cached_track = track;
  context->scanned = 1;
}

static void iVideoVlcPoll(IvideoSession* session)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  libvlc_state_t state = iVideoVlcState(context->player);
  IvideoInfo info;
  libvlc_time_t time, duration;
  int track;
  memset(&info, 0, sizeof(info));
  if (context->restarting)
  {
    if (state == libvlc_Ended)
      state = libvlc_Opening;
    else
      context->restarting = 0;
  }
  if (state == libvlc_Error)
  {
    iupVideoError(session, "VLC could not play this source");
    return;
  }
  if (state == libvlc_Playing || state == libvlc_Paused || state == libvlc_Ended)
    context->ready = 1;
  if (context->restore_spu && (state == libvlc_Playing || state == libvlc_Paused))
  {
    context->restore_spu = 0;
    libvlc_video_set_spu_delay(context->player, context->restore_spu_delay);
    iVideoVlcSetSpu(context->player, context->restore_spu_id);
  }
  if (context->ready && state == libvlc_Paused && context->play)
    libvlc_media_player_set_pause(context->player, 0);
  info.ready = context->ready;
  info.state = state == libvlc_Playing ? IVIDEO_PLAYING : (state == libvlc_Paused ? IVIDEO_PAUSED : IVIDEO_BUFFERING);
  info.ended = state == libvlc_Ended && context->previous != libvlc_Ended;
  info.seekable = iVideoVlcSeekable(context->player);
  time = iVideoVlcGetTime(context->player);
  duration = iVideoVlcGetLength(context->player);
  if (iupvlc_major == 4 && state == libvlc_Ended)
  {
    info.seekable = context->seekable;
    duration = context->length;
    time = duration;
  }
  else
  {
    context->seekable = info.seekable;
    if (duration > 0)
      context->length = duration;
  }
  info.position = time > 0 ? time / 1000.0 : 0;
  info.duration = duration > 0 ? duration / 1000.0 : 0;
  if (context->ready)
  {
    ivideoMutexLock(&context->lock);
    track = context->track;
    ivideoMutexUnlock(&context->lock);
    if (!context->scanned || track != context->cached_track)
      iVideoVlcScanTracks(context, track);
    info.audio = context->audio;
    info.video = context->video;
    info.width = context->width;
    info.height = context->height;
    info.aspect = context->aspect;
  }
  context->previous = state;
  iupVideoInfo(session, &info);
}

static int iVideoVlcSubtitleCount(IvideoSession* session)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  IvideoVlcTracks list;
  unsigned i, count = iVideoVlcTracksGet(context->media, &list);
  libvlc_media_track_t** tracks = list.items;
  int n = 0;
  for (i = 0; i < count; i++)
  {
    if (tracks[i]->i_type == libvlc_track_text)
      n++;
  }
  iVideoVlcTracksRelease(&list);
  return n;
}

static char* iVideoVlcSubtitleName(IvideoSession* session, int index, int language)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  IvideoVlcTracks list;
  unsigned i, count = iVideoVlcTracksGet(context->media, &list);
  libvlc_media_track_t** tracks = list.items;
  int n = 0;
  char* text = NULL;
  for (i = 0; i < count; i++)
  {
    if (tracks[i]->i_type != libvlc_track_text || ++n != index)
      continue;
    if (language)
      text = iupStrReturnStr(tracks[i]->psz_language);
    else if (tracks[i]->psz_description)
      text = iupStrReturnStr(tracks[i]->psz_description);
    else
      text = iupStrReturnStrf("Track %d", index);
    break;
  }
  iVideoVlcTracksRelease(&list);
  return text;
}

static int iVideoVlcSubtitleId(IvideoVlc* context, int index)
{
  IvideoVlcTracks list;
  unsigned i, count = iVideoVlcTracksGet(context->media, &list);
  libvlc_media_track_t** tracks = list.items;
  int n = 0, id = -1;
  for (i = 0; i < count; i++)
  {
    if (tracks[i]->i_type == libvlc_track_text && ++n == index)
    {
      id = tracks[i]->i_id;
      break;
    }
  }
  iVideoVlcTracksRelease(&list);
  return id;
}

static int iVideoVlcSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  int64_t offset = (int64_t)(delay * 1000000);
  int current = iVideoVlcGetSpu(context->player), id = -1, changed = 0;
  if (index > 0)
  {
    id = iVideoVlcSubtitleId(context, index);
    if (id < 0)
      return 0;
    context->subtitle_explicit = 1;
  }
  else if (index == 0)
    context->subtitle_explicit = 1;
  else
  {
    if (!context->subtitle_explicit)
      context->subtitle_auto = current;
    id = context->subtitle_auto;
  }
  if (libvlc_video_get_spu_delay(context->player) != offset)
  {
    libvlc_video_set_spu_delay(context->player, offset);
    changed = 1;
  }
  if (current != id)
  {
    if (iVideoVlcSetSpu(context->player, id) != 0)
      return 0;
    changed = 1;
  }
  if (changed && iVideoVlcState(context->player) == libvlc_Paused && iVideoVlcSeekable(context->player))
    iVideoVlcSetTime(context->player, iVideoVlcGetTime(context->player));
  return 1;
}

static int iVideoVlcAudioDevices(IvideoAudioDevice** devices)
{
  libvlc_media_player_t* player;
  libvlc_audio_output_device_t* list;
  libvlc_audio_output_device_t* device;
  int count = 0;
  if (!iVideoVlcLoad() || !iVideoVlcInstance())
    return 0;
  player = iVideoVlcPlayerNew(ivlc_instance);
  if (!player)
    return 0;
  list = libvlc_audio_output_device_enum(player);
  for (device = list; device; device = device->p_next)
  {
    if (device->psz_device && device->psz_device[0])
      count = iupVideoAddAudioDevice(devices, count, device->psz_device, device->psz_description);
  }
  if (list)
    libvlc_audio_output_device_list_release(list);
  libvlc_media_player_release(player);
  return count;
}

static void iVideoVlcApplyAdjust(IvideoVlc* context)
{
  const int* values = context->adjust;
  int enable = values[IVIDEO_BRIGHTNESS] || values[IVIDEO_CONTRAST] || values[IVIDEO_SATURATION] || values[IVIDEO_GAMMA];
  libvlc_video_set_adjust_float(context->player, libvlc_adjust_Brightness, 1.0f + values[IVIDEO_BRIGHTNESS] / 100.0f);
  libvlc_video_set_adjust_float(context->player, libvlc_adjust_Contrast, 1.0f + values[IVIDEO_CONTRAST] / 100.0f);
  libvlc_video_set_adjust_float(context->player, libvlc_adjust_Saturation, 1.0f + values[IVIDEO_SATURATION] / 100.0f);
  libvlc_video_set_adjust_float(context->player, libvlc_adjust_Gamma, (float)pow(2, values[IVIDEO_GAMMA] / 50.0));
  libvlc_video_set_adjust_int(context->player, libvlc_adjust_Enable, enable);
}

static void iVideoVlcAdjust(IvideoSession* session, const int* values)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  memcpy(context->adjust, values, sizeof(context->adjust));
  iVideoVlcApplyAdjust(context);
}

static void iVideoVlcAudioDevice(IvideoSession* session, const char* id)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  libvlc_time_t time = iVideoVlcGetTime(context->player);
  int spu = iVideoVlcGetSpu(context->player);
  int64_t delay = libvlc_video_get_spu_delay(context->player);
  libvlc_media_player_t* player = iVideoVlcNewPlayer(session, context, id);
  if (!player)
    return;
  iVideoVlcReleasePlayer(session, context);
  libvlc_audio_set_volume(player, context->volume);
  libvlc_audio_set_mute(player, context->mute);
  context->player = player;
  iVideoVlcApplyAdjust(context);
  if (!iVideoVlcStart(session, context, player))
    return;
  if (time > 0)
    iVideoVlcSetTime(context->player, time);
  context->previous = libvlc_Opening;
  if (context->ready)
  {
    context->restore_spu = 1;
    context->restore_spu_id = spu;
    context->restore_spu_delay = delay;
  }
}

const IvideoBackend iupVideoVlcBackend = {"VLC", iVideoVlcLoad, iVideoVlcOpen, iVideoVlcClose, iVideoVlcPlay, iVideoVlcSeek, iVideoVlcVolume, iVideoVlcPoll, NULL, iVideoVlcSubtitleCount, iVideoVlcSubtitleName, iVideoVlcSubtitleSelect, NULL, 1, iVideoVlcAudioDevices, iVideoVlcAudioDevice, iVideoVlcAdjust};
