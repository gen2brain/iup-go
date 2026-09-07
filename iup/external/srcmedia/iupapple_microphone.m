/** \file
 * \brief Microphone control, AVFoundation permission over the miniaudio device
 *
 * See Copyright Notice in "iup.h"
 */

#import <AVFoundation/AVFoundation.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_microphone.h"

char* iupdrvMicrophoneGetPermission(Ihandle* ih)
{
  (void)ih;
  switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio])
  {
  case AVAuthorizationStatusAuthorized: return "GRANTED";
  case AVAuthorizationStatusNotDetermined: return "PROMPT";
  default: return "DENIED";
  }
}

int iupdrvMicrophoneStart(Ihandle* ih, int device, int* channels, int* samplerate)
{
  AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];

  if (status == AVAuthorizationStatusNotDetermined)
  {
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
      dispatch_async(dispatch_get_main_queue(), ^{
        if (!iupObjectCheck(ih))
          return;
        iupMicrophonePermission(ih, granted ? 1 : 0);
        if (iupAttribGet(ih, "_IUP_MICROPHONE_PENDING"))
        {
          iupAttribSet(ih, "_IUP_MICROPHONE_PENDING", NULL);
          if (granted)
            iupMicrophoneDeviceStart(ih, device, channels, samplerate);
          else
            iupMicrophoneError(ih, "Microphone access denied");
        }
      });
    }];
    iupAttribSet(ih, "_IUP_MICROPHONE_PENDING", "1");
    return 1;
  }

  if (status != AVAuthorizationStatusAuthorized)
  {
    iupMicrophoneError(ih, "Microphone access denied");
    return 0;
  }

  return iupMicrophoneDeviceStart(ih, device, channels, samplerate);
}

void iupdrvMicrophoneStop(Ihandle* ih)
{
  iupAttribSet(ih, "_IUP_MICROPHONE_PENDING", NULL);
  iupMicrophoneDeviceStop(ih);
}
