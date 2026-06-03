/** \file
 * \brief WebAssembly List
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
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_drvfont.h"
#include "iup_list.h"

#include "iupwasm_drv.h"


#define IUPWASM_VLIST_ITEMH 20

static int wasmListIsVirtual(Ihandle* ih);

EM_JS(int, iupwasmJsCreateVList, (void), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createvlist', id: id });
  return id;
})

EM_JS(void, iupwasmJsVListInit, (int id, int itemH), {
  globalThis.__iupApply({ op: 'vlistinit', id: id, itemH: itemH });
})

EM_JS(void, iupwasmJsVListSetCount, (int id, int count), {
  globalThis.__iupApply({ op: 'vlistcount', id: id, count: count });
})

EM_JS(void, iupwasmJsVListSetFit, (int id, int fit), {
  globalThis.__iupApply({ op: 'vlistfit', id: id, fit: fit });
})

EM_JS(int, iupwasmJsVListScrollTop, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'vlistscrolltop', id: id });
  var w = globalThis.__iup.els[id]; return w ? (w.scrollTop | 0) : 0;
})
EM_JS(int, iupwasmJsVListClientH, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'vlistclienth', id: id });
  var w = globalThis.__iup.els[id]; return w ? (w.clientHeight | 0) : 0;
})

EM_JS(void, iupwasmJsVListWindow, (int id, int count), {
  globalThis.__iupApply({ op: 'vlistwindow', id: id, count: count });
})

EM_JS(void, iupwasmJsVListCell, (int id, int rowIdx, int pos, const char* text, int imgId), {
  globalThis.__iupApply({ op: 'vlistcell', id: id, rowIdx: rowIdx, pos: pos, text: UTF8ToString(text), imgId: imgId });
})

EM_JS(void, iupwasmJsVListSelect, (int id, int pos), {
  globalThis.__iupApply({ op: 'vlistselect', id: id, pos: pos });
})

EM_JS(int, iupwasmJsCreateList, (int dropdown, int editbox, int multiple, int size), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createlist', id: id, dropdown: dropdown, editbox: editbox, multiple: multiple, size: size });
  return id;
})

EM_JS(void, iupwasmJsListAppend, (int id, const char* text), {
  globalThis.__iupApply({ op: 'listappend', id: id, text: UTF8ToString(text) });
})

EM_JS(void, iupwasmJsListInsert, (int id, int pos, const char* text), {
  globalThis.__iupApply({ op: 'listinsert', id: id, pos: pos, text: UTF8ToString(text) });
})

EM_JS(void, iupwasmJsListRemove, (int id, int pos), {
  globalThis.__iupApply({ op: 'listremove', id: id, pos: pos });
})

EM_JS(void, iupwasmJsListRemoveAll, (int id), {
  globalThis.__iupApply({ op: 'listremoveall', id: id });
})

EM_JS(int, iupwasmJsListCount, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'listcount', id: id });
  var el = globalThis.__iup.els[id];
  return el ? el.__iupOpts.children.length : 0;
})

EM_JS(void, iupwasmJsListSetSelIndex, (int id, int idx), {
  globalThis.__iupApply({ op: 'listselindex', id: id, idx: idx });
})

EM_JS(int, iupwasmJsListGetSelIndex, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'listselindex', id: id });
  var el = globalThis.__iup.els[id];
  return (el && el.__iupOpts.selectedIndex !== undefined) ? el.__iupOpts.selectedIndex : -1;
})

EM_JS(void, iupwasmJsListSetMulti, (int id, const char* str), {
  globalThis.__iupApply({ op: 'listsetmulti', id: id, str: UTF8ToString(str) });
})

EM_JS(char*, iupwasmJsListGetMulti, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'listmulti', id: id });
  else { var el = globalThis.__iup.els[id]; s = ""; if (el) { var opts = el.__iupOpts.options; for (var i = 0; i < opts.length; i++) s += opts[i].selected ? '+' : '-'; } }
  var len = lengthBytesUTF8(s) + 1; var p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsListSetText, (int id, const char* str), {
  globalThis.__iupApply({ op: 'listsettext', id: id, str: UTF8ToString(str) });
})

EM_JS(char*, iupwasmJsListGetText, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'listtext', id: id });
  else { var el = globalThis.__iup.els[id]; s = el ? el.__iupVal.value : ""; }
  var len = lengthBytesUTF8(s) + 1; var p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(char*, iupwasmJsListItemText, (int id, int pos), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'listitemtext', id: id, pos: pos });
  else { var el = globalThis.__iup.els[id]; s = (el && el.__iupOpts.children[pos]) ? el.__iupOpts.children[pos].text : ""; }
  var len = lengthBytesUTF8(s) + 1; var p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsListWire, (int id, int editbox, int multiple, int dropdown), {
  globalThis.__iupApply({ op: 'listwire', id: id, editbox: editbox, multiple: multiple, dropdown: dropdown });
})

EM_JS(void, iupwasmJsListSetItemImage, (int id, int pos, int imgId, int w, int h), {
  globalThis.__iupApply({ op: 'listitemimage', id: id, pos: pos, imgId: imgId, w: w, h: h });
})
EM_JS(int, iupwasmJsListGetItemImage, (int id, int pos), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'listitemimg', id: id, pos: pos });
  var el = globalThis.__iup.els[id]; var opt = el && el.__iupOpts.children[pos];
  return opt ? (opt.__iupImgId || 0) : 0;
})
EM_JS(void, iupwasmJsListTopItem, (int id, int pos), { globalThis.__iupApply({ op: 'listtopitem', id: id, pos: pos }); })
EM_JS(void, iupwasmJsVListSetTop, (int id, int top), { globalThis.__iupApply({ op: 'vlistsettop', id: id, top: top }); })
EM_JS(void, iupwasmJsListSpacing, (int id, int s), { globalThis.__iupApply({ op: 'listspacing', id: id, s: s }); })
EM_JS(void, iupwasmJsListPadding, (int id, int h, int v), { globalThis.__iupApply({ op: 'listpadding', id: id, h: h, v: v }); })
EM_JS(void, iupwasmJsListShowDropdown, (int id, int on), { globalThis.__iupApply({ op: 'listshowdropdown', id: id, on: on }); })
EM_JS(void, iupwasmJsListDropExpand, (int id, int on), { globalThis.__iupApply({ op: 'listdropexpand', id: id, on: on }); })
EM_JS(void, iupwasmJsListDragDrop, (int id), { globalThis.__iupApply({ op: 'listdragdrop', id: id }); })
EM_JS(void, iupwasmJsListDndSource, (int id, int on), { globalThis.__iupApply({ op: 'listdndsrc', id: id, on: on }); })
EM_JS(void, iupwasmJsListDndTarget, (int id, int on), { globalThis.__iupApply({ op: 'listdndtgt', id: id, on: on }); })

EM_JS(int, iupwasmJsListXY2Pos, (int id, int y), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'listxy2pos', id: id, y: y });
  var el = globalThis.__iup.els[id], sel = el && el.__iupOpts;
  if (!sel || !sel.options || !sel.options.length) return 0;
  var vy = sel.getBoundingClientRect().top + y;
  for (var i = 0; i < sel.options.length; i++)
    if (vy <= sel.options[i].getBoundingClientRect().bottom) return i + 1;
  return sel.options.length;
})

static int wasmListConvertXYToPos(Ihandle* ih, int x, int y)
{
  int id = iupwasmIdOf(ih);
  (void)x;
  return id ? iupwasmJsListXY2Pos(id, y) : -1;
}

EMSCRIPTEN_KEEPALIVE void iupwasmListReorder(int id, int idDrag, int idDrop)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  int is_ctrl = 0;
  if (!ih)
    return;

  if (iupListCallDragDropCb(ih, idDrag, idDrop, &is_ctrl) == IUP_CONTINUE)
  {
    int count = iupdrvListGetCount(ih);
    char* src = IupGetAttributeId(ih, "", idDrag + 1);
    char* text = iupStrDup(src ? src : "");

    if (idDrop >= 0 && idDrop < count)
    {
      iupdrvListInsertItem(ih, idDrop, text);
      if (idDrag > idDrop)
        idDrag++;
    }
    else
    {
      iupdrvListAppendItem(ih, text);
      idDrop = count;
    }

    IupSetInt(ih, "VALUE", idDrop + 1);

    if (!is_ctrl)
      iupdrvListRemoveItem(ih, idDrag);

    free(text);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmDndTransfer(int srcId, int tgtId, int srcY, int tgtY)
{
  Ihandle* src = iupwasmHandleFromId(srcId);
  Ihandle* tgt = iupwasmHandleFromId(tgtId);
  IFnii beginCb;
  IFns sizeCb;
  IFnsVi dataCb;
  IFnsViii dropCb;
  IFni endCb;
  char* type;
  int size;
  void* buf;

  if (!src || !tgt)
    return;

  beginCb = (IFnii)IupGetCallback(src, "DRAGBEGIN_CB");
  if (beginCb && beginCb(src, 0, srcY) == IUP_IGNORE)
    return;

  sizeCb = (IFns)IupGetCallback(src, "DRAGDATASIZE_CB");
  dataCb = (IFnsVi)IupGetCallback(src, "DRAGDATA_CB");
  type = iupAttribGet(src, "DRAGTYPES");
  size = sizeCb ? sizeCb(src, type ? type : (char*)"") : 0;
  if (size <= 0 || !dataCb)
    return;

  buf = malloc(size);
  dataCb(src, type ? type : (char*)"", buf, size);

  dropCb = (IFnsViii)IupGetCallback(tgt, "DROPDATA_CB");
  if (dropCb)
  {
    char* dtype = iupAttribGet(tgt, "DROPTYPES");
    dropCb(tgt, dtype ? dtype : (char*)"", buf, size, 0, tgtY);
  }

  endCb = (IFni)IupGetCallback(src, "DRAGEND_CB");
  if (endCb)
    endCb(src, 0);

  free(buf);
}

EM_JS(void, iupwasmJsListEditReadOnly, (int id, int ro), { globalThis.__iupApply({ op: 'listeditreadonly', id: id, ro: ro }); })
EM_JS(void, iupwasmJsListEditMaxLen, (int id, int n), { globalThis.__iupApply({ op: 'listeditmaxlen', id: id, n: n }); })
EM_JS(void, iupwasmJsListEditAppend, (int id, const char* v), { globalThis.__iupApply({ op: 'listeditappend', id: id, value: UTF8ToString(v) }); })
EM_JS(void, iupwasmJsListEditInsert, (int id, const char* v), { globalThis.__iupApply({ op: 'listeditinsert', id: id, value: UTF8ToString(v) }); })
EM_JS(void, iupwasmJsListEditClipboard, (int id, const char* action), { globalThis.__iupApply({ op: 'listeditclipboard', id: id, action: UTF8ToString(action) }); })
EM_JS(void, iupwasmJsListEditSetCaret, (int id, int pos), { globalThis.__iupApply({ op: 'listeditcaret', id: id, pos: pos }); })
EM_JS(void, iupwasmJsListEditSetSel, (int id, int s, int e), { globalThis.__iupApply({ op: 'listeditselpos', id: id, s: s, e: e }); })
EM_JS(int, iupwasmJsListEditGetCaret, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'listeditcaret', id: id });
  var el = globalThis.__iup.els[id]; return (el && el.__iupVal) ? (el.__iupVal.selectionStart || 0) : 0;
})
EM_JS(char*, iupwasmJsListEditGetSel, (int id), {
  var s = (typeof document === 'undefined') ? globalThis.__iupReadSync({ op: 'listeditselpos', id: id }) : "";
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})
EM_JS(char*, iupwasmJsListEditGetSelText, (int id), {
  var s = (typeof document === 'undefined') ? globalThis.__iupReadSync({ op: 'listeditseltext', id: id }) : "";
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchListDropdown(int id, int state)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFni cb;
  if (!ih)
    return;
  cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb)
    cb(ih, state);
}

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchListEdit(int id, int c, const char* after)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnis cb;
  if (!ih)
    return IUP_DEFAULT;
  cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  if (cb)
    return cb(ih, c, (char*)after);
  return IUP_DEFAULT;
}

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchListAction(int id, int item, int state, const char* text)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnsii cb;
  if (!ih)
    return IUP_DEFAULT;
  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
    return cb(ih, (char*)text, item, state);
  return IUP_DEFAULT;
}

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchListDblclick(int id, int item, const char* text)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnis cb;
  if (!ih)
    return IUP_DEFAULT;
  cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
  if (cb)
    return cb(ih, item, (char*)text);
  return IUP_DEFAULT;
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchListCaret(int id, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniii cb;
  if (!ih)
    return;
  cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb)
    return;
  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, 1, pos+1, pos);
  }
}

/* sel is the full "+/-" string (one char per item); rebuild the selected-position
   array and route through the core, which fires MULTISELECT_CB or falls back to ACTION. */
