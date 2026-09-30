/** \file
 * \brief Menu Resources - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QWidget>
#include <QString>
#include <QPixmap>
#include <QKeySequence>
#include <QPoint>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QEvent>

#include <cstdlib>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_menu.h"
}

#include "iupqt_drv.h"


typedef struct _ImenuPos
{
  int x, y;
  Ihandle* ih;
} ImenuPos;

/****************************************************************************
 * Menu Popup
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  auto* menu = reinterpret_cast<QMenu*>(ih->handle);

  if (!menu)
  {
    return IUP_ERROR;
  }

  const char* value = iupAttribGet(ih, "POPUPALIGN");
  QPoint pos(x, y);
  QSize menuSize = menu->sizeHint();

  if (value)
  {
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      pos.setX(x - menuSize.width());
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      pos.setX(x - menuSize.width() / 2);

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      pos.setY(y - menuSize.height());
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      pos.setY(y - menuSize.height() / 2);
  }
  else
  {
    /* flip the popup when it would extend past the screen edge */
    QScreen* screen = QGuiApplication::screenAt(pos);
    if (screen)
    {
      QRect screenGeom = screen->availableGeometry();

      if (pos.y() + menuSize.height() > screenGeom.bottom())
        pos.setY(y - menuSize.height());

      if (pos.x() + menuSize.width() > screenGeom.right())
        pos.setX(x - menuSize.width());
    }
  }

  menu->exec(pos);

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  if (ih && ih->handle && iupMenuIsMenuBar(ih))
  {
    auto* menubar = reinterpret_cast<QMenuBar*>(ih->handle);
    int h = menubar->height();
    if (h <= 0)
      h = menubar->sizeHint().height();
    if (h > 0)
      return h;
  }

  /* Fallback estimate for when the menu bar is not yet mapped or has no size. */
  int ch;
  iupdrvFontGetCharSize(ih, nullptr, &ch);
  return 4 + ch + 4;
}

/****************************************************************************
 * Menu Callbacks
 ****************************************************************************/

static QAction* qtMenuGetNextAction(Ihandle* ih)
{
  Ihandle* next;
  for (next = ih->brother; next; next = next->brother)
    if (next->handle)
      return reinterpret_cast<QAction*>(next->handle);
  return nullptr;
}

static void qtMenuAboutToShow(Ihandle* ih)
{
  auto cb = static_cast<Icallback>(IupGetCallback(ih, "MENUOPEN_CB"));
  if (!cb && ih->parent)
    cb = static_cast<Icallback>(IupGetCallback(ih->parent, "MENUOPEN_CB"));
  if (cb)
    cb(ih);
}

static void qtMenuAboutToHide(Ihandle* ih)
{
  auto cb = static_cast<Icallback>(IupGetCallback(ih, "MENUCLOSE_CB"));
  if (!cb && ih->parent)
    cb = static_cast<Icallback>(IupGetCallback(ih->parent, "MENUCLOSE_CB"));
  if (cb)
    cb(ih);
}

/****************************************************************************
 * Item Callbacks
 ****************************************************************************/

static void qtMenuItemHighlight(Ihandle* ih)
{
  auto cb = static_cast<Icallback>(IupGetCallback(ih, "HIGHLIGHT_CB"));
  if (cb)
    cb(ih);
}

static void qtMenuItemTriggered(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);

  /* Undo Qt's auto-toggle so user callback sees the original state */
  if (action->isCheckable() && !iupAttribGetBoolean(ih, "AUTOTOGGLE") && !iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    action->blockSignals(true);
    action->setChecked(!action->isChecked());
    action->blockSignals(false);
  }

  /* QAction::activate auto-toggles checkable items; only AUTOTOGGLE ones need a manual flip */
  if (!action->isCheckable() && iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    if (iupAttribGetBoolean(ih, "VALUE"))
      iupAttribSet(ih, "VALUE", "OFF");
    else
      iupAttribSet(ih, "VALUE", "ON");

    auto* pixbuf = static_cast<QPixmap*>(iupImageGetImage(iupAttribGet(ih, "IMAGE"), ih, 0, nullptr));
    if (iupStrBoolean(iupAttribGet(ih, "VALUE")))
    {
      const char* impress = iupAttribGet(ih, "IMPRESS");
      if (impress)
        pixbuf = static_cast<QPixmap*>(iupImageGetImage(impress, ih, 0, nullptr));
    }
    if (pixbuf)
      action->setIcon(QIcon(*pixbuf));
  }

  auto cb = static_cast<Icallback>(IupGetCallback(ih, "ACTION"));
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qtMenuItemUpdateImage(Ihandle* ih, const char* value, const char* image, const char* impress)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  QPixmap* pixbuf = nullptr;

  if (!impress || !iupStrBoolean(value))
    pixbuf = static_cast<QPixmap*>(iupImageGetImage(image, ih, 0, nullptr));
  else
    pixbuf = static_cast<QPixmap*>(iupImageGetImage(impress, ih, 0, nullptr));

  if (pixbuf)
    action->setIcon(QIcon(*pixbuf));
  else
    action->setIcon(QIcon());
}

