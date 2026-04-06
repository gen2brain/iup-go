/** \file
 * \brief Menu Resources - FLTK Implementation
 *
 * FLTK uses an array-based menu model (Fl_Menu_Item[]) unlike GTK/Qt which
 * use widget/action trees. Menu items don't have individual native handles.
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_menu.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void fltkMenuItemActionCb(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  (void)w;

  if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    if (iupAttribGetBoolean(ih, "VALUE"))
      iupAttribSet(ih, "VALUE", "OFF");
    else
      iupAttribSet(ih, "VALUE", "ON");
  }

  Icallback cb = (Icallback)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

/****************************************************************************
 * Menu Rebuild - Walks IUP tree and populates Fl_Menu_ widget
 ****************************************************************************/

static void fltkMenuAddItems(Fl_Menu_* menuwidget, Ihandle* ih_menu, const char* path_prefix)
{
  Ihandle* child;
  int last_item_idx = -1;

  for (child = ih_menu->firstchild; child; child = child->brother)
  {
    const char* class_name = child->iclass->name;

    if (iupStrEqual(class_name, "submenu"))
    {
      char* title = iupAttribGet(child, "TITLE");
      if (!title) title = (char*)"";

      char c;
      char* clean_title = iupStrProcessMnemonic(title, &c, -1);

      char path[512];
      if (path_prefix && *path_prefix)
        snprintf(path, sizeof(path), "%s/%s", path_prefix, clean_title);
      else
        snprintf(path, sizeof(path), "%s", clean_title);

      if (clean_title != title) free(clean_title);

      int flags = FL_SUBMENU;

      if (!iupdrvIsActive(child))
        flags |= FL_MENU_INACTIVE;

      last_item_idx = menuwidget->add(path, 0, NULL, (void*)child, flags);

      Ihandle* submenu_menu = child->firstchild;
      if (submenu_menu)
        fltkMenuAddItems(menuwidget, submenu_menu, path);
    }
    else if (iupStrEqual(class_name, "menuitem"))
    {
      char* title = iupAttribGet(child, "TITLE");
      if (!title) title = (char*)"";

      char c;
      char* clean_title = iupStrProcessMnemonic(title, &c, -1);

      char path[512];
      if (path_prefix && *path_prefix)
        snprintf(path, sizeof(path), "%s/%s", path_prefix, clean_title);
      else
        snprintf(path, sizeof(path), "%s", clean_title);

      if (clean_title != title) free(clean_title);

      int flags = 0;

      if (iupAttribGetBoolean(child->parent, "RADIO"))
        flags |= FL_MENU_RADIO;
      else
      {
        char* hidemark = iupAttribGetStr(child, "HIDEMARK");
        if (!hidemark && !iupAttribGet(child, "VALUE"))
          hidemark = (char*)"YES";

        if (!iupStrBoolean(hidemark))
          flags |= FL_MENU_TOGGLE;
      }

      if (!iupdrvIsActive(child))
        flags |= FL_MENU_INACTIVE;

      if (iupAttribGetBoolean(child, "VALUE"))
        flags |= FL_MENU_VALUE;

      last_item_idx = menuwidget->add(path, 0, fltkMenuItemActionCb, (void*)child, flags);
    }
    else if (iupStrEqual(class_name, "menuseparator"))
    {
      if (last_item_idx >= 0)
      {
        const Fl_Menu_Item* items = menuwidget->menu();
        if (items)
          ((Fl_Menu_Item*)&items[last_item_idx])->flags |= FL_MENU_DIVIDER;
      }
    }
  }
}

static Ihandle* fltkMenuFindMenuBar(Ihandle* ih)
{
  Ihandle* menu = ih->parent;
  while (menu)
  {
    if (iupMenuIsMenuBar(menu))
      return menu;
    menu = menu->parent;
  }
  return NULL;
}

static void fltkMenuRebuild(Ihandle* ih_menu)
{
  Fl_Menu_* menuwidget = (Fl_Menu_*)ih_menu->handle;
  if (!menuwidget)
    return;

  menuwidget->clear();
  fltkMenuAddItems(menuwidget, ih_menu, "");
  menuwidget->redraw();
}

static void fltkMenuTriggerRebuild(Ihandle* ih)
{
  Ihandle* menubar = fltkMenuFindMenuBar(ih);
  if (menubar)
    fltkMenuRebuild(menubar);
}

/****************************************************************************
 * Popup Menu
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  Fl_Menu_Button popup(0, 0, 0, 0);

  fltkMenuAddItems(&popup, ih, "");

  int ox = Fl::event_x_root() - Fl::event_x();
  int oy = Fl::event_y_root() - Fl::event_y();
  const Fl_Menu_Item* m = popup.menu()->popup(x - ox, y - oy, NULL, NULL, NULL);
  if (m)
    popup.picked(m);

  return IUP_NOERROR;
}

/****************************************************************************
 * Menu Bar Size
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
  return 4 + ch + 4;
}

/****************************************************************************
 * Menu Class
 ****************************************************************************/

