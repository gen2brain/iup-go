/** \file
 * \brief Sensor Control - Motion sensors
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_SENSOR_H
#define __IUP_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup drv
 * @{ */

enum { IUP_SENSOR_READING, IUP_SENSOR_PERMISSION, IUP_SENSOR_ERROR };

enum { IUP_SENSOR_ACCELEROMETER, IUP_SENSOR_GYROSCOPE, IUP_SENSOR_MAGNETOMETER, IUP_SENSOR_GRAVITY,
       IUP_SENSOR_LINEARACCELERATION, IUP_SENSOR_ORIENTATION, IUP_SENSOR_COMPASS, IUP_SENSOR_TYPE_COUNT };

typedef struct _IsensorMsg
{
  int type;
  double x, y, z;
  long long timestamp;
  int granted;
  char error[128];
} IsensorMsg;

/* Copies the message and posts it to the main loop, can be called from any thread. */
IUP_SDK_API void iupSensorPost(Ihandle* ih, const IsensorMsg* msg);
/* TYPE attribute as one of the IUP_SENSOR_* enum values. */
IUP_SDK_API int iupSensorGetType(Ihandle* ih);

IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic);
IUP_SDK_API int iupdrvSensorIsAvailable(int type);
IUP_SDK_API int iupdrvSensorStart(Ihandle* ih);
IUP_SDK_API void iupdrvSensorStop(Ihandle* ih);
IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih);
IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
