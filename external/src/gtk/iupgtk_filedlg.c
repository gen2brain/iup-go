/** \file
 * \brief IupFileDlg pre-defined dialog
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
#include "iup_predialogs.h"
#include "iup_array.h"
#include "iup_drvinfo.h"

#include "iupgtk_drv.h"


static int gtkIsFile(const char* name)
{
  return g_file_test(name, G_FILE_TEST_IS_REGULAR);
}

static int gtkIsDirectory(const char* name)
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

static char* gtkFileDlgGetNextStr(char* str)
{
  /* after the 0 there is another string, 
     must know a priori how many strings are before using this */
  int len = (int)strlen(str);
  return str+len+1;
}

static void gtkFileDlgGetMultipleFiles(Ihandle* ih, GSList* list)
{
  char *filename = iupgtkStrConvertFromFilename((char*)list->data);

  char* dir = iupStrFileGetPath(filename);
  int dir_len = (int)strlen(dir);
  iupAttribSetStr(ih, "DIRECTORY", dir);

  /* check if just one file is selected */
  if (!list->next)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */


    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    iupAttribSetStrId(ih, "MULTIVALUE", 1, filename + dir_len);

    iupAttribSetStr(ih, "VALUE", filename);  /* here value is not separated by '|' */

    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);

    g_free(list->data);  /* must release also the list item */
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

    while (list)
    {
      filename = iupgtkStrConvertFromFilename((char*)list->data);
      len = (int)strlen(filename) - dir_len;

      cur_len = iupArrayCount(names_array);

      all_names = iupArrayAdd(names_array, len + 1);
      memcpy(all_names + cur_len, filename + dir_len, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", count, filename + dir_len);
      count++;

      g_free(list->data);  /* must release also the list item */
      list = list->next;
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

static void gtkFileDlgUpdatePreviewGLCanvas(Ihandle* ih)
{
  Ihandle* glcanvas = IupGetAttributeHandle(ih, "PREVIEWGLCANVAS");
  if (glcanvas)
  {
    iupAttribSet(glcanvas, iupgtkGetNativeWindowHandleName(), iupAttribGet(ih, iupgtkGetNativeWindowHandleName()));
    glcanvas->iclass->Map(glcanvas);  /* this will call Map only for the IupGLCanvas, NOT for the IupCanvas */
  }
}

static void gtkFileDlgPreviewRealize(GtkWidget *widget, Ihandle *ih)
{
  iupAttribSet(ih, "PREVIEWDC", iupgtkGetNativeGraphicsContext(widget));
  iupAttribSet(ih, "WID", (char*)widget);

  iupAttribSet(ih, "DRAWABLE", (char*)iupgtkGetWindow(widget));

  iupAttribSet(ih, iupgtkGetNativeWindowHandleName(), iupgtkGetNativeWidgetHandle(widget));
  if (iupdrvGetDisplay())
    iupAttribSet(ih, "XDISPLAY", (char*)iupdrvGetDisplay());

  gtkFileDlgUpdatePreviewGLCanvas(ih);
}

static void gtkFileDlgRealize(GtkWidget *widget, Ihandle *ih)
{
  /* callback here always exists */
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  cb(ih, NULL, "INIT");
  (void)widget;
}

static gboolean gtkFileDlgPreviewConfigureEvent(GtkWidget *widget, GdkEventConfigure *evt, Ihandle *ih)
{
  iupAttribSetInt(ih, "PREVIEWWIDTH", evt->width);
  iupAttribSetInt(ih, "PREVIEWHEIGHT", evt->height);

  (void)widget;
  return FALSE;
}

#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean gtkFileDlgPreviewDraw(GtkWidget *widget, cairo_t *cr, Ihandle *ih)
#else
static gboolean gtkFileDlgPreviewExposeEvent(GtkWidget *widget, GdkEventExpose *evt, Ihandle *ih)
#endif
{
  GtkFileChooser *file_chooser = (GtkFileChooser*)iupAttribGet(ih, "_IUPDLG_FILE_CHOOSER");
  char *filename = gtk_file_chooser_get_preview_filename(file_chooser);
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");

#if GTK_CHECK_VERSION(3, 0, 0)
  iupAttribSet(ih, "CAIRO_CR", (char*)cr);
#else
  (void)evt;
#endif

  /* callback here always exists */
  if (gtkIsFile(filename))
    cb(ih, iupgtkStrConvertFromFilename(filename), "PAINT");
  else
    cb(ih, NULL, "PAINT");

  iupAttribSet(ih, "CAIRO_CR", NULL);
  g_free(filename);
 
  (void)widget;
  return TRUE;  /* stop other handlers */
}

static void gtkFileDlgUpdatePreview(GtkFileChooser *file_chooser, Ihandle* ih)
{
  char *filename = gtk_file_chooser_get_preview_filename(file_chooser);

  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (cb) /* safety check - callback here always exists */
  {
    if (gtkIsFile(filename))
      cb(ih, iupgtkStrConvertFromFilename(filename), "SELECT");
    else
      cb(ih, iupgtkStrConvertFromFilename(filename), "OTHER");
  }

  g_free (filename);

  gtk_file_chooser_set_preview_widget_active(file_chooser, TRUE);
}
  
static char* gtkFileCheckExt(Ihandle* ih, const char* filename)
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

static int gtkFileDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  GtkWidget* dialog;
  GtkWidget* preview_canvas = NULL;
  GtkFileChooserAction action;
  const char *ok, *cancel, *open, *save, *help;
  IFnss file_cb;
  char* value;
  int response, filter_count = 0;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
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
#if GTK_CHECK_VERSION(3, 10, 0)
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      value = "Save _As";
    else
      value = "_Open";
#else
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      value = GTK_STOCK_SAVE_AS;
    else
      value = GTK_STOCK_OPEN;

    {
      GtkStockItem item;
      gtk_stock_lookup(value, &item);
      value = item.label;
    }
#endif

    iupAttribSetStr(ih, "TITLE", iupgtkStrConvertFromSystem(value));
    value = iupAttribGet(ih, "TITLE");
    iupStrRemoveChar(value, '_');
  }

#if GTK_CHECK_VERSION(3, 10, 0)
  ok = "_OK";
  cancel = "_Cancel";
  save = "_Save";
  open = "_Open";
  help = "_Help";
#else
  ok = GTK_STOCK_OK;
  cancel = GTK_STOCK_CANCEL;
  save = GTK_STOCK_SAVE;
  open = GTK_STOCK_OPEN;
  help = GTK_STOCK_HELP;
#endif

  dialog = gtk_file_chooser_dialog_new(iupgtkStrConvertToSystem(value), (GtkWindow*)parent, action, 
                                       cancel, GTK_RESPONSE_CANCEL, 
                                       NULL);
  if (!dialog)
    return IUP_ERROR;

  if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
    gtk_dialog_add_button(GTK_DIALOG(dialog), save, GTK_RESPONSE_OK);
  else if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
    gtk_dialog_add_button(GTK_DIALOG(dialog), open, GTK_RESPONSE_OK);
  else
    gtk_dialog_add_button(GTK_DIALOG(dialog), ok, GTK_RESPONSE_OK);

  if (IupGetCallback(ih, "HELP_CB"))
    gtk_dialog_add_button(GTK_DIALOG(dialog), help, GTK_RESPONSE_HELP);

#if GTK_CHECK_VERSION(2, 6, 0)
  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(dialog), TRUE);
