/** \file
 * \brief Location Control - Geolocation
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
#include "iup_location.h"


static void iLocationFix(Ihandle* ih, const IlocationMsg* msg)
{
  IFndd cb;

  iupAttribSetDouble(ih, "LATITUDE", msg->latitude);
  iupAttribSetDouble(ih, "LONGITUDE", msg->longitude);
  iupAttribSetDouble(ih, "HORIZONTALACCURACY", msg->accuracy);
  iupAttribSetStrf(ih, "TIMESTAMP", "%lld", msg->timestamp);

  if (msg->has_altitude) iupAttribSetDouble(ih, "ALTITUDE", msg->altitude);
  else iupAttribSet(ih, "ALTITUDE", NULL);
  if (msg->has_speed) iupAttribSetDouble(ih, "SPEED", msg->speed);
  else iupAttribSet(ih, "SPEED", NULL);
  if (msg->has_heading) iupAttribSetDouble(ih, "HEADING", msg->heading);
  else iupAttribSet(ih, "HEADING", NULL);

  cb = (IFndd)IupGetCallback(ih, "LOCATION_CB");
  if (cb && cb(ih, msg->latitude, msg->longitude) == IUP_CLOSE)
    IupExitLoop();
}

static int iLocationPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  IlocationMsg* msg = (IlocationMsg*)p;
  (void)s; (void)i; (void)d;

  if (!msg)
    return IUP_DEFAULT;

  if (msg->type == IUP_LOCATION_FIX)
    iLocationFix(ih, msg);
  else if (msg->type == IUP_LOCATION_PERMISSION)
  {
    IFni cb = (IFni)IupGetCallback(ih, "PERMISSION_CB");
    if (cb && cb(ih, msg->granted) == IUP_CLOSE)
      IupExitLoop();
  }
  else if (msg->type == IUP_LOCATION_ERROR)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb && cb(ih, msg->error) == IUP_CLOSE)
      IupExitLoop();
  }

  free(msg);
  return IUP_DEFAULT;
}

IUP_SDK_API void iupLocationPost(Ihandle* ih, const IlocationMsg* msg)
{
  IlocationMsg* copy = (IlocationMsg*)malloc(sizeof(IlocationMsg));
  if (!copy)
    return;
  *copy = *msg;
  IupPostMessage(ih, NULL, 0, 0, copy);
}

static int iLocationSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    if (!iupAttribGetBoolean(ih, "_IUP_LOCATION_ACTIVE") && iupdrvLocationStart(ih))
      iupAttribSet(ih, "_IUP_LOCATION_ACTIVE", "1");
  }
  else if (iupAttribGetBoolean(ih, "_IUP_LOCATION_ACTIVE"))
  {
    iupdrvLocationStop(ih);
    iupAttribSet(ih, "_IUP_LOCATION_ACTIVE", NULL);
  }
  return 0;
}

static char* iLocationGetActiveAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupAttribGetBoolean(ih, "_IUP_LOCATION_ACTIVE"));
}

static char* iLocationGetAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(iupdrvLocationIsAvailable());
}

static char* iLocationGetPermissionAttrib(Ihandle* ih)
{
  return iupdrvLocationGetPermission(ih);
}

static int iLocationCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  IupSetCallback(ih, "POSTMESSAGE_CB", (Icallback)iLocationPostMessage);
  return IUP_NOERROR;
}

static void iLocationDestroyMethod(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "_IUP_LOCATION_ACTIVE"))
    iupdrvLocationStop(ih);
  iupdrvLocationDestroy(ih);
}

/******************************************************************************/

IUP_API Ihandle* IupLocation(void)
{
  return IupCreate("location");
}

Iclass* iupLocationNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "location";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupLocationNewClass;
  ic->Create = iLocationCreateMethod;
  ic->Destroy = iLocationDestroyMethod;

  iupClassRegisterCallback(ic, "LOCATION_CB", "dd");
  iupClassRegisterCallback(ic, "PERMISSION_CB", "i");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  iupClassRegisterAttribute(ic, "ACTIVE", iLocationGetActiveAttrib, iLocationSetActiveAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACCURACY", NULL, NULL, IUPAF_SAMEASSYSTEM, "COARSE", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INTERVAL", NULL, NULL, IUPAF_SAMEASSYSTEM, "1000", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DISTANCE", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LATITUDE", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LONGITUDE", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALTITUDE", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HORIZONTALACCURACY", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPEED", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HEADING", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIMESTAMP", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AVAILABLE", iLocationGetAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PERMISSION", iLocationGetPermissionAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupdrvLocationInitClass(ic);

  return ic;
}
