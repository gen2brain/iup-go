/** \file
 * \brief IupPopover control for Qt
 *
 * See Copyright Notice in "iup.h"
 */

#include <QFrame>
#include <QVBoxLayout>
#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QPainter>


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

#include "iupqt_drv.h"


class IupQtPopover : public QFrame
{
public:
  Ihandle* iup_handle;
  QWidget* content_widget;
  bool autohide_enabled;

  IupQtPopover(Ihandle* ih, bool autohide, QWidget* anchor_toplevel)
    : QFrame(autohide ? nullptr : anchor_toplevel,
             autohide ? (Qt::Popup | Qt::FramelessWindowHint)
                      : (Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint))
    , iup_handle(ih)
    , content_widget(nullptr)
    , autohide_enabled(autohide)
  {
    setFrameStyle(QFrame::NoFrame);

    content_widget = new QWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->addWidget(content_widget);
    setLayout(layout);
  }

protected:
  void paintEvent(QPaintEvent* event) override
  {
    QFrame::paintEvent(event);

    QPainter p(this);
    p.setPen(palette().color(QPalette::Mid));
    p.drawRect(0, 0, width() - 1, height() - 1);
  }

  void hideEvent(QHideEvent* event) override
  {
    QFrame::hideEvent(event);

    IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(iup_handle, "SHOW_CB"));
    if (show_cb)
      show_cb(iup_handle, IUP_HIDE);
  }

  void showEvent(QShowEvent* event) override
  {
    QFrame::showEvent(event);

    IFni show_cb = reinterpret_cast<IFni>(IupGetCallback(iup_handle, "SHOW_CB"));
    if (show_cb)
      show_cb(iup_handle, IUP_SHOW);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QFrame::focusOutEvent(event);

    if (autohide_enabled)
    {
      QWidget* focus_widget = QApplication::focusWidget();
      if (!focus_widget || !isAncestorOf(focus_widget))
      {
        hide();
      }
    }
  }

  bool event(QEvent* event) override
  {
    if (event->type() == QEvent::WindowDeactivate)
    {
      if (autohide_enabled)
      {
        hide();
      }
    }
    return QFrame::event(event);
  }
};

static int qtPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  IupQtPopover* popover;

  if (iupStrBoolean(value))
  {
    auto* anchor = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUP_POPOVER_ANCHOR"));
    QWidget* anchor_widget;
    QPoint anchor_pos;
    QSize anchor_size;

    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popover = reinterpret_cast<IupQtPopover*>(ih->handle);
    anchor_widget = reinterpret_cast<QWidget*>(anchor->handle);

    {
      bool autohide = iupAttribGetBoolean(ih, "AUTOHIDE") != 0;
      if (autohide != popover->autohide_enabled)
      {
        QWidget* anchor_toplevel = autohide ? nullptr : anchor_widget->window();
        popover->setParent(anchor_toplevel);
        popover->setWindowFlags(autohide ? (Qt::Popup | Qt::FramelessWindowHint)
                                         : (Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint));
        popover->autohide_enabled = autohide;
      }
    }
    anchor_pos = anchor_widget->mapToGlobal(QPoint(0, 0));
    anchor_size = anchor_widget->size();

    if (ih->firstchild && ih->firstchild->handle)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih->firstchild);
    }

    int x, y;
    iupPopoverCalcPosition(ih,
      anchor_pos.x(), anchor_pos.y(),
      anchor_size.width(), anchor_size.height(),
      ih->currentwidth, ih->currentheight,
      &x, &y);

    popover->resize(ih->currentwidth, ih->currentheight);
    popover->move(x, y);
    popover->show();
  }
  else
  {
    if (ih->handle)
    {
      popover = reinterpret_cast<IupQtPopover*>(ih->handle);
      popover->hide();
    }
  }

  return 0;
}

static char* qtPopoverGetVisibleAttrib(Ihandle* ih)
{
  IupQtPopover* popover;

  if (!ih->handle)
    return const_cast<char*>("NO");

  popover = reinterpret_cast<IupQtPopover*>(ih->handle);
  return iupStrReturnBoolean(popover->isVisible());
}

static void qtPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static int qtPopoverMapMethod(Ihandle* ih)
{
  int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
  QWidget* anchor_toplevel = nullptr;

  if (!autohide)
  {
    auto* anchor = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUP_POPOVER_ANCHOR"));
    if (anchor && anchor->handle)
    {
      auto* anchor_widget = reinterpret_cast<QWidget*>(anchor->handle);
      anchor_toplevel = anchor_widget->window();
    }
  }

  auto* popover = new IupQtPopover(ih, autohide != 0, anchor_toplevel);

  ih->handle = reinterpret_cast<InativeHandle*>(popover);

  return IUP_NOERROR;
}

static void qtPopoverUnMapMethod(Ihandle* ih)
{
  auto* popover = reinterpret_cast<IupQtPopover*>(ih->handle);

  if (popover)
  {
    popover->hide();
    delete popover;
  }

  ih->handle = nullptr;
}

static void qtPopoverChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (child->handle)
  {
    auto* popover = reinterpret_cast<IupQtPopover*>(ih->handle);
    if (popover && popover->content_widget)
    {
      auto* child_widget = reinterpret_cast<QWidget*>(child->handle);
      child_widget->setParent(popover->content_widget);
    }
  }
}

extern "C" {

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = qtPopoverMapMethod;
  ic->UnMap = qtPopoverUnMapMethod;
  ic->LayoutUpdate = qtPopoverLayoutUpdateMethod;
  ic->ChildAdded = qtPopoverChildAddedMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", qtPopoverGetVisibleAttrib, qtPopoverSetVisibleAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}

}
