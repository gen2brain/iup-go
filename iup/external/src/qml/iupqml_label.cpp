/** \file
 * \brief Label Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstring>
#include <QQuickItem>
#include <QPixmap>
#include <QString>
#include <QUrl>
#include <QColor>

#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_label.h"
#include "iup_markup.h"
}

#include "iupqml_drv.h"


static QQuickItem* qmlLabelGetInner(Ihandle* ih)
{
  return reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_LABEL_INNER"));
}

static void qmlLabelSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  QQuickItem* image = qmlLabelGetInner(ih);
  if (!image || !name)
    return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(name, ih, make_inactive, bgcolor));

  if (pixmap && !pixmap->isNull())
    image->setProperty("source", QUrl(iupqmlImageUrl(pixmap)));
  else
    image->setProperty("source", QUrl());
}

extern "C" IUP_SDK_API void iupdrvLabelAddExtraPadding(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  (void)x;
  (void)y;
}

/****************************************************************************
 * Text
 ****************************************************************************/

static void qmlLabelUpdateText(Ihandle* ih, QQuickItem* label, const char* value)
{
  if (!value)
  {
    label->setProperty("text", QString());
    return;
  }

  if (iupAttribGetBoolean(ih, "MARKUP"))
  {
    char* html = iupMarkupToHtml(value);
    label->setProperty("textFormat", 1);
    label->setProperty("text", QString::fromUtf8(html));
    free(html);
    return;
  }

  char c;
  char* str = iupStrProcessMnemonic(value, &c, -1);

  if (str && str != value)
  {
    QString text = QString::fromUtf8(str);
    int idx = text.indexOf(QChar(c), 0, Qt::CaseInsensitive);
    if (idx >= 0)
    {
      if (iupqmlMnemonicVisible())
        text = text.left(idx).toHtmlEscaped() + "<u>" + text.mid(idx, 1).toHtmlEscaped() + "</u>" + text.mid(idx + 1).toHtmlEscaped();
      else
        text = text.toHtmlEscaped();
      label->setProperty("textFormat", 4);
      label->setProperty("text", text);
      if (IupGetDialog(ih))
        iupKeySetMnemonic(ih, c, -1);
    }
    else
    {
      label->setProperty("textFormat", 0);
      label->setProperty("text", text);
    }
    free(str);
  }
  else
  {
    label->setProperty("textFormat", 0);
    label->setProperty("text", QString::fromUtf8(value));
  }
}

static void qmlLabelMnemonicRefresh(Ihandle* ih)
{
  QQuickItem* label = qmlLabelGetInner(ih);
  if (label && ih->data->type == IUP_LABEL_TEXT)
    qmlLabelUpdateText(ih, label, iupAttribGet(ih, "TITLE"));
}

static int qmlLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QQuickItem* label = qmlLabelGetInner(ih);
    if (label)
    {
      qmlLabelUpdateText(ih, label, value);
      if (value && strchr(value, '&'))
        iupqmlMnemonicRegister(ih, qmlLabelMnemonicRefresh);
      return 1;
    }
  }

  return 0;
}

static char* qmlLabelGetTitleAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_TEXT)
    return iupAttribGet(ih, "TITLE");

  return nullptr;
}

static int qmlLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT || ih->data->type == IUP_LABEL_IMAGE)
  {
    QQuickItem* inner = qmlLabelGetInner(ih);
    if (!inner)
      return 0;

    char value1[30], value2[30];
    iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');

    int halign = Qt::AlignLeft, valign = Qt::AlignVCenter;
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

    if (iupStrEqualNoCase(value1, "ARIGHT"))
    {
      halign = Qt::AlignRight;
      ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
    }
    else if (iupStrEqualNoCase(value1, "ACENTER"))
    {
      halign = Qt::AlignHCenter;
      ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
    }

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
    {
      valign = Qt::AlignBottom;
      ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
    }
    else if (iupStrEqualNoCase(value2, "ATOP"))
    {
      valign = Qt::AlignTop;
      ih->data->vert_alignment = IUP_ALIGN_ATOP;
    }

    inner->setProperty("horizontalAlignment", halign);
    inner->setProperty("verticalAlignment", valign);
    return 1;
  }

  return 0;
}

