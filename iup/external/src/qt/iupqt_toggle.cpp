/** \file
 * \brief Toggle Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QCheckBox>
#include <QRadioButton>
#include <QToolButton>
#include <QAbstractButton>
#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QStyleOptionButton>
#include <QStyle>
#include <QPainter>
#include <QTimer>
#include <QPalette>
#include <QElapsedTimer>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <type_traits>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_toggle.h"
}

#include "iupqt_drv.h"

/* Image position constants (from iup_button.h to avoid struct redefinition) */
enum{IUP_IMGPOS_LEFT, IUP_IMGPOS_RIGHT, IUP_IMGPOS_TOP, IUP_IMGPOS_BOTTOM};

/****************************************************************************
 * Custom Qt Toggle Widgets with Event Handling
 ****************************************************************************/

/* Base class for toggle event handling */
template<typename BaseWidget>
class IupQtToggleBase : public BaseWidget
{
protected:
  Ihandle* iup_handle;
  bool last_was_double_click;

public:
  explicit IupQtToggleBase(Ihandle* ih) : BaseWidget(), iup_handle(ih), last_was_double_click(false) {}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    BaseWidget::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);

    /* Handle FLAT mode for image toggles (QToolButton only) */
    if constexpr (std::is_same<BaseWidget, QToolButton>::value)
    {
      if (iup_handle->data->type == IUP_TOGGLE_IMAGE && iup_handle->data->flat)
      {
        /* Show button when mouse enters, regardless of checked state */
        this->setAutoRaise(false);
      }
    }
  }

  void leaveEvent(QEvent* event) override
  {
    BaseWidget::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);

    /* Handle FLAT mode (QToolButton only) */
    if constexpr (std::is_same<BaseWidget, QToolButton>::value)
    {
      if (iup_handle->data->type == IUP_TOGGLE_IMAGE && iup_handle->data->flat)
      {
        /* Hide button when mouse leaves and not checked */
        if (!this->isChecked())
          this->setAutoRaise(true);
      }
    }
  }

  void focusInEvent(QFocusEvent* event) override
  {
    BaseWidget::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    BaseWidget::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }
    BaseWidget::keyPressEvent(event);
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override
  {
    /* Handle IGNOREDOUBLECLICK */
    if (iupAttribGetBoolean(iup_handle, "IGNOREDOUBLECLICK"))
    {
      event->ignore();
      return;
    }

    last_was_double_click = true;
    BaseWidget::mouseDoubleClickEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    last_was_double_click = false;
    BaseWidget::mousePressEvent(event);
  }
};

/* Specific toggle types */
class IupQtCheckBox : public IupQtToggleBase<QCheckBox>
{
public:
  explicit IupQtCheckBox(Ihandle* ih) : IupQtToggleBase<QCheckBox>(ih) {}
};

class IupQtRadioButton : public IupQtToggleBase<QRadioButton>
{
public:
  explicit IupQtRadioButton(Ihandle* ih) : IupQtToggleBase<QRadioButton>(ih) {}
};

class IupQtToolButton : public IupQtToggleBase<QToolButton>
{
public:
  explicit IupQtToolButton(Ihandle* ih) : IupQtToggleBase<QToolButton>(ih)
  {
    setCheckable(true);
    setAutoRaise(false);
  }
};

/****************************************************************************
 * Custom Switch Widget
 ****************************************************************************/

class IupQtSwitch : public IupQtToggleBase<QAbstractButton>
{
private:
  qreal thumb_position;  /* 0.0 = left (off), 1.0 = right (on) */
  QTimer* animation_timer;
  QElapsedTimer elapsed_timer;
  qreal animation_start;
  qreal animation_end;
  int animation_duration;  /* milliseconds */
  bool is_hovered;

  /* Switch dimensions */
  static constexpr int TRACK_WIDTH = 50;
  static constexpr int TRACK_HEIGHT = 26;
  static constexpr int THUMB_SIZE = 22;
  static constexpr int THUMB_MARGIN = 2;

