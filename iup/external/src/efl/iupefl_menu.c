/** \file
 * \brief Menu Resources
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_menu.h"

#include "iupefl_drv.h"


static void eflItemUpdateMark(Ihandle* ih);

/****************************************************************
                     Mnemonic Support
****************************************************************/

static char eflMenuExtractMnemonic(const char* title)
{
  char mnemonic = 0;
  if (title)
    iupStrProcessMnemonic(title, &mnemonic, -1);
  return mnemonic;
}

static void eflMenuStoreMnemonic(Ihandle* menu_bar, char mnemonic, Elm_Object_Item* item)
{
  char attr_name[32];
  if (mnemonic && item)
  {
    mnemonic = (char)tolower((unsigned char)mnemonic);
    snprintf(attr_name, sizeof(attr_name), "_IUP_MNEMONIC_%c", mnemonic);
    iupAttribSet(menu_bar, attr_name, (char*)item);
  }
}

Elm_Object_Item* iupeflMenuFindMnemonic(Ihandle* ih, char key)
{
  Ihandle* dialog;
  Ihandle* menu_bar;
  char attr_name[32];
  Elm_Object_Item* item;

  if (!ih)
    return NULL;

  dialog = IupGetDialog(ih);
  if (!dialog || !dialog->data || !dialog->data->menu)
    return NULL;

  menu_bar = dialog->data->menu;
  if (!menu_bar)
    return NULL;

  key = (char)tolower((unsigned char)key);
  snprintf(attr_name, sizeof(attr_name), "_IUP_MNEMONIC_%c", key);
  item = (Elm_Object_Item*)iupAttribGet(menu_bar, attr_name);

  return item;
}

/****************************************************************
                     Title Processing
****************************************************************/

static char* eflMenuGetDisplayTitle(const char* title)
{
  char* str;
  char* tab_pos;

  if (!title || !title[0])
    return (char*)title;

  str = iupStrProcessMnemonic(title, NULL, 0);
  if (!str)
    str = (char*)title;

  tab_pos = strchr(str, '\t');
  if (tab_pos)
  {
    int len = (int)(tab_pos - str);
    char* label = malloc(len + 1);
    memcpy(label, str, len);
    label[len] = '\0';

    if (str != title)
      free(str);
    return label;
  }

  return str;
}

/****************************************************************
                     Helper Functions
****************************************************************/

static Evas_Object* eflMenuGetRootMenu(Ihandle* ih)
{
  Ihandle* menu = ih;
  while (menu)
  {
    Evas_Object* root = (Evas_Object*)iupAttribGet(menu, "_IUP_EFL_MENU");
    if (root)
      return root;
    menu = menu->parent;
  }
  return NULL;
}

static Elm_Object_Item* eflMenuGetParentItem(Ihandle* ih)
{
  Ihandle* parent;

  if (!ih->parent)
    return NULL;

  parent = ih->parent;

  if (parent->iclass->nativetype == IUP_TYPEMENU)
  {
    Ihandle* submenu = parent->parent;
    if (submenu && submenu->iclass->nativetype == IUP_TYPEMENU)
    {
      return (Elm_Object_Item*)iupAttribGet(submenu, "_IUP_EFL_PARENT_ITEM");
    }
  }

  return NULL;
}

/****************************************************************
                     Hover Layer
****************************************************************/

static void eflMenuSetHoverLayer(Evas_Object* menu);

static void eflMenuShowCallback(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)e;
  (void)event_info;

  eflMenuSetHoverLayer(obj);

  cb = IupGetCallback(ih, "OPEN_CB");
  if (!cb && ih->parent)
    cb = IupGetCallback(ih->parent, "OPEN_CB");
  if (cb)
    cb(ih);
}

static void eflMenuSetHoverLayer(Evas_Object* menu)
{
  Evas* evas;
  Evas_Object* top;
  Evas_Object* obj;

  if (!menu)
    return;

  evas = evas_object_evas_get(menu);
  if (!evas)
    return;

  top = evas_object_top_get(evas);
  obj = evas_object_bottom_get(evas);

  while (obj)
  {
    Evas_Object* next = evas_object_above_get(obj);
    const char* type = evas_object_type_get(obj);

    if (type && strstr(type, "hover"))
      evas_object_layer_set(obj, EVAS_LAYER_MAX);

    if (obj == top)
      break;
    obj = next;
  }
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflMenuItemActivateCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)obj;
  (void)event_info;

  if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    if (iupAttribGetBoolean(ih, "VALUE"))
      iupAttribSet(ih, "VALUE", "OFF");
    else
      iupAttribSet(ih, "VALUE", "ON");

    eflItemUpdateMark(ih);
  }

  cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int result = cb(ih);
    if (result == IUP_CLOSE)
      IupExitLoop();
  }
}