/****************************************************************************
 * Menu Map/UnMap
 ****************************************************************************/

static int qtMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    auto* menubar = new QMenuBar();

    /* QMenuBar sizes to its contents; it has to span the whole window width */
    menubar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    ih->handle = reinterpret_cast<InativeHandle*>(menubar);

    /* children add themselves to the bar when IUP maps them */

    iupqtAddToParent(ih);
  }
  else
  {
    auto* menu = new QMenu();

    ih->handle = reinterpret_cast<InativeHandle*>(menu);

    if (ih->parent)
    {
      auto* parent_action = reinterpret_cast<QAction*>(ih->parent->handle);
      parent_action->setMenu(menu);

      QObject::connect(menu, &QMenu::aboutToShow, [ih]() {
        qtMenuAboutToShow(ih);
      });
      QObject::connect(menu, &QMenu::aboutToHide, [ih]() {
        qtMenuAboutToHide(ih);
      });
    }
    else
    {
      QObject::connect(menu, &QMenu::aboutToShow, [ih]() {
        qtMenuAboutToShow(ih);
      });
      QObject::connect(menu, &QMenu::aboutToHide, [ih]() {
        qtMenuAboutToHide(ih);
      });
    }
  }

  ih->serial = iupMenuGetChildId(ih);

  return IUP_NOERROR;
}

static void qtMenuUnMapMethod(Ihandle* ih)
{
  auto* radio_group = reinterpret_cast<QActionGroup*>(iupAttribGet(ih, "_IUPQT_RADIOGROUP"));
  delete radio_group;

  if (iupMenuIsMenuBar(ih))
    ih->parent = nullptr;

  auto* widget = reinterpret_cast<QWidget*>(ih->handle);
  delete widget;
}

/****************************************************************************
 * Menu Attributes
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtMenuMapMethod;
  ic->UnMap = qtMenuUnMapMethod;

  /* Used by iupdrvMenuGetMenuBarSize */
  iupClassRegisterAttribute(ic, "FONT", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, nullptr, nullptr, IUPAF_DEFAULT);
}

/****************************************************************************
 * Item Attribute Getters
 ****************************************************************************/

static char* qtMenuItemGetActiveAttrib(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  if (!action)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean(action->isEnabled());
}

/****************************************************************************
 * Item Attribute Setters
 ****************************************************************************/

static int qtMenuItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  qtMenuItemUpdateImage(ih, nullptr, value, nullptr);
  return 1;
}

static int qtMenuItemSetImageAttrib(Ihandle* ih, const char* value)
{
  qtMenuItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), value, iupAttribGet(ih, "IMPRESS"));
  return 1;
}

static int qtMenuItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  qtMenuItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), value);
  return 1;
}

static void qtMenuItemSetAccel(Ihandle* ih, QAction* action, const char* title)
{
  int code = iupMenuGetAccel(title);
  unsigned int keyval, state;

  if (!code || !IupGetDialog(ih))
  {
    action->setShortcut(QKeySequence());
    return;
  }

  iupdrvKeyEncode(code, &keyval, &state);
  action->setShortcut(QKeySequence(static_cast<int>(keyval | state)));
}

static int qtMenuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  char* str;

  if (!value)
  {
    str = const_cast<char*>("     ");
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  action->setText(QString::fromUtf8(str));
  qtMenuItemSetAccel(ih, action, str);

  if (str != value)
    free(str);

  return 1;
}

