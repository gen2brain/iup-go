/** \file
 * \brief WebAssembly IupHelp/IupExecute
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include <emscripten.h>

#include "iup.h"


EM_JS(void, iupwasmJsOpenUrl, (const char* url), {
  globalThis.__iupApply({ op: 'openurl', url: UTF8ToString(url) });
})

int IupHelp(const char* url)
{
  if (!url || !url[0])
    return 0;
  iupwasmJsOpenUrl(url);
  return 1;
}

int IupExecute(const char* filename, const char* parameters)
{
  (void)parameters;
  if (filename && (strncmp(filename, "http://", 7) == 0 || strncmp(filename, "https://", 8) == 0))
  {
    iupwasmJsOpenUrl(filename);
    return 1;
  }
  return -1;
}

int IupExecuteWait(const char* filename, const char* parameters)
{
  return IupExecute(filename, parameters);
}
