/** \file
 * \brief GTK Driver IupHelp for non Windows systems
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "iup.h"

#include "iup_str.h"


IUP_API int IupExecute(const char *filename, const char* parameters)
{
  GError *error = NULL;
  int ret = 1;
  char* cmd = (char*)malloc(sizeof(char)*(strlen(filename) + strlen(parameters) + 3));
  sprintf(cmd, "%s %s", filename, parameters);

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
  char* cmd = (char*)malloc(sizeof(char)*(strlen(filename) + strlen(parameters) + 3));
  sprintf(cmd, "%s %s", filename, parameters);

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
  char *browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");

  if (!browser)
  {
    char* system = IupGetGlobal("SYSTEM");
    if (iupStrEqualNoCase(system, "Linux") ||
        iupStrEqualNoCase(system, "FreeBSD"))
        browser = "firefox";
    else if (iupStrEqualNoCase(system, "MacOS"))
      browser = "safari";
    else if (iupStrEqualPartial(system, "CYGWIN"))
      browser = "iexplore";
    else
      browser = "netscape";
  }

  return IupExecute(browser, url);
}
