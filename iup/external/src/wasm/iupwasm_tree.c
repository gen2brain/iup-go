/** \file
 * \brief WebAssembly Tree
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
#include "iup_image.h"
#include "iup_tree.h"

#include "iupwasm_drv.h"


/* rowId->kind kept in C: driver runs on a Worker but needs kind synchronously for iupTreeAddToCache. */
static signed char* wasm_tree_kind = NULL;
static int wasm_tree_kind_cap = 0;

static void wasmTreeSetKind(int rowId, int kind)
{
  if (rowId < 0)
    return;
  if (rowId >= wasm_tree_kind_cap)
  {
    int cap = rowId + 64;
    wasm_tree_kind = (signed char*)realloc(wasm_tree_kind, cap);
    while (wasm_tree_kind_cap < cap)
      wasm_tree_kind[wasm_tree_kind_cap++] = -1;
  }
  wasm_tree_kind[rowId] = (signed char)kind;
}

static int wasmTreeGetKind(int rowId)
{
  if (rowId >= 0 && rowId < wasm_tree_kind_cap)
    return wasm_tree_kind[rowId];
  return -1;
}

/* nodes are flat pre-order rows; InodeHandle = row id in __iupTree.nodes */
EM_JS(int, iupwasmJsTreeCreate, (void), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  if (!globalThis.__iupTree) globalThis.__iupTree = { nodes: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'treecreate', id: id });
  return id;
})

EM_JS(void, iupwasmJsTreeReflow, (int treeId), {
  globalThis.__iupApply({ op: 'treereflow', id: treeId });
})

EM_JS(int, iupwasmJsTreeAddNode, (int treeId, int prevRowId, int kindPrev, int add, int isBranch, const char* title), {
  if (!globalThis.__iupTree) globalThis.__iupTree = { nodes: {}, next: 1 };
  var rowId = globalThis.__iupTree.next++;
  globalThis.__iupApply({ op: 'treeaddnode', id: treeId, rowId: rowId, prevRowId: prevRowId, kindPrev: kindPrev, add: add, isBranch: isBranch, title: UTF8ToString(title) });
  return rowId;
})

/* The node rows live on the main thread; on a Worker these read over the SAB. */
EM_JS(int, iupwasmJsTreeDepth, (int rowId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treedepth', rowId: rowId });
  var r = globalThis.__iupTree.nodes[rowId];
  return r ? r.__iupDepth : 0;
})

EM_JS(int, iupwasmJsTreeChildCount, (int rowId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treechildcount', rowId: rowId });
  var r = globalThis.__iupTree.nodes[rowId]; if (!r) return 0;
  var n = r.nextSibling, c = 0;
  while (n && n.__iupDepth > r.__iupDepth) { c++; n = n.nextSibling; }
  return c;
})

EM_JS(void, iupwasmJsTreeSetTitle, (int rowId, const char* title), {
  globalThis.__iupApply({ op: 'treesettitle', rowId: rowId, title: UTF8ToString(title) });
})

EM_JS(void, iupwasmJsTreeSetColor, (int rowId, const char* color), {
  globalThis.__iupApply({ op: 'treecolor', rowId: rowId, color: UTF8ToString(color) });
})

EM_JS(void, iupwasmJsTreeSetNodeImage, (int rowId, int imgId), {
  globalThis.__iupApply({ op: 'treenodeimage', rowId: rowId, imgId: imgId });
})

EM_JS(void, iupwasmJsTreeExpandAll, (int treeId, int expand), {
  globalThis.__iupApply({ op: 'treeexpandall', id: treeId, expand: expand });
})

EM_JS(int, iupwasmJsTreeGetTitle, (int rowId), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'treetitle', rowId: rowId });
  else { var r = globalThis.__iupTree.nodes[rowId]; s = r ? r.__iupTitleEl.textContent : ""; }
  var len = lengthBytesUTF8(s) + 1, ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

EM_JS(void, iupwasmJsTreeSetState, (int treeId, int rowId, int expanded), {
  globalThis.__iupApply({ op: 'treesetstate', id: treeId, rowId: rowId, expanded: expanded });
})

EM_JS(int, iupwasmJsTreeGetState, (int rowId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treestate', rowId: rowId });
  var r = globalThis.__iupTree.nodes[rowId];
  if (!r || r.__iupKind !== 0) return -1;
  return r.__iupExpanded ? 1 : 0;
})

EM_JS(void, iupwasmJsTreeFocus, (int treeId, int rowId), {
  globalThis.__iupApply({ op: 'treefocus', id: treeId, rowId: rowId });
})

EM_JS(int, iupwasmJsTreeGetFocus, (int treeId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treefocus', id: treeId });
  var tree = globalThis.__iup.els[treeId];
  return tree ? (tree.__iupTreeFocus || 0) : 0;
})

