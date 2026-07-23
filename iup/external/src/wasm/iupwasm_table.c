/** \file
 * \brief WebAssembly Table
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_table.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsTableCreate, (void), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'tablecreate', id: id });
  return id;
})

EM_JS(void, iupwasmJsTableSetGrid, (int id, int show), {
  globalThis.__iupApply({ op: 'tablegrid', id: id, show: show });
})

EM_JS(void, iupwasmJsTableBuild, (int id, int numlin, int numcol), {
  globalThis.__iupApply({ op: 'tablebuild', id: id, numlin: numlin, numcol: numcol });
})

EM_JS(void, iupwasmJsTableFeatures, (int id, int reorder, int resize, int dragdrop), {
  globalThis.__iupApply({ op: 'tablefeatures', id: id, reorder: reorder, resize: resize, dragdrop: dragdrop });
})

EM_JS(void, iupwasmJsTableMoveRow, (int id, int from, int to), {
  globalThis.__iupApply({ op: 'tablemoverow', id: id, from: from, to: to });
})

EM_JS(void, iupwasmJsTableColTitle, (int id, int col, const char* title), {
  globalThis.__iupApply({ op: 'tablecoltitle', id: id, col: col, title: UTF8ToString(title) });
})

EM_JS(void, iupwasmJsTableColWidth, (int id, int col, int w), {
  globalThis.__iupApply({ op: 'tablecolwidth', id: id, col: col, w: w });
})

EM_JS(int, iupwasmJsTableColWidthGet, (int id, int col), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'tablecolwidthget', id: id, col: col });
  var wrap = globalThis.__iup.els[id]; if (!wrap) return 0;
  var th = wrap.__iupHead.children[col - 1];
  return th ? th.offsetWidth : 0;
})

EM_JS(void, iupwasmJsTableRender, (int id, int lin, int col), {
  globalThis.__iupApply({ op: 'tablerender', id: id, lin: lin, col: col });
})

EM_JS(void, iupwasmJsTableSetCell, (int id, int lin, int col, const char* value), {
  globalThis.__iupApply({ op: 'tablesetcell', id: id, lin: lin, col: col, value: UTF8ToString(value) });
})

EM_JS(int, iupwasmJsTableGetCell, (int id, int lin, int col), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'tablegetcell', id: id, lin: lin, col: col });
  else { var wrap = globalThis.__iup.els[id]; s = ""; if (wrap) { var tr = wrap.__iupBody.children[lin - 1]; if (tr) { var td = tr.children[col - 1]; if (td) s = td.__iupText || ""; } } }
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

EM_JS(void, iupwasmJsTableSetFitImage, (int id, int fit, int maxH), {
  globalThis.__iupApply({ op: 'tablefitimage', id: id, fit: fit, maxH: maxH });
})

EM_JS(void, iupwasmJsTableSetCellImage, (int id, int lin, int col, int imgId), {
  globalThis.__iupApply({ op: 'tablecellimage', id: id, lin: lin, col: col, imgId: imgId });
})

EM_JS(void, iupwasmJsTableStripe, (int id, int alt, const char* even, const char* odd), {
  globalThis.__iupApply({ op: 'tablestripe', id: id, alt: alt, even: UTF8ToString(even), odd: UTF8ToString(odd) });
})

EM_JS(void, iupwasmJsTableColAlign, (int id, int col, const char* css), {
  globalThis.__iupApply({ op: 'tablecolalign', id: id, col: col, css: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsTableCellColor, (int id, int lin, int col, const char* bg, const char* fg), {
  globalThis.__iupApply({ op: 'tablecellcolor', id: id, lin: lin, col: col, bg: UTF8ToString(bg), fg: UTF8ToString(fg) });
})

EM_JS(int, iupwasmJsTableVScrollTop, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'tablevscrolltop', id: id });
  var w = globalThis.__iup.els[id]; return w ? (w.scrollTop | 0) : 0;
})

EM_JS(int, iupwasmJsTableVClientH, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'tablevclienth', id: id });
  var w = globalThis.__iup.els[id]; return w ? (w.clientHeight | 0) : 0;
})

EM_JS(void, iupwasmJsTableVirtualInit, (int id), {
  globalThis.__iupApply({ op: 'tablevirtualinit', id: id });
})

EM_JS(void, iupwasmJsTableVWindow, (int id, int count, int numCol, int top, int bot), {
  globalThis.__iupApply({ op: 'tablevwindow', id: id, count: count, numCol: numCol, top: top, bot: bot });
})

EM_JS(void, iupwasmJsTableVCell, (int id, int rowIdx, int col, int lin, const char* str, int imgId), {
  globalThis.__iupApply({ op: 'tablevcell', id: id, rowIdx: rowIdx, col: col, lin: lin, str: UTF8ToString(str), imgId: imgId });
})

EM_JS(void, iupwasmJsTableVStripe, (int id, int rowIdx, int lin), {
  globalThis.__iupApply({ op: 'tablevstripe', id: id, rowIdx: rowIdx, lin: lin });
})

EM_JS(void, iupwasmJsTableFocus, (int id, int lin, int col, int select, int focusrect), {
  globalThis.__iupApply({ op: 'tablefocus', id: id, lin: lin, col: col, select: select, focusrect: focusrect });
})

EM_JS(void, iupwasmJsTableScrollTo, (int id, int lin, int col), {
  globalThis.__iupApply({ op: 'tablescrollto', id: id, lin: lin, col: col });
})

static void wasmTableApplyColors(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int alt = iupAttribGetBoolean(ih, "ALTERNATECOLOR");
  char* even = iupAttribGetStr(ih, "EVENROWCOLOR");
  char* odd = iupAttribGetStr(ih, "ODDROWCOLOR");
  if (id)
    iupwasmJsTableStripe(id, alt, even ? even : "#ffffff", odd ? odd : "#f0f0f0");
}

static const char* wasmTableAlignCss(const char* a)
{
  if (a && (iupStrEqualNoCase(a, "ARIGHT") || iupStrEqualNoCase(a, "RIGHT"))) return "right";
  if (a && (iupStrEqualNoCase(a, "ACENTER") || iupStrEqualNoCase(a, "CENTER"))) return "center";
  return "left";
}

static void wasmTableApplyAlign(Ihandle* ih)
{
  int id = iupwasmIdOf(ih), col;
  if (!id) return;
  for (col = 1; col <= ih->data->num_col; col++)
  {
    char name[16];
    snprintf(name, sizeof(name), "ALIGNMENT%d", col);
    iupwasmJsTableColAlign(id, col, wasmTableAlignCss(iupAttribGet(ih, name)));
  }
}

static void wasmTableApplyCellColors(Ihandle* ih)
{
  int id = iupwasmIdOf(ih), lin, col;
  if (!id || iupAttribGetBoolean(ih, "VIRTUALMODE")) return;
  for (lin = 1; lin <= ih->data->num_lin; lin++)
    for (col = 1; col <= ih->data->num_col; col++)
    {
      char* bg = iupAttribGetId2(ih, "BGCOLOR", lin, col);
      char* fg = iupAttribGetId2(ih, "FGCOLOR", lin, col);
      if (!bg) bg = iupAttribGetId2(ih, "BGCOLOR", 0, col);
      if (!bg) bg = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
      if (!fg) fg = iupAttribGetId2(ih, "FGCOLOR", 0, col);
      if (!fg) fg = iupAttribGetId2(ih, "FGCOLOR", lin, 0);
      if (bg || fg)
        iupwasmJsTableCellColor(id, lin, col, bg ? bg : "", fg ? fg : "");
    }
}

static void wasmTableVirtualRender(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int rowH, numLin, numCol, first, count, i, c, showImage;
  sIFnii value_cb, image_cb;

  if (!id || !iupAttribGetBoolean(ih, "VIRTUALMODE"))
    return;

  rowH = iupdrvTableGetRowHeight(ih);
  numLin = ih->data->num_lin;
  numCol = ih->data->num_col;

  first = iupwasmJsTableVScrollTop(id) / rowH - 4;
  if (first < 0) first = 0;
  count = iupwasmJsTableVClientH(id) / rowH + 9;
  if (first + count > numLin) count = numLin - first;
  if (count < 0) count = 0;

  iupwasmJsTableVWindow(id, count, numCol, first * rowH, (numLin - first - count) * rowH);

  value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  image_cb = (sIFnii)IupGetCallback(ih, "IMAGE_CB");
  showImage = ih->data->show_image;
  for (i = 0; i < count; i++)
  {
    int lin = first + i + 1;
    for (c = 1; c <= numCol; c++)
    {
      char* v;
      int imgId = 0;
      if (showImage && image_cb)
      {
        char* name = image_cb(ih, lin, c);
        if (name && name[0])
          imgId = (int)(intptr_t)iupImageGetImage(name, ih, 0, NULL);
      }
      v = value_cb ? value_cb(ih, lin, c) : NULL;  /* must be the last string dispatch before use: result lives in a single recycled slot */
      iupwasmJsTableVCell(id, i, c - 1, lin, v ? v : "", imgId);
    }
    iupwasmJsTableVStripe(id, i, lin);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableVScroll(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  if (ih)
    wasmTableVirtualRender(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableHeaderClick(int id, int col)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFni sort_cb;
  if (!ih || !IupGetInt(ih, "SORTABLE"))
    return;
  sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (sort_cb)
    sort_cb(ih, col);
}

static void wasmTableUpdateFocus(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih);
  int select = !iupStrEqualNoCase(IupGetAttribute(ih, "SELECTIONMODE"), "NONE");
  int focusrect = iupAttribGetBoolean(ih, "FOCUSRECT");
  if (id && !iupAttribGetBoolean(ih, "VIRTUALMODE"))  /* virtual rows aren't lin-indexed in the DOM */
    iupwasmJsTableFocus(id, lin, col, select, focusrect);
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  int id = iupwasmIdOf(ih);
  (void)num_lin;
  if (!id) return;
  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    wasmTableVirtualRender(ih);
    return;
  }
  iupwasmJsTableBuild(id, ih->data->num_lin, ih->data->num_col);
  wasmTableApplyColors(ih);
}

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  int id = iupwasmIdOf(ih);
  (void)num_col;
  if (!id) return;
  iupwasmJsTableBuild(id, ih->data->num_lin, ih->data->num_col);
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  (void)pos;
  iupdrvTableSetNumLin(ih, ih->data->num_lin);
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  (void)pos;
  iupdrvTableSetNumLin(ih, ih->data->num_lin);
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  (void)pos;
  iupdrvTableSetNumCol(ih, ih->data->num_col);
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  (void)pos;
  iupdrvTableSetNumCol(ih, ih->data->num_col);
}

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableSetCell(id, lin, col, value ? value : "");
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih);
  char* ptr;
  char* ret;
  if (!id)
    return NULL;
  ptr = (char*)(intptr_t)iupwasmJsTableGetCell(id, lin, col);
  if (!ptr)
    return NULL;
  ret = iupStrReturnStr(ptr);
  free(ptr);
  return ret;
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  int id = iupwasmIdOf(ih);
  void* handle;
  if (!id)
    return;
  handle = iupImageGetImage(image, ih, 0, NULL);
  if (handle)
    iupwasmJsTableSetCellImage(id, lin, col, (int)(intptr_t)handle);
}

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableColTitle(id, col, title ? title : "");
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  char name[32];
  snprintf(name, sizeof(name), "TITLE%d", col);
  return iupAttribGet(ih, name);
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableColWidth(id, col, width);
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  int id = iupwasmIdOf(ih);
  return id ? iupwasmJsTableColWidthGet(id, col) : 0;
}

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  iupAttribSetInt(ih, "_IUPWASM_FOCUSLIN", lin);
  iupAttribSetInt(ih, "_IUPWASM_FOCUSCOL", col);
  wasmTableUpdateFocus(ih, lin, col);
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  if (lin) *lin = iupAttribGetInt(ih, "_IUPWASM_FOCUSLIN");
  if (col) *col = iupAttribGetInt(ih, "_IUPWASM_FOCUSCOL");
  if (lin && *lin == 0) *lin = 1;
  if (col && *col == 0) *col = 1;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableScrollTo(id, lin, col);
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  wasmTableApplyColors(ih);
  wasmTableApplyCellColors(ih);
  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
    wasmTableVirtualRender(ih);
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableSetGrid(id, show);
}

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 1;
}

