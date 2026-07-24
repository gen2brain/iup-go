/** \file
 * \brief WebAssembly Text
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_array.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_text.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsCreateText, (int multiline, int password, int spin, int formatting, int wordwrap), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createtext', id: id, multiline: multiline, password: password, spin: spin, formatting: formatting, wordwrap: wordwrap });
  return id;
})

/* SELECTION "l1,c1:l2,c2" (1-based, end-exclusive, col clamped to line end) -> a styled run. */
EM_JS(void, iupwasmJsTextAddTag, (int id, int l1, int c1, int l2, int c2, const char* css), {
  globalThis.__iupApply({ op: 'textaddtag', id: id, l1: l1, c1: c1, l2: l2, c2: c2, css: UTF8ToString(css) });
})

/* SELECTIONPOS "start:end" (0-based char offsets, used by the Markdown converter). */
EM_JS(void, iupwasmJsTextAddTagPos, (int id, int start, int end, const char* css), {
  globalThis.__iupApply({ op: 'textaddtagpos', id: id, start: start, end: end, css: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsTextAddImageTag, (int id, int l1, int c1, int l2, int c2, int imgId, int w, int h), {
  globalThis.__iupApply({ op: 'textaddimagetag', id: id, l1: l1, c1: c1, l2: l2, c2: c2, imgId: imgId, w: w, h: h });
})

EM_JS(void, iupwasmJsTextAddImageTagPos, (int id, int start, int end, int imgId, int w, int h), {
  globalThis.__iupApply({ op: 'textaddimagetagpos', id: id, start: start, end: end, imgId: imgId, w: w, h: h });
})

EM_JS(void, iupwasmJsTextAddAlign, (int id, int l1, int c1, int l2, int c2, const char* align), {
  globalThis.__iupApply({ op: 'textaddalign', id: id, l1: l1, c1: c1, l2: l2, c2: c2, align: UTF8ToString(align) });
})

EM_JS(void, iupwasmJsTextAddAlignPos, (int id, int start, int end, const char* align), {
  globalThis.__iupApply({ op: 'textaddalignpos', id: id, start: start, end: end, align: UTF8ToString(align) });
})

EM_JS(void, iupwasmJsTextAddPCss, (int id, int l1, int c1, int l2, int c2, const char* pcss), {
  globalThis.__iupApply({ op: 'textaddpcss', id: id, l1: l1, c1: c1, l2: l2, c2: c2, pcss: UTF8ToString(pcss) });
})

EM_JS(void, iupwasmJsTextAddPCssPos, (int id, int start, int end, const char* pcss), {
  globalThis.__iupApply({ op: 'textaddpcsspos', id: id, start: start, end: end, pcss: UTF8ToString(pcss) });
})

EM_JS(void, iupwasmJsTextAddNumbering, (int id, int l1, int c1, int l2, int c2, const char* kind, const char* nstyle), {
  globalThis.__iupApply({ op: 'textaddnumbering', id: id, l1: l1, c1: c1, l2: l2, c2: c2, kind: UTF8ToString(kind), nstyle: UTF8ToString(nstyle) });
})

EM_JS(void, iupwasmJsTextAddNumberingPos, (int id, int start, int end, const char* kind, const char* nstyle), {
  globalThis.__iupApply({ op: 'textaddnumberingpos', id: id, start: start, end: end, kind: UTF8ToString(kind), nstyle: UTF8ToString(nstyle) });
})

EM_JS(void, iupwasmJsTextAddLink, (int id, int l1, int c1, int l2, int c2, int idx), {
  globalThis.__iupApply({ op: 'textaddlink', id: id, l1: l1, c1: c1, l2: l2, c2: c2, idx: idx });
})

EM_JS(void, iupwasmJsTextAddLinkPos, (int id, int start, int end, int idx), {
  globalThis.__iupApply({ op: 'textaddlinkpos', id: id, start: start, end: end, idx: idx });
})

EM_JS(void, iupwasmJsTextSpinRange, (int id, int min, int max, int step), {
  globalThis.__iupApply({ op: 'textspinrange', id: id, min: min, max: max, step: step });
})

EM_JS(void, iupwasmJsTextSpinWire, (int id), { globalThis.__iupApply({ op: 'textspinwire', id: id }); })

EM_JS(void, iupwasmJsTextSetValue, (int id, const char* v), {
  globalThis.__iupApply({ op: 'textsetvalue', id: id, value: UTF8ToString(v) });
})

EM_JS(void, iupwasmJsTextAppend, (int id, const char* v, int multiline), {
  globalThis.__iupApply({ op: 'textappend', id: id, value: UTF8ToString(v), multiline: multiline });
})

EM_JS(char*, iupwasmJsTextGetValue, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'textvalue', id: id });
  else { var el = globalThis.__iup.els[id]; s = el ? el.value : ""; }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(void, iupwasmJsTextSetReadOnly, (int id, int ro), {
  globalThis.__iupApply({ op: 'textreadonly', id: id, ro: ro });
})

EM_JS(void, iupwasmJsTextClearTags, (int id), { globalThis.__iupApply({ op: 'textcleartags', id: id }); })

EM_JS(void, iupwasmJsTextSetPlaceholder, (int id, const char* v), {
  globalThis.__iupApply({ op: 'textplaceholder', id: id, value: UTF8ToString(v) });
})

EM_JS(void, iupwasmJsTextSetAlign, (int id, const char* a), {
  globalThis.__iupApply({ op: 'textalign', id: id, align: UTF8ToString(a) });
})

EM_JS(void, iupwasmJsTextSetMaxLength, (int id, int n), {
  globalThis.__iupApply({ op: 'textmaxlen', id: id, n: n });
})

EM_JS(int, iupwasmJsTextGetCaret, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'textcaret', id: id });
  var el = globalThis.__iup.els[id];
  return el ? (el.selectionStart || 0) : 0;
})

