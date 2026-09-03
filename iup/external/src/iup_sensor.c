/** \file
 * \brief Sensor Control - Motion sensors
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_sensor.h"


static const char* iSensorTypeNames[IUP_SENSOR_TYPE_COUNT] = {
  "ACCELEROMETER", "GYROSCOPE", "MAGNETOMETER", "GRAVITY", "LINEARACCELERATION", "ORIENTATION", "COMPASS"
};

IUP_SDK_API int iupSensorGetType(Ihandle* ih)
{
  const char* value = iupAttribGetStr(ih, "TYPE");
  int i;
  for (i = 0; i < IUP_SENSOR_TYPE_COUNT; i++)
  {
    if (iupStrEqualNoCase(value, iSensorTypeNames[i]))
      return i;
  }
  return IUP_SENSOR_ACCELEROMETER;
}

static void iSensorReading(Ihandle* ih, const IsensorMsg* msg)
{
  IFnddd cb;

  iupAttribSetDouble(ih, "X", msg->x);
  iupAttribSetDouble(ih, "Y", msg->y);
  iupAttribSetDouble(ih, "Z", msg->z);
  iupAttribSetStrf(ih, "VALUE", "%g %g %g", msg->x, msg->y, msg->z);
  iupAttribSetStrf(ih, "TIMESTAMP", "%lld", msg->timestamp);

  cb = (IFnddd)IupGetCallback(ih, "SENSOR_CB");
  if (cb && cb(ih, msg->x, msg->y, msg->z) == IUP_CLOSE)
    IupExitLoop();
}

static int iSensorPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  IsensorMsg* msg = (IsensorMsg*)p;
  (void)s; (void)i; (void)d;

  if (!msg)
    return IUP_DEFAULT;

  if (msg->type == IUP_SENSOR_READING)
    iSensorReading(ih, msg);
  else if (msg->type == IUP_SENSOR_PERMISSION)
  {
    IFni cb = (IFni)IupGetCallback(ih, "PERMISSION_CB");
    if (cb && cb(ih, msg->granted) == IUP_CLOSE)
      IupExitLoop();
  }
  else if (msg->type == IUP_SENSOR_ERROR)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb && cb(ih, msg->error) == IUP_CLOSE)
      IupExitLoop();
  }

  free(msg);
  return IUP_DEFAULT;
}

IUP_SDK_API void iupSensorPost(Ihandle* ih, const IsensorMsg* msg)
{
  IsensorMsg* copy = (IsensorMsg*)malloc(sizeof(IsensorMsg));
  if (!copy)
    return;
  *copy = *msg;
  IupPostMessage(ih, NULL, 0, 0, copy);
}

static int iSensorSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!iupAttribGetBoolean(ih, "_IUP_SENSOR_ACTIVE") && iupdrvSensorStart(ih))
      iupAttribSet(ih, "_IUP_SENSOR_ACTIVE", "1");
  }
  else if (iupAttribGetBoolean(ih, "_IUP_SENSOR_ACTIVE"))
  {
    iupdrvSensorStop(ih);
    iupAttribSet(ih, "_IUP_SENSOR_ACTIVE", NULL);
  }
  return 0;
}

static char* iSensorGetActiveAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGetBoolean(ih, "_IUP_SENSOR_ACTIVE"));
}

static int iSensorSetTypeAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (iupAttribGetBoolean(ih, "_IUP_SENSOR_ACTIVE"))
    return 0;
  return 1;
}

static char* iSensorGetAvailableAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupdrvSensorIsAvailable(iupSensorGetType(ih)));
}

static char* iSensorGetPermissionAttrib(Ihandle* ih)
{
  return iupdrvSensorGetPermission(ih);
}

static int iSensorCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)iSensorPostMessage);
  return IUP_NOERROR;
}

static void iSensorDestroyMethod(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "_IUP_SENSOR_ACTIVE"))
    iupdrvSensorStop(ih);
  iupdrvSensorDestroy(ih);
}

/******************************************************************************/

IUP_API Ihandle* IupSensor(void)
{
  return IupCreate("sensor");
}

Iclass* iupSensorNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "sensor";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupSensorNewClass;
  ic->Create = iSensorCreateMethod;
  ic->Destroy = iSensorDestroyMethod;

  iupClassRegisterCallback(ic, "SENSOR_CB", "ddd");
  iupClassRegisterCallback(ic, "PERMISSION_CB", "i");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterAttribute(ic, "TYPE", NULL, iSensorSetTypeAttrib, IUPAF_SAMEASSYSTEM, "ACCELEROMETER", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iSensorGetActiveAttrib, iSensorSetActiveAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INTERVAL", NULL, NULL, IUPAF_SAMEASSYSTEM, "200", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "X", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "Y", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "Z", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIMESTAMP", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AVAILABLE", iSensorGetAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PERMISSION", iSensorGetPermissionAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvSensorInitClass(ic);

  return ic;
}
