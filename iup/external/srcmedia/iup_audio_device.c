/** \file
 * \brief Audio device, platforms where miniaudio has its own backend
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup_miniaudio.h"

static ma_context iaudio_context;
static int iaudio_context_ready = 0;

ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  (void)engine;
  return NULL;
}

void iupdrvAudioDeviceRelease(void)
{
  if (iaudio_context_ready)
  {
    ma_context_uninit(&iaudio_context);
    iaudio_context_ready = 0;
  }
}

ma_context* iupdrvAudioContext(void)
{
  if (!iaudio_context_ready)
  {
    if (ma_context_init(NULL, 0, NULL, &iaudio_context) != MA_SUCCESS)
      return NULL;
    iaudio_context_ready = 1;
  }
  return &iaudio_context;
}

int iupdrvAudioDeviceList(ma_device_info** devices)
{
  ma_context* context = iupdrvAudioContext();
  ma_device_info* infos;
  ma_uint32 count;

  *devices = NULL;
  if (!context || ma_context_get_devices(context, &infos, &count, NULL, NULL) != MA_SUCCESS || count == 0)
    return 0;

  *devices = (ma_device_info*)malloc(count * sizeof(ma_device_info));
  if (!*devices)
    return 0;

  memcpy(*devices, infos, count * sizeof(ma_device_info));
  return (int)count;
}