EM_JS(void, iupwasmJsTreeDel, (int treeId, int rowId), {
  globalThis.__iupApply({ op: 'treedel', id: treeId, rowId: rowId });
})

EM_JS(void, iupwasmJsTreeStartRename, (int treeId, int rowId), {
  globalThis.__iupApply({ op: 'treestartrename', id: treeId, rowId: rowId });
})

/* navigation reads the pre-order DOM rows; mode 0 parent,1 next,2 prev,3 first,4 last */
EM_JS(int, iupwasmJsTreeNav, (int rowId, int mode), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treenav', rowId: rowId, mode: mode });
  var r = globalThis.__iupTree.nodes[rowId]; if (!r) return 0;
  if (mode === 0) return r.__iupParent || 0;
  if (mode === 1) { var n = r.nextElementSibling; while (n) { if (n.__iupDepth < r.__iupDepth) return 0; if (n.__iupDepth === r.__iupDepth) return n.__iupRowId; n = n.nextElementSibling; } return 0; }
  if (mode === 2) { var p = r.previousElementSibling; while (p) { if (p.__iupDepth < r.__iupDepth) return 0; if (p.__iupDepth === r.__iupDepth) return p.__iupRowId; p = p.previousElementSibling; } return 0; }
  if (mode === 3) { if (r.__iupDepth === 0) return r.parentNode.firstElementChild ? r.parentNode.firstElementChild.__iupRowId : 0; var pr = globalThis.__iupTree.nodes[r.__iupParent]; return (pr && pr.nextElementSibling) ? pr.nextElementSibling.__iupRowId : 0; }
  if (mode === 4) { var last = r, m = r.nextElementSibling; while (m) { if (m.__iupDepth < r.__iupDepth) break; if (m.__iupDepth === r.__iupDepth) last = m; m = m.nextElementSibling; } return last.__iupRowId; }
  return 0;
})

EM_JS(int, iupwasmJsTreeRootCount, (int treeId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treerootcount', id: treeId });
  var tree = globalThis.__iup.els[treeId]; if (!tree) return 0;
  var c = 0, ch = tree.children; for (var i = 0; i < ch.length; i++) if (ch[i].__iupDepth === 0) c++; return c;
})

EM_JS(void, iupwasmJsTreeDefImage, (int treeId, int which, int imgId), { globalThis.__iupApply({ op: 'treedefimage', id: treeId, which: which, imgId: imgId }); })
EM_JS(void, iupwasmJsTreeNodeImageExp, (int rowId, int imgId), { globalThis.__iupApply({ op: 'treenodeimageexp', rowId: rowId, imgId: imgId }); })
EM_JS(void, iupwasmJsTreeIndent, (int treeId, int px), { globalThis.__iupApply({ op: 'treeindent', id: treeId, px: px }); })
EM_JS(void, iupwasmJsTreeSpacing, (int treeId, int px), { globalThis.__iupApply({ op: 'treespacing', id: treeId, px: px }); })
EM_JS(void, iupwasmJsTreeTopItem, (int treeId, int rowId), { globalThis.__iupApply({ op: 'treetopitem', id: treeId, rowId: rowId }); })
EM_JS(void, iupwasmJsTreeTitleFont, (int rowId, const char* css), { globalThis.__iupApply({ op: 'treetitlefont', rowId: rowId, css: UTF8ToString(css) }); })

EM_JS(void, iupwasmJsTreeMark, (int treeId, int rowId, int on), { globalThis.__iupApply({ op: 'treemark', id: treeId, rowId: rowId, on: on }); })
EM_JS(void, iupwasmJsTreeMarkClear, (int treeId), { globalThis.__iupApply({ op: 'treemarkclear', id: treeId }); })
EM_JS(int, iupwasmJsTreeIsMarked, (int rowId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treemarked', rowId: rowId });
  var r = globalThis.__iupTree.nodes[rowId]; return (r && r.__iupMarked) ? 1 : 0;
})

/* rowIds allocated Worker-side to stay consistent with AddNode; DOM assigns base..base+count-1 to clones. */
EM_JS(int, iupwasmJsTreeAllocRows, (int n), {
  if (!globalThis.__iupTree) globalThis.__iupTree = { nodes: {}, next: 1 };
  var base = globalThis.__iupTree.next; globalThis.__iupTree.next += n; return base;
})
EM_JS(void, iupwasmJsTreeCopyMove, (int srcTree, int dstTree, int srcRowId, int dstRowId, int asChild, int isCopy, int base), {
  globalThis.__iupApply({ op: 'treecopymove', srcTree: srcTree, dstTree: dstTree, srcRowId: srcRowId, dstRowId: dstRowId, asChild: asChild, isCopy: isCopy, base: base });
})

