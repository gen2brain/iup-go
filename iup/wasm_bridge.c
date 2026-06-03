//go:build js && wasm && !tinygo

/* C->JS->Go callback bridge: trampolines forward to globalThis.iupGoDispatch. */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"


/* ih crosses as a plain number (the wasm32 pointer); i1..i4/sarg are per-callback args. */
EM_JS(int, iupwasmGoDispatch, (Ihandle* ih, const char* name, int i1, int i2, int i3, int i4, const char* sarg), {
  if (globalThis.iupGoDispatch)
    return globalThis.iupGoDispatch(ih, UTF8ToString(name), i1, i2, i3, i4, sarg ? UTF8ToString(sarg) : "") | 0;
  return 0;
})

/* Six-int variant for callbacks that carry more than four ints (e.g. cells DRAW_CB). */
EM_JS(int, iupwasmGoDispatch6, (Ihandle* ih, const char* name, int i1, int i2, int i3, int i4, int i5, int i6), {
  if (globalThis.iupGoDispatch6)
    return globalThis.iupGoDispatch6(ih, UTF8ToString(name), i1, i2, i3, i4, i5, i6) | 0;
  return 0;
})

static int wasmCbSpin(Ihandle* ih, int inc) { return iupwasmGoDispatch(ih, "SPIN_CB", inc, 0, 0, 0, 0); }
static int wasmCbNLines(Ihandle* ih) { return iupwasmGoDispatch(ih, "NLINES_CB", 0, 0, 0, 0, 0); }
static int wasmCbNCols(Ihandle* ih) { return iupwasmGoDispatch(ih, "NCOLS_CB", 0, 0, 0, 0, 0); }
static int wasmCbWidth(Ihandle* ih, int i) { return iupwasmGoDispatch(ih, "WIDTH_CB", i, 0, 0, 0, 0); }
static int wasmCbHeight(Ihandle* ih, int i) { return iupwasmGoDispatch(ih, "HEIGHT_CB", i, 0, 0, 0, 0); }
static int wasmCbCellsDraw(Ihandle* ih, int i, int j, int xmin, int xmax, int ymin, int ymax) { return iupwasmGoDispatch6(ih, "DRAW_CB", i, j, xmin, xmax, ymin, ymax); }

