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
#include "iup_drv.h"
#include "iup_list.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsCreateVList, (void), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createvlist', id: id });
  return id;
})

EM_JS(void, iupwasmJsVListInit, (int id, int itemH), {
  globalThis.__iupApply({ op: 'vlistinit', id: id, itemH: itemH });
})

EM_JS(void, iupwasmJsVListRowHeight, (int id, int itemH), {
  globalThis.__iupApply({ op: 'vlistrowh', id: id, itemH: itemH });
})

EM_JS(void, iupwasmJsVListSetCount, (int id, int count), {
  globalThis.__iupApply({ op: 'vlistcount', id: id, count: count });
})

EM_JS(void, iupwasmJsVListSetFit, (int id, int fit), {
  globalThis.__iupApply({ op: 'vlistfit', id: id, fit: fit });
})

EM_JS(void, iupwasmJsVListRowsBegin, (void), {
  globalThis.__iupVListRows = { texts: [], imgs: [] };
})

EM_JS(void, iupwasmJsVListRowsAdd, (const char* text, int imgId), {
  globalThis.__iupVListRows.texts.push(UTF8ToString(text)); globalThis.__iupVListRows.imgs.push(imgId);
})

EM_JS(void, iupwasmJsVListRowsEnd, (int id, int gen, int from), {
  var r = globalThis.__iupVListRows; globalThis.__iupVListRows = null;
  globalThis.__iupApply({ op: 'vlistrows', id: id, gen: gen, from: from, texts: r.texts, imgs: r.imgs });
})

EM_JS(void, iupwasmJsVListMarksBegin, (void), {
  globalThis.__iupVListMarks = [];
})

EM_JS(void, iupwasmJsVListMarksAdd, (int first, int last), {
  globalThis.__iupVListMarks.push(first, last);
})

EM_JS(void, iupwasmJsVListMarksEnd, (int id, int focus, int show), {
  var r = globalThis.__iupVListMarks; globalThis.__iupVListMarks = null;
  globalThis.__iupApply({ op: 'vlistmarks', id: id, ranges: r, focus: focus, show: show });
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
EM_JS(void, iupwasmJsVListSetTop, (int id, int pos), { globalThis.__iupApply({ op: 'vlistsettop', id: id, pos: pos }); })
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
  var vy = el.getBoundingClientRect().top + y;
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

EMSCRIPTEN_KEEPALIVE void iupwasmDndTransfer(int srcId, int tgtId, int srcY, int tgtX, int tgtY)
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
    dropCb(tgt, dtype ? dtype : (char*)"", buf, size, tgtX, tgtY);
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
EM_JS(void, iupwasmJsListEditSetSelText, (int id, const char* v), { globalThis.__iupApply({ op: 'listeditsetseltext', id: id, value: UTF8ToString(v) }); })
EM_JS(int, iupwasmJsListEditGetCaret, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'listeditcaret', id: id });
  var el = globalThis.__iup.els[id]; return (el && el.__iupVal) ? globalThis.__iupU2C(el.__iupVal.value, el.__iupVal.selectionStart || 0) : 0;
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

EM_JS(void, iupwasmJsListEditDone, (int id, int fix, const char* value, int caret), {
  globalThis.__iupApply({ op: 'texteditdone', id: id, fix: fix, value: UTF8ToString(value), caret: caret });
})

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchListEdit(int id, const char* oldvalue, const char* insert,
                                                  int start, int end, int remove_dir, int keyseq)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnis cb;
  int ret;

  if (!ih)
    return;

  if (iupwasmKeyIgnored(keyseq))
  {
    iupwasmJsListEditDone(id, 1, oldvalue ? oldvalue : "", start);
    return;
  }

  iupAttribSetStr(ih, "_IUPWASM_EDITOLD", oldvalue ? oldvalue : "");
  cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  ret = iupEditCallActionCb(ih, cb, (insert && insert[0]) ? insert : NULL, start, end,
                            ih->data->mask, ih->data->nc, remove_dir, 1);
  if (!iupObjectCheck(ih))
    return;
  iupAttribSet(ih, "_IUPWASM_EDITOLD", NULL);

  if (ret == 0)
  {
    iupwasmJsListEditDone(id, 1, oldvalue ? oldvalue : "", start);
    return;
  }

  if (ret > 0)
  {
    char rep[2];
    char* fixed;
    rep[0] = (char)ret;
    rep[1] = 0;
    fixed = iupStrInsert(oldvalue ? oldvalue : "", rep, start, end, 1);
    iupwasmJsListEditDone(id, 1, fixed ? fixed : "", start + 1);
    if (fixed && fixed != oldvalue)
      free(fixed);
  }
  else
    iupwasmJsListEditDone(id, 0, "", 0);

  iupBaseCallValueChangedCb(ih);
}

