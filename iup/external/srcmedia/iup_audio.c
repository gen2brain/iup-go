/** \file
 * \brief Audio player
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupmedia.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_media.h"
#include "iup_miniaudio.h"

enum { IAUDIO_MSG_END, IAUDIO_MSG_ERROR };

typedef struct _IaudioEngine
{
  ma_engine engine;
  ma_device_id id;
  int refs;
  struct _IaudioEngine* next;
} IaudioEngine;

struct _IcontrolData
{
  IaudioEngine* engine;
  ma_decoder decoder;
  ma_sound sound;
  int loaded;
  int paused;
};

static IaudioEngine iaudio_default_engine;
static int iaudio_default_ready = 0;
static IaudioEngine* iaudio_engines = NULL;
static ma_device_info* iaudio_devices = NULL;
static int iaudio_device_count = 0;

static int iAudioDefaultEngineInit(void)
{
  ma_engine_config config;

  if (iaudio_default_ready)
    return 1;

  config = ma_engine_config_init();
  config.pDevice = iupdrvAudioDeviceInit(&iaudio_default_engine.engine);
  if (ma_engine_init(&config, &iaudio_default_engine.engine) != MA_SUCCESS)
  {
    iupdrvAudioDeviceRelease();
    return 0;
  }

  iaudio_default_ready = 1;
  return 1;
}

static void iAudioEngineRelease(Iclass* ic)
{
  (void)ic;

  while (iaudio_engines)
  {
    IaudioEngine* engine = iaudio_engines;
    iaudio_engines = engine->next;
    ma_engine_uninit(&engine->engine);
    free(engine);
  }

  free(iaudio_devices);
  iaudio_devices = NULL;
  iaudio_device_count = 0;

  if (iaudio_default_ready)
  {
    ma_engine_uninit(&iaudio_default_engine.engine);
    iupdrvAudioDeviceRelease();
    iaudio_default_ready = 0;
  }
}

static void iAudioListDevices(void)
{
  free(iaudio_devices);
  iaudio_device_count = iupdrvAudioDeviceList(&iaudio_devices);
}

static IaudioEngine* iAudioEngineAcquire(int device)
{
  ma_engine_config config;
  ma_device_id* id;
  IaudioEngine* engine;

  if (device > 0 && !iaudio_devices)
    iAudioListDevices();

  if (device <= 0 || device > iaudio_device_count)
  {
    iaudio_default_engine.refs++;
    return &iaudio_default_engine;
  }

  id = &iaudio_devices[device - 1].id;
  for (engine = iaudio_engines; engine; engine = engine->next)
  {
    if (ma_device_id_equal(&engine->id, id))
    {
      engine->refs++;
      return engine;
    }
  }

  engine = (IaudioEngine*)calloc(1, sizeof(IaudioEngine));
  if (!engine)
    return NULL;
  engine->id = *id;

  config = ma_engine_config_init();
  config.pContext = iupdrvAudioContext();
  config.pPlaybackDeviceID = &engine->id;
  if (ma_engine_init(&config, &engine->engine) != MA_SUCCESS)
  {
    free(engine);
    return NULL;
  }

  engine->refs = 1;
  engine->next = iaudio_engines;
  iaudio_engines = engine;
  return engine;
}

static void iAudioEngineUnref(IaudioEngine* engine)
{
  IaudioEngine** link;

  engine->refs--;
  if (engine == &iaudio_default_engine || engine->refs > 0)
    return;

  for (link = &iaudio_engines; *link; link = &(*link)->next)
  {
    if (*link == engine)
    {
      *link = engine->next;
      break;
    }
  }

  ma_engine_uninit(&engine->engine);
  free(engine);
}

static void iAudioEndCallback(void* user_data, ma_sound* sound)
{
  (void)sound;
  IupPostMessage((Ihandle*)user_data, NULL, IAUDIO_MSG_END, 0, NULL);
}

static int iAudioPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  (void)d; (void)p;

  if (i == IAUDIO_MSG_END)
  {
    Icallback cb = IupGetCallback(ih, "PLAYEND_CB");
    if (cb && cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
  else if (i == IAUDIO_MSG_ERROR)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb && cb(ih, (char*)s) == IUP_CLOSE)
      IupExitLoop();
  }

  return IUP_DEFAULT;
}

static void iAudioUnload(Ihandle* ih)
{
  if (!ih->data->loaded)
    return;

  ma_sound_uninit(&ih->data->sound);
  ma_decoder_uninit(&ih->data->decoder);
  ih->data->loaded = 0;
  ih->data->paused = 0;
}

static float iAudioPercent(int value)
{
  if (value < -100) value = -100;
  if (value > 100) value = 100;
  return (float)value / 100.0f;
}

static void iAudioApplySettings(Ihandle* ih)
{
  ma_sound* sound = &ih->data->sound;
  ma_sound_set_volume(sound, iAudioPercent(iupAttribGetInt(ih, "VOLUME")));
  ma_sound_set_pan(sound, iAudioPercent(iupAttribGetInt(ih, "PAN")));
  ma_sound_set_pitch(sound, (float)iupAttribGetDouble(ih, "PITCH"));
  ma_sound_set_looping(sound, iupAttribGetBoolean(ih, "LOOP"));
}

static ma_result iAudioSoundInit(Ihandle* ih)
{
  ma_result result = ma_sound_init_from_data_source(&ih->data->engine->engine, &ih->data->decoder, MA_SOUND_FLAG_NO_SPATIALIZATION, NULL, &ih->data->sound);
  if (result != MA_SUCCESS)
    return result;

  ma_sound_set_end_callback(&ih->data->sound, iAudioEndCallback, ih);
  iAudioApplySettings(ih);
  return MA_SUCCESS;
}

static int iAudioSetFileAttrib(Ihandle* ih, const char* value)
{
  ma_result result;

  iAudioUnload(ih);

  if (!value)
    return 1;

  result = ma_decoder_init_file(value, NULL, &ih->data->decoder);
  if (result == MA_SUCCESS)
  {
    result = iAudioSoundInit(ih);
    if (result != MA_SUCCESS)
      ma_decoder_uninit(&ih->data->decoder);
  }

  if (result != MA_SUCCESS)
  {
    IupPostMessage(ih, ma_result_description(result), IAUDIO_MSG_ERROR, 0, NULL);
    return 0;
  }

  ih->data->loaded = 1;

  {
    ma_uint64 length;
    ma_sound_get_length_in_pcm_frames(&ih->data->sound, &length);
  }

  return 1;
}

static int iAudioSetAudioDeviceAttrib(Ihandle* ih, const char* value)
{
  IaudioEngine* engine;
  ma_uint64 cursor = 0;
  int playing = 0;
  int device = 0;

  if (value && (!iupStrToInt(value, &device) || device < 0))
    return 0;

  engine = iAudioEngineAcquire(device);
  if (!engine)
  {
    IupPostMessage(ih, "Audio device not available", IAUDIO_MSG_ERROR, 0, NULL);
    return 0;
  }

  if (engine == ih->data->engine)
  {
    iAudioEngineUnref(engine);
    return 1;
  }

  if (ih->data->loaded)
  {
    ma_sound_get_cursor_in_pcm_frames(&ih->data->sound, &cursor);
    playing = ma_sound_is_playing(&ih->data->sound);
    ma_sound_uninit(&ih->data->sound);
  }

  iAudioEngineUnref(ih->data->engine);
  ih->data->engine = engine;

  if (ih->data->loaded)
  {
    ma_result result = iAudioSoundInit(ih);
    if (result != MA_SUCCESS)
    {
      ma_decoder_uninit(&ih->data->decoder);
      ih->data->loaded = 0;
      ih->data->paused = 0;
      IupPostMessage(ih, ma_result_description(result), IAUDIO_MSG_ERROR, 0, NULL);
      return 1;
    }

    ma_sound_seek_to_pcm_frame(&ih->data->sound, cursor);
    if (playing)
      ma_sound_start(&ih->data->sound);
  }

  return 1;
}

static char* iAudioGetAudioDeviceCountAttrib(Ihandle* ih)
{
  (void)ih;
  iAudioListDevices();
  return iupStrReturnInt(iaudio_device_count + 1);
}

static char* iAudioGetAudioDeviceNameAttrib(Ihandle* ih, int id)
{
  (void)ih;
  if (id == 0)
    return "Default";
  if (!iaudio_devices)
    iAudioListDevices();
  if (id < 0 || id > iaudio_device_count)
    return NULL;
  return iupStrReturnStr(iaudio_devices[id - 1].name);
}

static int iAudioSetPlayAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->loaded)
  {
    ma_sound_start(&ih->data->sound);
    ih->data->paused = 0;
  }
  return 0;
}

static int iAudioSetPauseAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->loaded && ma_sound_is_playing(&ih->data->sound))
  {
    ma_sound_stop(&ih->data->sound);
    ih->data->paused = 1;
  }
  return 0;
}

static int iAudioSetStopAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->loaded)
  {
    ma_sound_stop(&ih->data->sound);
    ma_sound_seek_to_pcm_frame(&ih->data->sound, 0);
    ih->data->paused = 0;
  }
  return 0;
}

static char* iAudioGetStateAttrib(Ihandle* ih)
{
  if (ih->data->loaded)
  {
    if (ma_sound_is_playing(&ih->data->sound))
      return "PLAYING";
    if (ih->data->paused)
      return "PAUSED";
  }
  return "STOPPED";
}

static int iAudioSetVolumeAttrib(Ihandle* ih, const char* value)
{
  int volume;
  if (!iupStrToInt(value, &volume) || volume < 0)
    return 0;
  if (ih->data->loaded)
    ma_sound_set_volume(&ih->data->sound, iAudioPercent(volume));
  return 1;
}

static int iAudioSetPanAttrib(Ihandle* ih, const char* value)
{
  int pan;
  if (!iupStrToInt(value, &pan))
    return 0;
  if (ih->data->loaded)
    ma_sound_set_pan(&ih->data->sound, iAudioPercent(pan));
  return 1;
}

static int iAudioSetPitchAttrib(Ihandle* ih, const char* value)
{
  double pitch;
  if (!iupStrToDouble(value, &pitch) || pitch <= 0)
    return 0;
  if (ih->data->loaded)
    ma_sound_set_pitch(&ih->data->sound, (float)pitch);
  return 1;
}

static int iAudioSetLoopAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->loaded)
    ma_sound_set_looping(&ih->data->sound, iupStrBoolean(value));
  return 1;
}

static int iAudioSetPositionAttrib(Ihandle* ih, const char* value)
{
  double position;
  if (!iupStrToDouble(value, &position) || position < 0)
    return 0;
  if (ih->data->loaded)
    ma_sound_seek_to_second(&ih->data->sound, (float)position);
  return 0;
}

static char* iAudioGetPositionAttrib(Ihandle* ih)
{
  float position = 0;
  if (ih->data->loaded)
    ma_sound_get_cursor_in_seconds(&ih->data->sound, &position);
  return iupStrReturnDouble(position);
}

static char* iAudioGetDurationAttrib(Ihandle* ih)
{
  float length = 0;
  if (ih->data->loaded)
    ma_sound_get_length_in_seconds(&ih->data->sound, &length);
  return iupStrReturnDouble(length);
}

static char* iAudioGetChannelsAttrib(Ihandle* ih)
{
  ma_uint32 channels = 0;
  if (ih->data->loaded)
    ma_decoder_get_data_format(&ih->data->decoder, NULL, &channels, NULL, NULL, 0);
  return iupStrReturnInt((int)channels);
}

static char* iAudioGetSampleRateAttrib(Ihandle* ih)
{
  ma_uint32 rate = 0;
  if (ih->data->loaded)
    ma_decoder_get_data_format(&ih->data->decoder, NULL, NULL, &rate, NULL, 0);
  return iupStrReturnInt((int)rate);
}

static int iAudioCreateMethod(Ihandle* ih, void** params)
{
  (void)params;

  if (!iAudioDefaultEngineInit())
    return IUP_ERROR;

  ih->data = iupALLOCCTRLDATA();
  ih->data->engine = iAudioEngineAcquire(0);
  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)iAudioPostMessage);
  return IUP_NOERROR;
}

static void iAudioDestroyMethod(Ihandle* ih)
{
  iAudioUnload(ih);
  iAudioEngineUnref(ih->data->engine);
}

Iclass* iupAudioNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "audio";
  ic->cons = "Audio";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;
  ic->has_attrib_id = 1;

  ic->New = iupAudioNewClass;
  ic->Release = iAudioEngineRelease;
  ic->Create = iAudioCreateMethod;
  ic->Destroy = iAudioDestroyMethod;

  iupClassRegisterCallback(ic, "PLAYEND_CB", "");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterAttribute(ic, "FILE", NULL, iAudioSetFileAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLAY", NULL, iAudioSetPlayAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAUSE", NULL, iAudioSetPauseAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, iAudioSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATE", iAudioGetStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VOLUME", NULL, iAudioSetVolumeAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAN", NULL, iAudioSetPanAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PITCH", NULL, iAudioSetPitchAttrib, IUPAF_SAMEASSYSTEM, "1.0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LOOP", NULL, iAudioSetLoopAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSITION", iAudioGetPositionAttrib, iAudioSetPositionAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DURATION", iAudioGetDurationAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CHANNELS", iAudioGetChannelsAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAMPLERATE", iAudioGetSampleRateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUDIODEVICE", NULL, iAudioSetAudioDeviceAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUDIODEVICECOUNT", iAudioGetAudioDeviceCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "AUDIODEVICENAME", iAudioGetAudioDeviceNameAttrib, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}
