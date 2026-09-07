/** \file
 * \brief Audio device, platforms where miniaudio has its own backend
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup_miniaudio.h"

ma_device* iupdrvAudioDeviceInit(ma_engine* engine)
{
  (void)engine;
  return NULL;
}

void iupdrvAudioDeviceRelease(void)
{
}
