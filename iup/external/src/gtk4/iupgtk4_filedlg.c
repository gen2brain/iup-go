/** \file
 * \brief IupFileDlg pre-defined dialog for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_dlglist.h"
#include "iup_predialogs.h"
#include "iup_array.h"
#include "iup_drv.h"

#include "iupgtk4_drv.h"


static int gtk4IsFile(const char* name)
{
  return g_file_test(name, G_FILE_TEST_IS_REGULAR);
}

static int gtk4IsDirectory(const char* name)
{
  return g_file_test(name, G_FILE_TEST_IS_DIR);
}

static void iupStrRemoveChar(char* str, char c)
{
  char* p = str;
  while (*str)
  {
    if (*str != c)
    {
      *p = *str;
      p++;
    }

    str++;
  }
  *p = 0;
}

static char* gtk4FileDlgGetNextStr(char* str)
{
  /* after the 0 there is another string, must know a priori how many strings are before using this */
  int len = (int)strlen(str);
  return str+len+1;
}

static void gtk4FileDlgGetMultipleFiles(Ihandle* ih, GListModel* list)
{
  guint n_items = g_list_model_get_n_items(list);
  if (n_items == 0)
    return;

  GFile* first_file = g_list_model_get_item(list, 0);
  char* filename = iupgtk4StrConvertFromFilename(g_file_get_path(first_file));

  char* dir = iupStrFileGetPath(filename);
  int dir_len = (int)strlen(dir);
  iupAttribSetStr(ih, "DIRECTORY", dir);

  /* check if just one file is selected */
  if (n_items == 1)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    iupAttribSetStrId(ih, "MULTIVALUE", 1, filename + dir_len);
    iupAttribSetStr(ih, "VALUE", filename);  /* here value is not separated by '|' */
    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);

    g_object_unref(first_file);
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));  /* just set an initial size, but count is 0 */
    char *all_names;
    int cur_len, count = 0;

    int len = dir_len;
    if (dir[dir_len - 1] == '/' || dir[dir_len - 1] == '\\') len--;  /* remove last '/' */

    all_names = iupArrayAdd(names_array, len + 1);
    memcpy(all_names, dir, len);  /* does NOT includes last separator */
    all_names[len] = '|';

    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */
    count++;

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    for (guint i = 0; i < n_items; i++)
    {
      GFile* file = g_list_model_get_item(list, i);
      filename = iupgtk4StrConvertFromFilename(g_file_get_path(file));
      len = (int)strlen(filename) - dir_len;

      cur_len = iupArrayCount(names_array);

      all_names = iupArrayAdd(names_array, len + 1);
      memcpy(all_names + cur_len, filename + dir_len, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", count, filename + dir_len);
      count++;

      g_object_unref(file);
    }

    iupAttribSetInt(ih, "MULTIVALUECOUNT", count);

    cur_len = iupArrayCount(names_array);
    all_names = iupArrayInc(names_array);
    all_names[cur_len + 1] = 0;

    iupAttribSetStr(ih, "VALUE", all_names);

    iupArrayDestroy(names_array);
  }

  free(dir);
}


static char* gtk4FileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = (int)strlen(filename);
    int ext_len = (int)strlen(ext);
    if (filename[len - ext_len - 1] != '.')
    {
      char* new_filename = g_malloc(len + ext_len + 1 + 1);
      memcpy(new_filename, filename, len);
      new_filename[len] = '.';
      memcpy(new_filename + len + 1, ext, ext_len);
      new_filename[len + ext_len + 1] = 0;
      return new_filename;
    }
  }

  return (char*)filename;
}

/* Context for blocking async dialog */
typedef struct {
  Ihandle* ih;
  GFile* result_file;
  GListModel* result_files;
  gboolean cancelled;
  gboolean done;
  GMainLoop* loop;
  int filter_count;
  GtkFileFilter* selected_filter;
  GListStore* filters_store;
} Gtk4FileDialogData;

static void gtk4FileDlgOpenCallback(GObject* source, GAsyncResult* result, gpointer user_data)
{
  Gtk4FileDialogData* data = (Gtk4FileDialogData*)user_data;
  GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
  GError* error = NULL;

  data->result_file = gtk_file_dialog_open_finish(dialog, result, &error);
  if (error)
  {
    if (g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_CANCELLED) ||
        g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
      data->cancelled = TRUE;
    g_error_free(error);
  }

  data->selected_filter = gtk_file_dialog_get_default_filter(dialog);
  data->done = TRUE;
  g_main_loop_quit(data->loop);
}