/* the browser moves a <select> before the model sees the key, so a vetoed key is undone here */
static int wasmListKeyVetoed(Ihandle* ih, int id, int seq)
{
  if (!iupwasmKeyIgnored(seq))
    return 0;

  if (ih->data->is_multiple)
    iupwasmJsListSetMulti(id, iupAttribGet(ih, "_IUPWASM_LISTSEL"));
  else
    iupwasmJsListSetSelIndex(id, iupAttribGetInt(ih, "_IUPWASM_LISTSEL") - 1);

  return 1;
}

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchListAction(int id, int item, int state, const char* text, int seq)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnsii cb;
  if (!ih)
    return IUP_DEFAULT;
  if (wasmListKeyVetoed(ih, id, seq))
    return IUP_DEFAULT;
  iupAttribSetInt(ih, "_IUPWASM_LISTSEL", item);
  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb && !ih->data->is_multiple && state == 1)
  {
    iupListSingleCallActionCb(ih, cb, item);
    return IUP_DEFAULT;
  }
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

/* sel is the full "+/-" string, one char per item */
EMSCRIPTEN_KEEPALIVE void iupwasmDispatchListMulti(int id, const char* sel, int seq)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFns multi_cb;
  IFnsii cb;
  int i, len, sel_count, *pos;
  if (!ih || !sel)
    return;
  if (wasmListKeyVetoed(ih, id, seq))
    return;
  iupAttribSetStr(ih, "_IUPWASM_LISTSEL", (char*)sel);
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

static int wasmListIsVirtual(Ihandle* ih)
{
  return ih->data->is_virtual && !ih->data->is_dropdown && !ih->data->has_editbox;
}

static int wasmVListRowHeight(Ihandle* ih)
{
  int h = 0;
  iupdrvFontGetCharSize(ih, NULL, &h);
  if (ih->data->show_image)
  {
    int avail = h + 2 * ih->data->spacing, img_w = 16, img_h = 16;
    char* name = iupListGetItemImageCb(ih, 1);
    if (name)
    {
      iupImageGetInfo(name, &img_w, &img_h, NULL);
      if (ih->data->fit_image && img_h > avail)
        img_h = avail;
    }
    if (img_h > h)
      h = img_h;
  }
  iupdrvListAddItemSpace(ih, &h);
  return h + 2 * ih->data->spacing;
}

static unsigned char* wasmVListMarks(Ihandle* ih)
{
  unsigned char* marks = (unsigned char*)iupAttribGet(ih, "_IUPWASM_VLISTMARKS");
  int n = iupAttribGetInt(ih, "_IUPWASM_VLISTMARKN");
  int count = iupListGetItemCount(ih);
  if (!marks || n != count)
  {
    unsigned char* m = (unsigned char*)realloc(marks, count > 0 ? count : 1);
    if (!m)
      return NULL;
    if (count > n)
      memset(m + n, 0, count - n);
    iupAttribSet(ih, "_IUPWASM_VLISTMARKS", (char*)m);
    iupAttribSetInt(ih, "_IUPWASM_VLISTMARKN", count);
    marks = m;
  }
  return marks;
}

