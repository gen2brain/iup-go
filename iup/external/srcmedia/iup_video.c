/** \file
 * \brief Video player
 *
 * See Copyright Notice in "iup.h"
 */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupdraw.h"
#include "iupmedia.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"
#include "iup_thread.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_video.h"

typedef struct
{
  Ihandle* ih;
  unsigned references;
  unsigned long generation;
} IvideoLifetime;

typedef int (*IvideoStringCallback)(Ihandle*, const char*);

struct _IcontrolData
{
  iupCanvas canvas;
  IvideoSession* session;
  IvideoLifetime* lifetime;
  Ihandle* timer;
  Ihandle* image;
  unsigned char* frame;
  int frame_width, frame_height;
  char image_name[48];
  char backend[16];
  char* source;
  int url, intent, state, ready, ended, reported_error, reported_state;
  int pending_seek;
  double seek_position;
  IvideoInfo info;
};

static const char* iVideoStates[] = {"EMPTY", "LOADING", "STOPPED", "PLAYING", "PAUSED", "BUFFERING", "ENDED", "ERROR"};

void iupVideoInfo(IvideoSession* session, const IvideoInfo* info)
{
  iupdrvMutexLock(session->mutex);
  if (!session->closing && !session->info.error[0])
  {
    int ended = session->info.ended;
    session->info = *info;
    session->info.ended |= ended;
  }
  iupdrvMutexUnlock(session->mutex);
}

void iupVideoError(IvideoSession* session, const char* message)
{
  iupdrvMutexLock(session->mutex);
  if (!session->closing)
  {
    session->info.state = IVIDEO_ERROR;
    iupStrCopyN(session->info.error, sizeof(session->info.error), message ? message : "Video playback failed");
  }
  iupdrvMutexUnlock(session->mutex);
}

void iupVideoFrame(IvideoSession* session, const unsigned char* pixels, int width, int height, size_t stride, int channels)
{
  size_t size;
  int x, y;
  if (!pixels || width <= 0 || height <= 0 || (channels != 3 && channels != 4) ||
      width > INT_MAX / channels || height > INT_MAX / 3 / width || stride < (size_t)width * channels)
    return;
  size = (size_t)width * height * 3;
  iupdrvMutexLock(session->mutex);
  if (session->closing)
  {
    iupdrvMutexUnlock(session->mutex);
    return;
  }
  if (session->pending_width != width || session->pending_height != height)
  {
    unsigned char* buffer = (unsigned char*)realloc(session->pending, size);
    if (!buffer)
    {
      session->info.state = IVIDEO_ERROR;
      iupStrCopyN(session->info.error, sizeof(session->info.error), "Cannot allocate video frame");
      iupdrvMutexUnlock(session->mutex);
      return;
    }
    session->pending = buffer;
    session->pending_width = width;
    session->pending_height = height;
  }
  for (y = 0; y < height; y++)
  {
    const unsigned char* row = pixels + (size_t)y * stride;
    unsigned char* output = session->pending + (size_t)y * width * 3;
    if (channels == 3)
      memcpy(output, row, (size_t)width * 3);
    else
    {
      for (x = 0; x < width; x++)
      {
        output[x * 3] = row[x * 4];
        output[x * 3 + 1] = row[x * 4 + 1];
        output[x * 3 + 2] = row[x * 4 + 2];
      }
    }
  }
  session->pending_ready = 1;
  iupdrvMutexUnlock(session->mutex);
}

static void iVideoReleaseImage(Ihandle* ih)
{
  if (ih->data->image)
  {
    IupSetHandle(ih->data->image_name, NULL);
    IupDestroy(ih->data->image);
    ih->data->image = NULL;
  }
  free(ih->data->frame);
  ih->data->frame = NULL;
  ih->data->frame_width = ih->data->frame_height = 0;
}

static void iVideoClose(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  if (ih->data->lifetime)
    ih->data->lifetime->generation++;
  ih->data->session = NULL;
  if (session)
  {
    iupdrvMutexLock(session->mutex);
    session->closing = 1;
    iupdrvMutexUnlock(session->mutex);
    session->backend->close(session);
    free(session->pending);
    iupdrvMutexDestroy(session->mutex);
    free(session);
  }
  ih->data->ready = ih->data->ended = ih->data->reported_error = 0;
  memset(&ih->data->info, 0, sizeof(ih->data->info));
  iVideoReleaseImage(ih);
}

static void iVideoVolume(Ihandle* ih, int volume, int mute)
{
  IvideoSession* session = ih->data->session;
  if (session)
    session->backend->volume(session, volume, mute);
}

