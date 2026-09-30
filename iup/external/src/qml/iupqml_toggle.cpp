/** \file
 * \brief Toggle Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstring>
#include <QQuickItem>
#include <QPixmap>
#include <QString>
#include <QUrl>
#include <QColor>
#include <QEvent>

#include <cstdlib>
#include <cmath>
#include <memory>

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
#include "iup_key.h"
#include "iup_markup.h"
}

#include "iupqml_drv.h"

enum{IUP_IMGPOS_LEFT, IUP_IMGPOS_RIGHT, IUP_IMGPOS_TOP, IUP_IMGPOS_BOTTOM};

/****************************************************************************
 * Size Helpers
 ****************************************************************************/

static QQuickItem* qmlToggleImageTemplate()
{
  return iupqmlButtonFrameInset(iupqmlTemplateItem(IUPQML_IMPORTS "Button { checkable: true; icon.name: \"iup\" }"));
}

extern "C" IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
  std::unique_ptr<QQuickItem> measure(ih && iupAttribGet(ih, "IMAGE") ? iupqmlCreateMeasureButton(ih, 1) : nullptr);
  QQuickItem* button = measure ? measure.get() : qmlToggleImageTemplate();
  int has_user_padding = 0;

  if (ih)
  {
    int horiz_padding = 0, vert_padding = 0;
    char* padding = IupGetAttribute(ih, "PADDING");
    if (padding)
      iupStrToIntInt(padding, &horiz_padding, &vert_padding, 'x');
    has_user_padding = (horiz_padding > 0 || vert_padding > 0);
  }

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
    (*x) += inset_x + 2;
    (*y) += inset_y + 2;
    return;
  }

  int pad_x = inset_x + static_cast<int>(button->property("leftPadding").toDouble() + button->property("rightPadding").toDouble());
  int pad_y = inset_y + static_cast<int>(button->property("topPadding").toDouble() + button->property("bottomPadding").toDouble());

  if (ih && iupAttribGetBoolean(ih, "FLAT"))
  {
    (*x) += pad_x;
    (*y) += pad_y;
    return;
  }

  int height = (*y) + pad_y;
  int bg_h = static_cast<int>(button->property("implicitBackgroundHeight").toDouble()) + inset_y;
  if (height < bg_h)
    height = bg_h;

  (*x) += height - (*y);
  (*y) = height;
}

static void qmlToggleIndicatorSize(QQuickItem* control, int* w, int* h)
{
  *w = static_cast<int>(control->property("implicitIndicatorWidth").toDouble());
  *h = static_cast<int>(control->property("implicitIndicatorHeight").toDouble());

  QQuickItem* indicator = iupqmlGetItemProperty(control, "indicator");
  if (indicator)
  {
    if (*w <= 0) *w = static_cast<int>(std::ceil(indicator->width()));
    if (*h <= 0) *h = static_cast<int>(std::ceil(indicator->height()));
  }
}

extern "C" IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int* x, int* y, const char* str)
{
  QQuickItem* sw = iupqmlTemplateItem(IUPQML_IMPORTS "Switch { text: \"X\" }");
  int switch_w = 50, switch_h = 26, spacing = 6, pad_x = 12, pad_y = 12;
  (void)ih;

  if (sw)
  {
    qmlToggleIndicatorSize(sw, &switch_w, &switch_h);
    spacing = static_cast<int>(sw->property("spacing").toDouble());
    pad_x = static_cast<int>(sw->property("leftPadding").toDouble() + sw->property("rightPadding").toDouble());
    pad_y = static_cast<int>(sw->property("topPadding").toDouble() + sw->property("bottomPadding").toDouble());
  }

  (*x) += switch_w + pad_x;
  if ((*y) < switch_h + pad_y) (*y) = switch_h + pad_y;
  else (*y) += pad_y;

  if (str && str[0])
    (*x) += spacing;
}