/* must match the rendered cell box: charheight + 4px padding + 1px border */
IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  int h = 0;
  iupdrvFontGetCharSize(ih, NULL, &h);
  return h + 5;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  int h = 0;
  iupdrvFontGetCharSize(ih, NULL, &h);
  return h + 5;  /* same box as a row */
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  if (w) *w += 2;
  if (h) *h += 2;
}

/* clamp height to VISIBLELINES rows so EXPAND doesn't stretch the table; extra space stays empty */
static void wasmTableLayoutUpdate(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int h = ih->currentheight;
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int content = iupdrvTableGetHeaderHeight(ih) + iupdrvTableGetRowHeight(ih) * visiblelines + 2;
    if (h > content)
      h = content;
  }
  if (id)
    iupwasmJsSetPos(id, ih->x, ih->y, ih->currentwidth, h);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableCellClick(int id, int lin, int col)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniis click_cb;
  IFnii enter_cb;
  if (!ih)
    return;

  iupdrvTableSetFocusCell(ih, lin, col);

  enter_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (enter_cb)
    enter_cb(ih, lin, col);

  click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (click_cb && click_cb(ih, lin, col, (char*)"") == IUP_CLOSE)
    IupExitLoop();
}

EMSCRIPTEN_KEEPALIVE int iupwasmTableEditBegin(int id, int lin, int col)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnii cb;
  if (!ih)
    return 0;
  if (!iupAttribGetIntId(ih, "EDITABLE", col) && !iupAttribGetBoolean(ih, "EDITABLE"))
    return 0;
  cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (cb && cb(ih, lin, col) == IUP_IGNORE)
    return 0;
  return 1;
}

