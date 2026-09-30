/** \file
 * \brief Valuator Control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QColor>
#include <QKeyEvent>

#include <cmath>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
}

#include "iupqml_drv.h"


extern "C" IUP_SDK_API void iupdrvValGetMinSize(Ihandle* ih, int* w, int* h)
{
  QQuickItem* horiz = iupqmlTemplateItem(IUPQML_IMPORTS "Slider { orientation: Qt.Horizontal }");
  QQuickItem* vert = iupqmlTemplateItem(IUPQML_IMPORTS "Slider { orientation: Qt.Vertical }");
  int horiz_min_w = 20, horiz_min_h = 20, vert_min_w = 20, vert_min_h = 20;

  if (horiz)
  {
    horiz_min_w = static_cast<int>(horiz->implicitWidth());
    horiz_min_h = static_cast<int>(horiz->implicitHeight());
  }
  if (vert)
  {
    vert_min_w = static_cast<int>(vert->implicitWidth());
    vert_min_h = static_cast<int>(vert->implicitHeight());
  }

  if (horiz_min_w < 20) horiz_min_w = 20;
  if (horiz_min_h < 20) horiz_min_h = 20;
  if (vert_min_w < 20) vert_min_w = 20;
  if (vert_min_h < 20) vert_min_h = 20;

  int ticks_room = 0;
  if (ih->data->show_ticks > 1)
    ticks_room = iupStrEqualNoCase(iupAttribGetStr(ih, "TICKSPOS"), "BOTH") ? 12 : 6;

  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = horiz_min_w;
    *h = horiz_min_h + ticks_room;
  }
  else
  {
    *w = vert_min_w + ticks_room;
    *h = vert_min_h;
  }
}

static int qmlValFlipped(Ihandle* ih)
{
  if (ih->data->orientation == IVAL_VERTICAL)
    return !ih->data->inverted;
  return ih->data->inverted;
}

static void qmlValSnapToTicks(Ihandle* ih)
{
  if (ih->data->show_ticks < 2 || !iupAttribGetBoolean(ih, "STEPONTICKS") || ih->data->vmax <= ih->data->vmin)
    return;

  double tick = (ih->data->vmax - ih->data->vmin) / (ih->data->show_ticks - 1);
  ih->data->val = ih->data->vmin + std::round((ih->data->val - ih->data->vmin) / tick) * tick;
}

static void qmlValUpdateTicks(Ihandle* ih)
{
  auto* slider = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!slider)
    return;

  auto* ticks = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_VAL_TICKS"));
  if (ih->data->show_ticks < 2)
  {
    if (ticks)
    {
      delete ticks;
      iupAttribSet(ih, "_IUPQML_VAL_TICKS", nullptr);
    }
    return;
  }

  if (!ticks)
  {
    ticks = iupqmlCreateItem(IUPQML_IMPORTS
      "Item { id: tk; anchors.fill: parent; z: -1; property int count: 2; property int side: 1; property bool vertical: false\n"
      "  readonly property real hw: tk.parent && tk.parent.handle ? tk.parent.handle.width : 0\n"
      "  readonly property real hh: tk.parent && tk.parent.handle ? tk.parent.handle.height : 0\n"
      "  Repeater { model: tk.count\n"
      "    Item { required property int index\n"
      "      readonly property real pos: !tk.parent ? 0 : Math.round(tk.vertical ? tk.parent.topPadding + tk.hh / 2 + index * (tk.parent.availableHeight - tk.hh) / (tk.count - 1)\n"
      "                                                         : tk.parent.leftPadding + tk.hw / 2 + index * (tk.parent.availableWidth - tk.hw) / (tk.count - 1))\n"
      "      Rectangle { visible: tk.side & 1; color: tk.parent ? tk.parent.palette.dark : \"transparent\"; x: tk.vertical ? 1 : parent.pos; y: tk.vertical ? parent.pos : 1; width: tk.vertical ? 4 : 1; height: tk.vertical ? 1 : 4 }\n"
      "      Rectangle { visible: tk.side & 2; color: tk.parent ? tk.parent.palette.dark : \"transparent\"; x: tk.vertical ? tk.width - 5 : parent.pos; y: tk.vertical ? parent.pos : tk.height - 5; width: tk.vertical ? 4 : 1; height: tk.vertical ? 1 : 4 }\n"
      "    }\n"
      "  }\n"
      "}");
    if (!ticks)
      return;
    ticks->setParentItem(slider);
    ticks->setParent(slider);
    iupAttribSet(ih, "_IUPQML_VAL_TICKS", reinterpret_cast<char*>(ticks));
  }

  const char* pos = iupAttribGetStr(ih, "TICKSPOS");
  int side = iupStrEqualNoCase(pos, "BOTH") ? 3 : (iupStrEqualNoCase(pos, "REVERSE") ? 2 : 1);
  ticks->setProperty("vertical", ih->data->orientation != IVAL_HORIZONTAL);
  ticks->setProperty("side", side);
  ticks->setProperty("count", ih->data->show_ticks);
}

static int qmlValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int show_ticks = 0;
  if (value)
    iupStrToInt(value, &show_ticks);
  ih->data->show_ticks = show_ticks < 2 ? 0 : show_ticks;
  qmlValUpdateTicks(ih);
  return 0;
}

static void qmlValSetSliderValue(Ihandle* ih)
{
  auto* slider = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!slider)
    return;

  double value = ih->data->val;
  if (qmlValFlipped(ih))
    value = ih->data->vmax + ih->data->vmin - value;

  iupAttribSet(ih, "_IUPQML_IGNORE_MOVE", "1");
  slider->setProperty("value", value);
  iupAttribSet(ih, "_IUPQML_IGNORE_MOVE", nullptr);
}

static void qmlValUpdateValue(Ihandle* ih)
{
  auto* slider = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!slider)
    return;

  double old_val = ih->data->val;
  double value = slider->property("value").toDouble();
  if (qmlValFlipped(ih))
    value = ih->data->vmax + ih->data->vmin - value;

  ih->data->val = value;
  iupValCropValue(ih);
  if (iupAttribGetBoolean(ih, "STEPONTICKS") && ih->data->show_ticks > 1)
  {
    qmlValSnapToTicks(ih);
    qmlValSetSliderValue(ih);
  }

  IFn cb = static_cast<IFn>(IupGetCallback(ih, "VALUECHANGED_CB"));
  if (cb && ih->data->val != old_val)
    cb(ih);
}


static void qmlValUpdateRange(Ihandle* ih)
{
  auto* slider = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!slider)
    return;

  slider->setProperty("from", ih->data->vmin);
  slider->setProperty("to", ih->data->vmax);
  qmlValSetSliderValue(ih);
}

static int qmlValSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
  {
    iupValCropValue(ih);
    qmlValUpdateRange(ih);
  }
  return 0;
}

static int qmlValSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
  {
    iupValCropValue(ih);
    qmlValUpdateRange(ih);
  }
  return 0;
}

static int qmlValSetStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->step), 0.01);
  return 0;
}

static int qmlValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
  return 0;
}

static int qmlValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupValCropValue(ih);
    qmlValSetSliderValue(ih);
  }
  return 0;
}

static int qmlValSetInvertedAttrib(Ihandle* ih, const char* value)
{
  ih->data->inverted = iupStrBoolean(value);
  qmlValSetSliderValue(ih);
  return 0;
}

static int qmlValSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  auto* slider = reinterpret_cast<QQuickItem*>(ih->handle);

  if (!slider || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(slider, "window", QColor(r, g, b));
  return 1;
}

class IupQmlValKeyFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlValKeyFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!iupObjectCheck(ih))
      return false;

    QQuickItem* slider = qobject_cast<QQuickItem*>(obj);
    double delta;

    if (event->type() == QEvent::Wheel)
    {
      auto* we = static_cast<QWheelEvent*>(event);
      QPoint angle = we->angleDelta();
      int d = qAbs(angle.y()) < qAbs(angle.x()) ? angle.x() : (we->inverted() ? -angle.y() : angle.y());
      delta = (static_cast<double>(d) / QWheelEvent::DefaultDeltasPerStep) * ih->data->step;
    }
    else if (event->type() == QEvent::KeyPress)
    {
      auto* evt = static_cast<QKeyEvent*>(event);
      if (iupqmlKeyPressEvent(slider, evt, ih))
        return true;

      bool mirrored = slider && slider->property("mirrored").toBool();
      int key = evt->key();
      if (key == Qt::Key_PageUp)
        delta = ih->data->pagestep;
      else if (key == Qt::Key_PageDown)
        delta = -ih->data->pagestep;
      else if (ih->data->orientation == IVAL_HORIZONTAL && (key == Qt::Key_Left || key == Qt::Key_Right))
        delta = ((key == Qt::Key_Right) != mirrored) ? ih->data->step : -ih->data->step;
      else if (ih->data->orientation != IVAL_HORIZONTAL && (key == Qt::Key_Up || key == Qt::Key_Down))
        delta = key == Qt::Key_Up ? ih->data->step : -ih->data->step;
      else
        return false;
    }
    else
      return false;

    double old_val = ih->data->val;
    if (iupAttribGetBoolean(ih, "STEPONTICKS") && ih->data->show_ticks > 1)
    {
      double tick = 1.0 / (ih->data->show_ticks - 1);
      delta = delta > 0 ? qMax(delta, tick) : qMin(delta, -tick);
    }
    ih->data->val += delta * (ih->data->vmax - ih->data->vmin);
    iupValCropValue(ih);
    qmlValSnapToTicks(ih);
    qmlValSetSliderValue(ih);

    IFn cb = static_cast<IFn>(IupGetCallback(ih, "VALUECHANGED_CB"));
    if (cb && ih->data->val != old_val)
      cb(ih);
    return true;
  }
};

static int qmlValMapMethod(Ihandle* ih)
{
  QQuickItem* slider;

  if (ih->data->orientation == IVAL_HORIZONTAL)
    slider = iupqmlCreateItem(IUPQML_IMPORTS "Slider { orientation: Qt.Horizontal; live: true; wheelEnabled: true }");
  else
    slider = iupqmlCreateItem(IUPQML_IMPORTS "Slider { orientation: Qt.Vertical; live: true; wheelEnabled: true }");

  if (!slider)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(slider);

  slider->setProperty("from", ih->data->vmin);
  slider->setProperty("to", ih->data->vmax);
  qmlValSetSliderValue(ih);

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, slider);
  slider->setProperty("_iup_keys_handled", true);
  slider->installEventFilter(new IupQmlValKeyFilter(slider, ih));

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(slider, 0);

  qmlValUpdateTicks(ih);

  iupqmlConnect(slider, "moved()", [ih](void**) {
    if (!iupAttribGet(ih, "_IUPQML_IGNORE_MOVE"))
      qmlValUpdateValue(ih);
  });

  return IUP_NOERROR;
}

static void qmlValUnMapMethod(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPQML_VAL_TICKS", nullptr);
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

extern "C" IUP_SDK_API void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = qmlValMapMethod;
  ic->UnMap = qmlValUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlValSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, qmlValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", nullptr, qmlValSetInvertedAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, qmlValSetPageStepAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, qmlValSetStepAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFunc(ic, "MIN", nullptr, qmlValSetMinAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "MAX", nullptr, qmlValSetMaxAttrib);

  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, qmlValSetShowTicksAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TICKSPOS", nullptr, nullptr, "NORMAL", nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "STEPONTICKS", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "NO", IUPAF_DEFAULT);
}