EM_JS(void, iupwasmJsTextSetCaret, (int id, int pos), {
  globalThis.__iupApply({ op: 'textsetcaret', id: id, pos: pos });
})

EM_JS(void, iupwasmJsTextSetSelectionPos, (int id, int start, int end), {
  globalThis.__iupApply({ op: 'textsetselpos', id: id, start: start, end: end });
})

EM_JS(int, iupwasmJsTextGetSelectionPos, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'textselpos', id: id });
  else { var el = globalThis.__iup.els[id]; s = el ? ((el.selectionStart || 0) + ":" + (el.selectionEnd || 0)) : ""; }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(int, iupwasmJsTextGetSelectedText, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'textseltext', id: id });
  else { var el = globalThis.__iup.els[id]; s = el ? el.value.slice(el.selectionStart || 0, el.selectionEnd || 0) : ""; }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len);
  stringToUTF8(s, p, len);
  return p;
})

EM_JS(void, iupwasmJsTextInsert, (int id, const char* v), {
  globalThis.__iupApply({ op: 'textinsert', id: id, value: UTF8ToString(v) });
})

EM_JS(int, iupwasmJsTextCount, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'textcount', id: id });
  var el = globalThis.__iup.els[id];
  return el ? el.value.length : 0;
})

EM_JS(void, iupwasmJsTextSetTabSize, (int id, int n), {
  globalThis.__iupApply({ op: 'texttabsize', id: id, n: n });
})

EM_JS(void, iupwasmJsTextWire, (int id), { globalThis.__iupApply({ op: 'textwire', id: id }); })

/* lin/col forms compute against the live textarea value, so they are multiline-correct */
EM_JS(char*, iupwasmJsTextGetCaretLC, (int id), {
  var s = (typeof document === 'undefined') ? globalThis.__iupReadSync({ op: 'textcaretlc', id: id }) : "1,1";
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsTextSetCaretLC, (int id, int lin, int col), {
  globalThis.__iupApply({ op: 'textsetcaretlc', id: id, lin: lin, col: col });
})

EM_JS(char*, iupwasmJsTextGetSelectionLC, (int id), {
  var s = (typeof document === 'undefined') ? globalThis.__iupReadSync({ op: 'textsellc', id: id }) : "";
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsTextSetSelectionLC, (int id, int l1, int c1, int l2, int c2), {
  globalThis.__iupApply({ op: 'textsetsellc', id: id, l1: l1, c1: c1, l2: l2, c2: c2 });
})

EM_JS(int, iupwasmJsTextLineCount, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'textlinecount', id: id });
  var el = globalThis.__iup.els[id]; return el ? el.value.split('\n').length : 1;
})

