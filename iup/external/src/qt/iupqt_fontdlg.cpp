/** \file
 * \brief IupFontDlg pre-defined dialog - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QFontDialog>
#include <QFont>
#include <QFontInfo>
#include <QColor>
#include <QWidget>
#include <QScreen>
#include <QString>
#include <QDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QList>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drvfont.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Font Dialog Popup
 ****************************************************************************/

static int qtFontDlgPopup(Ihandle* ih, int x, int y)
{
  QWidget* parent = (QWidget*)iupDialogGetNativeParent(ih);
  Ihandle* parent_ih = nullptr;
  QFont initial_font;
  QColor initial_color = Qt::black;
  char* font_str;
  char* color_str;
  bool ok;
  int response;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  /* Find parent IUP dialog for focus restoration */
  if (parent)
  {
    Ihandle* test_ih = (Ihandle*)iupAttribGet((Ihandle*)parent, "_IUP_DIALOG");
    if (test_ih && iupObjectCheck(test_ih))
      parent_ih = IupGetDialog(test_ih);
  }

  font_str = iupAttribGet(ih, "VALUE");
  if (!font_str)
    font_str = IupGetGlobal("DEFAULTFONT");

  if (font_str)
  {
    char typeface[1024];
    int size = 8;
    int is_bold = 0,
        is_italic = 0,
        is_underline = 0,
        is_strikeout = 0;

    if (iupGetFontInfo(font_str, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      /* Map standard names to native names */
      const char* mapped_name = iupFontGetPangoName(typeface);
      if (mapped_name)
        strcpy(typeface, mapped_name);

      /* Convert size to pixels if negative (already in pixels) */
      int point_size = size;
      if (size < 0)
      {
        /* Size is in pixels, convert to points */
        int dpi = 96; /* default */
        if (parent)
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
          dpi = (int)parent->screen()->logicalDotsPerInch();
#else
          dpi = parent->logicalDpiY();
#endif
        }
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

  /* If no valid font was set, use default */
  if (initial_font.family().isEmpty())
  {
    initial_font = QFont();
  }

  color_str = iupAttribGet(ih, "COLOR");
  if (color_str)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(color_str, &r, &g, &b))
      initial_color = QColor(r, g, b);
  }

  QFontDialog::FontDialogOptions options = QFontDialog::FontDialogOptions();

  QFontDialog* dialog = new QFontDialog(parent);
  dialog->setCurrentFont(initial_font);

  /* To support PREVIEWTEXT, SHOWCOLOR, or HELP_CB, we need the non-native dialog */
  const char* preview_text = iupAttribGet(ih, "PREVIEWTEXT");
  bool show_color = iupAttribGetBoolean(ih, "SHOWCOLOR");
  bool has_help = (IupGetCallback(ih, "HELP_CB") != nullptr);

  if (preview_text || show_color || has_help)
    dialog->setOption(QFontDialog::DontUseNativeDialog, true);

  const char* title = iupAttribGet(ih, "TITLE");
  if (title)
    dialog->setWindowTitle(QString::fromUtf8(title));
  else
    dialog->setWindowTitle(QString::fromUtf8("Select Font"));

  if (preview_text && strcmp(preview_text, "NONE") != 0)
  {
    /* Find the sample text edit widget and set preview text */
    QList<QLineEdit*> lineEdits = dialog->findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits)
    {
      /* The sample text is typically in a read-only line edit */
      if (edit && edit->objectName().contains("sampleEdit", Qt::CaseInsensitive))
      {
        edit->setText(QString::fromUtf8(preview_text));
        break;
      }
    }

    /* Also try text edits */
    QList<QTextEdit*> textEdits = dialog->findChildren<QTextEdit*>();
    for (QTextEdit* edit : textEdits)
    {
      if (edit)
      {
        edit->setPlainText(QString::fromUtf8(preview_text));
        break;
      }
    }
  }

  /* Add help button if HELP_CB exists */
  if (has_help)
  {
    /* Find the button box and add help button */
    QDialogButtonBox* button_box = dialog->findChild<QDialogButtonBox*>();
    if (button_box)
    {
      QPushButton* help_button = button_box->addButton(
        QString::fromUtf8("Help"), QDialogButtonBox::HelpRole);

      QObject::connect(help_button, &QPushButton::clicked, [ih, dialog]() {
        Icallback cb = (Icallback)IupGetCallback(ih, "HELP_CB");
        if (cb && cb(ih) == IUP_CLOSE)
          dialog->reject();
      });
    }
  }

  ih->handle = (InativeHandle*)dialog;
  iupDialogUpdatePosition(ih);
  ih->handle = nullptr;

  response = dialog->exec();

  if (response == QDialog::Accepted)
  {
    QFont selected_font = dialog->selectedFont();
    QFontInfo font_info(selected_font);

    /* Build font string in IUP format */
    QString family = font_info.family();
    int point_size = font_info.pointSize();
    bool is_bold = font_info.bold();
    bool is_italic = font_info.italic();

    /* Get underline and strikeout from the font directly */
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

    /* Store color if color selection was shown
     * Note: Standard QFontDialog doesn't provide color selection.
     * This is a Qt platform limitation. Windows uses a custom dialog template, and Cocoa uses NSFontPanel with text attributes.
     * For Qt, we preserve the input COLOR value as a workaround. */
    if (show_color)
    {
      /* Keep the initial color - Qt limitation */
      /* Color remains as set via the COLOR attribute before popup */
    }
    else
    {
      iupAttribSet(ih, "COLOR", nullptr);
    }
  }
  else
  {
    iupAttribSet(ih, "VALUE", nullptr);
    iupAttribSet(ih, "COLOR", nullptr);
    iupAttribSet(ih, "STATUS", nullptr);
  }

  delete dialog;

  /* Restore focus to parent dialog (similar to MessageDlg and other dialogs) */
  if (parent_ih && iupObjectCheck(parent_ih))
  {
    IupSetFocus(parent_ih);
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qtFontDlgPopup;

  /* IupFontDialog Attributes */
  iupClassRegisterAttribute(ic, "COLOR", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCOLOR", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWTEXT", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
}
