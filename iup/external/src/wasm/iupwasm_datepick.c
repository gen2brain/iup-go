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

EM_JS(void, iupwasmJsDateRange, (int id, const char* min, const char* max), {
  globalThis.__iupApply({ op: 'daterange', id: id, min: UTF8ToString(min), max: UTF8ToString(max) });
})

EM_JS(void, iupwasmJsDateShowPicker, (int id), {
  globalThis.__iupApply({ op: 'dateshowpicker', id: id });
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
    return 1;

  if (!value || iupStrEqualNoCase(value, "TODAY"))
    wasmDatePickToday(&y, &m, &d);
  else if (sscanf(value, "%d%*[/-]%d%*[/-]%d", &y, &m, &d) != 3)
    return 0;

  iupStrDateClamp(iupAttribGet(ih, "MINDATE"), iupAttribGet(ih, "MAXDATE"), &y, &m, &d);

  snprintf(iso, sizeof(iso), "%04d-%02d-%02d", y, m, d);
  iupwasmJsDateSet(id, iso);
  return 0;
}

static char* wasmDatePickGetValueAttrib(Ihandle* ih);

static void wasmDatePickUpdateRange(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char min_iso[16] = "", max_iso[16] = "";
  int y, m, d;

  if (!id)
    return;

  if (iupStrToDate(iupAttribGet(ih, "MINDATE"), &y, &m, &d))
    snprintf(min_iso, sizeof(min_iso), "%04d-%02d-%02d", y, m, d);
  if (iupStrToDate(iupAttribGet(ih, "MAXDATE"), &y, &m, &d))
    snprintf(max_iso, sizeof(max_iso), "%04d-%02d-%02d", y, m, d);
  iupwasmJsDateRange(id, min_iso, max_iso);

  if (iupStrToDate(wasmDatePickGetValueAttrib(ih), &y, &m, &d) &&
      iupStrDateClamp(iupAttribGet(ih, "MINDATE"), iupAttribGet(ih, "MAXDATE"), &y, &m, &d))
  {
    char iso[16];
    snprintf(iso, sizeof(iso), "%04d-%02d-%02d", y, m, d);
    iupwasmJsDateSet(id, iso);
  }
}

static int wasmDatePickSetMinDateAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "MINDATE", value);
  wasmDatePickUpdateRange(ih);
  return 1;
}

static int wasmDatePickSetMaxDateAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "MAXDATE", value);
  wasmDatePickUpdateRange(ih);
  return 1;
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

static int wasmDatePickSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && iupStrBoolean(value))
    iupwasmJsDateShowPicker(id);
  return 0;
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

static void wasmDatePickComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
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
  iupClassRegisterAttribute(ic, "TODAY", wasmDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINDATE", NULL, wasmDatePickSetMinDateAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXDATE", NULL, wasmDatePickSetMaxDateAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, wasmDatePickSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

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