static int wasmCbAction(Ihandle* ih) { return iupwasmGoDispatch(ih, "ACTION", 0, 0, 0, 0, 0); }
static int wasmCbToggleAction(Ihandle* ih, int state) { return iupwasmGoDispatch(ih, "ACTION", state, 0, 0, 0, 0); }
static int wasmCbTextAction(Ihandle* ih, int c, char* v) { return iupwasmGoDispatch(ih, "ACTION", c, 0, 0, 0, v); }
static int wasmCbListAction(Ihandle* ih, char* t, int item, int state) { return iupwasmGoDispatch(ih, "ACTION", item, state, 0, 0, t); }
static int wasmCbLinkAction(Ihandle* ih, char* url) { return iupwasmGoDispatch(ih, "ACTION", 0, 0, 0, 0, url); }
static int wasmCbValueChanged(Ihandle* ih) { return iupwasmGoDispatch(ih, "VALUECHANGED_CB", 0, 0, 0, 0, 0); }
static int wasmCbKAny(Ihandle* ih, int c) { return iupwasmGoDispatch(ih, "K_ANY", c, 0, 0, 0, 0); }
static int wasmCbDblclick(Ihandle* ih, int item, char* t) { return iupwasmGoDispatch(ih, "DBLCLICK_CB", item, 0, 0, 0, t); }
static int wasmCbMultiselect(Ihandle* ih, char* t) { return iupwasmGoDispatch(ih, "MULTISELECT_CB", 0, 0, 0, 0, t); }
static int wasmCbCaret(Ihandle* ih, int lin, int col, int pos) { return iupwasmGoDispatch(ih, "CARET_CB", lin, col, pos, 0, 0); }
static int wasmCbGetFocus(Ihandle* ih) { return iupwasmGoDispatch(ih, "GETFOCUS_CB", 0, 0, 0, 0, 0); }
static int wasmCbKillFocus(Ihandle* ih) { return iupwasmGoDispatch(ih, "KILLFOCUS_CB", 0, 0, 0, 0, 0); }
static int wasmCbDropDown(Ihandle* ih, int state) { return iupwasmGoDispatch(ih, "DROPDOWN_CB", state, 0, 0, 0, 0); }
static int wasmCbEdit(Ihandle* ih, int c, char* after) { return iupwasmGoDispatch(ih, "EDIT_CB", c, 0, 0, 0, after); }
static int wasmCbButton(Ihandle* ih, int but, int pressed, int x, int y, char* status) { return iupwasmGoDispatch(ih, "BUTTON_CB", but, pressed, x, y, status); }
static int wasmCbTabChange(Ihandle* ih, Ihandle* nc, Ihandle* oc) { return iupwasmGoDispatch(ih, "TABCHANGE_CB", (int)(intptr_t)nc, (int)(intptr_t)oc, 0, 0, 0); }
static int wasmCbTabChangePos(Ihandle* ih, int np, int op) { return iupwasmGoDispatch(ih, "TABCHANGEPOS_CB", np, op, 0, 0, 0); }
static int wasmCbReorder(Ihandle* ih, int a, int b) { return iupwasmGoDispatch(ih, "REORDER_CB", a, b, 0, 0, 0); }
static int wasmCbTabClose(Ihandle* ih, int pos) { return iupwasmGoDispatch(ih, "TABCLOSE_CB", pos, 0, 0, 0, 0); }
static int wasmCbRightClick(Ihandle* ih, int pos) { return iupwasmGoDispatch(ih, "RIGHTCLICK_CB", pos, 0, 0, 0, 0); }
static int wasmCbActionCb(Ihandle* ih) { return iupwasmGoDispatch(ih, "ACTION_CB", 0, 0, 0, 0, 0); }
static int wasmCbNotify(Ihandle* ih, int id) { return iupwasmGoDispatch(ih, "NOTIFY_CB", id, 0, 0, 0, 0); }
static int wasmCbNotifyClose(Ihandle* ih, int reason) { return iupwasmGoDispatch(ih, "CLOSE_CB", reason, 0, 0, 0, 0); }
static int wasmCbClose(Ihandle* ih) { return iupwasmGoDispatch(ih, "CLOSE_CB", 0, 0, 0, 0, 0); }
static int wasmCbError(Ihandle* ih, char* msg) { return iupwasmGoDispatch(ih, "ERROR_CB", 0, 0, 0, 0, msg); }
static int wasmCbCompleted(Ihandle* ih, char* url) { return iupwasmGoDispatch(ih, "COMPLETED_CB", 0, 0, 0, 0, url); }
static int wasmCbNavigate(Ihandle* ih, char* url) { return iupwasmGoDispatch(ih, "NAVIGATE_CB", 0, 0, 0, 0, url); }
static int wasmCbNewWindow(Ihandle* ih, char* url) { return iupwasmGoDispatch(ih, "NEWWINDOW_CB", 0, 0, 0, 0, url); }
static int wasmCbUpdate(Ihandle* ih) { return iupwasmGoDispatch(ih, "UPDATE_CB", 0, 0, 0, 0, 0); }
/* Float-carrying dispatch for callbacks with double args (SCROLL_CB). */
EM_JS(int, iupwasmGoDispatchF, (Ihandle* ih, const char* name, int i1, double d1, double d2), {
  if (globalThis.iupGoDispatchF)
    return globalThis.iupGoDispatchF(ih, UTF8ToString(name), i1, d1, d2) | 0;
  return 0;
})

/* String-returning dispatch for callbacks that return char* (table VALUE_CB/IMAGE_CB).
   The result lives in a single recycled heap slot, valid until the next such call. */