static void eflMenuDismissedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)obj;
  (void)event_info;

  cb = IupGetCallback(ih, "MENUCLOSE_CB");
  if (!cb && ih->parent)
    cb = IupGetCallback(ih->parent, "MENUCLOSE_CB");
  if (cb)
    cb(ih);

  if (!ih->parent)
    iupeflModalLoopQuit();
}

/****************************************************************
                     Item Attributes
****************************************************************/

static void eflItemUpdateMark(Ihandle* ih)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");
  Evas_Object* icon;
  Evas_Object* layout;
  char* hidemark;
  int show_mark;
  int is_checked;

  if (!item)
    return;

  hidemark = iupAttribGet(ih, "HIDEMARK");
  show_mark = (hidemark == NULL || !iupStrEqualNoCase(hidemark, "YES"));
  is_checked = iupAttribGetBoolean(ih, "VALUE");

  icon = elm_object_item_part_content_get(item, NULL);
  layout = elm_menu_item_object_get(item);

  if (!icon || !layout)
    return;

  if (show_mark && is_checked)
  {
    if (!elm_icon_standard_set(icon, "object-select-symbolic"))
      if (!elm_icon_standard_set(icon, "emblem-default"))
        elm_icon_standard_set(icon, "emblem-ok");
    evas_object_show(icon);
    elm_layout_signal_emit(layout, "elm,state,icon,visible", "elm");
  }
  else
  {
    evas_object_hide(icon);
    elm_layout_signal_emit(layout, "elm,state,icon,hidden", "elm");
  }
  edje_object_message_signal_process(elm_layout_edje_get(layout));
}

static int eflItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");
  char* display_title;

  if (!item)
    return 1;

  if (!value)
    value = "";

  display_title = eflMenuGetDisplayTitle(value);
  elm_object_item_text_set(item, iupeflStrConvertToSystem(display_title));

  if (display_title != value)
    free(display_title);

  return 1;
}

static int eflItemSetValueAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");
  Evas_Object* icon;
  Evas_Object* layout;
  char* hidemark;
  int show_mark;
  int is_checked;

  if (!item)
    return 1;

  hidemark = iupAttribGet(ih, "HIDEMARK");
  show_mark = (hidemark == NULL || !iupStrEqualNoCase(hidemark, "YES"));
  is_checked = iupStrBoolean(value);

  icon = elm_object_item_part_content_get(item, NULL);
  layout = elm_menu_item_object_get(item);

  if (!icon || !layout)
    return 1;

  if (show_mark && is_checked)
  {
    if (!elm_icon_standard_set(icon, "object-select-symbolic"))
      if (!elm_icon_standard_set(icon, "emblem-default"))
        elm_icon_standard_set(icon, "emblem-ok");
    evas_object_show(icon);
    elm_layout_signal_emit(layout, "elm,state,icon,visible", "elm");
  }
  else
  {
    evas_object_hide(icon);
    elm_layout_signal_emit(layout, "elm,state,icon,hidden", "elm");
  }
  edje_object_message_signal_process(elm_layout_edje_get(layout));

  return 1;
}

static char* eflItemGetValueAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "VALUE");
}

static int eflItemSetImageAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item && value)
  {
    Evas_Object* menu = eflMenuGetRootMenu(ih);
    if (menu)
    {
      Evas_Object* image = iupeflImageGetImageForParent(value, ih, 0, menu);
      if (image)
        elm_object_item_part_content_set(item, "icon", image);
    }
  }

  return 1;
}

static int eflItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item)
    elm_object_item_disabled_set(item, !iupStrBoolean(value));

  return 0;
}

/****************************************************************
                     Item Methods
****************************************************************/

