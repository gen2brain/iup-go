/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_stdcontrols.h"

#include "iupgtk4_drv.h"


#define IUPGTK4_PRINT_SCALE (96.0 / 72.0)

static void gtk4PrintDlgGetPageSize(GtkPrintContext* context, int* width, int* height)
{
  *width = iupRound(gtk_print_context_get_width(context) * IUPGTK4_PRINT_SCALE);
  *height = iupRound(gtk_print_context_get_height(context) * IUPGTK4_PRINT_SCALE);
}

static void gtk4PrintDlgBeginPrint(GtkPrintOperation* op, GtkPrintContext* context, Ihandle* ih)
{
  int width, height;
  gtk4PrintDlgGetPageSize(context, &width, &height);
  gtk_print_operation_set_n_pages(op, iupPrintDlgPaginate(ih, width, height));
}

static void gtk4PrintDlgDrawPage(GtkPrintOperation* op, GtkPrintContext* context, gint page_nr, Ihandle* ih)
{
  cairo_t* cr = gtk_print_context_get_cairo_context(context);
  int width, height;

  gtk4PrintDlgGetPageSize(context, &width, &height);

  cairo_save(cr);
  cairo_scale(cr, 1.0 / IUPGTK4_PRINT_SCALE, 1.0 / IUPGTK4_PRINT_SCALE);

  if (iupPrintDlgDrawPage(ih, cr, page_nr + 1, width, height) == IUP_CLOSE)
    gtk_print_operation_cancel(op);

  cairo_restore(cr);
}

static int gtk4PrintDlgPopup(Ihandle* ih, int x, int y)
{
  GtkWindow* parent = iupgtk4GetTransientFor(ih);
  GtkPrintOperation* op;
  GtkPageSetup* page_setup;
  GtkPrintOperationResult result;
  GError* error = NULL;
  int page_count;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  op = gtk_print_operation_new();
  gtk_print_operation_set_job_name(op, iupgtk4StrConvertToSystem(iupPrintDlgGetJobName(ih)));
  gtk_print_operation_set_unit(op, GTK_UNIT_POINTS);
  gtk_print_operation_set_embed_page_setup(op, TRUE);

  page_setup = gtk_page_setup_new();
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE"))
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_LANDSCAPE);
  else
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
  gtk_print_operation_set_default_page_setup(op, page_setup);
  g_object_unref(page_setup);

  page_count = iupAttribGetInt(ih, "PAGECOUNT");
  if (page_count > 0)
    gtk_print_operation_set_n_pages(op, page_count);

  g_signal_connect(op, "begin-print", G_CALLBACK(gtk4PrintDlgBeginPrint), ih);
  g_signal_connect(op, "draw-page", G_CALLBACK(gtk4PrintDlgDrawPage), ih);

  result = gtk_print_operation_run(op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, parent, &error);

  if (result == GTK_PRINT_OPERATION_RESULT_APPLY)
  {
    GtkPageOrientation orientation = gtk_page_setup_get_orientation(gtk_print_operation_get_default_page_setup(op));
    if (orientation == GTK_PAGE_ORIENTATION_LANDSCAPE || orientation == GTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE)
      iupAttribSet(ih, "ORIENTATION", "LANDSCAPE");
    else
      iupAttribSet(ih, "ORIENTATION", "PORTRAIT");
    iupAttribSet(ih, "STATUS", "1");
  }
  else if (result == GTK_PRINT_OPERATION_RESULT_ERROR)
  {
    if (error)
      g_error_free(error);
    iupAttribSet(ih, "STATUS", "-1");
  }
  else
    iupAttribSet(ih, "STATUS", "0");

  g_object_unref(op);

  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtk4PrintDlgPopup;
}