static int qtMenuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);

  if (action->isCheckable())
  {
    if (iupAttribGetBoolean(ih->parent, "RADIO"))
      value = "ON";

    action->setChecked(iupStrBoolean(value));
    return 0;
  }
  else
  {
    qtMenuItemUpdateImage(ih, value, iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
    return 1;
  }
}

static char* qtMenuItemGetValueAttrib(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);

  if (action->isCheckable())
    return iupStrReturnChecked(action->isChecked());
  else
    return nullptr;
}

/****************************************************************************
 * Item Map
 ****************************************************************************/

static int qtMenuItemMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  auto* action = new QAction();

  ih->handle = reinterpret_cast<InativeHandle*>(action);
  ih->serial = iupMenuGetChildId(ih);

  bool is_radio = iupAttribGetBoolean(ih->parent, "RADIO");
  bool has_image = (iupAttribGet(ih, "IMAGE") != nullptr || iupAttribGet(ih, "TITLEIMAGE") != nullptr);

  if (is_radio)
  {
    action->setCheckable(true);

    auto* radio_group = reinterpret_cast<QActionGroup*>(iupAttribGet(ih->parent, "_IUPQT_RADIOGROUP"));
    if (!radio_group)
    {
      radio_group = new QActionGroup(nullptr);
      radio_group->setExclusive(true);
      iupAttribSet(ih->parent, "_IUPQT_RADIOGROUP", reinterpret_cast<char*>(radio_group));
    }
    action->setActionGroup(radio_group);
  }
  else if (!has_image)
  {
    const char* hidemark = iupAttribGetStr(ih, "HIDEMARK");
    if (!hidemark && !iupAttribGet(ih, "VALUE"))
      hidemark = "YES";

    if (!iupStrBoolean(hidemark))
    {
      action->setCheckable(true);
    }
  }

  QObject::connect(action, &QAction::hovered, [ih]() {
    qtMenuItemHighlight(ih);
  });

  QObject::connect(action, &QAction::triggered, [ih]() {
    qtMenuItemTriggered(ih);
  });

  {
    QAction* before = qtMenuGetNextAction(ih);
    if (iupMenuIsMenuBar(ih->parent))
    {
      auto* menubar = reinterpret_cast<QMenuBar*>(ih->parent->handle);
      menubar->insertAction(before, action);
    }
    else
    {
      auto* menu = reinterpret_cast<QMenu*>(ih->parent->handle);
      menu->insertAction(before, action);
    }
  }

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Item Class Init
 ****************************************************************************/

static void qtMenuItemUnMapMethod(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  delete action;
}

extern "C" IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtMenuItemMapMethod;
  ic->UnMap = qtMenuItemUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", qtMenuItemGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupMenuItem only */
  iupClassRegisterAttribute(ic, "VALUE", qtMenuItemGetValueAttrib, qtMenuItemSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", nullptr, qtMenuItemSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, qtMenuItemSetTitleImageAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qtMenuItemSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, qtMenuItemSetImpressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* IupMenuItem specific */
  iupClassRegisterAttribute(ic, "HIDEMARK", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "AUTOTOGGLE", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
}

/****************************************************************************
 * Submenu Attribute Getters
 ****************************************************************************/

static char* qtSubmenuGetActiveAttrib(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  if (!action)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean(action->isEnabled());
}

/****************************************************************************
 * Submenu Attribute Setters
 ****************************************************************************/

static int qtSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);

  if (value)
  {
    auto* pixbuf = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
    if (pixbuf)
      action->setIcon(QIcon(*pixbuf));
  }
  else
  {
    action->setIcon(QIcon());
  }

  return 1;
}

static int qtSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  char* str;

  if (!value)
  {
    str = const_cast<char*>("     ");
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  QString title = QString::fromUtf8(str);
  action->setText(title);

  if (str != value)
    free(str);

  return 1;
}

/****************************************************************************
 * Submenu Map
 ****************************************************************************/

