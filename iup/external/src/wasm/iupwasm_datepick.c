/** \file
 * \brief WebAssembly IupDatePick
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsCreateDate, (void), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createdate', id: id });
  return id;
})

EM_JS(void, iupwasmJsDateSet, (int id, const char* iso), {
  globalThis.__iupApply({ op: 'inputval', id: id, value: UTF8ToString(iso) });
})

EM_JS(int, iupwasmJsDateGet, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'inputvalue', id: id });
  else { var el = globalThis.__iup.els[id]; s = el ? el.value : ""; }
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

static void wasmDatePickToday(int* y, int* m, int* d)
{
  time_t timer;
  struct tm* t;
  time(&timer);
  t = localtime(&timer);
  *y = t ? t->tm_year + 1900 : 2000;
  *m = t ? t->tm_mon + 1 : 1;
  *d = t ? t->tm_mday : 1;
}

static int wasmDatePickSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char iso[16];
  int y, m, d;

  if (!id)
    return 0;

  if (!value || iupStrEqualNoCase(value, "TODAY"))
    wasmDatePickToday(&y, &m, &d);
  else if (sscanf(value, "%d%*[/-]%d%*[/-]%d", &y, &m, &d) != 3)
    return 0;

  snprintf(iso, sizeof(iso), "%04d-%02d-%02d", y, m, d);
  iupwasmJsDateSet(id, iso);
  return 0;
}

static char* wasmDatePickGetValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* ptr;
  int y, m, d;

  if (!id)
    return NULL;

  ptr = (char*)(intptr_t)iupwasmJsDateGet(id);
  if (ptr && sscanf(ptr, "%d-%d-%d", &y, &m, &d) == 3)
  {
    free(ptr);
    return iupStrReturnStrf("%d/%d/%d", y, m, d);
  }
  if (ptr) free(ptr);

  wasmDatePickToday(&y, &m, &d);
  return iupStrReturnStrf("%d/%d/%d", y, m, d);
}

static char* wasmDatePickGetTodayAttrib(Ihandle* ih)
{
  int y, m, d;
  (void)ih;
  wasmDatePickToday(&y, &m, &d);
  return iupStrReturnStrf("%d/%d/%d", y, m, d);
}

static int wasmDatePickMapMethod(Ihandle* ih)
{
  int id = iupwasmJsCreateDate();
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmAddToParent(ih);

  if (!iupAttribGet(ih, "VALUE"))
    wasmDatePickSetValueAttrib(ih, "TODAY");
  return IUP_NOERROR;
}

static void wasmDatePickComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)ih;
  (void)children_expand;
  *w = 130;
  *h = 24;
}

Iclass* iupDatePickNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "datepick";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  ic->New = iupDatePickNewClass;

  ic->Map = wasmDatePickMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->ComputeNaturalSize = wasmDatePickComputeNaturalSizeMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", wasmDatePickGetValueAttrib, wasmDatePickSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", wasmDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* native <input type=date> field layout is locale/UA governed, not scriptable */
  iupClassRegisterAttribute(ic, "SEPARATOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZEROPRECED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CALENDARWEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupDatePick(void)
{
  return IupCreate("datepick");
}
