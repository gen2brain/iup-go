/** \file
 * \brief Microphone capture device on miniaudio
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_miniaudio.h"
#include "iup_microphone.h"

typedef struct _ImicDevice
{
  ma_device device;
  Ihandle* ih;
  int stopping;
} ImicDevice;

static ma_context imic_context;
static int imic_context_ready = 0;
static ma_device_info* imic_devices = NULL;
static int imic_default_synthetic = 0;

static int iMicContextInit(void)
{
  if (imic_context_ready)
    return 1;
  if (ma_context_init(NULL, 0, NULL, &imic_context) != MA_SUCCESS)
    return 0;
  imic_context_ready = 1;
  return 1;
}

static void iMicContextRelease(Iclass* ic)
{
  (void)ic;
  free(imic_devices);
  imic_devices = NULL;
  if (imic_context_ready)
  {
    ma_context_uninit(&imic_context);
    imic_context_ready = 0;
  }
}

static ma_device_info* iMicDeviceInfos(ma_uint32* count)
{
  ma_device_info* infos = NULL;
  ma_device_info* list;
  ma_uint32 n = 0, i, k = 1;
  int has_default = 0;

  *count = 0;
  if (!iMicContextInit())
    return NULL;
  if (ma_context_get_devices(&imic_context, NULL, NULL, &infos, &n) != MA_SUCCESS)
    return NULL;

  for (i = 0; i < n; i++)
    if (infos[i].isDefault) has_default = 1;

  if (n == 0)
    return NULL;

  list = (ma_device_info*)realloc(imic_devices, (n + 1) * sizeof(ma_device_info));
  if (!list)
    return NULL;
  imic_devices = list;
  imic_default_synthetic = !has_default;

  if (has_default)
  {
    for (i = 0; i < n; i++)
    {
      if (infos[i].isDefault && has_default == 1)
      {
        list[0] = infos[i];
        has_default = 2;
      }
      else
        list[k++] = infos[i];
    }
  }
  else
  {
    memset(&list[0], 0, sizeof(ma_device_info));
    strcpy(list[0].name, "default");
    list[0].isDefault = MA_TRUE;
    for (i = 0; i < n; i++)
      list[k++] = infos[i];
  }

  *count = k;
  return list;
}

int iupdrvMicrophoneIsAvailable(void)
{
  return iMicContextInit();
}

int iupdrvMicrophoneGetDeviceCount(void)
{
  ma_uint32 count;
  iMicDeviceInfos(&count);
  return (int)count;
}

char* iupdrvMicrophoneGetDeviceName(int index)
{
  ma_uint32 count;
  ma_device_info* infos = iMicDeviceInfos(&count);
  if (!infos || index < 0 || (ma_uint32)index >= count)
    return NULL;
  return iupStrReturnStr(infos[index].name);
}

static void iMicDeviceData(ma_device* device, void* output, const void* input, ma_uint32 frames)
{
  ImicDevice* mic = (ImicDevice*)device->pUserData;
  (void)output;
  iupMicrophoneSamples(mic->ih, (const short*)input, (int)frames);
}

static void iMicDeviceNotification(const ma_device_notification* notification)
{
  ImicDevice* mic = (ImicDevice*)notification->pDevice->pUserData;
  if (notification->type == ma_device_notification_type_stopped && !mic->stopping)
    iupMicrophoneError(mic->ih, "Capture stopped");
}

int iupMicrophoneDeviceStart(Ihandle* ih, int device, int* channels, int* samplerate)
{
  ma_uint32 count;
  ma_device_info* infos = iMicDeviceInfos(&count);
  ma_device_config config;
  ImicDevice* mic;
  ma_result result;

  if (!imic_context_ready)
  {
    iupMicrophoneError(ih, "Audio not available");
    return 0;
  }

  if (!infos || (ma_uint32)device >= count)
  {
    iupMicrophoneError(ih, "Microphone not found");
    return 0;
  }

  mic = (ImicDevice*)calloc(1, sizeof(ImicDevice));
  if (!mic)
    return 0;
  mic->ih = ih;

  config = ma_device_config_init(ma_device_type_capture);
  if (!(device == 0 && imic_default_synthetic))
    config.capture.pDeviceID = &infos[device].id;
  config.capture.format = ma_format_s16;
  config.capture.channels = (ma_uint32)*channels;
  config.sampleRate = (ma_uint32)*samplerate;
  config.dataCallback = iMicDeviceData;
  config.notificationCallback = iMicDeviceNotification;
  config.pUserData = mic;

  result = ma_device_init(&imic_context, &config, &mic->device);
  if (result == MA_SUCCESS)
  {
    result = ma_device_start(&mic->device);
    if (result != MA_SUCCESS)
      ma_device_uninit(&mic->device);
  }

  if (result != MA_SUCCESS)
  {
    free(mic);
    iupMicrophoneError(ih, ma_result_description(result));
    return 0;
  }

  iupAttribSet(ih, "_IUP_MICROPHONE", (char*)mic);
  return 1;
}

void iupMicrophoneDeviceStop(Ihandle* ih)
{
  ImicDevice* mic = (ImicDevice*)iupAttribGet(ih, "_IUP_MICROPHONE");
  if (!mic)
    return;

  mic->stopping = 1;
  ma_device_uninit(&mic->device);
  free(mic);
  iupAttribSet(ih, "_IUP_MICROPHONE", NULL);
}

void iupdrvMicrophoneInitClass(Iclass* ic)
{
  ic->Release = iMicContextRelease;
}

#if !defined(__APPLE__) && !defined(__ANDROID__)

char* iupdrvMicrophoneGetPermission(Ihandle* ih)
{
  (void)ih;
  return iupdrvMicrophoneGetDeviceCount() > 0 ? "GRANTED" : "UNAVAILABLE";
}

int iupdrvMicrophoneStart(Ihandle* ih, int device, int* channels, int* samplerate)
{
  return iupMicrophoneDeviceStart(ih, device, channels, samplerate);
}

void iupdrvMicrophoneStop(Ihandle* ih)
{
  iupMicrophoneDeviceStop(ih);
}

#endif