extern "C" IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
  QQuickItem* cb;
  if (iupRadioFindToggleParent(ih))
    cb = iupqmlTemplateItem(IUPQML_IMPORTS "RadioButton { text: \"X\" }");
  else
    cb = iupqmlTemplateItem(IUPQML_IMPORTS "CheckBox { text: \"X\" }");
  int box_w = 18, box_h = 18, spacing = 6, pad_x = 12, pad_y = 12;

  if (cb)
  {
    qmlToggleIndicatorSize(cb, &box_w, &box_h);
    spacing = static_cast<int>(cb->property("spacing").toDouble());
    pad_x = static_cast<int>(cb->property("leftPadding").toDouble() + cb->property("rightPadding").toDouble());
    pad_y = static_cast<int>(cb->property("topPadding").toDouble() + cb->property("bottomPadding").toDouble());
  }

  (*x) += box_w + pad_x;
  if ((*y) < box_h + pad_y)
    (*y) = box_h + pad_y;
  else
    (*y) += pad_y;

  if (str && str[0])
  {
    int user_spacing = iupAttribGetInt(ih, "SPACING");
    (*x) += user_spacing > 0 ? user_spacing : spacing;
  }
}

/****************************************************************************
 * Helpers
 ****************************************************************************/

static int qmlToggleGetCheck(Ihandle* ih)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return 0;

  if (iupAttribGetBoolean(ih, "3STATE"))
  {
    int state = button->property("checkState").toInt();
    if (state == Qt::PartiallyChecked)
      return -1;
    return state == Qt::Checked ? 1 : 0;
  }

  return button->property("checked").toBool() ? 1 : 0;
}

static void qmlToggleSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!name || ih->data->type != IUP_TOGGLE_IMAGE || !button)
    return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(name, ih, make_inactive, bgcolor));

  if (pixmap && !pixmap->isNull())
  {
    iupqmlSetProperty(button, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
    iupqmlSetProperty(button, "icon.width", pixmap->width());
    iupqmlSetProperty(button, "icon.height", pixmap->height());
    iupqmlSetProperty(button, "icon.color", QColor(Qt::transparent));
    iupqmlSetProperty(button, "icon.cache", false);
    iupqmlButtonImageContent(button);
  }
  else
    iupqmlSetProperty(button, "icon.source", QUrl());
}

static void qmlToggleUpdateImage(Ihandle* ih, int active, int check)
{
  char* name;

  if (!active)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (name)
      qmlToggleSetPixmap(ih, name, 0);
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      qmlToggleSetPixmap(ih, name, 1);
    }
  }
  else
  {
    if (check)
    {
      name = iupAttribGet(ih, "IMPRESS");
      if (name)
        qmlToggleSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        qmlToggleSetPixmap(ih, name, 0);
      }
    }
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      if (name)
        qmlToggleSetPixmap(ih, name, 0);
    }
  }
}

static void qmlToggleUpdateLayout(Ihandle* ih)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return;

  int spacing = iupAttribGetInt(ih, "SPACING");
  if (spacing > 0)
    button->setProperty("spacing", static_cast<double>(spacing));

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    button->setProperty("display", 0);
  }
}

static void qmlToggleSetChecked(Ihandle* ih, int check)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!button)
    return;

  iupAttribSet(ih, "_IUPQML_IGNORE_TOGGLE", "1");
  if (iupAttribGetBoolean(ih, "3STATE"))
    button->setProperty("checkState", check == -1 ? static_cast<int>(Qt::PartiallyChecked) : check ? static_cast<int>(Qt::Checked) : static_cast<int>(Qt::Unchecked));
  else
    button->setProperty("checked", check ? true : false);
  iupAttribSet(ih, "_IUPQML_IGNORE_TOGGLE", nullptr);
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qmlToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "NOTDEF"))
  {
    if (iupAttribGetBoolean(ih, "3STATE"))
      qmlToggleSetChecked(ih, -1);
  }
  else if (iupStrEqualNoCase(value, "TOGGLE"))
    qmlToggleSetChecked(ih, !qmlToggleGetCheck(ih));
  else
    qmlToggleSetChecked(ih, iupStrBoolean(value));

  if (ih->data->is_radio && qmlToggleGetCheck(ih))
  {
    Ihandle* radio = iupRadioFindToggleParent(ih);
    if (radio)
    {
      auto* last = reinterpret_cast<Ihandle*>(iupAttribGet(radio, "_IUPQML_LASTRADIO"));
      if (last && last != ih && iupObjectCheck(last) && last->handle)
        qmlToggleSetChecked(last, 0);
      iupAttribSet(radio, "_IUPQML_LASTRADIO", reinterpret_cast<char*>(ih));
    }
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qmlToggleUpdateImage(ih, iupdrvIsActive(ih), qmlToggleGetCheck(ih));

  return 0;
}