static void iVideoSettings(Ihandle* ih)
{
  iVideoVolume(ih, iupAttribGetInt(ih, "VOLUME"), iupAttribGetBoolean(ih, "MUTE"));
}

static void iVideoStart(Ihandle* ih)
{
  const IvideoBackend* backend;
  char error[512] = "";
  int index;
  for (index = 0; (backend = iupdrvVideoBackend(index)) != NULL; index++)
  {
    IvideoSession* session;
    if (!iupStrEqualNoCase(ih->data->backend, "AUTO") && !iupStrEqualNoCase(ih->data->backend, backend->name))
      continue;
    if (!backend->available())
      continue;
    session = (IvideoSession*)calloc(1, sizeof(IvideoSession));
    if (!session)
      break;
    session->mutex = iupdrvMutexCreate();
    session->backend = backend;
    session->info.state = IVIDEO_LOADING;
    ih->data->session = session;
    if (backend->open(session, ih, ih->data->source, ih->data->url))
    {
      iVideoSettings(ih);
      return;
    }
    iupdrvMutexLock(session->mutex);
    iupStrCopyN(error, sizeof(error), session->info.error);
    iupdrvMutexUnlock(session->mutex);
    iVideoClose(ih);
  }
  ih->data->state = IVIDEO_ERROR;
  iupStrCopyN(ih->data->info.error, sizeof(ih->data->info.error), error[0] ? error : "No usable video backend");
}

static void iVideoOpen(Ihandle* ih)
{
  iVideoClose(ih);
  if (!ih->data->source)
    ih->data->state = IVIDEO_EMPTY;
  else
    ih->data->state = ih->handle ? IVIDEO_LOADING : IVIDEO_STOPPED;
  if (!ih->handle)
    return;
  if (ih->data->source)
    iVideoStart(ih);
  IupSetAttribute(ih->data->timer, "RUN", "YES");
}

static int iVideoCallback(Ihandle* ih, IvideoLifetime* lifetime, const char* name)
{
  unsigned long generation = lifetime->generation;
  Icallback cb = IupGetCallback(ih, name);
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
  return lifetime->ih != NULL && lifetime->generation == generation;
}

static void iVideoTakeFrame(Ihandle* ih, IvideoSession* session)
{
  unsigned char* old;
  int width, height, changed = 0;
  iupdrvMutexLock(session->mutex);
  if (session->pending_ready)
  {
    old = ih->data->frame;
    width = ih->data->frame_width;
    height = ih->data->frame_height;
    ih->data->frame = session->pending;
    ih->data->frame_width = session->pending_width;
    ih->data->frame_height = session->pending_height;
    session->pending = old;
    session->pending_width = width;
    session->pending_height = height;
    session->pending_ready = 0;
    changed = 1;
  }
  iupdrvMutexUnlock(session->mutex);
  if (!changed)
    return;
  width = ih->data->frame_width;
  height = ih->data->frame_height;
  if (ih->data->image && (IupGetInt(ih->data->image, "WIDTH") != width || IupGetInt(ih->data->image, "HEIGHT") != height))
  {
    IupSetHandle(ih->data->image_name, NULL);
    IupDestroy(ih->data->image);
    ih->data->image = NULL;
  }
  if (!ih->data->image)
  {
    ih->data->image = IupImageRGB(width, height, ih->data->frame);
    if (ih->data->image)
      IupSetHandle(ih->data->image_name, ih->data->image);
  }
  else
  {
    unsigned char* pixels = (unsigned char*)iupAttribGet(ih->data->image, "WID");
    memcpy(pixels, ih->data->frame, (size_t)width * height * 3);
    IupSetAttribute(ih->data->image, "CLEARCACHE", "YES");
  }
  IupUpdate(ih);
}

