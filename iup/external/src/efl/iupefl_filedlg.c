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

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_predialogs.h"
#include "iup_array.h"

#include "iupefl_drv.h"


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

static int eflFileDlgPopup(Ihandle* ih, int x, int y)
{
  Eo* win;
  Evas_Object* fileselector;
  char *dialogtype, *title, *file, *dir;
  int is_save = 0;
  int folder_mode = 0;
  int is_multi = 0;

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

  efl_gfx_hint_weight_set(fileselector, 1.0, 1.0);
  efl_gfx_hint_align_set(fileselector, EVAS_HINT_FILL, EVAS_HINT_FILL);

  efl_content_set(win, fileselector);
  efl_gfx_entity_size_set(win, EINA_SIZE2D(500, 450));
  efl_gfx_entity_visible_set(fileselector, EINA_TRUE);
  efl_gfx_entity_visible_set(win, EINA_TRUE);
  efl_ui_win_activate(win);

  efl_filedlg_status = 0;
  eflFileDlgFreePathList();

  iupeflModalLoopRun();

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

    if (!folder_mode && !iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    {
      char* cur_dir = iupAttribGet(ih, "DIRECTORY");
      if (cur_dir)
      {
        int ret = chdir(cur_dir);
        (void)ret;
      }
    }
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
  efl_del(win);
  eflFileDlgFreePathList();

  return IUP_NOERROR;
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = eflFileDlgPopup;
}
