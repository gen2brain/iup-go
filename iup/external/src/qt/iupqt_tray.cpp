/** \file
 * \brief Qt System Tray Driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QWidget>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_tray.h"
}

#include "iupqt_drv.h"


/* System tray icon data stored per tray control */
struct IupQtTrayData
{
  QSystemTrayIcon* tray_icon;
  Ihandle* ih;
  QMetaObject::Connection menu_connection;
};

static void qtTrayConnectSignals(IupQtTrayData* tray_data, Ihandle* ih)
{
  QObject::connect(tray_data->tray_icon, &QSystemTrayIcon::activated,
    [ih](QSystemTrayIcon::ActivationReason reason) {
      IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
      if (cb)
      {
        int button = 0;
        int pressed = 1;
        int dclick = 0;

        switch (reason)
        {
          case QSystemTrayIcon::Trigger:      /* Left click */
            button = 1;
            break;
          case QSystemTrayIcon::DoubleClick:  /* Double click */
            button = 1;
            dclick = 1;
            break;
          case QSystemTrayIcon::Context:      /* Right click */
            button = 3;
            break;
          case QSystemTrayIcon::MiddleClick:  /* Middle click */
            button = 2;
            break;
          default:
            return;
        }

        if (cb(ih, button, pressed, dclick) == IUP_CLOSE)
          IupExitLoop();
      }
    });
}

static void qtTrayApplyImage(IupQtTrayData* tray_data, Ihandle* ih)
{
  const char* value = iupAttribGet(ih, "_IUPQT_TRAY_IMAGE");
  if (!value)
    return;

  char* name = iupAttribGet(ih, value);
  if (!name)
    name = (char*)value;

  void* image_handle = IupGetHandle(name);
  if (image_handle)
  {
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, ih, 0, NULL);
    if (pixmap)
    {
      QIcon icon(*pixmap);
      tray_data->tray_icon->setIcon(icon);
      return;
    }
  }

  QPixmap pixmap(QString::fromUtf8(value));
  if (!pixmap.isNull())
  {
    QIcon icon(pixmap);
    tray_data->tray_icon->setIcon(icon);
  }
}

static void qtTrayApplyTip(IupQtTrayData* tray_data, Ihandle* ih)
{
  const char* value = iupAttribGet(ih, "_IUPQT_TRAY_TIP");
  if (value)
    tray_data->tray_icon->setToolTip(QString::fromUtf8(value));
}

#if defined(__unix__) || defined(__linux__)
static void qtTrayResetPlatformMenu(QMenu* menu)
{
  if (!menu)
    return;

  /* Reset the platform menu so Qt creates a fresh one with new IDs.
   * This is needed on Unix/Linux where Qt uses DBus for tray menus. */
  menu->setPlatformMenu(nullptr);

  /* Recursively reset submenus */
  const auto actions = menu->actions();
  for (QAction* action : actions)
  {
    if (action->menu())
      qtTrayResetPlatformMenu(action->menu());
  }
}
#endif

static void qtTrayApplyMenu(IupQtTrayData* tray_data, Ihandle* ih)
{
  Ihandle* menu = (Ihandle*)iupAttribGet(ih, "_IUPQT_TRAY_MENU");
  if (!menu || !iupObjectCheck(menu))
    return;

  if (!menu->handle)
  {
    if (IupMap(menu) == IUP_ERROR)
      return;
  }

  QMenu* qt_menu = (QMenu*)menu->handle;

#if defined(__unix__) || defined(__linux__)
  /* Reset platform menu to force Qt to create fresh DBus menu with new IDs */
  qtTrayResetPlatformMenu(qt_menu);
#endif

  tray_data->tray_icon->setContextMenu(qt_menu);

  tray_data->menu_connection = QObject::connect(qt_menu, &QMenu::aboutToShow,
    [ih]() {
      IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
      if (cb)
      {
        int ret = cb(ih, 3, 1, 0);
        if (ret == IUP_CLOSE)
          IupExitLoop();
      }
    });
}

static void qtTrayDestroyIcon(IupQtTrayData* tray_data)
{
  if (!tray_data)
    return;

  if (tray_data->menu_connection)
  {
    QObject::disconnect(tray_data->menu_connection);
    tray_data->menu_connection = QMetaObject::Connection();
  }

  if (tray_data->tray_icon)
  {
    tray_data->tray_icon->setContextMenu(nullptr);
    tray_data->tray_icon->hide();
    delete tray_data->tray_icon;
    tray_data->tray_icon = nullptr;
  }
}

static void qtTrayCreateIcon(IupQtTrayData* tray_data, Ihandle* ih)
{
  tray_data->tray_icon = new QSystemTrayIcon();
  qtTrayConnectSignals(tray_data, ih);
  qtTrayApplyImage(tray_data, ih);
  qtTrayApplyTip(tray_data, ih);
  qtTrayApplyMenu(tray_data, ih);
}