static int iVideoTick(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUPVIDEO");
  IvideoLifetime* lifetime = ih->data->lifetime;
  IvideoSession* session = ih->data->session;
  IvideoInfo info;
  unsigned long generation = lifetime->generation;
  int previous = ih->data->reported_state;
  lifetime->references++;
  if (session)
  {
    session->backend->poll(session);
    iupdrvMutexLock(session->mutex);
    info = session->info;
    session->info.ended = 0;
    iupdrvMutexUnlock(session->mutex);
    ih->data->info = info;
    iVideoTakeFrame(ih, session);
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
    if (info.error[0])
      ih->data->state = IVIDEO_ERROR;
    else if (info.ready)
    {
      int first = !ih->data->ready;
      ih->data->ready = 1;
      if (first)
      {
        iVideoSettings(ih);
        session->backend->play(session, ih->data->intent == 2);
        ih->data->state = ih->data->intent == 2 ? IVIDEO_BUFFERING : (ih->data->intent ? IVIDEO_PAUSED : IVIDEO_STOPPED);
        if (!iVideoCallback(ih, lifetime, "READY_CB"))
          goto done;
      }
      else if (!ih->data->ended)
        ih->data->state = ih->data->intent == 2 ? info.state : (ih->data->intent ? IVIDEO_PAUSED : IVIDEO_STOPPED);
      if (ih->data->pending_seek)
      {
        if (info.seekable)
          session->backend->seek(session, ih->data->seek_position);
        ih->data->pending_seek = 0;
        info.ended = 0;
      }
      if (info.ended && !ih->data->ended)
      {
        if (iupAttribGetBoolean(ih, "LOOP") && info.seekable)
        {
          session->backend->seek(session, 0);
          session->backend->play(session, ih->data->intent == 2);
        }
        else
        {
          ih->data->ended = 1;
          ih->data->state = IVIDEO_ENDED;
          if (!iVideoCallback(ih, lifetime, "PLAYEND_CB"))
            goto done;
        }
      }
    }
  }
  if (previous != ih->data->state)
  {
    IvideoStringCallback cb = (IvideoStringCallback)IupGetCallback(ih, "STATECHANGED_CB");
    ih->data->reported_state = ih->data->state;
    if (cb && cb(ih, iVideoStates[ih->data->state]) == IUP_CLOSE)
      IupExitLoop();
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
  }
  if (ih->data->state == IVIDEO_ERROR && !ih->data->reported_error)
  {
    IvideoStringCallback cb = (IvideoStringCallback)IupGetCallback(ih, "ERROR_CB");
    char message[512];
    iupStrCopyN(message, sizeof(message), ih->data->info.error);
    ih->data->reported_error = 1;
    if (cb && cb(ih, message) == IUP_CLOSE)
      IupExitLoop();
    if (!lifetime->ih || lifetime->generation != generation)
      goto done;
  }
  if (!ih->data->session)
    IupSetAttribute(timer, "RUN", "NO");
done:
  if (--lifetime->references == 0)
    free(lifetime);
  return IUP_DEFAULT;
}

static int iVideoRedraw(Ihandle* ih)
{
  int w, h, width, height, x = 0, y = 0;
  double aspect = ih->data->info.aspect;
  IupDrawBegin(ih);
  IupDrawGetSize(ih, &w, &h);
  iupAttribSetStr(ih, "DRAWCOLOR", iupAttribGetStr(ih, "BGCOLOR"));
  iupAttribSet(ih, "DRAWSTYLE", "FILL");
  IupDrawRectangle(ih, 0, 0, w - 1, h - 1);
  if (ih->data->image && w > 0 && h > 0)
  {
    if (!(aspect > 0) || !isfinite(aspect))
      aspect = (double)ih->data->frame_width / ih->data->frame_height;
    width = w;
    height = h;
    if (aspect * h > w)
    {
      height = (int)(w / aspect);
      y = (h - height) / 2;
    }
    else
    {
      width = (int)(h * aspect);
      x = (w - width) / 2;
    }
    if (width > 0 && height > 0)
      IupDrawImage(ih, ih->data->image_name, x, y, width, height);
  }
  IupDrawEnd(ih);
  return IUP_DEFAULT;
}

static int iVideoSetSource(Ihandle* ih, const char* value, int url)
{
  char* source = value ? iupStrDup(value) : NULL;
  if (value && !source)
    return 0;
  free(ih->data->source);
  ih->data->source = source;
  ih->data->url = url;
  ih->data->intent = 0;
  ih->data->pending_seek = 0;
  ih->data->seek_position = 0;
  iVideoOpen(ih);
  if (ih->handle)
    IupUpdate(ih);
  return 0;
}

static int iVideoSetFileAttrib(Ihandle* ih, const char* value)
{
  return iVideoSetSource(ih, value, 0);
}

static int iVideoSetUrlAttrib(Ihandle* ih, const char* value)
{
  return iVideoSetSource(ih, value, 1);
}

static char* iVideoGetFileAttrib(Ihandle* ih)
{
  return ih->data->url ? NULL : ih->data->source;
}

static char* iVideoGetUrlAttrib(Ihandle* ih)
{
  return ih->data->url ? ih->data->source : NULL;
}