static void wasmVListSync(Ihandle* ih, int show)
{
  int id = iupwasmIdOf(ih);
  if (!id)
    return;
  iupwasmJsVListMarksBegin();
  if (ih->data->is_multiple)
  {
    unsigned char* marks = wasmVListMarks(ih);
    int count = iupListGetItemCount(ih), i = 0;
    while (marks && i < count)
    {
      int first;
      while (i < count && !marks[i])
        i++;
      if (i >= count)
        break;
      first = i;
      while (i < count && marks[i])
        i++;
      iupwasmJsVListMarksAdd(first + 1, i);
    }
  }
  else
  {
    int sel = iupAttribGetInt(ih, "_IUPWASM_VLISTSEL");
    if (sel > 0)
      iupwasmJsVListMarksAdd(sel, sel);
  }
  iupwasmJsVListMarksEnd(id, iupAttribGetInt(ih, "_IUPWASM_VLISTFOCUS"), show);
}

static void wasmVListNotify(Ihandle* ih)
{
  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (ih->data->is_multiple)
  {
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    unsigned char* marks = wasmVListMarks(ih);
    int count = iupListGetItemCount(ih), i, n = 0;
    int* pos;
    if ((!cb && !multi_cb) || !marks)
      return;
    pos = (int*)malloc(sizeof(int) * (count > 0 ? count : 1));
    if (!pos)
      return;
    for (i = 0; i < count; i++)
      if (marks[i])
        pos[n++] = i;
    iupListMultipleCallActionCb(ih, cb, multi_cb, pos, n);
    free(pos);
  }
  else
  {
    int sel = iupAttribGetInt(ih, "_IUPWASM_VLISTSEL");
    if (cb && sel > 0)
      iupListSingleCallActionCb(ih, cb, sel);
  }
}

static char* wasmVListGetValue(Ihandle* ih)
{
  if (ih->data->is_multiple)
  {
    unsigned char* marks = wasmVListMarks(ih);
    int count = iupListGetItemCount(ih), i;
    char* str;
    if (!marks)
      return NULL;
    str = iupStrGetMemory(count + 1);
    for (i = 0; i < count; i++)
      str[i] = marks[i] ? '+' : '-';
    str[count] = 0;
    return str;
  }
  return iupStrReturnInt(iupAttribGetInt(ih, "_IUPWASM_VLISTSEL"));
}

static void wasmVListSetValue(Ihandle* ih, const char* value)
{
  if (ih->data->is_multiple)
  {
    unsigned char* marks = wasmVListMarks(ih);
    int count = iupListGetItemCount(ih), i, len = value ? (int)strlen(value) : 0;
    if (!marks)
      return;
    for (i = 0; i < count; i++)
      marks[i] = (i < len && value[i] == '+') ? 1 : 0;
    iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", wasmVListGetValue(ih));
  }
  else
  {
    int pos = 0;
    iupStrToInt(value, &pos);
    if (pos < 0 || pos > iupListGetItemCount(ih))
      pos = 0;
    iupAttribSetInt(ih, "_IUPWASM_VLISTSEL", pos);
    if (pos > 0)
    {
      iupAttribSetInt(ih, "_IUPWASM_VLISTFOCUS", pos);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    }
    else
      iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
  }
  wasmVListSync(ih, 0);
}

static void wasmVListSelectAt(Ihandle* ih, int pos, int shift, int ctrl, int move_only)
{
  int count = iupListGetItemCount(ih);
  if (count < 1)
    return;
  if (pos < 1)
    pos = 1;
  if (pos > count)
    pos = count;

  iupAttribSetInt(ih, "_IUPWASM_VLISTFOCUS", pos);

  if (ih->data->is_multiple)
  {
    unsigned char* marks = wasmVListMarks(ih);
    int anchor = iupAttribGetInt(ih, "_IUPWASM_VLISTANCHOR");
    if (!marks)
      return;
    if (move_only)
    {
      wasmVListSync(ih, 1);
      return;
    }
    if (shift && anchor > 0 && anchor <= count)
    {
      int a = anchor < pos ? anchor : pos, b = anchor < pos ? pos : anchor, i;
      if (!ctrl)
        memset(marks, 0, count);
      for (i = a; i <= b; i++)
        marks[i - 1] = 1;
    }
    else if (ctrl)
    {
      marks[pos - 1] = !marks[pos - 1];
      iupAttribSetInt(ih, "_IUPWASM_VLISTANCHOR", pos);
    }
    else
    {
      memset(marks, 0, count);
      marks[pos - 1] = 1;
      iupAttribSetInt(ih, "_IUPWASM_VLISTANCHOR", pos);
    }
  }
  else
    iupAttribSetInt(ih, "_IUPWASM_VLISTSEL", pos);

  wasmVListSync(ih, 1);
  wasmVListNotify(ih);
}

