/** \file
 * \brief Label Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QWidget>
#include <QPalette>
#include <QFont>
#include <QFontMetrics>

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
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_label.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Event Wrapper Widget (like GTK's EventBox)
 ****************************************************************************/

class IupQtEventWrapper : public QWidget
{
public:
  Ihandle* iup_handle;
  QWidget* child_widget;
  bool ht_transparent;

  IupQtEventWrapper(Ihandle* ih)
    : QWidget(), iup_handle(ih), child_widget(nullptr), ht_transparent(false)
  {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
  }

  void setChildWidget(QWidget* child)
  {
    child_widget = child;
    if (child)
    {
      child->setParent(this);
      child->move(0, 0);
    }
  }

  void setHitTransparent(bool transparent)
  {
    ht_transparent = transparent;
    if (transparent)
      setAttribute(Qt::WA_TransparentForMouseEvents, true);
    else
      setAttribute(Qt::WA_TransparentForMouseEvents, false);
  }

protected:
  bool event(QEvent* event) override
  {
    if (ht_transparent)
    {
      /* Let events pass through */
      event->ignore();
      return false;
    }

    return QWidget::event(event);
  }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEnterEvent* event) override
#else
  void enterEvent(QEvent* event) override
#endif
  {
    if (!ht_transparent)
    {
      QWidget::enterEvent(event);
      iupqtEnterLeaveEvent(this, event, iup_handle);
    }
  }

  void leaveEvent(QEvent* event) override
  {
    if (!ht_transparent)
    {
      QWidget::leaveEvent(event);
      iupqtEnterLeaveEvent(this, event, iup_handle);
    }
  }

  void mousePressEvent(QMouseEvent* event) override
  {
    if (!ht_transparent)
    {
      QWidget::mousePressEvent(event);
      iupqtMouseButtonEvent(this, event, iup_handle);
    }
  }

  void mouseReleaseEvent(QMouseEvent* event) override
  {
    if (!ht_transparent)
    {
      QWidget::mouseReleaseEvent(event);
      iupqtMouseButtonEvent(this, event, iup_handle);
    }
  }

  void resizeEvent(QResizeEvent* event) override
  {
    QWidget::resizeEvent(event);
    if (child_widget)
      child_widget->resize(size());
  }
};

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static QWidget* qtLabelGetInnerWidget(Ihandle* ih)
{
  IupQtEventWrapper* wrapper = (IupQtEventWrapper*)ih->handle;
  if (wrapper)
    return wrapper->child_widget;
  return nullptr;
}

static void qtLabelSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  if (!name)
    return;

  QWidget* widget = qtLabelGetInnerWidget(ih);
  QLabel* label = qobject_cast<QLabel*>(widget);

  if (label)
  {
    const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
    QPixmap* pixmap = (QPixmap*)iupImageGetImage(name, ih, make_inactive, bgcolor);

    if (pixmap && !pixmap->isNull())
    {
      label->setPixmap(*pixmap);
      label->setScaledContents(false);
    }
    else
    {
      label->clear();
    }
  }
}

/****************************************************************************
 * Border Size Calculation
 ****************************************************************************/

extern "C" void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

/****************************************************************************
 * Attribute Setters/Getters
 ****************************************************************************/

static int qtLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (label)
    {
      /* Qt doesn't have setText_with_mnemonic like GTK, so we need to
       * process mnemonics manually: remove & and underline the next character */
      if (value)
      {
        char c;
        char* str = iupStrProcessMnemonic(value, &c, -1); /* action=-1: remove & and return mnemonic in c */

        if (str && str != value)
        {
          /* Found a mnemonic - convert to HTML with underline */
          QString text = QString::fromUtf8(str);

          /* Find the mnemonic character and wrap it in <u> tags */
          int idx = text.indexOf(QChar(c), 0, Qt::CaseInsensitive);
          if (idx >= 0)
          {
            text = text.left(idx) + "<u>" + text.mid(idx, 1) + "</u>" + text.mid(idx + 1);
            label->setTextFormat(Qt::RichText);
            label->setText(text);
          }
          else
          {
            label->setTextFormat(Qt::PlainText);
            label->setText(text);
          }

          free(str);
        }
        else
        {
          label->setTextFormat(Qt::PlainText);
          label->setText(QString::fromUtf8(value));
        }
      }
      else
      {
        label->setText(QString());
      }

      return 1;
    }
  }

  return 0;
}