static int eflItemMapMethod(Ihandle* ih)
{
  Evas_Object* menu;
  Elm_Object_Item* parent_item;
  Elm_Object_Item* item;
  char* title;
  char* display_title;

  if (!ih->parent)
    return IUP_ERROR;

  menu = eflMenuGetRootMenu(ih);
  parent_item = eflMenuGetParentItem(ih);

  title = iupAttribGet(ih, "TITLE");
  if (!title)
    title = "";

  if (!menu)
    return IUP_ERROR;

  display_title = eflMenuGetDisplayTitle(title);

  item = elm_menu_item_add(menu, parent_item, NULL, iupeflStrConvertToSystem(display_title), eflMenuItemActivateCallback, ih);

  if (display_title != title)
    free(display_title);

  if (!item)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)menu;
  ih->serial = iupMenuGetChildId(ih);
  iupAttribSet(ih, "_IUP_EFL_ITEM", (char*)item);

  if (iupAttribGetBoolean(ih, "VALUE"))
    eflItemUpdateMark(ih);

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    elm_object_item_disabled_set(item, EINA_TRUE);

  return IUP_NOERROR;
}

static void eflItemUnMapMethod(Ihandle* ih)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item)
    elm_object_item_del(item);

  iupAttribSet(ih, "_IUP_EFL_ITEM", NULL);
  ih->handle = NULL;
}

void iupdrvItemInitClass(Iclass* ic)
{
  ic->Map = eflItemMapMethod;
  ic->UnMap = eflItemUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, eflItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", eflItemGetValueAttrib, eflItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, eflItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, eflItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}

/****************************************************************
                     Separator Methods
****************************************************************/

static int eflSeparatorMapMethod(Ihandle* ih)
{
  Evas_Object* menu;
  Elm_Object_Item* parent_item;
  Elm_Object_Item* item;

  if (!ih->parent)
    return IUP_ERROR;

  menu = eflMenuGetRootMenu(ih);
  if (!menu)
    return IUP_ERROR;

  parent_item = eflMenuGetParentItem(ih);

  item = elm_menu_item_separator_add(menu, parent_item);
  if (!item)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)menu;
  ih->serial = iupMenuGetChildId(ih);
  iupAttribSet(ih, "_IUP_EFL_ITEM", (char*)item);

  return IUP_NOERROR;
}

static void eflSeparatorUnMapMethod(Ihandle* ih)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item)
    elm_object_item_del(item);

  iupAttribSet(ih, "_IUP_EFL_ITEM", NULL);
  ih->handle = NULL;
}

void iupdrvSeparatorInitClass(Iclass* ic)
{
  ic->Map = eflSeparatorMapMethod;
  ic->UnMap = eflSeparatorUnMapMethod;
}

/****************************************************************
                     Submenu Attributes
****************************************************************/

static int eflSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");
  char* display_title;

  if (!item)
    return 1;

  if (!value)
    value = "";

  display_title = eflMenuGetDisplayTitle(value);
  elm_object_item_text_set(item, iupeflStrConvertToSystem(display_title));

  if (display_title != value)
    free(display_title);

  return 1;
}

static int eflSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item && value)
  {
    Evas_Object* menu = eflMenuGetRootMenu(ih);
    if (menu)
    {
      Evas_Object* image = iupeflImageGetImageForParent(value, ih, 0, menu);
      if (image)
        elm_object_item_part_content_set(item, "icon", image);
    }
  }

  return 1;
}

static int eflSubmenuSetActiveAttrib(Ihandle* ih, const char* value)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item)
    elm_object_item_disabled_set(item, !iupStrBoolean(value));

  return 0;
}

/****************************************************************
                     Submenu Methods
****************************************************************/

static int eflSubmenuMapMethod(Ihandle* ih)
{
  Evas_Object* menu;
  Elm_Object_Item* parent_item;
  Elm_Object_Item* item;
  char* title;
  char* display_title;
  char mnemonic;

  if (!ih->parent)
    return IUP_ERROR;

  menu = eflMenuGetRootMenu(ih);
  parent_item = eflMenuGetParentItem(ih);

  title = iupAttribGet(ih, "TITLE");
  if (!title)
    title = "";

  if (!menu)
    return IUP_ERROR;

  display_title = eflMenuGetDisplayTitle(title);
  mnemonic = eflMenuExtractMnemonic(title);

  item = elm_menu_item_add(menu, parent_item, NULL, iupeflStrConvertToSystem(display_title), NULL, NULL);

  if (display_title != title)
    free(display_title);

  if (!item)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)menu;
  ih->serial = iupMenuGetChildId(ih);

  iupAttribSet(ih, "_IUP_EFL_ITEM", (char*)item);
  iupAttribSet(ih, "_IUP_EFL_PARENT_ITEM", (char*)item);

  if (mnemonic && iupMenuIsMenuBar(ih->parent))
    eflMenuStoreMnemonic(ih->parent, mnemonic, item);

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    elm_object_item_disabled_set(item, EINA_TRUE);

  return IUP_NOERROR;
}