  /* Easing function: Quad in-out */
  qreal easeInOutQuad(qreal t) const
  {
    if (t < 0.5)
      return 2.0 * t * t;
    else
      return 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
  }

public:
  explicit IupQtSwitch(Ihandle* ih) : IupQtToggleBase<QAbstractButton>(ih),
    thumb_position(0.0), animation_timer(nullptr), animation_start(0.0),
    animation_end(0.0), animation_duration(120), is_hovered(false)
  {
    setCheckable(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    /* Create timer for animation */
    animation_timer = new QTimer(this);
    animation_timer->setInterval(16);  /* ~60 FPS */

    /* Connect timer to animation step */
    connect(animation_timer, &QTimer::timeout, this, [this]() {
      qint64 elapsed = elapsed_timer.elapsed();

      if (elapsed >= animation_duration)
      {
        /* Animation complete */
        thumb_position = animation_end;
        animation_timer->stop();
      }
      else
      {
        /* Interpolate with easing */
        qreal t = (qreal)elapsed / (qreal)animation_duration;
        qreal eased_t = easeInOutQuad(t);
        thumb_position = animation_start + (animation_end - animation_start) * eased_t;
      }

      update();  /* Trigger repaint */
    });

    /* Connect toggled signal to trigger animation */
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
      /* Start animation from current position to target position */
      animation_start = thumb_position;
      animation_end = checked ? 1.0 : 0.0;

      /* Always restart timer */
      elapsed_timer.start();
      animation_timer->start();
    });
  }

  /* Call this after the widget is fully constructed and VALUE is set */
  void initializeThumbPosition()
  {
    /* Set initial thumb position without animation */
    thumb_position = isChecked() ? 1.0 : 0.0;
    update();
  }

  ~IupQtSwitch()
  {
    if (animation_timer)
    {
      animation_timer->stop();
      delete animation_timer;
    }
  }

  /* Direct setter for programmatic changes (no animation) */
  void setThumbPosition(qreal pos)
  {
    thumb_position = pos;
    update();  /* Trigger repaint */
  }

  QSize sizeHint() const override
  {
    return QSize(TRACK_WIDTH, TRACK_HEIGHT);
  }

  QSize minimumSizeHint() const override
  {
    return QSize(TRACK_WIDTH, TRACK_HEIGHT);
  }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    IupQtToggleBase<QAbstractButton>::enterEvent(event);
    is_hovered = true;
    update();  /* Trigger repaint to show hover effect */
  }

  void leaveEvent(QEvent* event) override
  {
    IupQtToggleBase<QAbstractButton>::leaveEvent(event);
    is_hovered = false;
    update();  /* Trigger repaint to remove hover effect */
  }

  void paintEvent(QPaintEvent* event) override
  {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    /* Get colors from palette for theme adaptation */
    QPalette pal = palette();
    QColor track_color, thumb_color, border_color;

    if (!isEnabled())
    {
      /* Disabled state - use muted colors */
      track_color = pal.color(QPalette::Disabled, QPalette::Mid);
      thumb_color = pal.color(QPalette::Disabled, QPalette::Base);
      border_color = pal.color(QPalette::Disabled, QPalette::Dark);
    }
    else
    {
      /* Interpolate track color based on thumb position for smooth animation */
      QColor track_off_color = pal.color(QPalette::Active, QPalette::Mid);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
      /* Use Accent if it's a real color (macOS sets it to controlAccentColor).
         If Accent is white/near-white, it wasn't set properly - use Highlight instead. */
      QColor accent = pal.color(QPalette::Active, QPalette::Accent);
      QColor track_on_color = (accent.lightness() > 250) ? pal.color(QPalette::Active, QPalette::Highlight) : accent;
#else
      QColor track_on_color = pal.color(QPalette::Active, QPalette::Highlight);
#endif

      /* Blend colors based on thumb position */
      track_color = QColor(
        track_off_color.red() + (track_on_color.red() - track_off_color.red()) * thumb_position,
        track_off_color.green() + (track_on_color.green() - track_off_color.green()) * thumb_position,
        track_off_color.blue() + (track_on_color.blue() - track_off_color.blue()) * thumb_position
      );

      thumb_color = pal.color(QPalette::Active, QPalette::Base);
      border_color = track_color.darker(120);
    }

    /* Draw track (rounded rectangle) with border */
    /* Use slightly reduced radius and inset rect for smoother rounded ends */
    QRectF track_rect(0.5, 0.5, TRACK_WIDTH - 1, TRACK_HEIGHT - 1);
    qreal radius = (TRACK_HEIGHT - 1) / 2.0;
    painter.setBrush(track_color);
    painter.setPen(QPen(border_color, 1));
    painter.drawRoundedRect(track_rect, radius, radius);

    /* Calculate thumb position (interpolate between left and right) */
    int thumb_x_min = THUMB_MARGIN;
    int thumb_x_max = TRACK_WIDTH - THUMB_SIZE - THUMB_MARGIN;
    int thumb_x = thumb_x_min + (thumb_x_max - thumb_x_min) * thumb_position;
    int thumb_y = (TRACK_HEIGHT - THUMB_SIZE) / 2;

    /* Draw shadow for thumb (subtle depth effect) */
    if (isEnabled())
    {
      painter.setBrush(QColor(0, 0, 0, 30));
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(thumb_x + 1, thumb_y + 1, THUMB_SIZE, THUMB_SIZE);
    }

    /* Draw thumb (circle) with border */
    QColor thumb_draw_color = thumb_color;
    if (is_hovered && isEnabled())
    {
      /* Use system Light color for hover indication */
      thumb_draw_color = pal.color(QPalette::Active, QPalette::Light);
    }
    painter.setBrush(thumb_draw_color);
    painter.setPen(QPen(border_color.lighter(110), 1));
    painter.drawEllipse(thumb_x, thumb_y, THUMB_SIZE, THUMB_SIZE);
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

#define IUP_TOGGLE_BOX 18  /* Qt checkbox/radio size */


extern "C" void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

extern "C" void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* str)
{
  static int switch_w = -1;
  static int switch_h = -1;
  (void)ih;

  if (switch_w < 0)
  {
    IupQtSwitch temp_switch(NULL);
    QSize hint = temp_switch.sizeHint();

    switch_w = hint.width();
    switch_h = hint.height();
  }

  (*x) += 2 + switch_w + 2;
  if ((*y) < 2 + switch_h + 2) (*y) = 2 + switch_h + 2;
  else (*y) += 2 + 2;

  if (str && str[0])
    (*x) += 8;
}

