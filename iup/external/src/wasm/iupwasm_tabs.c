/** \file
 * \brief WebAssembly Tabs
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
#include "iup_str.h"
#include "iup_class.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_tabs.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsCreateTabs, (void), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createtabs', id: id });
  return id;
})

EM_JS(void, iupwasmJsTabsConfig, (int tabsId, int showClose, int reorder), {
  globalThis.__iupApply({ op: 'tabsconfig', id: tabsId, showClose: showClose, reorder: reorder });
})

EM_JS(int, iupwasmJsTabsAddPage, (int tabsId, const char* title), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var pid = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'tabsaddpage', id: tabsId, pid: pid, title: UTF8ToString(title) });
  return pid;
})

EM_JS(void, iupwasmJsTabsSetTip, (int tabsId, int pos, const char* tip), {
  globalThis.__iupApply({ op: 'tabssettip', id: tabsId, pos: pos, tip: UTF8ToString(tip) });
})

EM_JS(void, iupwasmJsTabsHideTab, (int tabsId, int pos), {
  globalThis.__iupApply({ op: 'tabshide', id: tabsId, pos: pos });
})

EM_JS(void, iupwasmJsTabsRemoveTab, (int tabsId, int pos), {
  globalThis.__iupApply({ op: 'tabsremove', id: tabsId, pos: pos });
})

EM_JS(void, iupwasmJsTabsReorderDom, (int tabsId, int from, int to), {
  globalThis.__iupApply({ op: 'tabsreorderdom', id: tabsId, from: from, to: to });
})

EMSCRIPTEN_KEEPALIVE void iupwasmTabsSetCurrent(int tabsId, int pos)
{
  EM_ASM({ globalThis.__iupApply({ op: 'tabssetcurrent', id: $0, pos: $1 }); }, tabsId, pos);
}

EM_JS(int, iupwasmJsTabsGetCurrent, (int tabsId), {
  var wrap = globalThis.__iup.els[tabsId];
  return wrap ? wrap.__iupCurrent : -1;
})

EM_JS(void, iupwasmJsTabsSetTitle, (int tabsId, int pos, const char* title), {
  globalThis.__iupApply({ op: 'tabssettitle', id: tabsId, pos: pos, title: UTF8ToString(title) });
})

EM_JS(void, iupwasmJsTabsSetImage, (int tabsId, int pos, int imgId, int w, int h), {
  globalThis.__iupApply({ op: 'tabssetimage', id: tabsId, pos: pos, imgId: imgId, w: w, h: h });
})

EM_JS(void, iupwasmJsTabsSetPadding, (int tabsId, int h, int v), {
  globalThis.__iupApply({ op: 'tabspadding', id: tabsId, h: h, v: v });
})

/* returns the native handle plus the size scaled into the TABIMAGESIZE box */
static void* wasmTabsScaledImage(Ihandle* ih, const char* name, int* out_w, int* out_h)
{
  void* img = name ? iupImageGetImage(name, ih, 0, NULL) : NULL;
  int w = 0, h = 0;
  if (img)
  {
    iupdrvImageGetInfo(img, &w, &h, NULL);
    iupTabsScaleImageSize(ih, w, h, &w, &h);
  }
  *out_w = w;
  *out_h = h;
  return img;
}

EM_JS(void, iupwasmJsTabsSetVisible, (int tabsId, int pos, int on), {
  globalThis.__iupApply({ op: 'tabssetvisible', id: tabsId, pos: pos, on: on });
})

EM_JS(void, iupwasmJsTabsSetType, (int tabsId, int type), {
  globalThis.__iupApply({ op: 'tabssettype', id: tabsId, type: type });
})

EM_JS(void, iupwasmJsTabsSetVertical, (int tabsId, int vertical), {
  globalThis.__iupApply({ op: 'tabssetvertical', id: tabsId, vertical: vertical });
})

