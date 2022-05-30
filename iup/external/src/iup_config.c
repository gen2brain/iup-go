/** \file
 * \brief Configuration file Utilities
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iup.h>
#include <iupcbs.h>

#include "iup_object.h"
#include "iup_config.h"
#include "iup_linefile.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_assert.h"

#include "iup_drvinfo.h"

#if defined(__APPLE__)
#import <TargetConditionals.h>
#endif

#define GROUPKEYSIZE 100
#define MAX_LINES 500


static char* strGetGroupKeyName(const char* group, const char* key)
{
  static char str[GROUPKEYSIZE];
  strcpy(str, group);
  strcat(str, ".");
  strcat(str, key);
  return str;
}

static char* strSetGroupKeyName(const char* groupkey, char* group)
{
  int len;
  const char* key = strchr(groupkey, '.');
  if (!key) return NULL;
  len = (int)(key - groupkey);
  memcpy(group, groupkey, len);
  group[len] = 0;
  return (char*)(key + 1);
}

IUP_API Ihandle* IupConfig(void)
{
  return IupUser();
}

static char* iConfigSetFilename(Ihandle* ih)
{
  char* app_name;
  char* app_path;
  int app_config, app_system;
  
  char filename[10240] = "";
  char* app_filename = IupGetAttribute(ih, "APP_FILENAME");
  if (app_filename)
    return app_filename;

  app_name = IupGetAttribute(ih, "APP_NAME");
  app_path = IupGetAttribute(ih, "APP_PATH");
  app_config = IupGetInt(ih, "APP_CONFIG");
  app_system = IupGetInt(ih, "APP_SYSTEMPATH");

  if (!app_name)
    return NULL;

  if (!app_config && iupdrvGetPreferencePath(filename, app_system))
  {
#if defined(__ANDROID__) || defined(__APPLE__) || defined(WIN32) || defined(__EMSCRIPTEN__)
    strcat(filename, app_name);
    strcat(filename, ".cfg");
#else
    /* UNIX format */
    strcat(filename, ".");
    strcat(filename, app_name);
#endif
  }
  else
  {
    if (!app_path)
      return NULL;

    strcat(filename, app_path);
#if defined(__ANDROID__) || defined(__APPLE__) || defined(WIN32) || defined(__EMSCRIPTEN__)
    /* these platforms shouldn't use a .dot file */
#else
    /* Unix generic hidden dot prefix */
    strcat(filename, ".");
#endif
    strcat(filename, app_name);
#if defined(__ANDROID__) || defined(__APPLE__) || defined(WIN32) || defined(__EMSCRIPTEN__)
    strcat(filename, ".cfg");
#endif
  }

  IupSetStrAttribute(ih, "FILENAME", filename);

  return IupGetAttribute(ih, "FILENAME");
}

static int sort_names_cb(const void* elem1, const void* elem2)
{
  char* str1 = *((char**)elem1);
  char* str2 = *((char**)elem2);
  return strcmp(str1, str2);
}

IUP_API int IupConfigLoad(Ihandle* ih)
{
  char group[GROUPKEYSIZE] = "";
  char key[GROUPKEYSIZE];
  IlineFile* line_file;
  
  char* filename = iConfigSetFilename(ih);
  if (!filename)
    return -3;

  line_file = iupLineFileOpen(filename);
  if (!line_file)
    return -1;

  do
  {
    const char* line_buffer;

    int line_len = iupLineFileReadLine(line_file);
    if (line_len == -1)
    {
      iupLineFileClose(line_file);
      return -2;
    }

    line_buffer = iupLineFileGetBuffer(line_file);

    if (line_buffer[0] == 0)  /* skip empty line */
      continue;

    if (line_buffer[0] == '#')  /* "#" signifies a comment line */
      continue;

    if (line_buffer[0] == '[')  /* group start */
    {
      group[0] = 0;
      sscanf(line_buffer, "[%[^]]s]", group);
    }
    else
    {
      const char* value;

      key[0] = 0;
      sscanf(line_buffer, "%[^=]s", key);

      value = strstr(line_buffer, "=");
      if (!value)                    
        value = line_buffer;
      else
        value++;  /* Skip '=' */

      IupConfigSetVariableStr(ih, group, key, value);
    }
  } while (!iupLineFileEOF(line_file));

  iupLineFileClose(line_file);
  return 0;
}

