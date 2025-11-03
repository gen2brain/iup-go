/** \file
 * \brief Clipboard for the Qt Driver - Qt Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QPixmap>
#include <QGuiApplication>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QBuffer>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static const char* qtClipboardGetFormatMimeType(Ihandle *ih)
{
  return iupAttribGetStr(ih, "FORMAT");
}

/****************************************************************************
 * TEXT Attribute
 ****************************************************************************/

static int qtClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear();
    return 0;
  }

  clipboard->setText(QString::fromUtf8(value));
  (void)ih;
  return 0;
}

static char* qtClipboardGetTextAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  QString text = clipboard->text();
  (void)ih;

  return iupStrReturnStr(text.toUtf8().constData());
}

static char* qtClipboardGetTextAvailableAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  (void)ih;

  return iupStrReturnBoolean(mimeData && mimeData->hasText());
}

/****************************************************************************
 * IMAGE Attributes
 ****************************************************************************/

static int qtClipboardSetImageAttrib(Ihandle *ih, const char *value)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear();
    return 0;
  }

  QPixmap* pixmap = (QPixmap*)iupImageGetImage(value, ih, 0, nullptr);
  if (pixmap && !pixmap->isNull())
  {
    clipboard->setPixmap(*pixmap);
    iupImageRemoveFromCache(ih, pixmap);
  }

  return 0;
}

static int qtClipboardSetNativeImageAttrib(Ihandle *ih, const char *value)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return 0;

  (void)ih;

  if (!value)
  {
    clipboard->clear();
    return 0;
  }

  clipboard->setPixmap(*(QPixmap*)value);
  return 0;
}

static char* qtClipboardGetNativeImageAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  QPixmap pixmap = clipboard->pixmap();

  if (pixmap.isNull())
    return nullptr;

  QPixmap* result = new QPixmap(pixmap);
  (void)ih;

  return (char*)result;
}

static char* qtClipboardGetImageAvailableAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  (void)ih;

  return iupStrReturnBoolean(mimeData && mimeData->hasImage());
}

/****************************************************************************
 * PDF/SVG Vector Image Attributes (Qt-specific, similar to Cocoa)
 ****************************************************************************/

static int qtClipboardSetNativeVectorImageAttrib(Ihandle *ih, const char *value)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear();
    return 0;
  }

  int data_size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (data_size > 0)
  {
    QMimeData *mimeData = new QMimeData();
    QByteArray byteArray((const char*)value, data_size);

    /* Qt supports PDF in clipboard */
    mimeData->setData("application/pdf", byteArray);
    clipboard->setMimeData(mimeData);
  }

  (void)ih;
  return 0;
}

static char* qtClipboardGetNativeVectorImageAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  if (!mimeData)
    return nullptr;

  QByteArray byteArray = mimeData->data("application/pdf");
  if (byteArray.isEmpty())
    return nullptr;

  int size = byteArray.size();
  void* data = iupStrGetMemory(size + 1);
  memcpy(data, byteArray.constData(), size);

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return (char*)data;
}

static char* qtClipboardGetPDFAvailableAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  (void)ih;

  return iupStrReturnBoolean(mimeData && mimeData->hasFormat("application/pdf"));
}

static int qtClipboardSetSaveNativeVectorImageAttrib(Ihandle *ih, const char *value)
{
  if (!value)
    return 0;

  QClipboard *clipboard = QGuiApplication::clipboard();
  if (!clipboard)
    return 0;

  const QMimeData *mimeData = clipboard->mimeData();
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

  (void)ih;
  return 0;
}

/****************************************************************************
 * HTML Attribute (Qt-specific)
 ****************************************************************************/

static int qtClipboardSetHTMLAttrib(Ihandle *ih, const char *value)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear();
    return 0;
  }

  QMimeData *mimeData = new QMimeData();
  mimeData->setHtml(QString::fromUtf8(value));
  clipboard->setMimeData(mimeData);

  (void)ih;
  return 0;
}

static char* qtClipboardGetHTMLAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  if (!mimeData || !mimeData->hasHtml())
    return nullptr;

  QString html = mimeData->html();
  (void)ih;

  return iupStrReturnStr(html.toUtf8().constData());
}

static char* qtClipboardGetHTMLAvailableAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  (void)ih;

  return iupStrReturnBoolean(mimeData && mimeData->hasHtml());
}

/****************************************************************************
 * FORMAT Attributes (Custom MIME types)
 ****************************************************************************/

static int qtClipboardSetFormatDataAttrib(Ihandle *ih, const char *value)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return 0;

  if (!value)
  {
    clipboard->clear();
    return 0;
  }

  const char* mime_type = qtClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return 0;

  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (!size)
    return 0;

  QMimeData *mimeData = new QMimeData();
  QByteArray byteArray((const char*)value, size);
  mimeData->setData(QString::fromUtf8(mime_type), byteArray);

  clipboard->setMimeData(mimeData);

  return 0;
}

static char* qtClipboardGetFormatDataAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const char* mime_type = qtClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  if (!mimeData)
    return nullptr;

  QByteArray byteArray = mimeData->data(QString::fromUtf8(mime_type));

  if (byteArray.isEmpty())
    return nullptr;

  int size = byteArray.size();
  void* data = iupStrGetMemory(size + 1); /* reserve room for terminator */
  memcpy(data, byteArray.constData(), size);

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return (char*)data;
}

static char* qtClipboardGetFormatDataStringAttrib(Ihandle *ih)
{
  char* data = qtClipboardGetFormatDataAttrib(ih);
  if (!data)
    return nullptr;

  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  data[size] = 0;
  return iupStrReturnStr(data);
}

static int qtClipboardSetFormatDataStringAttrib(Ihandle *ih, const char *value)
{
  if (value)
  {
    int len = (int)strlen(value);
    iupAttribSetInt(ih, "FORMATDATASIZE", len + 1);
    return qtClipboardSetFormatDataAttrib(ih, value);
  }
  else
    return qtClipboardSetFormatDataAttrib(ih, nullptr);
}

static char* qtClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
  QClipboard *clipboard = QGuiApplication::clipboard();

  if (!clipboard)
    return nullptr;

  const char* mime_type = qtClipboardGetFormatMimeType(ih);
  if (!mime_type)
    return nullptr;

  const QMimeData *mimeData = clipboard->mimeData();
  if (!mimeData)
    return iupStrReturnBoolean(0);

  return iupStrReturnBoolean(mimeData->hasFormat(QString::fromUtf8(mime_type)));
}

static int qtClipboardSetAddFormatAttrib(Ihandle *ih, const char *value)
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

  ic->name = (char*)"clipboard";
  ic->format = nullptr;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  /* TEXT attributes */
  iupClassRegisterAttribute(ic, "TEXT", qtClipboardGetTextAttrib, qtClipboardSetTextAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", qtClipboardGetTextAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* IMAGE attributes */
  iupClassRegisterAttribute(ic, "NATIVEIMAGE", qtClipboardGetNativeImageAttrib, qtClipboardSetNativeImageAttrib, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, qtClipboardSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", qtClipboardGetImageAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* PDF/Vector image attributes (similar to Cocoa) */
  iupClassRegisterAttribute(ic, "NATIVEVECTORIMAGE", qtClipboardGetNativeVectorImageAttrib, qtClipboardSetNativeVectorImageAttrib, nullptr, nullptr, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PDFAVAILABLE", qtClipboardGetPDFAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVENATIVEVECTORIMAGE", nullptr, qtClipboardSetSaveNativeVectorImageAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* HTML attributes (Qt-specific) */
  iupClassRegisterAttribute(ic, "HTML", qtClipboardGetHTMLAttrib, qtClipboardSetHTMLAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HTMLAVAILABLE", qtClipboardGetHTMLAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /* CUSTOM FORMAT attributes */
  iupClassRegisterAttribute(ic, "ADDFORMAT", nullptr, qtClipboardSetAddFormatAttrib, nullptr, nullptr, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", qtClipboardGetFormatAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", qtClipboardGetFormatDataAttrib, qtClipboardSetFormatDataAttrib, nullptr, nullptr, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", qtClipboardGetFormatDataStringAttrib, qtClipboardSetFormatDataStringAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);

  return ic;
}