#endif

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && action == GTK_FILE_CHOOSER_ACTION_OPEN)
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

#if GTK_CHECK_VERSION(2, 8, 0)
  if (!iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT") && action == GTK_FILE_CHOOSER_ACTION_SAVE)
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
#endif

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
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), iupgtkStrConvertToFilename(value));

  value = iupAttribGet(ih, "FILE");
  if (value)
  {
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), iupgtkStrConvertToFilename(value));
    else
    {
      if (gtkIsFile(value))  /* check if file exists */
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), iupgtkStrConvertToFilename(value));
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

      pattern = gtkFileDlgGetNextStr(name);

      pattern_count = iupStrReplace(pattern, ';', 0)+1;

      gtk_file_filter_set_name(filter, iupgtkStrConvertToSystem(name));

      for (j=0; j<pattern_count && pattern[0]; j++)
      {
        gtk_file_filter_add_pattern(filter, pattern);
        if (j<pattern_count-1)
          pattern = gtkFileDlgGetNextStr(pattern);
      }

      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

      sprintf(atrib, "_IUPDLG_FILTER%d", i+1);
      iupAttribSet(ih, atrib, (char*)filter);

      if (i+1 == filter_index)
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

      name = gtkFileDlgGetNextStr(pattern);
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

      gtk_file_filter_set_name(filter, iupgtkStrConvertToSystem(info));

      fstr = filters;
      for (i=0; i<pattern_count && fstr[0]; i++)
      {
        gtk_file_filter_add_pattern(filter, fstr);
        fstr = gtkFileDlgGetNextStr(fstr);
      }
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
      free(filters);
    }
  }

  file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (file_cb && action != GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
  {
    g_signal_connect(GTK_FILE_CHOOSER(dialog), "update-preview", G_CALLBACK(gtkFileDlgUpdatePreview), ih);
    g_signal_connect(dialog, "realize", G_CALLBACK(gtkFileDlgRealize), ih);

    if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
    {
      GtkWidget* frame = gtk_frame_new(NULL);
      int preview_width = iupAttribGetInt(ih, "PREVIEWWIDTH");
      int preview_height = iupAttribGetInt(ih, "PREVIEWHEIGHT");
      if (preview_width <= 0) preview_width = 200;
      if (preview_height <= 0) preview_height = 150;
      gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
      gtk_widget_set_size_request(frame, preview_width, preview_height);

      preview_canvas = gtk_drawing_area_new();
      gtk_widget_set_double_buffered(preview_canvas, FALSE);
      gtk_container_add(GTK_CONTAINER(frame), preview_canvas);
      gtk_widget_show(preview_canvas);

      g_signal_connect(preview_canvas, "configure-event", G_CALLBACK(gtkFileDlgPreviewConfigureEvent), ih);
#if GTK_CHECK_VERSION(3, 0, 0)
      g_signal_connect(preview_canvas, "draw", G_CALLBACK(gtkFileDlgPreviewDraw), ih);
#else
      g_signal_connect(preview_canvas, "expose-event", G_CALLBACK(gtkFileDlgPreviewExposeEvent), ih);
#endif
      g_signal_connect(preview_canvas, "realize", G_CALLBACK(gtkFileDlgPreviewRealize), ih);
      g_signal_connect(G_OBJECT(preview_canvas), "button-press-event", G_CALLBACK(iupgtkButtonEvent), ih);
      g_signal_connect(G_OBJECT(preview_canvas), "button-release-event", G_CALLBACK(iupgtkButtonEvent), ih);
      g_signal_connect(G_OBJECT(preview_canvas), "motion-notify-event", G_CALLBACK(iupgtkMotionNotifyEvent), ih);

      /* To receive mouse events on a drawing area, you will need to enable them. */
      gtk_widget_add_events(preview_canvas, GDK_EXPOSURE_MASK |
                            GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK |
                            GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
                            GDK_SCROLL_MASK |  /* Added for GTK3, but it seems to work ok for GTK2 */
                            GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                            GDK_FOCUS_CHANGE_MASK | GDK_STRUCTURE_MASK);

      iupAttribSet(ih, "_IUPDLG_FILE_CHOOSER", (char*)dialog);

      gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog), frame);
    }
  }
  
  /* initialize the widget */
  gtk_widget_realize(GTK_WIDGET(dialog));
  
  ih->handle = GTK_WIDGET(dialog);
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;  /* reset handle */

  do 
  {
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_HELP)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE)
        response = GTK_RESPONSE_CANCEL;
    }
    else if (response == GTK_RESPONSE_OK)
    {
      char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      int file_exist = gtkIsFile(filename);
      int dir_exist = gtkIsDirectory(filename);
      g_free(filename);

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

          filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
          ret = file_cb(ih, iupgtkStrConvertFromFilename(filename), "OK");
          g_free(filename);
          
          if (ret == IUP_IGNORE || ret == IUP_CONTINUE)
          {
            if (ret == IUP_CONTINUE)
            {
              value = iupAttribGet(ih, "FILE");
              if (value)
              {
                if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
                  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), iupgtkStrConvertToFilename(value));
                else
                {
                  if (gtkIsFile(value))  /* check if file exists */
                    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), iupgtkStrConvertToFilename(value));
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
  {
    if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
      iupgtkReleaseNativeGraphicsContext(preview_canvas, (void*)iupAttribGet(ih, "PREVIEWDC"));

    file_cb(ih, NULL, "FINISH");
  }

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
      GSList* file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

      gtkFileDlgGetMultipleFiles(ih, file_list);

      g_slist_free(file_list);
      file_exist = 1;
      dir_exist = 0;
    }
    else
    {
      char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      filename = gtkFileCheckExt(ih, filename);
      iupAttribSetStr(ih, "VALUE", iupgtkStrConvertFromFilename(filename));
      file_exist = gtkIsFile(filename);
      dir_exist = gtkIsDirectory(filename);

      /* store the directory */
      {
        char* dir = iupStrFileGetPath(filename);
        iupAttribSetStr(ih, "DIRECTORY", dir);
        free(dir);
      }

      g_free(filename);
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

    if (action != GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))  /* do change the current directory */
    {
      /* GtkFileChooser does not change the current directory */
      char* dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
      if (dir) 
      {
        g_chdir(dir);
        g_free(dir);
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

  gtk_widget_destroy(GTK_WIDGET(dialog));  

  return IUP_NOERROR;
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = gtkFileDlgPopup;

  iupClassRegisterAttribute(ic, "PREVIEWWIDTH", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWHEIGHT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* IupFileDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