IUP_API int IupConfigSave(Ihandle* ih)
{
  char* names[MAX_LINES];
  FILE* file;
  int i, count;
  char last_group[GROUPKEYSIZE] = "", group[GROUPKEYSIZE], *key;
  
  char* filename = iConfigSetFilename(ih);
  if (!filename)
    return -3;

  file = fopen(filename, "w");
  if (!file)
    return -1;

  count = IupGetAllAttributes(ih, names, MAX_LINES);

  qsort(names, count, sizeof(char*), sort_names_cb);

  for (i = 0; i<count; i++)
  {
    char* value;
    
    key = strSetGroupKeyName(names[i], group);
    if (!key)
      continue;

    if (!iupStrEqual(group, last_group))
    {
      /* write a new group */
      fprintf(file, "\n[%s]\n", group);
      strcpy(last_group, group);
    }

    value = IupGetAttribute(ih, names[i]);
    fprintf(file, "%s=%s\n", key, value);

    if (ferror(file))
    {
      fclose(file);
      return -2;
    }
  }

  fclose(file);
#if defined(__EMSCRIPTEN__)
  EM_ASM(
	FS.syncfs(function(err) {
        if(err) console.log('Error: FS.syncfs failed', err);
	  }
	);
  );
#endif
  return 0;
}

IUP_API void IupConfigSetVariableStrId(Ihandle* ih, const char* group, const char* key, int id, const char* value)
{
  char key_id[GROUPKEYSIZE];
  sprintf(key_id, "%s%d", key, id);
  IupConfigSetVariableStr(ih, group, key_id, value);
}

IUP_API void IupConfigSetVariableIntId(Ihandle* ih, const char* group, const char* key, int id, int value)
{
  char key_id[GROUPKEYSIZE];
  sprintf(key_id, "%s%d", key, id);
  IupConfigSetVariableInt(ih, group, key_id, value);
}

IUP_API void IupConfigSetVariableDoubleId(Ihandle* ih, const char* group, const char* key, int id, double value)
{
  char key_id[GROUPKEYSIZE];
  sprintf(key_id, "%s%d", key, id);
  IupConfigSetVariableDouble(ih, group, key_id, value);
}

IUP_API void IupConfigSetVariableStr(Ihandle* ih, const char* group, const char* key, const char* value)
{
  IupSetStrAttribute(ih, strGetGroupKeyName(group, key), value);
}

IUP_API void IupConfigSetVariableInt(Ihandle* ih, const char* group, const char* key, int value)
{
  IupSetInt(ih, strGetGroupKeyName(group, key), value);
}

IUP_API void IupConfigSetVariableDouble(Ihandle* ih, const char* group, const char* key, double value)
{
  IupSetDouble(ih, strGetGroupKeyName(group, key), value);
}

IUP_API const char* IupConfigGetVariableStr(Ihandle* ih, const char* group, const char* key)
{
  return IupGetAttribute(ih, strGetGroupKeyName(group, key));
}

IUP_API int IupConfigGetVariableInt(Ihandle* ih, const char* group, const char* key)
{
  return IupGetInt(ih, strGetGroupKeyName(group, key));
}

IUP_API double IupConfigGetVariableDouble(Ihandle* ih, const char* group, const char* key)
{
  return IupGetDouble(ih, strGetGroupKeyName(group, key));
}

IUP_API const char* IupConfigGetVariableStrDef(Ihandle* ih, const char* group, const char* key, const char* def)
{
  if (!IupGetAttribute(ih, strGetGroupKeyName(group, key)))
    return def;
  else
    return IupConfigGetVariableStr(ih, group, key);
}

