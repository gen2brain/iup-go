/** \file
 * \brief IupPrintDlg pre-defined dialog - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QPrinter>
#include <QPrinterInfo>
#include <QPageSize>
#include <QPageLayout>
#include <QPainter>
#include <QLocale>
#include <QString>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
}


static QList<QPageSize> qmlPrintDlgPageSizes(const char* printer, QPageSize* default_size)
{
  QList<QPageSize> sizes;

  if (printer)
  {
    QPrinterInfo info = QPrinterInfo::printerInfo(QString::fromUtf8(printer));
    sizes = info.supportedPageSizes();
    *default_size = info.defaultPageSize();
  }

  if (sizes.isEmpty())
  {
    sizes = { QPageSize(QPageSize::A3), QPageSize(QPageSize::A4), QPageSize(QPageSize::A5), QPageSize(QPageSize::B5),
              QPageSize(QPageSize::Letter), QPageSize(QPageSize::Legal), QPageSize(QPageSize::Executive),
              QPageSize(QPageSize::Tabloid) };
    *default_size = QPageSize(QLocale().measurementSystem() == QLocale::ImperialUSSystem ? QPageSize::Letter : QPageSize::A4);
  }

  return sizes;
}

static void qmlPrintDlgFillPrinters(Ihandle* list)
{
  QString default_name = QPrinterInfo::defaultPrinterName();
  const QStringList names = QPrinterInfo::availablePrinterNames();
  int index = 0;

  for (const QString& name : names)
  {
    index++;
    IupSetStrAttributeId(list, "", index, name.toUtf8().constData());
    if (name == default_name || index == 1)
      IupSetInt(list, "VALUE", index);
  }
}

static void qmlPrintDlgFillPapers(Ihandle* list, const char* printer)
{
  QPageSize default_size;
  const QList<QPageSize> sizes = qmlPrintDlgPageSizes(printer, &default_size);
  int index = 0;

  for (const QPageSize& size : sizes)
  {
    index++;
    IupSetStrAttributeId(list, "", index, size.name().toUtf8().constData());
    if (size.isEquivalentTo(default_size) || index == 1)
      IupSetInt(list, "VALUE", index);
  }
}

static int qmlPrintDlgPrint(Ihandle* ih, const char* printer_name, const char* filename, const char* paper, int landscape, int copies)
{
  QPrinter printer(QPrinter::HighResolution);
  QPageSize default_size;
  const QList<QPageSize> sizes = qmlPrintDlgPageSizes(printer_name, &default_size);
  QPainter painter;
  int resolution, width, height, count, page, printed = 0;
  QRect paint_rect;
  double scale;

  if (printer_name)
    printer.setPrinterName(QString::fromUtf8(printer_name));
  else
  {
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(QString::fromUtf8(filename));
  }

  printer.setDocName(QString::fromUtf8(iupPrintDlgGetJobName(ih)));
  printer.setCopyCount(copies);

  for (const QPageSize& size : sizes)
  {
    if (paper && size.name() == QString::fromUtf8(paper))
    {
      printer.setPageSize(size);
      break;
    }
  }
  printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);

  resolution = printer.resolution();
  paint_rect = printer.pageLayout().paintRectPixels(resolution);
  scale = resolution / 96.0;
  width = iupRound(paint_rect.width() / scale);
  height = iupRound(paint_rect.height() / scale);
  count = iupPrintDlgPaginate(ih, width, height);

  if (!painter.begin(&printer))
    return -1;

  for (page = 1; page <= count; page++)
  {
    int ret;

    if (!iupPrintDlgPageSelected(ih, page))
      continue;

    if (printed && !printer.newPage())
    {
      painter.end();
      return -1;
    }

    painter.save();
    painter.scale(scale, scale);
    ret = iupPrintDlgDrawPage(ih, &painter, page, width, height);
    painter.restore();
    printed++;

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

static const IprintDlgDriver qml_print_driver = {
  qmlPrintDlgFillPrinters,
  qmlPrintDlgFillPapers,
  qmlPrintDlgPrint
};

extern "C" IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  iupPrintDlgCustomInitClass(ic, &qml_print_driver);
}