extern "C" void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  int check_box = IUP_TOGGLE_BOX;

  (*x) += 2 + check_box + 2;
  if ((*y) < 2 + check_box + 2)
    (*y) = 2 + check_box + 2;
  else
    (*y) += 2 + 2;

  if (str && str[0])
  {
    int spacing = iupAttribGetInt(ih, "SPACING");
    if (spacing > 0)
      (*x) += spacing;
    else
      (*x) += 8;
  }
}

static int qtToggleGetCheck(Ihandle* ih)
{
  QAbstractButton* button = (QAbstractButton*)ih->handle;

  if (!button)
    return 0;

  /* Check for 3-state checkbox */
  QCheckBox* checkbox = qobject_cast<QCheckBox*>(button);
  if (checkbox && checkbox->isTristate())
  {
    Qt::CheckState state = checkbox->checkState();
    if (state == Qt::PartiallyChecked)
      return -1;
    else if (state == Qt::Checked)
      return 1;
    else
      return 0;
  }

  /* Regular checkbox or radio button */
  return button->isChecked() ? 1 : 0;
}

static void qtToggleSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  if (!name || ih->data->type != IUP_TOGGLE_IMAGE)
    return;

  QAbstractButton* button = (QAbstractButton*)ih->handle;

  if (button)
  {
    const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, ih, make_inactive, bgcolor);

    if (pixmap && !pixmap->isNull())
    {
      button->setIcon(QIcon(*pixmap));
      button->setIconSize(pixmap->size());
    }
    else
    {
      button->setIcon(QIcon());
    }
  }
}

static void qtToggleUpdateImage(Ihandle* ih, int active, int check)
{
  char* name;

  if (!active)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (name)
      qtToggleSetPixmap(ih, name, 0);
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      qtToggleSetPixmap(ih, name, 1);
    }
  }
  else
  {
    if (check)
    {
      name = iupAttribGet(ih, "IMPRESS");
      if (name)
        qtToggleSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        qtToggleSetPixmap(ih, name, 0);
      }
    }
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      if (name)
        qtToggleSetPixmap(ih, name, 0);
    }
  }
}

