/** \file
 * \brief Frame Control - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QString>
#include <QColor>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_frame.h"
}

#include "iupqml_drv.h"


static QQuickItem* qmlFrameTemplate(Ihandle* ih, int has_title)
{
  QQuickItem* box;
  if (has_title)
    box = iupqmlTemplateItem(IUPQML_IMPORTS "GroupBox { padding: 3; spacing: 2; title: \"X\"; contentItem: Item { } }");
  else
    box = iupqmlTemplateItem(IUPQML_IMPORTS "GroupBox { padding: 3; spacing: 2; contentItem: Item { } }");

  if (box && ih)
  {
    QFont* font = iupqmlGetIhFont(ih);
    if (font)
      box->setProperty("font", QVariant::fromValue(*font));
  }
  return box;
}

static int qmlFrameHasTitle(Ihandle* ih)
{
  const char* title = iupAttribGet(ih, "TITLE");
  return iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || (title && *title);
}

extern "C" IUP_SDK_API void iupdrvFrameGetDecorOffset(Ihandle* ih, int* x, int* y)
{
  QQuickItem* box = qmlFrameTemplate(ih, qmlFrameHasTitle(ih));
  *x = box ? static_cast<int>(box->property("leftPadding").toDouble()) : 3;
  *y = box ? static_cast<int>(box->property("topPadding").toDouble()) : 3;
}

extern "C" IUP_SDK_API int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" IUP_SDK_API int iupdrvFrameGetTitleHeight(Ihandle* ih, int* h)
{
  QQuickItem* box = qmlFrameTemplate(ih, 1);
  *h = box ? static_cast<int>(box->property("implicitLabelHeight").toDouble()) : 0;
  if (*h <= 0)
    iupdrvFontGetCharSize(ih, nullptr, h);
  return 1;
}

extern "C" IUP_SDK_API int iupdrvFrameGetDecorSize(Ihandle* ih, int* w, int* h)
{
  QQuickItem* box = qmlFrameTemplate(ih, qmlFrameHasTitle(ih));
  if (!box)
  {
    *w = 6;
    *h = qmlFrameHasTitle(ih) ? 24 : 6;
    return 1;
  }

  *w = static_cast<int>(box->property("leftPadding").toDouble() + box->property("rightPadding").toDouble());
  *h = static_cast<int>(box->property("topPadding").toDouble() + box->property("bottomPadding").toDouble());
  return 1;
}

/****************************************************************************
 * Attributes
 ****************************************************************************/

static int qmlFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    auto* box = reinterpret_cast<QQuickItem*>(ih->handle);
    if (box)
    {
      box->setProperty("title", value ? QString::fromUtf8(value) : QString());
      return 1;
    }
  }
  return 0;
}

static char* qmlFrameGetTitleAttrib(Ihandle* ih)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    auto* box = reinterpret_cast<QQuickItem*>(ih->handle);
    if (box)
    {
      QString title = box->property("title").toString();
      if (!title.isEmpty())
        return iupStrReturnStr(title.toUtf8().constData());
    }
  }
  return nullptr;
}

static void qmlFrameUpdateSunken(Ihandle* ih)
{
  auto* box = reinterpret_cast<QQuickItem*>(ih->handle);
  if (!box)
    return;

  auto* shade = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_FRAME_SHADE"));
  if (!iupAttribGet(ih, "_IUPFRAME_SUNKEN"))
  {
    if (shade)
    {
      delete shade;
      iupAttribSet(ih, "_IUPQML_FRAME_SHADE", nullptr);
    }
    return;
  }

  QQuickItem* background = iupqmlGetItemProperty(box, "background");
  if (shade || !background)
    return;

  shade = iupqmlCreateItem("import QtQuick\nItem { anchors.fill: parent\n"
                           "  Rectangle { anchors.fill: parent; color: \"transparent\"; border.color: palette.dark }\n"
                           "  Rectangle { anchors.fill: parent; anchors.leftMargin: 1; anchors.topMargin: 1; color: \"transparent\"; border.color: palette.light }\n"
                           "}");
  if (!shade)
    return;
  shade->setParentItem(background);
  shade->setParent(background);
  iupAttribSet(ih, "_IUPQML_FRAME_SHADE", reinterpret_cast<char*>(shade));
}