IUP_API const char* IupConfigGetVariableStrIdDef(Ihandle* ih, const char* group, const char* key, int id, const char* def)
{
  if (!IupGetAttributeId(ih, strGetGroupKeyName(group, key), id))
    return def;
  else
    return IupConfigGetVariableStrId(ih, group, key, id);
}

IUP_API int IupConfigGetVariableIntDef(Ihandle* ih, const char* group, const char* key, int def)
{
  if (!IupGetAttribute(ih, strGetGroupKeyName(group, key)))
    return def;
  else
    return IupConfigGetVariableInt(ih, group, key);
}

IUP_API int IupConfigGetVariableIntIdDef(Ihandle* ih, const char* group, const char* key, int id, int def)
{
  if (!IupGetAttributeId(ih, strGetGroupKeyName(group, key), id))
    return def;
  else
    return IupConfigGetVariableIntId(ih, group, key, id);
}

IUP_API double IupConfigGetVariableDoubleDef(Ihandle* ih, const char* group, const char* key, double def)
{
  if (!IupGetAttribute(ih, strGetGroupKeyName(group, key)))
    return def;
  else
    return IupConfigGetVariableDouble(ih, group, key);
}

IUP_API double IupConfigGetVariableDoubleIdDef(Ihandle* ih, const char* group, const char* key, int id, double def)
{
  if (!IupGetAttributeId(ih, strGetGroupKeyName(group, key), id))
    return def;
  else
    return IupConfigGetVariableDoubleId(ih, group, key, id);
}

IUP_API const char* IupConfigGetVariableStrId(Ihandle* ih, const char* group, const char* key, int id)
{
  char key_id[GROUPKEYSIZE];
  sprintf(key_id, "%s%d", key, id);
  return IupConfigGetVariableStr(ih, group, key_id);
}

IUP_API int IupConfigGetVariableIntId(Ihandle* ih, const char* group, const char* key, int id)
{
  char key_id[GROUPKEYSIZE];
  sprintf(key_id, "%s%d", key, id);
  return IupConfigGetVariableInt(ih, group, key_id);
}

IUP_API double IupConfigGetVariableDoubleId(Ihandle* ih, const char* group, const char* key, int id)
{
  char key_id[GROUPKEYSIZE];
  sprintf(key_id, "%s%d", key, id);
  return IupConfigGetVariableDouble(ih, group, key_id);
}

IUP_API void IupConfigCopy(Ihandle* ih1, Ihandle* ih2, const char* exclude_prefix)
{
  char *name;

  iupASSERT(iupObjectCheck(ih1));
  if (!iupObjectCheck(ih1))
    return;

  iupASSERT(iupObjectCheck(ih2));
  if (!iupObjectCheck(ih2))
    return;

  name = iupTableFirst(ih1->attrib);
  while (name)
  {
    if (!iupATTRIB_ISINTERNAL(name) && !iupStrEqualPartial(name, exclude_prefix))
      iupTableSet(ih2->attrib, name, iupTableGet(ih1->attrib, name), IUPTABLE_STRING);

    name = iupTableNext(ih1->attrib);
  }
}

/******************************************************************/


IUP_API void IupConfigSetListVariable(Ihandle* ih, const char *group, const char* key, const char* value, int add)
{
  const char* value_id;
  int last_id, found_id = 0;

  /* First search for the value in the list */
  int i = 1;
  do
  {
    value_id = IupConfigGetVariableStrId(ih, group, key, i);

    if (value_id && iupStrEqual(value_id, value))
    {
      found_id = i;
      if (add)
        return; /* nothing to do */
      else
        break;  /* remove later */
    }

    i++;
  } while (value_id);

  last_id = i - 2;

  if (found_id)
  {
    /* remove found_id by moving last item to replace old item */
    value_id = IupConfigGetVariableStrId(ih, group, key, last_id);
    IupConfigSetVariableStrId(ih, group, key, found_id, value_id);
    IupConfigSetVariableStrId(ih, group, key, last_id, NULL);
  }
  else
  {
    if (add)
    {
      /* add new item at the end */
      IupConfigSetVariableStrId(ih, group, key, last_id + 1, value);
    }
    /* if remove, nothing to do */
  }
}