static int fltkMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    int bar_h = iupdrvMenuGetMenuBarSize(ih);
    Fl_Menu_Bar* menubar = new Fl_Menu_Bar(0, 0, 100, bar_h);
    menubar->textsize(FL_NORMAL_SIZE);
    ih->handle = (InativeHandle*)menubar;

    iupfltkAddToParent(ih);
  }
  else
  {
    ih->handle = (InativeHandle*)1;
  }

  return IUP_NOERROR;
}

static void fltkMenuUnMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    Fl_Menu_Bar* menubar = (Fl_Menu_Bar*)ih->handle;
    if (menubar)
    {
      Fl_Group* parent = menubar->parent();
      if (parent)
        parent->remove(menubar);
      delete menubar;
    }
  }

  ih->handle = NULL;
}

static int fltkMenuSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (iupMenuIsMenuBar(ih) && ih->handle)
  {
    Fl_Menu_Bar* menubar = (Fl_Menu_Bar*)ih->handle;
    menubar->color(fl_rgb_color(r, g, b));
    menubar->redraw();
  }
  return 1;
}

extern "C" IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = fltkMenuMapMethod;
  ic->UnMap = fltkMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkMenuSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}

/****************************************************************************
 * Recent Menu
 ****************************************************************************/

static int fltkRecentItemAction(Ihandle* item)
{
  Ihandle* menu = item->parent;
  if (!menu) return IUP_DEFAULT;

  Icallback recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  Ihandle* config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");
  if (!recent_cb || !config) return IUP_DEFAULT;

  int index = iupAttribGetInt(item, "_IUP_RECENT_INDEX");
  char attr_name[32];
  snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", index);
  const char* filename = iupAttribGet(menu, attr_name);

  if (filename)
  {
    IupSetStrAttribute(config, "RECENTFILENAME", filename);
    IupSetStrAttribute(config, "TITLE", filename);
    config->parent = menu;

    int ret = recent_cb(config);

    config->parent = NULL;
    IupSetAttribute(config, "RECENTFILENAME", NULL);
    IupSetAttribute(config, "TITLE", NULL);

    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  return IUP_DEFAULT;
}

extern "C" IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

extern "C" IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  if (!menu)
    return -1;

  int max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  Ihandle* child = menu->firstchild;
  while (child)
  {
    Ihandle* next = child->brother;
    if (iupAttribGet(child, "_IUP_RECENT_ITEM"))
    {
      IupDetach(child);
      IupDestroy(child);
    }
    child = next;
  }

  for (int i = 0; i < count; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    Ihandle* item = IupMenuItem(filenames[i], NULL);
    iupAttribSet(item, "_IUP_RECENT_ITEM", "1");
    iupAttribSetInt(item, "_IUP_RECENT_INDEX", i);
    IupSetCallback(item, "ACTION", (Icallback)fltkRecentItemAction);
    IupAppend(menu, item);
    IupMap(item);
  }

  fltkMenuTriggerRebuild(menu);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}

/****************************************************************************
 * Item Class
 ****************************************************************************/

static int fltkMenuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  iupAttribSetStr(ih, "TITLE", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
}

static char* fltkMenuItemGetTitleAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "TITLE");
}

static int fltkMenuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "VALUE", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
}

static char* fltkMenuItemGetValueAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "VALUE");
}

static int fltkMenuItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);
  fltkMenuTriggerRebuild(ih);
  return 0;
}

static int fltkMenuItemMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)1;
  ih->serial = iupMenuGetChildId(ih);

  fltkMenuTriggerRebuild(ih);

  return IUP_NOERROR;
}

static void fltkMenuItemUnMapMethod(Ihandle* ih)
{
  ih->handle = NULL;
}

extern "C" IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = fltkMenuItemMapMethod;
  ic->UnMap = fltkMenuItemUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, fltkMenuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", fltkMenuItemGetValueAttrib, fltkMenuItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", fltkMenuItemGetTitleAttrib, fltkMenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "AUTOTOGGLE", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}

/****************************************************************************
 * Separator Class
 ****************************************************************************/

static int fltkMenuSeparatorMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)1;
  ih->serial = iupMenuGetChildId(ih);

  fltkMenuTriggerRebuild(ih);

  return IUP_NOERROR;
}

static void fltkMenuSeparatorUnMapMethod(Ihandle* ih)
{
  ih->handle = NULL;
}

extern "C" IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = fltkMenuSeparatorMapMethod;
  ic->UnMap = fltkMenuSeparatorUnMapMethod;
}

/****************************************************************************
 * Submenu Class
 ****************************************************************************/

static int fltkSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value) value = "";
  iupAttribSetStr(ih, "TITLE", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
}

static char* fltkSubmenuGetTitleAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "TITLE");
}

static int fltkSubmenuMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)1;
  ih->serial = iupMenuGetChildId(ih);

  return IUP_NOERROR;
}

static void fltkSubmenuUnMapMethod(Ihandle* ih)
{
  ih->handle = NULL;
}

extern "C" IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = fltkSubmenuMapMethod;
  ic->UnMap = fltkSubmenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, fltkMenuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", fltkSubmenuGetTitleAttrib, fltkSubmenuSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}
