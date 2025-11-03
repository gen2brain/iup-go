/** \file
 * \brief Qt Focus Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QWindow>
#include <QEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QGuiApplication>

#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_assert.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Focus Policy Control
 ****************************************************************************/

extern "C" IUP_DRV_API void iupqtSetCanFocus(QWidget* widget, int can)
{
  if (!widget)
    return;

  if (can)
  {
    /* Qt::StrongFocus allows both tab and click focus */
    widget->setFocusPolicy(Qt::StrongFocus);
  }
  else
  {
    widget->setFocusPolicy(Qt::NoFocus);
  }
}

/****************************************************************************
 * Set Focus to Widget
 ****************************************************************************/

extern "C" void iupdrvSetFocus(Ihandle* ih)
{
  QWidget* widget = (QWidget*)ih->handle;

  if (!widget)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (dialog && dialog->handle)
  {
    QWidget* dialog_widget = (QWidget*)dialog->handle;

    if (!dialog_widget->isActiveWindow())
    {
      dialog_widget->activateWindow();
      dialog_widget->raise();

      /* requestActivate() is not supported on Wayland - skip to avoid warning */
      if (QGuiApplication::platformName() != "wayland")
      {
        QWindow* window = dialog_widget->windowHandle();
        if (window)
          window->requestActivate();
      }
    }
  }

  widget->setFocus(Qt::OtherFocusReason);

  /* Immediately update global focus tracking - don't wait for FocusIn event
   * This ensures IupGetFocus() returns the correct value right away */
  iupSetCurrentFocus(ih);
}

/****************************************************************************
 * Focus Event Handler
 ****************************************************************************/

extern "C" int iupqtFocusInOutEvent(QWidget* widget, QEvent* evt, Ihandle* ih)
{
  (void)widget;

  if (!iupObjectCheck(ih))
    return 1; /* TRUE - event handled */

  if (evt->type() == QEvent::FocusIn)
  {
    QFocusEvent* focus_evt = static_cast<QFocusEvent*>(evt);

    /* Even when ACTIVE=NO the widget might get this event */
    if (!iupdrvIsActive(ih))
      return 1;

    iupSetCurrentFocus(ih);

    Ihandle* dialog = IupGetDialog(ih);
    if (dialog && ih != dialog)
      iupAttribSet(dialog, "_IUPQT_LASTFOCUS", (char*)ih);

    iupCallGetFocusCb(ih);

    (void)focus_evt;
  }
  else if (evt->type() == QEvent::FocusOut)
  {
    QFocusEvent* focus_evt = static_cast<QFocusEvent*>(evt);

    iupCallKillFocusCb(ih);

    (void)focus_evt;
  }

  return 0; /* FALSE - allow event to propagate */
}

/****************************************************************************
 * Dialog Focus Management
 ****************************************************************************/

/* Called when dialog gets focus - restore focus to last focused control */
extern "C" void iupqtDialogSetFocus(Ihandle* ih)
{
  Ihandle* dialog = IupGetDialog(ih);

  if (ih != dialog)
  {
    iupCallGetFocusCb(ih);
  }
  else
  {
    /* If a control inside the dialog had focus, restore it */
    Ihandle* lastfocus = (Ihandle*)iupAttribGet(ih, "_IUPQT_LASTFOCUS");

    if (iupObjectCheck(lastfocus))
    {
      iupCallGetFocusCb(ih);

      if (!iupAttribGetBoolean(ih, "IGNORELASTFOCUS"))
        IupSetFocus(lastfocus);

      return;
    }

    iupCallGetFocusCb(ih);
  }
}