EM_JS(void, iupwasmJsTreeShowToggle, (int treeId, int mode), { globalThis.__iupApply({ op: 'treeshowtoggle', id: treeId, mode: mode }); })
EM_JS(void, iupwasmJsTreeToggleValue, (int rowId, int state), { globalThis.__iupApply({ op: 'treetogglevalue', rowId: rowId, state: state }); })
EM_JS(int, iupwasmJsTreeGetToggleValue, (int rowId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treetogglevalue', rowId: rowId });
  var r = globalThis.__iupTree.nodes[rowId]; if (!r || !r.__iupChk) return 0; if (r.__iupChk.indeterminate) return -1; return r.__iupChk.checked ? 1 : 0;
})

EM_JS(void, iupwasmJsTreeToggleVisible, (int rowId, int on), { globalThis.__iupApply({ op: 'treetogglevisible', rowId: rowId, on: on }); })
EM_JS(int, iupwasmJsTreeGetToggleVisible, (int rowId), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treetogglevisible', rowId: rowId });
  var r = globalThis.__iupTree.nodes[rowId]; return (r && r.__iupChk && r.__iupChk.style.display !== 'none') ? 1 : 0;
})

EMSCRIPTEN_KEEPALIVE void iupwasmTreeToggle(int treeId, int rowId, int state)
{
  Ihandle* ih = iupwasmHandleFromId(treeId);
  int id;
  IFnii cb;
  if (!ih)
    return;
  id = iupTreeFindNodeId(ih, (InodeHandle*)(intptr_t)rowId);
  if (id < 0)
    return;
  cb = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
  if (cb)
    cb(ih, id, state);

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE && iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
    iupwasmJsTreeMark(treeId, rowId, state == 1);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTreeEvent(int treeId, int rowId, int type)
{
  Ihandle* ih = iupwasmHandleFromId(treeId);
  int id;
  if (!ih)
    return;
  id = iupTreeFindNodeId(ih, (InodeHandle*)(intptr_t)rowId);
  if (id < 0)
    return;

  if (type == 1)  /* expander toggle */
  {
    int expanded = iupwasmJsTreeGetState(rowId);
    IFni cb;
    if (expanded < 0) return;
    iupwasmJsTreeSetState(treeId, rowId, !expanded);
    cb = (IFni)IupGetCallback(ih, expanded ? "BRANCHCLOSE_CB" : "BRANCHOPEN_CB");
    if (cb) cb(ih, id);
  }
  else if (type == 2)  /* execute leaf */
  {
    IFni cb = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
    if (cb) cb(ih, id);
  }
  else if (type == 4)  /* execute branch (dblclick) toggles too */
  {
    int expanded = iupwasmJsTreeGetState(rowId);
    IFni cb;
    if (expanded >= 0)
      iupwasmJsTreeSetState(treeId, rowId, !expanded);
    cb = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
    if (cb) cb(ih, id);
  }
  else if (type == 3)  /* right click */
  {
    IFni cb;
    iupwasmJsTreeFocus(treeId, rowId);
    cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cb) cb(ih, id);
  }
}

EMSCRIPTEN_KEEPALIVE int iupwasmTreeRenameEnd(int treeId, int rowId, const char* text)
{
  Ihandle* ih = iupwasmHandleFromId(treeId);
  int id, ret = IUP_DEFAULT;
  IFnis cb;
  (void)treeId;
  if (!ih)
    return 1;
  id = iupTreeFindNodeId(ih, (InodeHandle*)(intptr_t)rowId);
  cb = (IFnis)IupGetCallback(ih, "RENAME_CB");
  if (cb)
    ret = cb(ih, id, (char*)text);
  return (ret == IUP_IGNORE) ? 0 : 1;
}

IUP_SDK_API void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  int treeId = iupwasmIdOf(ih);
  int prevRowId = 0, kindPrev = -1, newRowId;
  int was_empty = (ih->data->node_count == 0);

  if (!treeId)
    return;

  if (id == IUP_INVALID_ID && ih->data->node_count != 0)
    id = iupTreeFindNodeId(ih, iupdrvTreeGetFocusNode(ih));

  if (id >= 0)
  {
    prevRowId = (int)(intptr_t)iupTreeGetNode(ih, id);
    if (!prevRowId)
      return;
    kindPrev = wasmTreeGetKind(prevRowId);
  }

  newRowId = iupwasmJsTreeAddNode(treeId, prevRowId, kindPrev, add, (kind == ITREE_BRANCH) ? 1 : 0, title ? title : "");
  if (!newRowId)
    return;
  wasmTreeSetKind(newRowId, kind);

  if (kindPrev != -1)
    iupTreeAddToCache(ih, add, kindPrev, (InodeHandle*)(intptr_t)prevRowId, (InodeHandle*)(intptr_t)newRowId);
  else
    iupTreeAddToCache(ih, 0, 0, NULL, (InodeHandle*)(intptr_t)newRowId);

  /* root becomes focus so a subsequent no-id ADD targets it */
  if (was_empty)
    iupwasmJsTreeFocus(treeId, newRowId);
}

