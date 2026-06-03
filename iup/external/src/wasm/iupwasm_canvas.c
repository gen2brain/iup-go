/** \file
 * \brief WebAssembly Canvas
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_canvas.h"
#include "iup_key.h"

#include "iupwasm_drv.h"


/* plain canvas; gl=1 transfers it to the worker for WebGL, gl=0 keeps it on-screen for blitting */
EM_JS(int, iupwasmJsCreateCanvasEl, (int is_gl), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'create', id: id, tag: 'canvas', gl: is_gl });
  return id;
})

/* canvas-as-container: a div wrapping an inner canvas; a bare <canvas> can't host child DOM */
EM_JS(int, iupwasmJsCreateCanvasContainer, (void), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createcanvascontainer', id: id });
  return id;
})

/* IupScrollBox scrolls its oversized child via native browser scroll */
EM_JS(void, iupwasmJsCanvasScrollable, (int id), {
  globalThis.__iupApply({ op: 'canvasscrollable', id: id });
})

EM_JS(void, iupwasmJsCanvasSetup, (int cid), {
  globalThis.__iupApply({ op: 'canvassetup', id: cid });
})

/* SCROLLBAR canvas: native-scroll wrapper + spacer (virtual XMAX x YMAX) + sticky viewport canvas redrawn on scroll */
EM_JS(int, iupwasmJsCreateScrollCanvas, (int sbH, int sbV), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createscrollcanvas', id: id, sbH: sbH, sbV: sbV });
  return id;
})

EM_JS(void, iupwasmJsScrollCanvasRange, (int id, int w, int h), {
  globalThis.__iupApply({ op: 'scrollcanvasrange', id: id, w: w, h: h });
})

EM_JS(void, iupwasmJsScrollCanvasBorder, (int id), {
  globalThis.__iupApply({ op: 'scrollcanvasborder', id: id });
})

EM_JS(void, iupwasmJsScrollCanvasSetPos, (int id, int x, int y), {
  globalThis.__iupApply({ op: 'scrollcanvassetpos', id: id, x: x, y: y });
})

EM_JS(void, iupwasmJsScrollCanvasSetup, (int id), {
  globalThis.__iupApply({ op: 'scrollcanvassetup', id: id });
})

/* per-wheel-step scroll delta in document px (0 = browser default) */
EM_JS(void, iupwasmJsScrollCanvasLine, (int id, int linex, int liney), {
  globalThis.__iupApply({ op: 'scrollcanvasline', id: id, x: linex, y: liney });
})

/* honor explicit LINEX/LINEY only; 0 leaves the browser default wheel step (mirrors gtk's per-step delta) */
static void wasmCanvasUpdateLineStep(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int linex = 0, liney = 0;
  if (!id || ih->data->sb == IUP_SB_NONE)
    return;

  if (iupAttribGet(ih, "LINEX"))
    linex = (int)iupAttribGetDouble(ih, "LINEX");
  if (iupAttribGet(ih, "LINEY"))
    liney = (int)iupAttribGetDouble(ih, "LINEY");

  iupwasmJsScrollCanvasLine(id, linex, liney);
}

void iupwasmCanvasRedraw(Ihandle* ih)
{
  IFn cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmCanvasOnResize(int cid, int w, int h)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFnii cb;
  if (!ih)
    return;

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb && !iupAttribGet(ih, "_IUPWASM_INRESIZE"))
  {
    iupAttribSet(ih, "_IUPWASM_INRESIZE", "1");
    cb(ih, w, h);
    iupAttribSet(ih, "_IUPWASM_INRESIZE", NULL);
  }

  iupwasmCanvasRedraw(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmCanvasMotion(int cid, int x, int y, int mods)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFniis cb;
  char status[IUPKEY_STATUS_SIZE];
  if (!ih)
    return;
  cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    iupwasmFillStatus(status, mods);
    cb(ih, x, y, status);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmCanvasWheel(int cid, int delta, int x, int y)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFnfiis cb;
  if (!ih)
    return;
  cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
  if (cb)
    cb(ih, (float)delta, x, y, (char*)"");
}