static void gtk4FileDlgOpenMultipleCallback(GObject* source, GAsyncResult* result, gpointer user_data)
{
  Gtk4FileDialogData* data = (Gtk4FileDialogData*)user_data;
  GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
  GError* error = NULL;

  data->result_files = gtk_file_dialog_open_multiple_finish(dialog, result, &error);
  if (error)
  {
    if (g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_CANCELLED) ||
        g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
      data->cancelled = TRUE;
    g_error_free(error);
  }

  data->selected_filter = gtk_file_dialog_get_default_filter(dialog);
  data->done = TRUE;
  g_main_loop_quit(data->loop);
}

static void gtk4FileDlgSaveCallback(GObject* source, GAsyncResult* result, gpointer user_data)
{
  Gtk4FileDialogData* data = (Gtk4FileDialogData*)user_data;
  GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
  GError* error = NULL;

  data->result_file = gtk_file_dialog_save_finish(dialog, result, &error);
  if (error)
  {
    if (g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_CANCELLED) ||
        g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
      data->cancelled = TRUE;
    g_error_free(error);
  }

  data->selected_filter = gtk_file_dialog_get_default_filter(dialog);
  data->done = TRUE;
  g_main_loop_quit(data->loop);
}

static void gtk4FileDlgSelectFolderCallback(GObject* source, GAsyncResult* result, gpointer user_data)
{
  Gtk4FileDialogData* data = (Gtk4FileDialogData*)user_data;
  GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
  GError* error = NULL;

  data->result_file = gtk_file_dialog_select_folder_finish(dialog, result, &error);
  if (error)
  {
    if (g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_CANCELLED) ||
        g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
      data->cancelled = TRUE;
    g_error_free(error);
  }

  data->done = TRUE;
  g_main_loop_quit(data->loop);
}

static GtkWindow* gtk4FileDlgGetParentWindow(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    return (GtkWindow*)parent;

  Ihandle* ih_focus = IupGetFocus();
  if (ih_focus)
  {
    Ihandle* dlg = IupGetDialog(ih_focus);
    if (dlg && dlg->handle)
      return (GtkWindow*)dlg->handle;
  }

  /* Fallback: find first visible IUP dialog */
  {
    Ihandle* dlg_iter = iupDlgListFirst();
    while (dlg_iter)
    {
      if (dlg_iter->handle && dlg_iter != ih && iupdrvIsVisible(dlg_iter))
        return (GtkWindow*)dlg_iter->handle;
      dlg_iter = iupDlgListNext();
    }
  }

  return NULL;
}