IUP_SDK_API void iupdrvTreeUpdateMarkMode(Ihandle *ih)
{
  (void)ih;
}

IUP_SDK_API void iupdrvTreeAddBorders(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  if (w) *w += 2;
  if (h) *h += 2;
}

IUP_SDK_API InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  int treeId = iupwasmIdOf(ih);
  if (!treeId)
    return NULL;
  return (InodeHandle*)(intptr_t)iupwasmJsTreeGetFocus(treeId);
}

IUP_SDK_API int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  (void)ih;
  return iupwasmJsTreeChildCount((int)(intptr_t)node_handle);
}

static int wasmTreeComputeIdNew(Ihandle* ih, int idDst, int dstRow, int* asChild)
{
  int id_new = idDst + 1;
  *asChild = 0;
  if (wasmTreeGetKind(dstRow) == ITREE_BRANCH)
  {
    if (iupwasmJsTreeGetState(dstRow) == 1)
      *asChild = 1;
    else
      id_new += iupwasmJsTreeChildCount(dstRow);
  }
  return id_new;
}

static void wasmTreeCopyMoveNode(Ihandle* ih, int idSrc, int idDst, int isCopy)
{
  int treeId = iupwasmIdOf(ih);
  int srcRow = (int)(intptr_t)iupTreeGetNode(ih, idSrc);
  int dstRow = (int)(intptr_t)iupTreeGetNode(ih, idDst);
  int old_count = ih->data->node_count;
  int id_new, count, asChild, base = 0, k;

  if (!treeId || !srcRow || !dstRow)
    return;

  count = iupwasmJsTreeChildCount(srcRow) + 1;

  /* dst must not be inside the src subtree (would move a node into its own descendant) */
  if (idDst >= idSrc && idDst < idSrc + count)
    return;

  id_new = wasmTreeComputeIdNew(ih, idDst, dstRow, &asChild);

  if (!isCopy && id_new == idSrc)
    return;

  if (isCopy)
    base = iupwasmJsTreeAllocRows(count);

  iupwasmJsTreeCopyMove(treeId, treeId, srcRow, dstRow, asChild, isCopy, base);

  ih->data->node_count = old_count + count;
  iupTreeCopyMoveCache(ih, idSrc, id_new, count, isCopy);

  if (!isCopy)
    ih->data->node_count = old_count;
  else
    for (k = 0; k < count; k++)
      ih->data->node_cache[id_new + k].node_handle = (InodeHandle*)(intptr_t)(base + k);
}

IUP_SDK_API void iupdrvTreeDragDropCopyNode(Ihandle *src, Ihandle *dst, InodeHandle *itemSrc, InodeHandle *itemDst)
{
  int idSrc = iupTreeFindNodeId(src, itemSrc);
  int idDst = iupTreeFindNodeId(dst, itemDst);
  if (idSrc < 0 || idDst < 0)
    return;

  if (src == dst)
  {
    wasmTreeCopyMoveNode(dst, idSrc, idDst, 1);
    return;
  }

  {
    int srcTree = iupwasmIdOf(src), dstTree = iupwasmIdOf(dst);
    int srcRow = (int)(intptr_t)itemSrc, dstRow = (int)(intptr_t)itemDst;
    int old_count = dst->data->node_count;
    int count = iupwasmJsTreeChildCount(srcRow) + 1;
    int asChild, id_new, remain, base, k;
    if (!srcTree || !dstTree)
      return;
    id_new = wasmTreeComputeIdNew(dst, idDst, dstRow, &asChild);
    base = iupwasmJsTreeAllocRows(count);

    iupwasmJsTreeCopyMove(srcTree, dstTree, srcRow, dstRow, asChild, 1, base);

    dst->data->node_count = old_count + count;
    iupTreeIncCacheMem(dst);
    remain = dst->data->node_count - (id_new + count);
    if (remain > 0)
      memmove(dst->data->node_cache + id_new + count, dst->data->node_cache + id_new, remain * sizeof(InodeData));
    memset(dst->data->node_cache + id_new, 0, count * sizeof(InodeData));
    for (k = 0; k < count; k++)
      dst->data->node_cache[id_new + k].node_handle = (InodeHandle*)(intptr_t)(base + k);
  }
}

static char* wasmTreeGetTitleAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  char* ptr; char* ret;
  if (!rowId)
    return NULL;
  ptr = (char*)(intptr_t)iupwasmJsTreeGetTitle(rowId);
  ret = ptr ? iupStrReturnStr(ptr) : NULL;
  if (ptr) free(ptr);
  return ret;
}

static int wasmTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (rowId)
    iupwasmJsTreeSetTitle(rowId, value ? value : "");
  return 0;
}

static char* wasmTreeGetKindAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (!rowId)
    return NULL;
  return wasmTreeGetKind(rowId) == ITREE_BRANCH ? "BRANCH" : "LEAF";
}

static char* wasmTreeGetDepthAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (!rowId)
    return NULL;
  return iupStrReturnInt(iupwasmJsTreeDepth(rowId));
}

static char* wasmTreeGetStateAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  int st;
  if (!rowId)
    return NULL;
  st = iupwasmJsTreeGetState(rowId);
  if (st < 0)
    return NULL;
  return st ? "EXPANDED" : "COLLAPSED";
}

static int wasmTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (rowId)
    iupwasmJsTreeSetState(iupwasmIdOf(ih), rowId, iupStrEqualNoCase(value, "EXPANDED"));
  return 0;
}

static char* wasmTreeGetValueAttrib(Ihandle* ih)
{
  InodeHandle* focus = iupdrvTreeGetFocusNode(ih);
  int id = focus ? iupTreeFindNodeId(ih, focus) : -1;
  return iupStrReturnInt(id < 0 ? 0 : id);
}

static int wasmTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  int treeId = iupwasmIdOf(ih);
  int id, rowId;
  IFnii cb;

  if (!treeId)
    return 0;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
    id = 0;
  else if (!iupStrToInt(value, &id))
    return 0;

  rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (!rowId)
    return 0;

  iupwasmJsTreeFocus(treeId, rowId);
  cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (cb)
    cb(ih, id, 1);
  return 0;
}

/* fires NODEREMOVED_CB deepest-child-first then the node, mirroring gtkTreeCallNodeRemovedRec */
static void wasmTreeCallNodeRemovedRec(Ihandle* ih, IFns cb, int id, int depth)
{
  int child = id + 1;
  while (child < ih->data->node_count &&
         iupwasmJsTreeDepth((int)(intptr_t)ih->data->node_cache[child].node_handle) > depth)
  {
    int child_depth = iupwasmJsTreeDepth((int)(intptr_t)ih->data->node_cache[child].node_handle);
    if (child_depth == depth + 1)
      wasmTreeCallNodeRemovedRec(ih, cb, child, child_depth);
    child++;
  }
  cb(ih, (char*)ih->data->node_cache[id].userdata);
}

static void wasmTreeCallNodeRemoved(Ihandle* ih, int id, int count)
{
  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  if (cb && id >= 0 && id < ih->data->node_count)
  {
    int depth = iupwasmJsTreeDepth((int)(intptr_t)ih->data->node_cache[id].node_handle);
    (void)count;
    wasmTreeCallNodeRemovedRec(ih, cb, id, depth);
  }
}

static void wasmTreeDelOne(Ihandle* ih, int treeId, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  int count;
  if (!rowId)
    return;
  count = iupwasmJsTreeChildCount(rowId) + 1;
  wasmTreeCallNodeRemoved(ih, id, count);
  iupwasmJsTreeDel(treeId, rowId);
  ih->data->node_count -= count;
  iupTreeDelFromCache(ih, id, count);
}

static int wasmTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  int treeId = iupwasmIdOf(ih);

  if (!treeId)
    return 0;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    wasmTreeDelOne(ih, treeId, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    int i;
    for (i = ih->data->node_count - 1; i >= 0; i--)
    {
      int rowId = (int)(intptr_t)iupTreeGetNode(ih, i);
      if (rowId && iupwasmJsTreeIsMarked(rowId))
        wasmTreeDelOne(ih, treeId, i);
    }
    return 0;
  }

  if (id < 0)
    return 0;

  if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
    while (rowId && iupwasmJsTreeChildCount(rowId) > 0)
      wasmTreeDelOne(ih, treeId, id + 1);
    return 0;
  }

  wasmTreeDelOne(ih, treeId, id);
  return 0;
}

static int wasmTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  int treeId = iupwasmIdOf(ih);
  InodeHandle* focus;
  IFni show_cb;
  int id;
  (void)value;

  if (!treeId)
    return 0;

  focus = iupdrvTreeGetFocusNode(ih);
  if (!focus)
    return 0;
  id = iupTreeFindNodeId(ih, focus);

  show_cb = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (show_cb && show_cb(ih, id) == IUP_IGNORE)
    return 0;

  iupwasmJsTreeStartRename(treeId, (int)(intptr_t)focus);
  return 0;
}

static int wasmTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  unsigned char r, g, b;
  char css[20];
  if (rowId && value && iupStrToRGB(value, &r, &g, &b))
  {
    snprintf(css, sizeof(css), "rgb(%d,%d,%d)", r, g, b);
    iupwasmJsTreeSetColor(rowId, css);
  }
  return 1;
}

static int wasmTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (rowId)
    iupwasmJsTreeSetNodeImage(rowId, img ? (int)(intptr_t)img : 0);
  return 1;
}

static int wasmTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (rowId)
    iupwasmJsTreeNodeImageExp(rowId, img ? (int)(intptr_t)img : 0);
  return 1;
}

