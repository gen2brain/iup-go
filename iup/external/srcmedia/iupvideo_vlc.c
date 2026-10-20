/** \file
 * \brief Video control, libVLC engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "iup_video.h"
#include "iupvideo_vlc_dlopen.h"

typedef struct
{
  IvideoSession* session;
  libvlc_media_t* media;
  libvlc_media_player_t* player;
  libvlc_event_manager_t* events;
  pthread_mutex_t lock;
  int ready, play, attached, track, cached_track, scanned, audio, video, width, height;
  double aspect;
  libvlc_state_t previous;
} IvideoVlc;

typedef struct
{
  IvideoSession* session;
  pthread_mutex_t lock;
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
      pthread_mutex_lock(&context->lock);
      context->track = tracks[i]->i_id;
      pthread_mutex_unlock(&context->lock);
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
  if (pthread_mutex_init(&picture->lock, NULL) != 0)
  {
    free(picture);
    return 0;
  }
  picture->session = context->session;
  picture->pitch = (*width * 4 + 31) & ~31u;
  size = (size_t)picture->pitch * *height;
  if (posix_memalign((void**)&picture->pixels, 32, size) != 0)
  {
    pthread_mutex_destroy(&picture->lock);
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
  free(context->pixels);
  pthread_mutex_destroy(&context->lock);
  free(context);
}

static void* iVideoVlcLock(void* opaque, void** planes)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  pthread_mutex_lock(&context->lock);
  planes[0] = context->pixels;
  return context->pixels;
}

static void iVideoVlcUnlock(void* opaque, void* picture, void* const* planes)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  (void)picture;
  (void)planes;
  pthread_mutex_unlock(&context->lock);
}

static void iVideoVlcDisplay(void* opaque, void* picture)
{
  IvideoVlcPicture* context = (IvideoVlcPicture*)opaque;
  pthread_mutex_lock(&context->lock);
  iupVideoFrame(context->session, (const unsigned char*)picture, (int)context->width, (int)context->height, context->pitch, 4);
  pthread_mutex_unlock(&context->lock);
}

static void iVideoVlcClose(IvideoSession* session)
{
  IvideoVlc* context = (IvideoVlc*)session->context;
  if (!context)
    return;
  if (context->player)
  {
    if (context->attached)
      libvlc_event_detach(context->events, libvlc_MediaPlayerEncounteredError, iVideoVlcError, session);
    libvlc_media_player_stop(context->player);
    libvlc_media_player_release(context->player);
  }
  if (context->media)
    libvlc_media_release(context->media);
  pthread_mutex_destroy(&context->lock);
  free(context);
  session->context = NULL;
}

static int iVideoVlcOpen(IvideoSession* session, Ihandle* ih, const char* source, int url)
{
  const char* options[] = {"--quiet", "--no-video-title-show", "--no-osd", "--no-spu"};
  IvideoVlc* context;
  (void)ih;
  if (!ivlc_instance)
    ivlc_instance = libvlc_new(4, options);
  if (!ivlc_instance)
  {
    iupVideoError(session, "Cannot initialize VLC");
    return 0;
  }
  context = (IvideoVlc*)calloc(1, sizeof(IvideoVlc));
  if (!context)
    return 0;
  if (pthread_mutex_init(&context->lock, NULL) != 0)
  {
    free(context);
    return 0;
  }
  session->context = context;
  context->session = session;
  context->track = -1;
  context->cached_track = -1;
  context->media = url ? libvlc_media_new_location(ivlc_instance, source) : libvlc_media_new_path(ivlc_instance, source);
  if (!context->media)
  {
    iupVideoError(session, "VLC cannot open this source");
    return 0;
  }
  context->player = libvlc_media_player_new(ivlc_instance);
  if (!context->player)
  {
    iupVideoError(session, "Cannot create VLC player");
    return 0;
  }
  context->events = libvlc_media_player_event_manager(context->player);
  if (!context->events || libvlc_event_attach(context->events, libvlc_MediaPlayerEncounteredError, iVideoVlcError, session) != 0)
  {
    iupVideoError(session, "Cannot attach VLC playback events");
    return 0;
  }
  context->attached = 1;
  libvlc_media_add_option(context->media, ":start-paused");
  libvlc_media_player_set_media(context->player, context->media);
  libvlc_video_set_callbacks(context->player, iVideoVlcLock, iVideoVlcUnlock, iVideoVlcDisplay, context);
  libvlc_video_set_format_callbacks(context->player, iVideoVlcFormat, iVideoVlcCleanup);
  if (libvlc_media_player_play(context->player) != 0)
  {
    iupVideoError(session, "VLC cannot start this source");
    return 0;
  }
  return 1;
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
    pthread_mutex_lock(&context->lock);
    track = context->track;
    pthread_mutex_unlock(&context->lock);
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

const IvideoBackend iupVideoVlcBackend = {"VLC", iVideoVlcLoad, iVideoVlcOpen, iVideoVlcClose, iVideoVlcPlay, iVideoVlcSeek, iVideoVlcVolume, iVideoVlcPoll, NULL};
