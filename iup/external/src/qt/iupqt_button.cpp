/** \file
 * \brief Button Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QString>
#include <QStyleOption>
#include <QFrame>
#include <QPainter>
#include <QStylePainter>
#include <QFontMetrics>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Custom Qt Button with Event Handling
 ****************************************************************************/

class IupQtButton : public QPushButton
{
public:
  Ihandle* iup_handle;

  IupQtButton(const QString& text, Ihandle* ih)
    : QPushButton(text), iup_handle(ih)
  {
  }

  IupQtButton(Ihandle* ih)
    : QPushButton(), iup_handle(ih)
  {
  }

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    QPushButton::enterEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);

    /* Handle FLAT mode - show relief on hover */
    if (iupAttribGetBoolean(iup_handle, "FLAT"))
      setFlat(false);
  }

  void leaveEvent(QEvent* event) override
  {
    QPushButton::leaveEvent(event);
    iupqtEnterLeaveEvent(this, event, iup_handle);

    /* Handle FLAT mode - hide relief when not hovering */
    if (iupAttribGetBoolean(iup_handle, "FLAT"))
      setFlat(true);
  }

  void focusInEvent(QFocusEvent* event) override
  {
    QPushButton::focusInEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void focusOutEvent(QFocusEvent* event) override
  {
    QPushButton::focusOutEvent(event);
    iupqtFocusInOutEvent(this, event, iup_handle);
  }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      click();
      event->accept();
      return;
    }

    if (iupqtKeyPressEvent(this, event, iup_handle))
    {
      event->accept();
      return;
    }
    QPushButton::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    QPushButton::mousePressEvent(event);

    if (iup_handle->data->type & IUP_BUTTON_IMAGE)
    {
      char* name = iupAttribGet(iup_handle, "IMPRESS");
      if (name)
        qtButtonSetPixmap(iup_handle, name, 0);
    }

    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QPushButton::mouseReleaseEvent(event);
    if (iup_handle->data->type & IUP_BUTTON_IMAGE)
    {
      char* name = iupAttribGet(iup_handle, "IMPRESS");
      if (name)
      {
        name = iupAttribGet(iup_handle, "IMAGE");
        if (name)
          qtButtonSetPixmap(iup_handle, name, 0);
      }
    }

    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void paintEvent(QPaintEvent* event) override
  {
    char* bgcolor = iupAttribGet(iup_handle, "BGCOLOR");
    if (bgcolor && iup_handle->data->type == IUP_BUTTON_TEXT && !iupAttribGet(iup_handle, "TITLE"))
    {
      unsigned char r, g, b;
      if (iupStrToRGB(bgcolor, &r, &g, &b))
      {
        QStylePainter p(this);
        QStyleOptionButton option;
        initStyleOption(&option);

        /* Draw button frame/border */
        p.drawControl(QStyle::CE_PushButtonBevel, option);

        /* Get content rect inside the button */
        QRect content_rect = style()->subElementRect(QStyle::SE_PushButtonContents, &option, this);

        /* Fill content area with background color */
        p.fillRect(content_rect, QColor(r, g, b));

        /* Draw focus rect if needed */
        if (option.state & QStyle::State_HasFocus)
        {
          QStyleOptionFocusRect focus_opt;
          focus_opt.QStyleOption::operator=(option);
          focus_opt.rect = style()->subElementRect(QStyle::SE_PushButtonFocusRect, &option, this);
          p.drawPrimitive(QStyle::PE_FrameFocusRect, focus_opt);
        }
        return;
      }
    }

    /* For image-only buttons, draw icon centered manually to ensure cross-platform consistency */
    if (iup_handle->data->type == IUP_BUTTON_IMAGE)
    {
      QStylePainter p(this);
      QStyleOptionButton option;
      initStyleOption(&option);

      /* Draw button frame/border */
      p.drawControl(QStyle::CE_PushButtonBevel, option);

      /* Get content rect inside the button */
      QRect content_rect = style()->subElementRect(QStyle::SE_PushButtonContents, &option, this);

      /* Draw icon in content area respecting alignment */
      if (!icon().isNull())
      {
        QIcon::Mode mode = QIcon::Normal;
        if (!(option.state & QStyle::State_Enabled))
          mode = QIcon::Disabled;
        else if (option.state & QStyle::State_MouseOver)
          mode = QIcon::Active;

        QIcon::State state = (option.state & QStyle::State_On) ? QIcon::On : QIcon::Off;

        QSize icon_size = iconSize();
        QPixmap pixmap = icon().pixmap(icon_size, mode, state);

        int xpad = iup_handle->data->horiz_padding;
        int ypad = iup_handle->data->vert_padding;

        /* Calculate position based on alignment */
        int x, y;
        if (iup_handle->data->horiz_alignment == IUP_ALIGN_ARIGHT)
          x = content_rect.right() - pixmap.width() - xpad;
        else if (iup_handle->data->horiz_alignment == IUP_ALIGN_ALEFT)
          x = content_rect.x() + xpad;
        else /* ACENTER */
          x = content_rect.x() + (content_rect.width() - pixmap.width()) / 2;

        if (iup_handle->data->vert_alignment == IUP_ALIGN_ABOTTOM)
          y = content_rect.bottom() - pixmap.height() - ypad;
        else if (iup_handle->data->vert_alignment == IUP_ALIGN_ATOP)
          y = content_rect.y() + ypad;
        else /* ACENTER */
          y = content_rect.y() + (content_rect.height() - pixmap.height()) / 2;

        /* Apply button shift when pressed */
        if (option.state & (QStyle::State_On | QStyle::State_Sunken))
        {
          x += style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, &option, this);
          y += style()->pixelMetric(QStyle::PM_ButtonShiftVertical, &option, this);
        }

        p.drawPixmap(x, y, pixmap);
      }

      /* Draw focus rect if needed */
      if (option.state & QStyle::State_HasFocus)
      {
        QStyleOptionFocusRect focus_opt;
        focus_opt.QStyleOption::operator=(option);
        focus_opt.rect = style()->subElementRect(QStyle::SE_PushButtonFocusRect, &option, this);
        p.drawPrimitive(QStyle::PE_FrameFocusRect, focus_opt);
      }
      return;
    }

    QPushButton::paintEvent(event);
  }

