/** \file
 * \brief Microphone control
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_MICROPHONE_H
#define __IUP_MICROPHONE_H

#include "iup_class.h"

#ifdef __cplusplus
extern "C" {
#endif

int iupdrvMicrophoneIsAvailable(void);
int iupdrvMicrophoneGetDeviceCount(void);
char* iupdrvMicrophoneGetDeviceName(int index);
int iupdrvMicrophoneStart(Ihandle* ih, int device, int* channels, int* samplerate);
void iupdrvMicrophoneStop(Ihandle* ih);
char* iupdrvMicrophoneGetPermission(Ihandle* ih);
void iupdrvMicrophoneInitClass(Iclass* ic);

int iupMicrophoneDeviceStart(Ihandle* ih, int device, int* channels, int* samplerate);
void iupMicrophoneDeviceStop(Ihandle* ih);

void iupMicrophoneSamples(Ihandle* ih, const short* samples, int frames);
void iupMicrophoneError(Ihandle* ih, const char* message);
void iupMicrophonePermission(Ihandle* ih, int granted);

void iupandroidMicrophonePermission(Ihandle* ih, int granted);

#ifdef __cplusplus
}
#endif

#endif
