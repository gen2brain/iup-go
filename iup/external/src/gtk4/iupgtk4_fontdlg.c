/** \file
 * \brief IupFontDlg
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


typedef struct {
  GMainLoop* loop;
  PangoFontDescription* font_desc;
  gboolean success;
} FontDialogData;

static void gtk4FontDialogCallback(GObject* source, GAsyncResult* result, gpointer user_data)
{
  GtkFontDialog* dialog = GTK_FONT_DIALOG(source);
  FontDialogData* data = (FontDialogData*)user_data;

  data->font_desc = gtk_font_dialog_choose_font_finish(dialog, result, NULL);
  data->success = (data->font_desc != NULL);

  g_main_loop_quit(data->loop);
}

static int gtk4FontDlgPopup(Ihandle* ih, int x, int y)
{
  GtkFontDialog* dialog;
  GtkWindow* parent = NULL;
  char* font;
  PangoFontDescription* initial_font = NULL;
  FontDialogData data = {0};

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  dialog = gtk_font_dialog_new();
  if (!dialog)
    return IUP_ERROR;

  gtk_font_dialog_set_title(dialog, iupgtk4StrConvertToSystem(iupAttribGet(ih, "TITLE")));
  gtk_font_dialog_set_modal(dialog, TRUE);

  font = iupAttribGet(ih, "VALUE");
  if (!font)
    font = IupGetGlobal("DEFAULTFONT");

  if (font)
    initial_font = pango_font_description_from_string(font);

  {
    Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");
    if (!parent_ih)
      parent_ih = IupGetAttributeHandle(NULL, "PARENTDIALOG");
    if (parent_ih && parent_ih->handle)
      parent = GTK_WINDOW(parent_ih->handle);
  }

  data.loop = g_main_loop_new(NULL, FALSE);
  data.font_desc = NULL;
  data.success = FALSE;

  gtk_font_dialog_choose_font(dialog, parent, initial_font, NULL, gtk4FontDialogCallback, &data);

  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);

  if (data.success && data.font_desc)
  {
    char* fontname = pango_font_description_to_string(data.font_desc);
    iupAttribSetStr(ih, "VALUE", fontname);
    g_free(fontname);
    pango_font_description_free(data.font_desc);
    iupAttribSet(ih, "STATUS", "1");
  }
  else
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "STATUS", NULL);
  }

  if (initial_font)
    pango_font_description_free(initial_font);

  g_object_unref(dialog);

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtk4FontDlgPopup;

  /* GtkFontDialog (GTK 4.10+) does not support preview text customization */
  iupClassRegisterAttribute(ic, "PREVIEWTEXT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
