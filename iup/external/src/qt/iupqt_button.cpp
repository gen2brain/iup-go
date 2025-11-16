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

extern "C" void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2 * 6;  /* Typical Qt button padding */
  (void)ih;
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
    return "RIGHT";
  else if (ih->data->img_position == IUP_IMGPOS_TOP)
    return "TOP";
  else if (ih->data->img_position == IUP_IMGPOS_BOTTOM)
    return "BOTTOM";
  else
    return "LEFT";
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
    {
      IupQtButton* button = (IupQtButton*)ih->handle;
      button->setStyleSheet("");
      return 1;
    }
    return 0;
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtButton* button = (IupQtButton*)ih->handle;

  if (button)
  {
    QString style = QString("QPushButton { background-color: rgb(%1,%2,%3); }").arg(r).arg(g).arg(b);
    button->setStyleSheet(style);
    return 1;
  }

  return 0;
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

  QSize size_hint = button->sizeHint();

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