static int wasmTreeSetDefImage(Ihandle* ih, const char* value, int which)
{
  int treeId = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (treeId)
    iupwasmJsTreeDefImage(treeId, which, img ? (int)(intptr_t)img : 0);
  return 1;
}
static int wasmTreeSetImageLeafAttrib(Ihandle* ih, const char* value) { return wasmTreeSetDefImage(ih, value, 0); }
static int wasmTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value) { return wasmTreeSetDefImage(ih, value, 1); }
static int wasmTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value) { return wasmTreeSetDefImage(ih, value, 2); }

static char* wasmTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (!rowId)
    return NULL;
  return iupStrReturnInt(iupwasmJsTreeChildCount(rowId));
}

static int wasmTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  int treeId = iupwasmIdOf(ih);
  if (treeId)
    iupwasmJsTreeExpandAll(treeId, iupStrBoolean(value));
  return 0;
}

static char* wasmTreeGetNav(Ihandle* ih, int id, int mode)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  int target;
  if (!rowId)
    return NULL;
  target = iupwasmJsTreeNav(rowId, mode);
  if (!target)
    return NULL;
  return iupStrReturnInt(iupTreeFindNodeId(ih, (InodeHandle*)(intptr_t)target));
}

static char* wasmTreeGetParentAttrib(Ihandle* ih, int id) { return wasmTreeGetNav(ih, id, 0); }
static char* wasmTreeGetNextAttrib(Ihandle* ih, int id) { return wasmTreeGetNav(ih, id, 1); }
static char* wasmTreeGetPreviousAttrib(Ihandle* ih, int id) { return wasmTreeGetNav(ih, id, 2); }
static char* wasmTreeGetFirstAttrib(Ihandle* ih, int id) { return wasmTreeGetNav(ih, id, 3); }
static char* wasmTreeGetLastAttrib(Ihandle* ih, int id) { return wasmTreeGetNav(ih, id, 4); }

static char* wasmTreeGetRootCountAttrib(Ihandle* ih)
{
  int treeId = iupwasmIdOf(ih);
  if (!treeId)
    return NULL;
  return iupStrReturnInt(iupwasmJsTreeRootCount(treeId));
}

static int wasmTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), px = 16;
  if (id && iupStrToInt(value, &px))
    iupwasmJsTreeIndent(id, px);
  return 1;
}

static int wasmTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;
  if (id) { iupwasmJsTreeSpacing(id, ih->data->spacing); return 0; }
  return 1;
}

static int wasmTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  int treeId = iupwasmIdOf(ih), id = 0, rowId;
  if (treeId && iupStrToInt(value, &id))
  {
    rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
    if (rowId)
      iupwasmJsTreeTopItem(treeId, rowId);
  }
  return 0;
}

static int wasmTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  char css[256];
  if (!rowId)
    return 0;
  if (value)
  {
    iupwasmFontToCss(value, css, sizeof(css));
    iupwasmJsTreeTitleFont(rowId, css);
  }
  else
    iupwasmJsTreeTitleFont(rowId, "");
  return 0;
}

static void wasmTreeMarkId(Ihandle* ih, int id, int on)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (rowId)
    iupwasmJsTreeMark(iupwasmIdOf(ih), rowId, on);
}

static int wasmTreeCollectMarked(Ihandle* ih, int** ids)
{
  int i, n = ih->data->node_count, count = 0;
  int* arr = (int*)malloc(sizeof(int) * (n > 0 ? n : 1));
  for (i = 0; i < n; i++)
  {
    int rowId = (int)(intptr_t)iupTreeGetNode(ih, i);
    if (rowId && iupwasmJsTreeIsMarked(rowId))
      arr[count++] = i;
  }
  *ids = arr;
  return count;
}

/* mirrors gtkTreeCallMultiUnSelectionCb: reports nodes being cleared, excluding the new one */
static void wasmTreeCallMultiUnselection(Ihandle* ih, int new_select_id)
{
  IFnIi cbMulti = (IFnIi)IupGetCallback(ih, "MULTIUNSELECTION_CB");
  IFnii cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  int* ids;
  int count;
  if (!cbMulti && !cbSelec)
    return;
  count = wasmTreeCollectMarked(ih, &ids);
  if (count > 0)
  {
    if (cbMulti)
    {
      int i;
      for (i = 0; i < count; i++)
        if (ids[i] == new_select_id) { memmove(ids + i, ids + i + 1, (count - i - 1) * sizeof(int)); count--; break; }
      cbMulti(ih, ids, count);
    }
    else
    {
      int i;
      for (i = 0; i < count; i++)
        if (ids[i] != new_select_id) cbSelec(ih, ids[i], 0);
    }
  }
  free(ids);
}