EMSCRIPTEN_KEEPALIVE void iupwasmDispatchListMulti(int id, const char* sel)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFns multi_cb;
  IFnsii cb;
  int i, len, sel_count, *pos;
  if (!ih || !sel)
    return;
  multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (!multi_cb && !cb)
    return;

  len = (int)strlen(sel);
  pos = (int*)malloc(sizeof(int)*(len?len:1));
  if (!pos)
    return;
  sel_count = 0;
  for (i=0; i<len; i++)
    if (sel[i] == '+')
      pos[sel_count++] = i;

  iupListMultipleCallActionCb(ih, cb, multi_cb, pos, sel_count);
  free(pos);
}

static int wasmListSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!id)
    return 0;
  if (wasmListIsVirtual(ih))
  {
    int pos = 0;
    iupStrToInt(value, &pos);
    iupAttribSetInt(ih, "_IUPWASM_VLISTSEL", pos);
    iupwasmJsVListSelect(id, pos);
    return 0;
  }
  if (ih->data->has_editbox)
    iupwasmJsListSetText(id, value ? value : "");
  else if (ih->data->is_multiple)
    iupwasmJsListSetMulti(id, value ? value : "");
  else
  {
    int pos = 0;
    iupStrToInt(value, &pos);
    iupwasmJsListSetSelIndex(id, pos - 1);
  }
  return 0;
}

