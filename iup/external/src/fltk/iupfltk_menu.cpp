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
#include <FL/Fl_Image.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multi_Label.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_menu.h"
}

#include "iupfltk_drv.h"

typedef std::vector<Fl_Multi_Label*> FltkMenuLabelList;


/* Escape `/` (FLTK submenu separator); the `\t` shortcut tail is drawn right-aligned by fl_draw_shortcut. */
static char* fltkMenuBuildLabel(const char* title)
{
  if (!title) title = "";

  size_t len = strlen(title);

  /* Worst case: every char needs escaping */
  char* out = (char*)malloc(len * 2 + 1);
  size_t j = 0;
  for (size_t i = 0; i < len; i++)
  {
    if (title[i] == '/')
      out[j++] = '\\';
    out[j++] = title[i];
  }
  out[j] = 0;
  return out;
}

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
 * Image support via Fl_Multi_Label (image left, text right)
 ****************************************************************************/

static Fl_RGB_Image* fltkMenuItemPickImage(Ihandle* ih)
{
  const char* name = NULL;

  if (iupAttribGetBoolean(ih, "VALUE"))
    name = iupAttribGet(ih, "IMPRESS");
  if (!name)
    name = iupAttribGet(ih, "IMAGE");
  if (!name)
    name = iupAttribGet(ih, "TITLEIMAGE");
  if (!name)
    return NULL;

  return (Fl_RGB_Image*)iupImageGetImage(name, ih, 0, NULL);
}

static void fltkMenuApplyImage(Fl_Menu_* menuwidget, int item_idx, Ihandle* ih, FltkMenuLabelList* labels)
{
  if (item_idx < 0)
    return;

  Fl_RGB_Image* image = fltkMenuItemPickImage(ih);
  if (!image)
    return;

  Fl_Menu_Item* item = (Fl_Menu_Item*)&menuwidget->menu()[item_idx];

  Fl_Multi_Label* ml = new Fl_Multi_Label;
  ml->typea = FL_IMAGE_LABEL;
  ml->labela = (const char*)image;
  ml->typeb = FL_NORMAL_LABEL;
  ml->labelb = item->label();
  item->multi_label(ml);

  labels->push_back(ml);
}

static FltkMenuLabelList* fltkMenuLabelsAttach(Ihandle* ih_menu)
{
  FltkMenuLabelList* labels = (FltkMenuLabelList*)iupAttribGet(ih_menu, "_IUPFLTK_MENU_LABELS");
  if (!labels)
  {
    labels = new FltkMenuLabelList;
    iupAttribSet(ih_menu, "_IUPFLTK_MENU_LABELS", (char*)labels);
  }
  return labels;
}

static void fltkMenuLabelsClear(FltkMenuLabelList* labels)
{
  if (!labels)
    return;
  for (Fl_Multi_Label* ml : *labels)
    delete ml;
  labels->clear();
}

static void fltkMenuLabelsDestroy(Ihandle* ih_menu)
{
  FltkMenuLabelList* labels = (FltkMenuLabelList*)iupAttribGet(ih_menu, "_IUPFLTK_MENU_LABELS");
  if (!labels)
    return;
  fltkMenuLabelsClear(labels);
  delete labels;
  iupAttribSet(ih_menu, "_IUPFLTK_MENU_LABELS", NULL);
}

/****************************************************************************
 * Menu Rebuild - Walks IUP tree and populates Fl_Menu_ widget
 ****************************************************************************/

static void fltkMenuAddItems(Fl_Menu_* menuwidget, Ihandle* ih_menu, const char* path_prefix, FltkMenuLabelList* labels)
{
  Ihandle* child;
  int last_item_idx = -1;

  for (child = ih_menu->firstchild; child; child = child->brother)
  {
    const char* class_name = child->iclass->name;

    if (iupStrEqual(class_name, "submenu"))
    {
      char* title = iupAttribGet(child, "TITLE");
      char* label = fltkMenuBuildLabel(title);

      char path[512];
      if (path_prefix && *path_prefix)
        snprintf(path, sizeof(path), "%s/%s", path_prefix, label);
      else
        snprintf(path, sizeof(path), "%s", label);

      free(label);

      int flags = FL_SUBMENU;

      if (!iupdrvIsActive(child))
        flags |= FL_MENU_INACTIVE;

      last_item_idx = menuwidget->add(path, 0, NULL, (void*)child, flags);
      fltkMenuApplyImage(menuwidget, last_item_idx, child, labels);

      Ihandle* submenu_menu = child->firstchild;
      if (submenu_menu)
        fltkMenuAddItems(menuwidget, submenu_menu, path, labels);
    }
    else if (iupStrEqual(class_name, "menuitem"))
    {
      char* title = iupAttribGet(child, "TITLE");
      char* label = fltkMenuBuildLabel(title);

      char path[512];
      if (path_prefix && *path_prefix)
        snprintf(path, sizeof(path), "%s/%s", path_prefix, label);
      else
        snprintf(path, sizeof(path), "%s", label);

      free(label);

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
      fltkMenuApplyImage(menuwidget, last_item_idx, child, labels);
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

  FltkMenuLabelList* labels = fltkMenuLabelsAttach(ih_menu);
  fltkMenuLabelsClear(labels);

  menuwidget->clear();
  fltkMenuAddItems(menuwidget, ih_menu, "", labels);
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
  FltkMenuLabelList labels;

  fltkMenuAddItems(&popup, ih, "", &labels);

  int ox = Fl::event_x_root() - Fl::event_x();
  int oy = Fl::event_y_root() - Fl::event_y();
  const Fl_Menu_Item* m = popup.menu()->popup(x - ox, y - oy, NULL, NULL, NULL);
  if (m)
    popup.picked(m);

  for (Fl_Multi_Label* ml : labels)
    delete ml;

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
  fltkMenuLabelsDestroy(ih);

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

    Ihandle* item = IupMenuItem(filenames[i]);
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

static int fltkMenuItemSetImageAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "IMAGE", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
}

static int fltkMenuItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "IMPRESS", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
}

static int fltkMenuItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "TITLEIMAGE", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
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
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, fltkMenuItemSetTitleImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, fltkMenuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, fltkMenuItemSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

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

static int fltkSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "IMAGE", value);
  fltkMenuTriggerRebuild(ih);
  return 1;
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
  iupClassRegisterAttribute(ic, "IMAGE", NULL, fltkSubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}