EM_JS(void, iupwasmJsTabsSetFgColor, (int tabsId, const char* css), {
  globalThis.__iupApply({ op: 'tabsfgcolor', id: tabsId, css: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsTabsSetBgColor, (int tabsId, const char* css), {
  globalThis.__iupApply({ op: 'tabsbgcolor', id: tabsId, css: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsTabsSetShowClose, (int tabsId, int pos, int on), {
  globalThis.__iupApply({ op: 'tabsshowclose', id: tabsId, pos: pos, on: on });
})

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchTabChange(int tabsId, int newpos, int oldpos)
{
  Ihandle* ih = iupwasmHandleFromId(tabsId);
  Ihandle *newc, *oldc;
  IFnnn cb;
  IFnii cbpos;
  if (!ih)
    return;
  newc = IupGetChild(ih, newpos);
  oldc = IupGetChild(ih, oldpos);
  cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  if (cb)
    cb(ih, newc, oldc);
  cbpos = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
  if (cbpos)
    cbpos(ih, newpos, oldpos);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTabsClose(int tabsId, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(tabsId);
  IFni cb;
  int ret = IUP_DEFAULT;
  if (!ih)
    return;
  cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  if (cb)
    ret = cb(ih, pos);
  if (ret == IUP_CONTINUE)
  {
    Ihandle* child = IupGetChild(ih, pos);
    iupwasmJsTabsRemoveTab(tabsId, pos);
    if (child)
    {
      IupDestroy(child);
      IupRefreshChildren(ih);
    }
  }
  else if (ret == IUP_DEFAULT)
    iupwasmJsTabsHideTab(tabsId, pos);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTabsRightClick(int tabsId, int pos)
{
  Ihandle* ih = iupwasmHandleFromId(tabsId);
  IFni cb;
  if (!ih)
    return;
  cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
  if (cb)
    cb(ih, pos);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTabsReorder(int tabsId, int oldPos, int newPos)
{
  Ihandle* ih = iupwasmHandleFromId(tabsId);
  Ihandle *child, *ref_child;
  IFnii cb;
  if (!ih)
    return;
  cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  if (cb && cb(ih, oldPos, newPos) == IUP_IGNORE)
    return;
  child = IupGetChild(ih, oldPos);
  if (!child)
    return;
  ref_child = (oldPos < newPos) ? IupGetChild(ih, newPos + 1) : IupGetChild(ih, newPos);
  iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
  IupReparent(child, ih, ref_child);
  iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);
  iupwasmJsTabsReorderDom(tabsId, oldPos, newPos);
}

static int wasmTabsType(Ihandle* ih)
{
  char* t = iupAttribGet(ih, "TABTYPE");
  if (iupStrEqualNoCase(t, "BOTTOM")) return 1;
  if (iupStrEqualNoCase(t, "LEFT")) return 2;
  if (iupStrEqualNoCase(t, "RIGHT")) return 3;
  return 0;
}

static void wasmTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  int tabsId, pageId, pos;
  char* title;
  char* image;
  char* tip;
  if (!ih->handle || iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  tabsId = iupwasmIdOf(ih);
  pos = IupGetChildPos(ih, child);
  title = iupAttribGetId(ih, "TABTITLE", pos);
  if (!title) title = iupAttribGet(child, "TABTITLE");
  if (!title) title = "";

  pageId = iupwasmJsTabsAddPage(tabsId, title);
  iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)(intptr_t)pageId);

  image = iupAttribGetId(ih, "TABIMAGE", pos);
  if (!image) image = iupAttribGet(child, "TABIMAGE");
  if (image)
  {
    int iw, ih_h;
    void* img = wasmTabsScaledImage(ih, image, &iw, &ih_h);
    if (img)
      iupwasmJsTabsSetImage(tabsId, pos, (int)(intptr_t)img, iw, ih_h);
  }

  tip = iupAttribGetId(ih, "TABTIP", pos);
  if (!tip) tip = iupAttribGet(child, "TABTIP");
  if (tip)
    iupwasmJsTabsSetTip(tabsId, pos, tip);
}

static int wasmTabsMapMethod(Ihandle* ih)
{
  Ihandle* child;
  int id = iupwasmJsCreateTabs();
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmAddToParent(ih);

  ih->data->type = wasmTabsType(ih);
  ih->data->orientation = iupStrEqualNoCase(iupAttribGet(ih, "TABORIENTATION"), "VERTICAL") ? ITABS_VERTICAL : ITABS_HORIZONTAL;
  iupwasmJsTabsSetType(id, ih->data->type);
  iupwasmJsTabsConfig(id, ih->data->show_close, iupAttribGetBoolean(ih, "ALLOWREORDER"));

  for (child = ih->firstchild; child; child = child->brother)
    wasmTabsChildAddedMethod(ih, child);

  if (ih->data->orientation == ITABS_VERTICAL)
    iupwasmJsTabsSetVertical(id, 1);

  return IUP_NOERROR;
}

static int wasmTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTabsSetTitle(id, pos, value ? value : "");
  return 1;
}

static int wasmTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  int iw, ih_h;
  void* img = wasmTabsScaledImage(ih, value, &iw, &ih_h);
  if (id && img)
    iupwasmJsTabsSetImage(id, pos, (int)(intptr_t)img, iw, ih_h);
  return 1;
}

static int wasmTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTabsSetVisible(id, pos, iupStrBoolean(value));
  return 1;
}

static int wasmTabsSetTypeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (iupStrEqualNoCase(value, "BOTTOM")) ih->data->type = ITABS_BOTTOM;
  else if (iupStrEqualNoCase(value, "LEFT")) ih->data->type = ITABS_LEFT;
  else if (iupStrEqualNoCase(value, "RIGHT")) ih->data->type = ITABS_RIGHT;
  else ih->data->type = ITABS_TOP;
  if (id)
    iupwasmJsTabsSetType(id, ih->data->type);
  return 1;
}

static int wasmTabsSetOrientationAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->orientation = iupStrEqualNoCase(value, "VERTICAL") ? ITABS_VERTICAL : ITABS_HORIZONTAL;
  if (id)
    iupwasmJsTabsSetVertical(id, ih->data->orientation == ITABS_VERTICAL);
  return 1;
}

static int wasmTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (id)
  {
    iupwasmJsTabsSetPadding(id, ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  return 1;
}

static int wasmTabsSetTabTipAttrib(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTabsSetTip(id, pos, value ? value : "");
  return 1;
}

static char* wasmTabsGetShowCloseId(Ihandle* ih, int pos)
{
  (void)pos;
  return iupStrReturnBoolean(ih->data->show_close);
}

static void wasmTabsColorCss(const char* value, int is_bg, char* css, int csslen)
{
  unsigned char r, g, b;
  const char* var;
  if (!iupStrToRGB(value, &r, &g, &b))
  {
    css[0] = '\0';
    return;
  }
  var = iupwasmThemeColorVar(r, g, b, is_bg);
  if (var)
    iupStrCopyN(css, csslen, var);
  else
    snprintf(css, csslen, "rgb(%d,%d,%d)", r, g, b);
}

static int wasmTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char css[64];
  wasmTabsColorCss(value, 0, css, sizeof(css));
  if (id && css[0])
    iupwasmJsTabsSetFgColor(id, css);
  return 1;
}

static int wasmTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char css[64];
  wasmTabsColorCss(value, 1, css, sizeof(css));
  if (id && css[0])
    iupwasmJsTabsSetBgColor(id, css);
  return 1;
}

