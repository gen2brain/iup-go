/** \file
 * \brief WebAssembly ProgressBar
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
#include "iup_progressbar.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsCreateProgress, (int vertical), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createprogress', id: id, vertical: vertical });
  return id;
})

EM_JS(void, iupwasmJsProgressValue, (int id, double ratio), {
  globalThis.__iupApply({ op: 'progressvalue', id: id, ratio: ratio });
})

EM_JS(void, iupwasmJsProgressMarquee, (int id, int on), {
  globalThis.__iupApply({ op: 'progressmarquee', id: id, on: on });
})

EM_JS(void, iupwasmJsProgressApply, (int id), {
  globalThis.__iupApply({ op: 'progressapply', id: id });
})

EM_JS(void, iupwasmJsProgressFg, (int id, int r, int g, int b), {
  globalThis.__iupApply({ op: 'progressfg', id: id, fg: 'rgb(' + r + ',' + g + ',' + b + ')' });
  iupwasmJsProgressApply(id);
})

EM_JS(void, iupwasmJsProgressDashed, (int id, int on), {
  globalThis.__iupApply({ op: 'progressdashed', id: id, on: on });
  iupwasmJsProgressApply(id);
})

static double wasmProgressRange(Ihandle* ih)
{
  double r = ih->data->vmax - ih->data->vmin;
  return (r == 0) ? 1 : r;
}

static void wasmProgressUpdate(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsProgressValue(id, (ih->data->value - ih->data->vmin) / wasmProgressRange(ih));
}

static int wasmProgressSetValueAttrib(Ihandle* ih, const char* value)
{
  double v = 0;
  if (iupStrToDouble(value, &v))
  {
    if (v < ih->data->vmin) v = ih->data->vmin;
    if (v > ih->data->vmax) v = ih->data->vmax;
    ih->data->value = v;
    wasmProgressUpdate(ih);
  }
  return 0;
}

static int wasmProgressSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->marquee = iupStrBoolean(value);
  if (id)
    iupwasmJsProgressMarquee(id, ih->data->marquee);
  return 1;
}

static int wasmProgressSetDashedAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsProgressDashed(id, iupStrBoolean(value));
  return 1;
}

static int wasmProgressSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  int id = iupwasmIdOf(ih);
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;
  if (id)
    iupwasmJsProgressFg(id, r, g, b);
  return 1;
}

/* keep ORIENTATION in the hash (core setter drops it) so Map can read it */
static int wasmProgressSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
  {
    int min_w, min_h;
    iupdrvProgressBarGetMinSize(ih, &min_w, &min_h);
    if (iupStrEqualNoCase(value, "VERTICAL"))
      IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", min_w, 200);
    else
      IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", 200, min_h);
  }
  return 1;
}

static int wasmProgressMapMethod(Ihandle* ih)
{
  int vertical = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL");
  int id = iupwasmJsCreateProgress(vertical);
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  if (iupAttribGetBoolean(ih, "DASHED"))
    iupwasmJsProgressDashed(id, 1);

  if (iupAttribGetBoolean(ih, "MARQUEE"))
    iupwasmJsProgressMarquee(id, 1);
  else
    wasmProgressUpdate(ih);

  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  /* core hardcodes 200 for the long axis; return the bar thickness for both */
  (void)ih;
  if (w) *w = 18;
  if (h) *h = 18;
}

IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = wasmProgressMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, wasmProgressSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, wasmProgressSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, wasmProgressSetMarqueeAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, wasmProgressSetFgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, wasmProgressSetDashedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