static int qmlFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    iupAttribSet(ih, "_IUPFRAME_SUNKEN", iupStrBoolean(value) ? "1" : nullptr);
    qmlFrameUpdateSunken(ih);
    return 1;
  }
  return 0;
}

static char* qmlFrameGetSunkenAttrib(Ihandle* ih)
{
  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
    return iupStrReturnBoolean(iupAttribGet(ih, "_IUPFRAME_SUNKEN") != nullptr);
  return nullptr;
}

static int qmlFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  auto* box = reinterpret_cast<QQuickItem*>(ih->handle);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    value = iupBaseNativeParentGetBgColor(ih);

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (box)
  {
    iupqmlSetPaletteColor(box, "window", QColor(r, g, b));
    if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
      iupqmlSetProperty(box, "background.color", QColor(r, g, b));
  }

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 1;
  else
    return 0;
}

static int qmlFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  auto* box = reinterpret_cast<QQuickItem*>(ih->handle);

  if (!box || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(box, "windowText", QColor(r, g, b));
  iupqmlSetProperty(box, "label.color", QColor(r, g, b));
  return 1;
}

static int qmlFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
    iupqmlUpdateItemFont(ih, reinterpret_cast<QObject*>(ih->handle));

  return 1;
}

/****************************************************************************
 * Container
 ****************************************************************************/

static void* qmlFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUPQML_FRAME_INNER");
}

static void qmlFrameLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  auto* box = reinterpret_cast<QQuickItem*>(ih->handle);
  auto* inner = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_FRAME_INNER"));
  if (box && inner)
  {
    int dw, dh, dx, dy;
    iupdrvFrameGetDecorSize(ih, &dw, &dh);
    iupdrvFrameGetDecorOffset(ih, &dx, &dy);
    inner->setPosition(QPointF(dx, dy));
    inner->setSize(QSizeF(ih->currentwidth - dw > 0 ? ih->currentwidth - dw : 0,
                          ih->currentheight - dh > 0 ? ih->currentheight - dh : 0));
  }
}

static int qmlFrameMapMethod(Ihandle* ih)
{
  char* title;

  if (!ih->parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");
  if (title)
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  else if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
    iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");

  QQuickItem* box = iupqmlCreateItem(IUPQML_IMPORTS "GroupBox { padding: 3; spacing: 2; contentItem: Item { objectName: \"content\" } }");
  if (!box)
    return IUP_ERROR;

  if (title)
    box->setProperty("title", QString::fromUtf8(title));

  ih->handle = reinterpret_cast<InativeHandle*>(box);

  QQuickItem* inner = iupqmlGetItemProperty(box, "contentItem");
  if (!inner)
    inner = box->findChild<QQuickItem*>("content");
  iupAttribSet(ih, "_IUPQML_FRAME_INNER", reinterpret_cast<char*>(inner));

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, box);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    qmlFrameSetBgColorAttrib(ih, nullptr);

  return IUP_NOERROR;
}

static void qmlFrameUnMapMethod(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPQML_FRAME_INNER", nullptr);
  iupAttribSet(ih, "_IUPQML_FRAME_SHADE", nullptr);
  iupdrvBaseUnMapMethod(ih);
}

extern "C" IUP_SDK_API void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = qmlFrameMapMethod;
  ic->UnMap = qmlFrameUnMapMethod;
  ic->LayoutUpdate = qmlFrameLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = qmlFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, qmlFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, qmlFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", qmlFrameGetSunkenAttrib, qmlFrameSetSunkenAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", qmlFrameGetTitleAttrib, qmlFrameSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
