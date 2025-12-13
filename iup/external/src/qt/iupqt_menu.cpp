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
#include <QCursor>
#include <QPoint>
#include <QApplication>
#include <QEvent>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_label.h"
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

extern "C" int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  QMenu* menu = (QMenu*)ih->handle;

  if (!menu)
  {
    return IUP_ERROR;
  }

  const char* value = iupAttribGet(ih, "POPUPALIGN");
  QPoint pos(x, y);

  if (value)
  {
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

    QSize size = menu->sizeHint();

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      pos.setX(x - size.width());
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      pos.setX(x - size.width() / 2);

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      pos.setY(y - size.height());
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      pos.setY(y - size.height() / 2);
  }

  menu->exec(pos);

  return IUP_NOERROR;
}

extern "C" int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, nullptr, &ch);
  return 4 + ch + 4;
}

/****************************************************************************
 * Menu Callbacks
 ****************************************************************************/

static void qtMenuAboutToShow(Ihandle* ih)
{
  Icallback cb = (Icallback)IupGetCallback(ih, "OPEN_CB");
  if (!cb && ih->parent)
    cb = (Icallback)IupGetCallback(ih->parent, "OPEN_CB");
  if (cb)
    cb(ih);
}

static void qtMenuAboutToHide(Ihandle* ih)
{
  Icallback cb = (Icallback)IupGetCallback(ih, "MENUCLOSE_CB");
  if (!cb && ih->parent)
    cb = (Icallback)IupGetCallback(ih->parent, "MENUCLOSE_CB");
  if (cb)
    cb(ih);
}

/****************************************************************************
 * Item Callbacks
 ****************************************************************************/

static void qtItemHighlight(Ihandle* ih)
{
  Icallback cb = (Icallback)IupGetCallback(ih, "HIGHLIGHT_CB");
  if (cb)
    cb(ih);
}

static void qtItemTriggered(Ihandle* ih)
{
  QAction* action = (QAction*)ih->handle;

  /* Handle AUTOTOGGLE for items with images */
  if (!action->isCheckable() && iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    if (iupAttribGetBoolean(ih, "VALUE"))
      iupAttribSet(ih, "VALUE", "OFF");
    else
      iupAttribSet(ih, "VALUE", "ON");

    /* Update image if needed */
    QPixmap* pixbuf = (QPixmap*)iupImageGetImage(iupAttribGet(ih, "IMAGE"), ih, 0, nullptr);
    if (iupStrBoolean(iupAttribGet(ih, "VALUE")))
    {
      const char* impress = iupAttribGet(ih, "IMPRESS");
      if (impress)
        pixbuf = (QPixmap*)iupImageGetImage(impress, ih, 0, nullptr);
    }
    if (pixbuf)
      action->setIcon(QIcon(*pixbuf));
  }

  /* Call ACTION callback */
  Icallback cb = (Icallback)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qtItemUpdateImage(Ihandle* ih, const char* value, const char* image, const char* impress)
{
  QAction* action = (QAction*)ih->handle;
  QPixmap* pixbuf = nullptr;

  if (!impress || !iupStrBoolean(value))
    pixbuf = (QPixmap*)iupImageGetImage(image, ih, 0, nullptr);
  else
    pixbuf = (QPixmap*)iupImageGetImage(impress, ih, 0, nullptr);

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
    /* Top level menu used for MENU attribute in IupDialog (a menu bar) */
    QMenuBar* menubar = new QMenuBar();
    if (!menubar)
      return IUP_ERROR;

    /* Set size policy to expand horizontally to fill the window width.
     * By default QMenuBar has a preferred size based on its contents, but
     * we want it to always span the full width of the QMainWindow. */
    menubar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    ih->handle = (InativeHandle*)menubar;

    /* Don't add children here - they will be mapped later by IUP and will
     * add themselves to this menu bar when they are mapped. */

    iupqtAddToParent(ih);
  }
  else
  {
    /* Popup menu or submenu */
    QMenu* menu = new QMenu();
    if (!menu)
      return IUP_ERROR;

    ih->handle = (InativeHandle*)menu;

    if (ih->parent)
    {
      /* Parent is a submenu - set this as submenu */
      QAction* parent_action = (QAction*)ih->parent->handle;
      parent_action->setMenu(menu);

      /* Connect signals */
      QObject::connect(menu, &QMenu::aboutToShow, [ih]() {
        qtMenuAboutToShow(ih);
      });
      QObject::connect(menu, &QMenu::aboutToHide, [ih]() {
        qtMenuAboutToHide(ih);
      });
    }
    else
    {
      /* Top level popup menu */
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
  if (iupMenuIsMenuBar(ih))
    ih->parent = nullptr;

  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Menu Attributes
 ****************************************************************************/

extern "C" void iupdrvMenuInitClass(Iclass* ic)
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

static char* qtItemGetActiveAttrib(Ihandle* ih)
{
  QAction* action = (QAction*)ih->handle;
  if (!action)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean(action->isEnabled());
}

static char* qtItemGetTitleAttrib(Ihandle* ih)
{
  QAction* action = (QAction*)ih->handle;
  if (!action)
    return nullptr;

  QString title = action->text();
  if (!title.isEmpty())
    return iupStrReturnStr(title.toUtf8().constData());

  return nullptr;
}

/****************************************************************************
 * Item Attribute Setters
 ****************************************************************************/

static int qtItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  qtItemUpdateImage(ih, nullptr, value, nullptr);
  return 1;
}

static int qtItemSetImageAttrib(Ihandle* ih, const char* value)
{
  qtItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), value, iupAttribGet(ih, "IMPRESS"));
  return 1;
}

