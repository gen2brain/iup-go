/** \file
 * \brief Button Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QPixmap>
#include <QString>
#include <QUrl>
#include <QColor>
#include <QKeyEvent>

#include <cstdlib>
#include <memory>
#include <cstring>

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

#include "iupqml_drv.h"


static const char* qml_button_content =
  "import QtQuick\n"
  "import QtQuick.Controls.impl\n"
  "Item {\n"
  "  id: root\n"
  "  property Item control: null\n"
  "  property bool vertical: false\n"
  "  property bool imageFirst: true\n"
  "  property string markup: \"\"\n"
  "  readonly property string label: markup !== \"\" ? markup : control ? control.text : \"\"\n"
  "  property int alignment: Qt.AlignHCenter | Qt.AlignVCenter\n"
  "  implicitWidth: box.implicitWidth\n"
  "  implicitHeight: box.implicitHeight\n"
  "  Grid {\n"
  "    id: box\n"
  "    columns: root.vertical || !labelItem.visible ? 1 : 2\n"
  "    spacing: root.control ? root.control.spacing : 2\n"
  "    horizontalItemAlignment: Grid.AlignHCenter\n"
  "    verticalItemAlignment: Grid.AlignVCenter\n"
  "    x: (root.alignment & Qt.AlignLeft) ? 0 : (root.alignment & Qt.AlignRight) ? root.width - width : root.control ? Math.round((root.control.width - width) / 2) - root.x : Math.round((root.width - width) / 2)\n"
  "    y: (root.alignment & Qt.AlignTop) ? 0 : (root.alignment & Qt.AlignBottom) ? root.height - height : root.control ? Math.round((root.control.height - height) / 2) - root.y : Math.round((root.height - height) / 2)\n"
  "    Image {\n"
  "      visible: root.imageFirst && source != \"\"\n"
  "      source: root.control ? root.control.icon.source : \"\"\n"
  "      width: root.control ? root.control.icon.width : 0\n"
  "      height: root.control ? root.control.icon.height : 0\n"
  "      cache: false\n"
  "    }\n"
  "    MnemonicLabel {\n"
  "      id: labelItem\n"
  "      visible: root.label !== \"\"\n"
  "      text: root.label\n"
  "      font: root.control ? root.control.font : Qt.application.font\n"
  "      color: !root.control ? \"black\" : root.control.enabled ? root.control.palette.buttonText : root.control.palette.disabled.buttonText\n"
  "    }\n"
  "    Image {\n"
  "      visible: !root.imageFirst && source != \"\"\n"
  "      source: root.control ? root.control.icon.source : \"\"\n"
  "      width: root.control ? root.control.icon.width : 0\n"
  "      height: root.control ? root.control.icon.height : 0\n"
  "      cache: false\n"
  "    }\n"
  "  }\n"
  "}\n";

static QQuickItem* qmlButtonTemplate(Ihandle* ih)
{
  char* title = ih ? iupAttribGet(ih, "TITLE") : nullptr;
  int has_image = ih && iupAttribGet(ih, "IMAGE");
  int has_text = !has_image || (title && *title);

  if (has_image && has_text)
    return iupqmlButtonFrameInset(iupqmlTemplateItem(IUPQML_IMPORTS "Button { text: \"X\"; icon.name: \"iup\" }"));
  if (has_image)
    return iupqmlButtonFrameInset(iupqmlTemplateItem(IUPQML_IMPORTS "Button { icon.name: \"iup\" }"));
  return iupqmlButtonFrameInset(iupqmlTemplateItem(IUPQML_IMPORTS "Button { text: \"X\" }"));
}

static int qmlButtonAlignmentFlags(Ihandle* ih)
{
  int h = ih->data->horiz_alignment == IUP_ALIGN_ALEFT ? Qt::AlignLeft : ih->data->horiz_alignment == IUP_ALIGN_ARIGHT ? Qt::AlignRight : Qt::AlignHCenter;
  int v = ih->data->vert_alignment == IUP_ALIGN_ATOP ? Qt::AlignTop : ih->data->vert_alignment == IUP_ALIGN_ABOTTOM ? Qt::AlignBottom : Qt::AlignVCenter;
  return h | v;
}

IUP_DRV_API QQuickItem* iupqmlButtonImageContent(QQuickItem* button)
{
  auto* custom_item = button->property("_iup_content").value<QQuickItem*>();
  if (!custom_item)
  {
    QQuickItem* original = iupqmlGetItemProperty(button, "contentItem");
    custom_item = iupqmlCreateItem(qml_button_content);
    if (!custom_item || !original)
      return nullptr;

    custom_item->setParent(button);
    custom_item->setProperty("control", QVariant::fromValue<QQuickItem*>(button));
    button->setProperty("_iup_original_content", QVariant::fromValue<QQuickItem*>(original));
    button->setProperty("_iup_content", QVariant::fromValue<QQuickItem*>(custom_item));
  }

  if (iupqmlGetItemProperty(button, "contentItem") != custom_item)
  {
    button->setProperty("contentItem", QVariant::fromValue<QQuickItem*>(custom_item));
    custom_item->setVisible(true);
  }
  return custom_item;
}

static void qmlButtonSetContent(Ihandle* ih, QQuickItem* button, int custom)
{
  if (custom)
  {
    QQuickItem* custom_item = iupqmlButtonImageContent(button);
    if (!custom_item)
      return;

    custom_item->setProperty("vertical", ih->data->img_position == IUP_IMGPOS_TOP || ih->data->img_position == IUP_IMGPOS_BOTTOM);
    custom_item->setProperty("imageFirst", ih->data->img_position == IUP_IMGPOS_LEFT || ih->data->img_position == IUP_IMGPOS_TOP);
    custom_item->setProperty("alignment", qmlButtonAlignmentFlags(ih));
    return;
  }

  auto* custom_item = button->property("_iup_content").value<QQuickItem*>();
  auto* original = button->property("_iup_original_content").value<QQuickItem*>();
  if (custom_item && original && iupqmlGetItemProperty(button, "contentItem") == custom_item)
  {
    button->setProperty("contentItem", QVariant::fromValue<QQuickItem*>(original));
    original->setVisible(true);
  }
}

IUP_DRV_API QQuickItem* iupqmlCreateMeasureButton(Ihandle* ih, int checkable)
{
  QQuickItem* button = iupqmlButtonFrameInset(iupqmlCreateItem(checkable ? IUPQML_IMPORTS "Button { checkable: true }" : IUPQML_IMPORTS "Button { }"));
  if (!button)
    return nullptr;

  char* name = iupAttribGet(ih, "IMAGE");
  QPixmap* pixmap = name ? static_cast<QPixmap*>(iupImageGetImage(name, ih, 0, iupBaseNativeParentGetBgColorAttrib(ih))) : nullptr;
  if (pixmap && !pixmap->isNull())
  {
    iupqmlSetProperty(button, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
    iupqmlSetProperty(button, "icon.width", pixmap->width());
    iupqmlSetProperty(button, "icon.height", pixmap->height());
  }

  char* title = iupAttribGet(ih, "TITLE");
  if (!checkable && title && *title)
  {
    button->setProperty("text", QString::fromUtf8(title));
    button->setProperty("display", 2);
  }
  else
    button->setProperty("display", 0);

  if (pixmap && !pixmap->isNull())
    iupqmlButtonImageContent(button);

  iupqmlUpdateItemFont(ih, button);
  iupqmlMeasureItem(button);
  return button;
}

extern "C" IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
  int has_user_padding = 0;
  int has_bgcolor = 0;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    has_bgcolor = (!image && !(title && *title) && bgcolor != nullptr);
    has_user_padding = (ih->data->horiz_padding > 0 || ih->data->vert_padding > 0);

    if (has_bgcolor)
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      (*x) += charheight;
    }
  }

  int has_image = ih && iupAttribGet(ih, "IMAGE");
  std::unique_ptr<QQuickItem> measure(has_image ? iupqmlCreateMeasureButton(ih, 0) : nullptr);
  QQuickItem* button = has_image ? measure.get() : qmlButtonTemplate(ih);
  if (!button)
  {
    (*x) += 8;
    (*y) += 8;
    return;
  }

  int inset_x = static_cast<int>(button->property("leftInset").toDouble() + button->property("rightInset").toDouble());
  int inset_y = static_cast<int>(button->property("topInset").toDouble() + button->property("bottomInset").toDouble());

  if (has_user_padding)
  {
    int frame = inset_y + 2;
    if (has_image && !(iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER")))
    {
      int chrome = static_cast<int>(button->property("implicitBackgroundHeight").toDouble()) + inset_y - (*y);
      if (chrome > frame)
        frame = chrome;
    }
    (*x) += frame + inset_x - inset_y;
    (*y) += frame;
    return;
  }

  int pad_x = inset_x + static_cast<int>(button->property("leftPadding").toDouble() + button->property("rightPadding").toDouble());
  int pad_y = inset_y + static_cast<int>(button->property("topPadding").toDouble() + button->property("bottomPadding").toDouble());

  if (has_image)
  {
    char* title = iupAttribGet(ih, "TITLE");
    int height = (*y) + pad_y;
    int bg_h = static_cast<int>(button->property("implicitBackgroundHeight").toDouble()) + inset_y;
    int no_border = iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER");
    if (!no_border && height < bg_h)
      height = bg_h;

    if (!title || !*title)
    {
      int chrome = no_border ? pad_x : height - (*y);
      (*x) += chrome;
      (*y) = height;
      return;
    }

    (*x) += pad_x;
    int spacing = static_cast<int>(button->property("spacing").toDouble());
    if (spacing > ih->data->spacing)
      (*x) += spacing - ih->data->spacing;
    (*y) = height;
    return;
  }

  (*x) += pad_x;
  (*y) += pad_y;

  int min_w = static_cast<int>(button->property("implicitBackgroundWidth").toDouble());
  int min_h = static_cast<int>(button->property("implicitBackgroundHeight").toDouble());
  if (*x < min_w) *x = min_w;
  if (*y < min_h) *y = min_h;
}

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qmlButtonSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return;

  if (!name)
  {
    iupqmlSetProperty(button, "icon.source", QUrl());
    return;
  }

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(name, ih, make_inactive, bgcolor));

  if (pixmap && !pixmap->isNull())
  {
    iupqmlSetProperty(button, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
    iupqmlSetProperty(button, "icon.width", pixmap->width());
    iupqmlSetProperty(button, "icon.height", pixmap->height());
    iupqmlSetProperty(button, "icon.color", QColor(Qt::transparent));
    iupqmlSetProperty(button, "icon.cache", false);
  }
  else
    iupqmlSetProperty(button, "icon.source", QUrl());
}

static void qmlButtonUpdateLayout(Ihandle* ih)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return;

  int display;
  if (ih->data->type == IUP_BUTTON_IMAGE)
    display = 0;
  else if (ih->data->type == IUP_BUTTON_TEXT)
    display = 1;
  else if (ih->data->img_position == IUP_IMGPOS_TOP)
    display = 3;
  else
    display = 2;

  button->setProperty("display", display);
  button->setProperty("spacing", static_cast<double>(ih->data->spacing));
  qmlButtonSetContent(ih, button, ih->data->type & IUP_BUTTON_IMAGE);
}

static void qmlButtonUpdatePadding(Ihandle* ih)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return;

  int horiz = ih->data->horiz_padding, vert = ih->data->vert_padding;
  int no_border = (ih->data->type & IUP_BUTTON_IMAGE) && iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER");
  if (no_border || horiz > 0 || vert > 0)
    iupqmlSetPaddings(button, horiz, horiz, vert, vert);
  else
    iupqmlRestorePaddings(button, qmlButtonTemplate(ih));
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static QQuickItem* qmlButtonLabelItem(QQuickItem* content)
{
  if (!content)
    return nullptr;
  for (QQuickItem* child : content->childItems())
  {
    if (strcmp(child->metaObject()->className(), "QQuickMnemonicLabel") == 0)
      return child;
  }
  return nullptr;
}

static void qmlButtonSetMarkupText(QQuickItem* button, const char* value)
{
  char* stripped = iupMarkupStripTags(value ? value : "");
  char* html = iupMarkupToHtml(value ? value : "");
  button->setProperty("text", QString::fromUtf8(stripped).replace('&', "&&"));
  QQuickItem* content = iupqmlGetItemProperty(button, "contentItem");
  if (content)
  {
    QQuickItem* label = qmlButtonLabelItem(content);
    content->setProperty(label ? "markup" : "text", QString::fromUtf8(html).replace('&', "&&"));
    if (label)
      label->setProperty("textFormat", 1);
  }
  free(html);
  free(stripped);
}

static int qmlButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
    if (button)
    {
      if (iupAttribGetBoolean(ih, "MARKUP"))
        qmlButtonSetMarkupText(button, value);
      else
      {
        QQuickItem* content = iupqmlGetItemProperty(button, "contentItem");
        QQuickItem* label = qmlButtonLabelItem(content);
        if (label)
        {
          content->setProperty("markup", QString());
          label->setProperty("textFormat", 2);
        }
        iupqmlSetMnemonicTitle(ih, button, value);
      }

      qmlButtonUpdateLayout(ih);
      return 1;
    }
  }

  return 0;
}

static char* qmlButtonGetAlignmentAttrib(Ihandle* ih)
{
  char* horiz_align2str[3] = {const_cast<char*>("ALEFT"), const_cast<char*>("ACENTER"), const_cast<char*>("ARIGHT")};
  char* vert_align2str[3] = {const_cast<char*>("ATOP"), const_cast<char*>("ACENTER"), const_cast<char*>("ABOTTOM")};
  return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment],
                                   vert_align2str[ih->data->vert_alignment]);
}

static int qmlButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
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

  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (button)
  {
    int flags = qmlButtonAlignmentFlags(ih);
    QQuickItem* content = iupqmlGetItemProperty(button, "contentItem");
    if (content)
      content->setProperty("alignment", flags);

    auto* custom_item = button->property("_iup_content").value<QQuickItem*>();
    if (custom_item && custom_item != content)
      custom_item->setProperty("alignment", flags);
    auto* original = button->property("_iup_original_content").value<QQuickItem*>();
    if (original && original != content)
      original->setProperty("alignment", flags);
  }

  return 1;
}

static char* qmlButtonGetSpacingAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->spacing);
}

static int qmlButtonSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->spacing = 2;
  else
    iupStrToInt(value, &ih->data->spacing);

  if (ih->handle)
    qmlButtonUpdateLayout(ih);

  return 0;
}

static char* qmlButtonGetImagePositionAttrib(Ihandle* ih)
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

static int qmlButtonSetImagePositionAttrib(Ihandle* ih, const char* value)
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
    qmlButtonUpdateLayout(ih);

  return 0;
}

static int qmlButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  qmlButtonUpdatePadding(ih);

  return 0;
}

static char* qmlButtonGetBgColorAttrib(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE || iupAttribGet(ih, "IMPRESS"))
    return iupBaseNativeParentGetBgColorAttrib(ih);

  return nullptr;
}

static int qmlButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!value || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (button)
  {
    iupqmlSetPaletteColor(button, "button", QColor(r, g, b));
    if (ih->data->type == IUP_BUTTON_TEXT && !iupAttribGet(ih, "TITLE"))
      iupqmlSetProperty(button, "background.color", QColor(r, g, b));
  }
  return 1;
}

static int qmlButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (button)
  {
    iupqmlSetPaletteColor(button, "buttonText", QColor(r, g, b));
    return 1;
  }

  return 0;
}

static int qmlButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
    iupqmlUpdateItemFont(ih, reinterpret_cast<QObject*>(ih->handle));

  return 1;
}

static int qmlButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (iupdrvIsActive(ih))
      qmlButtonSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        qmlButtonSetPixmap(ih, value, 1);
    }

    qmlButtonUpdateLayout(ih);
    return 1;
  }

  return 0;
}

static int qmlButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        qmlButtonSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        qmlButtonSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }

  return 0;
}

static void qmlButtonApplyFlat(Ihandle* ih, int flat)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return;

  button->setProperty("flat", flat ? true : false);
  qmlButtonUpdatePadding(ih);
}

static int qmlButtonSetImPressAttrib(Ihandle* ih, const char* value)
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
      qmlButtonApplyFlat(ih, should_be_flat);
    }

    return 1;
  }

  return 0;
}

static int qmlButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        qmlButtonSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        qmlButtonSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      qmlButtonSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int qmlButtonSetFlatAttrib(Ihandle* ih, const char* value)
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
    qmlButtonApplyFlat(ih, should_be_flat);
    return 0;
  }
  return 1;
}

static int qmlButtonSetShowAsDefaultAttrib(Ihandle* ih, const char* value)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (button)
    button->setProperty("highlighted", iupStrBoolean(value) ? true : false);
  return 1;
}

/****************************************************************************
 * Events
 ****************************************************************************/

class IupQmlButtonKeyFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlButtonKeyFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (event->type() == QEvent::KeyPress && iupObjectCheck(ih))
    {
      auto* key = static_cast<QKeyEvent*>(event);
      if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
      {
        QMetaObject::invokeMethod(obj, "clicked", Qt::DirectConnection);
        return true;
      }
    }
    return false;
  }
};

static void qmlButtonClicked(Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static void qmlButtonLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  qmlButtonUpdateLayout(ih);
}

static int qmlButtonMapMethod(Ihandle* ih)
{
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

  QQuickItem* button = iupqmlButtonFrameInset(iupqmlCreateItem(IUPQML_IMPORTS "Button { }"));
  if (!button)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(button);

  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      qmlButtonSetTitleAttrib(ih, title);
  }

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      qmlButtonSetPixmap(ih, image, 0);
  }

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, button);
  button->installEventFilter(new IupQmlButtonKeyFilter(button, ih));

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(button, 0);

  if (iupAttribGetBoolean(ih, "FLAT"))
    qmlButtonApplyFlat(ih, 1);

  if (ih->data->type == IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    qmlButtonApplyFlat(ih, 1);

  iupqmlConnect(button, "clicked()", [ih](void**) {
    qmlButtonClicked(ih);
  });

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    iupqmlConnect(button, "pressed()", [ih](void**) {
      char* name = iupAttribGet(ih, "IMPRESS");
      if (name)
        qmlButtonSetPixmap(ih, name, 0);
    });
    iupqmlConnect(button, "released()", [ih](void**) {
      if (!iupAttribGet(ih, "IMPRESS"))
        return;
      char* name = iupAttribGet(ih, "IMAGE");
      if (name)
        qmlButtonSetPixmap(ih, name, 0);
    });
  }

  value = iupAttribGet(ih, "PADDING");
  if (value)
    qmlButtonSetPaddingAttrib(ih, value);

  value = iupAttribGet(ih, "SPACING");
  if (value)
    qmlButtonSetSpacingAttrib(ih, value);

  value = iupAttribGet(ih, "IMAGEPOSITION");
  if (value)
    qmlButtonSetImagePositionAttrib(ih, value);

  qmlButtonUpdateLayout(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = qmlButtonMapMethod;
  ic->LayoutUpdate = qmlButtonLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, qmlButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", qmlButtonGetBgColorAttrib, qmlButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", nullptr, qmlButtonSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", qmlButtonGetAlignmentAttrib, qmlButtonSetAlignmentAttrib, "ACENTER:ACENTER", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qmlButtonSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", nullptr, qmlButtonSetImInactiveAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, qmlButtonSetImPressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, qmlButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SPACING", qmlButtonGetSpacingAttrib, qmlButtonSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "2", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", qmlButtonGetImagePositionAttrib, qmlButtonSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "FLAT", nullptr, qmlButtonSetFlatAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARKUP", nullptr, nullptr, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SHOWASDEFAULT", nullptr, qmlButtonSetShowAsDefaultAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
}
