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

/* the only double-returning callback (matrix NUMERICGETVALUE_CB) */
EM_JS(double, iupwasmGoDispatchRetD, (Ihandle* ih, const char* name, int i1, int i2), {
  if (globalThis.iupGoDispatchRetD)
    return globalThis.iupGoDispatchRetD(ih, UTF8ToString(name), i1, i2);
  return 0;
})

/* two strings, a double and a status string: the plot tick formatters */
EM_JS(int, iupwasmGoDispatch2sds, (Ihandle* ih, const char* name, const char* s1, const char* s2,
                                   double d1, const char* s3), {
  if (globalThis.iupGoDispatch2sds)
    return globalThis.iupGoDispatch2sds(ih, UTF8ToString(name), s1 ? UTF8ToString(s1) : "",
      s2 ? UTF8ToString(s2) : "", d1, s3 ? UTF8ToString(s3) : "") | 0;
  return 0;
})

static char* wasmCbTableValue(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchStr(ih, "VALUE_CB", lin, col); }
static char* wasmCbTableImage(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchStr(ih, "IMAGE_CB", lin, col); }
static int wasmCbTableSort(Ihandle* ih, int col) { return iupwasmGoDispatch(ih, "SORT_CB", col, 0, 0, 0, 0); }
static char* wasmCbListValue(Ihandle* ih, int pos) { return iupwasmGoDispatchStr(ih, "VALUE_CB", pos, 0); }
static char* wasmCbListImage(Ihandle* ih, int pos) { return iupwasmGoDispatchStr(ih, "IMAGE_CB", pos, 0); }
static char* wasmCbCell(Ihandle* ih, int cell) { return iupwasmGoDispatchStr(ih, "CELL_CB", cell, 0); }
static double wasmCbNumericGetValue(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchRetD(ih, "NUMERICGETVALUE_CB", lin, col); }
static int wasmCbXTickFormatNumber(Ihandle* ih, char* fmt, char* out, double value, char* status) { return iupwasmGoDispatch2sds(ih, "XTICKFORMATNUMBER_CB", fmt, out, value, status); }
static int wasmCbYTickFormatNumber(Ihandle* ih, char* fmt, char* out, double value, char* status) { return iupwasmGoDispatch2sds(ih, "YTICKFORMATNUMBER_CB", fmt, out, value, status); }

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
static int wasmCbTerminalTitle(Ihandle* ih, char* title) { return iupwasmGoDispatch(ih, "TITLE_CB", 0, 0, 0, 0, title); }
static int wasmCbTerminalBell(Ihandle* ih) { return iupwasmGoDispatch(ih, "BELL_CB", 0, 0, 0, 0, 0); }
static int wasmCbTerminalSize(Ihandle* ih, int cols, int lines) { return iupwasmGoDispatch(ih, "TERMSIZE_CB", cols, lines, 0, 0, 0); }
static int wasmCbTerminalExit(Ihandle* ih, int status) { return iupwasmGoDispatch(ih, "EXIT_CB", status, 0, 0, 0, 0); }
static int wasmCbTextInput(Ihandle* ih, char* text) { return iupwasmGoDispatch(ih, "TEXTINPUT_CB", 0, 0, 0, 0, text); }

