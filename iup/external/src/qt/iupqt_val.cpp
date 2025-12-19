/** \file
 * \brief Valuator Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QSlider>
#include <QWidget>
#include <QEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>

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
#include "iup_val.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
}

#include "iupqt_drv.h"


/* Use a large internal range for better precision when dealing with doubles */
#define IVAL_RANGE 10000

/****************************************************************************
 * Custom Qt Slider with Event Handling
 ****************************************************************************/

class IupQtSlider : public QSlider
{
public:
  Ihandle* iup_handle;
  bool button_pressed;

  IupQtSlider(Qt::Orientation orientation, Ihandle* ih)
    : QSlider(orientation), iup_handle(ih), button_pressed(false)
  {
    setRange(0, IVAL_RANGE);
    setTracking(true);
  }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    QSlider::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void leaveEvent(QEvent* event) override
  {
    QSlider::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    QSlider::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QSlider::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    /* Handle inverted Home/End keys */
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

    /* Handle Ctrl+Arrows for page stepping */
    if (event->modifiers() & Qt::ControlModifier)
    {
      if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Up)
      {
        int newval = value() - pageStep();
        if (newval < minimum()) newval = minimum();
        setValue(newval);
        event->accept();
        return;
      }
      else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Down)
      {
        int newval = value() + pageStep();
        if (newval > maximum()) newval = maximum();
        setValue(newval);
        event->accept();
        return;
      }
    }

    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }

    QSlider::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    QSlider::mousePressEvent(event);
    button_pressed = true;
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QSlider::mouseReleaseEvent(event);
    button_pressed = false;
    iupqtMouseButtonEvent(this, event, iup_handle);
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qtValUpdateValue(IupQtSlider* slider, Ihandle* ih, bool button_release)
{
  double old_val = ih->data->val;
  int ival = slider->value();

  double fval = (double)ival / (double)IVAL_RANGE;

  ih->data->val = fval * (ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  iupValCropValue(ih);

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (ih->data->val == old_val)
      return;

    cb(ih);
  }
  else
  {
    IFnd cb_old = nullptr;

    if (button_release)
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
    else if (slider->button_pressed)
      cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    else
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");

    if (cb_old)
      cb_old(ih, ih->data->val);
  }
}

/****************************************************************************
 * Min Size Calculation
 ****************************************************************************/

extern "C" void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  if (horiz_min_w < 0)
  {
    QSlider temp_horiz(Qt::Horizontal);
    QSlider temp_vert(Qt::Vertical);

    QSize horiz_hint = temp_horiz.minimumSizeHint();
    QSize vert_hint = temp_vert.minimumSizeHint();

    horiz_min_w = horiz_hint.width();
    horiz_min_h = horiz_hint.height();
    vert_min_w = vert_hint.width();
    vert_min_h = vert_hint.height();

    if (horiz_min_w < 20) horiz_min_w = 20;
    if (horiz_min_h < 20) horiz_min_h = 20;
    if (vert_min_w < 20) vert_min_w = 20;
    if (vert_min_h < 20) vert_min_h = 20;
  }

  int ticks_size = 0;
  if (iupAttribGetInt(ih, "SHOWTICKS"))
  {
    char* tickspos = iupAttribGetStr(ih, "TICKSPOS");
    if (iupStrEqualNoCase(tickspos, "BOTH"))
      ticks_size = 2*8;
    else
      ticks_size = 8;
  }

  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = horiz_min_w;
    *h = horiz_min_h + ticks_size;
  }
  else
  {
    *w = vert_min_w + ticks_size;
    *h = vert_min_h;
  }
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
  {
    IupQtSlider* slider = (IupQtSlider*)ih->handle;
    if (slider)
    {
      int istep = (int)(ih->data->step * IVAL_RANGE);
      if (istep < 1) istep = 1;
      slider->setSingleStep(istep);
    }
  }
  return 0;
}

static int qtValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
  {
    IupQtSlider* slider = (IupQtSlider*)ih->handle;
    if (slider)
    {
      int ipagestep = (int)(ih->data->pagestep * IVAL_RANGE);
      if (ipagestep < 1) ipagestep = 1;
      slider->setPageStep(ipagestep);
    }
  }
  return 0;
}

static int qtValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    IupQtSlider* slider = (IupQtSlider*)ih->handle;
    if (slider)
    {
      double fval = (ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);

      if (fval < 0.0) fval = 0.0;
      if (fval > 1.0) fval = 1.0;

      int ival = (int)(fval * IVAL_RANGE);

      slider->blockSignals(true);
      slider->setValue(ival);
      slider->blockSignals(false);
    }
  }
  return 0;
}

static int qtValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int show_ticks = 0;

  if (value)
    show_ticks = atoi(value);

  if (show_ticks < 2) show_ticks = 0;

  ih->data->show_ticks = show_ticks;

  IupQtSlider* slider = (IupQtSlider*)ih->handle;
  if (slider)
  {
    if (show_ticks > 0)
    {
      int tick_interval = IVAL_RANGE / (show_ticks - 1);
      if (tick_interval < 1) tick_interval = 1;

      slider->setTickInterval(tick_interval);

      char* tickspos = iupAttribGetStr(ih, "TICKSPOS");
      if (iupStrEqualNoCase(tickspos, "BOTH"))
        slider->setTickPosition(QSlider::TicksBothSides);
      else if (iupStrEqualNoCase(tickspos, "REVERSE"))
      {
        if (ih->data->orientation == IVAL_HORIZONTAL)
          slider->setTickPosition(QSlider::TicksAbove);
        else
          slider->setTickPosition(QSlider::TicksRight);
      }
      else
      {
        if (ih->data->orientation == IVAL_HORIZONTAL)
          slider->setTickPosition(QSlider::TicksBelow);
        else
          slider->setTickPosition(QSlider::TicksLeft);
      }
    }
    else
    {
      slider->setTickPosition(QSlider::NoTicks);
    }
  }

  return 0;
}