static void wasmTreeCallMultiSelection(Ihandle* ih, int a, int b)
{
  IFnIi cbMulti = (IFnIi)IupGetCallback(ih, "MULTISELECTION_CB");
  IFnii cbSelec = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  int count = b - a + 1, i;
  if (count <= 0)
    return;
  if (cbMulti)
  {
    int* ids = (int*)malloc(sizeof(int) * count);
    for (i = 0; i < count; i++) ids[i] = a + i;
    cbMulti(ih, ids, count);
    free(ids);
  }
  else if (cbSelec)
  {
    for (i = 0; i < count; i++) cbSelec(ih, a + i, 1);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmTreeSelect(int treeId, int rowId, int ctrl, int shift)
{
  Ihandle* ih = iupwasmHandleFromId(treeId);
  int id;
  IFnii cb;
  if (!ih)
    return;
  id = iupTreeFindNodeId(ih, (InodeHandle*)(intptr_t)rowId);
  if (id < 0)
    return;
  cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
  {
    if (shift)
    {
      int start = iupAttribGetInt(ih, "_IUPWASM_MARKSTART");
      int a = start < id ? start : id, b = start < id ? id : start, i;
      wasmTreeCallMultiUnselection(ih, -1);
      iupwasmJsTreeMarkClear(treeId);
      for (i = a; i <= b; i++) wasmTreeMarkId(ih, i, 1);
      wasmTreeCallMultiSelection(ih, a, b);
    }
    else if (ctrl)
    {
      int marked = iupwasmJsTreeIsMarked(rowId);
      iupwasmJsTreeMark(treeId, rowId, !marked);
      if (cb) cb(ih, id, !marked);
      iupAttribSetInt(ih, "_IUPWASM_MARKSTART", id);
    }
    else
    {
      wasmTreeCallMultiUnselection(ih, id);
      iupwasmJsTreeMarkClear(treeId);
      wasmTreeMarkId(ih, id, 1);
      if (cb) cb(ih, id, 1);
      iupAttribSetInt(ih, "_IUPWASM_MARKSTART", id);
    }
  }
  else
  {
    iupwasmJsTreeMarkClear(treeId);
    wasmTreeMarkId(ih, id, 1);
    if (cb) cb(ih, id, 1);
  }
  iupwasmJsTreeFocus(treeId, rowId);
}

static char* wasmTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (!rowId)
    return NULL;
  return iupStrReturnBoolean(iupwasmJsTreeIsMarked(rowId));
}

static int wasmTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  wasmTreeMarkId(ih, id, iupStrBoolean(value));
  return 0;
}

static char* wasmTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  int i, count = ih->data->node_count;
  char* str = iupStrGetMemory(count + 1);
  for (i = 0; i < count; i++)
  {
    int rowId = (int)(intptr_t)iupTreeGetNode(ih, i);
    str[i] = (rowId && iupwasmJsTreeIsMarked(rowId)) ? '+' : '-';
  }
  str[count] = 0;
  return str;
}

static int wasmTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  int i, count, n = ih->data->node_count;
  if (ih->data->mark_mode == ITREE_MARK_SINGLE || !value)
    return 0;
  count = (int)strlen(value);
  if (count > n) count = n;
  for (i = 0; i < count; i++)
    wasmTreeMarkId(ih, i, value[i] == '+');
  return 0;
}

static int wasmTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  int treeId = iupwasmIdOf(ih), i, n = ih->data->node_count;
  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    return 0;
  if (iupStrEqualNoCase(value, "CLEARALL"))
    iupwasmJsTreeMarkClear(treeId);
  else if (iupStrEqualNoCase(value, "MARKALL"))
    for (i = 0; i < n; i++) wasmTreeMarkId(ih, i, 1);
  else if (iupStrEqualNoCase(value, "INVERTALL"))
    for (i = 0; i < n; i++)
    {
      int rowId = (int)(intptr_t)iupTreeGetNode(ih, i);
      if (rowId) iupwasmJsTreeMark(treeId, rowId, !iupwasmJsTreeIsMarked(rowId));
    }
  else if (iupStrEqualNoCase(value, "BLOCK"))
  {
    int start = iupAttribGetInt(ih, "_IUPWASM_MARKSTART");
    InodeHandle* focus = iupdrvTreeGetFocusNode(ih);
    int fid = focus ? iupTreeFindNodeId(ih, focus) : 0;
    int a = start < fid ? start : fid, b = start < fid ? fid : start;
    for (i = a; i <= b; i++) wasmTreeMarkId(ih, i, 1);
  }
  else if (iupStrEqualPartial(value, "INVERT"))
  {
    int id = 0;
    if (iupStrToInt(value + strlen("INVERT"), &id))
    {
      int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
      if (rowId) iupwasmJsTreeMark(treeId, rowId, !iupwasmJsTreeIsMarked(rowId));
    }
  }
  return 0;
}

static int wasmTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  int id;
  if (iupStrToInt(value, &id))
    iupAttribSetInt(ih, "_IUPWASM_MARKSTART", id);
  return 1;
}

