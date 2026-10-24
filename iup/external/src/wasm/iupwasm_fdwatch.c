/** \file
 * \brief File Descriptor Watch for the WebAssembly Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"


IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  (void)ih;
  return 0;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  (void)ih;
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "FD", NULL, NULL, IUPAF_SAMEASSYSTEM, "-1", IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