static int qtItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  qtItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), value);
  return 1;
}

static int qtItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  QAction* action = (QAction*)ih->handle;
  char* str;

  if (!value)
  {
    str = (char*)"     ";
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  /* Convert & to && for Qt (Qt uses && for mnemonic, IUP uses single &) */
  QString title = QString::fromUtf8(str);

  /* Extract mnemonic if present */
  int mnemonic_pos = title.indexOf('&');
  if (mnemonic_pos >= 0 && mnemonic_pos < title.length() - 1)
  {
    /* Qt uses single & for mnemonic */
    action->setText(title);
  }
  else
  {
    action->setText(title);
  }

  if (str != value)
    free(str);

  return 1;
}

static int qtItemSetValueAttrib(Ihandle* ih, const char* value)
{
  QAction* action = (QAction*)ih->handle;

  if (action->isCheckable())
  {
    /* For radio items, always set to ON */
    if (iupAttribGetBoolean(ih->parent, "RADIO"))
      value = "ON";

    /* Block signals to prevent triggering callback */
    action->blockSignals(true);
    action->setChecked(iupStrBoolean(value));
    action->blockSignals(false);

    return 0;
  }
  else
  {
    /* Update image based on value */
    qtItemUpdateImage(ih, value, iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
    return 1;
  }
}

static char* qtItemGetValueAttrib(Ihandle* ih)
{
  QAction* action = (QAction*)ih->handle;

  if (action->isCheckable())
    return iupStrReturnChecked(action->isChecked());
  else
    return nullptr;
}

static int qtItemSetKeyAttrib(Ihandle* ih, const char* value)
{
  QAction* action = (QAction*)ih->handle;

  if (value)
  {
    /* Convert IUP key string to Qt key sequence */
    /* QKeySequence handles standard formats like "Ctrl+S", "Shift+Alt+K", etc. */
    QString key_str = QString::fromUtf8(value);
    action->setShortcut(QKeySequence(key_str));
  }
  else
  {
    action->setShortcut(QKeySequence());
  }

  return 1;
}

/****************************************************************************
 * Item Map
 ****************************************************************************/

static int qtItemMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  QAction* action = new QAction();
  if (!action)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)action;
  ih->serial = iupMenuGetChildId(ih);

  /* Determine item type */
  bool is_radio = iupAttribGetBoolean(ih->parent, "RADIO");
  bool has_image = (iupAttribGet(ih, "IMAGE") != nullptr || iupAttribGet(ih, "TITLEIMAGE") != nullptr);

  if (is_radio)
  {
    /* Radio item */
    action->setCheckable(true);

    /* Get or create radio group for this menu */
    QActionGroup* radio_group = (QActionGroup*)iupAttribGet(ih->parent, "_IUPQT_RADIOGROUP");
    if (!radio_group)
    {
      radio_group = new QActionGroup(nullptr);
      radio_group->setExclusive(true);
      iupAttribSet(ih->parent, "_IUPQT_RADIOGROUP", (char*)radio_group);
    }
    action->setActionGroup(radio_group);
  }
  else if (!has_image)
  {
    /* Check if HIDEMARK is set */
    const char* hidemark = iupAttribGetStr(ih, "HIDEMARK");
    if (!hidemark && !iupAttribGet(ih, "VALUE"))
      hidemark = "YES"; /* Default to YES if no VALUE */

    if (!iupStrBoolean(hidemark))
    {
      /* Checkable item */
      action->setCheckable(true);
    }
  }

  /* Connect signals */
  QObject::connect(action, &QAction::hovered, [ih]() {
    qtItemHighlight(ih);
  });

  QObject::connect(action, &QAction::triggered, [ih]() {
    qtItemTriggered(ih);
  });

  /* Add to parent menu */
  if (iupMenuIsMenuBar(ih->parent))
  {
    QMenuBar* menubar = (QMenuBar*)ih->parent->handle;
    menubar->addAction(action);
  }
  else
  {
    QMenu* menu = (QMenu*)ih->parent->handle;
    menu->addAction(action);
  }

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Item Class Init
 ****************************************************************************/

