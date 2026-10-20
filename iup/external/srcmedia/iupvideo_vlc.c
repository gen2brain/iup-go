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
#include "iupvideo_vlc_dlopen.h"

typedef struct
{
  IvideoSession* session;
  libvlc_media_t* media;
  libvlc_media_player_t* player;
  libvlc_event_manager_t* events;
  ivideo_mutex_t lock;
  int ready, play, attached, track, cached_track, scanned, audio, video, width, height;
  int subtitle_auto, subtitle_explicit;
  int volume, mute, adjust[IVIDEO_ADJUST_COUNT];
  int restore_spu, restore_spu_id;
  int64_t restore_spu_delay;
  double aspect;
  libvlc_state_t previous;
} IvideoVlc;

typedef struct
{
  IvideoSession* session;
  ivideo_mutex_t lock;
  unsigned char* pixels;
  unsigned width, height, pitch;
} IvideoVlcPicture;

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

static void iVideoVlcError(const libvlc_event_t* event, void* opaque)
{
  (void)event;
  iupVideoError((IvideoSession*)opaque, "VLC could not play this source");
}

static unsigned iVideoVlcFormat(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines)
{
  IvideoVlc* context = (IvideoVlc*)*opaque;
  libvlc_media_track_t** tracks = NULL;
  unsigned i, count = libvlc_media_tracks_get(context->media, &tracks);
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
  if (tracks)
    libvlc_media_tracks_release(tracks, count);
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
  memcpy(chroma, "RGBA", 4);
  pitches[0] = picture->pitch;
  lines[0] = *height;
  return 1;
}

static void iVideoVlcCleanup(void* opaque)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  ivideoAlignedFree(context->pixels);
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
  ivideoMutexLock(&context->lock);
  iupVideoFrame(context->session, (const unsigned char*)picture, (int)context->width, (int)context->height, context->pitch, 4);
  ivideoMutexUnlock(&context->lock);
}

static void iVideoVlcReleasePlayer(IvideoSession* session, IvideoVlc* context)
{
  if (!context->player)
    return;
  if (context->attached)
    libvlc_event_detach(context->events, libvlc_MediaPlayerEncounteredError, iVideoVlcError, session);
  context->attached = 0;
  libvlc_media_player_stop(context->player);
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
  ivideoMutexDestroy(&context->lock);
  free(context);
  session->context = NULL;
}

static int iVideoVlcInstance(void)
{
  const char* options[] = {"--quiet", "--no-video-title-show", "--no-osd", "--no-sub-autodetect-file"};
  if (!ivlc_instance)
    ivlc_instance = libvlc_new((int)(sizeof(options) / sizeof(options[0])), options);
  return ivlc_instance != NULL;
}

static libvlc_media_player_t* iVideoVlcNewPlayer(IvideoSession* session, IvideoVlc* context, const char* device)
{
  libvlc_media_player_t* player = libvlc_media_player_new(ivlc_instance);
  libvlc_event_manager_t* events;
  if (!player)
  {
    iupVideoError(session, "Cannot create VLC player");
    return NULL;
  }
  events = libvlc_media_player_event_manager(player);
  if (!events || libvlc_event_attach(events, libvlc_MediaPlayerEncounteredError, iVideoVlcError, session) != 0)
  {
    iupVideoError(session, "Cannot attach VLC playback events");
    libvlc_media_player_release(player);
    return NULL;
  }
  if (device)
    libvlc_audio_output_device_set(player, NULL, device);
  libvlc_media_player_set_media(player, context->media);
  libvlc_video_set_callbacks(player, iVideoVlcLock, iVideoVlcUnlock, iVideoVlcDisplay, context);
  libvlc_video_set_format_callbacks(player, iVideoVlcFormat, iVideoVlcCleanup);
  return player;
}

static int iVideoVlcStart(IvideoSession* session, IvideoVlc* context, libvlc_media_player_t* player)
{
  context->player = player;
  context->events = libvlc_media_player_event_manager(player);
  context->attached = 1;
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
  (void)ih;
  if (!iVideoVlcInstance())
  {
    iupVideoError(session, "Cannot initialize VLC");
    return 0;
  }
  context = (IvideoVlc*)calloc(1, sizeof(IvideoVlc));
  if (!context)
    return 0;
  if (!ivideoMutexInit(&context->lock))
  {
    free(context);
    return 0;
  }
  session->context = context;
  context->session = session;
  context->track = -1;
  context->cached_track = -1;
  context->subtitle_auto = -1;
  context->media = url ? libvlc_media_new_location(ivlc_instance, source) : libvlc_media_new_path(ivlc_instance, source);
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
  context->play = play;
  libvlc_media_player_set_pause(context->player, !play);
}

