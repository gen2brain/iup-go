/** \file
 * \brief WebAssembly Base Functions
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
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_drv.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iupkey.h"

#include "iupwasm_drv.h"


/* shared DOM applier (web/iupwasm_dom.js, --pre-js): main mutates the DOM, a Worker forwards each command to main */
EM_JS(void, iupwasmJsInstallProxy, (void), {
  if (typeof document === 'undefined') {
    globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
    globalThis.__iupApply = function(c) { self.postMessage({ __iup: 1, cmd: c }); };
    return;
  }
  globalThis.__iupInstallApplier({
    call: function(name, args) { Module['_' + name].apply(null, args); },
    callRet: function(name, ret, types, args) { return Module.ccall(name, ret, types, args); },
    callType: function(name, types, args) { Module.ccall(name, null, types, args); },
    sync: true
  });
})

EM_JS(int, iupwasmJsCreate, (const char* tag), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'create', id: id, tag: UTF8ToString(tag) });
  return id;
})

EM_JS(void, iupwasmJsDestroy, (int id), { globalThis.__iupApply({ op: 'destroy', id: id }); })

EM_JS(void, iupwasmJsAddToBody, (int id), { globalThis.__iupApply({ op: 'addbody', id: id }); })

EM_JS(void, iupwasmJsAddChild, (int parent, int child), {
  globalThis.__iupApply({ op: 'addchild', pid: parent, id: child });
})

EM_JS(void, iupwasmJsSetPos, (int id, int x, int y, int w, int h), {
  globalThis.__iupApply({ op: 'pos', id: id, x: x, y: y, w: w, h: h });
})

EM_JS(void, iupwasmJsSetText, (int id, const char* txt), {
  globalThis.__iupApply({ op: 'text', id: id, text: UTF8ToString(txt) });
})

EM_JS(void, iupwasmJsSetVisible, (int id, int visible), {
  globalThis.__iupApply({ op: 'visible', id: id, v: visible });
})

EM_JS(void, iupwasmJsSetActive, (int id, int active), {
  globalThis.__iupApply({ op: 'active', id: id, a: active });
})

/* DOM stacking among absolutely-positioned siblings: top = move last, bottom = move first */
EM_JS(void, iupwasmJsSetZorder, (int id, int top), {
  globalThis.__iupApply({ op: 'zorder', id: id, top: top });
})

/* element origin in screen coords (viewport rect + window screen offset) */
EM_JS(void, iupwasmJsElemScreen, (int id, int ptr), {
  var a;
  if (typeof document === 'undefined') a = globalThis.__iupReadSync({ op: 'elemscreen', id: id });
  else {
    var el = globalThis.__iup.els[id];
    var r = el ? el.getBoundingClientRect() : { left: 0, top: 0 };
    a = [Math.round(r.left + (window.screenX || 0)), Math.round(r.top + (window.screenY || 0))];
  }
  HEAP32[ptr >> 2] = a[0];
  HEAP32[(ptr >> 2) + 1] = a[1];
})

EM_JS(void, iupwasmJsSetInactiveImage, (int id, int imgId, int isBg), {
  globalThis.__iupApply({ op: 'iminactive', id: id, imgId: imgId, isBg: isBg });
})

EM_JS(void, iupwasmJsSetBgColor, (int id, int r, int g, int b), {
  globalThis.__iupApply({ op: 'bg', id: id, r: r, g: g, b: b });
})

EM_JS(void, iupwasmJsSetFgColor, (int id, int r, int g, int b), {
  globalThis.__iupApply({ op: 'fg', id: id, r: r, g: g, b: b });
})