static char* qtLabelGetTitleAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (label)
    {
      QString text = label->text();
      /* Remove Qt's HTML formatting if any */
      return iupStrReturnStr(text.toUtf8().constData());
    }
  }

  return nullptr;
}

static int qtLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (!label)
      return 0;

    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

    /* Horizontal alignment */
    Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter;

    if (iupStrEqualNoCase(value1, "ARIGHT"))
    {
      align = Qt::AlignRight | Qt::AlignVCenter;
      ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
    }
    else if (iupStrEqualNoCase(value1, "ACENTER"))
    {
      align = Qt::AlignHCenter | Qt::AlignVCenter;
      ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
    }
    else if (iupStrEqualNoCase(value1, "ALEFT"))
    {
      align = Qt::AlignLeft | Qt::AlignVCenter;
      ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
    }

    /* Vertical alignment */
    if (iupStrEqualNoCase(value2, "ABOTTOM"))
    {
      align = (align & Qt::AlignHorizontal_Mask) | Qt::AlignBottom;
      ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
    }
    else if (iupStrEqualNoCase(value2, "ATOP"))
    {
      align = (align & Qt::AlignHorizontal_Mask) | Qt::AlignTop;
      ih->data->vert_alignment = IUP_ALIGN_ATOP;
    }
    else if (iupStrEqualNoCase(value2, "ACENTER"))
    {
      align = (align & Qt::AlignHorizontal_Mask) | Qt::AlignVCenter;
      ih->data->vert_alignment = IUP_ALIGN_ACENTER;
    }

    label->setAlignment(align);
    return 1;
  }
  else if (ih->data->type == IUP_LABEL_IMAGE)
  {
    /* Store alignment for image labels */
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
    else
      ih->data->horiz_alignment = IUP_ALIGN_ALEFT;

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
    else if (iupStrEqualNoCase(value2, "ATOP"))
      ih->data->vert_alignment = IUP_ALIGN_ATOP;
    else
      ih->data->vert_alignment = IUP_ALIGN_ACENTER;

    return 1;
  }

  return 0;
}

static char* qtLabelGetAlignmentAttrib(Ihandle* ih)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    char* horiz_align2str[3] = {(char*)"ALEFT", (char*)"ACENTER", (char*)"ARIGHT"};
    char* vert_align2str[3] = {(char*)"ATOP", (char*)"ACENTER", (char*)"ABOTTOM"};

    int horiz = ih->data->horiz_alignment;
    int vert = ih->data->vert_alignment;

    /* Validate indices */
    if (horiz < IUP_ALIGN_ALEFT || horiz > IUP_ALIGN_ARIGHT)
      horiz = IUP_ALIGN_ALEFT;
    if (vert < IUP_ALIGN_ATOP || vert > IUP_ALIGN_ABOTTOM)
      vert = IUP_ALIGN_ACENTER;

    return iupStrReturnStrf("%s:%s", horiz_align2str[horiz], vert_align2str[vert]);
  }

  return nullptr;
}

static int qtLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (label)
    {
      if (iupStrBoolean(value))
        label->setWordWrap(true);
      else
        label->setWordWrap(false);

      return 1;
    }
  }

  return 0;
}

static int qtLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (label)
    {
      if (iupStrBoolean(value))
      {
        /* Qt will automatically elide text that doesn't fit.
         * Don't set PlainText format so mnemonics still work. */
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
      }
      else
      {
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
      }

      return 1;
    }
  }

  return 0;
}

static int qtLabelSetSelectableAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (label)
    {
      if (iupStrBoolean(value))
      {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
      }
      else
      {
        label->setTextInteractionFlags(Qt::NoTextInteraction);
      }

      return 1;
    }
  }

  return 0;
}

static char* qtLabelGetSelectableAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QLabel* label = qobject_cast<QLabel*>(widget);

    if (label)
    {
      bool is_selectable = (label->textInteractionFlags() & Qt::TextSelectableByMouse) != 0;
      return iupStrReturnBoolean(is_selectable);
    }
  }

  return nullptr;
}

static int qtLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupQtEventWrapper* wrapper = (IupQtEventWrapper*)ih->handle;
  QWidget* widget = qtLabelGetInnerWidget(ih);

  if (wrapper && widget)
  {
    QPalette palette;
    QColor color(r, g, b);

    /* Set background for both wrapper and inner widget */
    palette = wrapper->palette();
    palette.setColor(QPalette::Window, color);
    wrapper->setPalette(palette);
    wrapper->setAutoFillBackground(true);

    palette = widget->palette();
    palette.setColor(QPalette::Window, color);
    widget->setPalette(palette);
    widget->setAutoFillBackground(true);

    return 1;
  }

  return 0;
}

static int qtLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QWidget* widget = qtLabelGetInnerWidget(ih);
  QLabel* label = qobject_cast<QLabel*>(widget);

  if (label)
  {
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, QColor(r, g, b));
    label->setPalette(palette);
    return 1;
  }

  return 0;
}

static int qtLabelSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    if (widget)
      iupqtUpdateWidgetFont(ih, widget);
  }

  return 1;
}

static char* qtLabelGetImageAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
    return iupAttribGet(ih, "IMAGE");

  return nullptr;
}

static int qtLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (iupdrvIsActive(ih))
      qtLabelSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        /* If not active and IMINACTIVE not defined, create inactive version */
        qtLabelSetPixmap(ih, value, 1);
      }
    }
    return 1;
  }

  return 0;
}

static char* qtLabelGetImInactiveAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
    return iupAttribGet(ih, "IMINACTIVE");

  return nullptr;
}

static int qtLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        qtLabelSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        qtLabelSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }

  return 0;
}

static int qtLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* Update the inactive image if necessary */
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        qtLabelSetPixmap(ih, name, 0);
      else
      {
        /* Create inactive version from IMAGE */
        name = iupAttribGet(ih, "IMAGE");
        qtLabelSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      qtLabelSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int qtLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    if (widget)
    {
      widget->setContentsMargins(ih->data->horiz_padding, ih->data->vert_padding,
                                 ih->data->horiz_padding, ih->data->vert_padding);
      return 0;
    }
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */

  return 0;
}

static int qtLabelSetSunkenAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_SEP_HORIZ || ih->data->type == IUP_LABEL_SEP_VERT)
  {
    QWidget* widget = qtLabelGetInnerWidget(ih);
    QFrame* frame = qobject_cast<QFrame*>(widget);

    if (frame)
    {
      if (iupStrBoolean(value))
      {
        if (ih->data->type == IUP_LABEL_SEP_HORIZ)
          frame->setFrameStyle(QFrame::HLine | QFrame::Sunken);
        else
          frame->setFrameStyle(QFrame::VLine | QFrame::Sunken);
      }
      else
      {
        if (ih->data->type == IUP_LABEL_SEP_HORIZ)
          frame->setFrameStyle(QFrame::HLine | QFrame::Plain);
        else
          frame->setFrameStyle(QFrame::VLine | QFrame::Plain);
      }

      return 1;
    }
  }

  return 0;
}

static int qtLabelSetHtTransparentAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupQtEventWrapper* wrapper = (IupQtEventWrapper*)ih->handle;
    if (wrapper)
    {
      wrapper->setHitTransparent(iupStrBoolean(value));
      return 1;
    }
  }

  return 0;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtLabelMapMethod(Ihandle* ih)
{
  char* value;
  QWidget* inner_widget = nullptr;

  /* Determine label type */
  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
      ih->data->type = IUP_LABEL_SEP_HORIZ;
    else /* "VERTICAL" */
      ih->data->type = IUP_LABEL_SEP_VERT;
  }
  else
  {
    value = iupAttribGet(ih, "IMAGE");
    if (value)
      ih->data->type = IUP_LABEL_IMAGE;
    else
      ih->data->type = IUP_LABEL_TEXT;
  }

  /* Create inner widget based on type */
  if (ih->data->type == IUP_LABEL_SEP_HORIZ)
  {
    QFrame* frame = new QFrame();
    frame->setFrameStyle(QFrame::HLine | QFrame::Plain);
    frame->setLineWidth(1);
    inner_widget = frame;
  }
  else if (ih->data->type == IUP_LABEL_SEP_VERT)
  {
    QFrame* frame = new QFrame();
    frame->setFrameStyle(QFrame::VLine | QFrame::Plain);
    frame->setLineWidth(1);
    inner_widget = frame;
  }
  else /* TEXT or IMAGE */
  {
    QLabel* label = new QLabel();
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    if (ih->data->type == IUP_LABEL_TEXT)
    {
      char* title = iupAttribGet(ih, "TITLE");
      if (title)
      {
        /* Process mnemonics: remove & and underline the mnemonic character */
        char c;
        char* str = iupStrProcessMnemonic(title, &c, -1); /* action=-1: remove & and return mnemonic in c */

        if (str && str != title)
        {
          /* Found a mnemonic - convert to HTML with underline */
          QString text = QString::fromUtf8(str);

          /* Find the mnemonic character and wrap it in <u> tags */
          int idx = text.indexOf(QChar(c), 0, Qt::CaseInsensitive);
          if (idx >= 0)
          {
            text = text.left(idx) + "<u>" + text.mid(idx, 1) + "</u>" + text.mid(idx + 1);
            label->setTextFormat(Qt::RichText);
            label->setText(text);
          }
          else
          {
            label->setTextFormat(Qt::PlainText);
            label->setText(text);
          }

          free(str);
        }
        else
        {
          label->setTextFormat(Qt::PlainText);
          label->setText(QString::fromUtf8(title));
        }
      }
      else
        label->setText(QString());
    }
    else /* IUP_LABEL_IMAGE */
    {
      char* image = iupAttribGet(ih, "IMAGE");
      if (image)
        qtLabelSetPixmap(ih, image, 0);
    }

    inner_widget = label;
  }

  if (!inner_widget)
    return IUP_ERROR;

  /* Create event wrapper */
  IupQtEventWrapper* wrapper = new IupQtEventWrapper(ih);
  wrapper->setChildWidget(inner_widget);

  ih->handle = (InativeHandle*)wrapper;

  /* Add to parent */
  iupqtAddToParent(ih);

  /* Configure focus */
  iupqtSetCanFocus(wrapper, 0);  /* Labels are not focusable */

  /* Set padding if specified */
  value = iupAttribGet(ih, "PADDING");
  if (value)
    qtLabelSetPaddingAttrib(ih, value);

  /* Update mnemonic */
  if (ih->data->type == IUP_LABEL_TEXT)
    iupqtUpdateMnemonic(ih);

  /* Set HTTRANSPARENT if specified */
  value = iupAttribGet(ih, "HTTRANSPARENT");
  if (value)
    qtLabelSetHtTransparentAttrib(ih, value);

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvLabelInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtLabelMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, qtLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, qtLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, qtLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, qtLabelSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", qtLabelGetTitleAttrib, qtLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupLabel only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", qtLabelGetAlignmentAttrib, qtLabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", qtLabelGetImageAttrib, qtLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", qtLabelGetImInactiveAttrib, qtLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, qtLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, qtLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, qtLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, qtLabelSetSunkenAttrib, NULL, NULL, IUPAF_DEFAULT);

  /* IupLabel Qt specific */
  iupClassRegisterAttribute(ic, "SELECTABLE", qtLabelGetSelectableAttrib, qtLabelSetSelectableAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_DEFAULT|IUPAF_NO_INHERIT);

  /* IupLabel Windows only */
  iupClassRegisterAttribute(ic, "HTTRANSPARENT", NULL, qtLabelSetHtTransparentAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}