static char* qmlToggleGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(qmlToggleGetCheck(ih));
}

static void qmlToggleSetText(Ihandle* ih, QQuickItem* button, const char* value)
{
  char c = 0;
  char* str = iupStrProcessMnemonic(value ? value : "", &c, -1);
  QString plain = QString::fromUtf8(str ? str : "");
  QString html = plain.toHtmlEscaped();

  if (str && str != value)
  {
    if (c)
    {
      int idx = plain.indexOf(QChar(c), 0, Qt::CaseInsensitive);
      if (idx >= 0 && iupqmlMnemonicVisible())
        html = plain.left(idx).toHtmlEscaped() + "<u>" + plain.mid(idx, 1).toHtmlEscaped() + "</u>" + plain.mid(idx + 1).toHtmlEscaped();
      if (IupGetDialog(ih))
        iupKeySetMnemonic(ih, c, -1);
    }
    free(str);
  }

  button->setProperty("text", QString(plain).replace('&', "&&"));
  iupqmlSetProperty(button, "contentItem.textFormat", 4);
  iupqmlSetProperty(button, "contentItem.elide", 3);
  iupqmlSetProperty(button, "contentItem.text", html);
}

static void qmlToggleMnemonicRefresh(Ihandle* ih)
{
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
  if (button && ih->data->type == IUP_TOGGLE_TEXT && !iupAttribGetBoolean(ih, "MARKUP"))
    qmlToggleSetText(ih, button, iupAttribGet(ih, "TITLE"));
}

static int qmlToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_radio && iupAttribGetBoolean(ih, "SWITCH"))
    iupToggleSwitchSetAccessibleTitle(ih, value);

  if (ih->data->type == IUP_TOGGLE_TEXT && !(!ih->data->is_radio && iupAttribGetBoolean(ih, "SWITCH")))
  {
    auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
    if (button)
    {
      if (iupAttribGetBoolean(ih, "MARKUP"))
      {
        char* stripped = iupMarkupStripTags(value ? value : "");
        char* html = iupMarkupToHtml(value ? value : "");
        button->setProperty("text", QString::fromUtf8(stripped).replace('&', "&&"));
        iupqmlSetProperty(button, "contentItem.textFormat", 1);
        iupqmlSetProperty(button, "contentItem.elide", 3);
        iupqmlSetProperty(button, "contentItem.text", QString::fromUtf8(html));
        free(html);
        free(stripped);
      }
      else
      {
        qmlToggleSetText(ih, button, value);
        if (value && strchr(value, '&'))
          iupqmlMnemonicRegister(ih, qmlToggleMnemonicRefresh);
      }

      qmlToggleUpdateLayout(ih);
      return 1;
    }
  }

  return 0;
}

static int qmlToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
    int horiz = ih->data->horiz_padding, vert = ih->data->vert_padding;
    if (horiz > 0 || vert > 0)
      iupqmlSetPaddings(button, horiz, horiz, vert, vert);
    else
      iupqmlRestorePaddings(button, qmlToggleImageTemplate());
    return 0;
  }

  return 1;
}

static int qmlToggleSetSpacingAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
    qmlToggleUpdateLayout(ih);
  return 1;
}

static int qmlToggleSetImagePositionAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_TOGGLE_IMAGE)
    return 0;

  iupAttribSet(ih, "IMAGEPOSITION", const_cast<char*>(value));

  if (ih->handle)
    qmlToggleUpdateLayout(ih);

  return 1;
}

static int qmlToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  auto* button = reinterpret_cast<QQuickItem*>(ih->handle);

  if (!button || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(button, "windowText", QColor(r, g, b));
  iupqmlSetPaletteColor(button, "buttonText", QColor(r, g, b));
  iupqmlSetPaletteColor(button, "text", QColor(r, g, b));

  return 1;
}

static char* qmlToggleGetBgColorAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return iupBaseNativeParentGetBgColorAttrib(ih);
  else
  {
    unsigned char r, g, b;
    char* color = iupBaseNativeParentGetBgColorAttrib(ih);
    if (iupStrToRGB(color, &r, &g, &b))
      return iupStrReturnRGB(r, g, b);
    return nullptr;
  }
}

static int qmlToggleSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
    iupqmlUpdateItemFont(ih, reinterpret_cast<QObject*>(ih->handle));

  return 1;
}

static int qmlToggleSetMarkupAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    if (iupStrBoolean(value))
      iupAttribSet(ih, "MARKUP", "1");
    else
      iupAttribSet(ih, "MARKUP", nullptr);

    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      qmlToggleSetTitleAttrib(ih, title);
  }
  return 0;
}

static int qmlToggleSetRightButtonAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT && ih->handle)
  {
    auto* button = reinterpret_cast<QQuickItem*>(ih->handle);
    button->setProperty("iupMirror", iupStrBoolean(value) ? true : false);
    return 1;
  }
  return 0;
}

static int qmlToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMAGE"))
      iupAttribSet(ih, "IMAGE", const_cast<char*>(value));

    qmlToggleUpdateImage(ih, iupdrvIsActive(ih), qmlToggleGetCheck(ih));
    return 1;
  }

  return 0;
}

static int qmlToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMINACTIVE"))
      iupAttribSet(ih, "IMINACTIVE", const_cast<char*>(value));

    qmlToggleUpdateImage(ih, iupdrvIsActive(ih), qmlToggleGetCheck(ih));
    return 1;
  }

  return 0;
}

static int qmlToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMPRESS"))
      iupAttribSet(ih, "IMPRESS", const_cast<char*>(value));

    qmlToggleUpdateImage(ih, iupdrvIsActive(ih), qmlToggleGetCheck(ih));
    return 1;
  }

  return 0;
}

static int qmlToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qmlToggleUpdateImage(ih, iupStrBoolean(value), qmlToggleGetCheck(ih));

  return iupBaseSetActiveAttrib(ih, value);
}

/****************************************************************************
 * Events
 ****************************************************************************/

static void qmlToggleToggled(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPQML_IGNORE_TOGGLE"))
    return;

  int check = qmlToggleGetCheck(ih);

  if (ih->data->is_radio)
  {
    Ihandle* radio = iupRadioFindToggleParent(ih);
    Ihandle* last = radio ? reinterpret_cast<Ihandle*>(iupAttribGet(radio, "_IUPQML_LASTRADIO")) : nullptr;

    if (!check)
    {
      if (last == ih)
        qmlToggleSetChecked(ih, 1);
      return;
    }

    if (last && last != ih && iupObjectCheck(last) && last->handle)
    {
      qmlToggleSetChecked(last, 0);
      if (last->data->type == IUP_TOGGLE_IMAGE)
        qmlToggleUpdateImage(last, iupdrvIsActive(last), 0);

      IFni last_cb = reinterpret_cast<IFni>(IupGetCallback(last, "ACTION"));
      if (last_cb && last_cb(last, 0) == IUP_CLOSE)
        IupExitLoop();
      if (iupObjectCheck(last))
        iupBaseCallValueChangedCb(last);
    }
    if (radio)
      iupAttribSet(radio, "_IUPQML_LASTRADIO", reinterpret_cast<char*>(ih));
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qmlToggleUpdateImage(ih, iupdrvIsActive(ih), check);

  IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "ACTION"));
  if (cb && cb(ih, check) == IUP_CLOSE)
    IupExitLoop();

  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);
}

