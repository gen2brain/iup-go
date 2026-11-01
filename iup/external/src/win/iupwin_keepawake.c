/** \file
 * \brief Keep the display awake
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include "iup.h"

#include "iup_drvinfo.h"


IUP_SDK_API void iupdrvSetKeepAwake(int keep)
{
  SetThreadExecutionState(keep ? ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED : ES_CONTINUOUS);
}