static void qtToggleUpdateLayout(Ihandle* ih)
{
  if (ih->data->type != IUP_TOGGLE_IMAGE)
    return;

  QAbstractButton* button = (QAbstractButton*)ih->handle;
  if (!button)
    return;

  /* Get IMAGEPOSITION attribute */
  int img_position = IUP_IMGPOS_LEFT;
  char* value = iupAttribGetStr(ih, "IMAGEPOSITION");
  if (value)
  {
    if (iupStrEqualNoCase(value, "RIGHT"))
      img_position = IUP_IMGPOS_RIGHT;
    else if (iupStrEqualNoCase(value, "TOP"))
      img_position = IUP_IMGPOS_TOP;
    else if (iupStrEqualNoCase(value, "BOTTOM"))
      img_position = IUP_IMGPOS_BOTTOM;
  }

  /* Update layout direction based on image position */
  switch (img_position)
  {
    case IUP_IMGPOS_LEFT:
      button->setLayoutDirection(Qt::LeftToRight);
      break;
    case IUP_IMGPOS_RIGHT:
      button->setLayoutDirection(Qt::RightToLeft);
      break;
    case IUP_IMGPOS_TOP:
    case IUP_IMGPOS_BOTTOM:
      /* Qt doesn't support top/bottom easily, would need custom widget */
      break;
  }

  /* Set spacing */
  int spacing = iupAttribGetInt(ih, "SPACING");
  if (spacing > 0)
  {
    QString style = QString("padding-left: %1px;").arg(spacing);
    button->setStyleSheet(style);
  }
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  QAbstractButton* button = (QAbstractButton*)ih->handle;

  if (!button)
    return 0;

  /* Block signals during programmatic change */
  button->blockSignals(true);

  if (iupStrEqualNoCase(value, "NOTDEF"))
  {
    /* Set 3-state indeterminate */
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(button);
    if (checkbox && checkbox->isTristate())
      checkbox->setCheckState(Qt::PartiallyChecked);
  }
  else if (iupStrEqualNoCase(value, "TOGGLE"))
  {
    /* Toggle current state */
    button->setChecked(!button->isChecked());

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
  }
  else
  {
    /* Set specific value */
    int check = iupStrBoolean(value);
    button->setChecked(check);

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
  }

  button->blockSignals(false);

  /* For switch widgets, update thumb position without animation when set programmatically */
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    /* Safe to static_cast since we know it's a switch widget */
    IupQtSwitch* switch_widget = static_cast<IupQtSwitch*>(button);
    if (switch_widget)
    {
      /* Update thumb position immediately (no animation during programmatic setValue) */
      switch_widget->setThumbPosition(button->isChecked() ? 1.0 : 0.0);
    }
  }

  return 0;
}

static char* qtToggleGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(qtToggleGetCheck(ih));
}

static int qtToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  /* Switch widgets do not have a title */
  if (iupAttribGetBoolean(ih, "SWITCH"))
    return 0;

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    QAbstractButton* button = (QAbstractButton*)ih->handle;

    if (button)
    {
      /* Check for MARKUP support */
      if (iupAttribGetBoolean(ih, "MARKUP"))
      {
        /* Qt buttons automatically render HTML/rich text */
        button->setText(QString::fromUtf8(value ? value : ""));
      }
      else
      {
        /* Process mnemonic and set text */
        char c = '&';
        char* str = iupStrProcessMnemonic(value, &c, 1);

        if (str)
        {
          button->setText(QString::fromUtf8(str));
          if (str != value)
            free(str);

          return 1;
        }
      }
    }
  }

  return 0;
}

static int qtToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return 0;  /* Text toggles don't support alignment */

  /* Store alignment for image toggles */
  iupAttribSet(ih, "ALIGNMENT", (char*)value);
  return 1;
}

static char* qtToggleGetAlignmentAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return NULL;

  char* value = iupAttribGet(ih, "ALIGNMENT");
  if (!value)
    return (char*)"ACENTER:ACENTER";

  return value;
}

