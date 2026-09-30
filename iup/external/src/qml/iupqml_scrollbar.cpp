/** \file
 * \brief Scrollbar Control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QKeyEvent>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_scrollbar.h"
}

#include "iupqml_drv.h"


extern "C" IUP_SDK_API void iupdrvScrollbarUpdate(Ihandle* ih)
{
  auto* sb = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!sb) return;

  double range = ih->data->vmax - ih->data->vmin;
  if (range <= 0) return;

  double size = ih->data->pagesize / range;
  if (size < 0.0) size = 0.0;
  if (size > 1.0) size = 1.0;

  double position = (ih->data->val - ih->data->vmin) / range;
  if (position < 0.0) position = 0.0;
  if (position > 1.0 - size) position = 1.0 - size;
  if (ih->data->inverted)
    position = 1.0 - size - position;

  iupAttribSet(ih, "_IUPQML_IGNORE_POS", "1");
  sb->setProperty("size", size);
  sb->setProperty("stepSize", ih->data->linestep / range);
  sb->setProperty("position", position);
  iupAttribSet(ih, "_IUPQML_IGNORE_POS", nullptr);
}

static void qmlScrollbarPositionChanged(Ihandle* ih)
{
  auto* sb = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!sb || iupAttribGet(ih, "_IUPQML_IGNORE_POS"))
    return;

  double old_val = ih->data->val;
  double range = ih->data->vmax - ih->data->vmin;
  double position = sb->property("position").toDouble();
  bool pressed = sb->property("pressed").toBool();

  if (ih->data->inverted)
    position = 1.0 - sb->property("size").toDouble() - position;

  ih->data->val = ih->data->vmin + position * range;
  iupScrollbarCropValue(ih);

  int op;
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    op = pressed ? IUP_SBDRAGH : IUP_SBPOSH;
  else
    op = pressed ? IUP_SBDRAGV : IUP_SBPOSV;

  auto scroll_cb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
  if (scroll_cb)
  {
    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      posx = static_cast<float>(ih->data->val);
    else
      posy = static_cast<float>(ih->data->val);

    scroll_cb(ih, op, posx, posy);
  }

  IFn valuechanged_cb = static_cast<IFn>(IupGetCallback(ih, "VALUECHANGED_CB"));
  if (valuechanged_cb && ih->data->val != old_val)
    valuechanged_cb(ih);
}

static void qmlScrollbarStep(Ihandle* ih, double delta, int op)
{
  double old_val = ih->data->val;

  ih->data->val += delta * (ih->data->vmax - ih->data->vmin);
  iupScrollbarCropValue(ih);
  iupdrvScrollbarUpdate(ih);

  auto scroll_cb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
  if (scroll_cb)
  {
    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      posx = static_cast<float>(ih->data->val);
    else
      posy = static_cast<float>(ih->data->val);
    scroll_cb(ih, op, posx, posy);
  }

  IFn valuechanged_cb = static_cast<IFn>(IupGetCallback(ih, "VALUECHANGED_CB"));
  if (valuechanged_cb && ih->data->val != old_val)
    valuechanged_cb(ih);
}

class IupQmlScrollbarFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlScrollbarFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!iupObjectCheck(ih))
      return false;

    int horiz = ih->data->orientation == ISCROLLBAR_HORIZONTAL;

    if (event->type() == QEvent::KeyPress)
    {
      auto* evt = static_cast<QKeyEvent*>(event);
      if (iupqmlKeyPressEvent(qobject_cast<QQuickItem*>(obj), evt, ih))
        return true;

      switch (evt->key())
      {
      case Qt::Key_Up:
        if (horiz) return false;
        qmlScrollbarStep(ih, -ih->data->linestep, IUP_SBUP);
        return true;
      case Qt::Key_Down:
        if (horiz) return false;
        qmlScrollbarStep(ih, ih->data->linestep, IUP_SBDN);
        return true;
      case Qt::Key_Left:
        if (!horiz) return false;
        qmlScrollbarStep(ih, -ih->data->linestep, IUP_SBLEFT);
        return true;
      case Qt::Key_Right:
        if (!horiz) return false;
        qmlScrollbarStep(ih, ih->data->linestep, IUP_SBRIGHT);
        return true;
      case Qt::Key_PageUp:
        qmlScrollbarStep(ih, -ih->data->pagestep, horiz ? IUP_SBPGLEFT : IUP_SBPGUP);
        return true;
      case Qt::Key_PageDown:
        qmlScrollbarStep(ih, ih->data->pagestep, horiz ? IUP_SBPGRIGHT : IUP_SBPGDN);
        return true;
      case Qt::Key_Home:
        qmlScrollbarStep(ih, -1.0, horiz ? IUP_SBPOSH : IUP_SBPOSV);
        return true;
      case Qt::Key_End:
        qmlScrollbarStep(ih, 1.0, horiz ? IUP_SBPOSH : IUP_SBPOSV);
        return true;
      default:
        return false;
      }
    }

    if (event->type() == QEvent::Wheel)
    {
      auto* evt = static_cast<QWheelEvent*>(event);
      int notches = evt->angleDelta().y() / 120;
      if (notches == 0)
        notches = evt->angleDelta().x() / 120;
      if (notches == 0)
        return false;
      if (notches > 0)
        qmlScrollbarStep(ih, -notches * ih->data->linestep, horiz ? IUP_SBLEFT : IUP_SBUP);
      else
        qmlScrollbarStep(ih, -notches * ih->data->linestep, horiz ? IUP_SBRIGHT : IUP_SBDN);
      evt->accept();
      return true;
    }

    return false;
  }
};

static int qmlScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    iupdrvScrollbarUpdate(ih);
  }
  return 0;
}

static int qmlScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    iupdrvScrollbarUpdate(ih);
  return 0;
}

static int qmlScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    iupdrvScrollbarUpdate(ih);
  return 0;
}

static int qmlScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    iupdrvScrollbarUpdate(ih);
  }
  return 0;
}

extern "C" IUP_SDK_API void iupdrvScrollbarGetMinSize(Ihandle* ih, int* w, int* h)
{
  int size = iupdrvGetScrollbarSize();

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 20;
    *h = size;
  }
  else
  {
    *w = size;
    *h = 20;
  }
}

static int qmlScrollbarMapMethod(Ihandle* ih)
{
  QQuickItem* sb;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    sb = iupqmlCreateItem(IUPQML_IMPORTS "ScrollBar { orientation: Qt.Horizontal; policy: ScrollBar.AlwaysOn; interactive: true }");
  else
    sb = iupqmlCreateItem(IUPQML_IMPORTS "ScrollBar { orientation: Qt.Vertical; policy: ScrollBar.AlwaysOn; interactive: true }");

  if (!sb)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(sb);
  sb->setProperty("active", true);

  iupdrvScrollbarUpdate(ih);

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, sb);
  sb->setProperty("_iup_keys_handled", true);
  sb->installEventFilter(new IupQmlScrollbarFilter(sb, ih));

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(sb, 0);

  iupqmlConnect(sb, "positionChanged()", [ih](void**) {
    qmlScrollbarPositionChanged(ih);
  });

  return IUP_NOERROR;
}

static void qmlScrollbarUnMapMethod(Ihandle* ih)
{
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

extern "C" IUP_SDK_API void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = qmlScrollbarMapMethod;
  ic->UnMap = qmlScrollbarUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, qmlScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, qmlScrollbarSetLineStepAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, qmlScrollbarSetPageStepAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, qmlScrollbarSetPageSizeAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
}
