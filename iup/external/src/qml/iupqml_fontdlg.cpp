/** \file
 * \brief IupFontDlg pre-defined dialog - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickWindow>
#include <QEventLoop>
#include <QFontInfo>
#include <QScreen>
#include <QString>
#include <QVariant>

#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
}

#include "iupqml_drv.h"


static int qmlFontDlgPopup(Ihandle* ih, int x, int y)
{
  QQuickWindow* parent = iupqmlGetParentWindow(ih);
  QQuickWindow* host;
  int host_owned;
  QFont initial_font;
  char* font_str;
  int accepted = 0;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  font_str = iupAttribGet(ih, "VALUE");
  if (!font_str)
    font_str = IupGetGlobal("DEFAULTFONT");

  if (font_str)
  {
    char typeface[1024];
    int size = 8;
    int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;

    if (iupGetFontInfo(font_str, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      const char* mapped_name = iupFontGetPangoName(typeface);
      if (mapped_name)
        iupStrCopyN(typeface, sizeof(typeface), mapped_name);

      int point_size = size;
      if (size < 0)
      {
        int dpi = 96;
        QScreen* screen = parent ? parent->screen() : nullptr;
        if (screen)
          dpi = static_cast<int>(screen->logicalDotsPerInch());
        point_size = (-size * 72) / dpi;
      }

      if (point_size > 0)
      {
        initial_font.setFamily(QString::fromUtf8(typeface));
        initial_font.setPointSize(point_size);
        initial_font.setBold(is_bold);
        initial_font.setItalic(is_italic);
        initial_font.setUnderline(is_underline);
        initial_font.setStrikeOut(is_strikeout);
      }
    }
  }

  if (initial_font.family().isEmpty())
    initial_font = QFont();

  QObject* dialog = iupqmlCreateObject("import QtQuick.Dialogs\nFontDialog { }");
  if (!dialog)
    return IUP_ERROR;

  const char* title = iupAttribGet(ih, "TITLE");
  if (!title)
    title = "Select Font";
  dialog->setProperty("title", QString::fromUtf8(title));

  host = iupqmlDialogHostWindow(ih, &host_owned);
  dialog->setProperty("parentWindow", QVariant::fromValue<QObject*>(host));
  dialog->setProperty("modality", Qt::ApplicationModal);

  dialog->setProperty("currentFont", QVariant::fromValue(initial_font));
  dialog->setProperty("selectedFont", QVariant::fromValue(initial_font));

  {
    QEventLoop loop;
    QObject* accept_slot = iupqmlConnect(dialog, "accepted()", [&accepted](void**) {
      accepted = 1;
    });
    QObject* visible_slot = iupqmlConnect(dialog, "visibleChanged()", [dialog, &loop](void**) {
      if (!dialog->property("visible").toBool())
        loop.quit();
    });

    iupqmlCallMethod(dialog, "open");
    if (dialog->property("visible").toBool())
      loop.exec();

    delete accept_slot;
    delete visible_slot;
  }

  if (accepted)
  {
    auto selected_font = dialog->property("selectedFont").value<QFont>();
    QFontInfo font_info(selected_font);

    QString family = font_info.family();
    int point_size = font_info.pointSize();
    bool is_bold = font_info.bold();
    bool is_italic = font_info.italic();
    bool is_underline = selected_font.underline();
    bool is_strikeout = selected_font.strikeOut();

    char font_value[256];
    snprintf(font_value, sizeof(font_value), "%s, %s%s%s%s%d",
             family.toUtf8().constData(),
             is_bold ? "Bold " : "",
             is_italic ? "Italic " : "",
             is_underline ? "Underline " : "",
             is_strikeout ? "Strikeout " : "",
             point_size);

    iupAttribSetStr(ih, "VALUE", font_value);
    iupAttribSet(ih, "STATUS", "1");
  }
  else
  {
    iupAttribSet(ih, "VALUE", nullptr);
    iupAttribSet(ih, "STATUS", nullptr);
  }

  delete dialog;
  if (host_owned)
    delete host;

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qmlFontDlgPopup;

  iupClassRegisterAttribute(ic, "PREVIEWTEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