static char* wasmListGetValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (!id)
    return NULL;
  if (wasmListIsVirtual(ih))
    return iupStrReturnInt(iupAttribGetInt(ih, "_IUPWASM_VLISTSEL"));
  if (ih->data->has_editbox)
  {
    char* p = iupwasmJsListGetText(id);
    char* r = iupStrReturnStr(p);
    free(p);
    return r;
  }
  if (ih->data->is_multiple)
  {
    char* p = iupwasmJsListGetMulti(id);
    char* r = iupStrReturnStr(p);
    free(p);
    return r;
  }
  return iupStrReturnInt(iupwasmJsListGetSelIndex(id) + 1);
}

static char* wasmListGetIdValueAttrib(Ihandle* ih, int id)
{
  int handleId = iupwasmIdOf(ih);
  int pos = iupListGetPosAttrib(ih, id);
  char* p, *r;
  if (!handleId || pos < 0)
    return NULL;
  p = iupwasmJsListItemText(handleId, pos);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmListIsVirtual(Ihandle* ih)
{
  return ih->data->is_virtual && !ih->data->is_dropdown && !ih->data->has_editbox;
}

static void wasmListVirtualRender(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int itemH, count, first, n, i, showimage;
  sIFni value_cb, image_cb;

  if (!id || !wasmListIsVirtual(ih))
    return;

  itemH = IUPWASM_VLIST_ITEMH;
  count = iupListGetItemCount(ih);
  first = iupwasmJsVListScrollTop(id) / itemH - 2;
  if (first < 0) first = 0;
  n = iupwasmJsVListClientH(id) / itemH + 5;
  if (n > 500) n = 500;  /* cap the windowed row request */
  if (first + n > count) n = count - first;
  if (n < 0) n = 0;

  iupwasmJsVListWindow(id, n);
  value_cb = (sIFni)IupGetCallback(ih, "VALUE_CB");
  image_cb = (sIFni)IupGetCallback(ih, "IMAGE_CB");
  showimage = ih->data->show_image;

  for (i = 0; i < n; i++)
  {
    int pos = first + i + 1;
    int imgId = 0;
    char* text;
    if (showimage && image_cb)
    {
      char* name = image_cb(ih, pos);
      if (name && name[0])
        imgId = (int)(intptr_t)iupImageGetImage(name, ih, 0, NULL);
    }
    text = value_cb ? value_cb(ih, pos) : NULL;
    iupwasmJsVListCell(id, i, pos, text ? text : "", imgId);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmListVScroll(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  if (ih)
    wasmListVirtualRender(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmListVClick(int id, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnsii action_cb;
  sIFni value_cb;
  char* text;
  if (!ih)
    return;
  iupwasmJsVListSelect(id, pos);
  iupAttribSetInt(ih, "_IUPWASM_VLISTSEL", pos);
  value_cb = (sIFni)IupGetCallback(ih, "VALUE_CB");
  text = value_cb ? value_cb(ih, pos) : NULL;
  action_cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (action_cb && action_cb(ih, text ? text : (char*)"", pos, 1) == IUP_CLOSE)
    IupExitLoop();
}

static int wasmListMapMethod(Ihandle* ih)
{
  int size = iupAttribGetInt(ih, "VISIBLELINES");
  int id;

  if (wasmListIsVirtual(ih))
  {
    id = iupwasmJsCreateVList();
    ih->handle = (InativeHandle*)(intptr_t)id;
    iupwasmRegisterHandle(id, ih);
    iupwasmJsVListInit(id, IUPWASM_VLIST_ITEMH);
    iupwasmJsVListSetFit(id, ih->data->fit_image);
    iupwasmAddToParent(ih);
    iupwasmJsVListSetCount(id, iupListGetItemCount(ih));  /* ITEMCOUNT setter returns 0 (not replayed), seed it here */
    wasmListVirtualRender(ih);
    return IUP_NOERROR;
  }

  id = iupwasmJsCreateList(ih->data->is_dropdown, ih->data->has_editbox, ih->data->is_multiple, size);
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmJsListWire(id, ih->data->has_editbox, ih->data->is_multiple, ih->data->is_dropdown);
  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)wasmListConvertXYToPos);
  if (ih->data->show_dragdrop)
    iupwasmJsListDragDrop(id);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
{
  return iupwasmJsListCount(iupwasmIdOf(ih));
}

IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsListAppend(id, value ? value : "");
}

IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsListInsert(id, pos, value ? value : "");
}

IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsListRemove(id, pos);
}

IUP_SDK_API void iupdrvListRemoveAllItems(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsListRemoveAll(id);
}

/* pos is 0-based */
static void wasmListItemImageHandle(Ihandle* ih, int pos, void* img)
{
  int handleId = iupwasmIdOf(ih);
  int w = 0, h = 0, ch = 0;
  if (!handleId)
    return;
  if (img)
  {
    int avail;
    iupdrvImageGetInfo(img, &w, &h, NULL);
    iupdrvFontGetCharSize(ih, NULL, &ch);
    avail = ch + 2 * ih->data->spacing;
    if (ih->data->fit_image && h > avail) { w = (w * avail) / h; h = avail; }
  }
  iupwasmJsListSetItemImage(handleId, pos, img ? (int)(intptr_t)img : 0, w, h);
}

IUP_SDK_API void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  int handleId = iupwasmIdOf(ih);
  if (!handleId)
    return NULL;
  return (void*)(intptr_t)iupwasmJsListGetItemImage(handleId, id - 1);
}

