/** \file
 * \brief WebAssembly Single Instance
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"

#include "iup_singleinstance.h"


IUP_SDK_API int iupdrvSingleInstanceSet(const char* name)
{
  (void)name;
  return 0;
}

IUP_SDK_API void iupdrvSingleInstanceClose(void)
{
}
