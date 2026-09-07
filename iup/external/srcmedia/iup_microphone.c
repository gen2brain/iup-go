/** \file
 * \brief Microphone control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupmedia.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_thread.h"
#include "iup_media.h"
#include "iup_miniaudio.h"
#include "iup_microphone.h"

#define IMIC_PENDING_SECONDS 4

enum { IMIC_MSG_SAMPLES, IMIC_MSG_ERROR, IMIC_MSG_PERMISSION };

typedef struct _ImicMsg
{
  int type;
  int granted;
  int stop;
  char message[256];
} ImicMsg;

struct _IcontrolData
{
  void* mutex;
  short* pending;
  int pending_frames, pending_capacity, pending_posted;
  short* block;
  int block_frames, block_capacity;
  int running, channels, samplerate;
  int level;
  ma_encoder encoder;
  int recording;
};

static void iMicPost(Ihandle* ih, const ImicMsg* msg)
{
  ImicMsg* copy = (ImicMsg*)malloc(sizeof(ImicMsg));
  if (!copy)
    return;
  *copy = *msg;
  IupPostMessage(ih, NULL, 0, 0, copy);
}

void iupMicrophoneSamples(Ihandle* ih, const short* samples, int frames)
{
  ImicMsg msg;
  int channels = ih->data->channels;
  int limit = ih->data->samplerate * IMIC_PENDING_SECONDS;
  int post = 0;

  iupdrvMutexLock(ih->data->mutex);
  if (ih->data->pending_frames + frames > limit)
    frames = limit - ih->data->pending_frames;
  if (frames > 0)
  {
    if (ih->data->pending_frames + frames > ih->data->pending_capacity)
    {
      int capacity = ih->data->pending_capacity ? ih->data->pending_capacity * 2 : 4096;
      short* buffer;
      while (capacity < ih->data->pending_frames + frames)
        capacity *= 2;
      buffer = (short*)realloc(ih->data->pending, (size_t)capacity * channels * sizeof(short));
      if (!buffer)
      {
        iupdrvMutexUnlock(ih->data->mutex);
        return;
      }
      ih->data->pending = buffer;
      ih->data->pending_capacity = capacity;
    }
    memcpy(ih->data->pending + (size_t)ih->data->pending_frames * channels, samples, (size_t)frames * channels * sizeof(short));
    ih->data->pending_frames += frames;
    if (!ih->data->pending_posted)
    {
      ih->data->pending_posted = 1;
      post = 1;
    }
  }
  iupdrvMutexUnlock(ih->data->mutex);

  if (post)
  {
    msg.type = IMIC_MSG_SAMPLES;
    iMicPost(ih, &msg);
  }
}

static void iMicPostError(Ihandle* ih, const char* message, int stop)
{
  ImicMsg msg;
  msg.type = IMIC_MSG_ERROR;
  msg.stop = stop;
  strncpy(msg.message, message ? message : "", sizeof(msg.message) - 1);
  msg.message[sizeof(msg.message) - 1] = 0;
  iMicPost(ih, &msg);
}

void iupMicrophoneError(Ihandle* ih, const char* message)
{
  iMicPostError(ih, message, 1);
}

void iupMicrophonePermission(Ihandle* ih, int granted)
{
  ImicMsg msg;
  msg.type = IMIC_MSG_PERMISSION;
  msg.granted = granted;
  iMicPost(ih, &msg);
}

static void iMicTakeBlock(Ihandle* ih)
{
  short* buffer;
  int capacity;

  iupdrvMutexLock(ih->data->mutex);
  buffer = ih->data->block;
  capacity = ih->data->block_capacity;
  ih->data->block = ih->data->pending;
  ih->data->block_frames = ih->data->pending_frames;
  ih->data->block_capacity = ih->data->pending_capacity;
  ih->data->pending = buffer;
  ih->data->pending_frames = 0;
  ih->data->pending_capacity = capacity;
  ih->data->pending_posted = 0;
  iupdrvMutexUnlock(ih->data->mutex);
}

static void iMicCloseFile(Ihandle* ih)
{
  if (!ih->data->recording)
    return;
  ma_encoder_uninit(&ih->data->encoder);
  ih->data->recording = 0;
}

static int iMicOpenFile(Ihandle* ih, const char* path)
{
  ma_encoder_config config;
  ma_result result;

  iMicCloseFile(ih);
  if (!path || !ih->data->running)
    return 1;

  config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, (ma_uint32)ih->data->channels, (ma_uint32)ih->data->samplerate);
  result = ma_encoder_init_file(path, &config, &ih->data->encoder);
  if (result != MA_SUCCESS)
  {
    iMicPostError(ih, ma_result_description(result), 0);
    return 0;
  }

  ih->data->recording = 1;
  return 1;
}

static void iMicStop(Ihandle* ih)
{
  if (!ih->data->running)
    return;

  iupdrvMicrophoneStop(ih);
  iMicCloseFile(ih);
  ih->data->running = 0;
  ih->data->level = 0;
}

static int iMicPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  ImicMsg* msg = (ImicMsg*)p;
  (void)s; (void)i; (void)d;

  if (!msg)
    return IUP_DEFAULT;

  if (msg->type == IMIC_MSG_SAMPLES)
  {
    iMicTakeBlock(ih);

    if (ih->data->running && ih->data->block_frames > 0)
    {
      IFniiV cb;
      int count = ih->data->block_frames * ih->data->channels;
      int peak = 0, k;

      for (k = 0; k < count; k++)
      {
        int v = ih->data->block[k];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
      }
      ih->data->level = (peak * 100) / 32767;

      if (ih->data->recording)
      {
        ma_result result = ma_encoder_write_pcm_frames(&ih->data->encoder, ih->data->block, (ma_uint64)ih->data->block_frames, NULL);
        if (result != MA_SUCCESS)
        {
          iMicCloseFile(ih);
          iMicPostError(ih, ma_result_description(result), 0);
        }
      }

      cb = (IFniiV)IupGetCallback(ih, "SAMPLES_CB");
      if (cb && cb(ih, ih->data->block_frames, ih->data->channels, ih->data->block) == IUP_CLOSE)
        IupExitLoop();
    }
  }
  else if (msg->type == IMIC_MSG_ERROR)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (msg->stop)
      iMicStop(ih);
    if (cb && cb(ih, msg->message) == IUP_CLOSE)
      IupExitLoop();
  }
  else if (msg->type == IMIC_MSG_PERMISSION)
  {
    IFni cb = (IFni)IupGetCallback(ih, "PERMISSION_CB");
    if (cb && cb(ih, msg->granted) == IUP_CLOSE)
      IupExitLoop();
  }

  free(msg);
  return IUP_DEFAULT;
}

static int iMicStart(Ihandle* ih, int device)
{
  int channels = iupAttribGetInt(ih, "CHANNELS");
  int samplerate = iupAttribGetInt(ih, "SAMPLERATE");

  if (channels < 1) channels = 1;
  if (samplerate < 1) samplerate = 44100;

  ih->data->channels = channels;
  ih->data->samplerate = samplerate;
  ih->data->pending_frames = 0;
  ih->data->pending_posted = 0;

  if (!iupdrvMicrophoneStart(ih, device, &ih->data->channels, &ih->data->samplerate))
    return 0;

  ih->data->running = 1;
  iMicOpenFile(ih, iupAttribGet(ih, "FILE"));
  return 1;
}

static int iMicSetRunAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!ih->data->running)
      iMicStart(ih, iupAttribGetInt(ih, "DEVICE"));
  }
  else
    iMicStop(ih);
  return 0;
}

static char* iMicGetRunAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->running);
}

static int iMicSetDeviceAttrib(Ihandle* ih, const char* value)
{
  int device;
  if (!iupStrToInt(value, &device) || device < 0)
    return 0;

  if (ih->data->running)
  {
    iMicStop(ih);
    iMicStart(ih, device);
  }
  return 1;
}

static int iMicSetFileAttrib(Ihandle* ih, const char* value)
{
  iMicOpenFile(ih, value);
  return 1;
}

static char* iMicGetChannelsAttrib(Ihandle* ih)
{
  if (ih->data->running)
    return iupStrReturnInt(ih->data->channels);
  return NULL;
}

static char* iMicGetSampleRateAttrib(Ihandle* ih)
{
  if (ih->data->running)
    return iupStrReturnInt(ih->data->samplerate);
  return NULL;
}

static char* iMicGetLevelAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->level);
}

static char* iMicGetDeviceCountAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnInt(iupdrvMicrophoneGetDeviceCount());
}

static char* iMicGetDeviceNameAttrib(Ihandle* ih, int id)
{
  (void)ih;
  return iupdrvMicrophoneGetDeviceName(id);
}

static char* iMicGetAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(iupdrvMicrophoneIsAvailable());
}

static char* iMicGetPermissionAttrib(Ihandle* ih)
{
  return iupdrvMicrophoneGetPermission(ih);
}

static int iMicCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  ih->data = iupALLOCCTRLDATA();
  ih->data->mutex = iupdrvMutexCreate();
  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)iMicPostMessage);
  return IUP_NOERROR;
}

static void iMicDestroyMethod(Ihandle* ih)
{
  iMicStop(ih);
  iupdrvMutexDestroy(ih->data->mutex);
  free(ih->data->pending);
  free(ih->data->block);
}

IUPMEDIA_API Ihandle* IupMicrophone(void)
{
  return IupCreate("microphone");
}

Iclass* iupMicrophoneNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "microphone";
  ic->cons = "Microphone";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;
  ic->has_attrib_id = 1;

  ic->New = iupMicrophoneNewClass;
  ic->Create = iMicCreateMethod;
  ic->Destroy = iMicDestroyMethod;

  iupClassRegisterCallback(ic, "SAMPLES_CB", "iiV");
  iupClassRegisterCallback(ic, "PERMISSION_CB", "i");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterAttribute(ic, "DEVICE", NULL, iMicSetDeviceAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DEVICECOUNT", iMicGetDeviceCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEVICENAME", iMicGetDeviceNameAttrib, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RUN", iMicGetRunAttrib, iMicSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CHANNELS", iMicGetChannelsAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAMPLERATE", iMicGetSampleRateAttrib, NULL, IUPAF_SAMEASSYSTEM, "44100", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILE", NULL, iMicSetFileAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEVEL", iMicGetLevelAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AVAILABLE", iMicGetAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PERMISSION", iMicGetPermissionAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvMicrophoneInitClass(ic);

  return ic;
}
