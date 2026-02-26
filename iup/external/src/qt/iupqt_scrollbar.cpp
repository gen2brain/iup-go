/** \file
 * \brief Scrollbar Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QScrollBar>
#include <QWidget>
#include <QEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QAbstractSlider>
#include <QProxyStyle>
#include <QStyleOptionSlider>
#include <QStyleFactory>
#include <QApplication>
#include <QPainter>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_scrollbar.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
}

#include "iupqt_drv.h"


#define ISCROLLBAR_RANGE 10000

class IupQtScrollBarStyle : public QProxyStyle
{
public:
  IupQtScrollBarStyle() : QProxyStyle()
  {
    QStyle* appStyle = QApplication::style();
    QStyle* base = nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QString name = appStyle->name();
    if (!name.isEmpty())
      base = QStyleFactory::create(name);
#endif

    if (!base)
    {
      QString objName = appStyle->objectName();
      if (!objName.isEmpty())
        base = QStyleFactory::create(objName);
    }

    if (!base)
      base = QStyleFactory::create(QStringLiteral("Fusion"));

    if (base)
      setBaseStyle(base);
  }

  int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
  {
    if (hint == SH_ScrollBar_Transient)
      return 0;
    return QProxyStyle::styleHint(hint, option, widget, returnData);
  }

  void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                          QPainter *painter, const QWidget *widget = nullptr) const override
  {
    if (control == CC_ScrollBar)
    {
      QStyleOptionSlider opt(*static_cast<const QStyleOptionSlider*>(option));
      opt.state |= State_MouseOver;
      QProxyStyle::drawComplexControl(control, &opt, painter, widget);
      return;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
  }

  QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                       SubControl subControl, const QWidget *widget = nullptr) const override
  {
    if (control == CC_ScrollBar)
    {
      QStyleOptionSlider opt(*static_cast<const QStyleOptionSlider*>(option));
      opt.state |= State_MouseOver;
      return QProxyStyle::subControlRect(control, &opt, subControl, widget);
    }
    return QProxyStyle::subControlRect(control, option, subControl, widget);
  }
};

class IupQtScrollBar : public QScrollBar
{
public:
  Ihandle* iup_handle;

  IupQtScrollBar(Qt::Orientation orientation, Ihandle* ih)
    : QScrollBar(orientation), iup_handle(ih)
  {
    setStyle(new IupQtScrollBarStyle());
    setRange(0, ISCROLLBAR_RANGE);
    setTracking(true);
  }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    QScrollBar::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void leaveEvent(QEvent* event) override
  {
    QScrollBar::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    QScrollBar::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QScrollBar::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (iup_handle->data->inverted)
    {
      if (event->key() == Qt::Key_Home)
      {
        setValue(maximum());
        event->accept();
        return;
      }
      else if (event->key() == Qt::Key_End)
      {
        setValue(minimum());
        event->accept();
        return;
      }
    }

    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }

    QScrollBar::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    QScrollBar::mousePressEvent(event);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QScrollBar::mouseReleaseEvent(event);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }
};

static void qtScrollbarUpdateNative(Ihandle* ih)
{
  IupQtScrollBar* sb = (IupQtScrollBar*)ih->handle;
  if (!sb) return;

  double range = ih->data->vmax - ih->data->vmin;
  if (range <= 0) return;

  int ipage = (int)((ih->data->pagesize / range) * ISCROLLBAR_RANGE);
  if (ipage < 1) ipage = 1;
  if (ipage > ISCROLLBAR_RANGE) ipage = ISCROLLBAR_RANGE;

  int imax = ISCROLLBAR_RANGE - ipage;
  if (imax < 0) imax = 0;

  int istep = (int)((ih->data->linestep / range) * ISCROLLBAR_RANGE);
  if (istep < 1) istep = 1;

  int ipagestep = (int)((ih->data->pagestep / range) * ISCROLLBAR_RANGE);
  if (ipagestep < 1) ipagestep = 1;

  int ipos = (int)(((ih->data->val - ih->data->vmin) / range) * ISCROLLBAR_RANGE);
  if (ipos < 0) ipos = 0;
  if (ipos > imax) ipos = imax;

  sb->blockSignals(true);
  sb->setRange(0, imax);
  sb->setPageStep(ipage);
  sb->setSingleStep(istep);
  sb->setValue(ipos);
  sb->blockSignals(false);
}


/*********************************************************************************************/


