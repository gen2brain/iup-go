/** \file
 * \brief Audio device, miniaudio backend feeding a Web Audio worklet through a shared ring
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "iup_miniaudio.h"

#define IWASM_AUDIO_CAPACITY 8192
#define IWASM_AUDIO_PERIOD 512

typedef struct _IwasmAudioDevice
{
  ma_device device;
  float* buffer;
  int running;
} IwasmAudioDevice;

static ma_context iwasm_audio_context;
static IwasmAudioDevice iwasm_audio_device;
static int iwasm_audio_ready = 0;

EM_JS(int, iupwasmJsAudioOpen, (void), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'audioopen' });
  return globalThis.__iupAudioOpen();
})

EM_JS(void, iupwasmJsAudioRingInit, (int capacity, int channels, int rate), {
  var sab = new SharedArrayBuffer(32 + capacity * channels * 4);
  var hdr = new Int32Array(sab, 0, 8);
  hdr[2] = capacity; hdr[3] = channels; hdr[4] = rate;
  globalThis.__iupAudioRing = { sab: sab, hdr: hdr, buf: new Float32Array(sab, 32) };
})

EM_JS(void, iupwasmJsAudioRingRelease, (void), { globalThis.__iupAudioRing = null; })

EM_JS(void, iupwasmJsAudioStart, (void), {
  globalThis.__iupApply({ op: 'audiostart', sab: globalThis.__iupAudioRing.sab });
})

EM_JS(void, iupwasmJsAudioStop, (void), { globalThis.__iupApply({ op: 'audiostop' }); })

EM_JS(int, iupwasmJsAudioFree, (void), {
  var r = globalThis.__iupAudioRing;
  if (!r) return 0;
  return r.hdr[2] - ((Atomics.load(r.hdr, 1) - Atomics.load(r.hdr, 0)) | 0);
})

EM_JS(void, iupwasmJsAudioWrite, (const float* src, int frames), {
  var r = globalThis.__iupAudioRing;
  if (!r) return;
  var ch = r.hdr[3], cap = r.hdr[2], w = Atomics.load(r.hdr, 1);
  var pos = (w & (cap - 1)) * ch, n = frames * ch;
  var s = HEAPF32.subarray(src >> 2, (src >> 2) + n);
  var first = Math.min(n, cap * ch - pos);
  r.buf.set(s.subarray(0, first), pos);
  if (first < n) r.buf.set(s.subarray(first), 0);
  Atomics.store(r.hdr, 1, (w + frames) | 0);
})

EMSCRIPTEN_KEEPALIVE void iupwasmAudioFill(void)
{
  IwasmAudioDevice* device = &iwasm_audio_device;
  if (!iwasm_audio_ready || !device->running)
    return;

  while (iupwasmJsAudioFree() >= IWASM_AUDIO_PERIOD)
  {
    ma_device_handle_backend_data_callback(&device->device, device->buffer, NULL, IWASM_AUDIO_PERIOD);
    iupwasmJsAudioWrite(device->buffer, IWASM_AUDIO_PERIOD);
  }
}

static ma_result wasmAudioEnumerateDevices(ma_context* context, ma_enum_devices_callback_proc callback, void* user_data)
{
  ma_device_info info;
  memset(&info, 0, sizeof(info));
  strcpy(info.name, "Web Audio");
  info.isDefault = MA_TRUE;
  callback(context, ma_device_type_playback, &info, user_data);
  return MA_SUCCESS;
}

static ma_result wasmAudioGetDeviceInfo(ma_context* context, ma_device_type type, const ma_device_id* id, ma_device_info* info)
{
  (void)context; (void)id;
  if (type != ma_device_type_playback)
    return MA_NO_DEVICE;

  strcpy(info->name, "Web Audio");
  info->isDefault = MA_TRUE;
  info->nativeDataFormatCount = 1;
  info->nativeDataFormats[0].format = ma_format_f32;
  info->nativeDataFormats[0].channels = 2;
  info->nativeDataFormats[0].sampleRate = 0;
  info->nativeDataFormats[0].flags = 0;
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceInit(ma_device* base, const ma_device_config* config, ma_device_descriptor* playback, ma_device_descriptor* capture)
{
  IwasmAudioDevice* device = (IwasmAudioDevice*)base;
  int rate;
  (void)capture;

  if (config->deviceType != ma_device_type_playback)
    return MA_DEVICE_TYPE_NOT_SUPPORTED;

  rate = iupwasmJsAudioOpen();
  if (rate <= 0)
    return MA_FAILED_TO_OPEN_BACKEND_DEVICE;

  device->buffer = (float*)malloc(IWASM_AUDIO_PERIOD * 2 * sizeof(float));
  if (!device->buffer)
    return MA_OUT_OF_MEMORY;

  iupwasmJsAudioRingInit(IWASM_AUDIO_CAPACITY, 2, rate);

  playback->format = ma_format_f32;
  playback->channels = 2;
  playback->sampleRate = (ma_uint32)rate;
  playback->periodSizeInFrames = IWASM_AUDIO_PERIOD;
  playback->periodCount = 1;
  ma_channel_map_init_standard(ma_standard_channel_map_default, playback->channelMap, sizeof(playback->channelMap) / sizeof(playback->channelMap[0]), playback->channels);
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceUninit(ma_device* base)
{
  IwasmAudioDevice* device = (IwasmAudioDevice*)base;
  iupwasmJsAudioRingRelease();
  free(device->buffer);
  device->buffer = NULL;
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceStart(ma_device* base)
{
  IwasmAudioDevice* device = (IwasmAudioDevice*)base;
  device->running = 1;
  iupwasmJsAudioStart();
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceStop(ma_device* base)
{
  IwasmAudioDevice* device = (IwasmAudioDevice*)base;
  device->running = 0;
  iupwasmJsAudioStop();
  return MA_SUCCESS;
}

static ma_result wasmAudioContextUninit(ma_context* context)
{
  (void)context;
  return MA_SUCCESS;
}

static ma_result wasmAudioContextInit(ma_context* context, const ma_context_config* config, ma_backend_callbacks* callbacks)
{
  (void)context; (void)config;
  callbacks->onContextUninit = wasmAudioContextUninit;
  callbacks->onContextEnumerateDevices = wasmAudioEnumerateDevices;
  callbacks->onContextGetDeviceInfo = wasmAudioGetDeviceInfo;
  callbacks->onDeviceInit = wasmAudioDeviceInit;
  callbacks->onDeviceUninit = wasmAudioDeviceUninit;
  callbacks->onDeviceStart = wasmAudioDeviceStart;
  callbacks->onDeviceStop = wasmAudioDeviceStop;
  return MA_SUCCESS;
}

static void wasmAudioEngineData(ma_device* device, void* output, const void* input, ma_uint32 frames)
{
  (void)input;
  ma_engine_read_pcm_frames((ma_engine*)device->pUserData, output, frames, NULL);
}

ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  ma_backend backends[] = { ma_backend_custom };
  ma_context_config context_config = ma_context_config_init();
  ma_device_config device_config;

  context_config.custom.onContextInit = wasmAudioContextInit;
  if (ma_context_init(backends, 1, &context_config, &iwasm_audio_context) != MA_SUCCESS)
    return NULL;

  device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_f32;
  device_config.playback.channels = 2;
  device_config.dataCallback = wasmAudioEngineData;
  device_config.pUserData = engine;
  if (ma_device_init(&iwasm_audio_context, &device_config, &iwasm_audio_device.device) != MA_SUCCESS)
  {
    ma_context_uninit(&iwasm_audio_context);
    return NULL;
  }

  iwasm_audio_ready = 1;
  return &iwasm_audio_device.device;
}

void iupdrvAudioDeviceRelease(void)
{
  if (!iwasm_audio_ready)
    return;

  ma_device_uninit(&iwasm_audio_device.device);
  ma_context_uninit(&iwasm_audio_context);
  iwasm_audio_ready = 0;
}
