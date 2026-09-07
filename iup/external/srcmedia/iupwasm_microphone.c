/** \file
 * \brief Microphone control, getUserMedia on the main thread with samples over a shared ring
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_microphone.h"

#define IWASM_MIC_CAPACITY 32768

EM_JS(int, iupwasmJsMicAvailable, (void), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'micavailable' });
  return globalThis.__iupMicrophoneAvailable();
})

EM_JS(char*, iupwasmJsMicDevices, (void), {
  if (typeof document !== 'undefined') return 0;
  var s = globalThis.__iupReadSync({ op: 'micdevices' });
  if (!s) return 0;
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(char*, iupwasmJsMicPermission, (void), {
  if (typeof document !== 'undefined') return 0;
  var s = globalThis.__iupReadSync({ op: 'micpermission' });
  if (!s) return 0;
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(int, iupwasmJsMicOpen, (int ihptr, int rate), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'micopen', ihptr: ihptr, rate: rate });
  return globalThis.__iupMicrophoneOpen(ihptr, rate);
})

EM_JS(void, iupwasmJsMicStart, (int ihptr, int device, int channels, int capacity), {
  var sab = new SharedArrayBuffer(32 + capacity * channels * 2);
  var hdr = new Int32Array(sab, 0, 8);
  hdr[2] = capacity; hdr[3] = channels;
  if (!globalThis.__iupMic) globalThis.__iupMic = {};
  globalThis.__iupMic[ihptr] = { hdr: hdr, buf: new Int16Array(sab, 32) };
  globalThis.__iupApply({ op: 'micstart', ihptr: ihptr, device: device, sab: sab });
})

EM_JS(void, iupwasmJsMicStop, (int ihptr), {
  globalThis.__iupApply({ op: 'micstop', ihptr: ihptr });
  if (globalThis.__iupMic) delete globalThis.__iupMic[ihptr];
})

EM_JS(int, iupwasmJsMicRead, (int ihptr, short* dst, int max_frames), {
  var m = globalThis.__iupMic && globalThis.__iupMic[ihptr];
  if (!m) return 0;
  var ch = m.hdr[3], cap = m.hdr[2], r = Atomics.load(m.hdr, 0), w = Atomics.load(m.hdr, 1);
  var frames = Math.min((w - r) | 0, max_frames);
  if (frames <= 0) return 0;
  var pos = (r & (cap - 1)) * ch, n = frames * ch, first = Math.min(n, cap * ch - pos);
  var out = HEAP16.subarray(dst >> 1, (dst >> 1) + n);
  out.set(m.buf.subarray(pos, pos + first), 0);
  if (first < n) out.set(m.buf.subarray(0, n - first), first);
  Atomics.store(m.hdr, 0, (r + frames) | 0);
  return frames;
})

EMSCRIPTEN_KEEPALIVE void iupwasmMicrophoneSamples(int ihptr)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  short* buffer;
  int frames;

  if (!ih || !iupObjectCheck(ih))
    return;

  buffer = (short*)iupAttribGet(ih, "_IUPWASM_MIC_BUFFER");
  if (!buffer)
    return;

  while ((frames = iupwasmJsMicRead(ihptr, buffer, IWASM_MIC_CAPACITY / 4)) > 0)
    iupMicrophoneSamples(ih, buffer, frames);
}

EMSCRIPTEN_KEEPALIVE void iupwasmMicrophonePermission(int ihptr, int granted)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  if (!ih || !iupObjectCheck(ih))
    return;
  iupAttribSet(ih, "_IUPWASM_MIC_PERMISSION", granted ? "GRANTED" : "DENIED");
  iupMicrophonePermission(ih, granted);
}

EMSCRIPTEN_KEEPALIVE void iupwasmMicrophoneError(int ihptr, int code)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  if (!ih || !iupObjectCheck(ih))
    return;
  iupMicrophoneError(ih, code == 1 ? "Microphone access denied" : code == 2 ? "Microphone not found" : code == 3 ? "Microphone in use" : "Microphone error");
}

int iupdrvMicrophoneIsAvailable(void)
{
  return iupwasmJsMicAvailable();
}

static char* wasmMicDeviceName(int index, int* count)
{
  char* devices = iupwasmJsMicDevices();
  char* name = NULL;
  char* line;
  int i = 0;

  *count = 0;
  if (!devices)
    return NULL;

  for (line = strtok(devices, "\n"); line; line = strtok(NULL, "\n"), i++)
  {
    if (i == index)
      name = iupStrReturnStr(line);
  }
  *count = i;

  free(devices);
  return name;
}

int iupdrvMicrophoneGetDeviceCount(void)
{
  int count;
  wasmMicDeviceName(-1, &count);
  return count;
}

char* iupdrvMicrophoneGetDeviceName(int index)
{
  int count;
  if (index < 0)
    return NULL;
  return wasmMicDeviceName(index, &count);
}

char* iupdrvMicrophoneGetPermission(Ihandle* ih)
{
  char* permission = iupAttribGet(ih, "_IUPWASM_MIC_PERMISSION");
  char* state;

  if (!iupwasmJsMicAvailable())
    return "UNAVAILABLE";
  if (permission)
    return permission;

  state = iupwasmJsMicPermission();
  if (!state)
    return "PROMPT";
  permission = iupStrReturnStr(state);
  free(state);
  return permission;
}

int iupdrvMicrophoneStart(Ihandle* ih, int device, int* channels, int* samplerate)
{
  short* buffer;
  int rate;

  if (!iupwasmJsMicAvailable())
  {
    iupMicrophoneError(ih, "Microphone not supported");
    return 0;
  }

  rate = iupwasmJsMicOpen((int)(intptr_t)ih, *samplerate);
  if (rate <= 0)
  {
    iupMicrophoneError(ih, "Microphone not supported");
    return 0;
  }

  if (*channels > 2)
    *channels = 2;
  *samplerate = rate;

  buffer = (short*)malloc((IWASM_MIC_CAPACITY / 4) * (*channels) * sizeof(short));
  if (!buffer)
    return 0;

  iupAttribSet(ih, "_IUPWASM_MIC_BUFFER", (char*)buffer);
  iupwasmJsMicStart((int)(intptr_t)ih, device, *channels, IWASM_MIC_CAPACITY);
  return 1;
}

void iupdrvMicrophoneStop(Ihandle* ih)
{
  short* buffer = (short*)iupAttribGet(ih, "_IUPWASM_MIC_BUFFER");

  iupwasmJsMicStop((int)(intptr_t)ih);
  free(buffer);
  iupAttribSet(ih, "_IUPWASM_MIC_BUFFER", NULL);
}

void iupdrvMicrophoneInitClass(Iclass* ic)
{
  (void)ic;
}