/* INPUT_CB bytes are not NUL terminated */
static int wasmCbTerminalInput(Ihandle* ih, char* bytes, int len)
{
  char buf[64];
  if (len < 0) len = 0;
  if (len > (int)sizeof(buf) - 1) len = (int)sizeof(buf) - 1;
  memcpy(buf, bytes, len);
  buf[len] = 0;
  return iupwasmGoDispatch(ih, "INPUT_CB", len, 0, 0, 0, buf);
}
static int wasmCbMotion(Ihandle* ih, int x, int y, char* st) { return iupwasmGoDispatch(ih, "MOTION_CB", x, y, 0, 0, st); }
static int wasmCbWheel(Ihandle* ih, float delta, int x, int y, char* st) { return iupwasmGoDispatch(ih, "WHEEL_CB", (int)delta, x, y, 0, st); }
static int wasmCbEnterWindow(Ihandle* ih) { return iupwasmGoDispatch(ih, "ENTERWINDOW_CB", 0, 0, 0, 0, 0); }
static int wasmCbLeaveWindow(Ihandle* ih) { return iupwasmGoDispatch(ih, "LEAVEWINDOW_CB", 0, 0, 0, 0, 0); }
static int wasmCbHelp(Ihandle* ih) { return iupwasmGoDispatch(ih, "HELP_CB", 0, 0, 0, 0, 0); }

EM_JS(int, iupwasmGoDispatch6s, (Ihandle* ih, const char* name, int i1, int i2, int i3, int i4, int i5, int i6, const char* sarg), {
  if (globalThis.iupGoDispatch6s)
    return globalThis.iupGoDispatch6s(ih, UTF8ToString(name), i1, i2, i3, i4, i5, i6, sarg ? UTF8ToString(sarg) : "") | 0;
  return 0;
})

