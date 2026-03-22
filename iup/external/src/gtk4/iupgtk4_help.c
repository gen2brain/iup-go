/** \file
 * \brief GTK4 Driver IupHelp for non Windows systems
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include "iup.h"

#include "iup_str.h"


IUP_API int IupExecute(const char *filename, const char* parameters)
{
  GError *error = NULL;
  int ret = 1;
  size_t cmd_size = sizeof(char)*(strlen(filename) + strlen(parameters) + 3);
  char* cmd = (char*)malloc(cmd_size);
  snprintf(cmd, cmd_size, "%s %s", filename, parameters);

  if (!g_spawn_command_line_async(cmd, &error))
  {
    if (error && error->code == G_FILE_ERROR_NOENT)
      ret = -2;
    else
      ret = -1;
  }

  if (error)
    g_error_free(error);

  free(cmd);

  return ret;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  GError *error = NULL;
  int ret = 1;
  size_t cmd_size = sizeof(char)*(strlen(filename) + strlen(parameters) + 3);
  char* cmd = (char*)malloc(cmd_size);
  snprintf(cmd, cmd_size, "%s %s", filename, parameters);

  if (!g_spawn_command_line_sync(cmd, NULL, NULL, NULL, &error))
  {
    if (error && error->code == G_FILE_ERROR_NOENT)
      ret = -2;
    else
      ret = -1;
  }

  if (error)
    g_error_free(error);

  free(cmd);

  return ret;
}

IUP_API int IupHelp(const char* url)
{
  GError *error = NULL;
  char *browser;

  if (g_app_info_launch_default_for_uri(url, NULL, &error))
  {
    return 1;
  }

  if (error)
  {
    if (error->code == G_IO_ERROR_NOT_FOUND)
    {
      g_error_free(error);
      return -2;
    }
    g_error_free(error);
  }

  browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");

  if (!browser)
  {
    char* system = IupGetGlobal("SYSTEM");
    if (iupStrEqualNoCase(system, "macOS"))
      browser = "open";
    else
      browser = "xdg-open";
  }

  return IupExecute(browser, url);
}
