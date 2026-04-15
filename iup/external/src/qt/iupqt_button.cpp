/** \file
 * \brief Button Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include <QIcon>
#include <QWidget>
#include <QKeyEvent>
#include <QString>

#include <cstdlib>
#include <cstdarg>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_markup.h"
}

#include "iupqt_drv.h"


static void qtButtonSetPixmap(Ihandle* ih, const char* name, int make_inactive);

class IupQtButton : public QPushButton
{
public:
  Ihandle* iup_handle;

  IupQtButton(const QString& text, Ihandle* ih) : QPushButton(text), iup_handle(ih) {}
  IupQtButton(Ihandle* ih) : QPushButton(), iup_handle(ih) {}

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  { QPushButton::enterEvent(event); iupqtEnterLeaveEvent(this, event, iup_handle); }
  void leaveEvent(QEvent* event) override
  { QPushButton::leaveEvent(event); iupqtEnterLeaveEvent(this, event, iup_handle); }
  void focusInEvent(QFocusEvent* event) override
  { QPushButton::focusInEvent(event); iupqtFocusInOutEvent(this, event, iup_handle); }
  void focusOutEvent(QFocusEvent* event) override
  { QPushButton::focusOutEvent(event); iupqtFocusInOutEvent(this, event, iup_handle); }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    { click(); event->accept(); return; }
    if (iupqtKeyPressEvent(this, event, iup_handle))
    { event->accept(); return; }
    QPushButton::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  { QPushButton::mousePressEvent(event); iupqtMouseButtonEvent(this, event, iup_handle); }
  void mouseReleaseEvent(QMouseEvent* event) override
  { QPushButton::mouseReleaseEvent(event); iupqtMouseButtonEvent(this, event, iup_handle); }
};

class IupQtImageButton : public QToolButton
{
public:
  Ihandle* iup_handle;

  explicit IupQtImageButton(Ihandle* ih) : QToolButton(), iup_handle(ih) {}

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  { QToolButton::enterEvent(event); iupqtEnterLeaveEvent(this, event, iup_handle); }
  void leaveEvent(QEvent* event) override
  { QToolButton::leaveEvent(event); iupqtEnterLeaveEvent(this, event, iup_handle); }
  void focusInEvent(QFocusEvent* event) override
  { QToolButton::focusInEvent(event); iupqtFocusInOutEvent(this, event, iup_handle); }
  void focusOutEvent(QFocusEvent* event) override
  { QToolButton::focusOutEvent(event); iupqtFocusInOutEvent(this, event, iup_handle); }

  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    { click(); event->accept(); return; }
    if (iupqtKeyPressEvent(this, event, iup_handle))
    { event->accept(); return; }
    QToolButton::keyPressEvent(event);
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    QToolButton::mousePressEvent(event);
    char* name = iupAttribGet(iup_handle, "IMPRESS");
    if (name)
      qtButtonSetPixmap(iup_handle, name, 0);
    iupqtMouseButtonEvent(this, event, iup_handle);
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    QToolButton::mouseReleaseEvent(event);
    char* impress = iupAttribGet(iup_handle, "IMPRESS");
    if (impress)
    {
      char* image = iupAttribGet(iup_handle, "IMAGE");
      if (image)
        qtButtonSetPixmap(iup_handle, image, 0);
    }
    iupqtMouseButtonEvent(this, event, iup_handle);
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qtButtonSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  if (!name)
    return;

  QAbstractButton* button = (QAbstractButton*)ih->handle;
  if (!button)
    return;

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

static void qtButtonUpdateLayout(Ihandle* ih)
{
  QAbstractButton* button = (QAbstractButton*)ih->handle;
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

  }
}

extern "C" IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 8;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    int has_image = (image != NULL);
    int has_text = (title != NULL && *title != 0);
    int has_bgcolor = (!has_image && !has_text && bgcolor != NULL);

    if (has_bgcolor)
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      (*x) += charheight;
    }
  }

  (*x) += border_size;
  (*y) += border_size;
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    QAbstractButton* button = (QAbstractButton*)ih->handle;

    if (button)
    {
      if (iupAttribGetBoolean(ih, "MARKUP"))
      {
        char* html = iupMarkupToHtml(value ? value : "");
        button->setText(QString());

        QLabel* markupLabel = button->findChild<QLabel*>("_iup_markup_label");
        if (!markupLabel)
        {
          markupLabel = new QLabel(button);
          markupLabel->setObjectName("_iup_markup_label");
          markupLabel->setAlignment(Qt::AlignCenter);
          markupLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
          if (!button->layout())
            button->setLayout(new QHBoxLayout());
          button->layout()->setContentsMargins(0, 0, 0, 0);
          button->layout()->addWidget(markupLabel);
        }
        markupLabel->setTextFormat(Qt::RichText);
        markupLabel->setText(QString::fromUtf8(html));
        free(html);
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
        }
      }

      qtButtonUpdateLayout(ih);
      return 1;
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
  iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');

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
    iupdrvPostRedraw(ih);

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

  return 0;
}

static char* qtButtonGetBgColorAttrib(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE || iupAttribGet(ih, "IMPRESS"))
    return iupBaseNativeParentGetBgColorAttrib(ih);

  return NULL;
}

static int qtButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!value || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  QAbstractButton* button = (QAbstractButton*)ih->handle;
  if (button)
  {
    QPalette palette = button->palette();
    palette.setColor(QPalette::Button, QColor(r, g, b));
    button->setPalette(palette);
    button->setAutoFillBackground(true);
  }
  return 1;
}

static int qtButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QAbstractButton* button = (QAbstractButton*)ih->handle;

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
    QAbstractButton* button = (QAbstractButton*)ih->handle;
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
      qtButtonSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        /* If not active and IMINACTIVE not defined, create inactive version */
        qtButtonSetPixmap(ih, value, 1);
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
        qtButtonSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        qtButtonSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }

  return 0;
}