/******************************************************************/

/* macOS/Cocoa needs a completely different implementation, so exclude Mac/Cocoa from compiling this. */
#if !defined(__APPLE__) || !defined(TARGET_OS_OSX)

static const char* iConfigGetRecentAttribName(const char* recent_name, const char* base_name)
{
  if (recent_name)
  {
    static char name[100];
    sprintf(name, "%s%s", recent_name, base_name);
    return name;
  }
  else
    return base_name;
}

static int iConfigItemRecent_CB(Ihandle* ih_item)
{
  Icallback recent_cb = IupGetCallback(ih_item, "RECENT_CB");
  if (recent_cb)
  {
    Ihandle* ih = (Ihandle*)IupGetAttribute(ih_item, "_IUP_CONFIG");
    IupSetStrAttribute(ih, "TITLE", IupGetAttribute(ih_item, "RECENTFILENAME"));  /* backward compatibility */
    ih->parent = ih_item;

    recent_cb(ih);

    ih->parent = NULL;
    IupSetAttribute(ih, "TITLE", NULL);
  }
  return IUP_DEFAULT;
}

static int iConfigListRecent_CB(Ihandle* list, char *text, int item, int state)
{
  (void)item;

  if (state == 1)
  {
    Ihandle* drop_button = IupGetAttributeHandle(IupGetDialog(list), "DROPBUTTON");
    Icallback recent_cb = IupGetCallback(list, "RECENT_CB");

    if (recent_cb)
    {
      Ihandle* ih = (Ihandle*)IupGetAttribute(list, "_IUP_CONFIG");
      IupSetStrAttribute(ih, "RECENTFILENAME", text);
      ih->parent = list;

      recent_cb(ih);

      ih->parent = NULL;
      IupSetAttribute(ih, "RECENTFILENAME", NULL);
    }

    if (drop_button)
      IupSetAttribute(drop_button, "SHOWDROPDOWN", "NO");
  }
  return IUP_DEFAULT;
}

static void iConfigBuildRecentMenu(Ihandle* ih, Ihandle* menu, int max_recent, const char* group_name, Icallback recent_cb)
{
  /* add the new items, reusing old ones */
  int i;
  int mapped = IupGetAttribute(menu, "WID") != NULL ? 1 : 0;
  const char* value;

  i = 1;
  do
  {
    value = IupConfigGetVariableStrId(ih, group_name, "File", i);
    if (value)
    {
      Ihandle* item = IupGetChild(menu, i - 1);
      if (item)
        IupSetStrAttribute(item, "TITLE", value);
      else
      {
        item = IupItem(value, NULL);
        IupSetAttribute(item, "_IUP_CONFIG", (char*)ih);
        IupSetCallback(item, "ACTION", iConfigItemRecent_CB);
        IupSetCallback(item, "RECENT_CB", recent_cb);
        IupAppend(menu, item);
        if (mapped) IupMap(item);
      }

      IupSetStrAttribute(item, "RECENTFILENAME", value); /* TITLE will convert the string to a native representation, so RECENTFILENAME will keep the original value */
    }
    i++;
  } while (value && i <= max_recent);
}

static void iConfigBuildRecentList(Ihandle* ih, Ihandle* list, int max_recent, const char* group_name, Icallback recent_cb)
{
  /* add the new items, reusing old ones */
  int i;
  const char* value;

  IupSetAttribute(list, "1", NULL);

  i = 1;
  do
  {
    value = IupConfigGetVariableStrId(ih, group_name, "File", i);
    if (value)
      IupSetStrAttributeId(list, "", i, value);
    i++;
  } while (value && i <= max_recent);


  IupSetCallback(list, "RECENT_CB", recent_cb);
  IupSetAttribute(list, "_IUP_CONFIG", (char*)ih);

  if (iupStrEqual(IupGetClassName(list), "flatlist"))
    IupSetCallback(list, "FLAT_ACTION", (Icallback)iConfigListRecent_CB);
  else
    IupSetCallback(list, "ACTION", (Icallback)iConfigListRecent_CB);
}