IUP_SDK_API int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  wasmListItemImageHandle(ih, id, hImage);
  return 0;
}

IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  int id = iupwasmIdOf(ih);
  if (!id || !wasmListIsVirtual(ih))
    return;
  iupwasmJsVListSetCount(id, count);
  wasmListVirtualRender(ih);
}

IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  if (w) *w += 4;
  if (h) *h += 2;
}

IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  (void)ih;
  if (h) *h += 1;  /* match the rendered option box so VISIBLELINES is exact */
}

static char* wasmListGetFitImageAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->fit_image);
}

static int wasmListSetFitImageAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->fit_image = iupStrBoolean(value);
  if (id && wasmListIsVirtual(ih))
    iupwasmJsVListSetFit(id, ih->data->fit_image);
  return 1;
}

static int wasmListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->has_editbox)
    iupwasmJsListEditReadOnly(id, iupStrBoolean(value));
  return 1;
}

static int wasmListSetNCAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), nc = 0;
  iupStrToInt(value, &nc);
  if (id && ih->data->has_editbox)
    iupwasmJsListEditMaxLen(id, nc);
  return 1;
}

static int wasmListSetAppendAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->has_editbox && value)
    iupwasmJsListEditAppend(id, value);
  return 0;
}

static int wasmListSetInsertAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->has_editbox && value)
    iupwasmJsListEditInsert(id, value);
  return 0;
}

