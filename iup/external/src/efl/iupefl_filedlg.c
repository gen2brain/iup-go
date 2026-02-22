/** \file
 * \brief IupFileDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_predialogs.h"
#include "iup_array.h"

#include "iupefl_drv.h"


typedef struct {
  char** patterns;
  int pattern_count;
} EflFileFilterData;

static int efl_filedlg_status = 0;
static Eina_List* efl_filedlg_paths = NULL;

static int eflIsFile(const char* name)
{
  struct stat st;
  if (stat(name, &st) != 0)
    return 0;
  return S_ISREG(st.st_mode);
}

static int eflIsDirectory(const char* name)
{
  struct stat st;
  if (stat(name, &st) != 0)
    return 0;
  return S_ISDIR(st.st_mode);
}

static char* eflFileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = (int)strlen(filename);
    int ext_len = (int)strlen(ext);
    if (len > 0 && filename[len - ext_len - 1] != '.')
    {
      char* new_filename = (char*)malloc(len + ext_len + 2);
      memcpy(new_filename, filename, len);
      new_filename[len] = '.';
      memcpy(new_filename + len + 1, ext, ext_len);
      new_filename[len + ext_len + 1] = 0;
      return new_filename;
    }
  }
  return (char*)filename;
}

static char* eflFileDlgGetNextStr(char* str)
{
  int len = (int)strlen(str);
  return str + len + 1;
}

static Eina_Bool eflFileDlgFilterFunc(const char* path, Eina_Bool dir, void* data)
{
  EflFileFilterData* filter_data = (EflFileFilterData*)data;
  const char* filename;
  int i;

  if (dir)
    return EINA_TRUE;

  filename = strrchr(path, '/');
  if (filename)
    filename++;
  else
    filename = path;

  for (i = 0; i < filter_data->pattern_count; i++)
  {
    if (fnmatch(filter_data->patterns[i], filename, 0) == 0)
      return EINA_TRUE;
  }

  return EINA_FALSE;
}

static EflFileFilterData* eflFileDlgParsePatterns(char* pattern_str)
{
  EflFileFilterData* data;
  char* filters;
  char* fstr;
  int count, i;

  filters = iupStrDup(pattern_str);
  count = iupStrReplace(filters, ';', 0) + 1;

  data = (EflFileFilterData*)malloc(sizeof(EflFileFilterData));
  data->patterns = (char**)malloc(count * sizeof(char*));
  data->pattern_count = count;

  fstr = filters;
  for (i = 0; i < count; i++)
  {
    data->patterns[i] = strdup(fstr);
    if (i < count - 1)
      fstr = eflFileDlgGetNextStr(fstr);
  }

  free(filters);
  return data;
}

static void eflFileDlgFreeFilterData(EflFileFilterData* data)
{
  int i;
  if (!data)
    return;
  for (i = 0; i < data->pattern_count; i++)
    free(data->patterns[i]);
  free(data->patterns);
  free(data);
}

static void eflFileDlgGetMultipleFiles(Ihandle* ih, Eina_List* paths)
{
  Eina_List* l;
  const char* path;
  char* filename;
  char* dir;
  int dir_len;

  if (!paths || eina_list_count(paths) == 0)
    return;

  filename = iupeflStrConvertFromSystem((char*)eina_list_data_get(paths));
  dir = iupStrFileGetPath(filename);
  dir_len = (int)strlen(dir);
  iupAttribSetStr(ih, "DIRECTORY", dir);

  if (eina_list_count(paths) == 1)
  {
    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    iupAttribSetStrId(ih, "MULTIVALUE", 1, filename + dir_len);
    iupAttribSetStr(ih, "VALUE", filename);
    iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));
    char* all_names;
    int cur_len, count = 0;
    int len = dir_len;

    if (dir[dir_len - 1] == '/')
      len--;

    all_names = iupArrayAdd(names_array, len + 1);
    memcpy(all_names, dir, len);
    all_names[len] = '|';

    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);
    count++;

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    EINA_LIST_FOREACH(paths, l, path)
    {
      filename = iupeflStrConvertFromSystem((char*)path);
      len = (int)strlen(filename) - dir_len;

      cur_len = iupArrayCount(names_array);

      all_names = iupArrayAdd(names_array, len + 1);
      memcpy(all_names + cur_len, filename + dir_len, len);
      all_names[cur_len + len] = '|';

      iupAttribSetStrId(ih, "MULTIVALUE", count, filename + dir_len);
      count++;
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

static void eflFileDlgFreePathList(void)
{
  if (efl_filedlg_paths)
  {
    Eina_List* l;
    char* p;
    EINA_LIST_FOREACH(efl_filedlg_paths, l, p)
      free(p);
    eina_list_free(efl_filedlg_paths);
    efl_filedlg_paths = NULL;
  }
}

static void eflFileDlgDoneCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  const char* path = (const char*)event_info;
  int is_multi = iupAttribGetBoolean(ih, "MULTIPLEFILES");

  eflFileDlgFreePathList();

  if (path && path[0])
  {
    if (is_multi)
    {
      const Eina_List* selected = elm_fileselector_selected_paths_get(obj);
      if (selected && eina_list_count(selected) > 0)
      {
        const Eina_List* l;
        const char* sel_path;
        EINA_LIST_FOREACH(selected, l, sel_path)
          efl_filedlg_paths = eina_list_append(efl_filedlg_paths, strdup(sel_path));
      }
      else
      {
        efl_filedlg_paths = eina_list_append(NULL, strdup(path));
      }
    }
    else
    {
      efl_filedlg_paths = eina_list_append(NULL, strdup(path));
    }
    efl_filedlg_status = 1;
  }
  else
  {
    efl_filedlg_status = 0;
  }

  iupeflModalLoopQuit();
}

static void eflFileDlgSelectedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  const char* path = (const char*)event_info;
  IFnss file_cb;

  (void)obj;

  file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (!file_cb || !path)
    return;

  if (eflIsFile(path))
    file_cb(ih, iupeflStrConvertFromSystem((char*)path), "SELECT");
  else
    file_cb(ih, iupeflStrConvertFromSystem((char*)path), "OTHER");
}

static int eflFileDlgPopup(Ihandle* ih, int x, int y)
{
  Eo* win;
  Evas_Object* fileselector;
  char *dialogtype, *title, *file, *dir, *value;
  int is_save = 0;
  int folder_mode = 0;
  int is_multi = 0;
  int filter_count = 0;
  EflFileFilterData** filter_list = NULL;
  IFnss file_cb;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  title = iupAttribGet(ih, "TITLE");

  win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
    efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_DIALOG_BASIC),
    efl_text_set(efl_added, title ? iupeflStrConvertToSystem(title) : ""));
  if (!win)
    return IUP_ERROR;

  fileselector = elm_fileselector_add(win);
  if (!fileselector)
  {
    efl_del(win);
    return IUP_ERROR;
  }

  dialogtype = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(dialogtype, "SAVE"))
  {
    is_save = 1;
    elm_fileselector_is_save_set(fileselector, EINA_TRUE);
  }
  else if (iupStrEqualNoCase(dialogtype, "DIR"))
  {
    folder_mode = 1;
    elm_fileselector_folder_only_set(fileselector, EINA_TRUE);
  }
  else
  {
    elm_fileselector_is_save_set(fileselector, EINA_FALSE);
  }

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && !is_save && !folder_mode)
  {
    is_multi = 1;
    elm_fileselector_multi_select_set(fileselector, EINA_TRUE);
  }

  elm_fileselector_expandable_set(fileselector, EINA_TRUE);
  elm_fileselector_buttons_ok_cancel_set(fileselector, EINA_TRUE);

  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
    elm_fileselector_hidden_visible_set(fileselector, EINA_TRUE);

  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
  {
    char* filters = iupStrDup(value);
    char* name;
    int i;

    filter_count = iupStrReplace(filters, '|', 0) / 2;
    if (filter_count > 0)
    {
      filter_list = (EflFileFilterData**)malloc(filter_count * sizeof(EflFileFilterData*));

      name = filters;
      for (i = 0; i < filter_count && name[0]; i++)
      {
        char* pattern = eflFileDlgGetNextStr(name);

        filter_list[i] = eflFileDlgParsePatterns(pattern);

        elm_fileselector_custom_filter_append(fileselector,
          eflFileDlgFilterFunc, filter_list[i],
          iupeflStrConvertToSystem(name));

        name = eflFileDlgGetNextStr(pattern);
      }

      filter_count = i;
    }

    free(filters);
  }
  else
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
    {
      char* info = iupAttribGet(ih, "FILTERINFO");
      if (!info)
        info = value;

      filter_list = (EflFileFilterData**)malloc(sizeof(EflFileFilterData*));
      filter_list[0] = eflFileDlgParsePatterns(value);
      filter_count = 1;

      elm_fileselector_custom_filter_append(fileselector,
        eflFileDlgFilterFunc, filter_list[0],
        iupeflStrConvertToSystem(info));
    }
  }

  dir = iupAttribGet(ih, "DIRECTORY");
  if (dir && dir[0])
    elm_fileselector_path_set(fileselector, iupeflStrConvertToSystem(dir));
  else
  {
    const char* home = getenv("HOME");
    if (home)
      elm_fileselector_path_set(fileselector, home);
  }

  file = iupAttribGet(ih, "FILE");
  if (file && file[0] && is_save)
    elm_fileselector_current_name_set(fileselector, iupeflStrConvertToSystem(file));

  evas_object_smart_callback_add(fileselector, "done", eflFileDlgDoneCallback, ih);
  evas_object_smart_callback_add(fileselector, "selected", eflFileDlgSelectedCallback, ih);

  efl_gfx_hint_weight_set(fileselector, 1.0, 1.0);
  efl_gfx_hint_align_set(fileselector, EVAS_HINT_FILL, EVAS_HINT_FILL);

  efl_content_set(win, fileselector);
  efl_gfx_entity_size_set(win, EINA_SIZE2D(500, 450));
  efl_gfx_entity_visible_set(fileselector, EINA_TRUE);
  efl_gfx_entity_visible_set(win, EINA_TRUE);
  efl_ui_win_activate(win);

  file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (file_cb)
    file_cb(ih, NULL, "INIT");

  efl_filedlg_status = 0;
  eflFileDlgFreePathList();

  for (;;)
  {
    iupeflModalLoopRun();

    if (efl_filedlg_status == 1 && efl_filedlg_paths && is_save &&
        !iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT"))
    {
      char* filename = iupeflStrConvertFromSystem((char*)eina_list_data_get(efl_filedlg_paths));
      char* final_filename = eflFileCheckExt(ih, filename);

      if (eflIsFile(final_filename))
      {
        int overwrite;
        char msg[512];
        const char* basename;

        basename = strrchr(final_filename, '/');
        if (basename)
          basename++;
        else
          basename = final_filename;

        snprintf(msg, sizeof(msg), "%s\n%s",
          basename,
          IupGetLanguageString("IUP_FILEOVERWRITE"));

        overwrite = IupAlarm(IupGetLanguageString("IUP_SAVEAS"),
          msg,
          IupGetLanguageString("IUP_YES"),
          IupGetLanguageString("IUP_NO"),
          NULL);

        if (final_filename != filename)
          free(final_filename);

        if (overwrite != 1)
        {
          eflFileDlgFreePathList();
          efl_filedlg_status = 0;
          continue;
        }
      }
      else
      {
        if (final_filename != filename)
          free(final_filename);
      }
    }

    if (efl_filedlg_status == 1 && efl_filedlg_paths && file_cb)
    {
      char* filename = iupeflStrConvertFromSystem((char*)eina_list_data_get(efl_filedlg_paths));
      int ret = file_cb(ih, filename, "OK");

      if (ret == IUP_IGNORE || ret == IUP_CONTINUE)
      {
        eflFileDlgFreePathList();
        efl_filedlg_status = 0;
        continue;
      }
    }

    break;
  }

  if (efl_filedlg_status == 1 && efl_filedlg_paths)
  {
    if (folder_mode)
    {
      char* selected_dir = iupeflStrConvertFromSystem((char*)eina_list_data_get(efl_filedlg_paths));
      iupAttribSetStr(ih, "VALUE", selected_dir);
      iupAttribSet(ih, "STATUS", "0");
      iupAttribSet(ih, "FILEEXIST", NULL);
    }
    else if (is_multi)
    {
      eflFileDlgGetMultipleFiles(ih, efl_filedlg_paths);
      iupAttribSet(ih, "STATUS", "0");
      iupAttribSet(ih, "FILEEXIST", "YES");
    }
    else
    {
      char* filename = iupeflStrConvertFromSystem((char*)eina_list_data_get(efl_filedlg_paths));
      char* final_filename;
      int file_exist, dir_exist;

      final_filename = eflFileCheckExt(ih, filename);
      iupAttribSetStr(ih, "VALUE", final_filename);

      {
        char* dir_path = iupStrFileGetPath(final_filename);
        iupAttribSetStr(ih, "DIRECTORY", dir_path);
        free(dir_path);
      }

      file_exist = eflIsFile(final_filename);
      dir_exist = eflIsDirectory(final_filename);

      if (dir_exist)
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

      if (final_filename != filename)
        free(final_filename);
    }

    if (filter_count > 0)
      iupAttribSetInt(ih, "FILTERUSED", 1);

    if (!folder_mode && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    {
      char* cur_dir = iupAttribGet(ih, "DIRECTORY");
      if (cur_dir)
      {
        int ret = chdir(cur_dir);
        (void)ret;
      }
    }

    if (file_cb)
      file_cb(ih, NULL, "FINISH");
  }
  else
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "DIRECTORY", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  evas_object_smart_callback_del(fileselector, "done", eflFileDlgDoneCallback);
  evas_object_smart_callback_del(fileselector, "selected", eflFileDlgSelectedCallback);
  efl_del(win);
  eflFileDlgFreePathList();

  if (filter_list)
  {
    int i;
    for (i = 0; i < filter_count; i++)
      eflFileDlgFreeFilterData(filter_list[i]);
    free(filter_list);
  }

  return IUP_NOERROR;
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = eflFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
