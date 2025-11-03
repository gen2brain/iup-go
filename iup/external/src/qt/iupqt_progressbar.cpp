/** \file
 * \brief Progress bar Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QProgressBar>
#include <QWidget>

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
#include "iup_progressbar.h"
#include "iup_drv.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Timer Callback for Marquee Mode
 ****************************************************************************/

static int qtProgressBarTimeCb(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_PROGRESSBAR");
  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (pbar)
  {
    int value = pbar->value();
    if (value >= pbar->maximum())
      pbar->setValue(pbar->minimum());
    else
      pbar->setValue(value + 1);
  }

  return IUP_DEFAULT;
}

/****************************************************************************
 * Min Size Calculation
 ****************************************************************************/

extern "C" void iupdrvProgressBarGetMinSize(Ihandle* ih, int *w, int *h)
{
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    *w = 20;
    *h = 100;
  }
  else
  {
    *w = 100;
    *h = 20;
  }
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee)
    return 0;

  QProgressBar* pbar = (QProgressBar*)ih->handle;
  if (!pbar)
    return 0;

  if (iupStrBoolean(value))
  {
    if (ih->data->timer)
      IupSetAttribute(ih->data->timer, "RUN", "YES");
  }
  else
  {
    if (ih->data->timer)
      IupSetAttribute(ih->data->timer, "RUN", "NO");

    pbar->setValue(pbar->minimum());
  }

  return 1;
}

static int qtProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (ih->data->marquee)
    return 0;

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  if (pbar)
  {
    /* Map value from [vmin, vmax] to [0, 1000] for precision */
    double fraction = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
    int int_value = (int)(fraction * 1000.0);
    pbar->setValue(int_value);
  }

  return 0;
}

static int qtProgressBarSetMinAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmin)))
  {
    qtProgressBarSetValueAttrib(ih, nullptr);
  }
  return 0;
}

static int qtProgressBarSetMaxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->vmax)))
  {
    qtProgressBarSetValueAttrib(ih, nullptr);
  }
  return 0;
}

static char* qtProgressBarGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static char* qtProgressBarGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int qtProgressBarSetDashedAttrib(Ihandle* ih, const char* value)
{
  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (ih->data->marquee)
    return 0;

  if (pbar)
  {
    if (iupStrBoolean(value))
    {
      ih->data->dashed = 1;
      /* Qt doesn't have a direct "dashed" mode, but we can use text format */
      pbar->setTextVisible(true);
      pbar->setFormat("%p%");
    }
    else /* Default */
    {
      ih->data->dashed = 0;

      /* Check SHOWTEXT attribute */
      if (!iupAttribGetBoolean(ih, "SHOWTEXT"))
        pbar->setTextVisible(false);
    }
  }

  return 0;
}

static int qtProgressBarSetShowTextAttrib(Ihandle* ih, const char* value)
{
  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (ih->data->marquee)
    return 0;

  if (pbar)
  {
    if (iupStrBoolean(value))
    {
      pbar->setTextVisible(true);

      /* Use custom text if available, otherwise percentage */
      const char* text = iupAttribGet(ih, "TEXT");
      if (text)
        pbar->setFormat(QString::fromUtf8(text));
      else
        pbar->setFormat("%p%");
    }
    else
    {
      pbar->setTextVisible(false);
    }
  }

  return 1;
}

static int qtProgressBarSetTextAttrib(Ihandle* ih, const char* value)
{
  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (ih->data->marquee)
    return 0;

  if (pbar && pbar->isTextVisible())
  {
    if (value)
      pbar->setFormat(QString::fromUtf8(value));
    else
      pbar->setFormat("%p%");  /* Default to percentage */
  }

  return 1;
}

static char* qtProgressBarGetTextAttrib(Ihandle* ih)
{
  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (!pbar || ih->data->marquee)
    return nullptr;

  QString format = pbar->format();
  if (format == "%p%" || format.isEmpty())
    return nullptr;

  return iupStrReturnStr(format.toUtf8().constData());
}