static char* wasmListGetValueAttrib(Ihandle* ih);

static int wasmListSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!id)
    return 0;
  if (wasmListIsVirtual(ih))
  {
    wasmVListSetValue(ih, value);
    return 0;
  }
  if (ih->data->has_editbox)
    iupwasmJsListSetText(id, value ? value : "");
  else if (ih->data->is_multiple)
  {
    iupwasmJsListSetMulti(id, value ? value : "");
    iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", wasmListGetValueAttrib(ih));
  }
  else
  {
    int pos = 0;
    iupStrToInt(value, &pos);
    iupwasmJsListSetSelIndex(id, pos - 1);
    if (pos > 0)
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
    else
      iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
  }
  return 0;
}

static char* wasmListGetValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (!id)
    return NULL;
  if (wasmListIsVirtual(ih))
    return wasmVListGetValue(ih);
  if (ih->data->has_editbox)
  {
    char* p;
    if (iupAttribGet(ih, "_IUPWASM_EDITOLD"))
      return iupStrReturnStr(iupAttribGet(ih, "_IUPWASM_EDITOLD"));
    p = iupwasmJsListGetText(id);
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
  if (ih->data->is_virtual && !ih->data->is_dropdown && !ih->data->has_editbox)
  {
    p = pos < ih->data->item_count ? iupListGetItemValueCb(ih, pos + 1) : NULL;
    return p ? iupStrReturnStr(p) : NULL;
  }
  p = iupwasmJsListItemText(handleId, pos);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

EMSCRIPTEN_KEEPALIVE void iupwasmListVRows(int id, int from, int to, int gen)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  sIFni value_cb, image_cb;
  int pos, count;

  if (!ih || !wasmListIsVirtual(ih))
    return;

  count = iupListGetItemCount(ih);
  if (to > count) to = count;

  value_cb = (sIFni)IupGetCallback(ih, "VALUE_CB");
  image_cb = ih->data->show_image ? (sIFni)IupGetCallback(ih, "IMAGE_CB") : NULL;

  iupwasmJsVListRowsBegin();
  for (pos = from; pos <= to; pos++)
  {
    int imgId = 0;
    char* text;
    if (image_cb)
    {
      char* name = image_cb(ih, pos);
      if (name && name[0])
        imgId = (int)(intptr_t)iupImageGetImage(name, ih, 0, NULL);
    }
    text = value_cb ? value_cb(ih, pos) : NULL;
    iupwasmJsVListRowsAdd(text ? text : "", imgId);
  }
  iupwasmJsVListRowsEnd(id, gen, from);
}

EMSCRIPTEN_KEEPALIVE void iupwasmListVClick(int id, int pos, int mods)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  if (!ih || !wasmListIsVirtual(ih))
    return;
  wasmVListSelectAt(ih, pos, mods & 1, mods & 2, 0);
}

EMSCRIPTEN_KEEPALIVE void iupwasmListVDblClick(int id, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnis cb;
  if (!ih || !wasmListIsVirtual(ih))
    return;
  cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
  if (cb && cb(ih, pos, iupListGetItemValueCb(ih, pos)) == IUP_CLOSE)
    IupExitLoop();
}

