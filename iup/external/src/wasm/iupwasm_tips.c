/** \file
 * \brief WebAssembly Tips
 *
 * See Copyright Notice in "iup.h"
 */

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_classbase.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsSetTip, (int id, const char* txt), {
  globalThis.__iupApply({ op: 'tip', id: id, text: UTF8ToString(txt) });
})

IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetTip(id, value);
  return 1;
}

IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}
