/** \file
 * \brief IupFileDlg pre-defined dialog - FLTK implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_predialogs.h"
#include "iup_array.h"
}

#include "iupfltk_drv.h"

#if !defined(_WIN32) && !defined(__APPLE__)
extern "C" {
#include "unix/iupunix_portal.h"
}
#endif


static int fltkIsFile(const char* path)
{
  if (!path || path[0] == 0)
    return 0;

  struct stat st;
  if (stat(path, &st) == 0 && (st.st_mode & S_IFREG))
    return 1;
  return 0;
}

static char* fltkFileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = (int)strlen(filename);
    int ext_len = (int)strlen(ext);

    if (len > ext_len && filename[len - ext_len - 1] == '.')
    {
      if (strcmp(filename + len - ext_len, ext) == 0)
        return (char*)filename;
    }

    const char* dot = strrchr(filename, '.');
    const char* slash = strrchr(filename, '/');

    if (!dot || (slash && dot < slash))
    {
      int new_len = len + ext_len + 2;
      char* new_filename = (char*)malloc(new_len);
      snprintf(new_filename, new_len, "%s.%s", filename, ext);
      return new_filename;
    }
  }

  return (char*)filename;
}

/* Convert a semicolon-separated pattern to fl_filename_match format.
   "*.png;*.jpg" -> "{*.png,*.jpg}"
   "*.*" -> "*"
   Single pattern stays as-is. */
static void fltkConvertPattern(const char* pattern, char* out, int out_size)
{
  if (strcmp(pattern, "*.*") == 0 || strcmp(pattern, "*") == 0)
  {
    iupStrCopyN(out, out_size, "*");
    return;
  }

  if (!strchr(pattern, ';'))
  {
    /* Single pattern, check for *.* */
    iupStrCopyN(out, out_size, pattern);
    return;
  }

  /* Multiple patterns: wrap in {} and replace ; with , */
  int oi = 0;
  out[oi++] = '{';
  for (int i = 0; pattern[i] && oi < out_size - 2; i++)
  {
    if (pattern[i] == ';')
      out[oi++] = ',';
    else
      out[oi++] = pattern[i];
  }
  out[oi++] = '}';
  out[oi] = '\0';
}

/* Convert IUP EXTFILTER format to Fl_File_Chooser filter format.
   IUP:  "Label|*.png;*.jpg|Label2|*.*"
   FLTK: "Label ({*.png,*.jpg})\tLabel2 (*)" tab-separated */
static char* fltkConvertExtFilter(const char* extfilter)
{
  if (!extfilter || !extfilter[0])
    return NULL;

  char* copy = strdup(extfilter);
  Iarray* result = iupArrayCreate(256, sizeof(char));
  int first = 1;

  char* p = copy;
  while (p && *p)
  {
    char* label = p;
    char* sep = strchr(label, '|');
    if (!sep) break;
    *sep = '\0';

    char* pattern = sep + 1;
    sep = strchr(pattern, '|');
    if (sep) *sep = '\0';

    if (!first)
    {
      int cur = iupArrayCount(result);
      char* buf = (char*)iupArrayAdd(result, 1);
      buf[cur] = '\t';
    }
    first = 0;

    char converted_pattern[512];
    fltkConvertPattern(pattern, converted_pattern, sizeof(converted_pattern));

    char entry[1024];
    snprintf(entry, sizeof(entry), "%s (%s)", label, converted_pattern);

    int entry_len = (int)strlen(entry);
    int cur = iupArrayCount(result);
    char* buf = (char*)iupArrayAdd(result, entry_len);
    memcpy(buf + cur, entry, entry_len);

    p = sep ? sep + 1 : NULL;
  }

  int cur = iupArrayCount(result);
  char* buf = (char*)iupArrayInc(result);
  buf[cur] = '\0';

  char* out = strdup(buf);
  iupArrayDestroy(result);
  free(copy);
  return out;
}

/* Convert IUP FILTER format to Fl_File_Chooser format.
   IUP:  "*.png;*.jpg" or "*.*"
   FLTK: "{*.png,*.jpg}" or "*" */
static char* fltkConvertFilter(const char* filter)
{
  if (!filter || !filter[0])
    return NULL;

  char out[512];
  fltkConvertPattern(filter, out, sizeof(out));
  return strdup(out);
}

