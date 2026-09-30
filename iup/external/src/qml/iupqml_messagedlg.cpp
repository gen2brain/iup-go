/** \file
 * \brief IupMessageDlg pre-defined dialog - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickWindow>
#include <QEventLoop>
#include <QString>
#include <QVariant>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
}

#include "iupqml_drv.h"


#define IUP_RESPONSE_1    1
#define IUP_RESPONSE_2    2
#define IUP_RESPONSE_3    3
#define IUP_RESPONSE_HELP 4

#define IUPQML_BUTTON_OK     0x00000400
#define IUPQML_BUTTON_YES    0x00004000
#define IUPQML_BUTTON_NO     0x00010000
#define IUPQML_BUTTON_RETRY  0x00080000
#define IUPQML_BUTTON_CANCEL 0x00400000
#define IUPQML_BUTTON_HELP   0x01000000

static char* qmlMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  return iupStrReturnBoolean(parent ? 0 : 1);
}

static int qmlMessageDlgPopup(Ihandle* ih, int x, int y)
{
  const char* buttons;
  const char* value;
  int button1 = 0, button2 = 0, button3 = 0;
  int flags = 0;
  int response;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  QObject* dialog = iupqmlCreateObject("import QtQuick.Dialogs\nMessageDialog { }");
  if (!dialog)
    return IUP_ERROR;

  value = iupAttribGet(ih, "TITLE");
  if (value)
    dialog->setProperty("title", QString::fromUtf8(value));

  int host_owned;
  QQuickWindow* host = iupqmlDialogHostWindow(ih, &host_owned);
  dialog->setProperty("parentWindow", QVariant::fromValue<QObject*>(host));
  dialog->setProperty("modality", static_cast<int>(Qt::ApplicationModal));

  value = iupAttribGet(ih, "VALUE");
  if (value)
    dialog->setProperty("text", QString::fromUtf8(value));

  buttons = iupAttribGetStr(ih, "BUTTONS");
  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    button1 = IUPQML_BUTTON_OK;
    button2 = IUPQML_BUTTON_CANCEL;
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    button1 = IUPQML_BUTTON_RETRY;
    button2 = IUPQML_BUTTON_CANCEL;
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    button1 = IUPQML_BUTTON_YES;
    button2 = IUPQML_BUTTON_NO;
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    button1 = IUPQML_BUTTON_YES;
    button2 = IUPQML_BUTTON_NO;
    button3 = IUPQML_BUTTON_CANCEL;
  }
  else
    button1 = IUPQML_BUTTON_OK;

  flags = button1 | button2 | button3;
  if (IupGetCallback(ih, "HELP_CB"))
    flags |= IUPQML_BUTTON_HELP;

  dialog->setProperty("buttons", flags);

  do
  {
    QEventLoop loop;
    int clicked = 0;

    QObject* click_slot = iupqmlConnect(dialog, "buttonClicked(QPlatformDialogHelper::StandardButton,QPlatformDialogHelper::ButtonRole)", [&clicked](void** args) {
      clicked = *static_cast<int*>(args[1]);
    });
    QObject* visible_slot = iupqmlConnect(dialog, "visibleChanged()", [dialog, &loop](void**) {
      if (!dialog->property("visible").toBool())
        loop.quit();
    });

    response = 0;
    iupqmlCallMethod(dialog, "open");
    if (dialog->property("visible").toBool())
      loop.exec();

    delete click_slot;
    delete visible_slot;

    if (clicked == IUPQML_BUTTON_HELP)
    {
      response = IUP_RESPONSE_HELP;
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
      {
        if (button3)
          response = IUP_RESPONSE_3;
        else if (!button2)
          response = IUP_RESPONSE_1;
        else
          response = IUP_RESPONSE_2;
      }
    }
    else if (clicked == button1)
      response = IUP_RESPONSE_1;
    else if (clicked && clicked == button2)
      response = IUP_RESPONSE_2;
    else if (clicked && clicked == button3)
      response = IUP_RESPONSE_3;
    else
    {
      if (button3)
        response = IUP_RESPONSE_3;
      else if (!button2)
        response = IUP_RESPONSE_1;
      else
        response = IUP_RESPONSE_2;
    }

  } while (response == IUP_RESPONSE_HELP);

  if (response == IUP_RESPONSE_3)
    IupSetAttribute(ih, "BUTTONRESPONSE", "3");
  else if (response == IUP_RESPONSE_2)
    IupSetAttribute(ih, "BUTTONRESPONSE", "2");
  else
    IupSetAttribute(ih, "BUTTONRESPONSE", "1");

  delete dialog;
  if (host_owned)
    delete host;

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = qmlMessageDlgPopup;

  iupClassRegisterAttribute(ic, "AUTOMODAL", qmlMessageDlgGetAutoModalAttrib, nullptr, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUTTONDEFAULT", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