public:
  static void qtButtonSetPixmap(Ihandle* ih, const char* name, int make_inactive);
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

void IupQtButton::qtButtonSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  if (!name)
    return;

  IupQtButton* button = (IupQtButton*)ih->handle;

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

static void qtButtonUpdateLayout(Ihandle* ih)
{
  IupQtButton* button = (IupQtButton*)ih->handle;
  if (!button)
    return;

  if (ih->data->type & IUP_BUTTON_IMAGE && ih->data->type & IUP_BUTTON_TEXT)
  {
    switch (ih->data->img_position)
    {
      case IUP_IMGPOS_LEFT:
        button->setLayoutDirection(Qt::LeftToRight);
        break;
      case IUP_IMGPOS_RIGHT:
        button->setLayoutDirection(Qt::RightToLeft);
        break;
      case IUP_IMGPOS_TOP:
      case IUP_IMGPOS_BOTTOM:
        /* Qt doesn't directly support top/bottom positioning
         * Would need custom layout - for now use default */
        button->setLayoutDirection(Qt::LeftToRight);
        break;
    }

    int spacing = ih->data->spacing;
    if (spacing >= 0)
    {
      QString style = QString("QPushButton { icon-size: %1px; }").arg(spacing);
      /* QPushButton doesn't have direct icon-text spacing control */
    }
  }
}

/****************************************************************************
 * Border Size Calculation
 ****************************************************************************/

static int qt_button_padding_text_x = -1;
static int qt_button_padding_text_y = -1;
static int qt_button_padding_image_x = -1;
static int qt_button_padding_image_y = -1;
static int qt_button_padding_both_x = -1;
static int qt_button_padding_both_y = -1;
static int qt_button_padding_measured = 0;

static void qtButtonMeasurePadding(void)
{
  /* Measure text-only button padding.
     Must use a long string to exceed Qt's minimum button width,
     otherwise we measure the minimum width, not the actual padding. */
  {
    const char* test_text = "This is a long test string for measuring";
    QPushButton temp_button(test_text);
    QSize button_size = temp_button.sizeHint();
    QFontMetrics fm(temp_button.font());
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int text_w = fm.horizontalAdvance(test_text);
#else
    int text_w = fm.width(test_text);
#endif
    int text_h = fm.height();

    qt_button_padding_text_x = button_size.width() - text_w;
    qt_button_padding_text_y = button_size.height() - text_h;
  }

  /* Measure image-only button padding using large icon to exceed minimum */
  {
    QPushButton temp_button;
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    temp_button.setIcon(QIcon(pixmap));
    temp_button.setIconSize(QSize(64, 64));
    QSize button_size = temp_button.sizeHint();

    qt_button_padding_image_x = button_size.width() - 64;
    qt_button_padding_image_y = button_size.height() - 64;
  }

  /* Image+text button uses same padding as text-only */
  qt_button_padding_both_x = qt_button_padding_text_x;
  qt_button_padding_both_y = qt_button_padding_text_y;

  qt_button_padding_measured = 1;
}

