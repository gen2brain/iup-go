/** \file
 * \brief Camera control, platforms without capture support
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iup_camera.h"

int iupdrvCameraIsAvailable(void)
{
  return 0;
}

int iupdrvCameraGetDeviceCount(void)
{
  return 0;
}

char* iupdrvCameraGetDeviceName(int index)
{
  (void)index;
  return NULL;
}

char* iupdrvCameraGetPermission(Ihandle* ih)
{
  (void)ih;
  return "UNAVAILABLE";
}

int iupdrvCameraStart(Ihandle* ih, int device, int* width, int* height, int* fps)
{
  (void)device; (void)width; (void)height; (void)fps;
  iupCameraError(ih, "Camera not supported");
  return 0;
}

void iupdrvCameraStop(Ihandle* ih)
{
  (void)ih;
}

void iupdrvCameraInitClass(Iclass* ic)
{
  (void)ic;
}
