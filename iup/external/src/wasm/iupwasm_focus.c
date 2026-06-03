/** \file
 * \brief WebAssembly Focus
 *
 * See Copyright Notice in "iup.h"
 */

#include <emscripten.h>

#include "iup.h"

#include "iup_drv.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsFocus, (int id), { globalThis.__iupApply({ op: 'focus', id: id }); })

IUP_SDK_API void iupdrvSetFocus(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsFocus(id);
}