extern "C" void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  int has_image = 0;
  int has_text = 0;
  int has_bgcolor = 0;

  if (!qt_button_padding_measured)
    qtButtonMeasurePadding();

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    has_image = (image != NULL);
    has_text = (title != NULL && *title != 0);
    has_bgcolor = (!has_image && !has_text && bgcolor != NULL);
  }

  if (has_bgcolor)
  {
    /* Use empty button's natural size */
    QPushButton temp_button;
    QSize hint = temp_button.sizeHint();
    (*x) += hint.width();
    (*y) += hint.height();
  }
  else if (has_image && has_text)
  {
    (*x) += qt_button_padding_both_x;
    (*y) += qt_button_padding_both_y;
  }
  else if (has_image)
  {
    (*x) += qt_button_padding_image_x;
    (*y) += qt_button_padding_image_y;
  }
  else
  {
    (*x) += qt_button_padding_text_x;
    (*y) += qt_button_padding_text_y;
  }
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    IupQtButton* button = (IupQtButton*)ih->handle;

    if (button)
    {
      /* Process mnemonic and set text */
      char c = '&';
      char* str = iupStrProcessMnemonic(value, &c, 1);

      if (str)
      {
        button->setText(QString::fromUtf8(str));
        if (str != value)
          free(str);

        qtButtonUpdateLayout(ih);
        return 1;
      }
    }
  }

  return 0;
}

static char* qtButtonGetAlignmentAttrib(Ihandle* ih)
{
  char* horiz_align2str[3] = {(char*)"ALEFT", (char*)"ACENTER", (char*)"ARIGHT"};
  char* vert_align2str[3] = {(char*)"ATOP", (char*)"ACENTER", (char*)"ABOTTOM"};
  return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment],
                                   vert_align2str[ih->data->vert_alignment]);
}

static int qtButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];
  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
  else
    ih->data->horiz_alignment = IUP_ALIGN_ACENTER;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  if (ih->handle)
  {
    IupQtButton* button = (IupQtButton*)ih->handle;

    QString align_style;
    if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
      align_style = "text-align: right; padding-right: 5px;";
    else if (ih->data->horiz_alignment == IUP_ALIGN_ALEFT)
      align_style = "text-align: left; padding-left: 5px;";
    else
      align_style = "text-align: center;";
  }

  return 1;
}

static char* qtButtonGetSpacingAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->spacing);
}

static int qtButtonSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->spacing = 2;
  else
    iupStrToInt(value, &ih->data->spacing);

  if (ih->handle)
    qtButtonUpdateLayout(ih);

  return 0;
}

static char* qtButtonGetImagePositionAttrib(Ihandle* ih)
{
  if (ih->data->img_position == IUP_IMGPOS_RIGHT)
    return const_cast<char*>("RIGHT");
  else if (ih->data->img_position == IUP_IMGPOS_TOP)
    return const_cast<char*>("TOP");
  else if (ih->data->img_position == IUP_IMGPOS_BOTTOM)
    return const_cast<char*>("BOTTOM");
  else
    return const_cast<char*>("LEFT");
}

static int qtButtonSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->img_position = IUP_IMGPOS_LEFT;
  else if (iupStrEqualNoCase(value, "LEFT"))
    ih->data->img_position = IUP_IMGPOS_LEFT;
  else if (iupStrEqualNoCase(value, "RIGHT"))
    ih->data->img_position = IUP_IMGPOS_RIGHT;
  else if (iupStrEqualNoCase(value, "TOP"))
    ih->data->img_position = IUP_IMGPOS_TOP;
  else if (iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->img_position = IUP_IMGPOS_BOTTOM;

  if (ih->handle)
    qtButtonUpdateLayout(ih);

  return 0;
}

static int qtButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    IupQtButton* button = (IupQtButton*)ih->handle;
    button->setContentsMargins(ih->data->horiz_padding, ih->data->vert_padding,
                               ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static char* qtButtonGetBgColorAttrib(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE && !iupAttribGet(ih, "IMPRESS"))
  {
    return iupBaseNativeParentGetBgColorAttrib(ih);
  }

  if (iupAttribGet(ih, "IMPRESS"))
    return iupBaseNativeParentGetBgColorAttrib(ih);

  return nullptr;
}

static int qtButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!value)
  {
    if (ih->handle)
      iupdrvPostRedraw(ih);
    return 1;
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->handle)
    iupdrvPostRedraw(ih);

  return 1;
}