/* Default mode implementation using GtkFileDialog (GTK 4.10+) */
static int gtk4FileDlgPopupDefault(Ihandle* ih, int x, int y)
{
  GtkWindow* parent;
  GtkFileDialog* dialog;
  Gtk4FileDialogData data;
  char* value;
  int is_save = 0, is_dir = 0, is_multiple = 0;

  (void)x;
  (void)y;

  parent = gtk4FileDlgGetParentWindow(ih);

  memset(&data, 0, sizeof(data));
  data.ih = ih;

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
    is_save = 1;
  else if (iupStrEqualNoCase(value, "DIR"))
    is_dir = 1;

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && !is_save && !is_dir)
    is_multiple = 1;

  dialog = gtk_file_dialog_new();
  if (!dialog)
    return IUP_ERROR;

  gtk_file_dialog_set_modal(dialog, TRUE);

  value = iupAttribGet(ih, "TITLE");
  if (value)
    gtk_file_dialog_set_title(dialog, iupgtk4StrConvertToSystem(value));
  else
  {
    if (is_save)
      gtk_file_dialog_set_title(dialog, "Save As");
    else if (is_dir)
      gtk_file_dialog_set_title(dialog, "Select Folder");
    else
      gtk_file_dialog_set_title(dialog, "Open");
  }

  if (is_save)
    gtk_file_dialog_set_accept_label(dialog, "_Save");
  else if (is_dir)
    gtk_file_dialog_set_accept_label(dialog, "_Select");
  else
    gtk_file_dialog_set_accept_label(dialog, "_Open");

  value = iupAttribGet(ih, "FILE");
  if (value && value[0] != 0 && (value[0] == '/' || value[1] == ':'))
  {
    char* dir = iupStrFileGetPath(value);
    int len = (int)strlen(dir);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    free(dir);
    iupAttribSetStr(ih, "FILE", value+len);
  }

  value = iupAttribGet(ih, "DIRECTORY");
  if (value)
  {
    GFile* file = g_file_new_for_path(iupgtk4StrConvertToFilename(value));
    gtk_file_dialog_set_initial_folder(dialog, file);
    g_object_unref(file);
  }

  value = iupAttribGet(ih, "FILE");
  if (value)
  {
    if (is_save)
      gtk_file_dialog_set_initial_name(dialog, iupgtk4StrConvertToFilename(value));
    else if (gtk4IsFile(value))
    {
      GFile* file = g_file_new_for_path(iupgtk4StrConvertToFilename(value));
      gtk_file_dialog_set_initial_file(dialog, file);
      g_object_unref(file);
    }
  }

  value = iupAttribGet(ih, "EXTFILTER");
  if (value && !is_dir)
  {
    char *name, *pattern, *filters = iupStrDup(value);
    char atrib[30];
    int i, pattern_count, j;
    int filter_index = iupAttribGetInt(ih, "FILTERUSED");
    GtkFileFilter* default_filter = NULL;

    if (!filter_index)
      filter_index = 1;

    data.filter_count = iupStrReplace(filters, '|', 0) / 2;
    data.filters_store = g_list_store_new(GTK_TYPE_FILE_FILTER);

    name = filters;
    for (i = 0; i < data.filter_count && name[0]; i++)
    {
      GtkFileFilter *filter = gtk_file_filter_new();

      pattern = gtk4FileDlgGetNextStr(name);
      pattern_count = iupStrReplace(pattern, ';', 0) + 1;

      gtk_file_filter_set_name(filter, iupgtk4StrConvertToSystem(name));

      for (j = 0; j < pattern_count && pattern[0]; j++)
      {
        gtk_file_filter_add_pattern(filter, pattern);
        if (j < pattern_count - 1)
          pattern = gtk4FileDlgGetNextStr(pattern);
      }

      g_list_store_append(data.filters_store, filter);

      sprintf(atrib, "_IUPDLG_FILTER%d", i + 1);
      iupAttribSet(ih, atrib, (char*)filter);

      if (i + 1 == filter_index)
        default_filter = filter;

      g_object_unref(filter);
      name = gtk4FileDlgGetNextStr(pattern);
    }

    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(data.filters_store));
    if (default_filter)
      gtk_file_dialog_set_default_filter(dialog, default_filter);

    free(filters);
  }
  else
  {
    value = iupAttribGet(ih, "FILTER");
    if (value && !is_dir)
    {
      char* filters = iupStrDup(value), *fstr;
      int pattern_count, i;
      GtkFileFilter *filter = gtk_file_filter_new();
      char* info = iupAttribGet(ih, "FILTERINFO");
      if (!info)
        info = value;

      pattern_count = iupStrReplace(filters, ';', 0) + 1;

      gtk_file_filter_set_name(filter, iupgtk4StrConvertToSystem(info));

      fstr = filters;
      for (i = 0; i < pattern_count && fstr[0]; i++)
      {
        gtk_file_filter_add_pattern(filter, fstr);
        fstr = gtk4FileDlgGetNextStr(fstr);
      }

      data.filters_store = g_list_store_new(GTK_TYPE_FILE_FILTER);
      g_list_store_append(data.filters_store, filter);
      gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(data.filters_store));
      gtk_file_dialog_set_default_filter(dialog, filter);

      g_object_unref(filter);
      free(filters);
    }
  }

  data.loop = g_main_loop_new(NULL, FALSE);

  if (is_save)
    gtk_file_dialog_save(dialog, (GtkWindow*)parent, NULL, gtk4FileDlgSaveCallback, &data);
  else if (is_dir)
    gtk_file_dialog_select_folder(dialog, (GtkWindow*)parent, NULL, gtk4FileDlgSelectFolderCallback, &data);
  else if (is_multiple)
    gtk_file_dialog_open_multiple(dialog, (GtkWindow*)parent, NULL, gtk4FileDlgOpenMultipleCallback, &data);
  else
    gtk_file_dialog_open(dialog, (GtkWindow*)parent, NULL, gtk4FileDlgOpenCallback, &data);

  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);

  if (!data.cancelled && (data.result_file || data.result_files))
  {
    int file_exist, dir_exist;

    if (data.filter_count && data.selected_filter)
    {
      int i;
      char atrib[30];

      for (i = 0; i < data.filter_count; i++)
      {
        sprintf(atrib, "_IUPDLG_FILTER%d", i + 1);
        if (data.selected_filter == (GtkFileFilter*)iupAttribGet(ih, atrib))
          iupAttribSetInt(ih, "FILTERUSED", i + 1);
      }
    }

    if (is_multiple && data.result_files)
    {
      gtk4FileDlgGetMultipleFiles(ih, data.result_files);
      g_object_unref(data.result_files);
      file_exist = 1;
      dir_exist = 0;
    }
    else if (data.result_file)
    {
      char* filename = g_file_get_path(data.result_file);
      filename = gtk4FileCheckExt(ih, filename);
      iupAttribSetStr(ih, "VALUE", iupgtk4StrConvertFromFilename(filename));
      file_exist = gtk4IsFile(filename);
      dir_exist = gtk4IsDirectory(filename);

      {
        char* dir = iupStrFileGetPath(filename);
        iupAttribSetStr(ih, "DIRECTORY", dir);
        free(dir);
      }

      if (!is_dir && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
      {
        char* dir = iupStrFileGetPath(filename);
        if (dir)
        {
          g_chdir(dir);
          free(dir);
        }
      }

      g_free(filename);
      g_object_unref(data.result_file);
    }
    else
    {
      file_exist = 0;
      dir_exist = 0;
    }

    if (dir_exist)
    {
      iupAttribSet(ih, "FILEEXIST", NULL);
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      if (file_exist)
      {
        iupAttribSet(ih, "FILEEXIST", "YES");
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        iupAttribSet(ih, "FILEEXIST", "NO");
        iupAttribSet(ih, "STATUS", "1");
      }
    }
  }
  else
  {
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  if (data.filters_store)
    g_object_unref(data.filters_store);

  g_object_unref(dialog);

  return IUP_NOERROR;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static int gtk4FileDlgPopupLegacy(Ihandle* ih, int x, int y)
{
  GtkWidget* dialog;
  GtkFileChooserAction action;
  const char *ok, *cancel, *open, *save, *help;
  IFnss file_cb;
  char* value;
  int filter_count = 0;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
    action = GTK_FILE_CHOOSER_ACTION_SAVE;
  else if (iupStrEqualNoCase(value, "DIR"))
    action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  else
    action = GTK_FILE_CHOOSER_ACTION_OPEN;

  value = iupAttribGet(ih, "TITLE");
  if (!value)
  {
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      value = "Save As";
    else
      value = "Open";

    iupAttribSetStr(ih, "TITLE", iupgtk4StrConvertFromSystem(value));
    value = iupAttribGet(ih, "TITLE");
  }

  ok = "OK";
  cancel = "Cancel";
  save = "Save";
  open = "Open";
  help = "Help";

  dialog = gtk_file_chooser_dialog_new(iupgtk4StrConvertToSystem(value), NULL, action,
                                       cancel, GTK_RESPONSE_CANCEL, NULL);
  if (!dialog)
    return IUP_ERROR;

  iupgtk4DialogSetTransientFor(GTK_WINDOW(dialog), ih);

  if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
    gtk_dialog_add_button(GTK_DIALOG(dialog), save, GTK_RESPONSE_OK);
  else if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
    gtk_dialog_add_button(GTK_DIALOG(dialog), open, GTK_RESPONSE_OK);
  else
    gtk_dialog_add_button(GTK_DIALOG(dialog), ok, GTK_RESPONSE_OK);

  if (IupGetCallback(ih, "HELP_CB"))
    gtk_dialog_add_button(GTK_DIALOG(dialog), help, GTK_RESPONSE_HELP);

  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
    g_object_set(dialog, "show-hidden", TRUE, NULL);

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && action == GTK_FILE_CHOOSER_ACTION_OPEN)
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

  /* just check for the path inside FILE */
  value = iupAttribGet(ih, "FILE");
  if (value && value[0] != 0 && (value[0] == '/' || value[1] == ':'))
  {
    char* dir = iupStrFileGetPath(value);
    int len = (int)strlen(dir);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    free(dir);

    iupAttribSetStr(ih, "FILE", value+len);  /* remove directory from value */
  }

  value = iupAttribGet(ih, "DIRECTORY");
  if (value)
  {
    GFile* file = g_file_new_for_path(iupgtk4StrConvertToFilename(value));
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), file, NULL);
    g_object_unref(file);
  }

  value = iupAttribGet(ih, "FILE");
  if (value)
  {
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), iupgtk4StrConvertToFilename(value));
    else
    {
      if (gtk4IsFile(value))  /* check if file exists */
      {
        GFile* file = g_file_new_for_path(iupgtk4StrConvertToFilename(value));
        gtk_file_chooser_set_file(GTK_FILE_CHOOSER(dialog), file, NULL);
        g_object_unref(file);
      }
    }
  }

  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
  {
    char *name, *pattern, *filters = iupStrDup(value);
    char atrib[30];
    int i, pattern_count, j;
    int filter_index = iupAttribGetInt(ih, "FILTERUSED");
    if (!filter_index)
      filter_index = 1;

    filter_count = iupStrReplace(filters, '|', 0) / 2;

    name = filters;
    for (i=0; i<filter_count && name[0]; i++)
    {
      GtkFileFilter *filter = gtk_file_filter_new();

      pattern = gtk4FileDlgGetNextStr(name);

      pattern_count = iupStrReplace(pattern, ';', 0)+1;

      gtk_file_filter_set_name(filter, iupgtk4StrConvertToSystem(name));

      for (j=0; j<pattern_count && pattern[0]; j++)
      {
        gtk_file_filter_add_pattern(filter, pattern);
        if (j<pattern_count-1)
          pattern = gtk4FileDlgGetNextStr(pattern);
      }

      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

      sprintf(atrib, "_IUPDLG_FILTER%d", i+1);
      iupAttribSet(ih, atrib, (char*)filter);

      if (i+1 == filter_index)
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

      name = gtk4FileDlgGetNextStr(pattern);
    }

    free(filters);
  }
  else
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
    {
      char* filters = iupStrDup(value), *fstr;
      int pattern_count, i;
      GtkFileFilter *filter = gtk_file_filter_new();
      char* info = iupAttribGet(ih, "FILTERINFO");
      if (!info)
        info = value;

      pattern_count = iupStrReplace(filters, ';', 0)+1;

      gtk_file_filter_set_name(filter, iupgtk4StrConvertToSystem(info));

      fstr = filters;
      for (i=0; i<pattern_count && fstr[0]; i++)
      {
        gtk_file_filter_add_pattern(filter, fstr);
        fstr = gtk4FileDlgGetNextStr(fstr);
      }
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
      free(filters);
    }
  }

  file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");

  gtk_widget_realize(GTK_WIDGET(dialog));

  ih->handle = GTK_WIDGET(dialog);
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;  /* reset handle */

  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_widget_set_visible(dialog, TRUE);

  gint response;
  gint* response_ptr = &response;

  /* Response callback to capture the response value */
  void response_cb(GtkDialog* d, gint r, gpointer data) {
    *(gint*)data = r;
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
    else if (response == GTK_RESPONSE_OK)
    {
      GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
      int file_exist = 0;
      int dir_exist = 0;

      if (file)
      {
        char* filename = g_file_get_path(file);
        file_exist = gtk4IsFile(filename);
        dir_exist = gtk4IsDirectory(filename);
        g_free(filename);
        g_object_unref(file);
      }

      if (action == GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
      {
        if (!dir_exist)
        {
          IupMessageError(ih, "IUP_INVALIDDIR");
          response = GTK_RESPONSE_HELP; /* to leave the dialog open */
          continue;
        }
      }
      else if (!iupAttribGetBoolean(ih, "MULTIPLEFILES"))
      {
        if (dir_exist)
        {
          IupMessageError(ih, "IUP_FILEISDIR");
          response = GTK_RESPONSE_HELP; /* to leave the dialog open */
          continue;
        }

        if (!file_exist)  /* if do not exist check ALLOWNEW */
        {
          value = iupAttribGet(ih, "ALLOWNEW");
          if (!value)
          {
            if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
              value = "YES";
            else
              value = "NO";
          }

          if (!iupStrBoolean(value))
          {
            IupMessageError(ih, "IUP_FILENOTEXIST");
            response = GTK_RESPONSE_HELP; /* to leave the dialog open */
            continue;
          }
        }

        if (file_cb)
        {
          int ret;
          GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
          char* filename = g_file_get_path(file);

          ret = file_cb(ih, iupgtk4StrConvertFromFilename(filename), "OK");

          g_free(filename);
          g_object_unref(file);

          if (ret == IUP_IGNORE || ret == IUP_CONTINUE)
          {
            if (ret == IUP_CONTINUE)
            {
              value = iupAttribGet(ih, "FILE");
              if (value)
              {
                if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
                  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), iupgtk4StrConvertToFilename(value));
                else
                {
                  if (gtk4IsFile(value))  /* check if file exists */
                  {
                    GFile* file = g_file_new_for_path(iupgtk4StrConvertToFilename(value));
                    gtk_file_chooser_set_file(GTK_FILE_CHOOSER(dialog), file, NULL);
                    g_object_unref(file);
                  }
                }
              }
            }

            response = GTK_RESPONSE_HELP; /* to leave the dialog open */
            continue;
          }
        }
      }
    }
  } while (response == GTK_RESPONSE_HELP);

  if (file_cb)
    file_cb(ih, NULL, "FINISH");

  if (response == GTK_RESPONSE_OK)
  {
    int file_exist, dir_exist;

    if (filter_count)
    {
      int i;
      char atrib[30];
      GtkFileFilter* filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));

      for (i=0; i<filter_count; i++)
      {
        sprintf(atrib, "_IUPDLG_FILTER%d", i+1);
        if (filter == (GtkFileFilter*)iupAttribGet(ih, atrib))
          iupAttribSetInt(ih, "FILTERUSED", i+1);
      }
    }

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
    {
      GListModel* file_list = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));

      gtk4FileDlgGetMultipleFiles(ih, file_list);

      g_object_unref(file_list);
      file_exist = 1;
      dir_exist = 0;
    }
    else
    {
      GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
      char* filename = g_file_get_path(file);
      filename = gtk4FileCheckExt(ih, filename);
      iupAttribSetStr(ih, "VALUE", iupgtk4StrConvertFromFilename(filename));
      file_exist = gtk4IsFile(filename);
      dir_exist = gtk4IsDirectory(filename);

      /* store the directory */
      {
        char* dir = iupStrFileGetPath(filename);
        iupAttribSetStr(ih, "DIRECTORY", dir);
        free(dir);
      }

      g_free(filename);
      g_object_unref(file);
    }

    if (dir_exist)
    {
      iupAttribSet(ih, "FILEEXIST", NULL);
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      if (file_exist)  /* check if file exists */
      {
        iupAttribSet(ih, "FILEEXIST", "YES");
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        iupAttribSet(ih, "FILEEXIST", "NO");
        iupAttribSet(ih, "STATUS", "1");
      }
    }

    if (action != GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    {
      /* GtkFileChooser does not change the current directory */
      GFile* current_folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
      if (current_folder)
      {
        char* dir = g_file_get_path(current_folder);
        g_chdir(dir);
        g_free(dir);
        g_object_unref(current_folder);
      }
    }
  }
  else
  {
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  gtk_window_destroy(GTK_WINDOW(dialog));

  return IUP_NOERROR;
}
G_GNUC_END_IGNORE_DEPRECATIONS

static int gtk4FileDlgPopup(Ihandle* ih, int x, int y)
{
  char* value;
  int use_legacy = 0;

  value = iupAttribGet(ih, "PORTAL");
  if (value && !iupStrBoolean(value))
    use_legacy = 1;

  if (use_legacy)
    return gtk4FileDlgPopupLegacy(ih, x, y);

  return gtk4FileDlgPopupDefault(ih, x, y);
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtk4FileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWPREVIEW", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWWIDTH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWHEIGHT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