static int qtSubmenuMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  auto* action = new QAction();

  ih->handle = reinterpret_cast<InativeHandle*>(action);
  ih->serial = iupMenuGetChildId(ih);

  QObject::connect(action, &QAction::hovered, [ih]() {
    qtMenuItemHighlight(ih);
  });

  char* title = iupAttribGet(ih, "TITLE");
  if (title)
  {
    char* str = iupMenuProcessTitle(ih, title);
    action->setText(QString::fromUtf8(str));
    qtMenuItemSetAccel(ih, action, str);
    if (str != title)
      free(str);
  }

  {
    QAction* before = qtMenuGetNextAction(ih);
    if (iupMenuIsMenuBar(ih->parent))
    {
      auto* menubar = reinterpret_cast<QMenuBar*>(ih->parent->handle);
      menubar->insertAction(before, action);
    }
    else
    {
      auto* menu = reinterpret_cast<QMenu*>(ih->parent->handle);
      menu->insertAction(before, action);
    }
  }

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Submenu Class Init
 ****************************************************************************/

static void qtSubmenuUnMapMethod(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  delete action;
}

extern "C" IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtSubmenuMapMethod;
  ic->UnMap = qtSubmenuUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", qtSubmenuGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupSubmenu only */
  iupClassRegisterAttribute(ic, "TITLE", nullptr, qtSubmenuSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qtSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, qtSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}

/****************************************************************************
 * Separator Map
 ****************************************************************************/

static int qtMenuSeparatorMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  auto* action = new QAction();
  action->setSeparator(true);

  ih->handle = reinterpret_cast<InativeHandle*>(action);
  ih->serial = iupMenuGetChildId(ih);

  {
    QAction* before = qtMenuGetNextAction(ih);
    if (iupMenuIsMenuBar(ih->parent))
    {
      auto* menubar = reinterpret_cast<QMenuBar*>(ih->parent->handle);
      menubar->insertAction(before, action);
    }
    else
    {
      auto* menu = reinterpret_cast<QMenu*>(ih->parent->handle);
      menu->insertAction(before, action);
    }
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * Separator Class Init
 ****************************************************************************/

static void qtMenuSeparatorUnMapMethod(Ihandle* ih)
{
  auto* action = reinterpret_cast<QAction*>(ih->handle);
  delete action;
}

extern "C" IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = qtMenuSeparatorMapMethod;
  ic->UnMap = qtMenuSeparatorUnMapMethod;
}


/****************************************************************************
 * Recent Menu Support
 ****************************************************************************/

static void qtRecentItemTriggered(Ihandle* menu, int index)
{
  auto recent_cb = reinterpret_cast<Icallback>(iupAttribGet(menu, "_IUP_RECENT_CB"));
  auto* config = reinterpret_cast<Ihandle*>(iupAttribGet(menu, "_IUP_CONFIG"));

  if (recent_cb && config)
  {
    char attr_name[32];
    const char* filename;

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", index);
    filename = iupAttribGet(menu, attr_name);

    if (filename)
    {
      IupSetStrAttribute(config, "RECENTFILENAME", filename);
      IupSetStrAttribute(config, "TITLE", filename);
      config->parent = menu;

      if (recent_cb(config) == IUP_CLOSE)
        IupExitLoop();

      config->parent = nullptr;
      IupSetAttribute(config, "RECENTFILENAME", nullptr);
      IupSetAttribute(config, "TITLE", nullptr);
    }
  }
}

extern "C" IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", reinterpret_cast<char*>(recent_cb));
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

extern "C" IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  QMenu* qmenu;
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  qmenu = reinterpret_cast<QMenu*>(menu->handle);
  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", reinterpret_cast<char*>(recent_cb));

  QList<QAction*> actions = qmenu->actions();

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    QString title = QString::fromUtf8(filenames[i]);

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    if (i < existing && i < actions.size())
    {
      actions[i]->setText(title);
    }
    else
    {
      auto* action = new QAction(title, qmenu);
      action->setData(QVariant(i));

      QObject::connect(action, &QAction::triggered, [menu, i]() {
        qtRecentItemTriggered(menu, i);
      });

      qmenu->addAction(action);
    }
  }

  actions = qmenu->actions();
  while (actions.size() > count && existing > count)
  {
    QAction* action = actions.last();
    qmenu->removeAction(action);
    delete action;

    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", existing - 1);
    iupAttribSet(menu, attr_name, nullptr);

    existing--;
    actions = qmenu->actions();
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}
