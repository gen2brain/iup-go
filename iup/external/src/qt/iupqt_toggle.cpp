/** \file
 * \brief Toggle Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QCheckBox>
#include <QRadioButton>
#include <QToolButton>
#include <QAbstractButton>
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
#include <QStylePainter>
#include <QTextDocument>
#include <QTimer>
#include <QElapsedTimer>

#include <cstdlib>
#include <type_traits>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_toggle.h"
#include "iup_markup.h"
}

#include "iupqt_drv.h"

/* Image position constants (from iup_button.h to avoid struct redefinition) */
enum{IUP_IMGPOS_LEFT, IUP_IMGPOS_RIGHT, IUP_IMGPOS_TOP, IUP_IMGPOS_BOTTOM};

/****************************************************************************
 * Custom Qt Toggle Widgets with Event Handling
 ****************************************************************************/

template<typename BaseWidget>
class IupQtToggleBase : public BaseWidget
{
protected:
  Ihandle* iup_handle;
  bool last_was_double_click;
  QString markup_html;

public:
  explicit IupQtToggleBase(Ihandle* ih) : BaseWidget(), iup_handle(ih), last_was_double_click(false) {}

  void setMarkupHtml(const QString& html) { markup_html = html; }
  void clearMarkupHtml() { markup_html.clear(); }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    BaseWidget::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);

    if constexpr (std::is_same<BaseWidget, QToolButton>::value)
    {
      if (iup_handle->data->type == IUP_TOGGLE_IMAGE && iup_handle->data->flat)
      {
        this->setAutoRaise(false);
      }
    }
  }

  void leaveEvent(QEvent* event) override
  {
    BaseWidget::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);

    if constexpr (std::is_same<BaseWidget, QToolButton>::value)
    {
      if (iup_handle->data->type == IUP_TOGGLE_IMAGE && iup_handle->data->flat)
      {
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

  void paintEvent(QPaintEvent* event) override
  {
    if constexpr (std::is_same_v<BaseWidget, QCheckBox> || std::is_same_v<BaseWidget, QRadioButton>)
    {
      if (markup_html.isEmpty())
      {
        BaseWidget::paintEvent(event);
        return;
      }

      QStylePainter p(this);
      QStyleOptionButton opt;
      this->initStyleOption(&opt);

      constexpr bool isRadio = std::is_same_v<BaseWidget, QRadioButton>;

      QStyleOptionButton subopt = opt;
      subopt.rect = this->style()->subElementRect(
        isRadio ? QStyle::SE_RadioButtonIndicator : QStyle::SE_CheckBoxIndicator, &opt, this);
      p.drawPrimitive(
        isRadio ? QStyle::PE_IndicatorRadioButton : QStyle::PE_IndicatorCheckBox, subopt);

      QRect contentsRect = this->style()->subElementRect(
        isRadio ? QStyle::SE_RadioButtonContents : QStyle::SE_CheckBoxContents, &opt, this);

      QTextDocument doc;
      doc.setHtml(markup_html);
      doc.setDefaultFont(this->font());
      doc.setTextWidth(contentsRect.width());

      p.save();
      p.translate(contentsRect.topLeft());
      qreal textHeight = doc.size().height();
      if (textHeight < contentsRect.height())
        p.translate(0, (contentsRect.height() - textHeight) / 2.0);
      doc.drawContents(&p);
      p.restore();

      if (opt.state & QStyle::State_HasFocus)
      {
        QStyleOptionFocusRect fropt;
        fropt.QStyleOption::operator=(opt);
        fropt.rect = this->style()->subElementRect(
          isRadio ? QStyle::SE_RadioButtonFocusRect : QStyle::SE_CheckBoxFocusRect, &opt, this);
        p.drawPrimitive(QStyle::PE_FrameFocusRect, fropt);
      }
    }
    else if constexpr (!std::is_same_v<BaseWidget, QAbstractButton>)
    {
      BaseWidget::paintEvent(event);
    }
  }
};

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

  static constexpr int TRACK_WIDTH = 50;
  static constexpr int TRACK_HEIGHT = 26;
  static constexpr int THUMB_SIZE = 22;
  static constexpr int THUMB_MARGIN = 2;

  static qreal easeInOutQuad(qreal t)
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

    animation_timer = new QTimer(this);
    animation_timer->setInterval(16);  /* ~60 FPS */

    connect(animation_timer, &QTimer::timeout, this, [this]() {
      qint64 elapsed = elapsed_timer.elapsed();

      if (elapsed >= animation_duration)
      {
        thumb_position = animation_end;
        animation_timer->stop();
      }
      else
      {
        qreal t = (qreal)elapsed / (qreal)animation_duration;
        qreal eased_t = easeInOutQuad(t);
        thumb_position = animation_start + (animation_end - animation_start) * eased_t;
      }

      update();
    });

    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
      animation_start = thumb_position;
      animation_end = checked ? 1.0 : 0.0;

      elapsed_timer.start();
      animation_timer->start();
    });
  }

  void initializeThumbPosition()
  {
    thumb_position = isChecked() ? 1.0 : 0.0;
    update();
  }

  ~IupQtSwitch() override
  {
    if (animation_timer)
    {
      animation_timer->stop();
      delete animation_timer;
    }
  }

  void setThumbPosition(qreal pos)
  {
    thumb_position = pos;
    update();
  }

  QSize sizeHint() const override
  {
    return QSize(TRACK_WIDTH, TRACK_HEIGHT);
  }

  QSize minimumSizeHint() const override
  {
    return QSize(TRACK_WIDTH, TRACK_HEIGHT);
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    IupQtToggleBase<QAbstractButton>::enterEvent(event);
    is_hovered = true;
    update();
  }

  void leaveEvent(QEvent* event) override
  {
    IupQtToggleBase<QAbstractButton>::leaveEvent(event);
    is_hovered = false;
    update();
  }

  void paintEvent(QPaintEvent* event) override
  {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPalette pal = palette();
    QColor track_color, thumb_color, border_color;

    if (!isEnabled())
    {
      track_color = pal.color(QPalette::Disabled, QPalette::Mid);
      thumb_color = pal.color(QPalette::Disabled, QPalette::Base);
      border_color = pal.color(QPalette::Disabled, QPalette::Dark);
    }
    else
    {
      QColor track_off_color = pal.color(QPalette::Active, QPalette::Mid);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
      /* macOS sets Accent to controlAccentColor; a near-white Accent means it was never set */
      QColor accent = pal.color(QPalette::Active, QPalette::Accent);
      QColor track_on_color = (accent.lightness() > 250) ? pal.color(QPalette::Active, QPalette::Highlight) : accent;
#else
      QColor track_on_color = pal.color(QPalette::Active, QPalette::Highlight);
#endif

      track_color = QColor(
        track_off_color.red() + (track_on_color.red() - track_off_color.red()) * thumb_position,
        track_off_color.green() + (track_on_color.green() - track_off_color.green()) * thumb_position,
        track_off_color.blue() + (track_on_color.blue() - track_off_color.blue()) * thumb_position
      );

      thumb_color = pal.color(QPalette::Active, QPalette::Base);
      border_color = track_color.darker(120);
    }

    QRectF track_rect(0.5, 0.5, TRACK_WIDTH - 1, TRACK_HEIGHT - 1);
    qreal radius = (TRACK_HEIGHT - 1) / 2.0;
    painter.setBrush(track_color);
    painter.setPen(QPen(border_color, 1));
    painter.drawRoundedRect(track_rect, radius, radius);

    int thumb_x_min = THUMB_MARGIN;
    int thumb_x_max = TRACK_WIDTH - THUMB_SIZE - THUMB_MARGIN;
    int thumb_x = thumb_x_min + (thumb_x_max - thumb_x_min) * thumb_position;
    int thumb_y = (TRACK_HEIGHT - THUMB_SIZE) / 2;

    if (isEnabled())
    {
      painter.setBrush(QColor(0, 0, 0, 30));
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(thumb_x + 1, thumb_y + 1, THUMB_SIZE, THUMB_SIZE);
    }

    QColor thumb_border = border_color.lighter(110);
    if (is_hovered && isEnabled())
    {
      thumb_border = track_color.lighter(130);
    }
    painter.setBrush(thumb_color);
    painter.setPen(QPen(thumb_border, is_hovered ? 2 : 1));
    painter.drawEllipse(thumb_x, thumb_y, THUMB_SIZE, THUMB_SIZE);
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

#define IUP_TOGGLE_BOX 18  /* Qt checkbox/radio size */


static int qt_toggle_border_x = -1;
static int qt_toggle_border_y = -1;
static int qt_toggle_struct_x = 0;
static int qt_toggle_struct_y = 0;

static void qtToggleMeasureBorders(void)
{
  QToolButton temp_button;
  QPixmap pixmap(64, 64);
  pixmap.fill(Qt::transparent);
  temp_button.setIcon(QIcon(pixmap));
  temp_button.setIconSize(QSize(64, 64));
  QSize button_size = temp_button.sizeHint();

  qt_toggle_border_x = button_size.width() - 64;
  qt_toggle_border_y = button_size.height() - 64;
  if (qt_toggle_border_x < 0) qt_toggle_border_x = 0;
  if (qt_toggle_border_y < 0) qt_toggle_border_y = 0;

  /* Structural border (frame only) for when user sets explicit PADDING */
  QStyleOptionToolButton opt;
  opt.initFrom(&temp_button);
  opt.rect = QRect(0, 0, button_size.width(), button_size.height());
  QStyle* style = temp_button.style();
  QRect contentRect = style->subControlRect(QStyle::CC_ToolButton, &opt, QStyle::SC_ToolButton, &temp_button);
  qt_toggle_struct_x = button_size.width() - contentRect.width();
  qt_toggle_struct_y = button_size.height() - contentRect.height();
  if (qt_toggle_struct_x < 0) qt_toggle_struct_x = 0;
  if (qt_toggle_struct_y < 0) qt_toggle_struct_y = 0;
}

extern "C" IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  if (ih && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupdrvButtonAddBorders(ih, x, y);
    return;
  }

  int has_user_padding = 0;

  if (qt_toggle_border_x < 0)
    qtToggleMeasureBorders();

  if (ih)
  {
    int horiz_padding = 0, vert_padding = 0;
    char* padding = IupGetAttribute(ih, "PADDING");
    if (padding)
      iupStrToIntInt(padding, &horiz_padding, &vert_padding, 'x');
    has_user_padding = (horiz_padding > 0 || vert_padding > 0);
  }

  if (has_user_padding)
  {
    (*x) += qt_toggle_struct_x;
    (*y) += qt_toggle_struct_y;
  }
  else
  {
    (*x) += qt_toggle_border_x;
    (*y) += qt_toggle_border_y;
  }
}

