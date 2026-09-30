/** \file
 * \brief Menu Resources - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include <QPixmap>
#include <QUrl>
#include <QColor>
#include <QKeySequence>
#include <QEventLoop>
#include <QPointer>

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

#include "iupqml_drv.h"


static bool qmlMenuInvoke(QObject* obj, const char* method, const char* type_name, QObject* arg)
{
  return QMetaObject::invokeMethod(obj, method, Qt::DirectConnection, QGenericArgument(type_name, &arg));
}

static bool qmlMenuInvokeAt(QObject* obj, const char* method, int index, const char* type_name, QObject* arg)
{
  return QMetaObject::invokeMethod(obj, method, Qt::DirectConnection, QGenericArgument("int", &index), QGenericArgument(type_name, &arg));
}

static int qmlMenuGetPos(Ihandle* ih)
{
  int pos = 0;
  for (Ihandle* c = ih->parent ? ih->parent->firstchild : nullptr; c && c != ih; c = c->brother)
    if (c->handle)
      pos++;
  return pos;
}

static QObject* qmlMenuGetParentObject(Ihandle* ih)
{
  if (!ih->parent)
    return nullptr;
  return reinterpret_cast<QObject*>(ih->parent->handle);
}

static void qmlMenuAttachItem(Ihandle* ih, QObject* item)
{
  QObject* parent = qmlMenuGetParentObject(ih);
  if (!parent)
    return;

  int pos = qmlMenuGetPos(ih);
  qmlMenuInvokeAt(parent, "insertItem", pos, "QQuickItem*", item);
  if (iupMenuIsMenuBar(ih->parent))
    iupqmlUnsetArrowCursors(qobject_cast<QQuickItem*>(parent));
}

static void qmlMenuDetachItem(Ihandle* ih, QObject* item)
{
  QObject* parent = qmlMenuGetParentObject(ih);
  if (parent)
    qmlMenuInvoke(parent, "removeItem", "QQuickItem*", item);
}

/****************************************************************************
 * Menu Popup
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  auto* menu = reinterpret_cast<QObject*>(ih->handle);
  if (!menu)
    return IUP_ERROR;

  QQuickWindow* window = iupqmlGetParentWindow(ih);
  if (!window)
    return IUP_ERROR;

  QQuickItem* content = window->contentItem();
  QPoint local = window->mapFromGlobal(QPoint(x, y));

  menu->setProperty("parent", QVariant::fromValue(content));

  const char* value = iupAttribGet(ih, "POPUPALIGN");
  if (value)
  {
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');
    int w = static_cast<int>(menu->property("width").toDouble());
    int h = static_cast<int>(menu->property("height").toDouble());

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      local.setX(local.x() - w);
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      local.setX(local.x() - w / 2);

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      local.setY(local.y() - h);
    else if (iupStrEqualNoCase(value2, "ACENTER"))
      local.setY(local.y() - h / 2);
  }

  QEventLoop loop;
  QPointer<QObject> slot = iupqmlConnect(menu, "closed()", [&loop](void**) {
    loop.quit();
  });

  iupqmlReleaseMouseGrab();

  QQuickItem* no_item = nullptr;
  qreal px = local.x(), py = local.y();
  QMetaObject::invokeMethod(menu, "popup", Qt::DirectConnection, QGenericArgument("qreal", &px), QGenericArgument("qreal", &py), QGenericArgument("QQuickItem*", &no_item));

  if (menu->property("visible").toBool())
    loop.exec();

  delete slot;

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
  if (iupqmlMenuBarIsNative())
    return 0;

  if (ih && ih->handle && iupMenuIsMenuBar(ih))
  {
    auto* menubar = reinterpret_cast<QQuickItem*>(ih->handle);
    int h = static_cast<int>(menubar->implicitHeight());
    if (h > 0)
      return h;
  }

  int ch;
  iupdrvFontGetCharSize(ih, nullptr, &ch);
  return 4 + ch + 4;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qmlMenuAboutToShow(Ihandle* ih)
{
  auto cb = static_cast<Icallback>(IupGetCallback(ih, "MENUOPEN_CB"));
  if (!cb && ih->parent)
    cb = static_cast<Icallback>(IupGetCallback(ih->parent, "MENUOPEN_CB"));
  if (cb)
    cb(ih);
}

static void qmlMenuAboutToHide(Ihandle* ih)
{
  auto cb = static_cast<Icallback>(IupGetCallback(ih, "MENUCLOSE_CB"));
  if (!cb && ih->parent)
    cb = static_cast<Icallback>(IupGetCallback(ih->parent, "MENUCLOSE_CB"));
  if (cb)
    cb(ih);
}

static void qmlMenuItemHighlight(Ihandle* ih)
{
  auto cb = static_cast<Icallback>(IupGetCallback(ih, "HIGHLIGHT_CB"));
  if (cb)
    cb(ih);
}

static void qmlMenuItemUpdateImage(Ihandle* ih, const char* value, const char* image, const char* impress)
{
  auto* item = reinterpret_cast<QObject*>(ih->handle);
  QPixmap* pixmap = nullptr;

  if (!impress || !iupStrBoolean(value))
    pixmap = static_cast<QPixmap*>(iupImageGetImage(image, ih, 0, nullptr));
  else
    pixmap = static_cast<QPixmap*>(iupImageGetImage(impress, ih, 0, nullptr));

  if (pixmap)
  {
    iupqmlSetProperty(item, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
    iupqmlSetProperty(item, "icon.width", pixmap->width());
    iupqmlSetProperty(item, "icon.height", pixmap->height());
  }
  else
    iupqmlSetProperty(item, "icon.source", QUrl());
}

static void qmlMenuItemTriggered(Ihandle* ih)
{
  auto* item = reinterpret_cast<QObject*>(ih->handle);

  if (item->property("checkable").toBool() && !iupAttribGetBoolean(ih, "AUTOTOGGLE") && !iupAttribGetBoolean(ih->parent, "RADIO"))
    item->setProperty("checked", !item->property("checked").toBool());

  if (!item->property("checkable").toBool() && iupAttribGetBoolean(ih, "AUTOTOGGLE"))
  {
    if (iupAttribGetBoolean(ih, "VALUE"))
      iupAttribSet(ih, "VALUE", "OFF");
    else
      iupAttribSet(ih, "VALUE", "ON");

    qmlMenuItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
  }

  if (iupAttribGetBoolean(ih->parent, "RADIO"))
  {
    for (Ihandle* c = ih->parent->firstchild; c; c = c->brother)
    {
      if (c != ih && c->handle && iupStrEqual(c->iclass->name, "menuitem"))
        (reinterpret_cast<QObject*>(c->handle))->setProperty("checked", false);
    }
    item->setProperty("checked", true);
  }

  auto cb = static_cast<Icallback>(IupGetCallback(ih, "ACTION"));
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();
}

/****************************************************************************
 * Menu Map/UnMap
 ****************************************************************************/