IUP_API void IupConfigRecentInit(Ihandle* ih, Ihandle* menu_list, Icallback recent_cb, int max_recent)
{

  char* recent_name = IupGetAttribute(ih, "RECENTNAME");
  const char* group_name = recent_name;
  if (!group_name) group_name = "Recent";

  IupSetCallback(ih, iConfigGetRecentAttribName(recent_name, "RECENT_CB"), recent_cb);
  IupSetInt(ih, iConfigGetRecentAttribName(recent_name, "RECENTMAX"), max_recent);

  if (iupStrEqual(IupGetClassName(menu_list), "menu"))
  {
    IupSetAttribute(ih, iConfigGetRecentAttribName(recent_name, "RECENTMENU"), (char*)menu_list);
    iConfigBuildRecentMenu(ih, menu_list, max_recent, group_name, recent_cb);
  }
  else
  {
    IupSetAttribute(ih, iConfigGetRecentAttribName(recent_name, "RECENTLIST"), (char*)menu_list);
    iConfigBuildRecentList(ih, menu_list, max_recent, group_name, recent_cb);
  }
}

IUP_API void IupConfigRecentUpdate(Ihandle* ih, const char* filename)
{
  const char* value;
  char* recent_name = IupGetAttribute(ih, "RECENTNAME");
  Ihandle* menu = (Ihandle*)IupGetAttribute(ih, iConfigGetRecentAttribName(recent_name, "RECENTMENU"));
  Ihandle* list = (Ihandle*)IupGetAttribute(ih, iConfigGetRecentAttribName(recent_name, "RECENTLIST"));
  Icallback recent_cb = IupGetCallback(ih, iConfigGetRecentAttribName(recent_name, "RECENT_CB"));
  int max_recent = IupGetInt(ih, iConfigGetRecentAttribName(recent_name, "RECENTMAX"));
  const char* group_name = recent_name;
  if (!group_name) group_name = "Recent";

  value = IupConfigGetVariableStr(ih, group_name, "File1");
  if (value && !iupStrEqual(value, filename))
  {
    /* must update the stack */
    int found = 0;

    /* First search for the new filename to avoid duplicates */
    int i = 1;
    do
    {
      value = IupConfigGetVariableStrId(ih, group_name, "File", i);

      if (value && iupStrEqual(value, filename))
      {
        found = i;
        break;
      }

      i++;
    } while (value && i <= max_recent);

    if (found)
      i = found;
    else
      i = max_recent;

    /* simply open space for the new filename */
    do
    {
      value = IupConfigGetVariableStrId(ih, group_name, "File", i - 1);
      IupConfigSetVariableStrId(ih, group_name, "File", i, value);

      i--;
    } while (i > 1);
  }

  /* push new at start always */
  IupConfigSetVariableStr(ih, group_name, "File1", filename);

  if (menu)
    iConfigBuildRecentMenu(ih, menu, max_recent, group_name, recent_cb);
  else
    iConfigBuildRecentList(ih, list, max_recent, group_name, recent_cb);
}
#else

/* NOT supported in MacOS for now */

IUP_API void IupConfigRecentInit(Ihandle* ih, Ihandle* menu_list, Icallback recent_cb, int max_recent)
{
}

IUP_API void IupConfigRecentUpdate(Ihandle* ih, const char* filename)
{
}

#endif /* macOS/Cocoa */


/*******************************************************************/


