/** \file
 * \brief WebAssembly Global Attributes
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_singleinstance.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmVirtualScreenPart, (int idx), {
  var a;
  if (typeof window === 'undefined') a = globalThis.__iupReadSync({ op: 'virtualscreen' });
  else a = [(screen.availLeft || 0) | 0, (screen.availTop || 0) | 0, screen.width || 800, screen.height || 600];
  return a[idx];
})

IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    iupdrvSingleInstanceSet(value);
    return 1;
  }
  (void)value;
  return 1;
}

IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "DARKMODE"))
    return iupStrReturnBoolean(iupwasmJsIsDarkMode());
  if (iupStrEqual(name, "VIRTUALSCREEN"))
    return iupStrReturnStrf("%d %d %d %d", iupwasmVirtualScreenPart(0), iupwasmVirtualScreenPart(1),
      iupwasmVirtualScreenPart(2), iupwasmVirtualScreenPart(3));
  if (iupStrEqual(name, "MONITORSINFO"))
    return iupStrReturnStrf("0 0 %d %d\n", iupwasmVirtualScreenPart(2), iupwasmVirtualScreenPart(3));
  if (iupStrEqual(name, "MONITORSCOUNT"))
    return iupStrReturnInt(1);
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
    return iupStrReturnBoolean(1);
  if (iupStrEqual(name, "UTF8MODE"))
    return iupStrReturnBoolean(1);
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
    return iupStrReturnBoolean(0);
  return NULL;
}