EM_JS(char*, iupwasmJsTextGetLineValue, (int id), {
  var s = (typeof document === 'undefined') ? globalThis.__iupReadSync({ op: 'textlinevalue', id: id }) : "";
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsTextClipboard, (int id, const char* action), {
  globalThis.__iupApply({ op: 'textclipboard', id: id, action: UTF8ToString(action) });
})

EM_JS(void, iupwasmJsTextScrollToPos, (int id, int pos), {
  globalThis.__iupApply({ op: 'textscrolltopos', id: id, pos: pos });
})

EM_JS(void, iupwasmJsTextPassword, (int id, int on), {
  globalThis.__iupApply({ op: 'textpassword', id: id, on: on });
})

EM_JS(void, iupwasmJsTextFilter, (int id, const char* mode), {
  globalThis.__iupApply({ op: 'textfilter', id: id, mode: UTF8ToString(mode) });
})

EM_JS(void, iupwasmJsTextOverwrite, (int id, int on), {
  globalThis.__iupApply({ op: 'textoverwrite', id: id, on: on });
})

EM_JS(void, iupwasmJsTextScrollTo, (int id, int lin), {
  globalThis.__iupApply({ op: 'textscrollto', id: id, lin: lin });
})

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchTextAction(int id, int ch, const char* newvalue)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnis cb;
  if (!ih)
    return IUP_DEFAULT;
  cb = (IFnis)IupGetCallback(ih, "ACTION");
  if (cb)
    return cb(ih, ch, (char*)newvalue);
  return IUP_DEFAULT;
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchValueChanged(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
    cb(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchTextCaret(int id, int lin, int col, int pos)
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
    cb(ih, lin, col, pos);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchSpin(int id, int inc)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFni cb;
  if (!ih)
    return;
  cb = (IFni)IupGetCallback(ih, "SPIN_CB");
  if (cb)
    cb(ih, inc);
}

static int wasmTextSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTextSetValue(id, value ? value : "");
  return 0;
}

static char* wasmTextGetValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id)
    return NULL;
  p = iupwasmJsTextGetValue(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && value)
    iupwasmJsTextAppend(id, value, ih->data->is_multiline);
  return 0;
}

static int wasmTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && iupAttribGetBoolean(ih, "SPIN"))
    iupwasmJsTextSetValue(id, value ? value : "0");
  return 1;
}

static char* wasmTextGetSpinValueAttrib(Ihandle* ih)
{
  return iupAttribGetBoolean(ih, "SPIN") ? wasmTextGetValueAttrib(ih) : NULL;
}

static int wasmTextSetSpinRangeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id && iupAttribGetBoolean(ih, "SPIN"))
    iupwasmJsTextSpinRange(id, iupAttribGetInt(ih, "SPINMIN"), iupAttribGetInt(ih, "SPINMAX"), iupAttribGetInt(ih, "SPININC"));
  return 1;
}

static int wasmTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTextSetReadOnly(id, iupStrBoolean(value));
  return 1;
}

static int wasmTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && value)
    iupwasmJsTextSetPlaceholder(id, value);
  return 1;
}

static int wasmTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  const char* css = "left";
  if (iupStrEqualNoCase(value, "ARIGHT")) css = "right";
  else if (iupStrEqualNoCase(value, "ACENTER")) css = "center";
  if (id)
    iupwasmJsTextSetAlign(id, css);
  return 1;
}

static int wasmTextSetNCAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  int nc = 0;
  iupStrToInt(value, &nc);
  if (id)
    iupwasmJsTextSetMaxLength(id, nc);
  return 1;
}

static char* wasmTextGetCaretPosAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return id ? iupStrReturnInt(iupwasmJsTextGetCaret(id)) : NULL;
}

static int wasmTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 0;
  if (id && iupStrToInt(value, &pos))
    iupwasmJsTextSetCaret(id, pos);
  return 0;
}

static char* wasmTextGetSelectionPosAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id)
    return NULL;
  p = (char*)(intptr_t)iupwasmJsTextGetSelectionPos(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), start = 0, end = 0;
  if (id && iupStrToIntInt(value, &start, &end, ':') == 2)
    iupwasmJsTextSetSelectionPos(id, start, end);
  return 0;
}

static char* wasmTextGetSelectedTextAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id)
    return NULL;
  p = (char*)(intptr_t)iupwasmJsTextGetSelectedText(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && value)
    iupwasmJsTextInsert(id, value);
  return 0;
}

static char* wasmTextGetCountAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return id ? iupStrReturnInt(iupwasmJsTextCount(id)) : NULL;
}

static int wasmTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), h = 0, v = 0;
  iupStrToIntInt(value, &h, &v, 'x');
  if (id)
    iupwasmJsSetPadding(id, h, v);
  return 1;
}

static int wasmTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), n = 8;
  iupStrToInt(value, &n);
  if (id)
    iupwasmJsTextSetTabSize(id, n);
  return 1;
}

static char* wasmTextGetCaretAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id) return NULL;
  p = (char*)(intptr_t)iupwasmJsTextGetCaretLC(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), lin = 1, col = 1;
  if (id && value && iupStrToIntInt(value, &lin, &col, ',') > 0)
  {
    if (!ih->data->is_multiline) { col = lin; lin = 1; }  /* single-line CARET is just the column */
    iupwasmJsTextSetCaretLC(id, lin, col);
  }
  return 0;
}

static char* wasmTextGetSelectionAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id) return NULL;
  p = (char*)(intptr_t)iupwasmJsTextGetSelectionLC(id);
  if (!p || !*p) { free(p); return NULL; }
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), l1 = 1, c1 = 1, l2 = 1, c2 = 1;
  char p1[40] = "", p2[40] = "";
  if (!id || !value) return 0;
  if (iupStrEqualNoCase(value, "NONE")) { iupwasmJsTextSetSelectionLC(id, 0, 0, 0, 0); return 0; }
  if (iupStrEqualNoCase(value, "ALL")) { iupwasmJsTextSetSelectionLC(id, 1, 1, 999999, 999999); return 0; }
  iupStrToStrStr(value, p1, sizeof(p1), p2, sizeof(p2), ':');
  iupStrToIntInt(p1, &l1, &c1, ',');
  iupStrToIntInt(p2, &l2, &c2, ',');
  if (!ih->data->is_multiline) { c1 = l1; l1 = 1; c2 = l2; l2 = 1; }
  iupwasmJsTextSetSelectionLC(id, l1, c1, l2, c2);
  return 0;
}

static char* wasmTextGetLineCountAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (!id || !ih->data->is_multiline) return "1";
  return iupStrReturnInt(iupwasmJsTextLineCount(id));
}

static char* wasmTextGetLineValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id) return NULL;
  p = (char*)(intptr_t)iupwasmJsTextGetLineValue(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && value)
    iupwasmJsTextClipboard(id, value);
  return 0;
}

static int wasmTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), pos = 0;
  if (id && iupStrToInt(value, &pos))
    iupwasmJsTextScrollToPos(id, pos);
  return 0;
}

static int wasmTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && !ih->data->is_multiline)
    iupwasmJsTextPassword(id, iupStrBoolean(value));
  return 1;
}

static int wasmTextMapMethod(Ihandle* ih)
{
  char* val;
  int password = iupAttribGetBoolean(ih, "PASSWORD");
  int spin = iupAttribGetBoolean(ih, "SPIN") && !ih->data->is_multiline;
  int wordwrap = iupAttribGetBoolean(ih, "WORDWRAP");
  int id = iupwasmJsCreateText(ih->data->is_multiline, password && !ih->data->is_multiline, spin, ih->data->has_formatting, wordwrap);
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  if (spin)
  {
    iupwasmJsTextSpinRange(id, iupAttribGetInt(ih, "SPINMIN"), iupAttribGetInt(ih, "SPINMAX"), iupAttribGetInt(ih, "SPININC"));
    val = iupAttribGet(ih, "SPINVALUE");
    if (val)
      iupwasmJsTextSetValue(id, val);
    iupwasmJsTextSpinWire(id);
  }

  val = iupAttribGet(ih, "VALUE");
  if (val)
    iupwasmJsTextSetValue(id, val);
  if (iupAttribGetBoolean(ih, "READONLY"))
    iupwasmJsTextSetReadOnly(id, 1);
  val = iupAttribGet(ih, "CUEBANNER");
  if (val)
    iupwasmJsTextSetPlaceholder(id, val);

  if (ih->data->has_formatting)  /* apply format tags added before map */
    iupTextUpdateFormatTags(ih);

  iupwasmJsTextWire(id);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvTextAddBorders(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  if (w) *w += 6;
  if (h) *h += 6;  /* 2px border + 4px vertical padding; line box itself is VISIBLELINES*charheight */
}

IUP_SDK_API void iupdrvTextAddExtraPadding(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  (void)w;
  (void)h;
}

IUP_SDK_API void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  (void)ih;
  (void)h;
  if (w) *w += 18;  /* native number-input spin arrows */
}

