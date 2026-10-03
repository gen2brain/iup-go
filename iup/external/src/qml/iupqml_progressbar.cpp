/** \file
 * \brief Progress bar Control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QColor>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"
}

#include "iupqml_drv.h"


static QQuickItem* qmlProgressBarGet(Ihandle* ih)
{
  return reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_PROGRESSBAR"));
}

extern "C" IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  QQuickItem* bar = iupqmlTemplateItem(IUPQML_IMPORTS "ProgressBar { }");
  int min_w = 120, min_h = 24;

  if (bar)
  {
    min_w = static_cast<int>(bar->implicitWidth());
    min_h = static_cast<int>(bar->implicitHeight());
    if (min_w < 1) min_w = 120;
    if (min_h < 1) min_h = 24;
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    *w = min_h;
    *h = min_w;
  }
  else
  {
    *w = min_w;
    *h = min_h;
  }
}

static void qmlProgressBarLayout(Ihandle* ih)
{
  auto* wrapper = reinterpret_cast<QQuickItem*>(ih->handle);
  QQuickItem* bar = qmlProgressBarGet(ih);
  if (!wrapper || !bar)
    return;

  int w = ih->currentwidth, h = ih->currentheight;

  if (!iupAttribGet(ih, "_IUPQML_CIRCULAR") && iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    bar->setSize(QSizeF(h, w));
    bar->setProperty("rotation", -90.0);
    bar->setProperty("transformOrigin", 0);
    bar->setPosition(QPointF(0, h));
  }
  else
  {
    bar->setPosition(QPointF(0, 0));
    bar->setSize(QSizeF(w, h));
  }
}

static void qmlProgressBarUpdateValue(Ihandle* ih)
{
  QQuickItem* bar = qmlProgressBarGet(ih);
  if (!bar || ih->data->marquee)
    return;

  bar->setProperty("from", ih->data->vmin);
  bar->setProperty("to", ih->data->vmax);
  bar->setProperty("value", ih->data->value);
}

static int qmlProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee || iupAttribGet(ih, "_IUPQML_CIRCULAR"))
    return 0;

  QQuickItem* bar = qmlProgressBarGet(ih);
  if (!bar)
    return 0;

  bar->setProperty("indeterminate", iupStrBoolean(value) ? true : false);
  return 1;
}

static int qmlProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->marquee)
    return 0;

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);
  qmlProgressBarUpdateValue(ih);
  return 0;
}

static int qmlProgressBarSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
  {
    iProgressBarCropValue(ih);
    qmlProgressBarUpdateValue(ih);
  }
  return 0;
}

static int qmlProgressBarSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
  {
    iProgressBarCropValue(ih);
    qmlProgressBarUpdateValue(ih);
  }
  return 0;
}

static char* qmlProgressBarGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static char* qmlProgressBarGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int qmlProgressBarSetDashedAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->marquee)
    return 0;

  ih->data->dashed = iupStrBoolean(value) ? 1 : 0;
  return 0;
}

static int qmlProgressBarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QQuickItem* bar = qmlProgressBarGet(ih);

  if (!bar || !iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupqmlSetPaletteColor(bar, "window", QColor(r, g, b));
  iupqmlSetPaletteColor(bar, "base", QColor(r, g, b));
  return 1;
}

static int qmlProgressBarSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QQuickItem* bar = qmlProgressBarGet(ih);

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (bar)
  {
    iupqmlSetPaletteColor(bar, "highlight", QColor(r, g, b));
    iupqmlSetPaletteColor(bar, "accent", QColor(r, g, b));
  }
  return 1;
}

static int qmlProgressBarMapMethod(Ihandle* ih)
{
  int circular = iupAttribGetBoolean(ih, "CIRCULAR");
  QQuickItem* wrapper = iupqmlCreateItem("import QtQuick\nItem { }");
  QQuickItem* bar = iupqmlCreateItem(circular ? IUPQML_IMPORTS "BusyIndicator { }" : IUPQML_IMPORTS "ProgressBar { }");
  if (!wrapper || !bar)
    return IUP_ERROR;

  bar->setParentItem(wrapper);
  bar->setParent(wrapper);
  iupAttribSet(ih, "_IUPQML_PROGRESSBAR", reinterpret_cast<char*>(bar));
  ih->handle = reinterpret_cast<InativeHandle*>(wrapper);

  if (circular)
  {
    iupAttribSet(ih, "_IUPQML_CIRCULAR", "1");
    ih->data->marquee = 1;
    iupqmlAddToParent(ih);
    iupqmlInstallFilter(ih, wrapper);
    iupqmlSetCanFocus(bar, 0);
    return IUP_NOERROR;
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }
    ih->expand = ih->expand & ~IUP_EXPAND_WIDTH;
  }
  else
    ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;

  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
    ih->data->marquee = 1;
    bar->setProperty("indeterminate", true);
  }
  else
  {
    ih->data->marquee = 0;
    qmlProgressBarUpdateValue(ih);
    if (iupAttribGetBoolean(ih, "DASHED"))
      ih->data->dashed = 1;
  }

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, wrapper);
  iupqmlSetCanFocus(bar, 0);

  return IUP_NOERROR;
}

static void qmlProgressBarUnMapMethod(Ihandle* ih)
{
  iupAttribSet(ih, "_IUPQML_PROGRESSBAR", nullptr);
  iupAttribSet(ih, "_IUPQML_CIRCULAR", nullptr);
  iupqmlTipsDestroy(ih);
  iupdrvBaseUnMapMethod(ih);
}

static void qmlProgressBarLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  qmlProgressBarLayout(ih);
}

extern "C" IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = qmlProgressBarMapMethod;
  ic->UnMap = qmlProgressBarUnMapMethod;
  ic->LayoutUpdate = qmlProgressBarLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlProgressBarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlProgressBarSetFgColorAttrib, nullptr, nullptr, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, qmlProgressBarSetValueAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIN", qmlProgressBarGetMinAttrib, qmlProgressBarSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", qmlProgressBarGetMaxAttrib, qmlProgressBarSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", iProgressBarGetDashedAttrib, qmlProgressBarSetDashedAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", nullptr, qmlProgressBarSetMarqueeAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWTEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