static void qmlMenuApplyFont(Ihandle* ih, QObject* obj)
{
  QFont* font = iupqmlGetIhFont(ih);
  if (font && obj)
    obj->setProperty("font", QVariant::fromValue(*font));
}

static int qmlMenuMapMethod(Ihandle* ih)
{
  if (iupMenuIsMenuBar(ih))
  {
    Ihandle* dialog = IupGetDialog(ih);
    QQuickWindow* window = dialog ? reinterpret_cast<QQuickWindow*>(dialog->handle) : nullptr;

    QQuickItem* menubar = iupqmlCreateItem(IUPQML_IMPORTS "MenuBar { }");
    if (!menubar)
      return IUP_ERROR;

    ih->handle = reinterpret_cast<InativeHandle*>(menubar);
    qmlMenuApplyFont(ih, menubar);

    if (window)
    {
      menubar->setParentItem(window->contentItem());
      menubar->setPosition(QPointF(0, 0));
    }
  }
  else if (ih->parent && ih->parent->handle && iupStrEqual(ih->parent->iclass->name, "submenu"))
  {
    ih->handle = ih->parent->handle;
    iupAttribSet(ih, "_IUPQML_SHARED_MENU", "1");
  }
  else
  {
    QObject* menu = iupqmlCreateObject(IUPQML_IMPORTS "Menu { popupType: Popup.Window }");
    if (!menu)
      return IUP_ERROR;

    ih->handle = reinterpret_cast<InativeHandle*>(menu);
    qmlMenuApplyFont(ih, menu);

    iupqmlConnect(menu, "aboutToShow()", [ih, menu](void**) { iupqmlMnemonicUpdate(menu); qmlMenuAboutToShow(ih); });
    iupqmlConnect(menu, "aboutToHide()", [ih](void**) { qmlMenuAboutToHide(ih); });
  }

  ih->serial = iupMenuGetChildId(ih);

  return IUP_NOERROR;
}

