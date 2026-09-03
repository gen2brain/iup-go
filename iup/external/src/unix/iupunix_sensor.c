/** \file
 * \brief Sensor (no system sensor service)
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_sensor.h"

IUP_SDK_API int iupdrvSensorIsAvailable(int type) { (void)type; return 0; }

IUP_SDK_API int iupdrvSensorStart(Ihandle* ih)
{
  IsensorMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), "Sensor not available");
  iupSensorPost(ih, &msg);
  return 0;
}

IUP_SDK_API void iupdrvSensorStop(Ihandle* ih) { (void)ih; }

IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih)
{
  (void)ih;
  return "UNAVAILABLE";
}

IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih) { (void)ih; }

IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic) { (void)ic; }