static int wasmCbFlatAction(Ihandle* ih) { return iupwasmGoDispatch(ih, "FLAT_ACTION", 0, 0, 0, 0, 0); }
static int wasmCbFlatToggleAction(Ihandle* ih, int state) { return iupwasmGoDispatch(ih, "FLAT_ACTION", state, 0, 0, 0, 0); }
static int wasmCbFlatListAction(Ihandle* ih, char* t, int item, int state) { return iupwasmGoDispatch(ih, "FLAT_ACTION", item, state, 0, 0, t); }
static int wasmCbSelect(Ihandle* ih, int cell, int type) { return iupwasmGoDispatch(ih, "SELECT_CB", cell, type, 0, 0, 0); }
static int wasmCbSwitch(Ihandle* ih, int prim, int sec) { return iupwasmGoDispatch(ih, "SWITCH_CB", prim, sec, 0, 0, 0); }
static int wasmCbExtended(Ihandle* ih, int cell) { return iupwasmGoDispatch(ih, "EXTENDED_CB", cell, 0, 0, 0, 0); }
static int wasmCbToggleValue(Ihandle* ih, int id, int state) { return iupwasmGoDispatch(ih, "TOGGLEVALUE_CB", id, state, 0, 0, 0); }
static int wasmCbMatrixToggleValue(Ihandle* ih, int lin, int col, int value) { return iupwasmGoDispatch(ih, "TOGGLEVALUE_CB", lin, col, value, 0, 0); }
static int wasmCbOpenClose(Ihandle* ih, int state) { return iupwasmGoDispatch(ih, "OPENCLOSE_CB", state, 0, 0, 0, 0); }
static int wasmCbDetached(Ihandle* ih, Ihandle* newParent, int x, int y) { return iupwasmGoDispatch(ih, "DETACHED_CB", (int)(intptr_t)newParent, x, y, 0, 0); }
static int wasmCbRestored(Ihandle* ih, Ihandle* oldParent, int x, int y) { return iupwasmGoDispatch(ih, "RESTORED_CB", (int)(intptr_t)oldParent, x, y, 0, 0); }
static int wasmCbEditBegin(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "EDITBEGIN_CB", lin, col, 0, 0, 0); }
static int wasmCbEditEnd(Ihandle* ih, int lin, int col, char* value, int apply) { return iupwasmGoDispatch(ih, "EDITEND_CB", lin, col, apply, 0, value); }
static int wasmCbMove(Ihandle* ih, int x, int y) { return iupwasmGoDispatch(ih, "MOVE_CB", x, y, 0, 0, 0); }
static int wasmCbFocus(Ihandle* ih, int focus) { return iupwasmGoDispatch(ih, "FOCUS_CB", focus, 0, 0, 0, 0); }
static int wasmCbTrayClick(Ihandle* ih, int but, int pressed, int dclick) { return iupwasmGoDispatch(ih, "TRAYCLICK_CB", but, pressed, dclick, 0, 0); }
static int wasmCbThread(Ihandle* ih) { return iupwasmGoDispatch(ih, "THREAD_CB", 0, 0, 0, 0, 0); }
static int wasmCbKeyPress(Ihandle* ih, int c, int press) { return iupwasmGoDispatch(ih, "KEYPRESS_CB", c, press, 0, 0, 0); }
static int wasmCbTextLink(Ihandle* ih, char* url) { return iupwasmGoDispatch(ih, "LINK_CB", 0, 0, 0, 0, url); }
static int wasmCbCancel(Ihandle* ih) { return iupwasmGoDispatch(ih, "CANCEL_CB", 0, 0, 0, 0, 0); }
static int wasmCbLayoutUpdate(Ihandle* ih) { return iupwasmGoDispatch(ih, "LAYOUTUPDATE_CB", 0, 0, 0, 0, 0); }
static int wasmCbValueChanging(Ihandle* ih, int start) { return iupwasmGoDispatch(ih, "VALUECHANGING_CB", start, 0, 0, 0, 0); }
static int wasmCbMatrixMouseMove(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "MOUSEMOVE_CB", lin, col, 0, 0, 0); }
static int wasmCbLeaveItem(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "LEAVEITEM_CB", lin, col, 0, 0, 0); }
static int wasmCbDropCheck(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "DROPCHECK_CB", lin, col, 0, 0, 0); }
static int wasmCbMatrixRelease(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "RELEASE_CB", lin, col, 0, 0, status); }
static int wasmCbMark(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "MARK_CB", lin, col, 0, 0, 0); }
static int wasmCbMarkEdit(Ihandle* ih, int lin, int col, int marked) { return iupwasmGoDispatch(ih, "MARKEDIT_CB", lin, col, marked, 0, 0); }
static int wasmCbMouseMotion(Ihandle* ih, int i, int j, int x, int y, char* status) { return iupwasmGoDispatch(ih, "MOUSEMOTION_CB", i, j, x, y, status); }
static int wasmCbMouseClick(Ihandle* ih, int b, int pressed, int i, int j, int x, int y, char* status) { return iupwasmGoDispatch6s(ih, "MOUSECLICK_CB", b, pressed, i, j, x, y, status); }
static int wasmCbDragDataSize(Ihandle* ih, char* type) { return iupwasmGoDispatch(ih, "DRAGDATASIZE_CB", 0, 0, 0, 0, type); }
/* Go fills a buffer of its own and the dispatcher copies it into data, which lives in the module heap */
static int wasmCbDragData(Ihandle* ih, char* type, void* data, int size) { return iupwasmGoDispatch(ih, "DRAGDATA_CB", (int)(intptr_t)data, size, 0, 0, type); }
static int wasmCbDestroy(Ihandle* ih) { return iupwasmGoDispatch(ih, "DESTROY_CB", 0, 0, 0, 0, 0); }
static int wasmCbMaskFail(Ihandle* ih, char* value) { return iupwasmGoDispatch(ih, "MASKFAIL_CB", 0, 0, 0, 0, value); }
static int wasmCbSwapBuffers(Ihandle* ih) { return iupwasmGoDispatch(ih, "SWAPBUFFERS_CB", 0, 0, 0, 0, 0); }
static int wasmCbParam(Ihandle* ih, int index, void* userdata) { (void)userdata; return iupwasmGoDispatch(ih, "PARAM_CB", index, 0, 0, 0, 0); }