static int qtValSetTicksPosAttrib(Ihandle* ih, const char* value)
{
  IupQtSlider* slider = (IupQtSlider*)ih->handle;
  if (!slider)
    return 0;

  if (ih->data->show_ticks <= 0)
    return 1;

  if (iupStrEqualNoCase(value, "BOTH"))
  {
    slider->setTickPosition(QSlider::TicksBothSides);
  }
  else if (iupStrEqualNoCase(value, "REVERSE"))
  {
    if (ih->data->orientation == IVAL_HORIZONTAL)
      slider->setTickPosition(QSlider::TicksAbove);
    else
      slider->setTickPosition(QSlider::TicksRight);
  }
  else
  {
    if (ih->data->orientation == IVAL_HORIZONTAL)
      slider->setTickPosition(QSlider::TicksBelow);
    else
      slider->setTickPosition(QSlider::TicksLeft);
  }

  return 1;
}

static int qtValSetStepOnTicksAttrib(Ihandle* ih, const char* value)
{
  /* Qt doesn't have direct support for snapping to tick marks like Cocoa's allowsTickMarkValuesOnly. */
  (void)ih;
  (void)value;
  return 1; /* Store value in hash table */
}

static char* qtValGetStepOnTicksAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "STEPONTICKS");
}

static char* qtValGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static char* qtValGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static char* qtValGetInvertedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->inverted);
}

static int qtValSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtSlider* slider = (IupQtSlider*)ih->handle;

  if (slider)
  {
    QPalette palette = slider->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    slider->setPalette(palette);
    slider->setAutoFillBackground(true);
    return 1;
  }

  return 0;
}

static int qtValSetTipAttrib(Ihandle* ih, const char* value)
{
  return iupdrvBaseSetTipAttrib(ih, value);
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static void qtValValueChanged(IupQtSlider* slider, Ihandle* ih)
{
  qtValUpdateValue(slider, ih, false);
}

static void qtValSliderReleased(IupQtSlider* slider, Ihandle* ih)
{
  qtValUpdateValue(slider, ih, true);
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtValMapMethod(Ihandle* ih)
{
  Qt::Orientation orientation;

  if (ih->data->orientation == IVAL_HORIZONTAL)
    orientation = Qt::Horizontal;
  else
    orientation = Qt::Vertical;

  IupQtSlider* slider = new IupQtSlider(orientation, ih);

  if (!slider)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)slider;

  if (ih->data->inverted)
  {
    slider->setInvertedAppearance(true);
    slider->setInvertedControls(true);  /* Also invert keyboard controls */
  }

  int istep = (int)(ih->data->step * IVAL_RANGE);
  if (istep < 1) istep = 1;
  slider->setSingleStep(istep);

  int ipagestep = (int)(ih->data->pagestep * IVAL_RANGE);
  if (ipagestep < 1) ipagestep = 1;
  slider->setPageStep(ipagestep);

  double fval = (ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
  if (fval < 0.0) fval = 0.0;
  if (fval > 1.0) fval = 1.0;
  int ival = (int)(fval * IVAL_RANGE);
  slider->setValue(ival);

  int show_ticks = iupAttribGetInt(ih, "SHOWTICKS");
  if (show_ticks > 0)
  {
    ih->data->show_ticks = show_ticks;
    qtValSetShowTicksAttrib(ih, iupAttribGetStr(ih, "SHOWTICKS"));
  }

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(slider, 0);

  QObject::connect(slider, &QSlider::valueChanged, [slider, ih]() {
    qtValValueChanged(slider, ih);
  });

  QObject::connect(slider, &QSlider::sliderReleased, [slider, ih]() {
    qtValSliderReleased(slider, ih);
  });

  iupqtUpdateMnemonic(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * UnMap Method
 ****************************************************************************/

static void qtValUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    IupQtSlider* slider = (IupQtSlider*)ih->handle;

    iupqtTipsDestroy(ih);

    /* Delete the slider - Qt will automatically disconnect signals */
    delete slider;
    ih->handle = nullptr;
  }
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvValInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtValMapMethod;
  ic->UnMap = qtValUnMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, qtValSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Common */
  iupClassRegisterAttribute(ic, "TIP", NULL, qtValSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupVal only */
  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, qtValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, qtValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, qtValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Tick marks */
  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, qtValSetShowTicksAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, qtValSetTicksPosAttrib, "NORMAL", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "STEPONTICKS", qtValGetStepOnTicksAttrib, qtValSetStepOnTicksAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_DEFAULT);

  /* MIN/MAX/INVERTED getters */
  iupClassRegisterAttribute(ic, "MIN", qtValGetMinAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", qtValGetMaxAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", qtValGetInvertedAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
}