EM_JS(char*, iupwasmGoDispatchStr, (Ihandle* ih, const char* name, int i1, int i2), {
  if (!globalThis.iupGoDispatchStr) return 0;
  var s = globalThis.iupGoDispatchStr(ih, UTF8ToString(name), i1, i2);
  if (s == null) s = "";
  if (globalThis.__iupStrRet) _free(globalThis.__iupStrRet);
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  globalThis.__iupStrRet = ptr;
  return ptr;
})

static char* wasmCbTableValue(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchStr(ih, "VALUE_CB", lin, col); }
static char* wasmCbTableImage(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchStr(ih, "IMAGE_CB", lin, col); }
static int wasmCbTableSort(Ihandle* ih, int col) { return iupwasmGoDispatch(ih, "SORT_CB", col, 0, 0, 0, 0); }
static char* wasmCbListValue(Ihandle* ih, int pos) { return iupwasmGoDispatchStr(ih, "VALUE_CB", pos, 0); }
static char* wasmCbListImage(Ihandle* ih, int pos) { return iupwasmGoDispatchStr(ih, "IMAGE_CB", pos, 0); }

static int wasmCbScroll(Ihandle* ih, int op, float posx, float posy) { return iupwasmGoDispatchF(ih, "SCROLL_CB", op, posx, posy); }
static int wasmCbTableClick(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "CLICK_CB", lin, col, 0, 0, status); }
static int wasmCbEnterItem(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "ENTERITEM_CB", lin, col, 0, 0, 0); }
static int wasmCbTableEdition(Ihandle* ih, int lin, int col, char* text) { return iupwasmGoDispatch(ih, "EDITION_CB", lin, col, 0, 0, text); }
static int wasmCbTableValueChanged(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "VALUECHANGED_CB", lin, col, 0, 0, 0); }
static int wasmCbThemeChanged(Ihandle* ih, int dark) { return iupwasmGoDispatch(ih, "THEMECHANGED_CB", dark, 0, 0, 0, 0); }
static int wasmCbShow(Ihandle* ih, int state) { return iupwasmGoDispatch(ih, "SHOW_CB", state, 0, 0, 0, 0); }
static int wasmCbDragDrop(Ihandle* ih, int dragId, int dropId, int isShift, int isCtrl) { return iupwasmGoDispatch(ih, "DRAGDROP_CB", dragId, dropId, isShift, isCtrl, 0); }
static int wasmCbSelection(Ihandle* ih, int id, int status) { return iupwasmGoDispatch(ih, "SELECTION_CB", id, status, 0, 0, 0); }
static int wasmCbBranchOpen(Ihandle* ih, int id) { return iupwasmGoDispatch(ih, "BRANCHOPEN_CB", id, 0, 0, 0, 0); }
static int wasmCbBranchClose(Ihandle* ih, int id) { return iupwasmGoDispatch(ih, "BRANCHCLOSE_CB", id, 0, 0, 0, 0); }
static int wasmCbExecuteLeaf(Ihandle* ih, int id) { return iupwasmGoDispatch(ih, "EXECUTELEAF_CB", id, 0, 0, 0, 0); }
static int wasmCbExecuteBranch(Ihandle* ih, int id) { return iupwasmGoDispatch(ih, "EXECUTEBRANCH_CB", id, 0, 0, 0, 0); }
static int wasmCbShowRename(Ihandle* ih, int id) { return iupwasmGoDispatch(ih, "SHOWRENAME_CB", id, 0, 0, 0, 0); }
static int wasmCbRename(Ihandle* ih, int id, char* title) { return iupwasmGoDispatch(ih, "RENAME_CB", id, 0, 0, 0, title); }
static int wasmCbNodeRemoved(Ihandle* ih, void* userdata) { return iupwasmGoDispatch(ih, "NODEREMOVED_CB", (int)(intptr_t)userdata, 0, 0, 0, 0); }
/* id arrays serialize to "id;id;..." for the single string slot Go parses */
static int wasmCbTreeMultiSel(Ihandle* ih, const char* name, int* ids, int n)
{
  char buf[1024]; int i, len = 0;
  for (i = 0; i < n && len < (int)sizeof(buf) - 16; i++)
    len += snprintf(buf + len, sizeof(buf) - len, "%d;", ids[i]);
  return iupwasmGoDispatch(ih, name, n, 0, 0, 0, buf);
}
static int wasmCbMultiSelection(Ihandle* ih, int* ids, int n) { return wasmCbTreeMultiSel(ih, "MULTISELECTION_CB", ids, n); }
static int wasmCbMultiUnselection(Ihandle* ih, int* ids, int n) { return wasmCbTreeMultiSel(ih, "MULTIUNSELECTION_CB", ids, n); }
static int wasmCbColorChange(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b) { return iupwasmGoDispatch(ih, "CHANGE_CB", r, g, b, 0, 0); }
static int wasmCbColorDrag(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b) { return iupwasmGoDispatch(ih, "DRAG_CB", r, g, b, 0, 0); }
static int wasmCbDragBegin(Ihandle* ih, int x, int y) { return iupwasmGoDispatch(ih, "DRAGBEGIN_CB", x, y, 0, 0, 0); }
static int wasmCbDragEnd(Ihandle* ih, int action) { return iupwasmGoDispatch(ih, "DRAGEND_CB", action, 0, 0, 0, 0); }
static int wasmCbDropMotion(Ihandle* ih, int x, int y, char* status) { return iupwasmGoDispatch(ih, "DROPMOTION_CB", x, y, 0, 0, status); }
static int wasmCbDropFiles(Ihandle* ih, char* name, int num, int x, int y) { return iupwasmGoDispatch(ih, "DROPFILES_CB", num, x, y, 0, name); }
static int wasmCbDropData(Ihandle* ih, char* type, void* data, int size, int x, int y) { return iupwasmGoDispatch(ih, "DROPDATA_CB", (int)(intptr_t)data, size, x, y, type); }
static int wasmCbMap(Ihandle* ih) { return iupwasmGoDispatch(ih, "MAP_CB", 0, 0, 0, 0, 0); }
static int wasmCbUnmap(Ihandle* ih) { return iupwasmGoDispatch(ih, "UNMAP_CB", 0, 0, 0, 0, 0); }
static int wasmCbHighlight(Ihandle* ih) { return iupwasmGoDispatch(ih, "HIGHLIGHT_CB", 0, 0, 0, 0, 0); }
static int wasmCbMenuOpen(Ihandle* ih) { return iupwasmGoDispatch(ih, "MENUOPEN_CB", 0, 0, 0, 0, 0); }
static int wasmCbMenuClose(Ihandle* ih) { return iupwasmGoDispatch(ih, "MENUCLOSE_CB", 0, 0, 0, 0, 0); }
static int wasmCbResize(Ihandle* ih, int w, int h) { return iupwasmGoDispatch(ih, "RESIZE_CB", w, h, 0, 0, 0); }
static int wasmCbMotion(Ihandle* ih, int x, int y, char* st) { return iupwasmGoDispatch(ih, "MOTION_CB", x, y, 0, 0, st); }
static int wasmCbWheel(Ihandle* ih, float delta, int x, int y, char* st) { return iupwasmGoDispatch(ih, "WHEEL_CB", (int)delta, x, y, 0, st); }
static int wasmCbEnterWindow(Ihandle* ih) { return iupwasmGoDispatch(ih, "ENTERWINDOW_CB", 0, 0, 0, 0, 0); }
static int wasmCbLeaveWindow(Ihandle* ih) { return iupwasmGoDispatch(ih, "LEAVEWINDOW_CB", 0, 0, 0, 0, 0); }
static int wasmCbHelp(Ihandle* ih) { return iupwasmGoDispatch(ih, "HELP_CB", 0, 0, 0, 0, 0); }

