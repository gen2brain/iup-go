/** \file
 * \brief Media controls
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_MEDIA_H
#define __IUP_MEDIA_H

#include "iup_class.h"

#ifdef __cplusplus
extern "C" {
#endif

Iclass* iupAudioNewClass(void);
Iclass* iupCameraNewClass(void);
Iclass* iupMicrophoneNewClass(void);

int iupandroidMediaPermissionState(const char* permission);
void iupandroidMediaRequestPermission(const char* permission, Ihandle* ih);

#ifdef __cplusplus
}
#endif

#endif
