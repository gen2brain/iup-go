/** \file
 * \brief WebAssembly Menu
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_menu.h"

#include "iupwasm_drv.h"


#define IUPWASM_MENUBAR_HEIGHT 24

EM_JS(void, iupwasmJsMenuInit, (void), { globalThis.__iupApply({ op: 'menuinit' }); })

EM_JS(int, iupwasmJsCreateMenuBar, (void), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createmenubar', id: id });
  return id;
})

EM_JS(int, iupwasmJsCreateDropdown, (void), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createdropdown', id: id });
  return id;
})

/* kind: 0 = item, 1 = submenu label, 2 = separator. */
EM_JS(int, iupwasmJsCreateMenuEntry, (int kind), {
  globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'createmenuentry', id: id, kind: kind });
  return id;
})

EM_JS(void, iupwasmJsMenuSetText, (int id, const char* txt, const char* accel, const char* mnem), {
  globalThis.__iupApply({ op: 'menutext', id: id, text: UTF8ToString(txt), accel: accel ? UTF8ToString(accel) : "", mnem: mnem ? UTF8ToString(mnem) : "" });
})

EM_JS(void, iupwasmJsMenuSetColor, (int id, const char* css, int isBg), {
  globalThis.__iupApply({ op: 'menucolor', id: id, css: UTF8ToString(css), isBg: isBg });
})

EM_JS(void, iupwasmJsMenuSetCheck, (int id, int on), {
  globalThis.__iupApply({ op: 'menucheck', id: id, on: on });
})

/* imgId 0 hides the item icon */
EM_JS(void, iupwasmJsMenuSetImage, (int id, int imgId), {
  globalThis.__iupApply({ op: 'menuimage', id: id, imgId: imgId });
})

EM_JS(void, iupwasmJsMenuHideMark, (int id, int hide), {
  globalThis.__iupApply({ op: 'menuhidemark', id: id, hide: hide });
})

EM_JS(void, iupwasmJsMenuInsert, (int parentId, int childId, int pos), {
  globalThis.__iupApply({ op: 'menuinsert', pid: parentId, cid: childId, pos: pos });
})

EM_JS(void, iupwasmJsMenuItemWire, (int id), { globalThis.__iupApply({ op: 'menuitemwire', id: id }); })

EM_JS(void, iupwasmJsSubmenuAttach, (int labelId, int dropId, int topLevel), {
  globalThis.__iupApply({ op: 'submenuattach', labelId: labelId, dropId: dropId, topLevel: topLevel });
})

EM_JS(void, iupwasmJsMenuPopup, (int dropId, int x, int y, const char* align), {
  globalThis.__iupApply({ op: 'menupopup', dropId: dropId, x: x, y: y, align: align ? UTF8ToString(align) : "" });
})

EM_JS(void, iupwasmJsRecentWire, (int entryId, int menuId, int index), {
  globalThis.__iupApply({ op: 'recentwire', id: entryId, menuId: menuId, index: index });
})

EMSCRIPTEN_KEEPALIVE void iupwasmMenuState(int id, int opening)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, opening ? "MENUOPEN_CB" : "MENUCLOSE_CB");
  if (cb)
    cb(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmMenuHighlight(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, "HIGHLIGHT_CB");
  if (cb)
    cb(ih);
}

EMSCRIPTEN_KEEPALIVE void iupwasmMenuHelp(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  Icallback cb;
  if (!ih)
    return;
  cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  (void)ih;
  return IUPWASM_MENUBAR_HEIGHT;
}

IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsMenuPopup(id, x, y, iupAttribGet(ih, "POPUPALIGN"));
  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  int menuId, max_recent, existing, i;
  char attr_name[32];

  if (!menu || !menu->handle)
    return -1;

  menuId = iupwasmIdOf(menu);
  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  for (i = 0; i < count; i++)
  {
    int entryId;

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_ITEMID%d", i);
    entryId = iupAttribGetInt(menu, attr_name);
    if (entryId)
      iupwasmJsMenuSetText(entryId, filenames[i], "", "");
    else
    {
      entryId = iupwasmJsCreateMenuEntry(0);
      iupwasmJsMenuSetText(entryId, filenames[i], "", "");
      iupwasmJsMenuInsert(menuId, entryId, i);
      iupwasmJsRecentWire(entryId, menuId, i);
      iupAttribSetInt(menu, attr_name, entryId);
    }
  }

  for (i = count; i < existing; i++)
  {
    int entryId;

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_ITEMID%d", i);
    entryId = iupAttribGetInt(menu, attr_name);
    if (entryId)
      iupwasmJsDestroy(entryId);
    iupAttribSet(menu, attr_name, NULL);

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, NULL);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}

EMSCRIPTEN_KEEPALIVE void iupwasmRecentActivate(int menuId, int index)
{
  Ihandle* menu = iupwasmHandleFromId(menuId);
  Icallback recent_cb;
  Ihandle* config;
  char attr_name[32];
  char* filename;

  if (!menu)
    return;

  recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");
  if (!recent_cb || !config)
    return;

  snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", index);
  filename = iupAttribGet(menu, attr_name);
  if (!filename)
    return;

  IupSetStrAttribute(config, "RECENTFILENAME", filename);
  IupSetStrAttribute(config, "TITLE", filename);
  config->parent = menu;
  recent_cb(config);
  config->parent = NULL;
  IupSetAttribute(config, "RECENTFILENAME", NULL);
  IupSetAttribute(config, "TITLE", NULL);
}

