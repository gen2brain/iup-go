/** \file
 * \brief Qt Quick Focus Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickWindow>
#include <QEvent>
#include <QGuiApplication>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iup_drv.h"
}

#include "iupqml_drv.h"


IUP_DRV_API void iupqmlSetCanFocus(QQuickItem* item, int can)
{
  if (!item)
    return;

  item->setActiveFocusOnTab(can ? true : false);
  if (item->metaObject()->indexOfProperty("focusPolicy") < 0)
    return;

  if (!can)
  {
    if (!item->property("_iup_focuspolicy").isValid())
      item->setProperty("_iup_focuspolicy", item->property("focusPolicy"));
    iupqmlSetProperty(item, "focusPolicy", static_cast<int>(Qt::NoFocus));
  }
  else if (item->property("_iup_focuspolicy").isValid())
    iupqmlSetProperty(item, "focusPolicy", item->property("_iup_focuspolicy"));
}

extern "C" IUP_SDK_API void iupdrvSetFocus(Ihandle* ih)
{
  QQuickItem* item = iupqmlCanvasGetItem(ih);

  if (!item && iupAttribGet(ih, "_IUPQML_FOCUS_FUNC"))
    item = (reinterpret_cast<QQuickItem* (*)(Ihandle*)>(iupAttribGet(ih, "_IUPQML_FOCUS_FUNC")))(ih);
  if (!item && ih->iclass->nativetype == IUP_TYPEDIALOG)
    item = iupqmlDialogGetContent(ih);
  if (!item)
    item = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_EVENT_ITEM"));
  if (!item)
    item = reinterpret_cast<QQuickItem*>(ih->handle);

  if (!item)
    return;

  Ihandle* dialog = IupGetDialog(ih);
  if (dialog && dialog->handle)
  {
    auto* window = reinterpret_cast<QQuickWindow*>(dialog->handle);

    if (!window->isActive() && QGuiApplication::platformName() != "wayland")
      window->requestActivate();
  }

  item->forceActiveFocus(dialog && iupAttribGet(dialog, "_IUPQML_KEYBOARD_FOCUS") ? Qt::TabFocusReason : Qt::OtherFocusReason);
}

static QQuickItem* qmlFocusItem(Ihandle* ih)
{
  auto* item = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_EVENT_ITEM"));
  if (!item)
    item = reinterpret_cast<QQuickItem*>(ih->handle);
  return item;
}

IUP_DRV_API int iupqmlFocusChanged(Ihandle* ih, int in)
{
  if (!iupObjectCheck(ih))
    return 1;

  if (in)
  {
    if (!iupdrvIsActive(ih))
      return 1;

    Ihandle* dialog = IupGetDialog(ih);
    if (dialog && ih != dialog)
      iupAttribSet(dialog, "_IUPQML_LASTFOCUS", reinterpret_cast<char*>(ih));

    Ihandle* previous = IupGetFocus();
    if (previous && previous != ih && iupObjectCheck(previous))
    {
      QQuickItem* item = qmlFocusItem(previous);
      if (item && item->isFocusScope() && !item->hasActiveFocus())
        iupCallKillFocusCb(previous);
    }

    iupCallGetFocusCb(ih);
  }
  else
    iupCallKillFocusCb(ih);

  return 0;
}

IUP_DRV_API int iupqmlFocusInOutEvent(QEvent* evt, Ihandle* ih)
{
  return iupqmlFocusChanged(ih, evt->type() == QEvent::FocusIn);
}


IUP_DRV_API void iupqmlDialogSetFocus(Ihandle* ih)
{
  Ihandle* dialog = IupGetDialog(ih);

  if (ih != dialog)
  {
    iupCallGetFocusCb(ih);
  }
  else
  {
    auto* lastfocus = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUPQML_LASTFOCUS"));

    if (iupObjectCheck(lastfocus) && lastfocus == IupGetFocus())
    {
      QQuickItem* item = qmlFocusItem(lastfocus);
      if (item && item->hasActiveFocus())
        return;
    }

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