IUP_API void IupConfigDialogShow(Ihandle* ih, Ihandle* dialog, const char* name)
{
  int shown = 0;
  int set_size = 0;

  /* does nothing if already visible */
  if (IupGetInt(dialog, "VISIBLE"))
  {
    IupShow(dialog);
    return;
  }

  /* set size only if dialog is resizable */
  if (IupGetInt(dialog, "RESIZE"))
  {
    /* dialog size from Config */
    int width = IupConfigGetVariableInt(ih, name, "Width");
    int height = IupConfigGetVariableInt(ih, name, "Height");
    if (width != 0 || height != 0)
    {
      int screen_width = 0, screen_height = 0;
      IupGetIntInt(NULL, "SCREENSIZE", &screen_width, &screen_height);
      if (width == 0)
        width = screen_width;
      if (height == 0)
        height = screen_height;

#ifdef WIN32
      IupSetfAttribute(dialog, "RASTERSIZE", "%dx%d", width, height);
#else
      IupMap(dialog);
      IupSetfAttribute(dialog, "CLIENTSIZE", "%dx%d", width, height);
#endif
      set_size = 1;
    }
  }

  if (IupConfigGetVariableIntDef(ih, name, "Maximized", 1) && IupGetInt(dialog, "RESIZE"))
  {
    IupSetAttribute(dialog, "PLACEMENT", "MAXIMIZED");
  }
  else
  {
    /* dialog position from Config */
    if (IupConfigGetVariableStr(ih, name, "X"))
    {
      int x = IupConfigGetVariableInt(ih, name, "X");
      int y = IupConfigGetVariableInt(ih, name, "Y");
      int virtual_x, virtual_y, virtual_w, virtual_h;
      int monitors_count = IupGetInt(NULL, "MONITORSCOUNT");
      if (monitors_count > 1)
      {
        char* virtual_screen = IupGetGlobal("VIRTUALSCREEN");
        sscanf(virtual_screen, "%d %d %d %d", &virtual_x, &virtual_y, &virtual_w, &virtual_h);
      }
      else
      {
        virtual_x = 0;
        virtual_y = 0;
        IupGetIntInt(NULL, "SCREENSIZE", &virtual_w, &virtual_h);
      }

      if (x < virtual_x) x = virtual_x;
      if (y < virtual_y) y = virtual_y;
      if (x > virtual_x + virtual_w) x = virtual_x;
      if (y > virtual_y + virtual_h) y = virtual_y;

      IupShowXY(dialog, x, y);
      shown = 1;
    }
  }

  if (!shown)
    IupShow(dialog);

  if (set_size)
    IupSetAttribute(dialog, "USERSIZE", NULL);  /* clear minimum restriction without reseting the current size */
}

IUP_API void IupConfigDialogClosed(Ihandle* ih, Ihandle* dialog, const char* name)
{
  int x = 0, y = 0;

  if (dialog->handle)
  {
    IupGetIntInt(dialog, "SCREENPOSITION", &x, &y);
    IupConfigSetVariableInt(ih, name, "X", x);
    IupConfigSetVariableInt(ih, name, "Y", y);
  }

  /* save size only if dialog is resizable */
  if (IupGetInt(dialog, "RESIZE"))
  {
    int width, height;
    int maximized;
    int screen_width = 0, screen_height = 0;

#ifdef WIN32
    IupGetIntInt(dialog, "RASTERSIZE", &width, &height);
#else
    IupGetIntInt(dialog, "CLIENTSIZE", &width, &height);
#endif
    IupConfigSetVariableInt(ih, name, "Width", width);
    IupConfigSetVariableInt(ih, name, "Height", height);

    maximized = IupGetInt(dialog, "MAXIMIZED"); /* Windows Only */
    IupGetIntInt(NULL, "SCREENSIZE", &screen_width, &screen_height);
    if (maximized ||
        ((x < 0 && (2 * x + width == screen_width)) &&    /* Works only for the main screen */
        (y < 0 && (2 * y + height == screen_height))))
        IupConfigSetVariableInt(ih, name, "Maximized", 1);
    else
      IupConfigSetVariableInt(ih, name, "Maximized", 0);
  }
}
