/** \file
 * \brief WebAssembly Frame
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_frame.h"

#include "iupwasm_drv.h"


#define IUPWASM_FRAME_BORDER 1
#define IUPWASM_FRAME_INSET 3
#define IUPWASM_FRAME_TITLE_H 18

static int wasmFrameTitleHeight(Ihandle* ih)
{
  char* title = iupAttribGet(ih, "TITLE");
  return (title && *title) ? IUPWASM_FRAME_TITLE_H : 0;
}

IUP_SDK_API int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 1;
}

IUP_SDK_API void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  if (x) *x = IUPWASM_FRAME_BORDER + IUPWASM_FRAME_INSET;
  if (y) *y = IUPWASM_FRAME_BORDER + IUPWASM_FRAME_INSET;  /* title height is added by the core */
}

IUP_SDK_API int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  int title_h = wasmFrameTitleHeight(ih);
  if (w) *w = 2 * (IUPWASM_FRAME_BORDER + IUPWASM_FRAME_INSET);
  if (h) *h = 2 * (IUPWASM_FRAME_BORDER + IUPWASM_FRAME_INSET) + title_h;
  return 1;
}

IUP_SDK_API int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  if (h) *h = wasmFrameTitleHeight(ih);
  return 1;
}

static void wasmFrameTitle(Ihandle* ih, int id, const char* value)
{
  char* clean = iupwasmStripMnemonic(value);
  (void)ih;
  iupwasmJsFrameSetTitle(id, clean);
  free(clean);
}

static int wasmFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && value)
    wasmFrameTitle(ih, id, value);
  return 1;
}

EM_JS(void, iupwasmJsFrameSunken, (int id, int on), { globalThis.__iupApply({ op: 'framesunken', id: id, on: on }); })

static int wasmFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsFrameSunken(id, iupStrBoolean(value));
  return 1;
}

static int wasmFrameMapMethod(Ihandle* ih)
{
  char* title;
  int id = iupwasmJsCreate("div");
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  iupwasmJsFrameStyle(id);

  title = iupAttribGet(ih, "TITLE");
  if (title && *title)
    wasmFrameTitle(ih, id, title);

  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = wasmFrameMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, wasmFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}