static void eflSubmenuUnMapMethod(Ihandle* ih)
{
  Elm_Object_Item* item = (Elm_Object_Item*)iupAttribGet(ih, "_IUP_EFL_ITEM");

  if (item)
    elm_object_item_del(item);

  iupAttribSet(ih, "_IUP_EFL_ITEM", NULL);
  iupAttribSet(ih, "_IUP_EFL_PARENT_ITEM", NULL);
  ih->handle = NULL;
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = eflSubmenuMapMethod;
  ic->UnMap = eflSubmenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, eflSubmenuSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, eflSubmenuSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, eflSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}

/****************************************************************
                     Recent Menu Support
****************************************************************/

static void eflRecentItemActivate(void* data, Evas_Object* obj, void* event_info)
{
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  Ihandle* menu = (Ihandle*)data;
  int index;
  Icallback recent_cb;
  Ihandle* config;

  (void)obj;

  if (!item || !menu)
    return;

  index = (int)(intptr_t)elm_object_item_data_get(item);
  recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");

  if (recent_cb && config)
  {
    char attr_name[32];
    const char* filename;

    sprintf(attr_name, "_IUP_RECENT_FILE%d", index);
    filename = iupAttribGet(menu, attr_name);

    if (filename)
    {
      IupSetStrAttribute(config, "RECENTFILENAME", filename);
      IupSetStrAttribute(config, "TITLE", filename);
      config->parent = menu;

      recent_cb(config);

      config->parent = NULL;
      IupSetAttribute(config, "RECENTFILENAME", NULL);
      IupSetAttribute(config, "TITLE", NULL);
    }
  }
}

int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  Evas_Object* elm_menu;
  Elm_Object_Item* parent_item;
  Elm_Object_Item* empty_item;
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  elm_menu = (Evas_Object*)iupAttribGet(menu, "_IUP_EFL_MENU");
  if (!elm_menu)
    return -1;

  parent_item = NULL;
  if (menu->parent)
    parent_item = (Elm_Object_Item*)iupAttribGet(menu->parent, "_IUP_EFL_PARENT_ITEM");

  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  empty_item = (Elm_Object_Item*)iupAttribGet(menu, "_IUP_RECENT_EMPTY");
  if (count > 0 && empty_item)
  {
    elm_object_item_del(empty_item);
    iupAttribSet(menu, "_IUP_RECENT_EMPTY", NULL);
  }

  for (i = 0; i < count; i++)
  {
    Elm_Object_Item* item;
    char attr_name[32];

    sprintf(attr_name, "_IUP_RECENT_ITEM%d", i);
    item = (Elm_Object_Item*)iupAttribGet(menu, attr_name);

    if (item)
    {
      elm_object_item_text_set(item, filenames[i]);
    }
    else
    {
      item = elm_menu_item_add(elm_menu, parent_item, NULL, filenames[i], eflRecentItemActivate, menu);
      if (item)
      {
        elm_object_item_data_set(item, (void*)(intptr_t)i);
        iupAttribSet(menu, attr_name, (char*)item);
      }
    }

    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);
  }

  for (; i < existing; i++)
  {
    Elm_Object_Item* item;
    char attr_name[32];

    sprintf(attr_name, "_IUP_RECENT_ITEM%d", i);
    item = (Elm_Object_Item*)iupAttribGet(menu, attr_name);
    if (item)
    {
      elm_object_item_del(item);
      iupAttribSet(menu, attr_name, NULL);
    }

    sprintf(attr_name, "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, NULL);
  }

  if (count == 0 && !iupAttribGet(menu, "_IUP_RECENT_EMPTY"))
  {
    empty_item = elm_menu_item_add(elm_menu, parent_item, NULL, "", NULL, NULL);
    if (empty_item)
    {
      elm_object_item_disabled_set(empty_item, EINA_TRUE);
      iupAttribSet(menu, "_IUP_RECENT_EMPTY", (char*)empty_item);
    }
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}

/****************************************************************
                     Menu Driver Functions
****************************************************************/

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  Evas_Object* menu = (Evas_Object*)ih->handle;
  Evas_Object* win;
  int wx = 0, wy = 0;

  if (!menu)
    return IUP_ERROR;

  win = iupeflGetMainWindow();
  if (win)
  {
    Evas* evas = evas_object_evas_get(win);
    if (evas)
    {
      Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
      if (ee)
        ecore_evas_geometry_get(ee, &wx, &wy, NULL, NULL);
    }
  }

  elm_menu_move(menu, x - wx, y - wy);
  evas_object_show(menu);
  eflMenuSetHoverLayer(menu);

  iupeflModalLoopRun();

  return IUP_NOERROR;
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  Evas_Object* menu;
  int height;

  if (!ih || !ih->handle)
    return 0;

  menu = (Evas_Object*)ih->handle;

  {
    Eina_Rect geometry = efl_gfx_entity_geometry_get(menu);
    height = geometry.h;
  }

  if (height == 0)
  {
    Eina_Size2D min_size = efl_gfx_hint_size_combined_min_get(menu);
    height = min_size.h;
  }

  if (height == 0)
  {
    int ch;
    iupdrvFontGetCharSize(ih, NULL, &ch);
    height = 4 + ch + 4;
  }

  return height;
}