EM_JS(int, iupwasmJsTextLcToPos, (int id, int lin, int col), {
  return globalThis.__iupReadSync({ op: 'textlctopos', id: id, lin: lin, col: col });
})

EM_JS(void, iupwasmJsTextPosToLc, (int id, int pos, int* lp, int* cp), {
  var r = globalThis.__iupReadSync({ op: 'textpostolc', id: id, pos: pos });
  HEAP32[lp >> 2] = r[0];
  HEAP32[cp >> 2] = r[1];
})

IUP_SDK_API void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  int id = iupwasmIdOf(ih);
  if (pos)
    *pos = id ? iupwasmJsTextLcToPos(id, lin, col) : ((col > 0) ? col - 1 : 0);
}

IUP_SDK_API void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  int id = iupwasmIdOf(ih);
  int l = 1, c = pos + 1;
  if (id)
    iupwasmJsTextPosToLc(id, pos, &l, &c);
  if (lin) *lin = l;
  if (col) *col = c;
}

IUP_SDK_API void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

IUP_SDK_API void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  (void)ih;
  (void)state;
}

IUP_SDK_API void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  int id = iupwasmIdOf(ih);
  int l1, c1, l2, c2, p1, p2, use_pos = 0;
  char css[320] = "";
  char* sel;
  char* selpos;
  char* v;
  unsigned char r, g, b;
  (void)bulk;

  if (!id || !ih->data->has_formatting)
    return;
  selpos = iupAttribGet(formattag, "SELECTIONPOS");
  sel = iupAttribGet(formattag, "SELECTION");
  if (selpos && sscanf(selpos, "%d:%d", &p1, &p2) == 2)
    use_pos = 1;
  else if (!sel || sscanf(sel, "%d,%d:%d,%d", &l1, &c1, &l2, &c2) != 4)
    return;

  v = iupAttribGet(formattag, "WEIGHT");
  if (v && (iupStrEqualNoCase(v, "BOLD") || iupStrEqualNoCase(v, "SEMIBOLD") || iupStrEqualNoCase(v, "HEAVY")))
    strcat(css, "font-weight:bold;");
  if (iupAttribGetBoolean(formattag, "ITALIC"))
    strcat(css, "font-style:italic;");
  {
    char deco[40] = "";
    v = iupAttribGet(formattag, "UNDERLINE");
    if (v && !iupStrEqualNoCase(v, "NONE"))
      strcat(deco, "underline ");
    if (iupAttribGetBoolean(formattag, "STRIKEOUT"))
      strcat(deco, "line-through ");
    if (deco[0])
    {
      strcat(css, "text-decoration:");
      strcat(css, deco);
      strcat(css, ";");
    }
  }
  v = iupAttribGet(formattag, "FGCOLOR");
  if (v && iupStrToRGB(v, &r, &g, &b))
  {
    char c[40];
    snprintf(c, sizeof(c), "color:rgb(%d,%d,%d);", r, g, b);
    strcat(css, c);
  }
  v = iupAttribGet(formattag, "BGCOLOR");
  if (v && iupStrToRGB(v, &r, &g, &b))
  {
    char c[50];
    snprintf(c, sizeof(c), "background-color:rgb(%d,%d,%d);", r, g, b);
    strcat(css, c);
  }
  v = iupAttribGet(formattag, "FONTSIZE");
  if (v)
  {
    int sz = 0;
    if (iupStrToInt(v, &sz) && sz > 0)
    {
      char c[30];
      snprintf(c, sizeof(c), "font-size:%dpt;", sz);
      strcat(css, c);
    }
  }
  v = iupAttribGet(formattag, "FONTSCALE");  /* Markdown headings scale relative to the base font */
  if (v)
  {
    double sc = 0;
    if (iupStrEqualNoCase(v, "XX-SMALL")) sc = 0.5787037037037;
    else if (iupStrEqualNoCase(v, "X-SMALL")) sc = 0.6444444444444;
    else if (iupStrEqualNoCase(v, "SMALL")) sc = 0.8333333333333;
    else if (iupStrEqualNoCase(v, "MEDIUM")) sc = 1.0;
    else if (iupStrEqualNoCase(v, "LARGE")) sc = 1.2;
    else if (iupStrEqualNoCase(v, "X-LARGE")) sc = 1.44;
    else if (iupStrEqualNoCase(v, "XX-LARGE")) sc = 1.728;
    else iupStrToDouble(v, &sc);
    if (sc > 0)
    {
      char c[30];
      snprintf(c, sizeof(c), "font-size:%.3gem;", sc);
      strcat(css, c);
    }
  }
  v = iupAttribGet(formattag, "FONTFACE");
  if (v && v[0])
  {
    char c[100];
    snprintf(c, sizeof(c), "font-family:%s;", v);
    strcat(css, c);
  }
  v = iupAttribGet(formattag, "RISE");
  if (v)
  {
    int rise = 0;
    if (iupStrEqualNoCase(v, "SUPERSCRIPT")) strcat(css, "vertical-align:super;");
    else if (iupStrEqualNoCase(v, "SUBSCRIPT")) strcat(css, "vertical-align:sub;");
    else if (iupStrToInt(v, &rise) && rise != 0)
    {
      char c[40];
      snprintf(c, sizeof(c), "vertical-align:%dpx;", rise);
      strcat(css, c);
    }
  }
  if (iupAttribGetBoolean(formattag, "SMALLCAPS"))
    strcat(css, "font-variant:small-caps;");

  v = iupAttribGet(formattag, "IMAGE");
  if (v && v[0])
  {
    void* img = iupImageGetImage(v, ih, 0, NULL);
    int imgId = (int)(intptr_t)img;
    int w = 0, hh = 0;
    iupStrToInt(iupAttribGet(formattag, "WIDTH"), &w);
    iupStrToInt(iupAttribGet(formattag, "HEIGHT"), &hh);
    if (imgId)
    {
      if (use_pos)
        iupwasmJsTextAddImageTagPos(id, p1, p2, imgId, w, hh);
      else
        iupwasmJsTextAddImageTag(id, l1, c1, l2, c2, imgId, w, hh);
    }
  }

  v = iupAttribGet(formattag, "ALIGNMENT");
  if (v)
  {
    const char* a = "left";
    if (iupStrEqualNoCase(v, "CENTER") || iupStrEqualNoCase(v, "ACENTER")) a = "center";
    else if (iupStrEqualNoCase(v, "RIGHT") || iupStrEqualNoCase(v, "ARIGHT")) a = "right";
    else if (iupStrEqualNoCase(v, "JUSTIFY") || iupStrEqualNoCase(v, "AJUSTIFY")) a = "justify";
    if (use_pos)
      iupwasmJsTextAddAlignPos(id, p1, p2, a);
    else
      iupwasmJsTextAddAlign(id, l1, c1, l2, c2, a);
  }

  {
    char pcss[160] = "";
    int n;
    v = iupAttribGet(formattag, "INDENT");
    if (v && iupStrToInt(v, &n) && n > 0)
    {
      char c[40];
      snprintf(c, sizeof(c), "padding-left:%dpx;", n);
      strcat(pcss, c);
    }
    v = iupAttribGet(formattag, "LINESPACING");
    if (v)
    {
      char c[40] = "";
      if (iupStrEqualNoCase(v, "SINGLE")) strcpy(c, "line-height:1;");
      else if (iupStrEqualNoCase(v, "ONEHALF")) strcpy(c, "line-height:1.5;");
      else if (iupStrEqualNoCase(v, "DOUBLE")) strcpy(c, "line-height:2;");
      else if (iupStrToInt(v, &n) && n > 0) snprintf(c, sizeof(c), "line-height:%dpx;", n);
      if (c[0]) strcat(pcss, c);
    }
    v = iupAttribGet(formattag, "SPACEBEFORE");
    if (v && iupStrToInt(v, &n) && n > 0)
    {
      char c[40];
      snprintf(c, sizeof(c), "margin-top:%dpx;", n);
      strcat(pcss, c);
    }
    v = iupAttribGet(formattag, "SPACEAFTER");
    if (v && iupStrToInt(v, &n) && n > 0)
    {
      char c[40];
      snprintf(c, sizeof(c), "margin-bottom:%dpx;", n);
      strcat(pcss, c);
    }
    if (pcss[0])
    {
      if (use_pos)
        iupwasmJsTextAddPCssPos(id, p1, p2, pcss);
      else
        iupwasmJsTextAddPCss(id, l1, c1, l2, c2, pcss);
    }
  }

  v = iupAttribGet(formattag, "NUMBERING");
  if (v && !iupStrEqualNoCase(v, "NONE"))
  {
    char* nstyle = iupAttribGet(formattag, "NUMBERINGSTYLE");
    if (!nstyle) nstyle = "";
    if (use_pos)
      iupwasmJsTextAddNumberingPos(id, p1, p2, v, nstyle);
    else
      iupwasmJsTextAddNumbering(id, l1, c1, l2, c2, v, nstyle);
  }

  v = iupAttribGet(formattag, "LINK");
  if (v && v[0])
  {
    int idx = iupAttribGetInt(ih, "_IUP_WASMLINK_COUNT");
    char name[40];
    snprintf(name, sizeof(name), "_IUP_WASMLINK_%d", idx);
    iupAttribSetStr(ih, name, v);
    iupAttribSetInt(ih, "_IUP_WASMLINK_COUNT", idx + 1);
    if (use_pos)
      iupwasmJsTextAddLinkPos(id, p1, p2, idx);
    else
      iupwasmJsTextAddLink(id, l1, c1, l2, c2, idx);
  }

  if (css[0])
  {
    if (use_pos)
      iupwasmJsTextAddTagPos(id, p1, p2, css);
    else
      iupwasmJsTextAddTag(id, l1, c1, l2, c2, css);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmTextLinkClick(int id, int idx)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  char name[40];
  char* url;
  IFns cb;
  if (!ih)
    return;
  snprintf(name, sizeof(name), "_IUP_WASMLINK_%d", idx);
  url = iupAttribGet(ih, name);
  if (!url)
    return;
  cb = (IFns)IupGetCallback(ih, "LINK_CB");
  if (cb)
    cb(ih, url);
}