EMSCRIPTEN_KEEPALIVE void iupwasmCanvasGesture(int cid, int gesture, int state, int x, int y, double v1, double v2)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFniiiidd cb;
  if (!ih)
    return;
  cb = (IFniiiidd)IupGetCallback(ih, "GESTURE_CB");
  if (cb && cb(ih, gesture, state, x, y, v1, v2) == IUP_CLOSE)
    IupExitLoop();
}

EMSCRIPTEN_KEEPALIVE void iupwasmCanvasTouch(int cid, int id, int x, int y, const char* status)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFniiis cb;
  if (!ih || iupStrEqualNoCase(iupAttribGet(ih, "TOUCH"), "NO"))
    return;
  cb = (IFniiis)IupGetCallback(ih, "TOUCH_CB");
  if (cb && cb(ih, id, x, y, (char*)status) == IUP_CLOSE)
    IupExitLoop();
}

/* points serialized as "id,x,y,state;..." (state = 'D'/'U'/'M' as int) */
EMSCRIPTEN_KEEPALIVE void iupwasmCanvasMultiTouch(int cid, int count, const char* points)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFniIIII cb;
  int *ids, *xs, *ys, *states, i;
  const char* p = points;
  if (!ih || count <= 0 || iupStrEqualNoCase(iupAttribGet(ih, "TOUCH"), "NO"))
    return;
  cb = (IFniIIII)IupGetCallback(ih, "MULTITOUCH_CB");
  if (!cb)
    return;
  ids = (int*)malloc(count * sizeof(int));
  xs = (int*)malloc(count * sizeof(int));
  ys = (int*)malloc(count * sizeof(int));
  states = (int*)malloc(count * sizeof(int));
  for (i = 0; i < count && p && *p; i++)
  {
    ids[i] = atoi(p); p = strchr(p, ','); if (p) p++;
    xs[i] = atoi(p); p = strchr(p, ','); if (p) p++;
    ys[i] = atoi(p); p = strchr(p, ','); if (p) p++;
    states[i] = atoi(p); p = strchr(p, ';'); if (p) p++;
  }
  if (cb(ih, count, ids, xs, ys, states) == IUP_CLOSE)
    IupExitLoop();
  free(ids); free(xs); free(ys); free(states);
}

EMSCRIPTEN_KEEPALIVE void iupwasmCanvasScroll(int cid, int sx, int sy)
{
  Ihandle* ih = iupwasmHandleFromId(cid);
  IFniff cb;
  double xmin, ymin;
  if (!ih)
    return;

  xmin = iupAttribGetDouble(ih, "XMIN");
  ymin = iupAttribGetDouble(ih, "YMIN");
  ih->data->posx = xmin + sx;
  ih->data->posy = ymin + sy;

  cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (cb)
    cb(ih, IUP_SBPOSV, (float)ih->data->posx, (float)ih->data->posy);
  else
    iupwasmCanvasRedraw(ih);  /* no SCROLL_CB: plain repaint */
}


static char* wasmCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    return iupStrReturnIntInt(ih->currentwidth, ih->currentheight, 'x');
  return NULL;
}