static char* iVideoGetStateAttrib(Ihandle* ih)
{
  return (char*)iVideoStates[ih->data->state];
}

static char* iVideoGetBackendAttrib(Ihandle* ih)
{
  return ih->data->backend;
}

static char* iVideoGetBackendUsedAttrib(Ihandle* ih)
{
  return ih->data->session ? (char*)ih->data->session->backend->name : NULL;
}

static char* iVideoGetDurationAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->info.duration);
}

static char* iVideoGetPositionAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->info.position);
}

static char* iVideoGetSeekableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->info.seekable);
}

static char* iVideoGetHasVideoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->info.video);
}

static char* iVideoGetHasAudioAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->info.audio);
}

static char* iVideoGetVideoSizeAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->info.width, ih->data->info.height, 'x');
}

static int iVideoSetBackendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->session && value && (iupStrEqualNoCase(value, "AUTO") || iupStrEqualNoCase(value, "NATIVE") ||
      iupStrEqualNoCase(value, "VLC") || iupStrEqualNoCase(value, "GSTREAMER") || iupStrEqualNoCase(value, "MPV")))
  {
    iupStrCopyN(ih->data->backend, sizeof(ih->data->backend), value);
    iupStrUpper(ih->data->backend, ih->data->backend);
  }
  return 0;
}

static char* iVideoGetAvailableAttrib(Ihandle* ih)
{
  int index;
  const IvideoBackend* backend;
  for (index = 0; (backend = iupdrvVideoBackend(index)) != NULL; index++)
    if ((iupStrEqualNoCase(ih->data->backend, "AUTO") || iupStrEqualNoCase(ih->data->backend, backend->name)) && backend->available())
      return "YES";
  return "NO";
}

static int iVideoSetPlayAttrib(Ihandle* ih, const char* value)
{
  IvideoSession* session = ih->data->session;
  (void)value;
  if (!ih->data->source)
    return 0;
  ih->data->intent = 2;
  if (session && ih->data->ready && ih->data->state != IVIDEO_ERROR)
  {
    if (ih->data->ended)
      session->backend->seek(session, 0);
    ih->data->ended = 0;
    session->backend->play(session, 1);
  }
  return 0;
}

static int iVideoSetPauseAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (!ih->data->source)
    return 0;
  ih->data->intent = 1;
  if (ih->data->session && ih->data->ready)
    ih->data->session->backend->play(ih->data->session, 0);
  return 0;
}

static int iVideoSetStopAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  ih->data->intent = 0;
  ih->data->ended = 0;
  ih->data->pending_seek = 1;
  ih->data->seek_position = 0;
  if (ih->data->session && ih->data->ready)
  {
    ih->data->session->backend->play(ih->data->session, 0);
    if (ih->data->info.seekable)
      ih->data->session->backend->seek(ih->data->session, 0);
    ih->data->pending_seek = 0;
  }
  return 0;
}

static int iVideoSetPositionAttrib(Ihandle* ih, const char* value)
{
  double position;
  if (iupStrToDouble(value, &position) && isfinite(position) && position >= 0)
  {
    ih->data->pending_seek = 1;
    ih->data->seek_position = position;
    ih->data->ended = 0;
  }
  return 0;
}

static int iVideoSetVolumeAttrib(Ihandle* ih, const char* value)
{
  int volume;
  if (!iupStrToInt(value, &volume) || volume < 0)
    return 0;
  if (volume > 100)
    volume = 100;
  iVideoVolume(ih, volume, iupAttribGetBoolean(ih, "MUTE"));
  iupAttribSetInt(ih, "VOLUME", volume);
  return 0;
}

static int iVideoSetMuteAttrib(Ihandle* ih, const char* value)
{
  iVideoVolume(ih, iupAttribGetInt(ih, "VOLUME"), iupStrBoolean(value));
  return 1;
}

static int iVideoCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();
  if (!ih->data)
    return IUP_ERROR;
  ih->data->lifetime = (IvideoLifetime*)calloc(1, sizeof(IvideoLifetime));
  if (!ih->data->lifetime)
  {
    free(ih->data);
    ih->data = NULL;
    return IUP_ERROR;
  }
  ih->data->lifetime->ih = ih;
  ih->data->lifetime->references = 1;
  strcpy(ih->data->backend, "AUTO");
  snprintf(ih->data->image_name, sizeof(ih->data->image_name), "_IUP_VIDEO_%p", (void*)ih);
  ih->data->timer = IupTimer();
  if (!ih->data->timer)
  {
    free(ih->data->lifetime);
    free(ih->data);
    ih->data = NULL;
    return IUP_ERROR;
  }
  iupAttribSet(ih->data->timer, "_IUPVIDEO", (char*)ih);
  IupSetInt(ih->data->timer, "TIME", 16);
  IupSetCallback(ih->data->timer, "ACTION_CB", (Icallback)iVideoTick);
  IupSetCallback(ih, "ACTION", (Icallback)iVideoRedraw);
  return IUP_NOERROR;
}