static void qmlMenuUnMapMethod(Ihandle* ih)
{
  int recent = iupAttribGetInt(ih, "_IUP_RECENT_COUNT");
  for (int i = 0; i < recent; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUPQML_RECENT_ITEM%d", i);
    iupAttribSet(ih, attr_name, nullptr);
  }
  if (recent)
    iupAttribSetInt(ih, "_IUP_RECENT_COUNT", 0);

  if (iupMenuIsMenuBar(ih))
    ih->parent = nullptr;

  if (iupAttribGet(ih, "_IUPQML_SHARED_MENU"))
  {
    iupAttribSet(ih, "_IUPQML_SHARED_MENU", nullptr);
    ih->handle = nullptr;
    return;
  }

  auto* obj = reinterpret_cast<QObject*>(ih->handle);
  if (obj)
  {
    QQuickItem* item = qobject_cast<QQuickItem*>(obj);
    if (item)
      item->setParentItem(nullptr);
    obj->deleteLater();
  }
  ih->handle = nullptr;
}

extern "C" IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = qmlMenuMapMethod;
  ic->UnMap = qmlMenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, nullptr, nullptr, IUPAF_DEFAULT);
}

/****************************************************************************
 * Menu Item
 ****************************************************************************/

static char* qmlMenuItemGetActiveAttrib(Ihandle* ih)
{
  auto* item = reinterpret_cast<QObject*>(ih->handle);
  if (!item)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean(item->property("enabled").toBool());
}

static int qmlMenuItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  qmlMenuItemUpdateImage(ih, nullptr, value, nullptr);
  return 1;
}

static int qmlMenuItemSetImageAttrib(Ihandle* ih, const char* value)
{
  qmlMenuItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), value, iupAttribGet(ih, "IMPRESS"));
  return 1;
}

static int qmlMenuItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  qmlMenuItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), value);
  return 1;
}

static void qmlMenuItemSetAccel(Ihandle* ih, const char* title)
{
  auto* shortcut = reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_SHORTCUT"));
  int code = iupMenuGetAccel(title);
  unsigned int keyval, state;
  Ihandle* dialog = IupGetDialog(ih);

  if (!code || !dialog || !dialog->handle)
  {
    if (shortcut)
      shortcut->setProperty("enabled", false);
    return;
  }

  iupdrvKeyEncode(code, &keyval, &state);

  if (!shortcut)
  {
    shortcut = iupqmlCreateObject("import QtQuick\nShortcut { context: Qt.WindowShortcut }");
    if (!shortcut)
      return;

    Ihandle* menubar = ih;
    while (menubar->parent && !iupMenuIsMenuBar(menubar))
      menubar = menubar->parent;
    if (menubar->handle)
      shortcut->setParent(reinterpret_cast<QObject*>(menubar->handle));

    iupAttribSet(ih, "_IUPQML_SHORTCUT", reinterpret_cast<char*>(shortcut));
    iupqmlConnect(shortcut, "activated()", [ih](void**) {
      if (!iupObjectCheck(ih) || !iupdrvIsActive(ih))
        return;
      auto* item = reinterpret_cast<QObject*>(ih->handle);
      if (item && item->property("checkable").toBool())
        iupqmlCallMethod(item, "toggle");
      qmlMenuItemTriggered(ih);
    });
  }

  shortcut->setProperty("sequence", QVariant::fromValue(QKeySequence(static_cast<int>(keyval | state))));
  shortcut->setProperty("enabled", true);
}

static int qmlMenuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* item = reinterpret_cast<QObject*>(ih->handle);
  char* str;

  if (!value)
  {
    str = const_cast<char*>("     ");
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  item->setProperty("text", QString::fromUtf8(str));
  iupqmlMnemonicUpdate(item);
  qmlMenuItemSetAccel(ih, str);

  auto* wrapper = reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_MENUBAR_WRAPPER"));
  if (wrapper)
    wrapper->setProperty("title", QString::fromUtf8(str));

  if (str != value)
    free(str);

  return 1;
}