EMSCRIPTEN_KEEPALIVE int iupwasmTableEditEnd(int id, int lin, int col, const char* text, int apply)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniisi editend_cb;
  IFniis edit_cb;
  IFnii vc_cb;
  if (!ih)
    return 0;

  editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb && editend_cb(ih, lin, col, (char*)text, apply) == IUP_IGNORE)
    return 0;

  if (!apply)
    return 0;

  edit_cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
  if (edit_cb && edit_cb(ih, lin, col, (char*)text) == IUP_IGNORE)
    return 0;

  iupdrvTableSetCellValue(ih, lin, col, text);

  vc_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vc_cb)
    vc_cb(ih, lin, col);
  return 1;
}

EMSCRIPTEN_KEEPALIVE int iupwasmTableReorder(int id, int oldCol, int newCol)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnii cb;
  if (!ih || !ih->data->allow_reorder)
    return 0;
  cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  if (cb && cb(ih, oldCol, newCol) == IUP_IGNORE)
    return 0;
  return 1;
}

EMSCRIPTEN_KEEPALIVE int iupwasmTableRowDragDrop(int id, int from, int before)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  int is_ctrl = 0;
  int to;

  if (!ih || !ih->data->show_dragdrop)
    return 0;

  if (iupTableCallDragDropCb(ih, from - 1, before - 1, &is_ctrl) != IUP_CONTINUE)
    return 0;

  to = (before > from) ? before - 1 : before;
  if (to > ih->data->num_lin) to = ih->data->num_lin;
  if (to < 1) to = 1;

  iupTableMoveLinAttribs(ih, from, to);
  iupwasmJsTableMoveRow(id, from, to);
  wasmTableApplyCellColors(ih);
  wasmTableUpdateFocus(ih, to, ih->data->num_col > 0 ? 1 : 0);
  return 1;
}