static int qtProgressBarSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (pbar)
  {
    QPalette palette = pbar->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    pbar->setPalette(palette);
    pbar->setAutoFillBackground(true);
    return 1;
  }

  return 0;
}

static int qtProgressBarSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QProgressBar* pbar = (QProgressBar*)ih->handle;

  if (pbar)
  {
    /* Set the progress bar fill color via stylesheet */
    QString style = QString("QProgressBar::chunk { background-color: rgb(%1,%2,%3); }")
                      .arg(r).arg(g).arg(b);
    pbar->setStyleSheet(style);
    return 1;
  }

  return 0;
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtProgressBarMapMethod(Ihandle* ih)
{
  QProgressBar* pbar = new QProgressBar();

  if (!pbar)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)pbar;

  /* Set range [0, 1000] for better precision with double values */
  pbar->setRange(0, 1000);
  pbar->setValue(0);

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    pbar->setOrientation(Qt::Vertical);
    pbar->setInvertedAppearance(true); /* Bottom to top */

    /* Swap width/height if needed */
    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }

    /* Vertical progress bars typically don't expand horizontally */
    ih->expand = ih->expand & ~IUP_EXPAND_WIDTH;
  }
  else
  {
    pbar->setOrientation(Qt::Horizontal);

    /* Horizontal progress bars typically don't expand vertically */
    ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;
  }

  /* Setup marquee mode if requested */
  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
    ih->data->marquee = 1;

    /* Create timer for animation */
    ih->data->timer = IupTimer();
    IupSetCallback(ih->data->timer, "ACTION_CB", (Icallback)qtProgressBarTimeCb);
    IupSetAttribute(ih->data->timer, "TIME", "100");
    iupAttribSet(ih->data->timer, "_IUP_PROGRESSBAR", (char*)ih);

    /* For Qt marquee, we'll use a range and animate manually */
    pbar->setRange(0, 100);
    pbar->setValue(0);
    pbar->setTextVisible(false);
  }
  else
  {
    ih->data->marquee = 0;
  }

  /* Progress bars should not accept focus */
  pbar->setFocusPolicy(Qt::NoFocus);

  iupqtAddToParent(ih);

  if (!ih->data->marquee)
  {
    if (iupAttribGetBoolean(ih, "SHOWTEXT"))
    {
      pbar->setTextVisible(true);

      const char* text = iupAttribGet(ih, "TEXT");
      if (text)
        pbar->setFormat(QString::fromUtf8(text));
      else
        pbar->setFormat("%p%");
    }
    else if (ih->data->dashed)
    {
      pbar->setTextVisible(true);
      pbar->setFormat("%p%");
    }
    else
    {
      pbar->setTextVisible(false);
    }
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * UnMap Method
 ****************************************************************************/

static void qtProgressBarUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    QProgressBar* pbar = (QProgressBar*)ih->handle;

    /* Stop and destroy timer if in marquee mode */
    if (ih->data->marquee && ih->data->timer)
    {
      IupSetAttribute(ih->data->timer, "RUN", "NO");
      IupDestroy(ih->data->timer);
      ih->data->timer = nullptr;
    }

    /* Destroy tooltip if any */
    iupqtTipsDestroy(ih);

    /* Delete the widget */
    delete pbar;
    ih->handle = nullptr;
  }
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvProgressBarInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtProgressBarMapMethod;
  ic->UnMap = qtProgressBarUnMapMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, qtProgressBarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, qtProgressBarSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);

  /* IupProgressBar only */
  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, qtProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIN", qtProgressBarGetMinAttrib, qtProgressBarSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", qtProgressBarGetMaxAttrib, qtProgressBarSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", iProgressBarGetDashedAttrib, qtProgressBarSetDashedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, qtProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Text display */
  iupClassRegisterAttribute(ic, "SHOWTEXT", NULL, qtProgressBarSetShowTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXT", qtProgressBarGetTextAttrib, qtProgressBarSetTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