static void fltkFileDlgGetMultipleFiles(Ihandle* ih, Fl_File_Chooser* chooser)
{
  int count = chooser->count();
  if (count == 0)
    return;

  const char* first_file = chooser->value(1);
  if (!first_file)
    return;

  char* dir = iupStrFileGetPath(first_file);
  int dir_len = (int)strlen(dir);

  iupAttribSetStr(ih, "DIRECTORY", dir);

  if (count == 1)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);

    int offset = dir_len;
    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      offset = 0;

    iupAttribSetStrId(ih, "MULTIVALUE", 1, first_file + offset);
    iupAttribSetStr(ih, "VALUE", first_file);
    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));
    int cur_len;
    int item_count = 0;

    int len = dir_len;
    if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\'))
      len--;

    char* all_names = (char*)iupArrayAdd(names_array, len + 1);
    memcpy(all_names, dir, len);
    all_names[len] = '|';

    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);
    item_count++;

    int offset = dir_len;
    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      offset = 0;

    for (int i = 1; i <= count; i++)
    {
      const char* file = chooser->value(i);
      if (!file) continue;

      len = (int)strlen(file) - offset;

      cur_len = iupArrayCount(names_array);
      all_names = (char*)iupArrayAdd(names_array, len + 1);
      memcpy(all_names + cur_len, file + offset, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", item_count, file + offset);
      item_count++;
    }

    iupAttribSetInt(ih, "MULTIVALUECOUNT", item_count);

    cur_len = iupArrayCount(names_array);
    all_names = (char*)iupArrayInc(names_array);
    all_names[cur_len] = 0;

    iupAttribSetStr(ih, "VALUE", all_names);

    iupArrayDestroy(names_array);
  }

  free(dir);
}

/*****************************************************************************
 * Preview Canvas for SHOWPREVIEW + FILE_CB
 *****************************************************************************/

class IupFltkPreviewCanvas : public Fl_Widget
{
public:
  Ihandle* iup_handle;

  IupFltkPreviewCanvas(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Widget(x, y, w, h), iup_handle(ih)
  {
    box(FL_DOWN_FRAME);
  }

protected:
  void draw() override
  {
    draw_box();

    IFnss cb = (IFnss)IupGetCallback(iup_handle, "FILE_CB");
    if (cb)
    {
      iupAttribSetInt(iup_handle, "PREVIEWWIDTH", w() - Fl::box_dw(box()));
      iupAttribSetInt(iup_handle, "PREVIEWHEIGHT", h() - Fl::box_dh(box()));

      Fl_File_Chooser* fc = (Fl_File_Chooser*)iupAttribGet(iup_handle, "_IUPFLTK_FILECHOOSER");
      if (fc && fc->value() && fltkIsFile(fc->value()))
        cb(iup_handle, (char*)fc->value(), (char*)"PAINT");
      else
        cb(iup_handle, NULL, (char*)"PAINT");
    }
  }
};

static void fltkFileDlgChooserCallback(Fl_File_Chooser* fc, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (!cb)
    return;

  const char* filename = fc->value();
  if (filename && fltkIsFile(filename))
    cb(ih, (char*)filename, (char*)"SELECT");
  else if (filename)
    cb(ih, (char*)filename, (char*)"OTHER");

  if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
  {
    IupFltkPreviewCanvas* preview = (IupFltkPreviewCanvas*)iupAttribGet(ih, "WID");
    if (preview)
      preview->redraw();
  }
}

/*****************************************************************************
 * Main Popup
 *****************************************************************************/

static int fltkFileDlgPopup(Ihandle* ih, int x, int y)
{
  const char* value;
  int dialogtype;
  int fc_type;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

#if !defined(_WIN32) && !defined(__APPLE__)
  {
    int use_portal = 0;
    value = iupAttribGet(ih, "PORTAL");
    if (value)
      use_portal = iupStrBoolean(value);
    else if (IupGetGlobal("SANDBOX"))
      use_portal = 1;

    if (use_portal)
    {
      if (iupUnixPortalFileDialog(ih) == IUP_NOERROR)
        return IUP_NOERROR;
    }
  }
#endif

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
  {
    fc_type = Fl_File_Chooser::CREATE;
    dialogtype = 1;
  }
  else if (iupStrEqualNoCase(value, "DIR"))
  {
    fc_type = Fl_File_Chooser::DIRECTORY;
    dialogtype = 2;
  }
  else
  {
    if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
      fc_type = Fl_File_Chooser::MULTI;
    else
      fc_type = Fl_File_Chooser::SINGLE;
    dialogtype = 0;
  }

  value = iupAttribGet(ih, "FILE");
  if (value && value[0] != 0 && (value[0] == '/' || value[1] == ':'))
  {
    char* dir = iupStrFileGetPath(value);
    int len = (int)strlen(dir);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    iupAttribSetStr(ih, "FILE", value + len);
    free(dir);
  }

  const char* directory = iupAttribGet(ih, "DIRECTORY");

  char* fc_filter = NULL;
  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
    fc_filter = fltkConvertExtFilter(value);
  else
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
      fc_filter = fltkConvertFilter(value);
  }

