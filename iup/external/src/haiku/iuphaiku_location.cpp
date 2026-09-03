/** \file
 * \brief Haiku Location (no system location service)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstring>

#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_location.h"

extern "C" IUP_SDK_API int iupdrvLocationIsAvailable(void) { return 0; }

extern "C" IUP_SDK_API int iupdrvLocationStart(Ihandle* ih)
{
  IlocationMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = IUP_LOCATION_ERROR;
  iupStrCopyN(msg.error, sizeof(msg.error), "Location service not available");
  iupLocationPost(ih, &msg);
  return 0;
}

extern "C" IUP_SDK_API void iupdrvLocationStop(Ihandle* ih) { (void)ih; }

extern "C" IUP_SDK_API char* iupdrvLocationGetPermission(Ihandle* ih)
{
  (void)ih;
  return (char*)"UNAVAILABLE";
}

extern "C" IUP_SDK_API void iupdrvLocationDestroy(Ihandle* ih) { (void)ih; }

extern "C" IUP_SDK_API void iupdrvLocationInitClass(Iclass* ic) { (void)ic; }