static void qtButtonApplyFlat(Ihandle* ih, int flat)
{
  QPushButton* pb = dynamic_cast<QPushButton*>((QWidget*)ih->handle);
  if (pb) { pb->setFlat(flat); return; }
  QToolButton* tb = dynamic_cast<QToolButton*>((QWidget*)ih->handle);
  if (tb) tb->setAutoRaise(flat);
}

static int qtButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (ih->handle)
    {
      int should_be_flat = iupAttribGetBoolean(ih, "FLAT");
      if (!should_be_flat &&
          iupAttribGet(ih, "IMPRESS") &&
          !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
      {
        should_be_flat = 1;
      }
      qtButtonApplyFlat(ih, should_be_flat);
    }

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
        qtButtonSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        qtButtonSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      qtButtonSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int qtButtonSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    int should_be_flat = iupStrBoolean(value);
    if (!should_be_flat &&
        ih->data->type == IUP_BUTTON_IMAGE &&
        iupAttribGet(ih, "IMPRESS") &&
        !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    {
      should_be_flat = 1;
    }
    qtButtonApplyFlat(ih, should_be_flat);
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
  QAbstractButton* button = (QAbstractButton*)ih->handle;
  if (!button)
    return;

  iupdrvBaseLayoutUpdateMethod(ih);
  qtButtonUpdateLayout(ih);
}

static int qtButtonMapMethod(Ihandle* ih)
{
  QAbstractButton* button;

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

  if (ih->data->type == IUP_BUTTON_IMAGE)
    button = new IupQtImageButton(ih);
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      button = new IupQtButton(QString::fromUtf8(title), ih);
    else
      button = new IupQtButton(ih);
  }

  ih->handle = (InativeHandle*)button;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      qtButtonSetPixmap(ih, image, 0);
  }

  if (ih->data->type == IUP_BUTTON_TEXT && iupAttribGet(ih, "BGCOLOR") && !iupAttribGet(ih, "TITLE"))
    qtButtonSetBgColorAttrib(ih, iupAttribGet(ih, "BGCOLOR"));

  iupqtAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqtSetCanFocus(button, 0);

  if (iupAttribGetBoolean(ih, "FLAT"))
    qtButtonApplyFlat(ih, 1);

  if (ih->data->type == IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
  {
    qtButtonApplyFlat(ih, 1);
    button->setStyleSheet(QString("QToolButton { border: none; background: transparent; }"));
  }

  QObject::connect(button, &QAbstractButton::clicked, [ih]() {
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

extern "C" IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
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