/* IupMatrix and IupMatrixEx */
static int wasmCbBusy(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "BUSY_CB", lin, col, 0, 0, status); }
static int wasmCbColorUpdate(Ihandle* ih) { return iupwasmGoDispatch(ih, "COLORUPDATE_CB", 0, 0, 0, 0, 0); }
static int wasmCbColResize(Ihandle* ih, int col) { return iupwasmGoDispatch(ih, "COLRESIZE_CB", col, 0, 0, 0, 0); }
static int wasmCbMatrixDrop(Ihandle* ih, Ihandle* drop, int lin, int col) { return iupwasmGoDispatch(ih, "DROP_CB", (int)(intptr_t)drop, lin, col, 0, 0); }
static int wasmCbDropSelect(Ihandle* ih, int lin, int col, Ihandle* drop, char* t, int item, int col2) { return iupwasmGoDispatch6s(ih, "DROPSELECT_CB", lin, col, (int)(intptr_t)drop, item, col2, 0, t); }
static int wasmCbDropShow(Ihandle* ih, int state) { return iupwasmGoDispatch(ih, "DROPSHOW_CB", state, 0, 0, 0, 0); }
static int wasmCbEditClick(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "EDITCLICK_CB", lin, col, 0, 0, status); }
static int wasmCbEditMouseMove(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "EDITMOUSEMOVE_CB", lin, col, 0, 0, 0); }
static int wasmCbEditRelease(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "EDITRELEASE_CB", lin, col, 0, 0, status); }
static int wasmCbHSpan(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "HSPAN_CB", lin, col, 0, 0, 0); }
static int wasmCbVSpan(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "VSPAN_CB", lin, col, 0, 0, 0); }
static int wasmCbMenuContext(Ihandle* ih, Ihandle* menu, int lin, int col) { return iupwasmGoDispatch(ih, "MENUCONTEXT_CB", (int)(intptr_t)menu, lin, col, 0, 0); }
static int wasmCbMenuContextClose(Ihandle* ih, Ihandle* menu, int lin, int col) { return iupwasmGoDispatch(ih, "MENUCONTEXTCLOSE_CB", (int)(intptr_t)menu, lin, col, 0, 0); }
static int wasmCbMenuDrop(Ihandle* ih, Ihandle* menu, int lin, int col) { return iupwasmGoDispatch(ih, "MENUDROP_CB", (int)(intptr_t)menu, lin, col, 0, 0); }
static int wasmCbPasteSize(Ihandle* ih, int numlin, int numcol) { return iupwasmGoDispatch(ih, "PASTESIZE_CB", numlin, numcol, 0, 0, 0); }
static int wasmCbResizeMatrix(Ihandle* ih, int w, int h) { return iupwasmGoDispatch(ih, "RESIZEMATRIX_CB", w, h, 0, 0, 0); }
static int wasmCbScrolling(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "SCROLLING_CB", lin, col, 0, 0, 0); }
static int wasmCbScrollTop(Ihandle* ih, int lin, int col) { return iupwasmGoDispatch(ih, "SCROLLTOP_CB", lin, col, 0, 0, 0); }
static int wasmCbSortColumnCompare(Ihandle* ih, int lin1, int lin2, int col) { return iupwasmGoDispatch(ih, "SORTCOLUMNCOMPARE_CB", lin1, lin2, col, 0, 0); }
static int wasmCbTips(Ihandle* ih, int x, int y) { return iupwasmGoDispatch(ih, "TIPS_CB", x, y, 0, 0, 0); }
static int wasmCbValueEdit(Ihandle* ih, int lin, int col, char* newval) { return iupwasmGoDispatch(ih, "VALUE_EDIT_CB", lin, col, 0, 0, newval); }
static char* wasmCbMatrixFont(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchStr(ih, "FONT_CB", lin, col); }
static char* wasmCbMatrixType(Ihandle* ih, int lin, int col) { return iupwasmGoDispatchStr(ih, "TYPE_CB", lin, col); }
static int wasmCbExtraButton(Ihandle* ih, int button, int pressed) { return iupwasmGoDispatch(ih, "EXTRABUTTON_CB", button, pressed, 0, 0, 0); }

