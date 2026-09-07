/** \file
 * \brief Audio device, miniaudio backend on the Haiku Media Kit
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include <SoundPlayer.h>
#include <MediaDefs.h>

#include "iup_miniaudio.h"

typedef struct _IhaikuAudioDevice
{
  ma_device device;
  BSoundPlayer* player;
} IhaikuAudioDevice;

static ma_context ihaiku_audio_context;
static IhaikuAudioDevice ihaiku_audio_device;
static int ihaiku_audio_ready = 0;

static void haikuAudioPlayBuffer(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
  IhaikuAudioDevice* device = (IhaikuAudioDevice*)cookie;
  ma_uint32 frames = (ma_uint32)(size / (format.channel_count * sizeof(float)));
  ma_device_handle_backend_data_callback(&device->device, buffer, NULL, frames);
}

static ma_result haikuAudioEnumerateDevices(ma_context* context, ma_enum_devices_callback_proc callback, void* user_data)
{
  ma_device_info info;
  memset(&info, 0, sizeof(info));
  strcpy(info.name, "Media Kit");
  info.isDefault = MA_TRUE;
  callback(context, ma_device_type_playback, &info, user_data);
  return MA_SUCCESS;
}

static ma_result haikuAudioGetDeviceInfo(ma_context* context, ma_device_type type, const ma_device_id* id, ma_device_info* info)
{
  (void)context; (void)id;
  if (type != ma_device_type_playback)
    return MA_NO_DEVICE;

  strcpy(info->name, "Media Kit");
  info->isDefault = MA_TRUE;
  info->nativeDataFormatCount = 1;
  info->nativeDataFormats[0].format = ma_format_f32;
  info->nativeDataFormats[0].channels = 0;
  info->nativeDataFormats[0].sampleRate = 0;
  info->nativeDataFormats[0].flags = 0;
  return MA_SUCCESS;
}

static ma_result haikuAudioDeviceInit(ma_device* base, const ma_device_config* config, ma_device_descriptor* playback, ma_device_descriptor* capture)
{
  IhaikuAudioDevice* device = (IhaikuAudioDevice*)base;
  media_raw_audio_format format = media_raw_audio_format::wildcard;
  (void)capture;

  if (config->deviceType != ma_device_type_playback)
    return MA_DEVICE_TYPE_NOT_SUPPORTED;

  if (playback->sampleRate == 0)
    playback->sampleRate = 48000;
  if (playback->channels == 0)
    playback->channels = 2;
  playback->periodSizeInFrames = ma_calculate_buffer_size_in_frames_from_descriptor(playback, playback->sampleRate, config->performanceProfile);

  format.format = media_raw_audio_format::B_AUDIO_FLOAT;
  format.byte_order = B_MEDIA_HOST_ENDIAN;
  format.frame_rate = (float)playback->sampleRate;
  format.channel_count = playback->channels;
  format.buffer_size = playback->periodSizeInFrames * playback->channels * sizeof(float);

  device->player = new BSoundPlayer(&format, "IupAudio", haikuAudioPlayBuffer, NULL, device);
  if (device->player->InitCheck() != B_OK)
  {
    delete device->player;
    device->player = NULL;
    return MA_FAILED_TO_OPEN_BACKEND_DEVICE;
  }

  format = device->player->Format();
  playback->format = ma_format_f32;
  playback->channels = format.channel_count;
  playback->sampleRate = (ma_uint32)format.frame_rate;
  playback->periodSizeInFrames = (ma_uint32)(format.buffer_size / (format.channel_count * sizeof(float)));
  playback->periodCount = 1;
  ma_channel_map_init_standard(ma_standard_channel_map_default, playback->channelMap, sizeof(playback->channelMap) / sizeof(playback->channelMap[0]), playback->channels);
  return MA_SUCCESS;
}

static ma_result haikuAudioDeviceUninit(ma_device* base)
{
  IhaikuAudioDevice* device = (IhaikuAudioDevice*)base;
  delete device->player;
  device->player = NULL;
  return MA_SUCCESS;
}

static ma_result haikuAudioDeviceStart(ma_device* base)
{
  IhaikuAudioDevice* device = (IhaikuAudioDevice*)base;
  return device->player->Start() == B_OK ? MA_SUCCESS : MA_FAILED_TO_START_BACKEND_DEVICE;
}

static ma_result haikuAudioDeviceStop(ma_device* base)
{
  IhaikuAudioDevice* device = (IhaikuAudioDevice*)base;
  device->player->Stop();
  return MA_SUCCESS;
}

static ma_result haikuAudioContextUninit(ma_context* context)
{
  (void)context;
  return MA_SUCCESS;
}

static ma_result haikuAudioContextInit(ma_context* context, const ma_context_config* config, ma_backend_callbacks* callbacks)
{
  (void)context; (void)config;
  callbacks->onContextUninit = haikuAudioContextUninit;
  callbacks->onContextEnumerateDevices = haikuAudioEnumerateDevices;
  callbacks->onContextGetDeviceInfo = haikuAudioGetDeviceInfo;
  callbacks->onDeviceInit = haikuAudioDeviceInit;
  callbacks->onDeviceUninit = haikuAudioDeviceUninit;
  callbacks->onDeviceStart = haikuAudioDeviceStart;
  callbacks->onDeviceStop = haikuAudioDeviceStop;
  return MA_SUCCESS;
}

static void haikuAudioEngineData(ma_device* device, void* output, const void* input, ma_uint32 frames)
{
  (void)input;
  ma_engine_read_pcm_frames((ma_engine*)device->pUserData, output, frames, NULL);
}

extern "C" ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  ma_backend backends[] = { ma_backend_custom };
  ma_context_config context_config = ma_context_config_init();
  ma_device_config device_config;

  context_config.custom.onContextInit = haikuAudioContextInit;
  if (ma_context_init(backends, 1, &context_config, &ihaiku_audio_context) != MA_SUCCESS)
    return NULL;

  device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_f32;
  device_config.dataCallback = haikuAudioEngineData;
  device_config.pUserData = engine;
  if (ma_device_init(&ihaiku_audio_context, &device_config, &ihaiku_audio_device.device) != MA_SUCCESS)
  {
    ma_context_uninit(&ihaiku_audio_context);
    return NULL;
  }

  ihaiku_audio_ready = 1;
  return &ihaiku_audio_device.device;
}

extern "C" void iupdrvAudioDeviceRelease(void)
{
  if (!ihaiku_audio_ready)
    return;

  ma_device_uninit(&ihaiku_audio_device.device);
  ma_context_uninit(&ihaiku_audio_context);
  ihaiku_audio_ready = 0;
}
