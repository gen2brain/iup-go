/** \file
 * \brief Qt IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <QMessageBox>
#include <QPushButton>
#include <QWidget>
#include <QString>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
}

#include "iupqt_drv.h"


/* Custom response codes to avoid conflicts */
#define IUP_RESPONSE_1    1
#define IUP_RESPONSE_2    2
#define IUP_RESPONSE_3    3
#define IUP_RESPONSE_HELP 4

/****************************************************************************
 * AUTOMODAL Attribute
 ****************************************************************************/

static char* qtMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  int automodal = 1;
  if (parent)
    automodal = 0;
  return iupStrReturnBoolean(automodal);
}

/****************************************************************************
 * Message Dialog Popup
 ****************************************************************************/

static int qtMessageDlgPopup(Ihandle* ih, int x, int y)
{
  QWidget* parent = iupqtGetParentWidget(ih);
  QMessageBox* dialog;
  QMessageBox::Icon icon_type = QMessageBox::NoIcon;
  const char* icon;
  const char* buttons;
  const char* title;
  const char* value;
  QPushButton* button1 = nullptr;
  QPushButton* button2 = nullptr;
  QPushButton* button3 = nullptr;
  QPushButton* help_button = nullptr;
  int response;
  int button_def;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  icon = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(icon, "ERROR"))
    icon_type = QMessageBox::Critical;
  else if (iupStrEqualNoCase(icon, "WARNING"))
    icon_type = QMessageBox::Warning;
  else if (iupStrEqualNoCase(icon, "INFORMATION"))
    icon_type = QMessageBox::Information;
  else if (iupStrEqualNoCase(icon, "QUESTION"))
    icon_type = QMessageBox::Question;

  dialog = new QMessageBox(icon_type, QString(), QString(), QMessageBox::NoButton, parent);

  if (!dialog)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");
  if (title)
    dialog->setWindowTitle(QString::fromUtf8(title));

  value = iupAttribGet(ih, "VALUE");
  if (value)
    dialog->setText(QString::fromUtf8(value));

  const char* ok_text = IupGetLanguageString("IUP_OK");
  const char* cancel_text = IupGetLanguageString("IUP_CANCEL");
  const char* yes_text = IupGetLanguageString("IUP_YES");
  const char* no_text = IupGetLanguageString("IUP_NO");
  const char* retry_text = IupGetLanguageString("IUP_RETRY");
  const char* help_text = IupGetLanguageString("IUP_HELP");

  if (!ok_text) ok_text = "OK";
  if (!cancel_text) cancel_text = "Cancel";
  if (!yes_text) yes_text = "Yes";
  if (!no_text) no_text = "No";
  if (!retry_text) retry_text = "Retry";
  if (!help_text) help_text = "Help";

  buttons = iupAttribGetStr(ih, "BUTTONS");
  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    button1 = dialog->addButton(QString::fromUtf8(ok_text), QMessageBox::AcceptRole);
    button2 = dialog->addButton(QString::fromUtf8(cancel_text), QMessageBox::RejectRole);
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    button1 = dialog->addButton(QString::fromUtf8(retry_text), QMessageBox::AcceptRole);
    button2 = dialog->addButton(QString::fromUtf8(cancel_text), QMessageBox::RejectRole);
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    button1 = dialog->addButton(QString::fromUtf8(yes_text), QMessageBox::YesRole);
    button2 = dialog->addButton(QString::fromUtf8(no_text), QMessageBox::NoRole);
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    button1 = dialog->addButton(QString::fromUtf8(yes_text), QMessageBox::YesRole);
    button2 = dialog->addButton(QString::fromUtf8(no_text), QMessageBox::NoRole);
    button3 = dialog->addButton(QString::fromUtf8(cancel_text), QMessageBox::RejectRole);
  }
  else /* OK */
  {
    button1 = dialog->addButton(QString::fromUtf8(ok_text), QMessageBox::AcceptRole);
  }

  /* Add Help button if HELP_CB exists */
  if (IupGetCallback(ih, "HELP_CB"))
  {
    help_button = dialog->addButton(QString::fromUtf8(help_text), QMessageBox::HelpRole);
  }

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 3 && button3)
    dialog->setDefaultButton(button3);
  else if (button_def == 2 && button2)
    dialog->setDefaultButton(button2);
  else if (button1)
    dialog->setDefaultButton(button1);

  /* Only force position if no parent, Qt centers on parent automatically */
  if (!parent)
  {
    ih->handle = (InativeHandle*)dialog;
    iupDialogUpdatePosition(ih);
    ih->handle = nullptr;
  }

  do
  {
    response = 0;
    dialog->exec();

    QAbstractButton* clicked_button = dialog->clickedButton();

    if (clicked_button == help_button)
    {
      response = IUP_RESPONSE_HELP;
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
      {
        /* User wants to close from help callback */
        if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
          response = IUP_RESPONSE_3;
        else if (iupStrEqualNoCase(buttons, "OK"))
          response = IUP_RESPONSE_1;
        else
          response = IUP_RESPONSE_2;
      }
    }
    else if (clicked_button == button1)
      response = IUP_RESPONSE_1;
    else if (clicked_button == button2)
      response = IUP_RESPONSE_2;
    else if (clicked_button == button3)
      response = IUP_RESPONSE_3;
    else
    {
      /* Dialog closed without button click (e.g., X button or Escape key) */
      if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
        response = IUP_RESPONSE_3; /* Cancel */
      else if (iupStrEqualNoCase(buttons, "OK"))
        response = IUP_RESPONSE_1; /* OK */
      else
        response = IUP_RESPONSE_2; /* Cancel/No */
    }

  } while (response == IUP_RESPONSE_HELP);

  if (response == IUP_RESPONSE_3)
    IupSetAttribute(ih, "BUTTONRESPONSE", "3");
  else if (response == IUP_RESPONSE_2)
    IupSetAttribute(ih, "BUTTONRESPONSE", "2");
  else
    IupSetAttribute(ih, "BUTTONRESPONSE", "1");

  delete dialog;

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qtMessageDlgPopup;

  /* Register AUTOMODAL attribute (feature parity with Windows and Cocoa) */
  iupClassRegisterAttribute(ic, "AUTOMODAL", qtMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
}
