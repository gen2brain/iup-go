/** \file
 * \brief WebAssembly Val (slider)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_val.h"

#include "iupwasm_drv.h"


#define IUPWASM_VAL_STEPS 1000

EM_JS(int, iupwasmJsCreateVal, (int vertical), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createrange', id: id, vertical: vertical, kind: 'val' });
  return id;
})

EM_JS(void, iupwasmJsValSet, (int id, int pos), {
  globalThis.__iupApply({ op: 'rangeset', id: id, pos: pos });
})

EM_JS(void, iupwasmJsValWire, (int id), { globalThis.__iupApply({ op: 'valwire', id: id }); })

EM_JS(void, iupwasmJsValStep, (int id, int step), {
  globalThis.__iupApply({ op: 'valstep', id: id, step: step });
})

EM_JS(void, iupwasmJsValTicks, (int id, int n), {
  globalThis.__iupApply({ op: 'valticks', id: id, n: n });
})

EM_JS(void, iupwasmJsValPageStep, (int id, int page), {
  globalThis.__iupApply({ op: 'valpagestep', id: id, page: page });
})

static double wasmValRange(Ihandle* ih)
{
  double r = ih->data->vmax - ih->data->vmin;
  return (r == 0) ? 1 : r;
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchVal(int id, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  ih->data->val = ih->data->vmin + ((double)pos / IUPWASM_VAL_STEPS) * wasmValRange(ih);
  cb = IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
}

static int wasmValSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  double v = 0;
  if (iupStrToDouble(value, &v))
  {
    if (v < ih->data->vmin) v = ih->data->vmin;
    if (v > ih->data->vmax) v = ih->data->vmax;
    ih->data->val = v;
    if (id)
      iupwasmJsValSet(id, (int)(((v - ih->data->vmin) / wasmValRange(ih)) * IUPWASM_VAL_STEPS + 0.5));
  }
  return 0;
}

static int wasmValSetStepAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  double step;
  if (iupStrToDouble(value, &step))
  {
    ih->data->step = step;
    if (id)
      iupwasmJsValStep(id, (int)(step * IUPWASM_VAL_STEPS + 0.5));
  }
  return 0;
}

static int wasmValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), n = 0;
  iupStrToInt(value, &n);
  if (id)
    iupwasmJsValTicks(id, n);
  return 1;
}

static int wasmValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  double page;
  if (iupStrToDouble(value, &page))
  {
    ih->data->pagestep = page;
    if (id)
      iupwasmJsValPageStep(id, (int)(page * IUPWASM_VAL_STEPS + 0.5));
  }
  return 0;
}

/* datalist tick marks render on the UA-chosen side only; REVERSE/BOTH fall back to NORMAL */
static int wasmValSetTicksPosAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), n;
  (void)value;
  n = iupAttribGetInt(ih, "SHOWTICKS");
  if (id && n > 1)
    iupwasmJsValTicks(id, n);
  return 1;
}

static int wasmValMapMethod(Ihandle* ih)
{
  int vertical = (ih->data->orientation == IVAL_VERTICAL);
  int id = iupwasmJsCreateVal(vertical);
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmJsValSet(id, (int)(((ih->data->val - ih->data->vmin) / wasmValRange(ih)) * IUPWASM_VAL_STEPS + 0.5));
  iupwasmJsValPageStep(id, (int)(ih->data->pagestep * IUPWASM_VAL_STEPS + 0.5));
  iupwasmJsValWire(id);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  if (ih->data->orientation == IVAL_VERTICAL)
  {
    if (w) *w = 22;
    if (h) *h = 80;
  }
  else
  {
    if (w) *w = 80;
    if (h) *h = 22;
  }
}

IUP_SDK_API void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = wasmValMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, wasmValSetValueAttrib, "0", NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, wasmValSetStepAttrib, IUPAF_SAMEASSYSTEM, "0.01", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, wasmValSetPageStepAttrib, IUPAF_SAMEASSYSTEM, "0.1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, wasmValSetShowTicksAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, wasmValSetTicksPosAttrib, IUPAF_SAMEASSYSTEM, "NORMAL", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}