static IupQtTrayData* qtTrayGetData(Ihandle* ih, int create)
{
  IupQtTrayData* tray_data = (IupQtTrayData*)iupAttribGet(ih, "_IUPQT_TRAY_DATA");

  if (!tray_data && create)
  {
    tray_data = new IupQtTrayData();
    tray_data->tray_icon = nullptr;
    tray_data->ih = ih;
    tray_data->menu_connection = QMetaObject::Connection();
    iupAttribSet(ih, "_IUPQT_TRAY_DATA", (char*)tray_data);
  }

  return tray_data;
}

/******************************************************************************/
/* Driver Interface Implementation                                            */
/******************************************************************************/

extern "C" int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data)
    return 0;

#if defined(__unix__) || defined(__linux__)
  /* On Unix/Linux, Qt uses DBus for tray icons. When visibility is toggled,
   * the DBus connection state gets out of sync with the menu IDs. */
  if (visible)
  {
    qtTrayDestroyIcon(tray_data);
    qtTrayCreateIcon(tray_data, ih);

    if (!tray_data->tray_icon->icon().isNull())
      tray_data->tray_icon->setVisible(true);
    else
      iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", "1");
  }
  else
  {
    qtTrayDestroyIcon(tray_data);
    iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
  }
#else
  /* On Windows and macOS, native APIs handle visibility correctly */
  if (!tray_data->tray_icon)
    qtTrayCreateIcon(tray_data, ih);

  if (visible)
  {
    if (!tray_data->tray_icon->icon().isNull())
      tray_data->tray_icon->setVisible(true);
    else
      iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", "1");
  }
  else
  {
    tray_data->tray_icon->setVisible(false);
    iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
  }
#endif

  return 1;
}

extern "C" int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data)
    return 0;

  /* Store for later use when icon is created/recreated */
  iupAttribSetStr(ih, "_IUPQT_TRAY_TIP", value);

  /* Apply immediately if icon exists */
  if (tray_data->tray_icon)
  {
    if (value)
      tray_data->tray_icon->setToolTip(QString::fromUtf8(value));
    else
      tray_data->tray_icon->setToolTip(QString());
  }

  return 1;
}

extern "C" int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data)
    return 0;

  /* Store for later use when icon is created/recreated */
  iupAttribSetStr(ih, "_IUPQT_TRAY_IMAGE", value);

  /* Apply immediately if icon exists */
  if (tray_data->tray_icon && value)
  {
    char* name = iupAttribGet(ih, value);
    if (!name)
      name = (char*)value;

    void* image_handle = IupGetHandle(name);
    if (image_handle)
    {
      QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, ih, 0, NULL);
      if (pixmap)
      {
        QIcon icon(*pixmap);
        tray_data->tray_icon->setIcon(icon);

        if (iupAttribGet(ih, "_IUPQT_TRAY_SHOW_PENDING"))
        {
          tray_data->tray_icon->show();
          iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
        }

        return 1;
      }
    }

    QPixmap pixmap(QString::fromUtf8(value));
    if (!pixmap.isNull())
    {
      QIcon icon(pixmap);
      tray_data->tray_icon->setIcon(icon);

      if (iupAttribGet(ih, "_IUPQT_TRAY_SHOW_PENDING"))
      {
        tray_data->tray_icon->show();
        iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
      }

      return 1;
    }
  }

  return value ? 0 : 1;
}

extern "C" int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data)
    return 0;

  /* Store for later use when icon is created/recreated */
  iupAttribSet(ih, "_IUPQT_TRAY_MENU", (char*)menu);

  /* Apply immediately if icon exists */
  if (tray_data->tray_icon)
  {
    if (tray_data->menu_connection)
    {
      QObject::disconnect(tray_data->menu_connection);
      tray_data->menu_connection = QMetaObject::Connection();
    }

    if (menu && iupObjectCheck(menu))
    {
      if (!menu->handle)
      {
        if (IupMap(menu) == IUP_ERROR)
          return 0;
      }

      QMenu* qt_menu = (QMenu*)menu->handle;
      tray_data->tray_icon->setContextMenu(qt_menu);

      tray_data->menu_connection = QObject::connect(qt_menu, &QMenu::aboutToShow,
        [ih]() {
          IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
          if (cb)
          {
            int ret = cb(ih, 3, 1, 0);
            if (ret == IUP_CLOSE)
              IupExitLoop();
          }
        });
    }
    else
    {
      tray_data->tray_icon->setContextMenu(nullptr);
    }
  }

  return 1;
}

extern "C" void iupdrvTrayDestroy(Ihandle* ih)
{
  IupQtTrayData* tray_data = (IupQtTrayData*)iupAttribGet(ih, "_IUPQT_TRAY_DATA");

  if (tray_data)
  {
    qtTrayDestroyIcon(tray_data);
    delete tray_data;
    iupAttribSet(ih, "_IUPQT_TRAY_DATA", nullptr);
  }
}

extern "C" int iupdrvTrayIsAvailable(void)
{
  return QSystemTrayIcon::isSystemTrayAvailable() ? 1 : 0;
}

extern "C" void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "BALLOON", NULL, NULL, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BALLOONINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BALLOONTIMEOUT", NULL, NULL, IUPAF_SAMEASSYSTEM, "10000", IUPAF_NO_INHERIT);
}