static char* qmlLabelGetAlignmentAttrib(Ihandle* ih)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    char* horiz_align2str[3] = {const_cast<char*>("ALEFT"), const_cast<char*>("ACENTER"), const_cast<char*>("ARIGHT")};
    char* vert_align2str[3] = {const_cast<char*>("ATOP"), const_cast<char*>("ACENTER"), const_cast<char*>("ABOTTOM")};

    int horiz = ih->data->horiz_alignment;
    int vert = ih->data->vert_alignment;

    if (horiz < IUP_ALIGN_ALEFT || horiz > IUP_ALIGN_ARIGHT)
      horiz = IUP_ALIGN_ALEFT;
    if (vert < IUP_ALIGN_ATOP || vert > IUP_ALIGN_ABOTTOM)
      vert = IUP_ALIGN_ACENTER;

    return iupStrReturnStrf("%s:%s", horiz_align2str[horiz], vert_align2str[vert]);
  }

  return nullptr;
}

static int qmlLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QQuickItem* label = qmlLabelGetInner(ih);
    if (label)
    {
      label->setProperty("wrapMode", iupStrBoolean(value) ? 4 : 0);
      return 1;
    }
  }

  return 0;
}

static int qmlLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    QQuickItem* label = qmlLabelGetInner(ih);
    if (label)
    {
      label->setProperty("elide", iupStrBoolean(value) ? 1 : 3);
      return 1;
    }
  }

  return 0;
}

static QQuickItem* qmlLabelCreateText(bool selectable)
{
  if (selectable)
    return iupqmlCreateItem(IUPQML_IMPORTS "TextEdit { readOnly: true; selectByMouse: true; color: palette.windowText; selectionColor: palette.highlight; selectedTextColor: palette.highlightedText; horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter }");
  return iupqmlCreateItem(IUPQML_IMPORTS "Label { horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter; elide: Text.ElideNone }");
}

static bool qmlLabelIsSelectable(QQuickItem* inner)
{
  return inner && strcmp(inner->metaObject()->className(), "QQuickTextEdit") == 0;
}

static int qmlLabelSetSelectableAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_TEXT)
    return 0;

  QQuickItem* old = qmlLabelGetInner(ih);
  bool selectable = iupStrBoolean(value);
  if (!old || qmlLabelIsSelectable(old) == selectable)
    return 1;

  QQuickItem* inner = qmlLabelCreateText(selectable);
  if (!inner)
    return 0;

  static const char* props[] = {"color", "font", "horizontalAlignment", "verticalAlignment", "wrapMode"};
  for (const char* name : props)
    inner->setProperty(name, old->property(name));
  inner->setPosition(old->position());
  inner->setSize(old->size());
  inner->setParentItem(old->parentItem());
  inner->setParent(old->parent());
  iupAttribSet(ih, "_IUPQML_LABEL_INNER", reinterpret_cast<char*>(inner));
  delete old;

  qmlLabelUpdateText(ih, inner, iupAttribGet(ih, "TITLE"));
  return 1;
}

static int qmlLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  auto* wrapper = reinterpret_cast<QQuickItem*>(ih->handle);
  if (wrapper)
  {
    auto* back = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_LABEL_BACK"));
    if (!back)
    {
      back = iupqmlCreateItem("import QtQuick\nRectangle { z: -1 }");
      if (back)
      {
        back->setParentItem(wrapper);
        back->setParent(wrapper);
        back->setPosition(QPointF(0, 0));
        back->setSize(wrapper->size());
        iupAttribSet(ih, "_IUPQML_LABEL_BACK", reinterpret_cast<char*>(back));
      }
    }
    if (back)
      back->setProperty("color", QColor(r, g, b));
    return 1;
  }

  return 0;
}

static int qmlLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QQuickItem* inner = qmlLabelGetInner(ih);
  if (inner)
  {
    inner->setProperty("color", QColor(r, g, b));
    return 1;
  }

  return 0;
}

static int qmlLabelSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    QQuickItem* inner = qmlLabelGetInner(ih);
    if (inner)
      iupqmlUpdateItemFont(ih, inner);
  }

  return 1;
}

static char* qmlLabelGetImageAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
    return iupAttribGet(ih, "IMAGE");

  return nullptr;
}

static int qmlLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (iupdrvIsActive(ih))
      qmlLabelSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        qmlLabelSetPixmap(ih, value, 1);
    }
    return 1;
  }

  return 0;
}

static int qmlLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        qmlLabelSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        qmlLabelSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }

  return 0;
}