static int wasmTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id && ih->data->is_multiline)
    iupwasmJsTextClearTags(id);
  return 0;
}

static int wasmTextSetFilterAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTextFilter(id, value ? value : "");
  return 1;
}

static int wasmTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTextOverwrite(id, iupStrBoolean(value));
  return 1;
}

static int wasmTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), lin = 1, col = 1;
  if (id && value)
  {
    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    iupwasmJsTextScrollTo(id, lin);
  }
  return 0;
}

IUP_SDK_API void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = wasmTextMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "VALUE", wasmTextGetValueAttrib, wasmTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", NULL, wasmTextSetReadOnlyAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, wasmTextSetSpinRangeAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, wasmTextSetSpinRangeAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, wasmTextSetSpinRangeAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", wasmTextGetSpinValueAttrib, wasmTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, wasmTextSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, wasmTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, wasmTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, wasmTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NC", NULL, wasmTextSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", wasmTextGetCaretPosAttrib, wasmTextSetCaretPosAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", wasmTextGetSelectionPosAttrib, wasmTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", wasmTextGetSelectedTextAttrib, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, wasmTextSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", wasmTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", NULL, wasmTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, wasmTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", wasmTextGetCaretAttrib, wasmTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", wasmTextGetSelectionAttrib, wasmTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", wasmTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", wasmTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, wasmTextSetClipboardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, wasmTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, wasmTextSetPasswordAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, wasmTextSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", NULL, wasmTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, wasmTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
}