static int qmlMenuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  auto* item = reinterpret_cast<QObject*>(ih->handle);

  if (item->property("checkable").toBool())
  {
    if (iupAttribGetBoolean(ih->parent, "RADIO"))
    {
      for (Ihandle* c = ih->parent->firstchild; c; c = c->brother)
      {
        if (c != ih && c->handle && iupStrEqual(c->iclass->name, "menuitem"))
          (reinterpret_cast<QObject*>(c->handle))->setProperty("checked", false);
      }
      value = "ON";
    }

    item->setProperty("checked", iupStrBoolean(value) ? true : false);
    return 0;
  }
  else
  {
    qmlMenuItemUpdateImage(ih, value, iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
    return 1;
  }
}

static char* qmlMenuItemGetValueAttrib(Ihandle* ih)
{
  auto* item = reinterpret_cast<QObject*>(ih->handle);

  if (item->property("checkable").toBool())
    return iupStrReturnChecked(item->property("checked").toBool());
  else
    return nullptr;
}

static int qmlMenuItemMapMethod(Ihandle* ih)
{
  if (!ih->parent || !ih->parent->handle)
    return IUP_ERROR;

  if (iupMenuIsMenuBar(ih->parent) && iupqmlMenuBarIsNative())
  {
    QObject* wrapper = iupqmlCreateObject(IUPQML_IMPORTS "Menu { }");
    QQuickItem* item = iupqmlCreateItem(IUPQML_IMPORTS "MenuItem { icon.color: \"transparent\" }");
    if (!wrapper || !item)
      return IUP_ERROR;

    ih->handle = reinterpret_cast<InativeHandle*>(item);
    ih->serial = iupMenuGetChildId(ih);
    iupAttribSet(ih, "_IUPQML_MENUBAR_WRAPPER", reinterpret_cast<char*>(wrapper));
    qmlMenuApplyFont(ih, wrapper);

    iupqmlConnect(item, "triggered()", [ih](void**) {
      qmlMenuItemTriggered(ih);
    });

    const char* title = iupAttribGet(ih, "TITLE");
    if (title)
    {
      char* str = iupMenuProcessTitle(ih, title);
      wrapper->setProperty("title", QString::fromUtf8(str));
      if (str != title)
        free(str);
    }

    qmlMenuInvokeAt(wrapper, "insertItem", 0, "QQuickItem*", item);
    qmlMenuInvokeAt(qmlMenuGetParentObject(ih), "insertMenu", qmlMenuGetPos(ih), "QQuickMenu*", wrapper);
    iupUpdateFontAttrib(ih);
    return IUP_NOERROR;
  }

  if (iupMenuIsMenuBar(ih->parent))
  {
    QQuickItem* bar_item = iupqmlCreateItem(IUPQML_IMPORTS "MenuBarItem { }");
    if (!bar_item)
      return IUP_ERROR;

    ih->handle = reinterpret_cast<InativeHandle*>(bar_item);
    ih->serial = iupMenuGetChildId(ih);
    iupAttribSet(ih, "_IUPQML_MENUBAR_ITEM", "1");

    iupqmlConnect(bar_item, "triggered()", [ih](void**) {
      qmlMenuItemTriggered(ih);
    });

    qmlMenuAttachItem(ih, bar_item);
    iupUpdateFontAttrib(ih);
    return IUP_NOERROR;
  }

  QQuickItem* item = iupqmlCreateItem(IUPQML_IMPORTS "MenuItem { icon.color: \"transparent\" }");
  if (!item)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(item);
  ih->serial = iupMenuGetChildId(ih);

  bool is_radio = iupAttribGetBoolean(ih->parent, "RADIO");
  bool has_image = (iupAttribGet(ih, "IMAGE") != nullptr || iupAttribGet(ih, "TITLEIMAGE") != nullptr);

  if (is_radio)
    item->setProperty("checkable", true);
  else if (!has_image)
  {
    const char* hidemark = iupAttribGetStr(ih, "HIDEMARK");
    if (!hidemark && !iupAttribGet(ih, "VALUE"))
      hidemark = "YES";

    if (!iupStrBoolean(hidemark))
      item->setProperty("checkable", true);
  }

  iupqmlConnect(item, "hoveredChanged()", [ih, item](void**) {
    if (item->property("hovered").toBool())
      qmlMenuItemHighlight(ih);
  });

  iupqmlConnect(item, "triggered()", [ih](void**) {
    qmlMenuItemTriggered(ih);
  });

  qmlMenuAttachItem(ih, item);

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

static void qmlMenuItemUnMapMethod(Ihandle* ih)
{
  auto* shortcut = reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_SHORTCUT"));
  if (shortcut)
  {
    shortcut->deleteLater();
    iupAttribSet(ih, "_IUPQML_SHORTCUT", nullptr);
  }

  auto* item = reinterpret_cast<QObject*>(ih->handle);
  auto* wrapper = reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_MENUBAR_WRAPPER"));
  if (wrapper)
  {
    QObject* parent = qmlMenuGetParentObject(ih);
    if (parent)
      qmlMenuInvoke(parent, "removeMenu", "QQuickMenu*", wrapper);
    wrapper->deleteLater();
    iupAttribSet(ih, "_IUPQML_MENUBAR_WRAPPER", nullptr);
  }
  else if (item)
    qmlMenuDetachItem(ih, item);
  if (item)
    item->deleteLater();
  iupAttribSet(ih, "_IUPQML_MENUBAR_ITEM", nullptr);
  ih->handle = nullptr;
}

extern "C" IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = qmlMenuItemMapMethod;
  ic->UnMap = qmlMenuItemUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", qmlMenuItemGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", qmlMenuItemGetValueAttrib, qmlMenuItemSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", nullptr, qmlMenuItemSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, qmlMenuItemSetTitleImageAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qmlMenuItemSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, qmlMenuItemSetImpressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "AUTOTOGGLE", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
}