static int eflMenuSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    Ihandle* pih = iupChildTreeGetNativeParent(ih);
    Evas_Object* win;
    Evas_Object* menu;

    if (!pih || !pih->handle)
      return IUP_ERROR;

    win = iupeflGetWidget(pih);
    menu = elm_win_main_menu_get(win);
    if (!menu)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)menu;
    iupAttribSet(ih, "_IUP_EFL_MENU", (char*)menu);

    evas_object_smart_callback_add(menu, "dismissed", eflMenuDismissedCallback, ih);
    evas_object_event_callback_add(menu, EVAS_CALLBACK_SHOW, eflMenuShowCallback, ih);
  }
  else if (ih->parent && ih->parent->iclass->nativetype == IUP_TYPEMENU)
  {
    Evas_Object* root = eflMenuGetRootMenu(ih->parent);
    if (!root)
      return IUP_ERROR;

    iupAttribSet(ih, "_IUP_EFL_MENU", (char*)root);
    iupAttribSet(ih, "_IUP_EFL_CONTENT_MENU", "1");
    ih->handle = (InativeHandle*)root;
  }
  else
  {
    Evas_Object* win = iupeflGetMainWindow();
    Evas_Object* menu;

    if (!win)
      return IUP_ERROR;

    menu = elm_menu_add(win);
    if (!menu)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)menu;
    iupAttribSet(ih, "_IUP_EFL_MENU", (char*)menu);

    evas_object_smart_callback_add(menu, "dismissed", eflMenuDismissedCallback, ih);
    evas_object_event_callback_add(menu, EVAS_CALLBACK_SHOW, eflMenuShowCallback, ih);
  }

  ih->serial = iupMenuGetChildId(ih);

  return IUP_NOERROR;
}

static void eflMenuUnMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    Evas_Object* menu = (Evas_Object*)ih->handle;
    if (menu)
    {
      evas_object_smart_callback_del(menu, "dismissed", eflMenuDismissedCallback);
      evas_object_event_callback_del(menu, EVAS_CALLBACK_SHOW, eflMenuShowCallback);
    }
    ih->parent = NULL;
  }
  else if (iupAttribGet(ih, "_IUP_EFL_CONTENT_MENU"))
  {
    iupAttribSet(ih, "_IUP_EFL_CONTENT_MENU", NULL);
  }
  else if (ih->handle)
  {
    Evas_Object* menu = (Evas_Object*)ih->handle;
    evas_object_smart_callback_del(menu, "dismissed", eflMenuDismissedCallback);
    evas_object_event_callback_del(menu, EVAS_CALLBACK_SHOW, eflMenuShowCallback);
    evas_object_del(menu);
  }

  iupAttribSet(ih, "_IUP_EFL_MENU", NULL);
  ih->handle = NULL;
}

void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = eflMenuMapMethod;
  ic->UnMap = eflMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflMenuSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}
