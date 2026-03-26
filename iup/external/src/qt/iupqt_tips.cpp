/** \file
 * \brief Qt Driver TIPS (tooltips) management
 *
 * See Copyright Notice in "iup.h"
 */

#include <QWidget>
#include <QToolTip>
#include <QHelpEvent>
#include <QEvent>
#include <QString>
#include <QRect>
#include <QPoint>
#include <QCursor>
#include <QApplication>
#include <QPalette>

#include <cstdio>
#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
}

#include "iupqt_drv.h"


static void qtTipUpdateStyle(Ihandle* ih)
{
  const char* value;
  unsigned char r, g, b;

  value = iupAttribGet(ih, "TIPFONT");
  if (value)
  {
    if (!iupStrEqualNoCase(value, "SYSTEM"))
    {
      QFont* qfont = iupqtGetQFont(value);
      if (qfont)
        QToolTip::setFont(*qfont);
    }
  }

  QPalette palette = QToolTip::palette();
  int palette_changed = 0;

  value = iupAttribGet(ih, "TIPBGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
  {
    palette.setColor(QPalette::ToolTipBase, QColor(r, g, b));
    palette_changed = 1;
  }

  value = iupAttribGet(ih, "TIPFGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
  {
    palette.setColor(QPalette::ToolTipText, QColor(r, g, b));
    palette_changed = 1;
  }

  if (palette_changed)
    QToolTip::setPalette(palette);
}


/****************************************************************************
 * Tooltip Event Filter
 ****************************************************************************/

class IupQtTooltipFilter : public QObject
{
protected:
  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (event->type() == QEvent::ToolTip)
    {
      QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
      QWidget* widget = qobject_cast<QWidget*>(obj);

      if (widget)
      {
        Ihandle* ih = nullptr;

        QVariant variant = widget->property("IUP_HANDLE");
        if (variant.isValid())
          ih = static_cast<Ihandle*>(variant.value<void*>());

        if (ih)
        {
          const char* tiprect = iupAttribGet(ih, "TIPRECT");
          if (tiprect)
          {
            int x1, y1, x2, y2;
            if (sscanf(tiprect, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4)
            {
              QPoint local_pos = widget->mapFromGlobal(helpEvent->globalPos());
              QRect tip_rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

              if (!tip_rect.contains(local_pos))
              {
                QToolTip::hideText();
                return true;
              }
            }
          }

          IFnii cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
          if (cb)
          {
            int x, y;
            iupdrvGetCursorPos(&x, &y);
            iupdrvScreenToClient(ih, &x, &y);
            cb(ih, x, y);
          }

          const char* tip = iupAttribGet(ih, "TIP");
          if (tip)
          {
            qtTipUpdateStyle(ih);
            QToolTip::showText(helpEvent->globalPos(), QString::fromUtf8(tip), widget);
          }
          else
          {
            QToolTip::hideText();
          }

          return true;
        }
      }
    }

    return QObject::eventFilter(obj, event);
  }
};

static IupQtTooltipFilter* qt_tooltip_filter = nullptr;

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static void qtTooltipSetTitle(Ihandle* ih, QWidget* widget, const char* value)
{
  (void)ih;

  if (!value || value[0] == 0)
    widget->setToolTip(QString());
  else
    widget->setToolTip(QString::fromUtf8(value));
}

static QWidget* qtTipGetWidget(Ihandle* ih)
{
  QWidget* widget = (QWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = (QWidget*)ih->handle;
  return widget;
}

/****************************************************************************
 * Attribute Functions
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = qtTipGetWidget(ih);

  if (!widget)
    return 0;

  if (!qt_tooltip_filter)
  {
    qt_tooltip_filter = new IupQtTooltipFilter();
    QApplication::instance()->installEventFilter(qt_tooltip_filter);
  }

  widget->setProperty("IUP_HANDLE", QVariant::fromValue((void*)ih));

  qtTooltipSetTitle(ih, widget, value);

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = qtTipGetWidget(ih);

  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (tip)
    {
      qtTipUpdateStyle(ih);

      QPoint globalPos = QCursor::pos();
      QToolTip::showText(globalPos, QString::fromUtf8(tip), widget);
    }
  }
  else
  {
    QToolTip::hideText();
  }

  return 0;
}

extern "C" IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(QToolTip::isVisible() ? 1 : 0);
}

/****************************************************************************
 * Tooltip Destroy
 ****************************************************************************/

extern "C" void iupqtTipsDestroy(Ihandle* ih)
{
  if (!ih)
    return;

  QWidget* widget = qtTipGetWidget(ih);
  if (!widget)
    return;

  widget->setToolTip(QString());
  widget->setProperty("IUP_HANDLE", QVariant());
}