/****************************************************************************
 * Submenu
 ****************************************************************************/

static char* qmlSubmenuGetActiveAttrib(Ihandle* ih)
{
  auto* menu = reinterpret_cast<QObject*>(ih->handle);
  if (!menu)
    return iupBaseGetActiveAttrib(ih);

  return iupStrReturnBoolean(menu->property("enabled").toBool());
}

static QQuickItem* qmlSubmenuGetItem(Ihandle* ih)
{
  QObject* parent = qmlMenuGetParentObject(ih);
  QQuickItem* item = nullptr;
  if (!parent || !QMetaObject::invokeMethod(parent, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, item), Q_ARG(int, qmlMenuGetPos(ih))))
    return nullptr;

  const char* menu_property = iupMenuIsMenuBar(ih->parent) ? "menu" : "subMenu";
  if (!item || qvariant_cast<QObject*>(item->property(menu_property)) != reinterpret_cast<QObject*>(ih->handle))
    return nullptr;
  return item;
}

static int qmlSubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  QQuickItem* item = qmlSubmenuGetItem(ih);
  if (!item)
    return 1;

  auto* pixmap = value ? static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr)) : nullptr;
  if (pixmap)
  {
    iupqmlSetProperty(item, "icon.color", QColor(Qt::transparent));
    iupqmlSetProperty(item, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
  }
  else
    iupqmlSetProperty(item, "icon.source", QUrl());

  return 1;
}

static int qmlSubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* menu = reinterpret_cast<QObject*>(ih->handle);
  char* str;

  if (!value)
  {
    str = const_cast<char*>("     ");
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  menu->setProperty("title", QString::fromUtf8(str));
  if (ih->parent && ih->parent->handle)
    iupqmlMnemonicUpdate(reinterpret_cast<QObject*>(ih->parent->handle));

  if (str != value)
    free(str);

  return 1;
}

static int qmlSubmenuMapMethod(Ihandle* ih)
{
  if (!ih->parent || !ih->parent->handle)
    return IUP_ERROR;

  QObject* menu = iupqmlCreateObject(IUPQML_IMPORTS "Menu { popupType: Popup.Window }");
  if (!menu)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(menu);
  ih->serial = iupMenuGetChildId(ih);
  qmlMenuApplyFont(ih, menu);

  char* title = iupAttribGet(ih, "TITLE");
  if (title)
  {
    char* str = iupMenuProcessTitle(ih, title);
    menu->setProperty("title", QString::fromUtf8(str));
    if (str != title)
      free(str);
  }

  iupqmlConnect(menu, "aboutToShow()", [ih, menu](void**) {
    Ihandle* child = ih->firstchild;
    iupqmlMnemonicUpdate(menu);
    qmlMenuItemHighlight(ih);
    if (child)
      qmlMenuAboutToShow(child);
  });
  iupqmlConnect(menu, "aboutToHide()", [ih](void**) {
    Ihandle* child = ih->firstchild;
    if (child)
      qmlMenuAboutToHide(child);
  });

  {
    QObject* parent = qmlMenuGetParentObject(ih);
    int pos = qmlMenuGetPos(ih);
    qmlMenuInvokeAt(parent, "insertMenu", pos, "QQuickMenu*", menu);
    menu->setParent(parent);
    if (iupMenuIsMenuBar(ih->parent))
      iupqmlUnsetArrowCursors(qobject_cast<QQuickItem*>(parent));
  }

  iupUpdateFontAttrib(ih);

  return IUP_NOERROR;
}

