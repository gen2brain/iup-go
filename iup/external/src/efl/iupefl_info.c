/** \file
 * \brief EFL System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <pwd.h>

#include "iup.h"
#include "iup_export.h"
#include "iup_str.h"
#include "iup_drvinfo.h"

#include "iupefl_drv.h"


IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  Ecore_Evas* ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);
  if (ee)
  {
    ecore_evas_screen_geometry_get(ee, NULL, NULL, width, height);
    ecore_evas_free(ee);
  }
  else
  {
    *width = 1920;
    *height = 1080;
  }
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  iupdrvGetScreenSize(width, height);
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  return 32;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  Ecore_Evas* ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);
  if (ee)
  {
    int dpi_x, dpi_y;
    ecore_evas_screen_dpi_get(ee, &dpi_x, &dpi_y);
    ecore_evas_free(ee);
    return (double)dpi_x;
  }
  return 96.0;
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  Eo* win = iupeflGetMainWindow();

  *x = 0;
  *y = 0;

  if (win)
  {
    Evas* evas = evas_object_evas_get(win);
    if (evas)
    {
      Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
      if (ee)
      {
        int wx, wy;
        ecore_evas_geometry_get(ee, &wx, &wy, NULL, NULL);
        ecore_evas_pointer_xy_get(ee, x, y);
        *x += wx;
        *y += wy;
      }
    }
  }
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  Eo* win;

  key[0] = ' ';
  key[1] = ' ';
  key[2] = ' ';
  key[3] = ' ';
  key[4] = 0;

  win = iupeflGetMainWindow();
  if (!win)
    return;

  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_SHIFT, NULL))
    key[0] = 'S';
  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_CONTROL, NULL))
    key[1] = 'C';
  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_ALT, NULL))
    key[2] = 'A';
  if (efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_META, NULL) ||
      efl_input_modifier_enabled_get(win, EFL_INPUT_MODIFIER_SUPER, NULL))
    key[3] = 'Y';
}

IUP_SDK_API char* iupdrvGetSystemName(void)
{
  struct utsname un;
  char* str = iupStrGetMemory(50);

  uname(&un);
  strcpy(str, un.sysname);

  return str;
}

IUP_SDK_API char* iupdrvGetSystemVersion(void)
{
  struct utsname un;
  char* str = iupStrGetMemory(100);

  uname(&un);
  strcpy(str, un.release);

  return str;
}

IUP_SDK_API char* iupdrvGetComputerName(void)
{
  char* str = iupStrGetMemory(100);

  gethostname(str, 100);

  return str;
}

IUP_SDK_API char* iupdrvGetUserName(void)
{
  char* str;
  struct passwd* pwd = getpwuid(getuid());

  if (!pwd)
    return NULL;

  str = iupStrGetMemory(100);
  strcpy(str, pwd->pw_name);

  return str;
}

IUP_SDK_API int iupdrvGetPreferencePath(char* filename, const char* app_name, int use_system)
{
  char* home;

  (void)use_system;

  home = getenv("HOME");
  if (!home)
    return 0;

  strcpy(filename, home);
  strcat(filename, "/.config/");

  mkdir(filename, S_IRWXU);

  if (app_name && app_name[0])
  {
    strcat(filename, app_name);
    strcat(filename, "/");
    mkdir(filename, S_IRWXU);
  }

  return 1;
}

IUP_SDK_API char* iupdrvLocaleInfo(void)
{
  char* locale = getenv("LC_ALL");
  if (!locale || !locale[0])
    locale = getenv("LC_MESSAGES");
  if (!locale || !locale[0])
    locale = getenv("LANG");

  if (locale && locale[0])
  {
    char* str = iupStrGetMemory(100);
    strcpy(str, locale);
    return str;
  }

  return NULL;
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
  return chdir(dir) == 0 ? 1 : 0;
}

IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  char* str = iupStrGetMemory(PATH_MAX);
  if (getcwd(str, PATH_MAX))
    return str;
  return NULL;
}

void* iupdrvGetDisplay(void)
{
  return NULL;
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  static int cached_size = 0;
  Eo* win;
  Evas* evas;
  Evas_Object* scroller;

  if (cached_size > 0)
    return cached_size;

  win = iupeflGetMainWindow();
  if (!win)
  {
    cached_size = 15;
    return cached_size;
  }

  evas = evas_object_evas_get(win);
  scroller = elm_scroller_add(win);
  if (scroller)
  {
    Evas_Object* content;
    int sw = 0, sh = 0, cw = 0, ch = 0;

    content = evas_object_rectangle_add(evas);
    evas_object_resize(content, 500, 500);
    elm_object_content_set(scroller, content);
    elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_ON, ELM_SCROLLER_POLICY_ON);
    evas_object_resize(scroller, 200, 200);
    evas_object_show(scroller);
    evas_smart_objects_calculate(evas);

    evas_object_geometry_get(scroller, NULL, NULL, &sw, &sh);
    elm_scroller_region_get(scroller, NULL, NULL, &cw, &ch);

    cached_size = sw - cw;
    if (cached_size <= 0)
      cached_size = 15;

    evas_object_del(scroller);
  }
  else
    cached_size = 15;

  return cached_size;
}

IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
  int options = LOG_CONS | LOG_PID;
  int priority = 0;

  if (iupStrEqualNoCase(type, "DEBUG"))
  {
    priority = LOG_DEBUG;
#ifdef LOG_PERROR
    options |= LOG_PERROR;
#endif
  }
  else if (iupStrEqualNoCase(type, "ERROR"))
    priority = LOG_ERR;
  else if (iupStrEqualNoCase(type, "WARNING"))
    priority = LOG_WARNING;
  else if (iupStrEqualNoCase(type, "INFO"))
    priority = LOG_INFO;

  openlog(NULL, options, LOG_USER);

  vsyslog(priority, format, arglist);

  closelog();
}

IUP_API void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
