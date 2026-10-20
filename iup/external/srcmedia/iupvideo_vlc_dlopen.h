/** \file
 * \brief libVLC 3 Dynamic Loading Support
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPVIDEO_VLC_DLOPEN_H
#define __IUPVIDEO_VLC_DLOPEN_H

#include <stdint.h>
#include <dlfcn.h>

typedef struct libvlc_instance_t libvlc_instance_t;
typedef struct libvlc_media_t libvlc_media_t;
typedef struct libvlc_media_player_t libvlc_media_player_t;
typedef struct libvlc_event_manager_t libvlc_event_manager_t;
typedef struct libvlc_event_t libvlc_event_t;
typedef int64_t libvlc_time_t;

typedef enum
{
  libvlc_NothingSpecial = 0,
  libvlc_Opening,
  libvlc_Buffering,
  libvlc_Playing,
  libvlc_Paused,
  libvlc_Stopped,
  libvlc_Ended,
  libvlc_Error
} libvlc_state_t;

typedef enum
{
  libvlc_track_unknown = -1,
  libvlc_track_audio = 0,
  libvlc_track_video = 1,
  libvlc_track_text = 2
} libvlc_track_type_t;

typedef enum
{
  libvlc_video_orient_top_left,
  libvlc_video_orient_top_right,
  libvlc_video_orient_bottom_left,
  libvlc_video_orient_bottom_right,
  libvlc_video_orient_left_top,
  libvlc_video_orient_left_bottom,
  libvlc_video_orient_right_top,
  libvlc_video_orient_right_bottom
} libvlc_video_orient_t;

enum
{
  libvlc_MediaPlayerEncounteredError = 0x10A
};

typedef struct
{
  float f_yaw;
  float f_pitch;
  float f_roll;
  float f_field_of_view;
} libvlc_video_viewpoint_t;

typedef struct
{
  unsigned i_height;
  unsigned i_width;
  unsigned i_sar_num;
  unsigned i_sar_den;
  unsigned i_frame_rate_num;
  unsigned i_frame_rate_den;
  libvlc_video_orient_t i_orientation;
  int i_projection;
  libvlc_video_viewpoint_t pose;
} libvlc_video_track_t;

typedef struct
{
  uint32_t i_codec;
  uint32_t i_original_fourcc;
  int i_id;
  libvlc_track_type_t i_type;
  int i_profile;
  int i_level;
  union
  {
    void* audio;
    libvlc_video_track_t* video;
    void* subtitle;
  };
  unsigned int i_bitrate;
  char* psz_language;
  char* psz_description;
} libvlc_media_track_t;

typedef void (*libvlc_callback_t)(const libvlc_event_t* event, void* data);
typedef void* (*libvlc_video_lock_cb)(void* opaque, void** planes);
typedef void (*libvlc_video_unlock_cb)(void* opaque, void* picture, void* const* planes);
typedef void (*libvlc_video_display_cb)(void* opaque, void* picture);
typedef unsigned (*libvlc_video_format_cb)(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines);
typedef void (*libvlc_video_cleanup_cb)(void* opaque);

static void* iupvlc_handle = NULL;

static const char* (*iupvlc_get_version)(void);
static libvlc_instance_t* (*iupvlc_new)(int, const char* const*);
static libvlc_media_t* (*iupvlc_media_new_path)(libvlc_instance_t*, const char*);
static libvlc_media_t* (*iupvlc_media_new_location)(libvlc_instance_t*, const char*);
static void (*iupvlc_media_add_option)(libvlc_media_t*, const char*);
static void (*iupvlc_media_release)(libvlc_media_t*);
static unsigned (*iupvlc_media_tracks_get)(libvlc_media_t*, libvlc_media_track_t***);
static void (*iupvlc_media_tracks_release)(libvlc_media_track_t**, unsigned);
static libvlc_media_player_t* (*iupvlc_media_player_new)(libvlc_instance_t*);
static void (*iupvlc_media_player_release)(libvlc_media_player_t*);
static void (*iupvlc_media_player_set_media)(libvlc_media_player_t*, libvlc_media_t*);
static int (*iupvlc_media_player_play)(libvlc_media_player_t*);
static void (*iupvlc_media_player_stop)(libvlc_media_player_t*);
static void (*iupvlc_media_player_set_pause)(libvlc_media_player_t*, int);
static libvlc_state_t (*iupvlc_media_player_get_state)(libvlc_media_player_t*);
static libvlc_time_t (*iupvlc_media_player_get_time)(libvlc_media_player_t*);
static void (*iupvlc_media_player_set_time)(libvlc_media_player_t*, libvlc_time_t);
static libvlc_time_t (*iupvlc_media_player_get_length)(libvlc_media_player_t*);
static int (*iupvlc_media_player_is_seekable)(libvlc_media_player_t*);
static libvlc_event_manager_t* (*iupvlc_media_player_event_manager)(libvlc_media_player_t*);
static int (*iupvlc_audio_set_volume)(libvlc_media_player_t*, int);
static void (*iupvlc_audio_set_mute)(libvlc_media_player_t*, int);
static void (*iupvlc_video_set_callbacks)(libvlc_media_player_t*, libvlc_video_lock_cb, libvlc_video_unlock_cb, libvlc_video_display_cb, void*);
static void (*iupvlc_video_set_format_callbacks)(libvlc_media_player_t*, libvlc_video_format_cb, libvlc_video_cleanup_cb);
static int (*iupvlc_event_attach)(libvlc_event_manager_t*, int, libvlc_callback_t, void*);
static void (*iupvlc_event_detach)(libvlc_event_manager_t*, int, libvlc_callback_t, void*);
static int (*iupvlc_video_set_spu)(libvlc_media_player_t*, int);
static int (*iupvlc_video_get_spu)(libvlc_media_player_t*);
static int (*iupvlc_video_set_spu_delay)(libvlc_media_player_t*, int64_t);
static int64_t (*iupvlc_video_get_spu_delay)(libvlc_media_player_t*);

#define libvlc_get_version iupvlc_get_version
#define libvlc_new iupvlc_new
#define libvlc_media_new_path iupvlc_media_new_path
#define libvlc_media_new_location iupvlc_media_new_location
#define libvlc_media_add_option iupvlc_media_add_option
#define libvlc_media_release iupvlc_media_release
#define libvlc_media_tracks_get iupvlc_media_tracks_get
#define libvlc_media_tracks_release iupvlc_media_tracks_release
#define libvlc_media_player_new iupvlc_media_player_new
#define libvlc_media_player_release iupvlc_media_player_release
#define libvlc_media_player_set_media iupvlc_media_player_set_media
#define libvlc_media_player_play iupvlc_media_player_play
#define libvlc_media_player_stop iupvlc_media_player_stop
#define libvlc_media_player_set_pause iupvlc_media_player_set_pause
#define libvlc_media_player_get_state iupvlc_media_player_get_state
#define libvlc_media_player_get_time iupvlc_media_player_get_time
#define libvlc_media_player_set_time iupvlc_media_player_set_time
#define libvlc_media_player_get_length iupvlc_media_player_get_length
#define libvlc_media_player_is_seekable iupvlc_media_player_is_seekable
#define libvlc_media_player_event_manager iupvlc_media_player_event_manager
#define libvlc_audio_set_volume iupvlc_audio_set_volume
#define libvlc_audio_set_mute iupvlc_audio_set_mute
#define libvlc_video_set_callbacks iupvlc_video_set_callbacks
#define libvlc_video_set_format_callbacks iupvlc_video_set_format_callbacks
#define libvlc_event_attach iupvlc_event_attach
#define libvlc_event_detach iupvlc_event_detach
#define libvlc_video_set_spu iupvlc_video_set_spu
#define libvlc_video_get_spu iupvlc_video_get_spu
#define libvlc_video_set_spu_delay iupvlc_video_set_spu_delay
#define libvlc_video_get_spu_delay iupvlc_video_get_spu_delay

static void* iupVlcSymbol(const char* name, int* missing)
{
  void* symbol = dlsym(iupvlc_handle, name);
  if (!symbol)
    *missing = 1;
  return symbol;
}

static int iupVlcOpen(void)
{
  int missing = 0;

  if (iupvlc_handle)
    return 1;

  iupvlc_handle = dlopen("libvlc.so.5", RTLD_NOW | RTLD_LOCAL);
  if (!iupvlc_handle)
    return 0;

  *(void**)&iupvlc_get_version = iupVlcSymbol("libvlc_get_version", &missing);
  *(void**)&iupvlc_new = iupVlcSymbol("libvlc_new", &missing);
  *(void**)&iupvlc_media_new_path = iupVlcSymbol("libvlc_media_new_path", &missing);
  *(void**)&iupvlc_media_new_location = iupVlcSymbol("libvlc_media_new_location", &missing);
  *(void**)&iupvlc_media_add_option = iupVlcSymbol("libvlc_media_add_option", &missing);
  *(void**)&iupvlc_media_release = iupVlcSymbol("libvlc_media_release", &missing);
  *(void**)&iupvlc_media_tracks_get = iupVlcSymbol("libvlc_media_tracks_get", &missing);
  *(void**)&iupvlc_media_tracks_release = iupVlcSymbol("libvlc_media_tracks_release", &missing);
  *(void**)&iupvlc_media_player_new = iupVlcSymbol("libvlc_media_player_new", &missing);
  *(void**)&iupvlc_media_player_release = iupVlcSymbol("libvlc_media_player_release", &missing);
  *(void**)&iupvlc_media_player_set_media = iupVlcSymbol("libvlc_media_player_set_media", &missing);
  *(void**)&iupvlc_media_player_play = iupVlcSymbol("libvlc_media_player_play", &missing);
  *(void**)&iupvlc_media_player_stop = iupVlcSymbol("libvlc_media_player_stop", &missing);
  *(void**)&iupvlc_media_player_set_pause = iupVlcSymbol("libvlc_media_player_set_pause", &missing);
  *(void**)&iupvlc_media_player_get_state = iupVlcSymbol("libvlc_media_player_get_state", &missing);
  *(void**)&iupvlc_media_player_get_time = iupVlcSymbol("libvlc_media_player_get_time", &missing);
  *(void**)&iupvlc_media_player_set_time = iupVlcSymbol("libvlc_media_player_set_time", &missing);
  *(void**)&iupvlc_media_player_get_length = iupVlcSymbol("libvlc_media_player_get_length", &missing);
  *(void**)&iupvlc_media_player_is_seekable = iupVlcSymbol("libvlc_media_player_is_seekable", &missing);
  *(void**)&iupvlc_media_player_event_manager = iupVlcSymbol("libvlc_media_player_event_manager", &missing);
  *(void**)&iupvlc_audio_set_volume = iupVlcSymbol("libvlc_audio_set_volume", &missing);
  *(void**)&iupvlc_audio_set_mute = iupVlcSymbol("libvlc_audio_set_mute", &missing);
  *(void**)&iupvlc_video_set_callbacks = iupVlcSymbol("libvlc_video_set_callbacks", &missing);
  *(void**)&iupvlc_video_set_format_callbacks = iupVlcSymbol("libvlc_video_set_format_callbacks", &missing);
  *(void**)&iupvlc_event_attach = iupVlcSymbol("libvlc_event_attach", &missing);
  *(void**)&iupvlc_event_detach = iupVlcSymbol("libvlc_event_detach", &missing);
  *(void**)&iupvlc_video_set_spu = iupVlcSymbol("libvlc_video_set_spu", &missing);
  *(void**)&iupvlc_video_get_spu = iupVlcSymbol("libvlc_video_get_spu", &missing);
  *(void**)&iupvlc_video_set_spu_delay = iupVlcSymbol("libvlc_video_set_spu_delay", &missing);
  *(void**)&iupvlc_video_get_spu_delay = iupVlcSymbol("libvlc_video_get_spu_delay", &missing);

  if (missing || libvlc_get_version()[0] != '3')
  {
    dlclose(iupvlc_handle);
    iupvlc_handle = NULL;
    return 0;
  }

  return 1;
}

#endif