static void qmlSubmenuUnMapMethod(Ihandle* ih)
{
  auto* menu = reinterpret_cast<QObject*>(ih->handle);
  if (menu)
  {
    QObject* parent = qmlMenuGetParentObject(ih);
    if (parent)
      qmlMenuInvoke(parent, "removeMenu", "QQuickMenu*", menu);
    menu->deleteLater();
  }
  ih->handle = nullptr;
}

extern "C" IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = qmlSubmenuMapMethod;
  ic->UnMap = qmlSubmenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", qmlSubmenuGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", nullptr, qmlSubmenuSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qmlSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", nullptr, qmlSubmenuSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}

/****************************************************************************
 * Separator
 ****************************************************************************/

static int qmlMenuSeparatorMapMethod(Ihandle* ih)
{
  if (!ih->parent || !ih->parent->handle)
    return IUP_ERROR;

  if (iupMenuIsMenuBar(ih->parent))
  {
    ih->handle = ih->parent->handle;
    ih->serial = iupMenuGetChildId(ih);
    iupAttribSet(ih, "_IUPQML_SHARED_MENU", "1");
    return IUP_NOERROR;
  }

  QQuickItem* item = iupqmlCreateItem(IUPQML_IMPORTS "MenuSeparator { }");
  if (!item)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(item);
  ih->serial = iupMenuGetChildId(ih);

  qmlMenuAttachItem(ih, item);

  return IUP_NOERROR;
}

static void qmlMenuSeparatorUnMapMethod(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPQML_SHARED_MENU"))
  {
    iupAttribSet(ih, "_IUPQML_SHARED_MENU", nullptr);
    ih->handle = nullptr;
    return;
  }

  auto* item = reinterpret_cast<QObject*>(ih->handle);
  if (item)
  {
    qmlMenuDetachItem(ih, item);
    item->deleteLater();
  }
  ih->handle = nullptr;
}

extern "C" IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = qmlMenuSeparatorMapMethod;
  ic->UnMap = qmlMenuSeparatorUnMapMethod;
}

/****************************************************************************
 * Recent Menu Support
 ****************************************************************************/

static void qmlRecentItemTriggered(Ihandle* menu, int index)
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
  QObject* qmenu;
  int max_recent, existing, i;

  if (!menu || !menu->handle)
    return -1;

  qmenu = reinterpret_cast<QObject*>(menu->handle);
  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", reinterpret_cast<char*>(recent_cb));

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    QString title = QString::fromUtf8(filenames[i]);

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);

    snprintf(attr_name, sizeof(attr_name), "_IUPQML_RECENT_ITEM%d", i);
    auto* item = reinterpret_cast<QObject*>(iupAttribGet(menu, attr_name));

    if (i < existing && item)
      item->setProperty("text", title);
    else
    {
      QQuickItem* new_item = iupqmlCreateItem(IUPQML_IMPORTS "MenuItem { }");
      if (!new_item)
        continue;
      new_item->setProperty("text", title);
      iupqmlConnect(new_item, "triggered()", [menu, i](void**) {
        qmlRecentItemTriggered(menu, i);
      });
      qmlMenuInvoke(qmenu, "addItem", "QQuickItem*", new_item);
      iupAttribSet(menu, attr_name, reinterpret_cast<char*>(new_item));
    }
  }

  for (i = count; i < existing; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUPQML_RECENT_ITEM%d", i);
    auto* item = reinterpret_cast<QObject*>(iupAttribGet(menu, attr_name));
    if (item)
    {
      qmlMenuInvoke(qmenu, "removeItem", "QQuickItem*", item);
      item->deleteLater();
      iupAttribSet(menu, attr_name, nullptr);
    }
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, nullptr);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
  return 0;
}
