/** \file
 * \brief WebAssembly Popover
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_layout.h"
#include "iup_popover.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsCreatePopover, (void), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createpopover', id: id });
  return id;
})

EM_JS(void, iupwasmJsElemRect, (int id, int ptr), {
  var a;
  if (typeof document === 'undefined') a = globalThis.__iupReadSync({ op: 'elemrect', id: id });
  else {
    var el = globalThis.__iup.els[id];
    var r = el ? el.getBoundingClientRect() : { left: 0, top: 0, width: 0, height: 0 };
    a = [Math.round(r.left), Math.round(r.top), Math.round(r.width), Math.round(r.height)];
  }
  HEAP32[ptr >> 2] = a[0];
  HEAP32[(ptr >> 2) + 1] = a[1];
  HEAP32[(ptr >> 2) + 2] = a[2];
  HEAP32[(ptr >> 2) + 3] = a[3];
})

EM_JS(void, iupwasmJsPopoverShow, (int id, int x, int y, int w, int h, int arrow, int position), {
  globalThis.__iupApply({ op: 'popovershow', id: id, x: x, y: y, w: w, h: h, arrow: arrow, position: position });
})

EM_JS(void, iupwasmJsPopoverHide, (int id), {
  globalThis.__iupApply({ op: 'popoverhide', id: id });
})

EM_JS(void, iupwasmJsPopoverAutohide, (int id, int anchorId, int install), {
  globalThis.__iupApply({ op: 'popoverauto', id: id, anchorId: anchorId, install: install });
})

EMSCRIPTEN_KEEPALIVE void iupwasmPopoverAutohide(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  if (ih)
    IupSetAttribute(ih, "VISIBLE", "NO");
}

static int wasmPopoverMapMethod(Ihandle* ih)
{
  int id = iupwasmJsCreatePopover();
  if (!id)
    return IUP_ERROR;
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  return IUP_NOERROR;
}

static void wasmPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static char* wasmPopoverGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return "NO";
  return iupStrReturnBoolean(iupAttribGetInt(ih, "_IUPWASM_POPOVER_SHOWN"));
}

static int wasmPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  IFni show_cb;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    int rect[4], x = 0, y = 0;
    int anchor_id;

    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    anchor_id = iupwasmIdOf(anchor);
    iupwasmJsElemRect(anchor_id, (int)(intptr_t)rect);

    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    iupPopoverCalcPosition(ih, rect[0], rect[1], rect[2], rect[3], ih->currentwidth, ih->currentheight, &x, &y);
    iupwasmJsPopoverShow(iupwasmIdOf(ih), x, y, ih->currentwidth, ih->currentheight,
                         iupAttribGetBoolean(ih, "ARROW"), iupPopoverGetPosition(ih));
    iupAttribSetInt(ih, "_IUPWASM_POPOVER_SHOWN", 1);

    if (iupAttribGetBoolean(ih, "AUTOHIDE"))
      iupwasmJsPopoverAutohide(iupwasmIdOf(ih), anchor_id, 1);

    show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    if (show_cb)
      show_cb(ih, IUP_SHOW);
  }
  else
  {
    if (ih->handle)
    {
      iupwasmJsPopoverAutohide(iupwasmIdOf(ih), 0, 0);
      iupwasmJsPopoverHide(iupwasmIdOf(ih));
      iupAttribSetInt(ih, "_IUPWASM_POPOVER_SHOWN", 0);

      show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_HIDE);
    }
  }

  return 0;
}

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = wasmPopoverMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = wasmPopoverLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", wasmPopoverGetVisibleAttrib, wasmPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
}