static int qtToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    QAbstractButton* button = (QAbstractButton*)ih->handle;
    button->setContentsMargins(ih->data->horiz_padding, ih->data->vert_padding,
                               ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }

  return 1;
}

static int qtToggleSetSpacingAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    qtToggleUpdateLayout(ih);
  return 0;
}

static int qtToggleSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_TOGGLE_IMAGE)
    return 0;

  iupAttribSet(ih, "IMAGEPOSITION", (char*)value);

  if (ih->handle)
    qtToggleUpdateLayout(ih);

  return 1;
}

static int qtToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QAbstractButton* button = (QAbstractButton*)ih->handle;

  if (!button || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  /* Set text color via palette */
  QPalette palette = button->palette();
  palette.setColor(QPalette::WindowText, QColor(r, g, b));
  palette.setColor(QPalette::ButtonText, QColor(r, g, b));
  button->setPalette(palette);

  return 1;
}

static char* qtToggleGetBgColorAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
  {
    /* For image toggles, return dialog background */
    unsigned char r, g, b;
    char* color = iupBaseNativeParentGetBgColorAttrib(ih);
    if (iupStrToRGB(color, &r, &g, &b))
      return iupStrReturnRGB(r, g, b);
    return NULL;
  }
}

static int qtToggleSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    QAbstractButton* button = (QAbstractButton*)ih->handle;
    if (button)
      iupqtUpdateWidgetFont(ih, button);
  }

  return 1;
}

static int qtToggleSetMarkupAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    if (iupStrBoolean(value))
      iupAttribSet(ih, "MARKUP", "1");
    else
      iupAttribSet(ih, "MARKUP", NULL);

    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      qtToggleSetTitleAttrib(ih, title);

    return 0;
  }
  return 0;
}

static int qtToggleSetRightButtonAttrib(Ihandle* ih, const char* value)
{
  /* Qt checkboxes can be laid out with checkbox on right using setLayoutDirection */
  if (ih->data->type == IUP_TOGGLE_TEXT && ih->handle)
  {
    QAbstractButton* button = (QAbstractButton*)ih->handle;

    if (iupStrBoolean(value))
      button->setLayoutDirection(Qt::RightToLeft);
    else
      button->setLayoutDirection(Qt::LeftToRight);

    return 1;
  }
  return 0;
}

static int qtToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMAGE"))
      iupAttribSet(ih, "IMAGE", (char*)value);

    qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
    return 1;
  }

  return 0;
}

static int qtToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMINACTIVE"))
      iupAttribSet(ih, "IMINACTIVE", (char*)value);

    qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
    return 1;
  }

  return 0;
}

static int qtToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMPRESS"))
      iupAttribSet(ih, "IMPRESS", (char*)value);

    qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
    return 1;
  }

  return 0;
}

static int qtToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* Update inactive image if necessary */
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qtToggleUpdateImage(ih, iupStrBoolean(value), qtToggleGetCheck(ih));

  return iupBaseSetActiveAttrib(ih, value);
}

/****************************************************************************
 * Event Callbacks
 ****************************************************************************/

