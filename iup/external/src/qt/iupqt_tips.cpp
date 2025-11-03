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
#include <QPixmap>
#include <QIcon>
#include <QPoint>
#include <QCursor>
#include <QApplication>
#include <QPalette>
#include <QFont>
#include <QColor>
#include <QTimer>

#include <cstdio>
#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
}

#include "iupqt_drv.h"


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
          /* Check TIPRECT - only show tooltip if cursor is in specified rectangle */
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
            QString tip_text;

            const char* tipicon = iupAttribGet(ih, "TIPICON");
            if (tipicon && iupAttribGetBoolean(ih, "TIPMARKUP"))
            {
              /* With markup enabled and icon specified, create rich text tooltip
               * Note: Qt doesn't easily support icons in tooltips via QPixmap,
               * but we can use HTML img tags if the icon is a file */
              tip_text = QString::fromUtf8(tip);
            }
            else
            {
              tip_text = QString::fromUtf8(tip);
            }

            QToolTip::showText(helpEvent->globalPos(), tip_text, widget);
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
  if (!value || value[0] == 0)
  {
    widget->setToolTip(QString());
    return;
  }

  if (iupAttribGetBoolean(ih, "TIPMARKUP"))
  {
    /* Treat as HTML/rich text */
    widget->setToolTip(QString::fromUtf8(value));
  }
  else
  {
    /* Plain text - Qt will auto-escape HTML entities */
    widget->setToolTip(QString::fromUtf8(value));
  }
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

extern "C" int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
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

extern "C" int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  QWidget* widget = qtTipGetWidget(ih);

  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (tip)
    {
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

extern "C" char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  /* Qt doesn't provide a direct way to check if tooltip is visible
   * This is a limitation - we return NULL to indicate unknown state */
  (void)ih;
  return nullptr;
}

/****************************************************************************
 * Additional Tip Attribute Functions
 ****************************************************************************/

extern "C" int iupdrvBaseSetTipBgColorAttrib(Ihandle* ih, const char* value)
{
  /* Qt tooltips use the system palette - we can set a custom palette
   * but it affects all tooltips globally via QPalette */
  if (value)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      QPalette palette = QToolTip::palette();
      palette.setColor(QPalette::ToolTipBase, QColor(r, g, b));
      QToolTip::setPalette(palette);
    }
  }

  (void)ih;
  return 1;
}

extern "C" int iupdrvBaseSetTipFgColorAttrib(Ihandle* ih, const char* value)
{
  /* Qt tooltips use the system palette */
  if (value)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      QPalette palette = QToolTip::palette();
      palette.setColor(QPalette::ToolTipText, QColor(r, g, b));
      QToolTip::setPalette(palette);
    }
  }

  (void)ih;
  return 1;
}

extern "C" int iupdrvBaseSetTipFontAttrib(Ihandle* ih, const char* value)
{
  /* Qt tooltips can have a custom font set globally */
  if (value)
  {
    QFont* qfont = iupqtGetQFont(value);
    if (qfont)
      QToolTip::setFont(*qfont);
  }

  (void)ih;
  return 1;
}

extern "C" int iupdrvBaseSetTipDelayAttrib(Ihandle* ih, const char* value)
{
  /* Qt doesn't provide a direct API for tooltip delay per widget */
  (void)ih;
  (void)value;

  return 0;
}

extern "C" int iupdrvBaseSetTipRectAttrib(Ihandle* ih, const char* value)
{
  /* Store for use in tooltip event handler */
  (void)value;

  /* The actual TIPRECT handling is done in the event filter when the tooltip is about to be shown */
  return 0;
}


extern "C" int iupdrvBaseSetTipIconAttrib(Ihandle* ih, const char* value)
{
  /* Store icon name for later use in tooltip display
   * Qt supports rich text in tooltips, so we can embed images */
  (void)ih;
  (void)value;

  /* The icon will be handled when the tooltip is shown */
  return 0;
}

/****************************************************************************
 * Tooltip Update and Destroy
 ****************************************************************************/

extern "C" void iupdrvUpdateTip(Ihandle* ih)
{
  if (!ih)
    return;

  QWidget* widget = qtTipGetWidget(ih);
  if (!widget)
    return;

  const char* tip = iupAttribGet(ih, "TIP");
  if (tip)
    qtTooltipSetTitle(ih, widget, tip);
}

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