/* IupMatrixList */
static int wasmCbImageValueChanged(Ihandle* ih, int item, int state) { return iupwasmGoDispatch(ih, "IMAGEVALUECHANGED_CB", item, state, 0, 0, 0); }
static int wasmCbListClick(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "LISTCLICK_CB", lin, col, 0, 0, status); }
static int wasmCbListDraw(Ihandle* ih, int lin, int col, int x1, int x2, int y1, int y2) { return iupwasmGoDispatch6(ih, "LISTDRAW_CB", lin, col, x1, x2, y1, y2); }
static int wasmCbListEdition(Ihandle* ih, int lin, int col, int mode, int update) { return iupwasmGoDispatch(ih, "LISTEDITION_CB", lin, col, mode, update, 0); }
static int wasmCbListInsert(Ihandle* ih, int pos) { return iupwasmGoDispatch(ih, "LISTINSERT_CB", pos, 0, 0, 0, 0); }
static int wasmCbListRelease(Ihandle* ih, int lin, int col, char* status) { return iupwasmGoDispatch(ih, "LISTRELEASE_CB", lin, col, 0, 0, status); }
static int wasmCbListRemove(Ihandle* ih, int pos) { return iupwasmGoDispatch(ih, "LISTREMOVE_CB", pos, 0, 0, 0, 0); }

