/** \file
 * \brief Web Browser Control for WebAssembly (HTML <iframe>).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_drvfont.h"
#include "iup_webbrowser.h"

#include "iupwasm_drv.h"


struct _IcontrolData { int dummy; };

typedef int (*wasmWebUrlCb)(Ihandle*, char*);

EM_JS(void, iupwasmJsWebSetup, (int id), { globalThis.__iupApply({ op: 'websetup', id: id }); })

EM_JS(void, iupwasmJsWebSetUrl, (int id, const char* url), {
  globalThis.__iupApply({ op: 'webseturl', id: id, url: UTF8ToString(url) });
})

EM_JS(void, iupwasmJsWebSetHtml, (int id, const char* html), {
  globalThis.__iupApply({ op: 'websethtml', id: id, html: UTF8ToString(html) });
})

EM_JS(char*, iupwasmJsWebGetHtml, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'webgethtml', id: id });
  else { var el = globalThis.__iup.els[id]; s = ""; try { var d = el && el.contentDocument; if (d) s = "<html>" + d.documentElement.innerHTML + "</html>"; } catch (e) {} }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(char*, iupwasmJsWebGetUrl, (int id), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'webgeturl', id: id });
  else { var el = globalThis.__iup.els[id]; s = ""; try { var w = el && el.contentWindow; if (w) s = w.location.href; } catch (e) {} if (s === "about:blank") s = ""; }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsWebReload, (int id), { globalThis.__iupApply({ op: 'webreload', id: id }); })

EM_JS(void, iupwasmJsWebStop, (int id), { globalThis.__iupApply({ op: 'webstop', id: id }); })

EM_JS(void, iupwasmJsWebGo, (int id, int n), { globalThis.__iupApply({ op: 'webgo', id: id, n: n }); })

EM_JS(void, iupwasmJsWebPrint, (int id), { globalThis.__iupApply({ op: 'webprint', id: id }); })

EM_JS(void, iupwasmJsWebSetZoom, (int id, int percent), { globalThis.__iupApply({ op: 'webzoom', id: id, percent: percent }); })

EM_JS(char*, iupwasmJsWebEval, (int id, const char* code), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'webeval', id: id, code: UTF8ToString(code) });
  else { var el = globalThis.__iup.els[id]; s = ""; try { var r = el.contentWindow.eval(UTF8ToString(code)); if (r !== undefined && r !== null) s = "" + r; } catch (e) { s = "" + e; } }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsWebDesignMode, (int id, int on), { globalThis.__iupApply({ op: 'webdesignmode', id: id, on: on }); })

EM_JS(int, iupwasmJsWebGetDesignMode, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'webdesignmodeget', id: id });
  var el = globalThis.__iup.els[id];
  try { return (el.contentDocument.designMode === "on") ? 1 : 0; } catch (e) { return 0; }
})

EM_JS(int, iupwasmJsWebGetDirty, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'webdirty', id: id });
  var el = globalThis.__iup.els[id];
  return (el && el.__iupDirty) ? 1 : 0;
})

EM_JS(void, iupwasmJsWebClearDirty, (int id), { globalThis.__iupApply({ op: 'webcleardirty', id: id }); })

EM_JS(void, iupwasmJsWebExec, (int id, const char* cmd, const char* val), {
  globalThis.__iupApply({ op: 'webexec', id: id, cmd: UTF8ToString(cmd), val: val ? UTF8ToString(val) : null });
})

EM_JS(char*, iupwasmJsWebQuery, (int id, const char* cmd, int kind), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'webquery', id: id, cmd: UTF8ToString(cmd), kind: kind });
  else { var el = globalThis.__iup.els[id]; s = ""; try { var d = el.contentDocument, c = UTF8ToString(cmd); if (kind === 0) s = d.queryCommandState(c) ? "YES" : "NO"; else if (kind === 1) s = d.queryCommandEnabled(c) ? "YES" : "NO"; else s = "" + d.queryCommandValue(c); } catch (e) {} }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(char*, iupwasmJsWebGetInner, (int id, const char* elemId), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'webgetinner', id: id, elemId: UTF8ToString(elemId) });
  else { var el = globalThis.__iup.els[id]; s = ""; try { var n = el.contentDocument.getElementById(UTF8ToString(elemId)); if (n) s = n.innerText; } catch (e) {} }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsWebSetInner, (int id, const char* elemId, const char* text), {
  globalThis.__iupApply({ op: 'websetinner', id: id, elemId: UTF8ToString(elemId), text: UTF8ToString(text) });
})

EM_JS(char*, iupwasmJsWebGetAttr, (int id, const char* elemId, const char* name), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'webgetattr', id: id, elemId: UTF8ToString(elemId), name: UTF8ToString(name) });
  else { var el = globalThis.__iup.els[id]; s = ""; try { var n = el.contentDocument.getElementById(UTF8ToString(elemId)); if (n) s = n.getAttribute(UTF8ToString(name)) || ""; } catch (e) {} }
  var len = lengthBytesUTF8(s) + 1, p = _malloc(len); stringToUTF8(s, p, len); return p;
})

EM_JS(void, iupwasmJsWebSetAttr, (int id, const char* elemId, const char* name, const char* val), {
  globalThis.__iupApply({ op: 'websetattr', id: id, elemId: UTF8ToString(elemId), name: UTF8ToString(name), val: UTF8ToString(val) });
})

EM_JS(void, iupwasmJsWebFind, (int id), { globalThis.__iupApply({ op: 'webfind', id: id }); })

EMSCRIPTEN_KEEPALIVE void iupwasmWebOnLoad(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  wasmWebUrlCb cb;
  if (!ih)
    return;

  /* the freshly created iframe fires one load for about:blank; ignore it */
  if (!iupAttribGet(ih, "_IUPWEB_NAV"))
    return;

  iupAttribSet(ih, "_IUPWEB_STATUS", "COMPLETED");

  cb = (wasmWebUrlCb)IupGetCallback(ih, "COMPLETED_CB");
  if (cb)
  {
    char* url = iupAttribGet(ih, "_IUPWEB_URL");
    cb(ih, url ? url : (char*)"");
  }
}