static int wasmListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->has_editbox && value)
    iupwasmJsListEditClipboard(id, value);
  return 0;
}

static char* wasmListGetCaretPosAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (!id || !ih->data->has_editbox) return NULL;
  return iupStrReturnInt(iupwasmJsListEditGetCaret(id));
}

static int wasmListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 0;
  if (id && ih->data->has_editbox && iupStrToInt(value, &pos))
    iupwasmJsListEditSetCaret(id, pos);
  return 0;
}

static char* wasmListGetSelectionPosAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id || !ih->data->has_editbox) return NULL;
  p = (char*)(intptr_t)iupwasmJsListEditGetSel(id);
  if (!p || !*p) { free(p); return NULL; }
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), s = 0, e = 0;
  if (!id || !ih->data->has_editbox || !value) return 0;
  if (iupStrEqualNoCase(value, "NONE")) { iupwasmJsListEditSetSel(id, 0, 0); return 0; }
  if (iupStrEqualNoCase(value, "ALL")) { iupwasmJsListEditSetSel(id, 0, 1000000); return 0; }
  iupStrToIntInt(value, &s, &e, ':');
  iupwasmJsListEditSetSel(id, s, e);
  return 0;
}

static char* wasmListGetSelectedTextAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id || !ih->data->has_editbox) return NULL;
  p = (char*)(intptr_t)iupwasmJsListEditGetSelText(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (!ih->data->show_image || pos < 0)
    return 0;
  wasmListItemImageHandle(ih, pos, value ? iupImageGetImage(value, ih, 0, NULL) : NULL);
  return 0;
}