static int iVideoMapMethod(Ihandle* ih)
{
  iVideoOpen(ih);
  return IUP_NOERROR;
}

static void iVideoUnMapMethod(Ihandle* ih)
{
  IupSetAttribute(ih->data->timer, "RUN", "NO");
  iVideoClose(ih);
  ih->data->intent = 0;
  ih->data->pending_seek = 0;
  ih->data->state = ih->data->source ? IVIDEO_STOPPED : IVIDEO_EMPTY;
}

static void iVideoDestroyMethod(Ihandle* ih)
{
  IvideoLifetime* lifetime = ih->data->lifetime;
  if (lifetime)
    lifetime->ih = NULL;
  iVideoClose(ih);
  if (ih->data->timer)
    IupDestroy(ih->data->timer);
  free(ih->data->source);
  if (lifetime && --lifetime->references == 0)
    free(lifetime);
}

static void iVideoLayoutUpdateMethod(Ihandle* ih)
{
  IvideoSession* session = ih->data->session;
  if (session && session->backend->layout)
    session->backend->layout(session, ih);
}

static void iVideoComputeNaturalSizeMethod(Ihandle* ih, int* width, int* height, int* expand)
{
  int cw, ch;
  (void)expand;
  iupdrvFontGetCharSize(ih, &cw, &ch);
  *width = cw * 48;
  *height = *width * 9 / 16;
}

IUPMEDIA_API Ihandle* IupVideo(void)
{
  return IupCreate("video");
}

Iclass* iupVideoNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));
  int flags = IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT;
  ic->name = "video";
  ic->cons = "Video";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->New = iupVideoNewClass;
  ic->Create = iVideoCreateMethod;
  ic->Map = iVideoMapMethod;
  ic->UnMap = iVideoUnMapMethod;
  ic->Destroy = iVideoDestroyMethod;
  ic->LayoutUpdate = iVideoLayoutUpdateMethod;
  ic->ComputeNaturalSize = iVideoComputeNaturalSizeMethod;

  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", NULL);
  iupClassRegisterReplaceAttribDef(ic, "BGCOLOR", "0 0 0", NULL);
  iupClassRegisterReplaceAttribDef(ic, "CANFOCUS", "NO", NULL);

  iupClassRegisterCallback(ic, "READY_CB", "");
  iupClassRegisterCallback(ic, "STATECHANGED_CB", "s");
  iupClassRegisterCallback(ic, "PLAYEND_CB", "");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterAttribute(ic, "FILE", iVideoGetFileAttrib, iVideoSetFileAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "URL", iVideoGetUrlAttrib, iVideoSetUrlAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "BACKEND", iVideoGetBackendAttrib, iVideoSetBackendAttrib, IUPAF_SAMEASSYSTEM, "AUTO", flags);
  iupClassRegisterAttribute(ic, "BACKENDUSED", iVideoGetBackendUsedAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "AVAILABLE", iVideoGetAvailableAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "STATE", iVideoGetStateAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "PLAY", NULL, iVideoSetPlayAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "PAUSE", NULL, iVideoSetPauseAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "STOP", NULL, iVideoSetStopAttrib, NULL, NULL, flags | IUPAF_WRITEONLY);
  iupClassRegisterAttribute(ic, "POSITION", iVideoGetPositionAttrib, iVideoSetPositionAttrib, NULL, NULL, flags);
  iupClassRegisterAttribute(ic, "DURATION", iVideoGetDurationAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "VIDEOSIZE", iVideoGetVideoSizeAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "SEEKABLE", iVideoGetSeekableAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "HASVIDEO", iVideoGetHasVideoAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "HASAUDIO", iVideoGetHasAudioAttrib, NULL, NULL, NULL, flags | IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "LOOP", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", flags);
  iupClassRegisterAttribute(ic, "VOLUME", NULL, iVideoSetVolumeAttrib, IUPAF_SAMEASSYSTEM, "100", flags);
  iupClassRegisterAttribute(ic, "MUTE", NULL, iVideoSetMuteAttrib, IUPAF_SAMEASSYSTEM, "NO", flags);
  return ic;
}