static int wasmWebSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  wasmWebUrlCb cb;
  if (!id || !value)
    return 1;

  cb = (wasmWebUrlCb)IupGetCallback(ih, "NAVIGATE_CB");
  if (cb && cb(ih, (char*)value) == IUP_IGNORE)
    return 1;

  iupAttribSetStr(ih, "_IUPWEB_URL", value);
  iupAttribSet(ih, "_IUPWEB_NAV", "1");
  iupAttribSet(ih, "_IUPWEB_STATUS", "LOADING");
  iupwasmJsWebClearDirty(id);
  iupwasmJsWebSetUrl(id, value);
  return 1;
}

static char* wasmWebGetValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id)
    return NULL;
  p = iupwasmJsWebGetUrl(id);
  if (p && *p)
    r = iupStrReturnStr(p);
  else
    r = iupAttribGet(ih, "_IUPWEB_URL");
  free(p);
  return r;
}

static int wasmWebSetHtmlAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (!id)
    return 1;
  iupAttribSet(ih, "_IUPWEB_URL", NULL);
  iupAttribSet(ih, "_IUPWEB_NAV", "1");
  iupAttribSet(ih, "_IUPWEB_STATUS", "LOADING");
  iupwasmJsWebClearDirty(id);
  iupwasmJsWebSetHtml(id, value ? value : "");
  return 1;
}

static char* wasmWebGetHtmlAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* p, *r;
  if (!id)
    return NULL;
  p = iupwasmJsWebGetHtml(id);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmWebSetReloadAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebReload(id);
  return 0;
}

static int wasmWebSetStopAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebStop(id);
  return 0;
}

static int wasmWebSetBackForwardAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), n = 0;
  if (id && iupStrToInt(value, &n))
    iupwasmJsWebGo(id, n);
  return 0;
}

static int wasmWebSetGoBackAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebGo(id, -1);
  return 0;
}

static int wasmWebSetGoForwardAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebGo(id, 1);
  return 0;
}

static int wasmWebSetPrintAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebPrint(id);
  return 0;
}

static int wasmWebSetZoomAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih), percent = 100;
  if (id && iupStrToInt(value, &percent))
    iupwasmJsWebSetZoom(id, percent);
  return 1;
}

static char* wasmWebGetStatusAttrib(Ihandle* ih)
{
  char* s = iupAttribGet(ih, "_IUPWEB_STATUS");
  return s ? s : (char*)"COMPLETED";
}

static int wasmWebSetJavascriptAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char* p;
  if (!id || !value)
    return 0;
  p = iupwasmJsWebEval(id, value);
  iupAttribSetStr(ih, "_IUPWEB_JSRESULT", p);
  free(p);
  return 0;
}

static char* wasmWebGetJavascriptAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPWEB_JSRESULT");
}

