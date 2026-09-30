/** \file
 * \brief Clipboard for the Qt Quick Driver - Qt Quick Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QClipboard>
#include <QMimeData>
#include <QPixmap>
#include <QGuiApplication>
#include <QString>
#include <QByteArray>
#include <QFile>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
}


/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static QClipboard::Mode qmlClipboardMode(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  if (clipboard && clipboard->supportsSelection() && iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTION"), "PRIMARY"))
    return QClipboard::Selection;
  return QClipboard::Clipboard;
}

static const char* qmlClipboardGetFormatMimeType(Ihandle* ih)
{
  return iupAttribGetStr(ih, "FORMAT");
}

/****************************************************************************
 * TEXT Attribute
 ****************************************************************************/

static int qmlClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear(mode);
    return 0;
  }

  clipboard->setText(QString::fromUtf8(value), mode);
  return 0;
}

static char* qmlClipboardGetTextAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  QString text = clipboard->text(mode);

  return iupStrReturnStr(text.toUtf8().constData());
}

static char* qmlClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);

  return iupStrReturnBoolean(mimeData && mimeData->hasText());
}

/****************************************************************************
 * IMAGE Attributes
 ****************************************************************************/

static int qmlClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear(mode);
    return 0;
  }

  auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(value, ih, 0, nullptr));
  if (pixmap && !pixmap->isNull())
    clipboard->setPixmap(*pixmap, mode);

  return 0;
}

static int qmlClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return 0;


  if (!value)
  {
    clipboard->clear(mode);
    return 0;
  }

  clipboard->setPixmap(*reinterpret_cast<const QPixmap*>(value), mode);
  return 0;
}

static char* qmlClipboardGetNativeImageAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  QPixmap pixmap = clipboard->pixmap(mode);

  if (pixmap.isNull())
    return nullptr;

  auto* result = new QPixmap(pixmap);

  return reinterpret_cast<char*>(result);
}

static char* qmlClipboardGetImageAvailableAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);

  return iupStrReturnBoolean(mimeData && mimeData->hasImage());
}

/****************************************************************************
 * PDF/SVG Vector Image Attributes (Qt-specific, similar to Cocoa)
 ****************************************************************************/

static int qmlClipboardSetNativeVectorImageAttrib(Ihandle* ih, const char* value)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear(mode);
    return 0;
  }

  int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (data_size > 0)
  {
    auto* mimeData = new QMimeData();
    QByteArray byteArray(value, data_size);

    mimeData->setData("application/pdf", byteArray);
    clipboard->setMimeData(mimeData, mode);
  }

  return 0;
}

static char* qmlClipboardGetNativeVectorImageAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);
  if (!mimeData)
    return nullptr;

  QByteArray byteArray = mimeData->data("application/pdf");
  if (byteArray.isEmpty())
    return nullptr;

  int size = byteArray.size();
  void* data = iupStrGetMemory(size + 1);
  memcpy(data, byteArray.constData(), size);

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return static_cast<char*>(data);
}

static char* qmlClipboardGetPDFAvailableAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);

  return iupStrReturnBoolean(mimeData && mimeData->hasFormat("application/pdf"));
}

static int qmlClipboardSetSaveNativeVectorImageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);
  if (!clipboard)
    return 0;

  const QMimeData* mimeData = clipboard->mimeData(mode);
  if (!mimeData)
    return 0;

  QByteArray byteArray = mimeData->data("application/pdf");
  if (byteArray.isEmpty())
    return 0;

  QFile file(QString::fromUtf8(value));
  if (file.open(QIODevice::WriteOnly))
  {
    file.write(byteArray);
    file.close();
  }

  return 0;
}

/****************************************************************************
 * HTML Attribute (Qt-specific)
 ****************************************************************************/

static int qmlClipboardSetHTMLAttrib(Ihandle* ih, const char* value)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear(mode);
    return 0;
  }

  auto* mimeData = new QMimeData();
  mimeData->setHtml(QString::fromUtf8(value));
  clipboard->setMimeData(mimeData, mode);

  return 0;
}

static char* qmlClipboardGetHTMLAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);
  if (!mimeData || !mimeData->hasHtml())
    return nullptr;

  QString html = mimeData->html();

  return iupStrReturnStr(html.toUtf8().constData());
}

static char* qmlClipboardGetHTMLAvailableAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);

  return iupStrReturnBoolean(mimeData && mimeData->hasHtml());
}

/****************************************************************************
 * FORMAT Attributes (Custom MIME types)
 ****************************************************************************/

static int qmlClipboardSetFormatDataAttrib(Ihandle* ih, const char* value)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear(mode);
    return 0;
  }

  const char* mime_type = qmlClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return 0;

  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (size <= 0)
    return 0;

  auto* mimeData = new QMimeData();
  QByteArray byteArray(value, size);
  mimeData->setData(QString::fromUtf8(mime_type), byteArray);

  clipboard->setMimeData(mimeData, mode);

  return 0;
}

static char* qmlClipboardGetFormatDataAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const char* mime_type = qmlClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);
  if (!mimeData)
    return nullptr;

  QByteArray byteArray = mimeData->data(QString::fromUtf8(mime_type));

  if (byteArray.isEmpty())
    return nullptr;

  int size = byteArray.size();
  void* data = iupStrGetMemory(size + 1); /* reserve room for terminator */
  memcpy(data, byteArray.constData(), size);

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return static_cast<char*>(data);
}

static char* qmlClipboardGetFormatDataStringAttrib(Ihandle* ih)
{
  char* data = qmlClipboardGetFormatDataAttrib(ih);
  if (!data)
    return nullptr;

  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  data[size] = 0;
  return iupStrReturnStr(data);
}

static int qmlClipboardSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    int len = static_cast<int>(strlen(value));
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1);
    return qmlClipboardSetFormatDataAttrib(ih, value);
  }
  else
    return qmlClipboardSetFormatDataAttrib(ih, nullptr);
}

static char* qmlClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  QClipboard::Mode mode = qmlClipboardMode(ih);

  if (!clipboard)
    return nullptr;

  const char* mime_type = qmlClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return nullptr;

  const QMimeData* mimeData = clipboard->mimeData(mode);
  if (!mimeData)
    return iupStrReturnBoolean(0);

  return iupStrReturnBoolean(mimeData->hasFormat(QString::fromUtf8(mime_type)));
}

static int qmlClipboardSetAddFormatAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

extern "C" Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(nullptr);

  ic->name = const_cast<char*>("clipboard");
  ic->format = nullptr;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", qmlClipboardGetTextAttrib, qmlClipboardSetTextAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", qmlClipboardGetTextAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", qmlClipboardGetNativeImageAttrib, qmlClipboardSetNativeImageAttrib, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qmlClipboardSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", qmlClipboardGetImageAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEVECTORIMAGE", qmlClipboardGetNativeVectorImageAttrib, qmlClipboardSetNativeVectorImageAttrib, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PDFAVAILABLE", qmlClipboardGetPDFAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVENATIVEVECTORIMAGE", nullptr, qmlClipboardSetSaveNativeVectorImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HTML", qmlClipboardGetHTMLAttrib, qmlClipboardSetHTMLAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTMLAVAILABLE", qmlClipboardGetHTMLAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", nullptr, qmlClipboardSetAddFormatAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", qmlClipboardGetFormatAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", qmlClipboardGetFormatDataAttrib, qmlClipboardSetFormatDataAttrib, nullptr, nullptr, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", qmlClipboardGetFormatDataStringAttrib, qmlClipboardSetFormatDataStringAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTION", nullptr, nullptr, "CLIPBOARD", nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}
