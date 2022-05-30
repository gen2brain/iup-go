/** \file
 * \brief IupFontDlg pre-defined dialog
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

#include "iupgtk_drv.h"


static int gtkFontDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  GtkWidget* dialog;
  int response;
  char* preview_text, *font;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

#if GTK_CHECK_VERSION(3, 2, 0)
  dialog = gtk_font_chooser_dialog_new(iupgtkStrConvertToSystem(iupAttribGet(ih, "TITLE")), GTK_WINDOW(parent));
#else
  dialog = gtk_font_selection_dialog_new(iupgtkStrConvertToSystem(iupAttribGet(ih, "TITLE")));
#endif
  if (!dialog)
    return IUP_ERROR;

#if !GTK_CHECK_VERSION(3, 2, 0)
  if (parent)
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
#endif

  font = iupAttribGet(ih, "VALUE");
  if (!font)
    font = IupGetGlobal("DEFAULTFONT");

#if GTK_CHECK_VERSION(3, 2, 0)
  gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), font);
#else
  gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), font);
#endif

  preview_text = iupAttribGet(ih, "PREVIEWTEXT");
  if (preview_text)
  {
    preview_text = iupgtkStrConvertToSystem(preview_text);
#if GTK_CHECK_VERSION(3, 2, 0)
    if (iupStrEqualNoCase(preview_text, "NONE"))
      gtk_font_chooser_set_show_preview_entry(GTK_FONT_CHOOSER(dialog), FALSE);
    else
      gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER(dialog), preview_text);
#else
    gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(dialog), preview_text);
#endif
  }

  if (IupGetCallback(ih, "HELP_CB"))
  {
#if GTK_CHECK_VERSION(3, 10, 0)
    const char* help = "_Help";
#else
    const char* help = GTK_STOCK_HELP;
#endif

    gtk_dialog_add_button(GTK_DIALOG(dialog), help, GTK_RESPONSE_HELP);
  }
  
  /* initialize the widget */
  gtk_widget_realize(GTK_WIDGET(dialog));
  
  ih->handle = GTK_WIDGET(dialog);
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;
                                    
  do 
  {
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_HELP)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
        response = GTK_RESPONSE_CANCEL;
    }
  } while (response == GTK_RESPONSE_HELP);

  if (response == GTK_RESPONSE_OK)
  {
#if GTK_CHECK_VERSION(3, 2, 0)
    char* fontname = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
#else
    char* fontname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
#endif
    iupAttribSetStr(ih, "VALUE", fontname);
    g_free(fontname);
    iupAttribSet(ih, "STATUS", "1");
  }
  else
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "STATUS", NULL);
  }

  gtk_widget_destroy(GTK_WIDGET(dialog));  

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtkFontDlgPopup;

  /* IupFontDialog GTK Only */
  iupClassRegisterAttribute(ic, "PREVIEWTEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
