/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QPageLayout>
#include <QString>
#include <QWidget>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_stdcontrols.h"
}

#include "iupqt_drv.h"


static int qtPrintDlgPrintPages(Ihandle* ih, QPrinter& printer)
{
  int resolution = printer.resolution();
  QRect paint_rect = printer.pageLayout().paintRectPixels(resolution);
  double scale = resolution / 96.0;
  int width = iupRound(paint_rect.width() / scale);
  int height = iupRound(paint_rect.height() / scale);
  int count = iupPrintDlgPaginate(ih, width, height);
  int first = 1, last = count, page;
  QPainter painter;

  if (printer.fromPage() > 0)
  {
    first = printer.fromPage();
    last = printer.toPage() < count ? printer.toPage() : count;
  }

  if (!painter.begin(&printer))
    return -1;

  for (page = first; page <= last; page++)
  {
    int ret;

    if (page != first && !printer.newPage())
    {
      painter.end();
      return -1;
    }

    painter.save();
    painter.scale(scale, scale);
    ret = iupPrintDlgDrawPage(ih, &painter, page, width, height);
    painter.restore();

    if (ret == IUP_CLOSE)
    {
      printer.abort();
      painter.end();
      return 0;
    }
  }

  painter.end();
  return 1;
}

static int qtPrintDlgPopup(Ihandle* ih, int x, int y)
{
  QWidget* parent = iupqtGetParentWidget(ih);
  QPrinter printer(QPrinter::HighResolution);
  int page_count = iupAttribGetInt(ih, "PAGECOUNT");
  int ret;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  printer.setDocName(QString::fromUtf8(iupPrintDlgGetJobName(ih)));
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE"))
    printer.setPageOrientation(QPageLayout::Landscape);
  else
    printer.setPageOrientation(QPageLayout::Portrait);

  QPrintDialog dialog(&printer, parent);
  dialog.setMinMax(1, page_count > 0 ? page_count : 9999);
  dialog.setOption(QAbstractPrintDialog::PrintPageRange, true);
  dialog.setOption(QAbstractPrintDialog::PrintSelection, false);
  dialog.setOption(QAbstractPrintDialog::PrintCurrentPage, false);

  if (dialog.exec() != QDialog::Accepted)
  {
    iupAttribSet(ih, "STATUS", "0");
    return IUP_NOERROR;
  }

  iupAttribSet(ih, "ORIENTATION", printer.pageLayout().orientation() == QPageLayout::Landscape ? "LANDSCAPE" : "PORTRAIT");

  ret = qtPrintDlgPrintPages(ih, printer);
  iupAttribSet(ih, "STATUS", ret == 1 ? "1" : (ret == 0 ? "0" : "-1"));

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qtPrintDlgPopup;
}