static void qtToggleToggled(Ihandle* ih, bool checked)
{
  /* Check if we should ignore this toggle event */
  if (iupAttribGet(ih, "_IUPQT_IGNORE_TOGGLE"))
    return;

  int check = checked ? 1 : 0;

  /* Update image if needed */
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qtToggleUpdateImage(ih, iupdrvIsActive(ih), check);

  /* Call ACTION callback */
  IFni cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, check) == IUP_CLOSE)
    IupExitLoop();

  /* Call VALUECHANGED_CB */
  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  QAbstractButton* button = nullptr;

  if (!ih->parent)
    return IUP_ERROR;

  /* Determine toggle type */
  char* value = iupAttribGet(ih, "IMAGE");
  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  /* Create appropriate widget */
  if (radio)
  {
    /* Radio button - use QToolButton for image toggles, QRadioButton for text */
    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      /* Image radio button uses QToolButton with auto-exclusive mode */
      IupQtToolButton* tool_btn = new IupQtToolButton(ih);
      tool_btn->setAutoExclusive(true);  /* Makes it behave like a radio button */
      button = tool_btn;
    }
    else
    {
      /* Text radio button uses QRadioButton */
      IupQtRadioButton* radio_btn = new IupQtRadioButton(ih);
      button = radio_btn;

      /* Link to radio group */
      Ihandle* last_toggle = (Ihandle*)iupAttribGet(radio, "_IUPQT_LASTRADIOBUTTON");
      if (last_toggle)
      {
        QRadioButton* last_btn = (QRadioButton*)last_toggle->handle;
        if (last_btn)
        {
          /* Qt automatically groups radio buttons in the same parent */
        }
      }
      else
      {
        /* This is the first radio button in the group */
        radio_btn->setChecked(true);
      }
      iupAttribSet(radio, "_IUPQT_LASTRADIOBUTTON", (char*)ih);
    }

    /* Make sure it has at least one name */
    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);

    ih->data->is_radio = 1;
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT)
    {
      /* Check if SWITCH control is requested */
      if (iupAttribGetBoolean(ih, "SWITCH"))
      {
        /* Create custom switch widget */
        IupQtSwitch* switch_widget = new IupQtSwitch(ih);
        button = switch_widget;
      }
      else
      {
        /* Regular checkbox */
        IupQtCheckBox* checkbox = new IupQtCheckBox(ih);
        button = checkbox;

        /* Enable 3-state if requested */
        if (iupAttribGetBoolean(ih, "3STATE"))
          checkbox->setTristate(true);
      }
    }
    else
    {
      /* Image toggle button */
      IupQtToolButton* tool_btn = new IupQtToolButton(ih);
      button = tool_btn;
    }
  }

  if (!button)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)button;

  /* Set initial title/image */
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      qtToggleSetTitleAttrib(ih, title);
  }
  else
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      qtToggleUpdateImage(ih, 1, 0);
  }

  /* Add to parent */
  iupqtAddToParent(ih);

  /* Configure focus */
  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(button, 0);

  /* Configure FLAT style for image toggles */
  if (ih->data->type == IUP_TOGGLE_IMAGE && iupAttribGetBoolean(ih, "FLAT"))
  {
    ih->data->flat = 1;
    QToolButton* tool_btn = qobject_cast<QToolButton*>(button);
    if (tool_btn)
      tool_btn->setAutoRaise(true);
  }

  /* Connect toggled signal */
  QObject::connect(button, &QAbstractButton::toggled, [ih](bool checked) {
    qtToggleToggled(ih, checked);
  });

  /* Set padding if specified */
  value = iupAttribGet(ih, "PADDING");
  if (value)
    qtToggleSetPaddingAttrib(ih, value);

  /* Set RIGHTBUTTON if specified */
  if (iupAttribGetBoolean(ih, "RIGHTBUTTON"))
    qtToggleSetRightButtonAttrib(ih, "YES");

  /* Set SPACING if specified */
  value = iupAttribGet(ih, "SPACING");
  if (value)
    qtToggleSetSpacingAttrib(ih, value);

  /* Set IMAGEPOSITION if specified */
  value = iupAttribGet(ih, "IMAGEPOSITION");
  if (value)
    qtToggleSetImagePositionAttrib(ih, value);

  /* Update mnemonic */
  iupqtUpdateMnemonic(ih);

  /* Initialize thumb position for switch widgets after all setup is complete */
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    /* Safe to static_cast since we know it's a switch widget based on SWITCH attribute */
    IupQtSwitch* switch_widget = static_cast<IupQtSwitch*>(button);
    if (switch_widget)
      switch_widget->initializeThumbPosition();
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtToggleMapMethod;

  /* Driver Dependent Attribute functions */

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, qtToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, qtToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", qtToggleGetBgColorAttrib, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, qtToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, qtToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", qtToggleGetAlignmentAttrib, qtToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, qtToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, qtToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, qtToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qtToggleGetValueAttrib, qtToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, qtToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* Additional attributes */
  iupClassRegisterAttribute(ic, "SPACING", NULL, qtToggleSetSpacingAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", NULL, qtToggleSetImagePositionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, qtToggleSetMarkupAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, qtToggleSetRightButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IGNOREDOUBLECLICK", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Only for QCheckBox with 3STATE */
  iupClassRegisterAttribute(ic, "3STATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
