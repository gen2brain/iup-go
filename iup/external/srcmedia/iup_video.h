/** \file
 * \brief Video control
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_VIDEO_H
#define __IUP_VIDEO_H

#include <stddef.h>
#include <stdio.h>
#include "iup.h"
#include "iup_class.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { IVIDEO_EMPTY, IVIDEO_LOADING, IVIDEO_STOPPED, IVIDEO_PLAYING, IVIDEO_PAUSED, IVIDEO_BUFFERING, IVIDEO_ENDED, IVIDEO_ERROR };

typedef struct _IvideoSession IvideoSession;

typedef struct _IvideoInfo
{
  int state, ready, ended, seekable, video, audio;
  int width, height;
  double position, duration, aspect;
  char error[512];
} IvideoInfo;

typedef struct _IvideoAudioDevice
{
  char* id;
  char* name;
} IvideoAudioDevice;

enum { IVIDEO_BRIGHTNESS, IVIDEO_CONTRAST, IVIDEO_SATURATION, IVIDEO_GAMMA, IVIDEO_ADJUST_COUNT };

typedef struct _IvideoBackend
{
  const char* name;
  int (*available)(void);
  int (*open)(IvideoSession* session, Ihandle* ih, const char* source, int url);
  void (*close)(IvideoSession* session);
  void (*play)(IvideoSession* session, int play);
  void (*seek)(IvideoSession* session, double position);
  void (*volume)(IvideoSession* session, int volume, int mute);
  void (*poll)(IvideoSession* session);
  void (*layout)(IvideoSession* session, Ihandle* ih);
  int (*subtitle_count)(IvideoSession* session);
  char* (*subtitle_name)(IvideoSession* session, int index, int language);
  int (*subtitle_select)(IvideoSession* session, int index, double delay);
  int (*subtitle_text)(IvideoSession* session, const char* text, int size, const char* color);
  int audio;
  int (*audio_devices)(IvideoAudioDevice** devices);
  void (*audio_device)(IvideoSession* session, const char* id);
  void (*adjust)(IvideoSession* session, const int* values);
} IvideoBackend;

struct _IvideoSession
{
  const IvideoBackend* backend;
  void* context;
  const char* audio;
  const char* audio_device;
  void* mutex;
  int closing;
  IvideoInfo info;
  unsigned char* pending;
  int pending_width, pending_height, pending_ready;
  char* subtitle_cue;
  char subtitle_error[512];
};

Iclass* iupVideoNewClass(void);
const IvideoBackend* iupdrvVideoBackend(int index);
void iupVideoInfo(IvideoSession* session, const IvideoInfo* info);
void iupVideoError(IvideoSession* session, const char* message);
void iupVideoFrame(IvideoSession* session, const unsigned char* pixels, int width, int height, size_t stride, int channels);
void iupVideoSubtitleCue(IvideoSession* session, const char* text);
void iupVideoSubtitleError(IvideoSession* session, const char* message);
int iupVideoAddAudioDevice(IvideoAudioDevice** devices, int count, const char* id, const char* name);
void iupVideoFreeAudioDevices(IvideoAudioDevice* devices, int count);
char* iupVideoReadSubtitleStream(FILE* file);
char* iupdrvVideoReadSubtitle(const char* filename);
void iupdrvVideoInhibitScreenSaver(int inhibit);

#ifdef __cplusplus
}
#endif
#endif
