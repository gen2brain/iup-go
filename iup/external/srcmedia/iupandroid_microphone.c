/** \file
 * \brief Microphone control, runtime permission over the miniaudio device
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_media.h"
#include "iup_microphone.h"

char* iupdrvMicrophoneGetPermission(Ihandle* ih)
{
  int state = iupandroidMediaPermissionState("android.permission.RECORD_AUDIO");
  (void)ih;
  return state == 1 ? "GRANTED" : state == 2 ? "DENIED" : state == 0 ? "PROMPT" : "UNAVAILABLE";
}

int iupdrvMicrophoneStart(Ihandle* ih, int device, int* channels, int* samplerate)
{
  int state = iupandroidMediaPermissionState("android.permission.RECORD_AUDIO");

  if (state == 1)
    return iupMicrophoneDeviceStart(ih, device, channels, samplerate);

  if (state != 0)
  {
    iupMicrophoneError(ih, "Microphone access denied");
    return 0;
  }

  iupandroidMediaRequestPermission("android.permission.RECORD_AUDIO", ih);
  iupAttribSetStrf(ih, "_IUP_MICROPHONE_PENDING", "%d %d %d", device, *channels, *samplerate);
  return 1;
}

void iupdrvMicrophoneStop(Ihandle* ih)
{
  iupAttribSet(ih, "_IUP_MICROPHONE_PENDING", NULL);
  iupMicrophoneDeviceStop(ih);
}

void iupandroidMicrophonePermission(Ihandle* ih, int granted)
{
  char* pending = iupAttribGet(ih, "_IUP_MICROPHONE_PENDING");
  int device = 0, channels = 1, samplerate = 44100;

  iupMicrophonePermission(ih, granted);
  if (!pending)
    return;

  sscanf(pending, "%d %d %d", &device, &channels, &samplerate);
  iupAttribSet(ih, "_IUP_MICROPHONE_PENDING", NULL);
  if (granted)
    iupMicrophoneDeviceStart(ih, device, &channels, &samplerate);
  else
    iupMicrophoneError(ih, "Microphone access denied");
}
