/** \file
 * \brief EFL Driver IupHelp for non Windows systems
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iup_str.h"

#include "iupefl_drv.h"


IUP_API int IupExecute(const char *filename, const char* parameters)
{
  int ret = 1;
  char* cmd;
  Eina_Bool success;

  cmd = (char*)malloc(sizeof(char)*(strlen(filename) + strlen(parameters) + 3));
  sprintf(cmd, "%s %s", filename, parameters);

  success = ecore_exe_run(cmd, NULL) != NULL;
  if (!success)
    ret = -1;

  free(cmd);

  return ret;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  int ret = 1;
  char* cmd;
  int exit_status;

  cmd = (char*)malloc(sizeof(char)*(strlen(filename) + strlen(parameters) + 3));
  sprintf(cmd, "%s %s", filename, parameters);

  exit_status = system(cmd);
  if (exit_status == -1)
    ret = -1;
  else if (exit_status != 0)
    ret = -1;

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
    char* system_name = IupGetGlobal("SYSTEM");
    if (iupStrEqualNoCase(system_name, "Linux") || iupStrEqualNoCase(system_name, "FreeBSD"))
        browser = "xdg-open";
    else if (iupStrEqualNoCase(system_name, "MacOS"))
      browser = "open";
    else
      browser = "firefox";
  }

  return IupExecute(browser, url);
}
