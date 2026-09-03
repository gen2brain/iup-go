/** \file
 * \brief Location Control - Geolocation
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_LOCATION_H
#define __IUP_LOCATION_H

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup drv
 * @{ */

enum { IUP_LOCATION_FIX, IUP_LOCATION_PERMISSION, IUP_LOCATION_ERROR };

typedef struct _IlocationMsg
{
  int type;
  double latitude, longitude, altitude, accuracy, speed, heading;
  long long timestamp;
  int has_altitude, has_speed, has_heading;
  int granted;
  char error[128];
} IlocationMsg;

/* Copies the message and posts it to the main loop, can be called from any thread. */
IUP_SDK_API void iupLocationPost(Ihandle* ih, const IlocationMsg* msg);

IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic);
IUP_SDK_API int iupdrvLocationIsAvailable(void);
IUP_SDK_API int iupdrvLocationStart(Ihandle* ih);
IUP_SDK_API void iupdrvLocationStop(Ihandle* ih);
IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih);
IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