static int qtButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtButton* button = (IupQtButton*)ih->handle;

  if (button)
  {
    QPalette palette = button->palette();
    palette.setColor(QPalette::ButtonText, QColor(r, g, b));
    button->setPalette(palette);
    return 1;
  }

  return 0;
}

static int qtButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    IupQtButton* button = (IupQtButton*)ih->handle;
    if (button)
      iupqtUpdateWidgetFont(ih, button);
  }

  return 1;
}

static int qtButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (iupdrvIsActive(ih))
      IupQtButton::qtButtonSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        /* If not active and IMINACTIVE not defined, create inactive version */
        IupQtButton::qtButtonSetPixmap(ih, value, 1);
      }
    }

    qtButtonUpdateLayout(ih);
    return 1;
  }

  return 0;
}

static int qtButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        IupQtButton::qtButtonSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        IupQtButton::qtButtonSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }

  return 0;
}

static int qtButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    /* Update flat state if IMAGE+IMPRESS without IMPRESSBORDER */
    if (ih->handle)
    {
      IupQtButton* button = (IupQtButton*)ih->handle;
      int should_be_flat = iupAttribGetBoolean(ih, "FLAT");
      if (!should_be_flat &&
          iupAttribGet(ih, "IMPRESS") &&
          !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
      {
        should_be_flat = 1;
      }
      button->setFlat(should_be_flat);
    }

    /* Trigger redraw to update IMPRESS image */
    iupdrvPostRedraw(ih);
    return 1;
  }

  return 0;
}

static int qtButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        IupQtButton::qtButtonSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        IupQtButton::qtButtonSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      IupQtButton::qtButtonSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int qtButtonSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtButton* button = (IupQtButton*)ih->handle;

    int should_be_flat = iupStrBoolean(value);
    if (!should_be_flat &&
        ih->data->type == IUP_BUTTON_IMAGE &&
        iupAttribGet(ih, "IMPRESS") &&
        !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    {
      should_be_flat = 1;
    }

    button->setFlat(should_be_flat);
    return 0;
  }
  return 1;
}

/****************************************************************************
 * Event Callbacks
 ****************************************************************************/

static void qtButtonClicked(Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

/****************************************************************************
 * Map/UnMap Methods
 ****************************************************************************/

static void qtButtonLayoutUpdateMethod(Ihandle *ih)
{
  IupQtButton* button = (IupQtButton*)ih->handle;
  if (!button)
    return;

  iupdrvBaseLayoutUpdateMethod(ih);
  qtButtonUpdateLayout(ih);
}

static int qtButtonMapMethod(Ihandle* ih)
{
  IupQtButton* button;

  char* value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    value = iupAttribGet(ih, "TITLE");
    if (value && *value != 0)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  char* title = iupAttribGet(ih, "TITLE");
  if (title)
    button = new IupQtButton(QString::fromUtf8(title), ih);
  else
    button = new IupQtButton(ih);

  if (!button)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)button;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      IupQtButton::qtButtonSetPixmap(ih, image, 0);
  }

  if (!title && iupAttribGet(ih, "BGCOLOR"))
  {
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    qtButtonSetBgColorAttrib(ih, bgcolor);
  }

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(button, 0);

  if (iupAttribGetBoolean(ih, "FLAT"))
    button->setFlat(true);

  /* Also set flat for IMAGE+IMPRESS buttons without IMPRESSBORDER */
  if (ih->data->type == IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
  {
    button->setFlat(true);
  }

  QObject::connect(button, &QPushButton::clicked, [ih]() {
    qtButtonClicked(ih);
  });

  value = iupAttribGet(ih, "PADDING");
  if (value)
    qtButtonSetPaddingAttrib(ih, value);

  value = iupAttribGet(ih, "SPACING");
  if (value)
    qtButtonSetSpacingAttrib(ih, value);

  value = iupAttribGet(ih, "IMAGEPOSITION");
  if (value)
    qtButtonSetImagePositionAttrib(ih, value);

  iupqtUpdateMnemonic(ih);
  qtButtonUpdateLayout(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = qtButtonMapMethod;
  ic->LayoutUpdate = qtButtonLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, qtButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, qtButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", qtButtonGetBgColorAttrib, qtButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, qtButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, qtButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", qtButtonGetAlignmentAttrib, qtButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, qtButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, qtButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, qtButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, qtButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SPACING", qtButtonGetSpacingAttrib, qtButtonSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "2", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", qtButtonGetImagePositionAttrib, qtButtonSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "FLAT", NULL, qtButtonSetFlatAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}
