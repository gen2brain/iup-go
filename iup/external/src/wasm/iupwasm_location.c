/** \file
 * \brief Location (navigator.geolocation)
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
#include "iup_location.h"

#include "iupwasm_drv.h"

EM_JS(int, iupwasmJsLocationAvailable, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'locavailable' });
  return (navigator.geolocation && (typeof isSecureContext === 'undefined' || isSecureContext)) ? 1 : 0;
})

EM_JS(void, iupwasmJsLocationStart, (int ihptr, int fine, int interval, int distance), {
  globalThis.__iupApply({ op: 'locstart', ihptr: ihptr, fine: fine, interval: interval, distance: distance });
})

EM_JS(void, iupwasmJsLocationStop, (int ihptr), {
  globalThis.__iupApply({ op: 'locstop', ihptr: ihptr });
})

EMSCRIPTEN_KEEPALIVE void iupwasmLocationFix(int ihptr, double lat, double lon, double alt, int has_alt, double acc,
                                             double speed, int has_speed, double heading, int has_heading, double time_ms)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  IlocationMsg msg;
  if (!ih || !iupObjectCheck(ih)) return;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_FIX;
  msg.latitude = lat;
  msg.longitude = lon;
  msg.altitude = alt;
  msg.has_altitude = has_alt;
  msg.accuracy = acc;
  msg.speed = speed;
  msg.has_speed = has_speed;
  msg.heading = heading;
  msg.has_heading = has_heading;
  msg.timestamp = (long long)time_ms;
  iupLocationPost(ih, &msg);
}

EMSCRIPTEN_KEEPALIVE void iupwasmLocationError(int ihptr, int code)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  IlocationMsg msg;
  if (!ih || !iupObjectCheck(ih)) return;

  if (code == 1)
  {
    memset(&msg, 0, sizeof(msg));
    msg.type = IUP_LOCATION_PERMISSION;
    msg.granted = 0;
    iupLocationPost(ih, &msg);
    iupAttribSet(ih, "_IUPWASM_LOCATION_DENIED", "1");
  }

  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), code == 1? "Location access denied": code == 2? "Position unavailable": "Location timeout");
  iupLocationPost(ih, &msg);
}

EMSCRIPTEN_KEEPALIVE void iupwasmLocationGranted(int ihptr)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  IlocationMsg msg;
  if (!ih || !iupObjectCheck(ih)) return;
  if (iupAttribGet(ih, "_IUPWASM_LOCATION_GRANTED")) return;
  iupAttribSet(ih, "_IUPWASM_LOCATION_GRANTED", "1");
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_PERMISSION;
  msg.granted = 1;
  iupLocationPost(ih, &msg);
}

IUP_SDK_API int iupdrvLocationIsAvailable(void)
{
  return iupwasmJsLocationAvailable();
}

IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  if (!iupwasmJsLocationAvailable())
  {
    IlocationMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = IUP_LOCATION_ERROR;
    iupStrCopyN(msg.error, sizeof(msg.error), "Geolocation not available");
    iupLocationPost(ih, &msg);
    return 0;
  }
  iupwasmJsLocationStart((int)(intptr_t)ih, iupStrEqualNoCase(iupAttribGetStr(ih, "ACCURACY"), "FINE"),
                         iupAttribGetInt(ih, "INTERVAL"), iupAttribGetInt(ih, "DISTANCE"));
  return 1;
}

IUP_SDK_API void iupdrvLocationStop(Ihandle* ih)
{
  iupwasmJsLocationStop((int)(intptr_t)ih);
}

IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  if (!iupwasmJsLocationAvailable()) return "UNAVAILABLE";
  if (iupAttribGet(ih, "_IUPWASM_LOCATION_GRANTED")) return "GRANTED";
  if (iupAttribGet(ih, "_IUPWASM_LOCATION_DENIED")) return "DENIED";
  return "PROMPT";
}

IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih)
{
  iupwasmJsLocationStop((int)(intptr_t)ih);
}

IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic)
{
  (void)ic;
}