EM_JS(int, iupwasmGoDispatchGesture, (Ihandle* ih, int gesture, int state, int x, int y, double v1, double v2), {
  if (globalThis.iupGoDispatchGesture)
    return globalThis.iupGoDispatchGesture(ih, gesture, state, x, y, v1, v2) | 0;
  return 0;
})
static int wasmCbGesture(Ihandle* ih, int g, int s, int x, int y, double v1, double v2) { return iupwasmGoDispatchGesture(ih, g, s, x, y, v1, v2); }
static int wasmCbTouch(Ihandle* ih, int id, int x, int y, char* st) { return iupwasmGoDispatch(ih, "TOUCH_CB", id, x, y, 0, st); }
/* multitouch arrays serialize to "id,x,y,state;..." for the single string slot Go parses */
static int wasmCbMultiTouch(Ihandle* ih, int count, int* ids, int* xs, int* ys, int* states)
{
  char buf[1024]; int i, n = 0;
  for (i = 0; i < count && n < (int)sizeof(buf) - 32; i++)
    n += snprintf(buf + n, sizeof(buf) - n, "%d,%d,%d,%d;", ids[i], xs[i], ys[i], states[i]);
  return iupwasmGoDispatch(ih, "MULTITOUCH_CB", count, 0, 0, 0, buf);
}