static char* wasmListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  return iupdrvListGetImageHandle(ih, id);
}

static int wasmListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 1;
  if (id && !ih->data->is_dropdown && iupStrToInt(value, &pos))
  {
    if (wasmListIsVirtual(ih))
      iupwasmJsVListSetTop(id, (pos - 1) * IUPWASM_VLIST_ITEMH);
    else
      iupwasmJsListTopItem(id, pos - 1);
  }
  return 0;
}

static int wasmListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (ih->data->is_dropdown)
    return 0;
  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;
  if (id) { iupwasmJsListSpacing(id, ih->data->spacing); return 0; }
  return 1;
}

static int wasmListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!ih->data->has_editbox)
    return 0;
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (id) { iupwasmJsListPadding(id, ih->data->horiz_padding, ih->data->vert_padding); return 0; }
  return 1;
}

/* native <select> cannot be opened from script (browser limit); only the custom editbox+dropdown popup honors it */
static int wasmListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->is_dropdown)
    iupwasmJsListShowDropdown(id, iupStrBoolean(value));
  return 0;
}

static int wasmListSetDropExpandAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->is_dropdown)
    iupwasmJsListDropExpand(id, iupStrBoolean(value));
  return 1;
}

/* CARET/SELECTION are the 1-based editbox aliases of CARETPOS/SELECTIONPOS */
static char* wasmListGetCaretAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (!id || !ih->data->has_editbox) return NULL;
  return iupStrReturnInt(iupwasmJsListEditGetCaret(id) + 1);
}

static int wasmListSetCaretAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 1;
  if (id && ih->data->has_editbox && iupStrToInt(value, &pos))
    iupwasmJsListEditSetCaret(id, pos > 0 ? pos - 1 : 0);
  return 0;
}

static char* wasmListGetSelectionAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih), s = 0, e = 0;
  char* p;
  if (!id || !ih->data->has_editbox) return NULL;
  p = (char*)(intptr_t)iupwasmJsListEditGetSel(id);
  if (!p || !*p) { free(p); return NULL; }
  iupStrToIntInt(p, &s, &e, ':');
  free(p);
  return iupStrReturnStrf("%d:%d", s + 1, e);
}

static int wasmListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), s = 1, e = 1;
  if (!id || !ih->data->has_editbox || !value) return 0;
  if (iupStrEqualNoCase(value, "NONE")) { iupwasmJsListEditSetSel(id, 0, 0); return 0; }
  if (iupStrEqualNoCase(value, "ALL")) { iupwasmJsListEditSetSel(id, 0, 1000000); return 0; }
  iupStrToIntInt(value, &s, &e, ':');
  iupwasmJsListEditSetSel(id, s > 0 ? s - 1 : 0, e);
  return 0;
}

static int wasmListSetScrollToPosEditAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 0;
  if (id && ih->data->has_editbox && iupStrToInt(value, &pos))
    iupwasmJsListEditSetCaret(id, pos);  /* moving the caret scrolls the input into view */
  return 0;
}

IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = wasmListMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "READONLY", NULL, wasmListSetReadOnlyAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NC", NULL, wasmListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, wasmListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, wasmListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, wasmListSetClipboardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", wasmListGetCaretPosAttrib, wasmListSetCaretPosAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", wasmListGetSelectionPosAttrib, wasmListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", wasmListGetSelectedTextAttrib, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITIMAGE", wasmListGetFitImageAttrib, wasmListSetFitImageAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, wasmListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, wasmListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, wasmListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, wasmListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, wasmListSetDropExpandAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", wasmListGetCaretAttrib, wasmListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", wasmListGetSelectionAttrib, wasmListSetSelectionAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, wasmListSetScrollToPosEditAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, wasmListSetScrollToPosEditAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_SUPPORTED);  /* native <select> popup item count is UA-controlled */
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);  /* DOM always repaints */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, wasmListSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", wasmListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IDVALUE", wasmListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", wasmListGetValueAttrib, wasmListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
}