class IupQmlToggleClickFilter : public QObject
{
public:
  IupQmlToggleClickFilter(QObject* parent) : QObject(parent) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (event->type() == QEvent::MouseButtonDblClick)
    {
      static_cast<QQuickItem*>(obj)->ungrabMouse();
      return true;
    }
    return false;
  }
};

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qmlToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  QQuickItem* button = nullptr;

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
      button = iupqmlButtonFrameInset(iupqmlCreateItem(IUPQML_IMPORTS "Button { checkable: true; autoExclusive: false }"));
    else
      button = iupqmlCreateItem(IUPQML_IMPORTS "RadioButton { autoExclusive: false; property bool iupMirror: false; LayoutMirroring.enabled: iupMirror }");

    ih->data->is_radio = 1;

    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT)
    {
      if (iupAttribGetBoolean(ih, "SWITCH"))
        button = iupqmlCreateItem(IUPQML_IMPORTS "Switch { property bool iupMirror: false; LayoutMirroring.enabled: iupMirror }");
      else if (iupAttribGetBoolean(ih, "3STATE"))
        button = iupqmlCreateItem(IUPQML_IMPORTS "CheckBox { tristate: true; property bool iupMirror: false; LayoutMirroring.enabled: iupMirror }");
      else
        button = iupqmlCreateItem(IUPQML_IMPORTS "CheckBox { property bool iupMirror: false; LayoutMirroring.enabled: iupMirror }");
    }
    else
      button = iupqmlButtonFrameInset(iupqmlCreateItem(IUPQML_IMPORTS "Button { checkable: true }"));
  }

  if (!button)
    return IUP_ERROR;

  ih->handle = reinterpret_cast<InativeHandle*>(button);

  if (radio && !iupAttribGet(radio, "_IUPQML_LASTRADIO"))
  {
    qmlToggleSetChecked(ih, 1);
    iupAttribSet(radio, "_IUPQML_LASTRADIO", reinterpret_cast<char*>(ih));
  }

  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      qmlToggleSetTitleAttrib(ih, title);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    qmlToggleUpdateImage(ih, 1, qmlToggleGetCheck(ih));

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, button);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(button, 0);

  if (ih->data->type == IUP_TOGGLE_IMAGE && iupAttribGetBoolean(ih, "FLAT"))
  {
    ih->data->flat = 1;
    button->setProperty("flat", true);
  }

  iupqmlConnect(button, "toggled()", [ih](void**) {
    qmlToggleToggled(ih);
  });

  if (iupAttribGetBoolean(ih, "IGNOREDOUBLECLICK"))
    button->installEventFilter(new IupQmlToggleClickFilter(button));

  value = iupAttribGet(ih, "PADDING");
  if (value)
    qmlToggleSetPaddingAttrib(ih, value);

  if (iupAttribGetBoolean(ih, "RIGHTBUTTON"))
    qmlToggleSetRightButtonAttrib(ih, "YES");

  value = iupAttribGet(ih, "IMAGEPOSITION");
  if (value)
    qmlToggleSetImagePositionAttrib(ih, value);

  qmlToggleUpdateLayout(ih);

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = qmlToggleMapMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, qmlToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", qmlToggleGetBgColorAttrib, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", nullptr, qmlToggleSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", nullptr, nullptr, "ACENTER:ACENTER", nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qmlToggleSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", nullptr, qmlToggleSetImInactiveAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", nullptr, qmlToggleSetImPressAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", qmlToggleGetValueAttrib, qmlToggleSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, qmlToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "SPACING", nullptr, qmlToggleSetSpacingAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", nullptr, qmlToggleSetImagePositionAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", nullptr, qmlToggleSetMarkupAttrib, nullptr, nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", nullptr, qmlToggleSetRightButtonAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IGNOREDOUBLECLICK", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "3STATE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
}
