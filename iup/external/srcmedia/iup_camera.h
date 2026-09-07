/** \file
 * \brief Camera control
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_CAMERA_H
#define __IUP_CAMERA_H

#include "iup_class.h"

#ifdef __cplusplus
extern "C" {
#endif

int iupdrvCameraIsAvailable(void);
int iupdrvCameraGetDeviceCount(void);
char* iupdrvCameraGetDeviceName(int index);
int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps);
void iupdrvCameraStop(Ihandle* ih);
char* iupdrvCameraGetPermission(Ihandle* ih);
void iupdrvCameraInitClass(Iclass* ic);

void iupCameraFrame(Ihandle* ih, const unsigned char* rgb, int width, int height);
void iupCameraError(Ihandle* ih, const char* message);
void iupCameraPermission(Ihandle* ih, int granted);
void iupCameraRotate(const unsigned char* src, int width, int height, int angle, unsigned char* dst);

void iupandroidCameraPermission(Ihandle* ih, int granted);

#ifdef __cplusplus
}
#endif

#endif