extern "C" void iupdrvItemInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtItemMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", qtItemGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupItem only */
  iupClassRegisterAttribute(ic, "VALUE", qtItemGetValueAttrib, qtItemSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", qtItemGetTitleAttrib, qtItemSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, qtItemSetTitleImageAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qtItemSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, qtItemSetImpressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "KEY", nullptr, qtItemSetKeyAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  /* IupItem specific */
  iupClassRegisterAttribute(ic, "HIDEMARK", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "AUTOTOGGLE", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
}

/****************************************************************************
 * Submenu Attribute Getters
 ****************************************************************************/

static char* qtSubmenuGetActiveAttrib(Ihandle* ih)
{
  QAction* action = (QAction*)ih->handle;
  if (!action)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean(action->isEnabled());
}

static char* qtSubmenuGetTitleAttrib(Ihandle* ih)
{
  QAction* action = (QAction*)ih->handle;
  if (!action)
    return nullptr;

  QString title = action->text();
  if (!title.isEmpty())
    return iupStrReturnStr(title.toUtf8().constData());

  return nullptr;
}

/****************************************************************************
 * Submenu Attribute Setters
 ****************************************************************************/

static int qtSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  QAction* action = (QAction*)ih->handle;

  if (value)
  {
    QPixmap* pixbuf = (QPixmap*)iupImageGetImage(value, ih, 0, nullptr);
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
  QAction* action = (QAction*)ih->handle;
  char* str;

  if (!value)
  {
    str = (char*)"     ";
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

  /* Create action for submenu */
  QAction* action = new QAction();
  if (!action)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)action;
  ih->serial = iupMenuGetChildId(ih);

  /* Connect highlight signal */
  QObject::connect(action, &QAction::hovered, [ih]() {
    qtItemHighlight(ih);
  });

  /* Set initial title from TITLE attribute. */
  char* title = iupAttribGet(ih, "TITLE");
  if (title)
  {
    char* str = iupMenuProcessTitle(ih, title);
    action->setText(QString::fromUtf8(str));
    if (str != title)
      free(str);
  }

  /* Add to parent menu */
  if (iupMenuIsMenuBar(ih->parent))
  {
    QMenuBar* menubar = (QMenuBar*)ih->parent->handle;
    menubar->addAction(action);
  }
  else
  {
    QMenu* menu = (QMenu*)ih->parent->handle;
    menu->addAction(action);
  }

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Submenu Class Init
 ****************************************************************************/

extern "C" void iupdrvSubmenuInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtSubmenuMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", qtSubmenuGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupSubmenu only */
  iupClassRegisterAttribute(ic, "TITLE", qtSubmenuGetTitleAttrib, qtSubmenuSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qtSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, qtSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}

/****************************************************************************
 * Separator Map
 ****************************************************************************/

static int qtSeparatorMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  /* Create separator action */
  QAction* action = new QAction();
  if (!action)
    return IUP_ERROR;

  action->setSeparator(true);

  ih->handle = (InativeHandle*)action;
  ih->serial = iupMenuGetChildId(ih);

  /* Add to parent menu */
  if (iupMenuIsMenuBar(ih->parent))
  {
    QMenuBar* menubar = (QMenuBar*)ih->parent->handle;
    menubar->addAction(action);
  }
  else
  {
    QMenu* menu = (QMenu*)ih->parent->handle;
    menu->addAction(action);
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * Separator Class Init
 ****************************************************************************/

extern "C" void iupdrvSeparatorInitClass(Iclass* ic)
{
  ic->Map = qtSeparatorMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
}


/****************************************************************************
 * Recent Menu Support
 ****************************************************************************/

static void qtRecentItemTriggered(Ihandle* menu, int index)
{
  Icallback recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  Ihandle* config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");

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

extern "C" int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

extern "C" int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  QMenu* qmenu;
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  qmenu = (QMenu*)menu->handle;
  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

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
      QAction* action = new QAction(title, qmenu);
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
