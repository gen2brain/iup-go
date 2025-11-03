/** \file
 * \brief System Tray Support - Qt Implementation
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
}

#include "iupqt_drv.h"


/* System tray icon data stored per dialog */
struct IupQtTrayData
{
  QSystemTrayIcon* tray_icon;
  QMenu* context_menu;
  Ihandle* menu_ih;
  Ihandle* dialog_ih;  /* Store dialog handle for callbacks */
};

/****************************************************************************
 * Get or Create Tray Data Structure
 ****************************************************************************/

static IupQtTrayData* qtTrayGetData(Ihandle* ih, int create)
{
  IupQtTrayData* tray_data = (IupQtTrayData*)iupAttribGet(ih, "_IUPQT_TRAY_DATA");

  if (!tray_data && create)
  {
    tray_data = new IupQtTrayData();
    tray_data->tray_icon = new QSystemTrayIcon();
    tray_data->context_menu = nullptr;
    tray_data->menu_ih = nullptr;
    tray_data->dialog_ih = ih;

    /* Connect activation signal (click, double-click, etc.) */
    QObject::connect(tray_data->tray_icon, &QSystemTrayIcon::activated,
      [ih](QSystemTrayIcon::ActivationReason reason) {
        IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
        if (cb)
        {
          int button = 0;
          int pressed = 1;  /* Single click = pressed */
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

          cb(ih, button, pressed, dclick);
        }
      });

    iupAttribSet(ih, "_IUPQT_TRAY_DATA", (char*)tray_data);
  }

  return tray_data;
}

/****************************************************************************
 * Set Tray Icon Visibility
 ****************************************************************************/

extern "C" int iupqtSetTrayAttrib(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data || !tray_data->tray_icon)
    return 0;

  if (iupStrBoolean(value))
  {
    /* Show tray icon - but only if an icon has been set.
     * Qt requires an icon to be set before showing the tray icon.
     * If no icon is set yet, we'll show it when TRAYIMAGE is set.
     */
    if (!tray_data->tray_icon->isVisible())
    {
      if (!tray_data->tray_icon->icon().isNull())
      {
        tray_data->tray_icon->show();
      }
      else
      {
        /* Store that we want to show the tray, will be shown when icon is set */
        iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", "1");
      }
    }
  }
  else
  {
    /* Hide tray icon */
    if (tray_data->tray_icon->isVisible())
    {
      tray_data->tray_icon->hide();
    }
    iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
  }

  return 1;
}

/****************************************************************************
 * Set Tray Tooltip
 ****************************************************************************/

extern "C" int iupqtSetTrayTipAttrib(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data || !tray_data->tray_icon)
    return 0;

  if (value)
    tray_data->tray_icon->setToolTip(QString::fromUtf8(value));
  else
    tray_data->tray_icon->setToolTip(QString());

  return 1;
}

/****************************************************************************
 * Set Tray Icon Image
 ****************************************************************************/

extern "C" int iupqtSetTrayImageAttrib(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data || !tray_data->tray_icon)
    return 0;

  if (value)
  {
    /* Get image handle */
    char* name = iupAttribGet(ih, value);
    if (!name)
      name = (char*)value;

    /* Try to get the image */
    void* image_handle = IupGetHandle(name);
    if (image_handle)
    {
      QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, ih, 0, NULL);
      if (pixmap)
      {
        QIcon icon(*pixmap);
        tray_data->tray_icon->setIcon(icon);

        /* If TRAY=YES was set before the icon, show the tray now */
        if (iupAttribGet(ih, "_IUPQT_TRAY_SHOW_PENDING"))
        {
          tray_data->tray_icon->show();
          iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
        }

        return 1;
      }
    }

    /* Try to load from file */
    QPixmap pixmap(QString::fromUtf8(value));
    if (!pixmap.isNull())
    {
      QIcon icon(pixmap);
      tray_data->tray_icon->setIcon(icon);

      /* If TRAY=YES was set before the icon, show the tray now */
      if (iupAttribGet(ih, "_IUPQT_TRAY_SHOW_PENDING"))
      {
        tray_data->tray_icon->show();
        iupAttribSet(ih, "_IUPQT_TRAY_SHOW_PENDING", NULL);
      }

      return 1;
    }
  }

  return 0;
}