  value = iupAttribGet(ih, "TITLE");

  Fl_File_Chooser* chooser = new Fl_File_Chooser(directory, fc_filter, fc_type, value);

  if (fc_filter)
    free(fc_filter);

  if (iupAttribGet(ih, "EXTFILTER"))
  {
    int filter_index = iupAttribGetInt(ih, "FILTERUSED");
    if (filter_index > 0)
      chooser->filter_value(filter_index - 1);
  }

  value = iupAttribGet(ih, "FILE");
  if (value)
    chooser->value(value);

  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
    chooser->showHiddenButton->value(1);

  IupFltkPreviewCanvas* preview_canvas = NULL;
  IFnss file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");

  if (file_cb && dialogtype != 2)
  {
    chooser->callback(fltkFileDlgChooserCallback, ih);
    iupAttribSet(ih, "_IUPFLTK_FILECHOOSER", (char*)chooser);

    if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
    {
      int preview_width = iupAttribGetInt(ih, "PREVIEWWIDTH");
      int preview_height = iupAttribGetInt(ih, "PREVIEWHEIGHT");
      if (preview_width <= 0) preview_width = 200;
      if (preview_height <= 0) preview_height = 150;

      chooser->preview(0);

      preview_canvas = new IupFltkPreviewCanvas(0, 0, preview_width, preview_height, ih);
      chooser->add_extra(preview_canvas);

      ih->handle = (InativeHandle*)preview_canvas;
      iupAttribSet(ih, "WID", (char*)preview_canvas);
    }
  }

  chooser->show();

  Fl_Window* dlg_win = Fl::first_window();
  if (dlg_win)
  {
    if (iupDialogGetNativeParent(ih))
      iupfltkX11SetSkipTaskbar(dlg_win);

    InativeHandle* saved_handle = ih->handle;
    ih->handle = (InativeHandle*)dlg_win;
    iupDialogUpdatePosition(ih);
    ih->handle = saved_handle;
  }

  if (file_cb)
    file_cb(ih, NULL, (char*)"INIT");

  while (chooser->shown())
    Fl::wait();

  if (file_cb)
    file_cb(ih, NULL, (char*)"FINISH");

  int got_value = (chooser->value() && chooser->value()[0]);

  if (got_value)
  {
    if (iupAttribGet(ih, "EXTFILTER"))
      iupAttribSetInt(ih, "FILTERUSED", chooser->filter_value() + 1);

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && dialogtype == 0)
    {
      fltkFileDlgGetMultipleFiles(ih, chooser);
      iupAttribSet(ih, "FILEEXIST", "YES");
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      const char* filename = chooser->value();
      char* final_filename = fltkFileCheckExt(ih, filename);

      if (dialogtype == 1 && !iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT"))
      {
        if (fltkIsFile(final_filename))
        {
          if (!fl_choice("File already exists. Overwrite?", "Cancel", "OK", NULL))
          {
            if (final_filename != filename)
              free(final_filename);

            iupAttribSet(ih, "FILTERUSED", NULL);
            iupAttribSet(ih, "VALUE", NULL);
            iupAttribSet(ih, "FILEEXIST", NULL);
            iupAttribSet(ih, "STATUS", "-1");

            iupAttribSet(ih, "_IUPFLTK_FILECHOOSER", NULL);
            iupAttribSet(ih, "WID", NULL);
            ih->handle = NULL;
            delete chooser;
            delete preview_canvas;
            return IUP_NOERROR;
          }
        }
      }

      iupAttribSetStr(ih, "VALUE", final_filename);

      char* dir = iupStrFileGetPath(final_filename);
      iupAttribSetStr(ih, "DIRECTORY", dir);
      free(dir);

      int file_exist = fltkIsFile(final_filename);

      if (final_filename != filename)
        free(final_filename);

      if (dialogtype == 2)
      {
        iupAttribSet(ih, "FILEEXIST", NULL);
        iupAttribSet(ih, "STATUS", "0");
      }
      else if (file_exist)
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

    if (dialogtype != 2 && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    {
      const char* dir = iupAttribGet(ih, "DIRECTORY");
      if (dir)
        iupdrvSetCurrentDirectory(dir);
    }
  }
  else
  {
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  iupAttribSet(ih, "_IUPFLTK_FILECHOOSER", NULL);
  iupAttribSet(ih, "WID", NULL);
  ih->handle = NULL;

  delete chooser;
  delete preview_canvas;

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = fltkFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
