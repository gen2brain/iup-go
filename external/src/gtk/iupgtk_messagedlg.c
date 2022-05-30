/** \file
 * \brief GTK IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupgtk_drv.h"

/* Sometimes GTK decides to invert the buttons position because of the GNOME Guidelines.
   To avoid that we define different Ids for the buttons. */
#define IUP_RESPONSE_1 -100
#define IUP_RESPONSE_2 -200
#define IUP_RESPONSE_3 -300
#define IUP_RESPONSE_HELP -400

#ifndef GTK_MESSAGE_OTHER
#define GTK_MESSAGE_OTHER GTK_MESSAGE_INFO
#endif

static int gtkMessageDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  GtkMessageType type = GTK_MESSAGE_OTHER;
  GtkWidget* dialog;
  char *icon, *buttons, *title;
  const char *ok, *cancel, *yes, *no, *help, *retry = IupGetLanguageString("IUP_RETRY");
  int response, button_def;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  icon = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(icon, "ERROR"))
    type = GTK_MESSAGE_ERROR;
  else if (iupStrEqualNoCase(icon, "WARNING"))
    type = GTK_MESSAGE_WARNING;
  else if (iupStrEqualNoCase(icon, "INFORMATION"))
    type = GTK_MESSAGE_INFO;
  else if (iupStrEqualNoCase(icon, "QUESTION"))
    type = GTK_MESSAGE_QUESTION;

  dialog = gtk_message_dialog_new((GtkWindow*)parent,
                                  0,
                                  type,
                                  GTK_BUTTONS_NONE,
                                  "%s",
                                  iupgtkStrConvertToSystem(iupAttribGet(ih, "VALUE")));
  if (!dialog)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");
  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), iupgtkStrConvertToSystem(title));

#if GTK_CHECK_VERSION(3, 10, 0)
  ok = "_OK";
  cancel = "_Cancel";
  yes = "_Yes";
  no = "_No";
  help = "_Help";
#else
  ok = GTK_STOCK_OK;
  cancel = GTK_STOCK_CANCEL;
  yes = GTK_STOCK_YES;
  no = GTK_STOCK_NO;
  help = GTK_STOCK_HELP;
#endif

  buttons = iupAttribGetStr(ih, "BUTTONS");
  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          ok,
                          IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          cancel,
                          IUP_RESPONSE_2);
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          retry,
                          IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          cancel,
                          IUP_RESPONSE_2);
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          yes,
                          IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          no,
                          IUP_RESPONSE_2);
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          yes,
                          IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          no,
                          IUP_RESPONSE_2);
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          cancel,
                          IUP_RESPONSE_3);
  }
  else /* OK */
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          ok,
                          IUP_RESPONSE_1);
  }

  if (IupGetCallback(ih, "HELP_CB"))
    gtk_dialog_add_button(GTK_DIALOG(dialog), help, IUP_RESPONSE_HELP);

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def == 3)
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), IUP_RESPONSE_3);
  else if (button_def == 2)
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), IUP_RESPONSE_2);
  else
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), IUP_RESPONSE_1);
  
  /* initialize the widget */
  gtk_widget_realize(dialog);
  
  ih->handle = dialog;
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;

  do 
  {
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == IUP_RESPONSE_HELP)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
      {
        if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
          response = IUP_RESPONSE_3;
        else if(iupStrEqualNoCase(buttons, "OK"))
          response = IUP_RESPONSE_1;
        else
          response = IUP_RESPONSE_2;
      }
    }
  } while (response == IUP_RESPONSE_HELP);

  if (response == IUP_RESPONSE_3)
    IupSetAttribute(ih, "BUTTONRESPONSE", "3");
  else if (response == IUP_RESPONSE_2)
    IupSetAttribute(ih, "BUTTONRESPONSE", "2");
  else
    IupSetAttribute(ih, "BUTTONRESPONSE", "1");

  gtk_widget_destroy(dialog);  

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtkMessageDlgPopup;
}
