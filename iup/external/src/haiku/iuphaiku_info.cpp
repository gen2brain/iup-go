/** \file
 * \brief Haiku System Info
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <unistd.h>

#include <Application.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Path.h>
#include <Point.h>
#include <Screen.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_singleinstance.h"
}


extern "C" IUP_SDK_API int iupdrvScaleNaturalPx(int natural)
{
  return natural;
}

extern "C" IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  (void)x;
  (void)y;
  (void)add;
}

extern "C" IUP_SDK_API void iupdrvGetScreenSize(int *w, int *h)
{
  BScreen screen;
  BRect frame = screen.Frame();
  if (w) *w = frame.IntegerWidth() + 1;
  if (h) *h = frame.IntegerHeight() + 1;
}

extern "C" IUP_SDK_API void iupdrvGetFullSize(int *w, int *h)
{
  iupdrvGetScreenSize(w, h);
}

extern "C" IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  BScreen screen;
  switch (screen.ColorSpace())
  {
    case B_RGB32: case B_RGBA32: return 32;
    case B_RGB24: return 24;
    case B_RGB16: return 16;
    case B_RGB15: case B_RGBA15: return 15;
    case B_CMAP8: return 8;
    default: return 32;
  }
}

extern "C" IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  return 96.0;
}

extern "C" IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  BPoint where;
  uint32 buttons = 0;
  get_mouse(&where, &buttons);
  if (x) *x = (int)where.x;
  if (y) *y = (int)where.y;
}

extern "C" IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  if (!key) return;
  uint32 mods = ::modifiers();
  key[0] = (mods & B_SHIFT_KEY)   ? 'S' : ' ';
  key[1] = (mods & B_CONTROL_KEY) ? 'C' : ' ';
  key[2] = (mods & B_OPTION_KEY)  ? 'A' : ' ';
  key[3] = (mods & B_COMMAND_KEY) ? 'Y' : ' ';
  key[4] = 0;
}

extern "C" IUP_SDK_API char* iupdrvGetSystemName(void)
{
  return (char*)"Haiku";
}

extern "C" IUP_SDK_API char* iupdrvGetSystemVersion(void)
{
  static char version[64];
  utsname u;
  if (uname(&u) == 0)
  {
    snprintf(version, sizeof(version), "%s", u.release);
    return version;
  }
  return (char*)"R1";
}

extern "C" IUP_SDK_API char* iupdrvGetComputerName(void)
{
  static char name[256];
  if (gethostname(name, sizeof(name)) == 0)
    return name;
  return (char*)"";
}

extern "C" IUP_SDK_API char* iupdrvGetUserName(void)
{
  passwd* pw = getpwuid(getuid());
  if (pw && pw->pw_name)
    return pw->pw_name;
  return (char*)"";
}

extern "C" IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  static char dir[4096];
  if (getcwd(dir, sizeof(dir))) return dir;
  return (char*)"";
}

extern "C" IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
  if (!dir) return 0;
  return chdir(dir) == 0 ? 1 : 0;
}

extern "C" IUP_SDK_API int iupdrvGetUserDir(char *path, int size, int kind)
{
  if (!path || size <= 0) return 0;
  directory_which which = B_USER_DIRECTORY;
  switch (kind)
  {
    case IUP_USER_DIR_CACHE:  which = B_USER_CACHE_DIRECTORY; break;
    case IUP_USER_DIR_DATA:   which = B_USER_DATA_DIRECTORY; break;
    case IUP_USER_DIR_CONFIG: which = B_USER_SETTINGS_DIRECTORY; break;
    case IUP_USER_DIR_TEMP:   which = B_SYSTEM_TEMP_DIRECTORY; break;
  }
  BPath p;
  if (find_directory(which, &p) != B_OK)
    return 0;
  snprintf(path, size, "%s", p.Path());
  return 1;
}

extern "C" IUP_SDK_API int iupdrvGetPreferencePath(char *filename, const char *app_name, int use_system)
{
  if (!filename) return 0;
  if (!app_name || !app_name[0]) { filename[0] = '\0'; return 0; }

  if (use_system)
  {
    BPath p;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) != B_OK) { filename[0] = '\0'; return 0; }
    snprintf(filename, 10240, "%s/%s", p.Path(), app_name);
    mkdir(filename, 0755);
    snprintf(filename + strlen(filename), 10240 - strlen(filename), "/config");
    return 1;
  }

  const char* home = getenv("HOME");
  if (!home) { filename[0] = '\0'; return 0; }
  snprintf(filename, 10240, "%s/.%s", home, app_name);
  return 1;
}

extern "C" IUP_SDK_API char* iupdrvLocaleInfo(void)
{
  return (char*)"UTF-8";
}

extern "C" IUP_SDK_API void* iupdrvGetDisplay(void)
{
  return NULL;
}

extern "C" IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
  int priority = LOG_INFO;
  int options = LOG_CONS | LOG_PID;

  if      (iupStrEqualNoCase(type, "DEBUG"))   priority = LOG_DEBUG;
  else if (iupStrEqualNoCase(type, "ERROR"))   priority = LOG_ERR;
  else if (iupStrEqualNoCase(type, "WARNING")) priority = LOG_WARNING;
  else if (iupStrEqualNoCase(type, "INFO"))    priority = LOG_INFO;

  openlog(NULL, options, LOG_USER);
  vsyslog(priority, format, arglist);
  closelog();
}

extern "C" IUP_API void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
