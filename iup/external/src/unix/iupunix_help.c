/** \file
 * \brief Motif Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_str.h"

#include "iupunix_portal.h"

IUP_API int IupExecute(const char *filename, const char* parameters)
{
  int ret;
  if (parameters)
  {
    size_t cmd_size = sizeof(char)*(strlen(filename) + strlen(parameters) + 3);
    char* cmd = (char*)malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s %s &", filename, parameters);
    ret = system(cmd);
    free(cmd);
  }
  else
  {
    size_t cmd_size = sizeof(char)*(strlen(filename) + 3);
    char* cmd = (char*)malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s &", filename);
    ret = system(cmd);
    free(cmd);
  }
  if (ret == -1)
    return -1;
  return 1;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  int ret;
  if (parameters)
  {
    size_t cmd_size = sizeof(char)*(strlen(filename) + strlen(parameters) + 3);
    char* cmd = (char*)malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s %s", filename, parameters);
    ret = system(cmd);
    free(cmd);
  }
  else
    ret = system(filename);
  if (ret == -1)
    return -1;
  return 1;
}

IUP_API int IupHelp(const char *url)
{
  char *browser;

  if (iupUnixPortalHelp(url) == 1)
    return 1;

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
