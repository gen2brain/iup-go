/** \file
 * \brief IupFontDlg pre-defined dialog for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include <string.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupgtk4_drv.h"


static int gtk4FontDlgPopup(Ihandle* ih, int x, int y)
{
  GtkWidget* dialog;
  char* preview_text, *font;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  /* GTK4 uses GtkFontChooserDialog (same as GTK3.2+) */
  dialog = gtk_font_chooser_dialog_new(iupgtk4StrConvertToSystem(iupAttribGet(ih, "TITLE")), NULL);
  if (!dialog)
    return IUP_ERROR;

  iupgtk4DialogSetTransientFor(GTK_WINDOW(dialog), ih);

  font = iupAttribGet(ih, "VALUE");
  if (!font)
    font = IupGetGlobal("DEFAULTFONT");

  gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), font);

  preview_text = iupAttribGet(ih, "PREVIEWTEXT");
  if (preview_text)
  {
    preview_text = iupgtk4StrConvertToSystem(preview_text);
    if (iupStrEqualNoCase(preview_text, "NONE"))
      gtk_font_chooser_set_show_preview_entry(GTK_FONT_CHOOSER(dialog), FALSE);
    else
      gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER(dialog), preview_text);
  }

  if (IupGetCallback(ih, "HELP_CB"))
  {
    const char* help = "Help";
    gtk_dialog_add_button(GTK_DIALOG(dialog), help, GTK_RESPONSE_HELP);
  }

  /* initialize the widget */
  gtk_widget_realize(GTK_WIDGET(dialog));

  ih->handle = GTK_WIDGET(dialog);
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;

  /* gtk_dialog_run removed - use manual event loop */
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_widget_set_visible(dialog, TRUE);

  gint response;
  gint* response_ptr = &response;

  /* Response callback to capture the response value */
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

    if (response == GTK_RESPONSE_HELP)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
        response = GTK_RESPONSE_CANCEL;
    }
  } while (response == GTK_RESPONSE_HELP);

  if (response == GTK_RESPONSE_OK)
  {
    char* fontname = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
    iupAttribSetStr(ih, "VALUE", fontname);
    g_free(fontname);
    iupAttribSet(ih, "STATUS", "1");
  }
  else
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "STATUS", NULL);
  }

  gtk_window_destroy(GTK_WINDOW(dialog));

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtk4FontDlgPopup;

  iupClassRegisterAttribute(ic, "PREVIEWTEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
