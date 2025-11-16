/** \file
 * \brief GTK4 IupMessageDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupgtk4_drv.h"


#define IUP_RESPONSE_1 -100
#define IUP_RESPONSE_2 -200
#define IUP_RESPONSE_3 -300
#define IUP_RESPONSE_HELP -400

#ifndef GTK_MESSAGE_OTHER
#define GTK_MESSAGE_OTHER GTK_MESSAGE_INFO
#endif

static int gtk4MessageDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  GtkMessageType type = GTK_MESSAGE_OTHER;
  GtkWidget* dialog;
  char *icon, *buttons, *title;
  const char *ok, *cancel, *yes, *no, *help, *retry = IupGetLanguageString("IUP_RETRY");
  int button_def;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
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

  dialog = gtk_message_dialog_new((GtkWindow*)parent, 0, type, GTK_BUTTONS_NONE, "%s", iupgtk4StrConvertToSystem(iupAttribGet(ih, "VALUE")));
  if (!dialog)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");
  if (title)
    gtk_window_set_title(GTK_WINDOW(dialog), iupgtk4StrConvertToSystem(title));

  ok = "OK";
  cancel = "Cancel";
  yes = "Yes";
  no = "No";
  help = "Help";

  buttons = iupAttribGetStr(ih, "BUTTONS");
  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog), ok, IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog), cancel, IUP_RESPONSE_2);
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog), retry, IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog), cancel, IUP_RESPONSE_2);
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog), yes, IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog), no, IUP_RESPONSE_2);
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog), yes, IUP_RESPONSE_1);
    gtk_dialog_add_button(GTK_DIALOG(dialog), no, IUP_RESPONSE_2);
    gtk_dialog_add_button(GTK_DIALOG(dialog), cancel, IUP_RESPONSE_3);
  }
  else
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog), ok, IUP_RESPONSE_1);
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

  gtk_widget_realize(dialog);

  ih->handle = dialog;
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_widget_set_visible(dialog, TRUE);

  gint response;
  gint* response_ptr = &response;

  void response_cb(GtkDialog* d, gint r, gpointer data) {
    *(gint*)data = r;
    (void)d;
  }

  do
  {
    response = GTK_RESPONSE_NONE;

    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    gulong response_handler = g_signal_connect(dialog, "response", G_CALLBACK(response_cb), response_ptr);
    gulong quit_handler = g_signal_connect_swapped(dialog, "response", G_CALLBACK(g_main_loop_quit), loop);

    g_main_loop_run(loop);

    g_signal_handler_disconnect(dialog, response_handler);
    g_signal_handler_disconnect(dialog, quit_handler);
    g_main_loop_unref(loop);

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

  gtk_window_destroy(GTK_WINDOW(dialog));

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtk4MessageDlgPopup;
}