static int wasmMenuMapMethod(Ihandle* ih)
{
  int id;

  iupwasmJsMenuInit();

  if (iupMenuIsMenuBar(ih))
  {
    id = iupwasmJsCreateMenuBar();
    if (!id)
      return IUP_ERROR;
    ih->handle = (InativeHandle*)(intptr_t)id;
    iupwasmRegisterHandle(id, ih);
    iupwasmAddToParent(ih);
  }
  else
  {
    id = iupwasmJsCreateDropdown();
    if (!id)
      return IUP_ERROR;
    ih->handle = (InativeHandle*)(intptr_t)id;
    iupwasmRegisterHandle(id, ih);

    /* a non-bar menu is its parent submenu's dropdown; top-level if that submenu sits in the menubar */
    if (ih->parent)
    {
      int label = iupwasmIdOf(ih->parent);
      int top_level = ih->parent->parent && iupMenuIsMenuBar(ih->parent->parent);
      if (label)
        iupwasmJsSubmenuAttach(label, id, top_level);
    }
  }

  ih->serial = iupMenuGetChildId(ih);
  return IUP_NOERROR;
}

static int wasmSubmenuMapMethod(Ihandle* ih)
{
  int id, pos;

  if (!ih->parent)
    return IUP_ERROR;

  id = iupwasmJsCreateMenuEntry(1);
  if (!id)
    return IUP_ERROR;
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  ih->serial = iupMenuGetChildId(ih);

  pos = IupGetChildPos(ih->parent, ih);
  iupwasmJsMenuInsert(iupwasmIdOf(ih->parent), id, pos);
  return IUP_NOERROR;
}

static int wasmMenuItemMapMethod(Ihandle* ih)
{
  int id, pos;

  if (!ih->parent)
    return IUP_ERROR;

  id = iupwasmJsCreateMenuEntry(0);
  if (!id)
    return IUP_ERROR;
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  ih->serial = iupMenuGetChildId(ih);

  iupwasmJsMenuItemWire(id);

  pos = IupGetChildPos(ih->parent, ih);
  iupwasmJsMenuInsert(iupwasmIdOf(ih->parent), id, pos);
  return IUP_NOERROR;
}

static int wasmMenuSeparatorMapMethod(Ihandle* ih)
{
  int id, pos;

  if (!ih->parent)
    return IUP_ERROR;

  id = iupwasmJsCreateMenuEntry(2);
  if (!id)
    return IUP_ERROR;
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  pos = IupGetChildPos(ih->parent, ih);
  iupwasmJsMenuInsert(iupwasmIdOf(ih->parent), id, pos);
  return IUP_NOERROR;
}

static int wasmMenuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    char mnem = 0;
    char mnemstr[2];
    char* clean;
    char* tab;
    const char* accel = "";
    if (!value)
      value = "";
    clean = iupStrProcessMnemonic(value, &mnem, -1);
    if (clean == value)
      clean = iupStrDup(value);
    tab = strchr(clean, '\t');
    if (tab)
    {
      *tab = '\0';
      accel = tab + 1;
    }
    mnemstr[0] = mnem;
    mnemstr[1] = '\0';
    iupwasmJsMenuSetText(id, clean, accel, mnemstr);
    free(clean);
  }
  return 1;
}

static int wasmMenuItemSetColor(Ihandle* ih, const char* value, int is_bg)
{
  int id = iupwasmIdOf(ih);
  unsigned char r, g, b;
  const char* var;
  char css[64];
  if (!id || !iupStrToRGB(value, &r, &g, &b))
    return 1;
  var = iupwasmThemeColorVar(r, g, b, is_bg);
  if (var)
    iupStrCopyN(css, sizeof(css), var);
  else
    snprintf(css, sizeof(css), "rgb(%d,%d,%d)", r, g, b);
  iupwasmJsMenuSetColor(id, css, is_bg);
  return 1;
}

static int wasmMenuItemSetFgColorAttrib(Ihandle* ih, const char* value)
{
  return wasmMenuItemSetColor(ih, value, 0);
}

static int wasmMenuItemSetBgColorAttrib(Ihandle* ih, const char* value)
{
  return wasmMenuItemSetColor(ih, value, 1);
}

static void wasmMenuItemUpdateImage(Ihandle* ih, int value_on)
{
  int id = iupwasmIdOf(ih);
  char* image = iupAttribGet(ih, "IMAGE");
  char* impress = iupAttribGet(ih, "IMPRESS");
  char* titleimage = iupAttribGet(ih, "TITLEIMAGE");
  const char* name = NULL;
  void* img;
  if (impress && value_on)
    name = impress;
  else if (image)
    name = image;
  else if (titleimage)
    name = titleimage;
  if (!id)
    return;
  img = name ? iupImageGetImage(name, ih, 0, NULL) : NULL;
  iupwasmJsMenuSetImage(id, img ? (int)(intptr_t)img : 0);
}

static int wasmMenuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  int on = iupStrBoolean(value);
  if (id)
    iupwasmJsMenuSetCheck(id, on);
  wasmMenuItemUpdateImage(ih, on);
  return 1;
}

static int wasmMenuItemSetImageAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  wasmMenuItemUpdateImage(ih, iupAttribGetBoolean(ih, "VALUE"));
  return 1;
}

static int wasmMenuItemSetHideMarkAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsMenuHideMark(id, iupStrBoolean(value));
  return 1;
}


IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = wasmMenuMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
}

IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = wasmMenuItemMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmMenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", NULL, wasmMenuItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, wasmMenuItemSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, wasmMenuItemSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, wasmMenuItemSetHideMarkAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, wasmMenuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, wasmMenuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, wasmMenuItemSetImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
}

IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = wasmMenuSeparatorMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
}

IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = wasmSubmenuMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmMenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, wasmMenuItemSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, wasmMenuItemSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, wasmMenuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, wasmMenuItemSetImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
}