static int wasmTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  int idDst = 0;
  if (!ih->handle)
    return 0;
  if (iupStrEqualNoCase(value, "ROOT")) idDst = 0;
  else if (!iupStrToInt(value, &idDst)) return 0;
  wasmTreeCopyMoveNode(ih, id, idDst, 0);
  return 0;
}

static int wasmTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  int idDst = 0;
  if (!ih->handle)
    return 0;
  if (iupStrEqualNoCase(value, "ROOT")) idDst = 0;
  else if (!iupStrToInt(value, &idDst)) return 0;
  wasmTreeCopyMoveNode(ih, id, idDst, 1);
  return 0;
}

static char* wasmTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  int st;
  if (!ih->data->show_toggle || !rowId)
    return NULL;
  st = iupwasmJsTreeGetToggleValue(rowId);
  if (st == -1)
    return "NOTDEF";
  return st ? "ON" : "OFF";
}

static int wasmTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  int st;
  if (!ih->data->show_toggle || !rowId)
    return 0;
  if (iupStrEqualNoCase(value, "NOTDEF"))
    st = -1;
  else
    st = iupStrBoolean(value) ? 1 : 0;
  iupwasmJsTreeToggleValue(rowId, st);
  return 0;
}

static char* wasmTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (!ih->data->show_toggle || !rowId)
    return NULL;
  return iupStrReturnBoolean(iupwasmJsTreeGetToggleVisible(rowId));
}

static int wasmTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  int rowId = (int)(intptr_t)iupTreeGetNode(ih, id);
  if (rowId)
    iupwasmJsTreeToggleVisible(rowId, iupStrBoolean(value));
  return 0;
}

EM_JS(void, iupwasmJsTreeDndSource, (int id, int on), { globalThis.__iupApply({ op: 'treedndsrc', id: id, on: on }); })
EM_JS(void, iupwasmJsTreeDndTarget, (int id, int on), { globalThis.__iupApply({ op: 'treedndtgt', id: id, on: on }); })

EM_JS(int, iupwasmJsTreeXY2Row, (int treeId, int y), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'treexy2row', id: treeId, y: y });
  var tree = globalThis.__iup.els[treeId];
  if (!tree) return 0;
  var vy = tree.getBoundingClientRect().top + y, last = 0;
  for (var i = 0; i < tree.children.length; i++) {
    var r = tree.children[i];
    if (r.style.display === 'none') continue;
    last = r.__iupRowId;
    if (vy <= r.getBoundingClientRect().bottom) return r.__iupRowId;
  }
  return last;
})

static int wasmTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  int treeId = iupwasmIdOf(ih);
  int rowId;
  (void)x;
  if (!treeId)
    return -1;
  rowId = iupwasmJsTreeXY2Row(treeId, y);
  if (!rowId)
    return -1;
  return iupTreeFindNodeId(ih, (InodeHandle*)(intptr_t)rowId);
}

static int wasmTreeMapMethod(Ihandle* ih)
{
  int id = iupwasmJsTreeCreate();
  char* addroot;
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)wasmTreeConvertXYToPos);
  iupwasmAddToParent(ih);

  if (ih->data->show_toggle)
    iupwasmJsTreeShowToggle(id, ih->data->show_toggle);

  addroot = iupAttribGet(ih, "ADDROOT");
  if (!addroot || iupStrBoolean(addroot))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvTreeInitClass(Iclass* ic)
{
  ic->Map = wasmTreeMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttributeId(ic, "TITLE", wasmTreeGetTitleAttrib, wasmTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", wasmTreeGetKindAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", wasmTreeGetDepthAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "STATE", wasmTreeGetStateAttrib, wasmTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, wasmTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VALUE", wasmTreeGetValueAttrib, wasmTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, wasmTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", NULL, wasmTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, wasmTreeSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, wasmTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, wasmTreeSetImageLeafAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, wasmTreeSetImageBranchCollapsedAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, wasmTreeSetImageBranchExpandedAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMGLEAF", NULL, wasmTreeSetImageLeafAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMGCOLLAPSED", NULL, wasmTreeSetImageBranchCollapsedAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMGEXPANDED", NULL, wasmTreeSetImageBranchExpandedAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", wasmTreeGetChildCountAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, wasmTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "PARENT", wasmTreeGetParentAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", wasmTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", wasmTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", wasmTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", wasmTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", wasmTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INDENTATION", NULL, wasmTreeSetIndentationAttrib, "16", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, wasmTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, wasmTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", NULL, wasmTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "MARKED", wasmTreeGetMarkedAttrib, wasmTreeSetMarkedAttrib, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", NULL, wasmTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, wasmTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", NULL, wasmTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", wasmTreeGetMarkedNodesAttrib, wasmTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", wasmTreeGetToggleValueAttrib, wasmTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", wasmTreeGetToggleVisibleAttrib, wasmTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, wasmTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, wasmTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
