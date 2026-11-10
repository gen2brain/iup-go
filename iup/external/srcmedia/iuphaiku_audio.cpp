/** \file
 * \brief Audio device, miniaudio backend on the Haiku Media Kit
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include <MediaRoster.h>
#include <SoundPlayer.h>
#include <MediaDefs.h>

#include "iup.h"
#include "iup_str.h"
#include "iup_miniaudio.h"

#define IHAIKU_AUDIO_SLOTS 8
#define IHAIKU_AUDIO_NODES 32

typedef struct _IhaikuAudioSlot
{
  ma_device* device;
  BSoundPlayer* player;
} IhaikuAudioSlot;

static ma_context ihaiku_audio_context;
static int ihaiku_audio_context_ready = 0;
static ma_device ihaiku_audio_device;
static int ihaiku_audio_ready = 0;
static IhaikuAudioSlot ihaiku_audio_slots[IHAIKU_AUDIO_SLOTS];

static int haikuAudioSlot(ma_device* device)
{
  for (int slot = 0; slot < IHAIKU_AUDIO_SLOTS; slot++)
  {
    if (ihaiku_audio_slots[slot].device == device)
      return slot;
  }
  return -1;
}

static int haikuAudioOutputNodes(live_node_info* nodes)
{
  media_format format;
  int32 count = IHAIKU_AUDIO_NODES;

  format.type = B_MEDIA_RAW_AUDIO;
  format.u.raw_audio = media_raw_audio_format::wildcard;
  if (BMediaRoster::Roster()->GetLiveNodes(nodes, &count, &format, nullptr, nullptr, B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT) != B_OK)
    return 0;
  return static_cast<int>(count);
}

static void haikuAudioPlayBuffer(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
  auto* slot = static_cast<IhaikuAudioSlot*>(cookie);
  auto frames = static_cast<ma_uint32>(size / (format.channel_count * sizeof(float)));
  ma_device_handle_backend_data_callback(slot->device, buffer, nullptr, frames);
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

static BSoundPlayer* haikuAudioCreatePlayer(const ma_device_id* id, const media_raw_audio_format& format, IhaikuAudioSlot* slot)
{
  BMediaRoster* roster = BMediaRoster::Roster();
  media_node output;
  bool mixer_output = true;

  if (id && roster->GetAudioOutput(&output) == B_OK)
  {
    mixer_output = output.node == id->custom.i;
    roster->ReleaseNode(output);
  }

  if (!mixer_output)
  {
    live_node_info nodes[IHAIKU_AUDIO_NODES];
    int count = haikuAudioOutputNodes(nodes);
    for (int i = 0; i < count; i++)
    {
      if (nodes[i].node.node == id->custom.i)
      {
        media_multi_audio_format multi = media_multi_audio_format::wildcard;
        static_cast<media_raw_audio_format&>(multi) = format;
        return new BSoundPlayer(nodes[i].node, &multi, "IupAudio", nullptr, haikuAudioPlayBuffer, nullptr, slot);
      }
    }
    return nullptr;
  }

  return new BSoundPlayer(&format, "IupAudio", haikuAudioPlayBuffer, nullptr, slot);
}

static ma_result haikuAudioDeviceInit(ma_device* device, const ma_device_config* config, ma_device_descriptor* playback, ma_device_descriptor* capture)
{
  media_raw_audio_format format = media_raw_audio_format::wildcard;
  int index = haikuAudioSlot(nullptr);
  IhaikuAudioSlot* slot;
  (void)capture;

  if (config->deviceType != ma_device_type_playback)
    return MA_DEVICE_TYPE_NOT_SUPPORTED;
  if (index < 0)
    return MA_TOO_MANY_OPEN_FILES;

  if (playback->sampleRate == 0)
    playback->sampleRate = 48000;
  if (playback->channels == 0)
    playback->channels = 2;
  playback->periodSizeInFrames = ma_calculate_buffer_size_in_frames_from_descriptor(playback, playback->sampleRate, config->performanceProfile);

  format.format = media_raw_audio_format::B_AUDIO_FLOAT;
  format.byte_order = B_MEDIA_HOST_ENDIAN;
  format.frame_rate = static_cast<float>(playback->sampleRate);
  format.channel_count = playback->channels;
  format.buffer_size = playback->periodSizeInFrames * playback->channels * sizeof(float);

  slot = &ihaiku_audio_slots[index];
  slot->device = device;
  slot->player = haikuAudioCreatePlayer(playback->pDeviceID, format, slot);
  if (!slot->player || slot->player->InitCheck() != B_OK)
  {
    delete slot->player;
    slot->player = nullptr;
    slot->device = nullptr;
    return MA_FAILED_TO_OPEN_BACKEND_DEVICE;
  }

  format = slot->player->Format();
  playback->format = ma_format_f32;
  playback->channels = format.channel_count;
  playback->sampleRate = static_cast<ma_uint32>(format.frame_rate);
  playback->periodSizeInFrames = static_cast<ma_uint32>(format.buffer_size / (format.channel_count * sizeof(float)));
  playback->periodCount = 1;
  ma_channel_map_init_standard(ma_standard_channel_map_default, playback->channelMap, sizeof(playback->channelMap) / sizeof(playback->channelMap[0]), playback->channels);
  return MA_SUCCESS;
}

static ma_result haikuAudioDeviceUninit(ma_device* device)
{
  int index = haikuAudioSlot(device);
  if (index < 0)
    return MA_SUCCESS;

  delete ihaiku_audio_slots[index].player;
  ihaiku_audio_slots[index].player = nullptr;
  ihaiku_audio_slots[index].device = nullptr;
  return MA_SUCCESS;
}

static ma_result haikuAudioDeviceStart(ma_device* device)
{
  int index = haikuAudioSlot(device);
  if (index < 0)
    return MA_INVALID_OPERATION;
  return ihaiku_audio_slots[index].player->Start() == B_OK ? MA_SUCCESS : MA_FAILED_TO_START_BACKEND_DEVICE;
}

static ma_result haikuAudioDeviceStop(ma_device* device)
{
  int index = haikuAudioSlot(device);
  if (index >= 0)
    ihaiku_audio_slots[index].player->Stop();
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
  ma_engine_read_pcm_frames(static_cast<ma_engine*>(device->pUserData), output, frames, nullptr);
}

extern "C" ma_context* iupdrvAudioContext(void)
{
  if (!ihaiku_audio_context_ready)
  {
    ma_backend backends[] = { ma_backend_custom };
    ma_context_config context_config = ma_context_config_init();
    context_config.custom.onContextInit = haikuAudioContextInit;
    if (ma_context_init(backends, 1, &context_config, &ihaiku_audio_context) != MA_SUCCESS)
      return nullptr;
    ihaiku_audio_context_ready = 1;
  }
  return &ihaiku_audio_context;
}

extern "C" ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  ma_context* context = iupdrvAudioContext();
  ma_device_config device_config;

  if (!context)
    return nullptr;

  device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_f32;
  device_config.dataCallback = haikuAudioEngineData;
  device_config.pUserData = engine;
  if (ma_device_init(context, &device_config, &ihaiku_audio_device) != MA_SUCCESS)
    return nullptr;

  ihaiku_audio_ready = 1;
  return &ihaiku_audio_device;
}

extern "C" void iupdrvAudioDeviceRelease(void)
{
  if (ihaiku_audio_ready)
  {
    ma_device_uninit(&ihaiku_audio_device);
    ihaiku_audio_ready = 0;
  }

  if (ihaiku_audio_context_ready)
  {
    ma_context_uninit(&ihaiku_audio_context);
    ihaiku_audio_context_ready = 0;
  }
}

extern "C" int iupdrvAudioDeviceList(ma_device_info** devices)
{
  live_node_info nodes[IHAIKU_AUDIO_NODES];
  int count = haikuAudioOutputNodes(nodes);

  *devices = nullptr;
  if (count == 0 || !iupdrvAudioContext())
    return 0;

  *devices = static_cast<ma_device_info*>(calloc(static_cast<size_t>(count), sizeof(ma_device_info)));
  if (!*devices)
    return 0;

  for (int i = 0; i < count; i++)
  {
    (*devices)[i].id.custom.i = nodes[i].node.node;
    iupStrCopyN((*devices)[i].name, static_cast<int>(sizeof((*devices)[i].name)), nodes[i].name);
  }
  return count;
}
