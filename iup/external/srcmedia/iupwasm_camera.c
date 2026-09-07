/** \file
 * \brief Camera control, getUserMedia on the main thread with frames over a shared buffer
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
#include "iup_camera.h"

#include "iupwasm_drv.h"

EM_JS(int, iupwasmJsCameraAvailable, (void), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'camavailable' });
  return globalThis.__iupCameraAvailable();
})

EM_JS(char*, iupwasmJsCameraDevices, (void), {
  if (typeof document !== 'undefined') return 0;
  var s = globalThis.__iupReadSync({ op: 'camdevices' });
  if (!s) return 0;
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(char*, iupwasmJsCameraPermission, (void), {
  if (typeof document !== 'undefined') return 0;
  var s = globalThis.__iupReadSync({ op: 'campermission' });
  if (!s) return 0;
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(void, iupwasmJsCameraStart, (int ihptr, int device, int width, int height, int fps), {
  var sab = new SharedArrayBuffer(16 + width * height * 3);
  if (!globalThis.__iupCamera) globalThis.__iupCamera = {};
  globalThis.__iupCamera[ihptr] = { hdr: new Int32Array(sab, 0, 4), u8: new Uint8Array(sab, 16) };
  globalThis.__iupApply({ op: 'camstart', ihptr: ihptr, device: device, w: width, h: height, fps: fps, sab: sab });
})

EM_JS(void, iupwasmJsCameraStop, (int ihptr), {
  globalThis.__iupApply({ op: 'camstop', ihptr: ihptr });
  if (globalThis.__iupCamera) delete globalThis.__iupCamera[ihptr];
})

EM_JS(int, iupwasmJsCameraTake, (int ihptr, int* width, int* height), {
  var cam = globalThis.__iupCamera && globalThis.__iupCamera[ihptr];
  if (!cam || Atomics.compareExchange(cam.hdr, 0, 2, 3) !== 2) return 0;
  HEAP32[width >> 2] = cam.hdr[1];
  HEAP32[height >> 2] = cam.hdr[2];
  return 1;
})

EM_JS(void, iupwasmJsCameraCopy, (int ihptr, unsigned char* dst, int size), {
  var cam = globalThis.__iupCamera[ihptr];
  HEAPU8.set(cam.u8.subarray(0, size), dst);
  Atomics.store(cam.hdr, 0, 0);
})

EM_JS(void, iupwasmJsCameraDrop, (int ihptr), {
  var cam = globalThis.__iupCamera && globalThis.__iupCamera[ihptr];
  if (cam) Atomics.store(cam.hdr, 0, 0);
})

EMSCRIPTEN_KEEPALIVE void iupwasmCameraFrame(int ihptr)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  unsigned char* buffer;
  int size, width, height;

  if (!ih || !iupObjectCheck(ih) || !iupAttribGet(ih, "_IUPWASM_CAMERA_RUNNING"))
    return;

  if (!iupwasmJsCameraTake(ihptr, &width, &height))
    return;

  size = width * height * 3;

  buffer = (unsigned char*)iupAttribGet(ih, "_IUPWASM_CAMERA_BUFFER");
  if (iupAttribGetInt(ih, "_IUPWASM_CAMERA_BUFFER_SIZE") != size)
  {
    buffer = (unsigned char*)realloc(buffer, size);
    if (!buffer)
    {
      iupwasmJsCameraDrop(ihptr);
      return;
    }
    iupAttribSet(ih, "_IUPWASM_CAMERA_BUFFER", (char*)buffer);
    iupAttribSetInt(ih, "_IUPWASM_CAMERA_BUFFER_SIZE", size);
  }

  iupwasmJsCameraCopy(ihptr, buffer, size);
  iupCameraFrame(ih, buffer, width, height);
}

EMSCRIPTEN_KEEPALIVE void iupwasmCameraPermission(int ihptr, int granted)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  if (!ih || !iupObjectCheck(ih))
    return;
  iupAttribSet(ih, "_IUPWASM_CAMERA_PERMISSION", granted ? "GRANTED" : "DENIED");
  iupCameraPermission(ih, granted);
}

EMSCRIPTEN_KEEPALIVE void iupwasmCameraError(int ihptr, int code)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  if (!ih || !iupObjectCheck(ih))
    return;
  iupCameraError(ih, code == 1 ? "Camera access denied" : code == 2 ? "Camera not found" : code == 3 ? "Camera in use" : "Camera error");
}

int iupdrvCameraIsAvailable(void)
{
  return iupwasmJsCameraAvailable();
}

static char* wasmCameraDeviceName(int index, int* count)
{
  char* devices = iupwasmJsCameraDevices();
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

int iupdrvCameraGetDeviceCount(void)
{
  int count;
  wasmCameraDeviceName(-1, &count);
  return count;
}

char* iupdrvCameraGetDeviceName(int index)
{
  int count;
  if (index < 0)
    return NULL;
  return wasmCameraDeviceName(index, &count);
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  char* permission = iupAttribGet(ih, "_IUPWASM_CAMERA_PERMISSION");
  char* state;

  if (!iupwasmJsCameraAvailable())
    return "UNAVAILABLE";
  if (permission)
    return permission;

  state = iupwasmJsCameraPermission();
  if (!state)
    return "PROMPT";
  permission = iupStrReturnStr(state);
  free(state);
  return permission;
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  if (!iupwasmJsCameraAvailable())
  {
    iupCameraError(ih, "Camera not supported");
    return 0;
  }

  iupAttribSet(ih, "_IUPWASM_CAMERA_RUNNING", "1");
  iupwasmJsCameraStart((int)(intptr_t)ih, device, *width, *height, *fps);
  return 1;
}

void iupdrvCameraStop(Ihandle* ih)
{
  unsigned char* buffer = (unsigned char*)iupAttribGet(ih, "_IUPWASM_CAMERA_BUFFER");

  iupAttribSet(ih, "_IUPWASM_CAMERA_RUNNING", NULL);
  iupwasmJsCameraStop((int)(intptr_t)ih);

  free(buffer);
  iupAttribSet(ih, "_IUPWASM_CAMERA_BUFFER", NULL);
  iupAttribSet(ih, "_IUPWASM_CAMERA_BUFFER_SIZE", NULL);
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}