static int qmlLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        qmlLabelSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        qmlLabelSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      qmlLabelSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static void qmlLabelLayoutInner(Ihandle* ih)
{
  auto* wrapper = reinterpret_cast<QQuickItem*>(ih->handle);
  QQuickItem* inner = qmlLabelGetInner(ih);
  if (!wrapper || !inner)
    return;

  int hp = ih->data->horiz_padding, vp = ih->data->vert_padding;
  int w = ih->currentwidth - 2*hp, h = ih->currentheight - 2*vp;
  if (w < 0) w = 0;
  if (h < 0) h = 0;

  if (ih->data->type == IUP_LABEL_SEP_HORIZ)
  {
    inner->setPosition(QPointF(0, ih->currentheight / 2));
    inner->setSize(QSizeF(ih->currentwidth, 1));
  }
  else if (ih->data->type == IUP_LABEL_SEP_VERT)
  {
    inner->setPosition(QPointF(ih->currentwidth / 2, 0));
    inner->setSize(QSizeF(1, ih->currentheight));
  }
  else
  {
    inner->setPosition(QPointF(hp, vp));
    inner->setSize(QSizeF(w, h));
  }

  auto* back = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_LABEL_BACK"));
  if (back)
    back->setSize(wrapper->size());
}

static int qmlLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    qmlLabelLayoutInner(ih);
    return 0;
  }

  return 1;
}

static int qmlLabelSetSunkenAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

static int qmlLabelSetHtTransparentAttrib(Ihandle* ih, const char* value)
{
  auto* wrapper = reinterpret_cast<QQuickItem*>(ih->handle);
  if (wrapper)
  {
    int transparent = iupStrBoolean(value);
    wrapper->setAcceptedMouseButtons(transparent ? Qt::NoButton : Qt::AllButtons);
    wrapper->setAcceptHoverEvents(!transparent);
    return 1;
  }

  return 0;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static void qmlLabelLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  qmlLabelLayoutInner(ih);
}

static int qmlLabelMapMethod(Ihandle* ih)
{
  char* value;
  QQuickItem* inner = nullptr;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
      ih->data->type = IUP_LABEL_SEP_HORIZ;
    else
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

  if (ih->data->type == IUP_LABEL_SEP_HORIZ || ih->data->type == IUP_LABEL_SEP_VERT)
    inner = iupqmlCreateItem("import QtQuick\nRectangle { color: palette.mid }");
  else if (ih->data->type == IUP_LABEL_IMAGE)
    inner = iupqmlCreateItem("import QtQuick\nImage { fillMode: Image.Pad; cache: false; horizontalAlignment: Image.AlignLeft; verticalAlignment: Image.AlignVCenter }");
  else
    inner = qmlLabelCreateText(iupAttribGetBoolean(ih, "SELECTABLE"));

  if (!inner)
    return IUP_ERROR;

  QQuickItem* wrapper = iupqmlCreateItem("import QtQuick\nItem { clip: true }");
  if (!wrapper)
  {
    delete inner;
    return IUP_ERROR;
  }

  inner->setParentItem(wrapper);
  inner->setParent(wrapper);
  iupAttribSet(ih, "_IUPQML_LABEL_INNER", reinterpret_cast<char*>(inner));

  ih->handle = reinterpret_cast<InativeHandle*>(wrapper);

  if (ih->data->type == IUP_LABEL_TEXT)
    qmlLabelUpdateText(ih, inner, iupAttribGet(ih, "TITLE"));

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, wrapper);
  wrapper->setAcceptedMouseButtons(Qt::AllButtons);

  value = iupAttribGet(ih, "PADDING");
  if (value)
    qmlLabelSetPaddingAttrib(ih, value);

  value = iupAttribGet(ih, "HTTRANSPARENT");
  if (value)
    qmlLabelSetHtTransparentAttrib(ih, value);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void qmlLabelUnMapMethod(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPQML_LABEL_INNER", nullptr);
  iupAttribSet(ih, "_IUPQML_LABEL_BACK", nullptr);
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = qmlLabelMapMethod;
  ic->UnMap = qmlLabelUnMapMethod;
  ic->LayoutUpdate = qmlLabelLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, qmlLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlLabelSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", qmlLabelGetTitleAttrib, qmlLabelSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", qmlLabelGetAlignmentAttrib, qmlLabelSetAlignmentAttrib, "ALEFT:ACENTER", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", qmlLabelGetImageAttrib, qmlLabelSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", nullptr, qmlLabelSetImInactiveAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, qmlLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "WORDWRAP", nullptr, qmlLabelSetWordWrapAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", nullptr, qmlLabelSetEllipsisAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SUNKEN", nullptr, qmlLabelSetSunkenAttrib, nullptr, nullptr, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "SELECTABLE", nullptr, qmlLabelSetSelectableAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HTTRANSPARENT", nullptr, qmlLabelSetHtTransparentAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARKUP", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
}
