/** \file
 * \brief Frame Control - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QGroupBox>
#include <QFrame>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPalette>
#include <QLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QEvent>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"
}

#include "iupqt_drv.h"


extern "C" void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  *x = 0;
  *y = 0;
}

extern "C" int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  (void)ih;
  (void)h;
  return 0;
}

extern "C" int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  /* Create a temporary QGroupBox to measure decoration size */
  QGroupBox tempBox;

  const char* title = iupAttribGet(ih, "TITLE");
  if (title && *title)
  {
    /* Titled frame */
    tempBox.setTitle(QString::fromUtf8(title));
    tempBox.setStyleSheet("QGroupBox { border: 1px solid gray; padding: 2px; margin-top: 0.5em; } "
                          "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 3px; }");
  }
  else
  {
    /* Untitled frame */
    tempBox.setTitle(QString());
    tempBox.setStyleSheet("QGroupBox { border: 1px solid gray; padding: 2px; margin-top: 0px; }");
  }

  /* DON'T add a layout - we want to measure just the QGroupBox chrome itself
   * The layout margins are handled separately by IUP */
  tempBox.setMinimumSize(100, 100);
  tempBox.adjustSize();

  QRect contentsRect = tempBox.contentsRect();
  QRect frameRect = tempBox.rect();

  /* Decoration is the difference between the widget rect and its contents rect */
  *w = (frameRect.width() - contentsRect.width());
  *h = (frameRect.height() - contentsRect.height());

  return 1;
}

/****************************************************************************
 * Attribute Setters/Getters
 ****************************************************************************/

static int qtFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    QGroupBox* groupbox = qobject_cast<QGroupBox*>((QWidget*)ih->handle);
    if (groupbox)
    {
      if (value)
        groupbox->setTitle(QString::fromUtf8(value));
      else
        groupbox->setTitle(QString());
      return 1;
    }
  }
  return 0;
}

static char* qtFrameGetTitleAttrib(Ihandle* ih)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    QGroupBox* groupbox = qobject_cast<QGroupBox*>((QWidget*)ih->handle);
    if (groupbox)
    {
      QString title = groupbox->title();
      if (!title.isEmpty())
        return iupStrReturnStr(title.toUtf8().constData());
    }
  }
  return nullptr;
}

static int qtFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  /* SUNKEN only applies to untitled frames */
  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    QGroupBox* groupbox = qobject_cast<QGroupBox*>((QWidget*)ih->handle);
    if (groupbox)
    {
      if (iupStrBoolean(value))
      {
        /* Sunken style - inset border */
        groupbox->setStyleSheet("QGroupBox { border: 1px inset gray; padding: 2px; margin-top: 0px; }");
        iupAttribSet(ih, "_IUPFRAME_SUNKEN", "1");
      }
      else
      {
        /* Raised/normal style - solid border */
        groupbox->setStyleSheet("QGroupBox { border: 1px solid gray; padding: 2px; margin-top: 0px; }");
        iupAttribSet(ih, "_IUPFRAME_SUNKEN", NULL);
      }

      return 1;
    }
  }
  return 0;
}

static char* qtFrameGetSunkenAttrib(Ihandle* ih)
{
  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    return iupStrReturnBoolean(iupAttribGet(ih, "_IUPFRAME_SUNKEN") != NULL);
  }
  return nullptr;
}

static int qtFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QWidget* widget = (QWidget*)ih->handle;

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    value = iupBaseNativeParentGetBgColor(ih);
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (widget)
  {
    QWidget* inner = nullptr;

    QGroupBox* groupbox = qobject_cast<QGroupBox*>(widget);
    if (groupbox)
    {
      QList<QWidget*> children = groupbox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
      if (!children.isEmpty())
        inner = children.first();
    }

    QColor color(r, g, b);

    if (inner)
    {
      QPalette palette = inner->palette();
      palette.setColor(QPalette::Window, color);
      inner->setPalette(palette);
    }
  }

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 1;
  else
    return 0;
}

static int qtFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  QGroupBox* groupbox = qobject_cast<QGroupBox*>((QWidget*)ih->handle);

  if (!groupbox)
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QPalette palette = groupbox->palette();
  palette.setColor(QPalette::WindowText, QColor(r, g, b));
  groupbox->setPalette(palette);

  return 1;
}

static int qtFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    QGroupBox* groupbox = qobject_cast<QGroupBox*>((QWidget*)ih->handle);
    if (groupbox)
      iupqtUpdateWidgetFont(ih, groupbox);
  }

  return 1;
}

/****************************************************************************
 * Inner Container Access
 ****************************************************************************/

static void* qtFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;

  QWidget* widget = (QWidget*)ih->handle;

  if (!widget)
    return NULL;

  QGroupBox* groupbox = qobject_cast<QGroupBox*>(widget);
  if (groupbox)
  {
    QList<QWidget*> children = groupbox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    if (!children.isEmpty())
      return children.first();
  }

  return NULL;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtFrameMapMethod(Ihandle* ih)
{
  QWidget* frame_widget = nullptr;
  QWidget* inner_parent = nullptr;
  char* title;

  if (!ih->parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");

  QGroupBox* groupbox = new QGroupBox();

  if (title)
  {
    groupbox->setTitle(QString::fromUtf8(title));
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");

    /* Stylesheet for titled frame */
    groupbox->setStyleSheet("QGroupBox { border: 1px solid gray; padding: 2px; margin-top: 0.5em; } "
                           "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 3px; }");
  }
  else
  {
    /* Leave title empty for untitled frame */
    groupbox->setTitle(QString());

    /* Stylesheet for untitled frame */
    groupbox->setStyleSheet("QGroupBox { border: 1px solid gray; padding: 2px; margin-top: 0px; }");

    if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  groupbox->setFlat(false);
  frame_widget = groupbox;

  if (!frame_widget)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)frame_widget;

  inner_parent = iupqtNativeContainerNew(0);

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(inner_parent);

  frame_widget->setLayout(layout);

  iupAttribSet(ih, "_IUPQT_FRAME_INNER", (char*)inner_parent);

  iupqtAddToParent(ih);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    qtFrameSetBgColorAttrib(ih, NULL);

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = qtFrameMapMethod;
  ic->GetInnerNativeContainerHandle = qtFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, qtFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, qtFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, qtFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", qtFrameGetSunkenAttrib, qtFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, qtFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", qtFrameGetTitleAttrib, qtFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