/* ACTION/CLOSE_CB have per-class signatures, disambiguated by class name. */
EMSCRIPTEN_KEEPALIVE void iupwasmGoSetCallback(Ihandle* ih, const char* name)
{
  if (strcmp(name, "ACTION") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "toggle") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbToggleAction);
    else if (cls && (strcmp(cls, "text") == 0 || strcmp(cls, "multiline") == 0))
      IupSetCallback(ih, name, (Icallback)wasmCbTextAction);
    else if (cls && strcmp(cls, "list") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbListAction);
    else if (cls && strcmp(cls, "link") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbLinkAction);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbAction);
  }
  else if (strcmp(name, "VALUECHANGED_CB") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "table") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbTableValueChanged);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbValueChanged);
  }
  else if (strcmp(name, "CLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTableClick);
  else if (strcmp(name, "ENTERITEM_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEnterItem);
  else if (strcmp(name, "EDITION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTableEdition);
  else if (strcmp(name, "THEMECHANGED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbThemeChanged);
  else if (strcmp(name, "SHOW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbShow);
  else if (strcmp(name, "DRAGDROP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDragDrop);
  else if (strcmp(name, "SELECTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbSelection);
  else if (strcmp(name, "BRANCHOPEN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbBranchOpen);
  else if (strcmp(name, "BRANCHCLOSE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbBranchClose);
  else if (strcmp(name, "EXECUTELEAF_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbExecuteLeaf);
  else if (strcmp(name, "EXECUTEBRANCH_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbExecuteBranch);
  else if (strcmp(name, "SHOWRENAME_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbShowRename);
  else if (strcmp(name, "RENAME_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbRename);
  else if (strcmp(name, "NODEREMOVED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNodeRemoved);
  else if (strcmp(name, "MULTISELECTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMultiSelection);
  else if (strcmp(name, "MULTIUNSELECTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMultiUnselection);
  else if (strcmp(name, "DRAGBEGIN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDragBegin);
  else if (strcmp(name, "DRAGEND_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDragEnd);
  else if (strcmp(name, "DROPMOTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropMotion);
  else if (strcmp(name, "DROPFILES_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropFiles);
  else if (strcmp(name, "DROPDATA_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropData);
  else if (strcmp(name, "K_ANY") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbKAny);
  else if (strcmp(name, "DBLCLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDblclick);
  else if (strcmp(name, "MULTISELECT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMultiselect);
  else if (strcmp(name, "CARET_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbCaret);
  else if (strcmp(name, "GETFOCUS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbGetFocus);
  else if (strcmp(name, "KILLFOCUS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbKillFocus);
  else if (strcmp(name, "DROPDOWN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropDown);
  else if (strcmp(name, "EDIT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEdit);
  else if (strcmp(name, "BUTTON_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbButton);
  else if (strcmp(name, "CHANGE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbColorChange);
  else if (strcmp(name, "DRAG_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbColorDrag);
  else if (strcmp(name, "TABCHANGE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTabChange);
  else if (strcmp(name, "TABCHANGEPOS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTabChangePos);
  else if (strcmp(name, "REORDER_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbReorder);
  else if (strcmp(name, "TABCLOSE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTabClose);
  else if (strcmp(name, "RIGHTCLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbRightClick);
  else if (strcmp(name, "ACTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbActionCb);
  else if (strcmp(name, "NOTIFY_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNotify);
  else if (strcmp(name, "ERROR_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbError);
  else if (strcmp(name, "COMPLETED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbCompleted);
  else if (strcmp(name, "NAVIGATE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNavigate);
  else if (strcmp(name, "NEWWINDOW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNewWindow);
  else if (strcmp(name, "UPDATE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbUpdate);
  else if (strcmp(name, "CLOSE_CB") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "notify") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbNotifyClose);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbClose);
  }
  else if (strcmp(name, "MAP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMap);
  else if (strcmp(name, "UNMAP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbUnmap);
  else if (strcmp(name, "HIGHLIGHT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbHighlight);
  else if (strcmp(name, "MENUOPEN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMenuOpen);
  else if (strcmp(name, "MENUCLOSE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMenuClose);
  else if (strcmp(name, "RESIZE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbResize);
  else if (strcmp(name, "SCROLL_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbScroll);
  else if (strcmp(name, "MOTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMotion);
  else if (strcmp(name, "ENTERWINDOW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEnterWindow);
  else if (strcmp(name, "LEAVEWINDOW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbLeaveWindow);
  else if (strcmp(name, "HELP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbHelp);
  else if (strcmp(name, "WHEEL_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbWheel);
  else if (strcmp(name, "GESTURE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbGesture);
  else if (strcmp(name, "TOUCH_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTouch);
  else if (strcmp(name, "MULTITOUCH_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMultiTouch);
  else if (strcmp(name, "SPIN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbSpin);
  else if (strcmp(name, "NLINES_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNLines);
  else if (strcmp(name, "NCOLS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNCols);
  else if (strcmp(name, "WIDTH_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbWidth);
  else if (strcmp(name, "HEIGHT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbHeight);
  else if (strcmp(name, "DRAW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbCellsDraw);
  else if (strcmp(name, "VALUE_CB") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "list") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbListValue);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbTableValue);
  }
  else if (strcmp(name, "IMAGE_CB") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "list") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbListImage);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbTableImage);
  }
  else if (strcmp(name, "SORT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTableSort);
}

static int wasmCbGetParam(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;
  return iupwasmGoDispatch(dialog, "GETPARAM_ACTION_CB", param_index, 0, 0, 0, 0);
}

/* IupGetParam is variadic; Go marshals param_data and calls this fixed-arity form. */
EMSCRIPTEN_KEEPALIVE int iupwasmGetParamv(const char* title, const char* format, int has_action, int param_count, int param_extra, void** param_data)
{
  return IupGetParamv(title, has_action ? wasmCbGetParam : NULL, NULL, format, param_count, param_extra, param_data);
}

EM_JS(int, iupwasmGoDispatchIdle, (void), {
  if (globalThis.iupGoIdleDispatch)
    return globalThis.iupGoIdleDispatch() | 0;
  return -1;
})

static int wasmCbIdle(void) { return iupwasmGoDispatchIdle(); }

EMSCRIPTEN_KEEPALIVE void iupwasmGoSetIdle(int on)
{
  IupSetFunction("IDLE_ACTION", on ? (Icallback)wasmCbIdle : NULL);
}

EMSCRIPTEN_KEEPALIVE Ihandle* iupwasmTabs0(void) { return IupTabs(NULL); }

/* variadic constructors don't go through ccall; build empty, Go IupAppends children */
EMSCRIPTEN_KEEPALIVE Ihandle* iupwasmVbox0(void) { return IupVbox(NULL); }
EMSCRIPTEN_KEEPALIVE Ihandle* iupwasmHbox0(void) { return IupHbox(NULL); }
EMSCRIPTEN_KEEPALIVE Ihandle* iupwasmMenu0(void) { return IupMenu(NULL); }
