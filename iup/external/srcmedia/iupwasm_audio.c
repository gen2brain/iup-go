/** \file
 * \brief Audio device, miniaudio backend feeding a Web Audio worklet through a shared ring per output
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "iup.h"
#include "iup_str.h"
#include "iup_miniaudio.h"

#define IWASM_AUDIO_CAPACITY 8192
#define IWASM_AUDIO_PERIOD 512
#define IWASM_AUDIO_SLOTS 8

typedef struct _IwasmAudioSlot
{
  ma_device* device;
  float* buffer;
  int running;
} IwasmAudioSlot;

static ma_context iwasm_audio_context;
static int iwasm_audio_context_ready = 0;
static ma_device iwasm_audio_device;
static int iwasm_audio_ready = 0;
static IwasmAudioSlot iwasm_audio_slots[IWASM_AUDIO_SLOTS];

EM_JS(int, iupwasmJsAudioOpen, (int slot, const char* sink), {
  var s = UTF8ToString(sink);
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'audioopen', slot: slot, sink: s });
  return globalThis.__iupAudioOpen(slot, s);
})

EM_JS(void, iupwasmJsAudioClose, (int slot), { globalThis.__iupApply({ op: 'audioclose', slot: slot }); })

EM_JS(void, iupwasmJsAudioRingInit, (int slot, int capacity, int channels, int rate), {
  var sab = new SharedArrayBuffer(32 + capacity * channels * 4);
  var hdr = new Int32Array(sab, 0, 8);
  hdr[2] = capacity; hdr[3] = channels; hdr[4] = rate;
  var rings = globalThis.__iupAudioRings = globalThis.__iupAudioRings || {};
  rings[slot] = { sab: sab, hdr: hdr, buf: new Float32Array(sab, 32) };
})

EM_JS(void, iupwasmJsAudioRingRelease, (int slot), {
  if (globalThis.__iupAudioRings) delete globalThis.__iupAudioRings[slot];
})

EM_JS(void, iupwasmJsAudioStart, (int slot), {
  globalThis.__iupApply({ op: 'audiostart', slot: slot, sab: globalThis.__iupAudioRings[slot].sab });
})

EM_JS(void, iupwasmJsAudioStop, (int slot), { globalThis.__iupApply({ op: 'audiostop', slot: slot }); })

EM_JS(int, iupwasmJsAudioFree, (int slot), {
  var r = globalThis.__iupAudioRings && globalThis.__iupAudioRings[slot];
  if (!r) return 0;
  return r.hdr[2] - ((Atomics.load(r.hdr, 1) - Atomics.load(r.hdr, 0)) | 0);
})

EM_JS(void, iupwasmJsAudioWrite, (int slot, const float* src, int frames), {
  var r = globalThis.__iupAudioRings && globalThis.__iupAudioRings[slot];
  if (!r) return;
  var ch = r.hdr[3], cap = r.hdr[2], w = Atomics.load(r.hdr, 1);
  var pos = (w & (cap - 1)) * ch, n = frames * ch;
  var s = HEAPF32.subarray(src >> 2, (src >> 2) + n);
  var first = Math.min(n, cap * ch - pos);
  r.buf.set(s.subarray(0, first), pos);
  if (first < n) r.buf.set(s.subarray(first), 0);
  Atomics.store(r.hdr, 1, (w + frames) | 0);
})

EM_JS(char*, iupwasmJsAudioDevices, (void), {
  if (typeof document !== 'undefined') return 0;
  var s = globalThis.__iupReadSync({ op: 'auddevices' });
  if (!s) return 0;
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

static int wasmAudioSlot(ma_device* device)
{
  int slot;
  for (slot = 0; slot < IWASM_AUDIO_SLOTS; slot++)
  {
    if (iwasm_audio_slots[slot].device == device)
      return slot;
  }
  return -1;
}

EMSCRIPTEN_KEEPALIVE void iupwasmAudioFill(int slot)
{
  IwasmAudioSlot* audio;
  if (slot < 0 || slot >= IWASM_AUDIO_SLOTS)
    return;

  audio = &iwasm_audio_slots[slot];
  if (!audio->device || !audio->running)
    return;

  while (iupwasmJsAudioFree(slot) >= IWASM_AUDIO_PERIOD)
  {
    ma_device_handle_backend_data_callback(audio->device, audio->buffer, NULL, IWASM_AUDIO_PERIOD);
    iupwasmJsAudioWrite(slot, audio->buffer, IWASM_AUDIO_PERIOD);
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

static ma_result wasmAudioDeviceInit(ma_device* device, const ma_device_config* config, ma_device_descriptor* playback, ma_device_descriptor* capture)
{
  const char* sink = playback->pDeviceID ? playback->pDeviceID->custom.s : "";
  int slot = wasmAudioSlot(NULL);
  int rate;
  (void)capture;

  if (config->deviceType != ma_device_type_playback)
    return MA_DEVICE_TYPE_NOT_SUPPORTED;
  if (slot < 0)
    return MA_TOO_MANY_OPEN_FILES;

  rate = iupwasmJsAudioOpen(slot, sink);
  if (rate <= 0)
    return MA_FAILED_TO_OPEN_BACKEND_DEVICE;

  iwasm_audio_slots[slot].buffer = (float*)malloc(IWASM_AUDIO_PERIOD * 2 * sizeof(float));
  if (!iwasm_audio_slots[slot].buffer)
  {
    iupwasmJsAudioClose(slot);
    return MA_OUT_OF_MEMORY;
  }

  iwasm_audio_slots[slot].device = device;
  iupwasmJsAudioRingInit(slot, IWASM_AUDIO_CAPACITY, 2, rate);

  playback->format = ma_format_f32;
  playback->channels = 2;
  playback->sampleRate = (ma_uint32)rate;
  playback->periodSizeInFrames = IWASM_AUDIO_PERIOD;
  playback->periodCount = 1;
  ma_channel_map_init_standard(ma_standard_channel_map_default, playback->channelMap, sizeof(playback->channelMap) / sizeof(playback->channelMap[0]), playback->channels);
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceUninit(ma_device* device)
{
  int slot = wasmAudioSlot(device);
  if (slot < 0)
    return MA_SUCCESS;

  iupwasmJsAudioRingRelease(slot);
  iupwasmJsAudioClose(slot);
  free(iwasm_audio_slots[slot].buffer);
  memset(&iwasm_audio_slots[slot], 0, sizeof(IwasmAudioSlot));
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceStart(ma_device* device)
{
  int slot = wasmAudioSlot(device);
  if (slot < 0)
    return MA_INVALID_OPERATION;

  iwasm_audio_slots[slot].running = 1;
  iupwasmJsAudioStart(slot);
  return MA_SUCCESS;
}

static ma_result wasmAudioDeviceStop(ma_device* device)
{
  int slot = wasmAudioSlot(device);
  if (slot < 0)
    return MA_SUCCESS;

  iwasm_audio_slots[slot].running = 0;
  iupwasmJsAudioStop(slot);
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

ma_context* iupdrvAudioContext(void)
{
  if (!iwasm_audio_context_ready)
  {
    ma_backend backends[] = { ma_backend_custom };
    ma_context_config context_config = ma_context_config_init();
    context_config.custom.onContextInit = wasmAudioContextInit;
    if (ma_context_init(backends, 1, &context_config, &iwasm_audio_context) != MA_SUCCESS)
      return NULL;
    iwasm_audio_context_ready = 1;
  }
  return &iwasm_audio_context;
}

ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  ma_context* context = iupdrvAudioContext();
  ma_device_config device_config;

  if (!context)
    return NULL;

  device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_f32;
  device_config.playback.channels = 2;
  device_config.dataCallback = wasmAudioEngineData;
  device_config.pUserData = engine;
  if (ma_device_init(context, &device_config, &iwasm_audio_device) != MA_SUCCESS)
    return NULL;

  iwasm_audio_ready = 1;
  return &iwasm_audio_device;
}

void iupdrvAudioDeviceRelease(void)
{
  if (iwasm_audio_ready)
  {
    ma_device_uninit(&iwasm_audio_device);
    iwasm_audio_ready = 0;
  }

  if (iwasm_audio_context_ready)
  {
    ma_context_uninit(&iwasm_audio_context);
    iwasm_audio_context_ready = 0;
  }
}

int iupdrvAudioDeviceList(ma_device_info** devices)
{
  char* list = iupwasmJsAudioDevices();
  char* line;
  int count = 0, total = 1;

  *devices = NULL;
  if (!list)
    return 0;

  for (line = list; *line; line++)
  {
    if (*line == '\n')
      total++;
  }

  *devices = (ma_device_info*)calloc((size_t)total, sizeof(ma_device_info));
  if (*devices)
  {
    for (line = strtok(list, "\n"); line && count < total; line = strtok(NULL, "\n"))
    {
      char* name = strchr(line, '\t');
      if (!name)
        continue;
      *name++ = 0;
      iupStrCopyN((*devices)[count].id.custom.s, (int)sizeof((*devices)[count].id.custom.s), line);
      iupStrCopyN((*devices)[count].name, (int)sizeof((*devices)[count].name), name);
      count++;
    }

    if (count == 0)
    {
      free(*devices);
      *devices = NULL;
    }
  }

  free(list);
  return count;
}