static int wasmTableMapMethod(Ihandle* ih)
{
  int id, col;

  id = iupwasmJsTableCreate();
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  iupwasmJsTableFeatures(id, ih->data->allow_reorder, ih->data->user_resize, ih->data->show_dragdrop);

  {
    int charheight = 0;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    iupwasmJsTableSetFitImage(id, ih->data->fit_image, charheight);
  }

  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    iupwasmJsTableBuild(id, 0, ih->data->num_col);  /* header only; rows are windowed on scroll */
    wasmTableApplyColors(ih);
    iupwasmJsTableVirtualInit(id);
  }
  else
    iupwasmJsTableBuild(id, ih->data->num_lin, ih->data->num_col);

  for (col = 1; col <= ih->data->num_col; col++)
  {
    char name[32];
    snprintf(name, sizeof(name), "TITLE%d", col);
    {
      char* t = iupAttribGet(ih, name);
      if (t)
        iupwasmJsTableColTitle(id, col, t);
    }
  }

  wasmTableApplyAlign(ih);
  wasmTableApplyCellColors(ih);

  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

static char* wasmTableGetFitImageAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->fit_image);
}

static int wasmTableSetFitImageAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->fit_image = iupStrBoolean(value);
  if (id)
  {
    int charheight = 0;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    iupwasmJsTableSetFitImage(id, ih->data->fit_image, charheight);
  }
  return 1;
}

static int wasmTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->allow_reorder = iupStrBoolean(value);
  if (id)
    iupwasmJsTableFeatures(id, ih->data->allow_reorder, ih->data->user_resize, ih->data->show_dragdrop);
  return 0;
}

static int wasmTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->user_resize = iupStrBoolean(value);
  if (id)
    iupwasmJsTableFeatures(id, ih->data->allow_reorder, ih->data->user_resize, ih->data->show_dragdrop);
  return 0;
}

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = wasmTableMapMethod;
  ic->LayoutUpdate = wasmTableLayoutUpdate;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "FITIMAGE", wasmTableGetFitImageAttrib, wasmTableSetFitImageAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, wasmTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, wasmTableSetUserResizeAttrib);
}