int iupwasmListVKeyNav(Ihandle* ih, int code)
{
  int focus, page, count, pos;
  int shift = ih->data->is_multiple && iup_isShiftXkey(code);
  int ctrl = ih->data->is_multiple && iup_isCtrlXkey(code);
  if (!wasmListIsVirtual(ih) || iup_isAltXkey(code))
    return 0;
  count = iupListGetItemCount(ih);
  if (count < 1)
    return 0;
  focus = iupAttribGetInt(ih, "_IUPWASM_VLISTFOCUS");
  if (focus < 1 && !ih->data->is_multiple)
    focus = iupAttribGetInt(ih, "_IUPWASM_VLISTSEL");
  page = iupAttribGetInt(ih, "_IUPWASM_VLISTROWH");
  page = page > 0 ? (ih->currentheight - 2) / page : 1;
  page = page > 1 ? page - 1 : 1;

  switch (iup_XkeyBase(code))
  {
    case K_UP:    pos = focus - 1; break;
    case K_DOWN:  pos = focus + 1; break;
    case K_PGUP:  pos = focus - page; break;
    case K_PGDN:  pos = focus + page; break;
    case K_HOME:  pos = 1; break;
    case K_END:   pos = count; break;
    case K_SP:
      if (!ih->data->is_multiple || focus < 1)
        return 0;
      wasmVListSelectAt(ih, focus, 0, 1, 0);
      return 1;
    default:
      return 0;
  }
  wasmVListSelectAt(ih, pos, shift, 0, ctrl && !shift);
  return 1;
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
    iupAttribSetInt(ih, "_IUPWASM_VLISTROWH", wasmVListRowHeight(ih));
    iupwasmJsVListInit(id, iupAttribGetInt(ih, "_IUPWASM_VLISTROWH"));
    iupwasmJsVListSetFit(id, ih->data->fit_image);
    iupwasmAddToParent(ih);
    iupwasmJsVListSetCount(id, iupListGetItemCount(ih));  /* ITEMCOUNT setter returns 0 (not replayed), seed it here */
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
  iupListSetInitialItems(ih);
  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
{
  if (wasmListIsVirtual(ih))
    return ih->data->item_count;
  return iupwasmJsListCount(iupwasmIdOf(ih));
}

static int wasmListSortedPos(Ihandle* ih, int id, const char* value)
{
  int i, count = iupwasmJsListCount(id);
  for (i = 0; i < count; i++)
  {
    char* text = iupwasmJsListItemText(id, i);
    int after = iupStrCompare(text ? text : "", value, 0, 1) > 0;
    free(text);
    if (after)
      return i;
  }
  return count;
}

IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!value)
    value = "";
  if (!id)
    return;
  if (iupAttribGetBoolean(ih, "SORT"))
    iupwasmJsListInsert(id, wasmListSortedPos(ih, id, value), value);
  else
    iupwasmJsListAppend(id, value);
}

IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!value)
    value = "";
  if (id)
  {
    if (iupAttribGetBoolean(ih, "SORT"))
      pos = wasmListSortedPos(ih, id, value);
    iupwasmJsListInsert(id, pos, value);
  }
  iupListUpdateOldValue(ih, pos, 0);
}

IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsListRemove(id, pos);
  iupListUpdateOldValue(ih, pos, 1);
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
    iupwasmImageGetSize(img, &w, &h);
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
  if (iupAttribGetInt(ih, "_IUPWASM_VLISTSEL") > count)
  {
    iupAttribSetInt(ih, "_IUPWASM_VLISTSEL", 0);
    iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
  }
  if (iupAttribGetInt(ih, "_IUPWASM_VLISTFOCUS") > count)
    iupAttribSetInt(ih, "_IUPWASM_VLISTFOCUS", count);
  if (iupAttribGetInt(ih, "_IUPWASM_VLISTANCHOR") > count)
    iupAttribSet(ih, "_IUPWASM_VLISTANCHOR", NULL);
  if (ih->data->is_multiple && iupAttribGet(ih, "_IUPLIST_OLDVALUE"))
    iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", wasmVListGetValue(ih));
  wasmVListSync(ih, 0);
}

IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int* w, int* h)
{
  if (w)
  {
    *w += 4;
    if (ih->data->is_dropdown)
      *w += iupdrvGetScrollbarSize();
  }
  if (h) *h += 2;
}

IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int* h)
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
      iupwasmJsVListSetTop(id, pos);
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
  return iupStrReturnIntInt(s + 1, e + 1, ':');
}

static int wasmListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), s = 1, e = 1;
  if (!id || !ih->data->has_editbox || !value) return 0;
  if (iupStrEqualNoCase(value, "NONE")) { iupwasmJsListEditSetSel(id, 0, 0); return 0; }
  if (iupStrEqualNoCase(value, "ALL")) { iupwasmJsListEditSetSel(id, 0, 1000000); return 0; }
  if (iupStrToIntInt(value, &s, &e, ':') != 2 || s < 1 || e < 1) return 0;
  iupwasmJsListEditSetSel(id, s - 1, e - 1);
  return 0;
}

static int wasmListSetScrollToPosEditAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 0;
  if (id && ih->data->has_editbox && iupStrToInt(value, &pos))
    iupwasmJsListEditSetCaret(id, pos);  /* moving the caret scrolls the input into view */
  return 0;
}

static int wasmListSetScrollToEditAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 1;
  if (id && ih->data->has_editbox && iupStrToInt(value, &pos))
    iupwasmJsListEditSetCaret(id, pos > 0 ? pos - 1 : 0);
  return 0;
}

static int wasmListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->has_editbox && value)
    iupwasmJsListEditSetSelText(id, value);
  return 0;
}

static int wasmListSetSilent(Ihandle* ih, const char* value, IattribSetFunc set)
{
  int ret = set(ih, value);
  int id = iupwasmIdOf(ih);
  if (id && ih->data->has_editbox)
    ih->data->last_caret_pos = iupwasmJsListEditGetCaret(id);
  return ret;
}

static int wasmListSetValueAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetValueAttrib);
}

static int wasmListSetAppendAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetAppendAttrib);
}

static int wasmListSetInsertAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetInsertAttrib);
}

static int wasmListSetClipboardAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetClipboardAttrib);
}

static int wasmListSetCaretPosAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetCaretPosAttrib);
}

static int wasmListSetSelectionPosAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetSelectionPosAttrib);
}

static int wasmListSetSelectedTextAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetSelectedTextAttrib);
}

static int wasmListSetCaretAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetCaretAttrib);
}

static int wasmListSetSelectionAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetSelectionAttrib);
}

static int wasmListSetScrollToEditAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetScrollToEditAttrib);
}

static int wasmListSetScrollToPosEditAttribSilent(Ihandle* ih, const char* value)
{
  return wasmListSetSilent(ih, value, wasmListSetScrollToPosEditAttrib);
}

static void wasmListUnMapMethod(Ihandle* ih)
{
  unsigned char* marks = (unsigned char*)iupAttribGet(ih, "_IUPWASM_VLISTMARKS");
  if (marks)
  {
    free(marks);
    iupAttribSet(ih, "_IUPWASM_VLISTMARKS", NULL);
    iupAttribSet(ih, "_IUPWASM_VLISTMARKN", NULL);
  }
  iupdrvBaseUnMapMethod(ih);
}

static void wasmListLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  if (wasmListIsVirtual(ih))
  {
    int h = wasmVListRowHeight(ih);
    if (h != iupAttribGetInt(ih, "_IUPWASM_VLISTROWH"))
    {
      iupAttribSetInt(ih, "_IUPWASM_VLISTROWH", h);
      iupwasmJsVListRowHeight(iupwasmIdOf(ih), h);
    }
  }
}

IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = wasmListMapMethod;
  ic->UnMap = wasmListUnMapMethod;
  ic->LayoutUpdate = wasmListLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "READONLY", NULL, wasmListSetReadOnlyAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", iupwasmGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NC", NULL, wasmListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, wasmListSetAppendAttribSilent, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, wasmListSetInsertAttribSilent, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, wasmListSetClipboardAttribSilent, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", wasmListGetCaretPosAttrib, wasmListSetCaretPosAttribSilent, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", wasmListGetSelectionPosAttrib, wasmListSetSelectionPosAttribSilent, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", wasmListGetSelectedTextAttrib, wasmListSetSelectedTextAttribSilent, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITIMAGE", wasmListGetFitImageAttrib, wasmListSetFitImageAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, wasmListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, wasmListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, wasmListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, wasmListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, wasmListSetDropExpandAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", wasmListGetCaretAttrib, wasmListSetCaretAttribSilent, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", wasmListGetSelectionAttrib, wasmListSetSelectionAttribSilent, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, wasmListSetScrollToEditAttribSilent, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, wasmListSetScrollToPosEditAttribSilent, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_SUPPORTED);  /* native <select> popup item count is UA-controlled */
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);  /* DOM always repaints */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, wasmListSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", wasmListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IDVALUE", wasmListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", wasmListGetValueAttrib, wasmListSetValueAttribSilent, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
}
