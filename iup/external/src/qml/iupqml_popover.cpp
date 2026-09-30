/** \file
 * \brief IupPopover control for Qt Quick
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickWindow>
#include <QPointF>
#include <QVariant>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_popover.h"
}

#include "iupqml_drv.h"


#define IUPQML_POPUP_NOAUTOCLOSE 0x00
#define IUPQML_POPUP_CLOSEONESCAPE 0x01
#define IUPQML_POPUP_CLOSEONPRESSOUTSIDE 0x02

static QObject* qmlPopoverGetPopup(Ihandle* ih)
{
  return reinterpret_cast<QObject*>(iupAttribGet(ih, "_IUPQML_POPOVER"));
}

static void qmlPopoverUpdatePolicy(Ihandle* ih, QObject* popup)
{
  int policy = IUPQML_POPUP_NOAUTOCLOSE;
  if (iupAttribGetBoolean(ih, "AUTOHIDE"))
    policy = IUPQML_POPUP_CLOSEONESCAPE | IUPQML_POPUP_CLOSEONPRESSOUTSIDE;
  popup->setProperty("closePolicy", policy);
}

static int qmlPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  QObject* popup;

  if (iupStrBoolean(value))
  {
    auto* anchor = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUP_POPOVER_ANCHOR"));
    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popup = qmlPopoverGetPopup(ih);
    QQuickItem* anchor_item = iupqmlGetItem(anchor);
    if (!popup || !anchor_item)
      return 0;

    QQuickWindow* window = anchor_item->window();
    if (!window)
      return 0;

    popup->setProperty("parent", QVariant::fromValue<QObject*>(window->contentItem()));
    qmlPopoverUpdatePolicy(ih, popup);

    if (ih->firstchild && ih->firstchild->handle)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih->firstchild);
    }

    int padding = popup->property("padding").toInt();
    int total_w = ih->currentwidth + 2 * padding;
    int total_h = ih->currentheight + 2 * padding;

    QPointF anchor_pos = anchor_item->mapToGlobal(QPointF(0, 0));
    int x, y;
    iupPopoverCalcPosition(ih,
      static_cast<int>(anchor_pos.x()), static_cast<int>(anchor_pos.y()),
      static_cast<int>(anchor_item->width()), static_cast<int>(anchor_item->height()),
      total_w, total_h,
      &x, &y);

    QPoint local = window->mapFromGlobal(QPoint(x, y));

    popup->setProperty("x", local.x());
    popup->setProperty("y", local.y());
    popup->setProperty("width", total_w);
    popup->setProperty("height", total_h);

    iupAttribSet(ih, "_IUPQML_POPOVER_SHOWN", "1");
    iupqmlCallMethod(popup, "open");
  }
  else
  {
    popup = qmlPopoverGetPopup(ih);
    iupAttribSet(ih, "_IUPQML_POPOVER_SHOWN", nullptr);
    if (popup)
      iupqmlCallMethod(popup, "close");
  }

  return 0;
}

static char* qmlPopoverGetVisibleAttrib(Ihandle* ih)
{
  QObject* popup = qmlPopoverGetPopup(ih);
  if (!popup)
    return const_cast<char*>("NO");

  return iupStrReturnBoolean(iupAttribGet(ih, "_IUPQML_POPOVER_SHOWN") != nullptr && popup->property("visible").toBool());
}

static void qmlPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static int qmlPopoverMapMethod(Ihandle* ih)
{
  QObject* popup = iupqmlCreateObject(IUPQML_IMPORTS "Popup { padding: 1; modal: false; focus: true; popupType: Popup.Window; contentItem: Item { } }");
  if (!popup)
    return IUP_ERROR;

  QQuickItem* content = iupqmlGetItemProperty(popup, "contentItem");
  if (!content)
  {
    delete popup;
    return IUP_ERROR;
  }

  ih->handle = reinterpret_cast<InativeHandle*>(content);
  iupAttribSet(ih, "_IUPQML_POPOVER", reinterpret_cast<char*>(popup));
  iupqmlSetIhandle(popup, ih);
  iupqmlSetIhandle(content, ih);

  qmlPopoverUpdatePolicy(ih, popup);

  iupqmlConnect(popup, "opened()", [ih](void**) {
    if (!iupObjectCheck(ih))
      return;
    IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOW_CB"));
    if (show_cb)
      show_cb(ih, IUP_SHOW);
  });

  iupqmlConnect(popup, "closed()", [ih](void**) {
    if (!iupObjectCheck(ih) || iupAttribGet(ih, "_IUPQML_POPOVER_UNMAP"))
      return;
    iupAttribSet(ih, "_IUPQML_POPOVER_SHOWN", nullptr);
    IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "SHOW_CB"));
    if (show_cb)
      show_cb(ih, IUP_HIDE);
  });

  return IUP_NOERROR;
}

static void qmlPopoverUnMapMethod(Ihandle* ih)
{
  QObject* popup = qmlPopoverGetPopup(ih);
  if (popup)
  {
    iupAttribSet(ih, "_IUPQML_POPOVER_UNMAP", "1");
    iupqmlCallMethod(popup, "close");
    popup->deleteLater();
    iupAttribSet(ih, "_IUPQML_POPOVER", nullptr);
    iupAttribSet(ih, "_IUPQML_POPOVER_UNMAP", nullptr);
  }
  iupAttribSet(ih, "_IUPQML_POPOVER_SHOWN", nullptr);

  ih->handle = nullptr;
}

static void qmlPopoverChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (child->handle && ih->handle)
  {
    QQuickItem* child_item = iupqmlGetItem(child);
    if (child_item)
      child_item->setParentItem(reinterpret_cast<QQuickItem*>(ih->handle));
  }
}

extern "C" {

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = qmlPopoverMapMethod;
  ic->UnMap = qmlPopoverUnMapMethod;
  ic->LayoutUpdate = qmlPopoverLayoutUpdateMethod;
  ic->ChildAdded = qmlPopoverChildAddedMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", qmlPopoverGetVisibleAttrib, qmlPopoverSetVisibleAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}

}
