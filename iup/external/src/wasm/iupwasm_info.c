/** \file
 * \brief WebAssembly System Info
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drvinfo.h"


/* the browser viewport is the placement area: dialogs, menus and popovers live inside it, not the physical monitor */
EM_JS(int, iupwasmScreenAvailWidth, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'viewport' })[0];
  return window.innerWidth || 800;
})

EM_JS(int, iupwasmScreenAvailHeight, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'viewport' })[1];
  return window.innerHeight || 600;
})

EM_JS(int, iupwasmScreenFullWidth, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'fullsize' })[0];
  return screen.width || window.innerWidth || 800;
})

EM_JS(int, iupwasmScreenFullHeight, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'fullsize' })[1];
  return screen.height || window.innerHeight || 600;
})

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  if (width) *width = iupwasmScreenFullWidth();
  if (height) *height = iupwasmScreenFullHeight();
}

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  if (width) *width = iupwasmScreenAvailWidth();
  if (height) *height = iupwasmScreenAvailHeight();
}

IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  return 24;
}

EM_JS(int, iupwasmScreenDpi, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'screendpi' });
  return Math.round((window.devicePixelRatio || 1) * 96);
})

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  return iupwasmScreenDpi();
}

IUP_SDK_API int iupdrvScaleNaturalPx(int px)
{
  return px;
}

IUP_SDK_API char* iupdrvGetSystemVersion(void)
{
  return NULL;
}

IUP_SDK_API char* iupdrvGetSystemName(void)
{
  return iupStrReturnStr("WebAssembly");
}

IUP_SDK_API char* iupdrvGetComputerName(void)
{
  return NULL;
}

IUP_SDK_API char* iupdrvGetUserName(void)
{
  return NULL;
}

EM_JS(int, iupwasmKeyMods, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'keystate' });
  return globalThis.__iupMods || 0;
})

EM_JS(int, iupwasmCursorX, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'cursorpos' })[0];
  var m = globalThis.__iupMouse || [0, 0];
  return Math.round(m[0] + (window.screenX || 0));
})
EM_JS(int, iupwasmCursorY, (void), {
  if (typeof window === 'undefined') return globalThis.__iupReadSync({ op: 'cursorpos' })[1];
  var m = globalThis.__iupMouse || [0, 0];
  return Math.round(m[1] + (window.screenY || 0));
})

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  if (key)
  {
    int mods = iupwasmKeyMods();
    key[0] = (mods & 1) ? 'S' : ' ';
    key[1] = (mods & 2) ? 'C' : ' ';
    key[2] = (mods & 4) ? 'A' : ' ';
    key[3] = (mods & 8) ? 'Y' : ' ';
    key[4] = 0;
  }
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  if (x) *x = iupwasmCursorX();
  if (y) *y = iupwasmCursorY();
}

IUP_SDK_API char* iupdrvLocaleInfo(void)
{
  return iupStrReturnStr("UTF-8");
}

IUP_SDK_API int iupdrvGetPreferencePath(char *filename, const char *app_name, int use_system)
{
  (void)app_name;
  (void)use_system;
  if (filename) filename[0] = 0;
  return 0;
}

IUP_SDK_API int iupdrvGetUserDir(char *path, int size, int kind)
{
  (void)size;
  (void)kind;
  if (path) path[0] = 0;
  return 0;
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
  (void)dir;
  return 0;
}

IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  return NULL;
}

void IupLogV(const char* type, const char* format, va_list arglist)
{
  (void)type;
  vfprintf(stderr, format, arglist);
  fputc('\n', stderr);
}

void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