static void iVideoVlcSeek(IvideoSession* session, double position)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  if (position > (double)LLONG_MAX / 1000)
    return;
  if (libvlc_media_player_get_state(context->player) == libvlc_Ended)
  {
    libvlc_media_player_stop(context->player);
    libvlc_media_player_play(context->player);
    context->previous = libvlc_Opening;
  }
  libvlc_media_player_set_time(context->player, (libvlc_time_t)(position * 1000));
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
  libvlc_media_track_t** tracks = NULL;
  unsigned i, count = libvlc_media_tracks_get(context->media, &tracks);
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
  if (tracks)
    libvlc_media_tracks_release(tracks, count);
  context->cached_track = track;
  context->scanned = 1;
}

static void iVideoVlcPoll(IvideoSession* session)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  libvlc_state_t state = libvlc_media_player_get_state(context->player);
  IvideoInfo info;
  libvlc_time_t time, duration;
  int track;
  memset(&info, 0, sizeof(info));
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
    libvlc_video_set_spu(context->player, context->restore_spu_id);
  }
  if (context->ready && state == libvlc_Paused && context->play)
    libvlc_media_player_set_pause(context->player, 0);
  info.ready = context->ready;
  info.state = state == libvlc_Playing ? IVIDEO_PLAYING : (state == libvlc_Paused ? IVIDEO_PAUSED : IVIDEO_BUFFERING);
  info.ended = state == libvlc_Ended && context->previous != libvlc_Ended;
  info.seekable = libvlc_media_player_is_seekable(context->player);
  time = libvlc_media_player_get_time(context->player);
  duration = libvlc_media_player_get_length(context->player);
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
  libvlc_media_track_t** tracks = NULL;
  unsigned i, count = libvlc_media_tracks_get(context->media, &tracks);
  int n = 0;
  for (i = 0; i < count; i++)
  {
    if (tracks[i]->i_type == libvlc_track_text)
      n++;
  }
  if (tracks)
    libvlc_media_tracks_release(tracks, count);
  return n;
}

static char* iVideoVlcSubtitleName(IvideoSession* session, int index, int language)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  libvlc_media_track_t** tracks = NULL;
  unsigned i, count = libvlc_media_tracks_get(context->media, &tracks);
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
  if (tracks)
    libvlc_media_tracks_release(tracks, count);
  return text;
}

static int iVideoVlcSubtitleId(IvideoVlc* context, int index)
{
  libvlc_media_track_t** tracks = NULL;
  unsigned i, count = libvlc_media_tracks_get(context->media, &tracks);
  int n = 0, id = -1;
  for (i = 0; i < count; i++)
  {
    if (tracks[i]->i_type == libvlc_track_text && ++n == index)
    {
      id = tracks[i]->i_id;
      break;
    }
  }
  if (tracks)
    libvlc_media_tracks_release(tracks, count);
  return id;
}

static int iVideoVlcSubtitleSelect(IvideoSession* session, int index, double delay)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  int64_t offset = (int64_t)(delay * 1000000);
  int current = libvlc_video_get_spu(context->player), id = -1, changed = 0;
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
    if (libvlc_video_set_spu(context->player, id) != 0)
      return 0;
    changed = 1;
  }
  if (changed && libvlc_media_player_get_state(context->player) == libvlc_Paused && libvlc_media_player_is_seekable(context->player))
    libvlc_media_player_set_time(context->player, libvlc_media_player_get_time(context->player));
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
  player = libvlc_media_player_new(ivlc_instance);
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
  libvlc_time_t time = libvlc_media_player_get_time(context->player);
  int spu = libvlc_video_get_spu(context->player);
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
    libvlc_media_player_set_time(context->player, time);
  context->previous = libvlc_Opening;
  if (context->ready)
  {
    context->restore_spu = 1;
    context->restore_spu_id = spu;
    context->restore_spu_delay = delay;
  }
}

const IvideoBackend iupVideoVlcBackend = {"VLC", iVideoVlcLoad, iVideoVlcOpen, iVideoVlcClose, iVideoVlcPlay, iVideoVlcSeek, iVideoVlcVolume, iVideoVlcPoll, NULL, iVideoVlcSubtitleCount, iVideoVlcSubtitleName, iVideoVlcSubtitleSelect, NULL, 1, iVideoVlcAudioDevices, iVideoVlcAudioDevice, iVideoVlcAdjust};