extern "C" IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* str)
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

extern "C" IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
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
      /* Qt has no top/bottom image placement */
      break;
  }

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

  button->blockSignals(true);

  if (iupStrEqualNoCase(value, "NOTDEF"))
  {
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(button);
    if (checkbox && checkbox->isTristate())
      checkbox->setCheckState(Qt::PartiallyChecked);
  }
  else if (iupStrEqualNoCase(value, "TOGGLE"))
  {
    button->setChecked(!button->isChecked());

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
  }
  else
  {
    int check = iupStrBoolean(value);
    button->setChecked(check);

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      qtToggleUpdateImage(ih, iupdrvIsActive(ih), qtToggleGetCheck(ih));
  }

  button->blockSignals(false);

  if (ih->data->type == IUP_TOGGLE_TEXT && !ih->data->is_radio && iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupQtSwitch* switch_widget = static_cast<IupQtSwitch*>(button);
    switch_widget->setThumbPosition(button->isChecked() ? 1.0 : 0.0);
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
      if (iupAttribGetBoolean(ih, "MARKUP"))
      {
        char* html = iupMarkupToHtml(value ? value : "");
        char* stripped = iupMarkupStripTags(value ? value : "");

        button->setText(QString::fromUtf8(stripped));
        free(stripped);

        IupQtCheckBox* cb = dynamic_cast<IupQtCheckBox*>(button);
        IupQtRadioButton* rb = dynamic_cast<IupQtRadioButton*>(button);
        if (cb)
          cb->setMarkupHtml(QString::fromUtf8(html));
        else if (rb)
          rb->setMarkupHtml(QString::fromUtf8(html));

        free(html);
        return 1;
      }
      else
      {
        if (dynamic_cast<IupQtCheckBox*>(button))
          static_cast<IupQtCheckBox*>(button)->clearMarkupHtml();
        else if (dynamic_cast<IupQtRadioButton*>(button))
          static_cast<IupQtRadioButton*>(button)->clearMarkupHtml();

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

static int qtToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    QAbstractButton* button = (QAbstractButton*)ih->handle;

    if (ih->data->horiz_padding > 0 || ih->data->vert_padding > 0)
    {
      button->setStyleSheet(
          QString("QToolButton { padding: %1px %2px; min-width: 0; min-height: 0; }")
              .arg(ih->data->vert_padding)
              .arg(ih->data->horiz_padding));
    }
    else
    {
      button->setStyleSheet(QString());
    }
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

  /* Set text color via palette; the check/radio label is drawn from Text */
  QPalette palette = button->palette();
  palette.setColor(QPalette::WindowText, QColor(r, g, b));
  palette.setColor(QPalette::ButtonText, QColor(r, g, b));
  palette.setColor(QPalette::Text, QColor(r, g, b));
  button->setPalette(palette);

  return 1;
}

static char* qtToggleGetBgColorAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
  {
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
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qtToggleUpdateImage(ih, iupStrBoolean(value), qtToggleGetCheck(ih));

  return iupBaseSetActiveAttrib(ih, value);
}

/****************************************************************************
 * Event Callbacks
 ****************************************************************************/

static void qtToggleToggled(Ihandle* ih, bool checked)
{
  if (iupAttribGet(ih, "_IUPQT_IGNORE_TOGGLE"))
    return;

  int check = checked ? 1 : 0;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qtToggleUpdateImage(ih, iupdrvIsActive(ih), check);

  IFni cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, check) == IUP_CLOSE)
    IupExitLoop();

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

  char* value = iupAttribGet(ih, "IMAGE");
  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  if (radio)
  {
    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      IupQtToolButton* tool_btn = new IupQtToolButton(ih);
      tool_btn->setAutoExclusive(true);
      button = tool_btn;
    }
    else
    {
      IupQtRadioButton* radio_btn = new IupQtRadioButton(ih);
      button = radio_btn;

      Ihandle* last_toggle = (Ihandle*)iupAttribGet(radio, "_IUPQT_LASTRADIOBUTTON");
      if (!last_toggle)
        radio_btn->setChecked(true);
      iupAttribSet(radio, "_IUPQT_LASTRADIOBUTTON", (char*)ih);
    }

    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);

    ih->data->is_radio = 1;
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT)
    {
      if (iupAttribGetBoolean(ih, "SWITCH"))
      {
        IupQtSwitch* switch_widget = new IupQtSwitch(ih);
        button = switch_widget;
      }
      else
      {
        IupQtCheckBox* checkbox = new IupQtCheckBox(ih);
        button = checkbox;

        if (iupAttribGetBoolean(ih, "3STATE"))
          checkbox->setTristate(true);
      }
    }
    else
    {
      IupQtToolButton* tool_btn = new IupQtToolButton(ih);
      button = tool_btn;
    }
  }

  ih->handle = (InativeHandle*)button;

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

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(button, 0);

  if (ih->data->type == IUP_TOGGLE_IMAGE && iupAttribGetBoolean(ih, "FLAT"))
  {
    ih->data->flat = 1;
    QToolButton* tool_btn = qobject_cast<QToolButton*>(button);
    if (tool_btn)
      tool_btn->setAutoRaise(true);
  }

  QObject::connect(button, &QAbstractButton::toggled, [ih](bool checked) {
    qtToggleToggled(ih, checked);
  });

  value = iupAttribGet(ih, "PADDING");
  if (value)
    qtToggleSetPaddingAttrib(ih, value);

  if (iupAttribGetBoolean(ih, "RIGHTBUTTON"))
    qtToggleSetRightButtonAttrib(ih, "YES");

  value = iupAttribGet(ih, "SPACING");
  if (value)
    qtToggleSetSpacingAttrib(ih, value);

  value = iupAttribGet(ih, "IMAGEPOSITION");
  if (value)
    qtToggleSetImagePositionAttrib(ih, value);

  iupqtUpdateMnemonic(ih);

  if (ih->data->type == IUP_TOGGLE_TEXT && !ih->data->is_radio && iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupQtSwitch* switch_widget = static_cast<IupQtSwitch*>(button);
    switch_widget->initializeThumbPosition();
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
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
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, NULL, "ACENTER:ACENTER", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, qtToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, qtToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, qtToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qtToggleGetValueAttrib, qtToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, qtToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "SPACING", NULL, qtToggleSetSpacingAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", NULL, qtToggleSetImagePositionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, qtToggleSetMarkupAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, qtToggleSetRightButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IGNOREDOUBLECLICK", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "3STATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
