/** \file
 * \brief WebAssembly Scrollbar
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_scrollbar.h"

#include "iupwasm_drv.h"


#define IUPWASM_SB_STEPS 1000

EM_JS(int, iupwasmJsCreateScrollbar, (int vertical), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createrange', id: id, vertical: vertical, kind: 'scroll' });
  return id;
})

EM_JS(void, iupwasmJsScrollbarSet, (int id, int pos), {
  globalThis.__iupApply({ op: 'rangeset', id: id, pos: pos });
})

static double wasmSbRange(Ihandle* ih)
{
  double r = ih->data->vmax - ih->data->vmin;
  return (r == 0) ? 1 : r;
}

static void wasmSbReflect(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsScrollbarSet(id, (int)(((ih->data->val - ih->data->vmin) / wasmSbRange(ih)) * IUPWASM_SB_STEPS + 0.5));
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchScrollbar(int id, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniff scroll_cb;
  IFn vc_cb;
  int op;
  if (!ih)
    return;

  ih->data->val = ih->data->vmin + ((double)pos / IUPWASM_SB_STEPS) * wasmSbRange(ih);
  iupScrollbarCropValue(ih);

  scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    {
      op = IUP_SBPOSH;
      scroll_cb(ih, op, (float)ih->data->val, 0);
    }
    else
    {
      op = IUP_SBPOSV;
      scroll_cb(ih, op, 0, (float)ih->data->val);
    }
  }

  vc_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vc_cb)
    vc_cb(ih);
}

static int wasmSbSetValueAttrib(Ihandle* ih, const char* value)
{
  double v;
  if (iupStrToDouble(value, &v))
  {
    ih->data->val = v;
    iupScrollbarCropValue(ih);
    wasmSbReflect(ih);
  }
  return 0;
}

static int wasmSbSetLineStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDouble(value, &ih->data->linestep);
  return 0;
}

static int wasmSbSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDouble(value, &ih->data->pagestep);
  return 0;
}

static int wasmSbSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &ih->data->pagesize))
  {
    iupScrollbarCropValue(ih);
    wasmSbReflect(ih);
  }
  return 0;
}

static int wasmScrollbarMapMethod(Ihandle* ih)
{
  int vertical = (ih->data->orientation == ISCROLLBAR_VERTICAL);
  int id = iupwasmJsCreateScrollbar(vertical);
  if (!id)
    return IUP_ERROR;
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  wasmSbReflect(ih);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  if (ih->data->orientation == ISCROLLBAR_VERTICAL)
  {
    if (w) *w = 16;
    if (h) *h = 80;
  }
  else
  {
    if (w) *w = 80;
    if (h) *h = 16;
  }
}

IUP_SDK_API void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = wasmScrollbarMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, wasmSbSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, wasmSbSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, wasmSbSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, wasmSbSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
