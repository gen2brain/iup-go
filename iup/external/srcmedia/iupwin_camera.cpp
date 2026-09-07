/** \file
 * \brief Camera control, Media Foundation capture
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_camera.h"

typedef HRESULT (WINAPI *MFStartupFunc)(ULONG, DWORD);
typedef HRESULT (WINAPI *MFCreateAttributesFunc)(IMFAttributes**, UINT32);
typedef HRESULT (WINAPI *MFCreateMediaTypeFunc)(IMFMediaType**);
typedef HRESULT (WINAPI *MFEnumDeviceSourcesFunc)(IMFAttributes*, IMFActivate***, UINT32*);
typedef HRESULT (WINAPI *MFCreateSourceReaderFromMediaSourceFunc)(IMFMediaSource*, IMFAttributes*, IMFSourceReader**);

static MFStartupFunc winMFStartup;
static MFCreateAttributesFunc winMFCreateAttributes;
static MFCreateMediaTypeFunc winMFCreateMediaType;
static MFEnumDeviceSourcesFunc winMFEnumDeviceSources;
static MFCreateSourceReaderFromMediaSourceFunc winMFCreateSourceReaderFromMediaSource;
static int win_camera_loaded = -1;

typedef struct _IwinCamera
{
  Ihandle* ih;
  IMFMediaSource* source;
  IMFSourceReader* reader;
  HANDLE thread;
  volatile LONG quit;
  int width, height;
  LONG stride;
  int nv12;
  unsigned char* rgb;
} IwinCamera;

static int winCameraLoad(void)
{
  HMODULE mfplat, mf, mfreadwrite;

  if (win_camera_loaded >= 0)
    return win_camera_loaded;
  win_camera_loaded = 0;

  mfplat = LoadLibraryW(L"mfplat.dll");
  mf = LoadLibraryW(L"mf.dll");
  mfreadwrite = LoadLibraryW(L"mfreadwrite.dll");
  if (!mfplat || !mf || !mfreadwrite)
    return 0;

  winMFStartup = (MFStartupFunc)GetProcAddress(mfplat, "MFStartup");
  winMFCreateAttributes = (MFCreateAttributesFunc)GetProcAddress(mfplat, "MFCreateAttributes");
  winMFCreateMediaType = (MFCreateMediaTypeFunc)GetProcAddress(mfplat, "MFCreateMediaType");
  winMFEnumDeviceSources = (MFEnumDeviceSourcesFunc)GetProcAddress(mf, "MFEnumDeviceSources");
  winMFCreateSourceReaderFromMediaSource = (MFCreateSourceReaderFromMediaSourceFunc)GetProcAddress(mfreadwrite, "MFCreateSourceReaderFromMediaSource");
  if (!winMFStartup || !winMFCreateAttributes || !winMFCreateMediaType || !winMFEnumDeviceSources || !winMFCreateSourceReaderFromMediaSource)
    return 0;

  if (FAILED(winMFStartup(MF_VERSION, MFSTARTUP_LITE)))
    return 0;

  win_camera_loaded = 1;
  return 1;
}

static IMFActivate** winCameraEnumerate(UINT32* count)
{
  IMFAttributes* attributes = NULL;
  IMFActivate** devices = NULL;

  *count = 0;
  if (!winCameraLoad())
    return NULL;

  if (FAILED(winMFCreateAttributes(&attributes, 1)))
    return NULL;
  attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
  if (FAILED(winMFEnumDeviceSources(attributes, &devices, count)))
    devices = NULL;
  attributes->Release();
  return devices;
}

static void winCameraFreeDevices(IMFActivate** devices, UINT32 count)
{
  UINT32 i;
  for (i = 0; i < count; i++)
    devices[i]->Release();
  CoTaskMemFree(devices);
}

int iupdrvCameraIsAvailable(void)
{
  return winCameraLoad();
}

int iupdrvCameraGetDeviceCount(void)
{
  UINT32 count;
  IMFActivate** devices = winCameraEnumerate(&count);
  if (devices)
    winCameraFreeDevices(devices, count);
  return (int)count;
}

char* iupdrvCameraGetDeviceName(int index)
{
  UINT32 count;
  IMFActivate** devices = winCameraEnumerate(&count);
  char* name = NULL;

  if (!devices)
    return NULL;

  if (index >= 0 && (UINT32)index < count)
  {
    WCHAR* wname = NULL;
    UINT32 length = 0;
    if (SUCCEEDED(devices[index]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &wname, &length)))
    {
      int size = WideCharToMultiByte(CP_UTF8, 0, wname, -1, NULL, 0, NULL, NULL);
      char* utf8 = iupStrGetMemory(size);
      WideCharToMultiByte(CP_UTF8, 0, wname, -1, utf8, size, NULL, NULL);
      name = utf8;
      CoTaskMemFree(wname);
    }
  }

  winCameraFreeDevices(devices, count);
  return name;
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  (void)ih;
  return (char*)(winCameraLoad() ? "GRANTED" : "UNAVAILABLE");
}

static void winCameraConsider(IMFMediaType* type, int req_width, int req_height, int req_fps, DWORD index, DWORD* best_index, long* best_score)
{
  UINT32 width = 0, height = 0, num = 0, den = 1;
  long score;
  int fps;

  if (FAILED(MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height)) || !width || !height)
    return;
  MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &num, &den);
  fps = den ? (int)(num / den) : 0;

  score = labs((long)width * (long)height - (long)req_width * (long)req_height) * 4;
  if (fps && fps < req_fps)
    score += (long)(req_fps - fps) * 100000;

  if (*best_score < 0 || score < *best_score)
  {
    *best_score = score;
    *best_index = index;
  }
}

static int winCameraChooseType(IMFSourceReader* reader, int req_width, int req_height, int req_fps, int* width, int* height, int* fps)
{
  IMFMediaType* type = NULL;
  DWORD index = 0, best_index = 0;
  long best_score = -1;
  UINT32 w = 0, h = 0, num = 0, den = 1;

  while (SUCCEEDED(reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, index, &type)))
  {
    winCameraConsider(type, req_width, req_height, req_fps, index, &best_index, &best_score);
    type->Release();
    index++;
  }
  if (best_score < 0)
    return 0;

  if (FAILED(reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, best_index, &type)))
    return 0;
  if (FAILED(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, type)))
  {
    type->Release();
    return 0;
  }
  MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &w, &h);
  MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &num, &den);
  type->Release();

  *width = (int)w;
  *height = (int)h;
  if (den && num)
    *fps = (int)(num / den);
  return 1;
}

static int winCameraSetOutput(IwinCamera* camera, const GUID& subtype)
{
  IMFMediaType* type = NULL;
  IMFMediaType* current = NULL;
  HRESULT hr;

  if (FAILED(winMFCreateMediaType(&type)))
    return 0;
  type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  type->SetGUID(MF_MT_SUBTYPE, subtype);
  MFSetAttributeSize(type, MF_MT_FRAME_SIZE, (UINT32)camera->width, (UINT32)camera->height);
  hr = camera->reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, type);
  type->Release();
  if (FAILED(hr))
    return 0;

  camera->stride = 0;
  if (SUCCEEDED(camera->reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &current)))
  {
    UINT32 w = 0, h = 0;
    INT32 stride = 0;
    if (SUCCEEDED(MFGetAttributeSize(current, MF_MT_FRAME_SIZE, &w, &h)) && w && h)
    {
      camera->width = (int)w;
      camera->height = (int)h;
    }
    if (SUCCEEDED(current->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&stride)))
      camera->stride = stride;
    current->Release();
  }
  return 1;
}

static unsigned char winCameraClamp(int value)
{
  return (unsigned char)(value < 0 ? 0 : value > 255 ? 255 : value);
}

static void winCameraConvert(IwinCamera* camera, const unsigned char* src, DWORD length)
{
  int width = camera->width, height = camera->height, x, y;
  unsigned char* rgb = camera->rgb;

  if (camera->nv12)
  {
    LONG stride = camera->stride > 0 ? camera->stride : width;
    const unsigned char* uv = src + stride * height;
    if ((DWORD)(stride * height * 3 / 2) > length)
      return;
    for (y = 0; y < height; y++)
    {
      const unsigned char* row = src + y * stride;
      const unsigned char* chroma = uv + (y / 2) * stride;
      for (x = 0; x < width; x++)
      {
        int c = row[x] - 16, d = chroma[x & ~1] - 128, e = chroma[(x & ~1) + 1] - 128;
        rgb[0] = winCameraClamp((298 * c + 409 * e + 128) >> 8);
        rgb[1] = winCameraClamp((298 * c - 100 * d - 208 * e + 128) >> 8);
        rgb[2] = winCameraClamp((298 * c + 516 * d + 128) >> 8);
        rgb += 3;
      }
    }
    return;
  }

  {
    LONG stride = camera->stride ? camera->stride : width * 4;
    const unsigned char* base = src;
    if (stride < 0)
      base = src + (LONG)(height - 1) * (-stride);
    if ((DWORD)(labs(stride) * height) > length)
      return;
    for (y = 0; y < height; y++)
    {
      const unsigned char* row = base + y * stride;
      for (x = 0; x < width; x++)
      {
        rgb[0] = row[2]; rgb[1] = row[1]; rgb[2] = row[0];
        row += 4;
        rgb += 3;
      }
    }
  }
}

static DWORD WINAPI winCameraThread(LPVOID arg)
{
  IwinCamera* camera = (IwinCamera*)arg;

  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  while (!camera->quit)
  {
    DWORD stream = 0, flags = 0;
    LONGLONG timestamp = 0;
    IMFSample* sample = NULL;
    HRESULT hr = camera->reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &stream, &flags, &timestamp, &sample);

    if (FAILED(hr))
    {
      iupCameraError(camera->ih, "Camera stopped");
      break;
    }
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
      iupCameraError(camera->ih, "Camera disconnected");
      break;
    }
    if (!sample)
      continue;

    IMFMediaBuffer* buffer = NULL;
    if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)))
    {
      BYTE* data = NULL;
      DWORD length = 0;
      if (SUCCEEDED(buffer->Lock(&data, NULL, &length)))
      {
        winCameraConvert(camera, data, length);
        buffer->Unlock();
        iupCameraFrame(camera->ih, camera->rgb, camera->width, camera->height);
      }
      buffer->Release();
    }
    sample->Release();
  }

  CoUninitialize();
  return 0;
}

static void winCameraRelease(IwinCamera* camera)
{
  if (camera->reader)
    camera->reader->Release();
  if (camera->source)
  {
    camera->source->Shutdown();
    camera->source->Release();
  }
  free(camera->rgb);
  free(camera);
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  UINT32 count;
  IMFActivate** devices;
  IMFAttributes* attributes = NULL;
  IwinCamera* camera;
  HRESULT hr;

  devices = winCameraEnumerate(&count);
  if (!devices || device < 0 || (UINT32)device >= count)
  {
    if (devices)
      winCameraFreeDevices(devices, count);
    iupCameraError(ih, "Camera not found");
    return 0;
  }

  camera = (IwinCamera*)calloc(1, sizeof(IwinCamera));
  camera->ih = ih;

  hr = devices[device]->ActivateObject(IID_IMFMediaSource, (void**)&camera->source);
  winCameraFreeDevices(devices, count);
  if (FAILED(hr))
  {
    winCameraRelease(camera);
    iupCameraError(ih, hr == E_ACCESSDENIED ? "Camera access denied" : "Cannot open camera");
    return 0;
  }

  if (FAILED(winMFCreateAttributes(&attributes, 1)))
  {
    winCameraRelease(camera);
    iupCameraError(ih, "Cannot open camera");
    return 0;
  }
  attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
  hr = winMFCreateSourceReaderFromMediaSource(camera->source, attributes, &camera->reader);
  attributes->Release();
  if (FAILED(hr))
  {
    winCameraRelease(camera);
    iupCameraError(ih, "Cannot open camera");
    return 0;
  }

  camera->reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
  camera->reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

  if (!winCameraChooseType(camera->reader, *width, *height, *fps, &camera->width, &camera->height, fps))
  {
    winCameraRelease(camera);
    iupCameraError(ih, "No supported video format");
    return 0;
  }

  if (winCameraSetOutput(camera, MFVideoFormat_RGB32))
    camera->nv12 = 0;
  else if (winCameraSetOutput(camera, MFVideoFormat_NV12))
    camera->nv12 = 1;
  else
  {
    winCameraRelease(camera);
    iupCameraError(ih, "No supported pixel format");
    return 0;
  }

  camera->rgb = (unsigned char*)malloc(camera->width * camera->height * 3);
  camera->thread = CreateThread(NULL, 0, winCameraThread, camera, 0, NULL);
  if (!camera->thread)
  {
    winCameraRelease(camera);
    iupCameraError(ih, "Cannot start capture thread");
    return 0;
  }

  *width = camera->width;
  *height = camera->height;
  iupAttribSet(ih, "_IUP_CAMERA", (char*)camera);
  return 1;
}

void iupdrvCameraStop(Ihandle* ih)
{
  IwinCamera* camera = (IwinCamera*)iupAttribGet(ih, "_IUP_CAMERA");
  if (!camera)
    return;

  InterlockedExchange(&camera->quit, 1);
  WaitForSingleObject(camera->thread, INFINITE);
  CloseHandle(camera->thread);
  winCameraRelease(camera);
  iupAttribSet(ih, "_IUP_CAMERA", NULL);
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}
