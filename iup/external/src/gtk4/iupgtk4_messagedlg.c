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


typedef struct {
  GMainLoop* loop;
  int button_response;
} AlertDialogData;

static void gtk4AlertDialogCallback(GObject* source, GAsyncResult* result, gpointer user_data)
{
  GtkAlertDialog* dialog = GTK_ALERT_DIALOG(source);
  AlertDialogData* data = (AlertDialogData*)user_data;
  GError* error = NULL;

  int response = gtk_alert_dialog_choose_finish(dialog, result, &error);

  if (error)
  {
    g_error_free(error);
    data->button_response = -1;
  }
  else
  {
    data->button_response = response;
  }

  g_main_loop_quit(data->loop);
}

static int gtk4MessageDlgPopup(Ihandle* ih, int x, int y)
{
  GtkAlertDialog* dialog;
  GtkWindow* parent = NULL;
  char *buttons;
  const char* button_labels[5];
  int num_buttons = 0;
  int button_def;
  int cancel_button = -1;
  AlertDialogData data = {0};

  (void)x;
  (void)y;

  dialog = gtk_alert_dialog_new("%s", iupgtk4StrConvertToSystem(iupAttribGet(ih, "VALUE")));
  if (!dialog)
    return IUP_ERROR;

  gtk_alert_dialog_set_modal(dialog, TRUE);

  /* Get parent window */
  {
    Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");
    if (!parent_ih)
      parent_ih = IupGetAttributeHandle(NULL, "PARENTDIALOG");
    if (parent_ih && parent_ih->handle)
      parent = GTK_WINDOW(parent_ih->handle);
  }

  buttons = iupAttribGetStr(ih, "BUTTONS");
  if (iupStrEqualNoCase(buttons, "OKCANCEL"))
  {
    button_labels[0] = "OK";
    button_labels[1] = "Cancel";
    num_buttons = 2;
    cancel_button = 1;
  }
  else if (iupStrEqualNoCase(buttons, "RETRYCANCEL"))
  {
    button_labels[0] = IupGetLanguageString("IUP_RETRY");
    button_labels[1] = "Cancel";
    num_buttons = 2;
    cancel_button = 1;
  }
  else if (iupStrEqualNoCase(buttons, "YESNO"))
  {
    button_labels[0] = "Yes";
    button_labels[1] = "No";
    num_buttons = 2;
    cancel_button = 1;
  }
  else if (iupStrEqualNoCase(buttons, "YESNOCANCEL"))
  {
    button_labels[0] = "Yes";
    button_labels[1] = "No";
    button_labels[2] = "Cancel";
    num_buttons = 3;
    cancel_button = 2;
  }
  else
  {
    button_labels[0] = "OK";
    num_buttons = 1;
  }

  button_labels[num_buttons] = NULL;
  gtk_alert_dialog_set_buttons(dialog, button_labels);

  button_def = iupAttribGetInt(ih, "BUTTONDEFAULT");
  if (button_def >= 1 && button_def <= num_buttons)
    gtk_alert_dialog_set_default_button(dialog, button_def - 1);
  else
    gtk_alert_dialog_set_default_button(dialog, 0);

  if (cancel_button >= 0)
    gtk_alert_dialog_set_cancel_button(dialog, cancel_button);

  data.loop = g_main_loop_new(NULL, FALSE);
  data.button_response = -1;

  gtk_alert_dialog_choose(dialog, parent, NULL, gtk4AlertDialogCallback, &data);

  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);

  if (data.button_response >= 0)
    IupSetInt(ih, "BUTTONRESPONSE", data.button_response + 1);
  else
    IupSetAttribute(ih, "BUTTONRESPONSE", "1");

  g_object_unref(dialog);

  return IUP_NOERROR;
}

void iupdrvMessageDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtk4MessageDlgPopup;

  /* GtkAlertDialog (GTK 4.10+) does not support dialog type icons */
  iupClassRegisterAttribute(ic, "DIALOGTYPE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