static int wasmTabsSetShowCloseId(Ihandle* ih, int pos, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (pos == IUP_INVALID_ID)
  {
    ih->data->show_close = iupStrBoolean(value);
    if (id)
      iupwasmJsTabsConfig(id, ih->data->show_close, iupAttribGetBoolean(ih, "ALLOWREORDER"));
  }
  else if (id)
    iupwasmJsTabsSetShowClose(id, pos, iupStrBoolean(value));
  return 1;
}

static int wasmTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTabsConfig(id, ih->data->show_close, iupStrBoolean(value));
  return 1;
}

IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmTabsSetCurrent(id, pos);
}

IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return id ? iupwasmJsTabsGetCurrent(id) : -1;
}

IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  (void)pos;
  return iupAttribGetBoolean(child, "TABVISIBLE") || !iupAttribGet(child, "TABVISIBLE");
}

IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

IUP_SDK_API int iupdrvTabsExtraMargin(void)
{
  return 0;
}

IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int img_w = 0, img_h = 0;
  if (tab_image)
    wasmTabsScaledImage(ih, tab_image, &img_w, &img_h);

  if (tab_width)
  {
    int w = (tab_title && tab_title[0]) ? iupdrvFontGetStringWidth(ih, tab_title) : (img_w ? 0 : 32);
    w += 22;
    if (img_w)
    {
      w += img_w;
      if (tab_title && tab_title[0])
        w += 4;
    }
    if (iupAttribGetBoolean(ih, "SHOWCLOSE"))
      w += 20;
    *tab_width = w;
  }
  if (tab_height)
  {
    int h = 18;
    if (img_h + 4 > h)
      h = img_h + 4;
    *tab_height = h;
  }
}

IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = wasmTabsMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;
  ic->ChildAdded = wasmTabsChildAddedMethod;

  iupClassRegisterAttributeId(ic, "TABTITLE", NULL, wasmTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, wasmTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", NULL, wasmTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABTYPE", NULL, wasmTabsSetTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", NULL, wasmTabsSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, wasmTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, wasmTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, wasmTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTIP", NULL, wasmTabsSetTabTipAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", wasmTabsGetShowCloseId, wasmTabsSetShowCloseId, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, wasmTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");
  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);  /* single tab row only, as in GTK */
}
