/** \file
 * \brief WebAssembly IupClipboard
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"


/* local cache backs Get/HasText; the OS write goes to main (no navigator.clipboard on a Worker) */
EM_JS(void, iupwasmJsClipboardSet, (const char* txt), {
  var s = UTF8ToString(txt);
  globalThis.__iupClipText = s;
  globalThis.__iupApply({ op: 'clipwrite', text: s });
})

EM_JS(int, iupwasmJsClipboardGet, (void), {
  var s = globalThis.__iupClipText;
  if (s === undefined || s === null) return 0;
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

EM_JS(int, iupwasmJsClipboardHasText, (void), {
  var s = globalThis.__iupClipText;
  return (s !== undefined && s !== null && s.length > 0) ? 1 : 0;
})

static int wasmClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  iupwasmJsClipboardSet(value ? value : "");
  return 0;
}

static char* wasmClipboardGetTextAttrib(Ihandle* ih)
{
  char* ptr = (char*)(intptr_t)iupwasmJsClipboardGet();
  char* ret;
  (void)ih;
  if (!ptr)
    return NULL;
  ret = iupStrReturnStr(ptr);
  free(ptr);
  return ret;
}

static char* wasmClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(iupwasmJsClipboardHasText());
}

Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", wasmClipboardGetTextAttrib, wasmClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", wasmClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}