static int wasmWebSetEditableAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id) iupwasmJsWebDesignMode(id, iupStrBoolean(value));
  return 1;
}

static char* wasmWebGetEditableAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return iupStrReturnBoolean(id ? iupwasmJsWebGetDesignMode(id) : 0);
}

static int wasmWebSetNewAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id)
  {
    iupAttribSet(ih, "_IUPWEB_NAV", "1");
    iupwasmJsWebClearDirty(id);
    iupwasmJsWebSetHtml(id, "<html><body></body></html>");
  }
  return 0;
}

static char* wasmWebGetDirtyAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return iupStrReturnBoolean(id ? iupwasmJsWebGetDirty(id) : 0);
}

static int wasmWebSetExecCommandAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && value) iupwasmJsWebExec(id, value, NULL);
  return 0;
}

/* write-only editing commands that take no argument */
static int wasmWebSetCommandAttrib(Ihandle* ih, const char* value, const char* cmd)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebExec(id, cmd, NULL);
  return 0;
}

static int wasmWebSetUndoAttrib(Ihandle* ih, const char* v)      { return wasmWebSetCommandAttrib(ih, v, "undo"); }
static int wasmWebSetRedoAttrib(Ihandle* ih, const char* v)      { return wasmWebSetCommandAttrib(ih, v, "redo"); }
static int wasmWebSetCutAttrib(Ihandle* ih, const char* v)       { return wasmWebSetCommandAttrib(ih, v, "cut"); }
static int wasmWebSetCopyAttrib(Ihandle* ih, const char* v)      { return wasmWebSetCommandAttrib(ih, v, "copy"); }
static int wasmWebSetPasteAttrib(Ihandle* ih, const char* v)     { return wasmWebSetCommandAttrib(ih, v, "paste"); }
static int wasmWebSetSelectAllAttrib(Ihandle* ih, const char* v) { return wasmWebSetCommandAttrib(ih, v, "selectAll"); }

/* write-only editing commands whose value is the argument */
static int wasmWebExecValueAttrib(Ihandle* ih, const char* value, const char* cmd)
{
  int id = iupwasmIdOf(ih);
  if (id && value) iupwasmJsWebExec(id, cmd, value);
  return 1;
}

static int wasmWebSetInsertTextAttrib(Ihandle* ih, const char* v)  { return wasmWebExecValueAttrib(ih, v, "insertText"); }
static int wasmWebSetInsertHtmlAttrib(Ihandle* ih, const char* v)  { return wasmWebExecValueAttrib(ih, v, "insertHTML"); }
static int wasmWebSetInsertImageAttrib(Ihandle* ih, const char* v) { return wasmWebExecValueAttrib(ih, v, "insertImage"); }
static int wasmWebSetCreateLinkAttrib(Ihandle* ih, const char* v)  { return wasmWebExecValueAttrib(ih, v, "createLink"); }
static int wasmWebSetFontNameAttrib(Ihandle* ih, const char* v)    { return wasmWebExecValueAttrib(ih, v, "fontName"); }
static int wasmWebSetFontSizeAttrib(Ihandle* ih, const char* v)    { return wasmWebExecValueAttrib(ih, v, "fontSize"); }
static int wasmWebSetFormatBlockAttrib(Ihandle* ih, const char* v) { return wasmWebExecValueAttrib(ih, v, "formatBlock"); }
static int wasmWebSetForeColorAttrib(Ihandle* ih, const char* v)   { return wasmWebExecValueAttrib(ih, v, "foreColor"); }
static int wasmWebSetBackColorAttrib(Ihandle* ih, const char* v)   { return wasmWebExecValueAttrib(ih, v, "hiliteColor"); }

static int wasmWebSetFindAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  (void)value;
  if (id) iupwasmJsWebFind(id);
  return 0;
}

static char* wasmWebQuery(Ihandle* ih, int kind)
{
  int id = iupwasmIdOf(ih);
  char* cmd = iupAttribGet(ih, "COMMAND");
  char* p, *r;
  if (!id || !cmd)
    return NULL;
  p = iupwasmJsWebQuery(id, cmd, kind);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static char* wasmWebGetCommandStateAttrib(Ihandle* ih)   { return wasmWebQuery(ih, 0); }
static char* wasmWebGetCommandEnabledAttrib(Ihandle* ih) { return wasmWebQuery(ih, 1); }
static char* wasmWebGetCommandValueAttrib(Ihandle* ih)   { return wasmWebQuery(ih, 2); }

static int wasmWebSetInnerTextAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  if (id && elem) iupwasmJsWebSetInner(id, elem, value ? value : "");
  return 0;
}

static char* wasmWebGetInnerTextAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  char* p, *r;
  if (!id || !elem)
    return NULL;
  p = iupwasmJsWebGetInner(id, elem);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmWebSetAttributeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  char* name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  if (id && elem && name) iupwasmJsWebSetAttr(id, elem, name, value ? value : "");
  return 0;
}

static char* wasmWebGetAttributeAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  char* elem = iupAttribGet(ih, "ELEMENT_ID");
  char* name = iupAttribGet(ih, "ATTRIBUTE_NAME");
  char* p, *r;
  if (!id || !elem || !name)
    return NULL;
  p = iupwasmJsWebGetAttr(id, elem, name);
  r = iupStrReturnStr(p);
  free(p);
  return r;
}

static int wasmWebMapMethod(Ihandle* ih)
{
  int id;
  if (!ih->parent)
    return IUP_ERROR;

  id = iupwasmJsCreate("iframe");
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  iupwasmJsWebSetup(id);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

static int wasmWebCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  ih->data = iupALLOCCTRLDATA();
  ih->expand = IUP_EXPAND_BOTH;
  return IUP_NOERROR;
}

static void wasmWebComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0, natural_h = 0;
  (void)children_expand;
  iupdrvFontGetCharSize(ih, &natural_w, &natural_h);
  *w = natural_w;
  *h = natural_h;
}

Iclass* iupWebBrowserNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "webbrowser";
  ic->cons = "WebBrowser";
  ic->format = NULL;
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;

  ic->New = iupWebBrowserNewClass;
  ic->Create = wasmWebCreateMethod;
  ic->Map = wasmWebMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->ComputeNaturalSize = wasmWebComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterCallback(ic, "NEWWINDOW_CB", "s");
  iupClassRegisterCallback(ic, "NAVIGATE_CB", "s");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");
  iupClassRegisterCallback(ic, "COMPLETED_CB", "s");
  iupClassRegisterCallback(ic, "UPDATE_CB", "");

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", wasmWebGetValueAttrib, wasmWebSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTML", wasmWebGetHtmlAttrib, wasmWebSetHtmlAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STATUS", wasmWebGetStatusAttrib, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", NULL, wasmWebSetZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "RELOAD", NULL, wasmWebSetReloadAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, wasmWebSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKFORWARD", NULL, wasmWebSetBackForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOBACK", NULL, wasmWebSetGoBackAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GOFORWARD", NULL, wasmWebSetGoForwardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINT", NULL, wasmWebSetPrintAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "JAVASCRIPT", wasmWebGetJavascriptAttrib, wasmWebSetJavascriptAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ELEMENT_ID", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE_NAME", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INNERTEXT", wasmWebGetInnerTextAttrib, wasmWebSetInnerTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ATTRIBUTE", wasmWebGetAttributeAttrib, wasmWebSetAttributeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", wasmWebGetEditableAttrib, wasmWebSetEditableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NEW", NULL, wasmWebSetNewAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXECCOMMAND", NULL, wasmWebSetExecCommandAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UNDO", NULL, wasmWebSetUndoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDO", NULL, wasmWebSetRedoAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUT", NULL, wasmWebSetCutAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPY", NULL, wasmWebSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTE", NULL, wasmWebSetPasteAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTALL", NULL, wasmWebSetSelectAllAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIND", NULL, wasmWebSetFindAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTTEXT", NULL, wasmWebSetInsertTextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTHTML", NULL, wasmWebSetInsertHtmlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERTIMAGE", NULL, wasmWebSetInsertImageAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CREATELINK", NULL, wasmWebSetCreateLinkAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTNAME", NULL, wasmWebSetFontNameAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONTSIZE", NULL, wasmWebSetFontSizeAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATBLOCK", NULL, wasmWebSetFormatBlockAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORECOLOR", NULL, wasmWebSetForeColorAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", NULL, wasmWebSetBackColorAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMMAND", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDSTATE", wasmWebGetCommandStateAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDENABLED", wasmWebGetCommandEnabledAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMMANDVALUE", wasmWebGetCommandValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DIRTY", wasmWebGetDirtyAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* no browser API: history not enumerable, no sync file access, no print preview */
  iupClassRegisterAttribute(ic, "BACKCOUNT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORWARDCOUNT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOBACK", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANGOFORWARD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "ITEMHISTORY", NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPENFILE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEFILE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PRINTPREVIEW", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  return ic;
}