/* IupPlot: the cd canvas argument is not exposed to Go */
static int wasmCbPlotDraw(Ihandle* ih, void* cnv) { (void)cnv; return iupwasmGoDispatch(ih, "PREDRAW_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotPostDraw(Ihandle* ih, void* cnv) { (void)cnv; return iupwasmGoDispatch(ih, "POSTDRAW_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotSelectBegin(Ihandle* ih) { return iupwasmGoDispatch(ih, "SELECTBEGIN_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotSelectEnd(Ihandle* ih) { return iupwasmGoDispatch(ih, "SELECTEND_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotDeleteBegin(Ihandle* ih) { return iupwasmGoDispatch(ih, "DELETEBEGIN_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotDeleteEnd(Ihandle* ih) { return iupwasmGoDispatch(ih, "DELETEEND_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotPropsChanged(Ihandle* ih) { return iupwasmGoDispatch(ih, "PROPERTIESCHANGED_CB", 0, 0, 0, 0, 0); }
static int wasmCbPlotDSPropsChanged(Ihandle* ih, int ds) { return iupwasmGoDispatch(ih, "DSPROPERTIESCHANGED_CB", ds, 0, 0, 0, 0); }

EM_JS(int, iupwasmGoDispatch2s, (Ihandle* ih, const char* name, const char* s1, const char* s2), {
  if (globalThis.iupGoDispatch2s)
    return globalThis.iupGoDispatch2s(ih, UTF8ToString(name), s1 ? UTF8ToString(s1) : "", s2 ? UTF8ToString(s2) : "") | 0;
  return 0;
})

/* widest shape: everything IupPlot's sample callbacks need */
EM_JS(int, iupwasmGoDispatchD, (Ihandle* ih, const char* name, int i1, int i2, int i3, int i4,
                                double d1, double d2, double d3, double d4, const char* sarg), {
  if (globalThis.iupGoDispatchD)
    return globalThis.iupGoDispatchD(ih, UTF8ToString(name), i1, i2, i3, i4, d1, d2, d3, d4,
                                     sarg ? UTF8ToString(sarg) : "") | 0;
  return 0;
})

EM_JS(char*, iupwasmGoDispatchStrS, (Ihandle* ih, const char* name, int i1, int i2, const char* sarg), {
  if (!globalThis.iupGoDispatchStrS) return 0;
  var s = globalThis.iupGoDispatchStrS(ih, UTF8ToString(name), i1, i2, sarg ? UTF8ToString(sarg) : "");
  if (s == null) s = "";
  if (globalThis.__iupStrRet2) _free(globalThis.__iupStrRet2);
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  globalThis.__iupStrRet2 = ptr;
  return ptr;
})

static int wasmCbFile(Ihandle* ih, char* filename, char* status) { return iupwasmGoDispatch2s(ih, "FILE_CB", filename, status); }
static int wasmCbPlotPropsValidate(Ihandle* ih, char* name, char* value) { return iupwasmGoDispatch2s(ih, "PROPERTIESVALIDATE_CB", name, value); }
static int wasmCbPostMessage(Ihandle* ih, char* s, int i, double d, void* p) { (void)d; (void)p; return iupwasmGoDispatch(ih, "POSTMESSAGE_CB", i, 0, 0, 0, s); }

static int wasmCbPlotButton(Ihandle* ih, int button, int pressed, double x, double y, char* status) { return iupwasmGoDispatchD(ih, "PLOTBUTTON_CB", button, pressed, 0, 0, x, y, 0, 0, status); }
static int wasmCbPlotMotion(Ihandle* ih, double x, double y, char* status) { return iupwasmGoDispatchD(ih, "PLOTMOTION_CB", 0, 0, 0, 0, x, y, 0, 0, status); }
static int wasmCbPlotClickSample(Ihandle* ih, int ds, int sample, double x, double y, int button) { return iupwasmGoDispatchD(ih, "CLICKSAMPLE_CB", ds, sample, button, 0, x, y, 0, 0, 0); }
static int wasmCbPlotClickSegment(Ihandle* ih, int ds, int s1, double x1, double y1, int s2, double x2, double y2, int button) { return iupwasmGoDispatchD(ih, "CLICKSEGMENT_CB", ds, s1, s2, button, x1, y1, x2, y2, 0); }
static int wasmCbPlotDelete(Ihandle* ih, int ds, int sample, double x, double y) { return iupwasmGoDispatchD(ih, "DELETE_CB", ds, sample, 0, 0, x, y, 0, 0, 0); }
static int wasmCbPlotDrawSample(Ihandle* ih, int ds, int sample, double x, double y, int state) { return iupwasmGoDispatchD(ih, "DRAWSAMPLE_CB", ds, sample, state, 0, x, y, 0, 0, 0); }
static int wasmCbPlotEditSample(Ihandle* ih, int ds, int sample, double x, double y) { return iupwasmGoDispatchD(ih, "EDITSAMPLE_CB", ds, sample, 0, 0, x, y, 0, 0, 0); }

static char* wasmCbTranslateValue(Ihandle* ih, int lin, int col, char* value) { return iupwasmGoDispatchStrS(ih, "TRANSLATEVALUE_CB", lin, col, value); }
static int wasmCbNumericSetValue(Ihandle* ih, int lin, int col, double value) { return iupwasmGoDispatchD(ih, "NUMERICSETVALUE_CB", lin, col, 0, 0, value, 0, 0, 0, 0); }
static int wasmCbPlotDSPropsValidate(Ihandle* ih, Ihandle* p1, Ihandle* p2, int ds) { return iupwasmGoDispatch(ih, "DSPROPERTIESVALIDATE_CB", (int)(intptr_t)p1, (int)(intptr_t)p2, ds, 0, 0); }

/* Go returns the color as "r g b", empty when the cell has none */
static int wasmCbMatrixColor(Ihandle* ih, const char* name, int lin, int col, int* r, int* g, int* b)
{
  char* s = iupwasmGoDispatchStr(ih, name, lin, col);
  int rr = 0, gg = 0, bb = 0;
  if (!s || !s[0] || sscanf(s, "%d %d %d", &rr, &gg, &bb) != 3)
    return IUP_IGNORE;
  *r = rr; *g = gg; *b = bb;
  return IUP_DEFAULT;
}
static int wasmCbBgColor(Ihandle* ih, int lin, int col, int* r, int* g, int* b) { return wasmCbMatrixColor(ih, "BGCOLOR_CB", lin, col, r, g, b); }
static int wasmCbFgColor(Ihandle* ih, int lin, int col, int* r, int* g, int* b) { return wasmCbMatrixColor(ih, "FGCOLOR_CB", lin, col, r, g, b); }

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
  else if (strcmp(name, "INPUT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTerminalInput);
  else if (strcmp(name, "TITLE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTerminalTitle);
  else if (strcmp(name, "BELL_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTerminalBell);
  else if (strcmp(name, "TERMSIZE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTerminalSize);
  else if (strcmp(name, "EXIT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTerminalExit);
  else if (strcmp(name, "TEXTINPUT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTextInput);
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
  else if (strcmp(name, "FLAT_ACTION") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "flattoggle") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbFlatToggleAction);
    else if (cls && strcmp(cls, "flatlist") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbFlatListAction);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbFlatAction);
  }
  else if (strcmp(name, "TOGGLEVALUE_CB") == 0)
  {
    const char* cls = IupGetClassName(ih);
    if (cls && strcmp(cls, "tree") == 0)
      IupSetCallback(ih, name, (Icallback)wasmCbToggleValue);
    else
      IupSetCallback(ih, name, (Icallback)wasmCbMatrixToggleValue);
  }
  else if (strcmp(name, "SELECT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbSelect);
  else if (strcmp(name, "SWITCH_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbSwitch);
  else if (strcmp(name, "EXTENDED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbExtended);
  else if (strcmp(name, "CELL_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbCell);
  else if (strcmp(name, "NUMERICGETVALUE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNumericGetValue);
  else if (strcmp(name, "XTICKFORMATNUMBER_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbXTickFormatNumber);
  else if (strcmp(name, "YTICKFORMATNUMBER_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbYTickFormatNumber);
  else if (strcmp(name, "OPENCLOSE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbOpenClose);
  else if (strcmp(name, "DETACHED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDetached);
  else if (strcmp(name, "RESTORED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbRestored);
  else if (strcmp(name, "EDITBEGIN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEditBegin);
  else if (strcmp(name, "EDITEND_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEditEnd);
  else if (strcmp(name, "MOVE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMove);
  else if (strcmp(name, "FOCUS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbFocus);
  else if (strcmp(name, "TRAYCLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTrayClick);
  else if (strcmp(name, "THREAD_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbThread);
  else if (strcmp(name, "KEYPRESS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbKeyPress);
  else if (strcmp(name, "LINK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTextLink);
  else if (strcmp(name, "CANCEL_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbCancel);
  else if (strcmp(name, "LAYOUTUPDATE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbLayoutUpdate);
  else if (strcmp(name, "VALUECHANGING_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbValueChanging);
  else if (strcmp(name, "MOUSEMOVE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMatrixMouseMove);
  else if (strcmp(name, "LEAVEITEM_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbLeaveItem);
  else if (strcmp(name, "DROPCHECK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropCheck);
  else if (strcmp(name, "RELEASE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMatrixRelease);
  else if (strcmp(name, "MARK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMark);
  else if (strcmp(name, "MARKEDIT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMarkEdit);
  else if (strcmp(name, "MOUSEMOTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMouseMotion);
  else if (strcmp(name, "MOUSECLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMouseClick);
  else if (strcmp(name, "DRAGDATASIZE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDragDataSize);
  else if (strcmp(name, "DRAGDATA_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDragData);
  else if (strcmp(name, "DESTROY_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDestroy);
  else if (strcmp(name, "MASKFAIL_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMaskFail);
  else if (strcmp(name, "SWAPBUFFERS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbSwapBuffers);
  else if (strcmp(name, "PARAM_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbParam);
  else if (strcmp(name, "BUSY_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbBusy);
  else if (strcmp(name, "COLORUPDATE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbColorUpdate);
  else if (strcmp(name, "COLRESIZE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbColResize);
  else if (strcmp(name, "DROP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMatrixDrop);
  else if (strcmp(name, "DROPSELECT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropSelect);
  else if (strcmp(name, "DROPSHOW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbDropShow);
  else if (strcmp(name, "EDITCLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEditClick);
  else if (strcmp(name, "EDITMOUSEMOVE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEditMouseMove);
  else if (strcmp(name, "EDITRELEASE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbEditRelease);
  else if (strcmp(name, "HSPAN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbHSpan);
  else if (strcmp(name, "VSPAN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbVSpan);
  else if (strcmp(name, "MENUCONTEXT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMenuContext);
  else if (strcmp(name, "MENUCONTEXTCLOSE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMenuContextClose);
  else if (strcmp(name, "MENUDROP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMenuDrop);
  else if (strcmp(name, "PASTESIZE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPasteSize);
  else if (strcmp(name, "RESIZEMATRIX_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbResizeMatrix);
  else if (strcmp(name, "SCROLLING_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbScrolling);
  else if (strcmp(name, "SCROLLTOP_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbScrollTop);
  else if (strcmp(name, "SORTCOLUMNCOMPARE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbSortColumnCompare);
  else if (strcmp(name, "TIPS_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTips);
  else if (strcmp(name, "VALUE_EDIT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbValueEdit);
  else if (strcmp(name, "FONT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMatrixFont);
  else if (strcmp(name, "TYPE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbMatrixType);
  else if (strcmp(name, "EXTRABUTTON_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbExtraButton);
  else if (strcmp(name, "IMAGEVALUECHANGED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbImageValueChanged);
  else if (strcmp(name, "LISTCLICK_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbListClick);
  else if (strcmp(name, "LISTDRAW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbListDraw);
  else if (strcmp(name, "LISTEDITION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbListEdition);
  else if (strcmp(name, "LISTINSERT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbListInsert);
  else if (strcmp(name, "LISTRELEASE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbListRelease);
  else if (strcmp(name, "LISTREMOVE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbListRemove);
  else if (strcmp(name, "PREDRAW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDraw);
  else if (strcmp(name, "POSTDRAW_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotPostDraw);
  else if (strcmp(name, "SELECTBEGIN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotSelectBegin);
  else if (strcmp(name, "SELECTEND_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotSelectEnd);
  else if (strcmp(name, "DELETEBEGIN_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDeleteBegin);
  else if (strcmp(name, "DELETEEND_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDeleteEnd);
  else if (strcmp(name, "PROPERTIESCHANGED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotPropsChanged);
  else if (strcmp(name, "DSPROPERTIESCHANGED_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDSPropsChanged);
  else if (strcmp(name, "FILE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbFile);
  else if (strcmp(name, "PROPERTIESVALIDATE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotPropsValidate);
  else if (strcmp(name, "POSTMESSAGE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPostMessage);
  else if (strcmp(name, "PLOTBUTTON_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotButton);
  else if (strcmp(name, "PLOTMOTION_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotMotion);
  else if (strcmp(name, "CLICKSAMPLE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotClickSample);
  else if (strcmp(name, "CLICKSEGMENT_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotClickSegment);
  else if (strcmp(name, "DELETE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDelete);
  else if (strcmp(name, "DRAWSAMPLE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDrawSample);
  else if (strcmp(name, "EDITSAMPLE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotEditSample);
  else if (strcmp(name, "TRANSLATEVALUE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbTranslateValue);
  else if (strcmp(name, "BGCOLOR_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbBgColor);
  else if (strcmp(name, "FGCOLOR_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbFgColor);
  else if (strcmp(name, "NUMERICSETVALUE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbNumericSetValue);
  else if (strcmp(name, "DSPROPERTIESVALIDATE_CB") == 0)
    IupSetCallback(ih, name, (Icallback)wasmCbPlotDSPropsValidate);
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