EM_JS(void, iupwasmJsSetBgVar, (int id, const char* css), {
  globalThis.__iupApply({ op: 'bgvar', id: id, css: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsSetFgVar, (int id, const char* css), {
  globalThis.__iupApply({ op: 'fgvar', id: id, css: UTF8ToString(css) });
})

/* color matching a theme global maps to its CSS var() so it tracks dark-mode; explicit colors stay fixed rgb */
const char* iupwasmThemeColorVar(unsigned char r, unsigned char g, unsigned char b, int is_bg)
{
  static const struct { const char* global; const char* var; int bg; } map[] = {
    { "DLGBGCOLOR", "var(--iup-bg)", 1 }, { "TXTBGCOLOR", "var(--iup-txtbg)", 1 }, { "MENUBGCOLOR", "var(--iup-menubg)", 1 },
    { "DLGFGCOLOR", "var(--iup-fg)", 0 }, { "TXTFGCOLOR", "var(--iup-txtfg)", 0 }, { "MENUFGCOLOR", "var(--iup-menufg)", 0 },
  };
  int i;
  for (i = 0; i < 6; i++)
  {
    unsigned char gr, gg, gb;
    if (map[i].bg != is_bg)
      continue;
    if (iupStrToRGB(IupGetGlobal(map[i].global), &gr, &gg, &gb) && gr == r && gg == g && gb == b)
      return map[i].var;
  }
  return NULL;
}

EM_JS(void, iupwasmJsWireClick, (int id), { globalThis.__iupApply({ op: 'wire', id: id }); })

EM_JS(void, iupwasmJsInstallMarkup, (void), {
  if (globalThis.__iupMarkup) return;
  globalThis.__iupMarkup = function(s) {
    var q = String.fromCharCode(34);
    s = s.replace(new RegExp('<span +([^>]*)>', 'g'), function(m, attrs) {
      var parts = [], are = new RegExp('([A-Za-z_]+) *= *' + q + '([^' + q + ']*)' + q, 'g'), mm;
      while ((mm = are.exec(attrs))) {
        var k = mm[1], v = mm[2];
        if (k === 'foreground' || k === 'color') parts.push('color:' + v);
        else if (k === 'background' || k === 'bgcolor') parts.push('background-color:' + v);
        else if (k === 'font_family' || k === 'face') parts.push('font-family:' + v);
        else if (k === 'font_size' || k === 'size') parts.push('font-size:' + v + 'pt');
        else if (k === 'font_weight' || k === 'weight') parts.push('font-weight:' + v);
        else if (k === 'font_style') parts.push('font-style:' + v);
        else if (k === 'underline') parts.push('text-decoration:underline');
        else if (k === 'strikethrough' && v === 'true') parts.push('text-decoration:line-through');
      }
      return '<span style=' + q + parts.join(';') + q + '>';
    });
    s = s.split('<big>').join('<span style=' + q + 'font-size:larger' + q + '>').split('</big>').join('</span>');
    return s.split('<small>').join('<span style=' + q + 'font-size:smaller' + q + '>').split('</small>').join('</span>');
  };
})

EM_JS(void, iupwasmJsSetMarkup, (int id, const char* txt), {
  globalThis.__iupApply({ op: 'markup', id: id, html: globalThis.__iupMarkup(UTF8ToString(txt)) });
})

EM_JS(void, iupwasmJsButtonStyle, (int id), { globalThis.__iupApply({ op: 'btnstyle', id: id }); })

EM_JS(void, iupwasmJsButtonAlign, (int id, const char* css), {
  globalThis.__iupApply({ op: 'btnalign', id: id, a: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsButtonSetText, (int id, const char* txt), {
  globalThis.__iupApply({ op: 'btntext', id: id, text: UTF8ToString(txt) });
})

/* image before label (IMAGEPOSITION=LEFT); gap is button SPACING */
EM_JS(void, iupwasmJsButtonSetImage, (int id, int imgId, int gap), {
  globalThis.__iupApply({ op: 'btnimg', id: id, imgId: imgId, gap: gap });
})

EM_JS(void, iupwasmJsButtonFlat, (int id), { globalThis.__iupApply({ op: 'btnflat', id: id }); })

/* imgId 0 clears the pressed image */
EM_JS(void, iupwasmJsButtonSetImpress, (int id, int imgId), {
  globalThis.__iupApply({ op: 'btnimpress', id: id, imgId: imgId });
})

EM_JS(void, iupwasmJsButtonDefault, (int id, int on), { globalThis.__iupApply({ op: 'btndefault', id: id, on: on }); })

EM_JS(void, iupwasmJsDialogStyle, (int id), { globalThis.__iupApply({ op: 'dlgstyle', id: id }); })

EM_JS(void, iupwasmJsSetDocTitle, (const char* txt), {
  globalThis.__iupApply({ op: 'doctitle', text: UTF8ToString(txt) });
})

EM_JS(void, iupwasmJsInstallKeyHandler, (void), {
  if (typeof document !== 'undefined' && globalThis.__iupInstallKeyHandler) globalThis.__iupInstallKeyHandler();
})

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchFocus(int id, int getfocus)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  if (!ih)
    return;
  if (getfocus)
    iupCallGetFocusCb(ih);  /* updates the core focus pointer so IupGetFocus works */
  else
    iupCallKillFocusCb(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchButton(int id, int but, int pressed, int x, int y)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniiiis cb;
  if (!ih)
    return;
  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    if (cb(ih, but, pressed, x, y, (char*)"") == IUP_CLOSE)
      IupExitLoop();
  }
}

void iupwasmFillStatus(char* status, int mods)
{
  strcpy(status, IUPKEY_STATUS_INIT);
  if (mods & 1) iupKEY_SETSHIFT(status);
  if (mods & 2) iupKEY_SETCONTROL(status);
  if (mods & 4) iupKEY_SETALT(status);
  if (mods & 8) iupKEY_SETBUTTON1(status);
  if (mods & 16) iupKEY_SETBUTTON2(status);
  if (mods & 32) iupKEY_SETBUTTON3(status);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchMotion(int id, int x, int y, int mods)
{
  Ihandle* ih = iupwasmHandleFromId(id);
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

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchEnter(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
    cb(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchLeave(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
    cb(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchHelp(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchKey(int id, int code)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  int result;
  if (!ih)
    return 0;
  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return 0;
  }
  if (result == IUP_IGNORE)
    return 1;

  /* DEFAULTENTER / DEFAULTESC: fire the named button's ACTION */
  if (code == K_CR || code == K_ESC)
  {
    Ihandle* dlg = IupGetDialog(ih);
    char* name = dlg ? iupAttribGet(dlg, code == K_ESC ? "DEFAULTESC" : "DEFAULTENTER") : NULL;
    if (name)
    {
      Ihandle* bt = IupGetHandle(name);
      if (bt)
      {
        Icallback acb = IupGetCallback(bt, "ACTION");
        if (acb && acb(bt) == IUP_CLOSE)
          IupExitLoop();
        return 1;
      }
    }
  }
  return 0;
}

EM_JS(int, iupwasmJsCreateToggle, (int isRadio, int group), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createtoggle', id: id, isRadio: isRadio, group: group });
  return id;
})

EM_JS(void, iupwasmJsToggleSetText, (int id, const char* txt), {
  globalThis.__iupApply({ op: 'toggletext', id: id, text: UTF8ToString(txt) });
})

EM_JS(void, iupwasmJsToggleSetValue, (int id, int on), {
  globalThis.__iupApply({ op: 'togglevalue', id: id, on: on });
})

EM_JS(void, iupwasmJsToggleImageMode, (int id), { globalThis.__iupApply({ op: 'toggleimg', id: id }); })

EM_JS(void, iupwasmJsToggleSwitchMode, (int id), { globalThis.__iupApply({ op: 'toggleswitch', id: id }); })

EM_JS(int, iupwasmJsToggleGetValue, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'togglevalue', id: id });
  var el = globalThis.__iup.els[id];
  if (el && el.__iupInput && el.__iupInput.indeterminate) return -1;
  return (el && el.__iupInput && el.__iupInput.checked) ? 1 : 0;
})

EM_JS(void, iupwasmJsToggleWire, (int id), { globalThis.__iupApply({ op: 'togglewire', id: id }); })

EM_JS(void, iupwasmJsSetPadding, (int id, int h, int v), {
  globalThis.__iupApply({ op: 'padding', id: id, h: h, v: v });
})

EM_JS(void, iupwasmJsSetTextAlign, (int id, const char* align), {
  globalThis.__iupApply({ op: 'textalign', id: id, align: UTF8ToString(align) });
})

EM_JS(void, iupwasmJsSetCursor, (int id, const char* css), {
  globalThis.__iupApply({ op: 'cursor', id: id, css: UTF8ToString(css) });
})

const char* iupwasmCssAlign(const char* iupalign)
{
  if (iupalign && (iupStrEqualNoCase(iupalign, "ARIGHT") || iupStrEqualNoCase(iupalign, "RIGHT")))
    return "right";
  if (iupalign && (iupStrEqualNoCase(iupalign, "ACENTER") || iupStrEqualNoCase(iupalign, "CENTER")))
    return "center";
  return "left";
}

char* iupwasmStripMnemonic(const char* title)
{
  char c;
  char* clean;
  if (!title)
    return iupStrDup("");
  clean = iupStrProcessMnemonic(title, &c, 0);
  if (clean == title)
    return iupStrDup(title);
  return clean;
}

EM_JS(void, iupwasmJsLabelStyle, (int id), { globalThis.__iupApply({ op: 'labelstyle', id: id }); })

EM_JS(void, iupwasmJsFrameStyle, (int id), { globalThis.__iupApply({ op: 'framestyle', id: id }); })

EM_JS(void, iupwasmJsFrameSetTitle, (int id, const char* txt), {
  globalThis.__iupApply({ op: 'frametitle', id: id, text: UTF8ToString(txt) });
})


static Ihandle** s_handles = NULL;
static unsigned char* s_visible = NULL;
static int s_handles_count = 0;

int iupwasmIdOf(Ihandle* ih)
{
  if (!ih || !ih->handle || ih->handle == (InativeHandle*)-1)
    return 0;
  return (int)(intptr_t)ih->handle;
}

void iupwasmRegisterHandle(int id, Ihandle* ih)
{
  if (id >= s_handles_count)
  {
    int i, n = id + 16;
    s_handles = (Ihandle**)realloc(s_handles, n * sizeof(Ihandle*));
    s_visible = (unsigned char*)realloc(s_visible, n * sizeof(unsigned char));
    for (i = s_handles_count; i < n; i++)
    {
      s_handles[i] = NULL;
      s_visible[i] = 1;
    }
    s_handles_count = n;
  }
  s_handles[id] = ih;
  /* dialogs are mapped hidden and shown later; other widgets default visible */
  s_visible[id] = (ih && ih->iclass->nativetype == IUP_TYPEDIALOG) ? 0 : 1;
}

void iupwasmSetVisibleState(int id, int visible)
{
  if (id > 0 && id < s_handles_count)
    s_visible[id] = visible ? 1 : 0;
}

void iupwasmUnregisterHandle(int id)
{
  if (id > 0 && id < s_handles_count)
    s_handles[id] = NULL;
}

Ihandle* iupwasmHandleFromId(int id)
{
  if (id > 0 && id < s_handles_count)
    return s_handles[id];
  return NULL;
}

void iupwasmAddToParent(Ihandle* ih)
{
  /* resolves inner containers (e.g. a tab page) so the child lands in its real container */
  InativeHandle* parent_handle = iupChildTreeGetNativeParentHandle(ih);
  int pid = (int)(intptr_t)parent_handle;
  int cid = iupwasmIdOf(ih);
  if (pid > 0 && cid)
    iupwasmJsAddChild(pid, cid);
}

static void iupwasmMenuItemToggle(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    Ihandle* child;
    for (child = ih->parent->firstchild; child; child = child->brother)
      if (iupStrEqual(child->iclass->name, "menuitem"))
        IupSetAttribute(child, "VALUE", child == ih ? "ON" : "OFF");
  }
  else if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
    IupSetAttribute(ih, "VALUE", iupAttribGetBoolean(ih, "VALUE") ? "OFF" : "ON");
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchAction(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;

  if (iupStrEqual(ih->iclass->name, "menuitem"))
    iupwasmMenuItemToggle(ih);

  cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}


IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  (void)ih;
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  iupwasmAddToParent(ih);
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetPos(id, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    iupwasmUnregisterHandle(id);
    iupwasmJsDestroy(id);
  }
}

/* canvas and its subclasses (colorbrowser, dial, gauge, cells, matrix) all redraw via ACTION */
IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{
  if (ih->iclass->nativetype == IUP_TYPECANVAS)
    iupwasmCanvasRedraw(ih);
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{
  if (ih->iclass->nativetype == IUP_TYPECANVAS)
    iupwasmCanvasRedraw(ih);
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  int id = iupwasmIdOf(ih);
  int origin[2] = { 0, 0 };
  if (id)
    iupwasmJsElemScreen(id, (int)(intptr_t)origin);
  *x -= origin[0];
  *y -= origin[1];
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  int id = iupwasmIdOf(ih);
  int origin[2] = { 0, 0 };
  if (id)
    iupwasmJsElemScreen(id, (int)(intptr_t)origin);
  *x += origin[0];
  *y += origin[1];
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    if (iupStrEqualNoCase(value, "TOP"))
      iupwasmJsSetZorder(id, 1);
    else if (iupStrEqualNoCase(value, "BOTTOM"))
      iupwasmJsSetZorder(id, 0);
  }
  return 0;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    iupwasmSetVisibleState(id, visible);
    iupwasmJsSetVisible(id, visible);
  }
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  Ihandle* parent;
  if (!id || !s_visible[id])
    return 0;
  /* a child is only visible if every native ancestor is visible too */
  for (parent = ih->parent; parent; parent = parent->parent)
  {
    if (parent->iclass->nativetype != IUP_TYPEVOID)
    {
      int pid = iupwasmIdOf(parent);
      if (pid && !s_visible[pid])
        return 0;
    }
  }
  return 1;
}

IUP_SDK_API int iupdrvIsActive(Ihandle *ih)
{
  (void)ih;
  return 1;
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetActive(id, enable);
}

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  const char* var;
  int id = iupwasmIdOf(ih);
  if (!iupStrToRGB(value, &r, &g, &b))
  {
    if (id)
      iupwasmJsSetBgVar(id, "");  /* reset clears the inline color so it reverts to default */
    return 0;
  }
  if (id)
  {
    var = iupwasmThemeColorVar(r, g, b, 1);
    if (var)
      iupwasmJsSetBgVar(id, var);
    else
      iupwasmJsSetBgColor(id, r, g, b);
  }
  return 1;
}

IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  const char* var;
  int id = iupwasmIdOf(ih);
  if (!iupStrToRGB(value, &r, &g, &b))
  {
    if (id)
      iupwasmJsSetFgVar(id, "");
    return 0;
  }
  if (id)
  {
    var = iupwasmThemeColorVar(r, g, b, 0);
    if (var)
      iupwasmJsSetFgVar(id, var);
    else
      iupwasmJsSetFgColor(id, r, g, b);
  }
  return 1;
}

/* IUP cursor name -> CSS cursor; image cursors fall back to default */
static const char* wasmCursorCss(const char* name)
{
  if (!name || iupStrEqualNoCase(name, "ARROW") || iupStrEqualNoCase(name, "UPARROW"))
    return "default";
  if (iupStrEqualNoCase(name, "NONE") || iupStrEqualNoCase(name, "NULL")) return "none";
  if (iupStrEqualNoCase(name, "BUSY") || iupStrEqualNoCase(name, "WAIT")) return "wait";
  if (iupStrEqualNoCase(name, "APPSTARTING")) return "progress";
  if (iupStrEqualNoCase(name, "CROSS") || iupStrEqualNoCase(name, "PEN")) return "crosshair";
  if (iupStrEqualNoCase(name, "HAND")) return "pointer";
  if (iupStrEqualNoCase(name, "HELP")) return "help";
  if (iupStrEqualNoCase(name, "MOVE")) return "move";
  if (iupStrEqualNoCase(name, "TEXT") || iupStrEqualNoCase(name, "IBEAM")) return "text";
  if (iupStrEqualNoCase(name, "NO")) return "not-allowed";
  if (iupStrEqualNoCase(name, "RESIZE_N") || iupStrEqualNoCase(name, "RESIZE_S")) return "ns-resize";
  if (iupStrEqualNoCase(name, "RESIZE_E") || iupStrEqualNoCase(name, "RESIZE_W")) return "ew-resize";
  if (iupStrEqualNoCase(name, "RESIZE_NE") || iupStrEqualNoCase(name, "RESIZE_SW")) return "nesw-resize";
  if (iupStrEqualNoCase(name, "RESIZE_NW") || iupStrEqualNoCase(name, "RESIZE_SE")) return "nwse-resize";
  if (iupStrEqualNoCase(name, "SPLITTER_VERT")) return "col-resize";
  if (iupStrEqualNoCase(name, "SPLITTER_HORIZ")) return "row-resize";
  return "default";
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetCursor(id, wasmCursorCss(value));
  return 1;
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  return 15;
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  (void)ic;
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  (void)ic;
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  (void)key;
  (void)press;
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  (void)x;
  (void)y;
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  (void)x;
  (void)y;
  (void)bt;
  (void)status;
}

IUP_SDK_API void iupdrvSleep(int time)
{
  (void)time;
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  (void)ih;
  (void)title;
}