/****************************************************************************
 * Set Tray Context Menu
 ****************************************************************************/

extern "C" int iupqtSetTrayMenuAttrib(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 1);
  if (!tray_data || !tray_data->tray_icon)
    return 0;

  if (!value || !value[0])
  {
    /* Remove menu */
    tray_data->tray_icon->setContextMenu(nullptr);
    tray_data->context_menu = nullptr;
    tray_data->menu_ih = nullptr;
    return 1;
  }

  /* IUPAF_IHANDLENAME means value is a string name, convert it to Ihandle */
  Ihandle* menu_ih = IupGetHandle(value);

  if (menu_ih && iupObjectCheck(menu_ih))
  {
    /* On Linux with SNI protocol, we MUST use setContextMenu() for the menu to work.
     * The activated signal doesn't fire for right-clicks with SNI.
     * The menu must be mapped before we can use it.
     */
    QMenu* menu = (QMenu*)menu_ih->handle;

    if (menu)
    {
      tray_data->tray_icon->setContextMenu(menu);
      tray_data->context_menu = menu;
      tray_data->menu_ih = menu_ih;
      return 1;
    }
    else
    {
      tray_data->menu_ih = menu_ih;
      tray_data->context_menu = nullptr;
    }
  }
  else
  {
    /* Remove menu */
    tray_data->tray_icon->setContextMenu(nullptr);
    tray_data->context_menu = nullptr;
    tray_data->menu_ih = nullptr;
  }

  return 1;
}

extern "C" void iupqtTrayCleanup(Ihandle* ih)
{
  IupQtTrayData* tray_data = (IupQtTrayData*)iupAttribGet(ih, "_IUPQT_TRAY_DATA");

  if (tray_data)
  {
    if (tray_data->tray_icon)
    {
      tray_data->tray_icon->hide();
      delete tray_data->tray_icon;
      tray_data->tray_icon = nullptr;
    }

    /* Note: context_menu is owned by the menu Ihandle, don't delete it here */
    tray_data->context_menu = nullptr;
    tray_data->menu_ih = nullptr;

    delete tray_data;
    iupAttribSet(ih, "_IUPQT_TRAY_DATA", nullptr);
  }
}

/****************************************************************************
 * Show Tray Balloon Notification (Qt 5.9+)
 ****************************************************************************/

extern "C" int iupqtSetTrayBalloonAttrib(Ihandle* ih, const char* value)
{
  IupQtTrayData* tray_data = qtTrayGetData(ih, 0);
  if (!tray_data || !tray_data->tray_icon || !tray_data->tray_icon->isVisible())
    return 0;

  if (!value)
    return 0;

  const char* title = iupAttribGetStr(ih, "TRAYTITLE");
  const char* info = value;

  const char* balloon_info = iupAttribGetStr(ih, "TRAYBALLOONINFO");
  QSystemTrayIcon::MessageIcon icon_type = QSystemTrayIcon::Information;

  if (balloon_info)
  {
    if (iupStrEqualNoCase(balloon_info, "ERROR"))
      icon_type = QSystemTrayIcon::Critical;
    else if (iupStrEqualNoCase(balloon_info, "WARNING"))
      icon_type = QSystemTrayIcon::Warning;
    else if (iupStrEqualNoCase(balloon_info, "INFO"))
      icon_type = QSystemTrayIcon::Information;
  }

  int timeout = iupAttribGetInt(ih, "TRAYBALLOONTIMEOUT");
  if (timeout <= 0)
    timeout = 10000;  /* Default 10 seconds */

  tray_data->tray_icon->showMessage(
    title ? QString::fromUtf8(title) : QString(),
    QString::fromUtf8(info),
    icon_type,
    timeout
  );

  return 1;
}

extern "C" int iupdrvIsSystemTrayAvailable(void)
{
  return QSystemTrayIcon::isSystemTrayAvailable() ? 1 : 0;
}