static void qtScrollbarActionTriggered(IupQtScrollBar* sb, Ihandle* ih, int action)
{
  double old_val = ih->data->val;
  double range = ih->data->vmax - ih->data->vmin;
  int imax = sb->maximum();
  int ipos = sb->sliderPosition();

  if (imax > 0)
    ih->data->val = ((double)ipos / (double)ISCROLLBAR_RANGE) * range + ih->data->vmin;
  else
    ih->data->val = ih->data->vmin;
  iupScrollbarCropValue(ih);

  int op = -1;
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    switch (action)
    {
    case QAbstractSlider::SliderSingleStepSub: op = IUP_SBLEFT; break;
    case QAbstractSlider::SliderSingleStepAdd: op = IUP_SBRIGHT; break;
    case QAbstractSlider::SliderPageStepSub:   op = IUP_SBPGLEFT; break;
    case QAbstractSlider::SliderPageStepAdd:   op = IUP_SBPGRIGHT; break;
    case QAbstractSlider::SliderMove:          op = IUP_SBDRAGH; break;
    default:                                   op = IUP_SBPOSH; break;
    }
  }
  else
  {
    switch (action)
    {
    case QAbstractSlider::SliderSingleStepSub: op = IUP_SBUP; break;
    case QAbstractSlider::SliderSingleStepAdd: op = IUP_SBDN; break;
    case QAbstractSlider::SliderPageStepSub:   op = IUP_SBPGUP; break;
    case QAbstractSlider::SliderPageStepAdd:   op = IUP_SBPGDN; break;
    case QAbstractSlider::SliderMove:          op = IUP_SBDRAGV; break;
    default:                                   op = IUP_SBPOSV; break;
    }
  }

  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      posx = (float)ih->data->val;
    else
      posy = (float)ih->data->val;

    scroll_cb(ih, op, posx, posy);
  }

  IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (valuechanged_cb)
  {
    if (ih->data->val != old_val)
      valuechanged_cb(ih);
  }
}


/*********************************************************************************************/


static int qtScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    qtScrollbarUpdateNative(ih);
  }
  return 0;
}

static int qtScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    qtScrollbarUpdateNative(ih);
  return 0;
}

static int qtScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    qtScrollbarUpdateNative(ih);
  return 0;
}

static int qtScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    qtScrollbarUpdateNative(ih);
  }
  return 0;
}


/*********************************************************************************************/


extern "C" void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  if (horiz_min_w < 0)
  {
    QScrollBar temp_horiz(Qt::Horizontal);
    QScrollBar temp_vert(Qt::Vertical);

    QSize horiz_hint = temp_horiz.minimumSizeHint();
    QSize vert_hint = temp_vert.minimumSizeHint();

    horiz_min_w = horiz_hint.width();
    horiz_min_h = horiz_hint.height();
    vert_min_w = vert_hint.width();
    vert_min_h = vert_hint.height();

    if (horiz_min_w < 20) horiz_min_w = 20;
    if (horiz_min_h < 15) horiz_min_h = 15;
    if (vert_min_w < 15) vert_min_w = 15;
    if (vert_min_h < 20) vert_min_h = 20;
  }

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = horiz_min_w;
    *h = horiz_min_h;
  }
  else
  {
    *w = vert_min_w;
    *h = vert_min_h;
  }
}

static int qtScrollbarMapMethod(Ihandle* ih)
{
  Qt::Orientation orientation;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
    orientation = Qt::Horizontal;
  else
    orientation = Qt::Vertical;

  IupQtScrollBar* sb = new IupQtScrollBar(orientation, ih);

  if (!sb)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)sb;

  if (ih->data->inverted)
  {
    sb->setInvertedAppearance(true);
    sb->setInvertedControls(true);
  }

  qtScrollbarUpdateNative(ih);

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(sb, 0);

  QObject::connect(sb, &QScrollBar::actionTriggered, [sb, ih](int action) {
    qtScrollbarActionTriggered(sb, ih, action);
  });

  iupqtUpdateMnemonic(ih);

  return IUP_NOERROR;
}

static void qtScrollbarUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    IupQtScrollBar* sb = (IupQtScrollBar*)ih->handle;

    iupqtTipsDestroy(ih);

    delete sb;
    ih->handle = nullptr;
  }
}

extern "C" void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = qtScrollbarMapMethod;
  ic->UnMap = qtScrollbarUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, nullptr, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, qtScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, qtScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, qtScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, qtScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
