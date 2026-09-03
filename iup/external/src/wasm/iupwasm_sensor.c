/** \file
 * \brief Sensor (Generic Sensor API and DeviceMotionEvent)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <emscripten.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_sensor.h"

#include "iupwasm_drv.h"

EM_JS(int, iupwasmJsSensorAvailable, (int type), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'senavailable', type: type });
  return globalThis.__iupSensorAvailable(type);
})

EM_JS(void, iupwasmJsSensorStart, (int ihptr, int type, int interval), {
  globalThis.__iupApply({ op: 'senstart', ihptr: ihptr, type: type, interval: interval });
})

EM_JS(void, iupwasmJsSensorStop, (int ihptr), {
  globalThis.__iupApply({ op: 'senstop', ihptr: ihptr });
})

EMSCRIPTEN_KEEPALIVE void iupwasmSensorReading(int ihptr, double x, double y, double z, double time_ms)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  IsensorMsg msg;
  if (!ih || !iupObjectCheck(ih)) return;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_READING;
  msg.x = x;
  msg.y = y;
  msg.z = z;
  msg.timestamp = (long long)time_ms;
  iupSensorPost(ih, &msg);
}

EMSCRIPTEN_KEEPALIVE void iupwasmSensorPermission(int ihptr, int granted)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  IsensorMsg msg;
  if (!ih || !iupObjectCheck(ih)) return;
  iupAttribSet(ih, "_IUPWASM_SENSOR_PERMISSION", granted? "GRANTED": "DENIED");
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_PERMISSION;
  msg.granted = granted;
  iupSensorPost(ih, &msg);
}

EMSCRIPTEN_KEEPALIVE void iupwasmSensorError(int ihptr, int code)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  IsensorMsg msg;
  if (!ih || !iupObjectCheck(ih)) return;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_SENSOR_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), code == 1? "Sensor access denied": code == 2? "Sensor not available": "Sensor error");
  iupSensorPost(ih, &msg);
}

IUP_SDK_API int iupdrvSensorIsAvailable(int type)
{
  return iupwasmJsSensorAvailable(type);
}

IUP_SDK_API int iupdrvSensorStart(Ihandle* ih)
{
  int type = iupSensorGetType(ih);
  if (!iupwasmJsSensorAvailable(type))
  {
    IsensorMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = IUP_SENSOR_ERROR;
    iupStrCopyN(msg.error, sizeof(msg.error), "Sensor not available");
    iupSensorPost(ih, &msg);
    return 0;
  }
  iupwasmJsSensorStart((int)(intptr_t)ih, type, iupAttribGetInt(ih, "INTERVAL"));
  return 1;
}

IUP_SDK_API void iupdrvSensorStop(Ihandle* ih)
{
  iupwasmJsSensorStop((int)(intptr_t)ih);
}

IUP_SDK_API char* iupdrvSensorGetPermission(Ihandle* ih)
{
  char* permission = iupAttribGet(ih, "_IUPWASM_SENSOR_PERMISSION");
  if (!iupwasmJsSensorAvailable(iupSensorGetType(ih))) return "UNAVAILABLE";
  if (permission) return permission;
  return iupwasmJsSensorAvailable(-1)? "PROMPT": "GRANTED";
}

IUP_SDK_API void iupdrvSensorDestroy(Ihandle* ih)
{
  iupwasmJsSensorStop((int)(intptr_t)ih);
}

IUP_SDK_API void iupdrvSensorInitClass(Iclass* ic)
{
  (void)ic;
}