static int wasmCanvasMapMethod(Ihandle* ih)
{
  int id;

  if (!ih->parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  {
    int is_gl = IupClassMatch(ih, "glcanvas");  /* GL must use the plain transferable canvas */
    if (!is_gl && ih->iclass->childtype == IUP_CHILDNONE && ih->data->sb != IUP_SB_NONE)
      id = iupwasmJsCreateScrollCanvas(ih->data->sb & IUP_SB_HORIZ, ih->data->sb & IUP_SB_VERT);
    else if (!is_gl && ih->iclass->childtype != IUP_CHILDNONE)
      id = iupwasmJsCreateCanvasContainer();
    else
      id = iupwasmJsCreateCanvasEl(is_gl);
  }
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  if (iupStrEqual(IupGetClassName(ih), "scrollbox"))
  {
    iupwasmJsCanvasScrollable(id);
    iupAttribSet(IupGetDialog(ih), "_IUPWASM_HASSCROLL", "1");
  }

  if (ih->iclass->childtype == IUP_CHILDNONE && ih->data->sb != IUP_SB_NONE)
  {
    if (iupAttribGetBoolean(ih, "BORDER"))
      iupwasmJsScrollCanvasBorder(id);
    iupwasmJsScrollCanvasRange(id, (int)iupAttribGetDouble(ih, "XMAX"), (int)iupAttribGetDouble(ih, "YMAX"));
    iupwasmJsScrollCanvasSetup(id);
    wasmCanvasUpdateLineStep(ih);
  }
  else
  {
    if (iupAttribGetBoolean(ih, "BORDER"))
      iupwasmJsFrameStyle(id);
    iupwasmJsCanvasSetup(id);
  }

  iupwasmAddToParent(ih);

  return IUP_NOERROR;
}

static int wasmCanvasSetLineXAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  wasmCanvasUpdateLineStep(ih);
  return 1;
}

static int wasmCanvasSetLineYAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  wasmCanvasUpdateLineStep(ih);
  return 1;
}

static void wasmCanvasSyncScroll(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->sb != IUP_SB_NONE)
  {
    double xmin = iupAttribGetDouble(ih, "XMIN");
    double ymin = iupAttribGetDouble(ih, "YMIN");
    iupwasmJsScrollCanvasSetPos(id, (int)(ih->data->posx - xmin), (int)(ih->data->posy - ymin));
  }
}

static int wasmCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  double dx, xmin, xmax;
  if (!iupStrToDouble(value, &dx))
    return 1;
  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  if (ih->data->posx > xmax - dx)
  {
    double posx = xmax - dx;
    if (posx < xmin) posx = xmin;
    ih->data->posx = posx;
  }
  return 1;
}

static int wasmCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  double dy, ymin, ymax;
  if (!iupStrToDouble(value, &dy))
    return 1;
  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");
  if (ih->data->posy > ymax - dy)
  {
    double posy = ymax - dy;
    if (posy < ymin) posy = ymin;
    ih->data->posy = posy;
  }
  return 1;
}

static int wasmCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  double posx, xmin, xmax, dx;
  if (!iupStrToDouble(value, &posx))
    return 1;
  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  dx = iupAttribGetDouble(ih, "DX");
  if (dx < xmax - xmin)
  {
    if (posx < xmin) posx = xmin;
    if (posx > xmax - dx) posx = xmax - dx;
  }
  ih->data->posx = posx;
  wasmCanvasSyncScroll(ih);
  return 1;
}

static int wasmCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  double posy, ymin, ymax, dy;
  if (!iupStrToDouble(value, &posy))
    return 1;
  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");
  dy = iupAttribGetDouble(ih, "DY");
  if (dy < ymax - ymin)
  {
    if (posy < ymin) posy = ymin;
    if (posy > ymax - dy) posy = ymax - dy;
  }
  ih->data->posy = posy;
  wasmCanvasSyncScroll(ih);
  return 1;
}

IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = wasmCanvasMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "DRAWSIZE", wasmCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, wasmCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, wasmCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEX", NULL, wasmCanvasSetLineXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEY", NULL, wasmCanvasSetLineYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, wasmCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, wasmCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAWABLE", NULL, NULL, NULL, NULL, IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "TOUCH_CB", "iiis");
  iupClassRegisterCallback(ic, "MULTITOUCH_CB", "iIII");
  iupClassRegisterCallback(ic, "GESTURE_CB", "iiiidd");

  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
}
